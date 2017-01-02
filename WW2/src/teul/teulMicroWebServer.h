// Created: Tue Nov 22 13:41:59 CST 2011
// Author: Oliver "kfsone" Smith <osmith@playnet.com>

#pragma once

#include <atomic>
#include <shared_mutex>
#include "wwii/wwiiHash.h"
#include "include/pnThread.h"

namespace Teul
{
	namespace uWeb
    {
        struct HTTPRequest
        {
            std::string method; // ASCII
            std::string path; // ASCII
            std::string version; // ASCII
            std::unordered_map<std::string, std::string> headers; // ASCII
            std::vector<std::string> orderedHeaders; // ASCII
            std::string requestData; // raw byte stream
        };
        
        struct HTTPResponse
        {
            std::string version; // ASCII
            int responseCode;
            std::string statusMessage; // ASCII
            std::vector<std::string> orderedHeaders; // ASCII
            std::string responseData; // raw byte stream (probably UTF-8)
            std::string responseVirtualFilePath; // use sendfile()
        };
        
        //! @internal
        // Because Client is here.
        class HTTPRequestHeaderReader;
        class HTTPRequestBodyReader;
        class HTTPRequestParser
        {
            public:
                HTTPRequestParser();
            
                const HTTPRequest &getRequest() const { return m_request; }
            
                bool parseMoreData(const std::string &data); // Return true if parse resulted in a complete request, pass empty data for EOF, throw exception on error
                size_t getBytesUnused() const;
                void reset();

            private:
                std::string::size_type m_bytesSoFar;
                HTTPRequest m_request;
                std::unique_ptr<HTTPRequestHeaderReader> m_headersParser;
                std::unique_ptr<HTTPRequestBodyReader> m_bodyParser;
                enum class State : uint8_t {
                    Ready,
                    ParsingHeaders,
                    ParsingBody,
                    Complete,
                };
                State m_state;
        };

        // The choice of std::list<> here is carefully considered:
        // - To make the unregister call below work, a container which does not
        //   invalidate iterators on erasure of other elements is required. Only
        //   list<> and forward_list<> do this.
        // - forward_list<> meets the required semantics, but elements may only
        //   be erased using the iterator to the *preceeding* element. This
        //   would compromise the ability for a Client to track its own iterator
        //   by requiring it to track another Client's lifetime, negating the
        //   gains of forward_list<>.
        class Client;
        using ClientList = std::list<std::unique_ptr<Client>>;
        
        //! @internal
        //! std::default_delete<> requires a complete type.
        class Server;
        class Client : public PNThread
        {
            public:
                Client(Server &owner, SOCKET fd);
            
                virtual ~Client();
            
                void start(ClientList::iterator &myIterator);
                
                virtual void run() override;
                virtual void onCancel() override;
            
            private:
                Server &m_owner;
                ClientList::iterator m_myIterator;
                std::atomic<SOCKET> m_clientSock;
                HTTPRequestParser m_requestParser;
                std::chrono::steady_clock::time_point m_currentRequestStart;
                std::string m_transferBuffer; // raw byte string
            
                void sendResponse(const HTTPResponse &response);
                bool pollSocket();
                size_t readData();
                void handleData(size_t nbytes);
        };
        
		class Server : public PNThread
		{
            public:
                using VirtualFileHandler = std::function<void(const HTTPRequest &/*request*/, HTTPResponse &/*response*/)>;

                Server() :
                    PNThread(),
                    m_rootPath(""), m_port(0), m_config_mutex(), m_listenSock(INVALID_SOCKET), m_resourceList("[]")
                {
                }

                const std::string& rootPath() const { return m_rootPath; }
                unsigned short port() const { return m_port; }

                void start(const char* const rootPath_, const unsigned short port_);

                virtual void run() override;
                virtual void onCancel() override;

                void addVirtualFile(std::string desc_, std::string path_, VirtualFileHandler handler_);

            private:
                using VirtualFileHandlers = std::unordered_map<std::string, std::pair<VirtualFileHandler /*handler*/, std::string /*description*/>>;

                std::string m_rootPath;
                unsigned short m_port;
                std::shared_timed_mutex m_config_mutex;
                std::atomic<SOCKET> m_listenSock;
                VirtualFileHandlers m_virtualFileHandlers;
                std::string m_resourceList;
            
                void indexHandler(const HTTPRequest &request, HTTPResponse &response);
                void physicalHandler(const HTTPRequest &request, HTTPResponse &response);
            
                // Client-only API
                friend class Client;
            
                std::mutex m_clientList_mutex;
                ClientList m_clientList;
                ClientList m_defunctClientList;
                void reap();
                void reap_unlocked();
            
                void client_unregisterClient(ClientList::iterator &client);
                VirtualFileHandler client_findHandlerForRequest(const HTTPRequest &request);
		};

        class HTTPResponseBuilder
        {
            public:
                HTTPResponseBuilder(); // always returns version 1.1 responses
                HTTPResponseBuilder(const HTTPRequest &originalRequest);
            
                HTTPResponseBuilder &status(int responseCode, const std::string &statusMessage = ""); // empty message = lookup in known response codes array
                HTTPResponseBuilder &header(const std::string &headerName, const std::string &headerValue, bool replace = true); // if replace == false, create another copy
                HTTPResponseBuilder &content(const std::string &data, bool suppressAutoContentLength = false, bool append = true);
            
                HTTPResponseBuilder &JSON(const std::string &JSONData); // set content-type etc. automatically
            
                HTTPResponseBuilder &file(const std::string &filePath, bool autodetectContentType = true);
            
                void build(HTTPResponse &result); // Moves - once build() is called, the builder is invalid
            
                const HTTPResponse &getResponse() const;
                HTTPResponse copyResponse(); // slow, copies lots of data
            
            private:
                HTTPResponse m_response;
        };
        
		extern Server s_debugServer;
	}
}

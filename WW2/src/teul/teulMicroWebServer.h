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
        
        class Server;
        
        /// @internal
        class HTTPRequestParser
        {
            public:
                HTTPRequestParser() : m_request(), m_parseBuffer(), m_parsePointer(0), m_knownContentLength(UINT64_MAX), m_seenHeaderTerminator(false) {}
                HTTPRequest getRequest() const { return m_request; }
                bool parseMoreData(const std::string &data); // Return true if parse resulted in a complete request, throw exception on error
                void validateRequest() const; // Throw exception on error
            private:
                HTTPRequest m_request;
                std::string m_parseBuffer; // raw byte stream
                std::string::size_type m_parsePointer;
                uint64_t m_knownContentLength;
                bool m_seenHeaderTerminator;
        };
        
        /// @internal
        class Client : public PNThread
        {
            public:
                Client(Server &owner, SOCKET fd) :
                    m_owner(owner), m_clientSock(fd)
                {}
            
                virtual ~Client();
            
                virtual void run() override;
                virtual void onCancel() override;
            
            private:
                Server &m_owner;
                std::atomic<SOCKET> m_clientSock;
                HTTPRequestParser m_requestParser;
                std::chrono::steady_clock::time_point m_requestStart;
            
                // API guarantee: A 404 response with no virtual file path will never throw an exception.
                void sendResponse(const HTTPResponse &response);
        };
    }
}
// I hate C++.
static inline bool operator==(const Teul::uWeb::Client &a, const Teul::uWeb::Client &b)
{
    return &a == &b;
}
namespace std {
    template <> struct hash<Teul::uWeb::Client> {
        typedef Teul::uWeb::Client argument_type;
        typedef std::size_t result_type;
        result_type operator()(argument_type const &a) const { return reinterpret_cast<size_t>(&a); }
    };
}
namespace Teul {
    namespace uWeb {
        
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
                std::unordered_set<Client> m_clientList;
            
                void client_unregisterClient(Client &client);
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

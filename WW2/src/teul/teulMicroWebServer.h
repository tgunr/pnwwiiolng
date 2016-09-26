#pragma once
// Created: Tue Nov 22 13:41:59 CST 2011
// Author: Oliver "kfsone" Smith <osmith@playnet.com>

#include "wwii/wwiiHash.h"
#include "include/pnThread.h"
#include <atomic>

namespace Teul
{
	namespace uWeb
	{
		//! @class Teul::uWeb::Server
		//! @brief Micro Web-server for providing
		class Server : public PNThread
		{
		public:
			typedef void (*VirtualFileHandler)(SOCKET /*fd*/, const char* /*path*/, const char* /*args*/);

			Server()
				: PNThread()
				, m_rootPath("")
				, m_port(0)
				, m_config_mutex()
				, m_listenSock(INVALID_SOCKET)
				, m_resourceList("[]")
				{
				}

			void start(const char* const rootPath_, const unsigned short port_)
			{
				ABORT_IF(isStarted());
				m_rootPath = rootPath_;
				m_port = port_;
				PNThread::start();
			}
			const std::string& rootPath() const { return m_rootPath; }
			unsigned short port() const { return m_port; }
			virtual void run() override;
			virtual void onCancel() override
			{
				// If we aren't the ones cancelling the thread, do nothing.
				// We caused cancellation. Close the socket so that the
				// 'accept()' loop breaks and the thread can tell it needs
				// to terminate.
				SOCKET socket = m_listenSock.exchange(INVALID_SOCKET);
				if ( socket != INVALID_SOCKET )
					OS_CLOSESOCKET(socket);
				std::this_thread::yield();
			}
			void addVirtualFile(std::string desc_, std::string path_, VirtualFileHandler handler_);


        private:
			typedef std::unordered_map< std::string, std::pair<VirtualFileHandler /*handler*/, std::string /*description*/> > VirtualFileHandlers;

			std::string m_rootPath;
			unsigned short m_port;
			std::mutex m_config_mutex;
			std::atomic<SOCKET> m_listenSock;
			VirtualFileHandlers m_virtualFileHandlers;
			std::string m_resourceList;

			void processHTTPRequest(const SOCKET fd, char* const request);
		};

		extern Server s_debugServer;
	}
}

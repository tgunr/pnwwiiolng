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
			Server()
				: PNThread()
				, m_rootPath("")
				, m_port(0)
				, m_config_mutex()
				, m_listenSock(INVALID_SOCKET)
				, m_resourceList("[]")
				{
				}

			virtual ~Server() {}

		public:
			void start(const char* const rootPath_, const unsigned short port_)
			{
				ABORT_IF(isStarted());
				m_rootPath = rootPath_;
				m_port = port_;
				PNThread::start();
			}

		private:
			std::string m_rootPath;
		private:
			unsigned short m_port;
		private:
			std::mutex m_config_mutex;
		private:
			std::atomic<int> m_listenSock;

		public:
			const std::string& rootPath() const { return m_rootPath; }
		public:
			unsigned short port() const { return m_port; }

		public:
			virtual void run() OVERRIDE_FN;

		public:
			virtual void onCancel() OVERRIDE_FN
			{
				// If we aren't the ones cancelling the thread, do nothing.
				// We caused cancellation. Close the socket so that the
				// 'accept()' loop breaks and the thread can tell it needs
				// to terminate.
				int socket = m_listenSock.exchange(INVALID_SOCKET);
				if ( socket != INVALID_SOCKET )
					OS_CLOSESOCKET(socket);
				std::this_thread::yield();
			}
		public:
			typedef void (*VirtualFileHandler)(unsigned int /*fd*/, const char* /*path*/, const char* /*args*/);
		private:
			typedef std::unordered_map< std::string, std::pair<VirtualFileHandler /*handler*/, std::string /*description*/> > VirtualFileHandlers;
			VirtualFileHandlers m_virtualFileHandlers;
		private:
			std::string m_resourceList;

		public:
			void addVirtualFile(std::string desc_, std::string path_, VirtualFileHandler handler_);

		private:
			void processHTTPRequest(const int fd, char* const request);
		};

		extern Server s_debugServer;
	}
}


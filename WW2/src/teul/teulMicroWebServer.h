// Created: Tue Nov 22 13:41:59 CST 2011
// Author: Oliver "kfsone" Smith <osmith@playnet.com>

#pragma once

#include <atomic>
#include "wwii/wwiiHash.h"
#include "include/pnThread.h"

namespace Teul
{
	namespace uWeb
	{
		//! @class Teul::uWeb::Server
		//! @brief Micro Web-server
		class Server : public PNThread
		{
		public:
			using VirtualFileHandler = void(*)(SOCKET /*fd*/, const char* /*path*/, const char* /*args*/);

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
			std::mutex m_config_mutex;
			std::atomic<SOCKET> m_listenSock;
			VirtualFileHandlers m_virtualFileHandlers;
			std::string m_resourceList;

			void processHTTPRequest(const SOCKET fd, char* const request);
		};

		extern Server s_debugServer;
	}
}

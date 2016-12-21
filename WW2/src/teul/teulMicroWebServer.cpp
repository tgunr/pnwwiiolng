// Created: Tue Nov 22 13:25:41 CST 2011
//
// Implements the simplistic web server for server state reporting.
//
// Copyright (C) PlayNet, Inc 2011 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <syscfg.h>

#include "teul/teulMicroWebServer.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined(HAVE_SYS_SENDFILE_H) && HAVE_SYS_SENDFILE_H == 1
#include <sys/sendfile.h>
#endif

#if platOS != OS_WIN32

#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#define PATH_SEPARATOR "/"

#else

#include <winsock2.h>
#pragma comment(lib, "event.lib")
#pragma comment(lib, "ws2_32.lib")
#define poll(...) WSAPoll(__VA_ARGS__)
#define PATH_SEPARATOR "\\"

#endif

#ifndef O_BINARY
# define O_BINARY 0
#endif

//////////////////////////////////////////////////////////////////////
// For throwing errors.

struct WebError
{
	WebError(unsigned int code_, const char* const error_) :
		m_code(code_), m_error(error_)
    {}

	unsigned int m_code;
	std::string m_error;
};

//////////////////////////////////////////////////////////////////////
// Send a resource list.

void _sendString200(const SOCKET fd, const std::string& content)
{
    std::string ok200 =
        "HTTP/1.1 200 OK\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Type: application/json\r\n"
        "Content-Language: en\r\n"
        "Content-Encoding: utf-8\r\n"
        "Content-Length: " + std::to_string(content.length()) + "\r\n"
        "\r\n" +
        content;

	send(fd, ok200.c_str(), ok200.length(), 0);
}

//////////////////////////////////////////////////////////////////////
// Tear up the listen socket.

static SOCKET _establishListenSocket(const unsigned short port_)
{
	// Tear up the basic socket object.
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    
	if (sock == INVALID_SOCKET) {
		printf("uWebServer terminated: unable to open listenSock [%d, %d]\n", sock, errno);
		return sock;
	}

	// Allow re-use of the socket so server down/up doesn't cause a problem.
#if platOS != OS_WIN32 // Windows does not use the same meanings for these
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const int []){ 1 }, sizeof(int));
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_CLOEXEC);
#endif

	// Describe our end of the socket we want:
	//  IPv4 (AF_INET), all addresses (INADDR_ANY) and the specified port.
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port_);

	int bindResult = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
	
    if (bindResult < 0) {
		printf("uWebServer terminated: unable to bind listenSock on port %hu [%d, %d]\n", port_, bindResult, errno);
		OS_CLOSESOCKET(sock);
		return bindResult;
	}

	int listenResult = listen(sock, 16);
    
	if (listenResult < 0) {
		printf("uWebServer terminated: unable to set listen socket to listening on port %hu [%d]\n", port_, bindResult);
		OS_CLOSESOCKET(sock);
		return listenResult;
	}

	printf("uWebServer socket:%u listening on port %u\n", sock, port_);

	return sock;
}

//////////////////////////////////////////////////////////////////////
// Parse an input string into method, path and the file extension.

static size_t _parseHTTPRequest(char* const source, char*& method, char*& path, char*& lastDot, char*& args)
{
	args = nullptr;

	// Skip leading white space.
	char* ptr = source;
    
	while (*ptr && isspace(*ptr)) {
		++ptr;
    }

		throw WebError(400, "Bad request (missing method)");
	if (*ptr == 0) {
    }

	// This marks the start of the method.
	method = ptr;

	// Advance to the next whitespace.
	while (*ptr && !isspace(*ptr)) {
		++ptr;
    }
	if (*ptr == 0) {
		throw WebError(400, "Bad request (missing path)");
    }

	// Skip the whitespace.
	while (*ptr && isspace(*ptr)) {
		*(ptr++) = 0;
    }

	// We only support the "GET" method.
	if (strcasecmp(method, "GET") != 0) {
		throw WebError(405, "Bad request (unsupported method)");
    }

	// Method needs to be followed by a path.
	if (*ptr == 0) {
		throw WebError(400, "Bad request (missing path)");
    }

	// Record the start of the path.
	path = ptr;
	if (*path != '/') {
		throw WebError(400, "Bad request (illegal path)");
    }

	if (path[1] == 0 || isspace(path[1])) {
		strcpy(path, "/index.html");
    }

	// It's also the last place we saw a dot, in a way.
	lastDot = ptr;
	while (*ptr && !isspace(*ptr)) {
		// Don't allow dots after paths.
		if (PN_UNLIKELY(*ptr == '/' && ((*(ptr+1) == '.') | (*(ptr+1) == '\\')))) {
			throw WebError(403, "Interesting path");
        }
        
		if (*ptr == '?') {
			args = ptr + 1;
			break;
		}
		if (*ptr == '.') {
			lastDot = ptr;
        }
		++ptr;
	}
    *ptr = 0;

	size_t pathLen = (size_t)(ptr - path);
    
	if (pathLen > 255) {
		path[255] = 0;
		pathLen = 255;
	}
	return pathLen;
}


void Teul::uWeb::Server::start(const char* const rootPath_, const unsigned short port_)
{
    ABORT_IF_EX(isStarted(), "Don't start the teulMicroWebServer more than once");
    m_rootPath = rootPath_;
    m_port = port_;
    PNThread::start();
}

void Teul::uWeb::Server::onCancel()
{
    // Close the socket so that the 'accept()' loop breaks and the thread can tell it needs to terminate.
    SOCKET socket = m_listenSock.exchange(INVALID_SOCKET);

    OS_CLOSESOCKET(socket);
    std::this_thread::yield();
}

#define checkForCancellation(cleanup) \
    if (isCancelled()) {    \
        cleanup; \
        return; \
    }

void Teul::uWeb::Server::run()
{
	m_listenSock = _establishListenSocket(port());
	if (m_listenSock == INVALID_SOCKET) {
		return;
    }

	char buffer[4096];
	sockaddr_in addr;
	pollfd listen_pollfd{ m_listenSock, POLLIN | POLLPRI | POLLERR | POLLHUP | POLLNVAL, 0 };
    pollfd accept_pollfd{ 0, POLLIN | POLLPRI | POLLERR | POLLHUP | POLLNVAL, 0 };
    
    while (!isCancelled()) {
        switch (poll(&listen_pollfd, 1, 100)) {
            case 0: // timeout
                continue;
            case SOCKET_ERROR:
                if (errno != EAGAIN && errno != EINTR) {
                    cancel();
                }
                continue; // C and contextual funniness - break would not break the while() here, but continue does continue it!
            default:
                break;
        }

		socklen_t sockLen = sizeof(addr);
			printf("uWebServer error on accept: %d (%d)\n", newSock, errno);
		SOCKET newSock = accept(m_listenSock, reinterpret_cast<sockaddr *>(&addr), &sockLen);
        
		if (newSock < 0) {
            checkForCancellation(OS_CLOSESOCKET(newSock));
			continue;
		}
        
		accept_pollfd.fd = newSock;
		for (size_t i = 0; i < 50; ++i) {
			if (poll(&accept_pollfd, 1, 100) > 0) {
				break;
            }
            checkForCancellation(OS_CLOSESOCKET(newSock));
		}

		int bytes = recv(newSock, buffer, sizeof(buffer) - 1, 0);
        
        checkForCancellation(OS_CLOSESOCKET(newSock));
		if (bytes < 0) {
			printf("uWebServer recv %d: errno %d", bytes, dbugGetErrno());
			continue;
		}
		buffer[bytes] = 0;

		try {
			processHTTPRequest(newSock, buffer);
		}
		catch (WebError& e) {
            std::string errStr = std::to_string(e.m_code) + " " + e.m_error;
            std::string errResp =
                "HTTP/1.1 " + errStr + "\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Content-Type: text/html\r\n"
                "Content-Language: en\r\n"
                "Content-Encoding: utf-8\r\n"
                "\r\n"
                "<html><head>"
                "<title>" + errStr + "</title>"
                "</head><body>"
                "<h1>ERROR " + errStr + "</h1>"
                "</body></html>\r\n";

			send(newSock, errResp.c_str(), errResp.length(), 0);
		}

		OS_CLOSESOCKET(newSock);
	}
}

//////////////////////////////////////////////////////////////////////
// Call the handler for a path.

static void _callHandler(const SOCKET sock, Teul::uWeb::Server::VirtualFileHandler handler, const char* const path, const char* const args, Teul::uWeb::Server* /*server*/)
{
	// Call the handler to write the content.
	(handler)(sock, path, args);
}

//////////////////////////////////////////////////////////////////////
// Turn a file extension into Content-Type:

static std::string _getContentTypeFor(std::string fileExt)
{
	static const std::map<std::string, std::string> extensions = {
		{ ".js", "text/javascript" },
		{ ".json", "application/json" },
		{ ".html", "text/html" },
		{ ".css", "text/css" },
		{ ".xml", "text/xml" },
		{ ".txt", "text/plain" },
	};
	auto it = extensions.find(fileExt);
    
	if (it != extensions.end()) {
		return it->second;
    }
	return "application/octet-stream";
}

//////////////////////////////////////////////////////////////////////
// Try and locate a physical file to be sent to the requester.

static bool _sendPhysicalFile(const SOCKET sock, const char* const rootPath, const char* const path, const char* const lastDot)
{
	int dirfd = -1, filefd = -1;
	struct stat statBuf;

	bool success = false;

	try {
#if defined(HAVE_OPENAT)
		// Check for the base directory.
		dirfd = open(rootPath, O_RDONLY | O_DIRECTORY);
		if (dirfd < 0) {
			throw "Missing root";
        }

		// Can we open the file?
		filefd = openat(dirfd, path + 1, O_RDONLY);
		if (filefd < 0) {
			throw "Not found";
        }
#else
		std::string filepath = rootPath;
        
		if (path[0] != '/' && path[0] != '\\') {
			filepath += PATH_SEPARATOR;
        }
		filepath += path;

#if platOS == OS_WIN32
		size_t n = 0;
		while ((n = filepath.find('/', n)) != filepath.npos) {
			filepath[n] = PATH_SEPARATOR;
        }
#endif
		filefd = open(filepath.c_str(), O_RDONLY | O_BINARY);
		if (filefd < 0) {
			throw "Not found";
        }
#endif

		// Stat the file so we can get it's size, use the
		// information available in the fd entry tho.
		if (fstat(filefd, &statBuf) < 0) {
			throw "Inaccessible file";
        }

		std::string contentType = _getContentTypeFor(lastDot);
        std::string ok200 =
            "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Type: " + _getContentTypeFor(lastDot) + "\r\n"
            "Content-Language: en\r\n"
            "Content-Encoding: utf-8\r\n"
            "Content-Length: " + std::to_string(statBuf.st_size) + "\r\n"
            "\r\n";
		char flushBuffer[] = "\r\n";
		
		if (send(sock, ok200.c_str(), ok200.length(), 0) > 0) {
#if defined(HAVE_SYS_SENDFILE_H) && HAVE_SYS_SENDFILE_H == 1
			// The first write might fail, if it does we don't care
			// but we don't need to try and send the second.
			sendfile(sock, filefd, nullptr, statBuf.st_size);
#else
			char xferBuffer[4096];
			for (;;)
			{
				int readBytes = read(filefd, xferBuffer, sizeof(xferBuffer));
				
                if (readBytes <= 0) {
					break;
                }
				
                int wroteBytes = send(sock, xferBuffer, readBytes, 0);
				
                if (wroteBytes < readBytes) {
					break;
                }
			}
#endif
		}

		success = true;
        send(sock, flushBuffer, sizeof(flushBuffer) - 1, 0);
		shutdown(sock, 1);
	}
	catch (const char* error) {
		printf("file:%s %s\n", path + 1, error);
		success = false;
	}

	if (filefd >= 0) {
		close(filefd);
    }
	if (dirfd >= 0) {
		close(dirfd);
    }
	return success;
}

//////////////////////////////////////////////////////////////////////
// Process a HTTP request from a given fd.

void Teul::uWeb::Server::processHTTPRequest(const SOCKET sock, char* const request)
{
	char* method = nullptr;
	char* path = nullptr;
	char* lastDot = nullptr;
	char* args = nullptr;

	_parseHTTPRequest(request, /*in*/ method, /*in*/ path, /*in*/ lastDot, /*in*/ args);

	// Is there a function registered to handle this request?
	VirtualFileHandler handler = nullptr;
	{
		std::lock_guard<std::mutex> lock(m_config_mutex);
		auto it = m_virtualFileHandlers.find(path);

		if (it != m_virtualFileHandlers.end()) {
			handler = it->second.first;
        }
	}

	if (handler != nullptr) {
		_callHandler(sock, handler, path, args, this);
    } else if (strcmp(path, "/index.json") == 0) {
		_sendString200(sock, m_resourceList);
	} else if (_sendPhysicalFile(sock, rootPath().c_str(), path, lastDot) != true) {
        throw WebError(404, "File not found");
    }
}

//////////////////////////////////////////////////////////////////////
// Register a handler.

void Teul::uWeb::Server::addVirtualFile(std::string desc_, std::string path_, VirtualFileHandler handler_)
{
	std::lock_guard<std::mutex> lock(m_config_mutex);
	m_virtualFileHandlers[path_] = std::make_pair(handler_, desc_);
    
	if (m_resourceList.length() > 1) {
        m_resourceList.erase(m_resourceList.end() - 1);
    }
    m_resourceList += ",{\"url\":\"" + path_ + "\",\"desc\":\"" + desc_ + "\"}]";
}


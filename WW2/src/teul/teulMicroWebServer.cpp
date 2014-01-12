#include <syscfg.h> // Path is project-specific.
// Created: Tue Nov 22 13:25:41 CST 2011
//
// Implements the simplistic web server for server state reporting.
//
// Copyright (C) PlayNet, Inc 2011 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulMicroWebServer.h"
#if platOS != OS_WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#else
#include <winsock2.h>
#endif
#include <errno.h>
#if defined(HAVE_SYS_SENDFILE_H) && HAVE_SYS_SENDFILE_H == 1
#include <sys/sendfile.h>
#endif

#include <event2/event.h>
#if platOS == OS_WIN32
# pragma comment(lib, "event.lib")
# pragma comment(lib, "ws2_32.lib")
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef O_BINARY
# define O_BINARY 0
#endif

//////////////////////////////////////////////////////////////////////
// For throwing errors.

struct WebError
{
	WebError(unsigned int code_, const char* const error_)
		: m_code(code_)
		, m_error(error_)
		{}

	unsigned int m_code;
	std::string m_error;
};

//////////////////////////////////////////////////////////////////////
// Send a resource list.

void
_sendString200(const int fd, const std::string& content)
{
	static const char ok200[] = "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: text/application-json\r\nContent-Language: en\r\nContent-Encoding: UTF-8\r\nContent-Length: %" PRSizeT "\r\n\r\n%s\n";
	char* buf = new char[sizeof(ok200) + content.length() + 1];
	int bytes = sprintf(buf, ok200, content.length() + 1, content.c_str());
	send(fd, buf, bytes, 0);
	delete [] buf;
}

//////////////////////////////////////////////////////////////////////
// Tear up the listen socket.

static unsigned int
_establishListenSocket(const unsigned short port_)
{
	// Tear up the basic socket object.
	const int sock = socket(AF_INET, SOCK_STREAM, 0);
	if ( sock == INVALID_SOCKET )
	{
		printf("uWebServer terminated: unable to open listenSock [%d, %d]\n", sock, errno);
		return sock;
	}

	// Allow re-use of the socket so server down/up doesn't cause a problem.
	evutil_make_listen_socket_reuseable(sock);
	evutil_make_socket_closeonexec(sock);

	// Describe our end of the socket we want:
	//  IPv4 (AF_INET), all addresses (INADDR_ANY) and the specified port.
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port_);

	const int bindResult = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
	if ( bindResult < 0 )
	{
		printf("uWebServer terminated: unable to bind listenSock on port %hu [%d, %d]\n", port_, bindResult, errno);
		close(sock);
		return bindResult;
	}

	const int listenResult = listen(sock, 16);
	if ( listenResult < 0 )
	{
		printf("uWebServer terminated: unable to set listen socket to listening on port %hu [%d]\n", port_, bindResult);
		close(sock);
		return listenResult;
	}

	evutil_make_listen_socket_reuseable(sock);

	printf("uWebServer socket:%u listening on port %hu\n", sock, port_);

	return sock;
}

//////////////////////////////////////////////////////////////////////
// Parse an input string into method, path and the file extension.

static size_t
_parseHTTPRequest(char* const source, char*& method, char*& path, char*& lastDot, char*& args)
{
	args = nullptr;

	// Skip leading white space.
	char* ptr = source;
	while ( *ptr && isspace(*ptr) )
		++ptr;

	if ( *ptr == 0 )
		throw WebError(400, "Bad request (missing method)");

	// This marks the start of the method.
	method = ptr;

	// Advance to the next whitespace.
	while ( *ptr && !isspace(*ptr) )
		++ptr;
	if ( *ptr == 0 )
		throw WebError(400, "Bad request (missing path)");

	// Skip the whitespace.
	while ( *ptr && isspace(*ptr) )
		*(ptr++) = 0;

	// We only support the "GET" method.
	if ( strcasecmp(method, "GET") != 0 )
		throw WebError(405, "Bad request (unsupported method)");

	// Method needs to be followed by a path.
	if ( *ptr == 0 )
		throw WebError(400, "Bad request (missing path)");

	// Record the start of the path.
	path = ptr;
	if ( *path != '/' )
		throw WebError(400, "Bad request (illegal path)");

	if ( path[1] == 0 || isspace(path[1]) )
		strcpy(path, "/index.html");

	// It's also the last place we saw a dot, in a way.
	lastDot = ptr;
	while ( *ptr && !isspace(*ptr) )
	{
		// Don't allow dots after paths.
		if ( PN_UNLIKELY(*ptr == '/' && ((*(ptr+1) == '.') | (*(ptr+1) == '\\'))) )
			throw WebError(403, "Interesting path");
		if ( *ptr == '?' )
		{
			args = ptr + 1;
			break;
		}
		if ( *ptr == '.' )
			lastDot = ptr;
		++ptr;
	}

	*ptr = 0;

	size_t pathLen = (size_t)(ptr - path);
	if ( pathLen > 255 )
	{
		path[255] = 0;
		pathLen = 255;
	}

	return pathLen;
}

void
Teul::uWeb::Server::run()
{
	m_listenSock = _establishListenSocket(port());
	if (m_listenSock == INVALID_SOCKET)
		return;

	char buffer[4096];
	sockaddr_in addr;

	for ( ; !isCancelled(); )
	{
		// Make sure we're not providing a denial of service to the host by using all the CPU.
		std::this_thread::yield();

		socklen_t sockLen = sizeof(addr);
		const int newSock = accept(m_listenSock, (struct sockaddr*)&addr, &sockLen);
		if ( newSock < 0 )
		{
			printf("uWebServer error on accept: %d (%d)\n", newSock, errno);
			continue;
		}

		int bytes = recv(newSock, buffer, sizeof(buffer) - 1, 0);
		buffer[bytes] = 0;

		try
		{
			processHTTPRequest(newSock, buffer);
		}
		catch (WebError& e)
		{
			static const char errFmt[] = "HTTP/1.1 %u %s\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: text/html\r\nContent-Language: en\r\nAllow: GET\r\n\r\n<html><head><title>%u %s</title></head><body><h1>ERROR %u: %s</h1></body></html>\r\n\r\n";
			char* myBuf = new char[sizeof(errFmt) + e.m_error.size() * 3 + 32];
			const int len = sprintf(myBuf, errFmt, e.m_code, e.m_error.c_str(), e.m_code, e.m_error.c_str(), e.m_code, e.m_error.c_str());
			send(newSock, myBuf, len, 0);
			delete [] myBuf;
		}

		OS_CLOSESOCKET(newSock);
	}
}

//////////////////////////////////////////////////////////////////////
// Call the handler for a path.

static void
_callHandler(const unsigned int sock, Teul::uWeb::Server::VirtualFileHandler handler, const char* const path, const char* const args, Teul::uWeb::Server* /*server*/)
{
	// Write the HTTP header first
	static const char ok200[] = "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: text/javascript\r\nContent-Language: en\r\nContent-Encoding: UTF-8\r\n\r\n";
	send(sock, ok200, sizeof(ok200) - 1, 0);

	// Call the handler to write the content.
	(handler)(sock, path, args);
}

//////////////////////////////////////////////////////////////////////
// Turn a file extension into Content-Type:

static const char*
_getContentTypeFor(const char* const fileExt)
{
	if ( strcmp(fileExt, ".js") == 0 )
		return "text/javascript";
	else if ( strcmp(fileExt, ".json") == 0 )
		return "text/application-json";
	else if ( strcmp(fileExt, ".html") == 0 )
		return "text/html";
	else if ( strcmp(fileExt, ".css") == 0 )
		return "text/css";
	else if ( strcmp(fileExt, ".xml") == 0 )
		return "text/xml";
	else if ( strcmp(fileExt, ".txt") == 0 )
		return "text/plain";
	return "application/octet-stream";
}

//////////////////////////////////////////////////////////////////////
// Try and locate a physical file to be sent to the requestee.

static bool
_sendPhysicalFile(const unsigned int sock, const char* const rootPath, const char* const path, const char* const lastDot)
{
	int dirfd = -1, filefd = -1;
	struct stat statBuf;

	bool success = false;

	try
	{
#if defined(HAVE_OPENAT)
		// Check for the base directory.
		dirfd = open(rootPath, O_RDONLY | O_DIRECTORY);
		if ( dirfd < 0 )
			throw "Missing root";

		// Can we open the file?
		filefd = openat(dirfd, path + 1, O_RDONLY);
		if ( filefd < 0 )
			throw "Not found";
#else
		std::string filepath = rootPath;
		if ( path[0] != '/' && path[0] != '\\' )
			filepath += PATH_SEPARATOR;
		filepath += path;
#if platOS == OS_WIN32
		size_t n = 0;
		while ( (n = filepath.find('/', n)) != filepath.npos )
			filepath[n] = PATH_SEPARATOR;
#endif
		filefd = open(filepath.c_str(), O_RDONLY | O_BINARY);
		if ( filefd < 0 )
			throw "Not found";
#endif

		// Stat the file so we can get it's size, use the
		// information available in the fd entry tho.
		if ( fstat(filefd, &statBuf) < 0 )
			throw "Inaccessible file";

		static const char ok200[] = "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: %s\r\nContent-Language: en\r\nContent-Encoding: UTF-8\r\nContent-Length: %llu\r\n\r\n";
		char myBuf[sizeof(ok200) + 128];

		const char* contentType = _getContentTypeFor(lastDot);

		int len = sprintf(myBuf, ok200, contentType, (UINT64)statBuf.st_size);
		if ( send(sock, myBuf, len, 0) > 0 )
		{
#if defined(HAVE_SYS_SENDFILE_H) && HAVE_SYS_SENDFILE_H == 1
			// The first write might fail, if it does we don't care
			// but we don't need to try and send the second.
			sendfile(sock, filefd, nullptr, statBuf.st_size);
#else
			char xferBuffer[4096];
			for (;; )
			{
				size_t readBytes = read(filefd, xferBuffer, sizeof(xferBuffer));
				if ( readBytes <= 0 )
					break;
				size_t wroteBytes = send(sock, xferBuffer, readBytes, 0);
				if ( wroteBytes < readBytes )
					break;
			}
#endif
		}

		success = true;

		char flushBuffer[] = "\r\n";
		send(sock, flushBuffer, sizeof(flushBuffer) - 1, 0);
		shutdown(sock, 1);
	}
	catch ( const char* error )
	{
		printf("file:%s %s\n", path + 1, error);
		success = false;
	}

	if ( filefd >= 0 )
		close(filefd);
	if ( dirfd >= 0 )
		close(dirfd);

	return success;
}

//////////////////////////////////////////////////////////////////////
// Process a HTTP request from a given fd.

void
Teul::uWeb::Server::processHTTPRequest(const int sock, char* const request)
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
		if ( it != m_virtualFileHandlers.end() )
			handler = it->second.first;
	}

	if ( handler != nullptr )
	{
		_callHandler(sock, handler, path, args, this);
		return;
	}

	if ( _sendPhysicalFile(sock, rootPath().c_str(), path, lastDot) == true )
		return;

	if ( strcmp(path, "/index.json") == 0 )
	{
		_sendString200(sock, m_resourceList);
		return;
	}

	throw WebError(404, "File not found");
}

//////////////////////////////////////////////////////////////////////
// Register a handler.

void
Teul::uWeb::Server::addVirtualFile(std::string desc_, std::string path_, VirtualFileHandler handler_)
{
	// '{"url":"<url>","desc":"<desc>"}]
	const size_t bufLen = 9 + path_.length() + 10 + desc_.length() + 2 + 2 + 1;
	char* buffer = new char[bufLen];
	snprintf(buffer, bufLen - 1, "{\"url\":\"%s\",\"desc\":\"%s\"}]", path_.c_str(), desc_.c_str());

	std::lock_guard<std::mutex> lock(m_config_mutex);
	m_virtualFileHandlers[path_] = std::make_pair(handler_, desc_);

	// Replace the ']' at the end of the list with a comma.
	m_resourceList.erase(m_resourceList.end() - 1);
	if ( m_resourceList.length() > 1 )
		m_resourceList.append(",");

	m_resourceList.append(buffer);

	delete [] buffer;
}


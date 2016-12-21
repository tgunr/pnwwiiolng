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
#include "fio/fio.h"

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

#define checkForCancellation(cleanup) \
    if (isCancelled()) {    \
        cleanup; \
        return; \
    }

static const std::vector<std::string> s_knownMethods = {
    "GET",
    "POST",
    "HEAD",
    "OPTIONS",
    "PUT",
    "PATCH",
    "DELETE",
};

static const std::unordered_map<int, std::string> s_knownResponseCodes = {
    { 100, "Continue" },
    { 200, "OK" },
    { 201, "Created" },
    { 204, "No Content" },
    { 206, "Partial Content" },
    { 301, "Moved Permanently" },
    { 302, "Found" },
    { 307, "Temporary Redirect" },
    { 400, "Bad Request" },
    { 401, "Unauthorized" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
    { 405, "Method Not Allowed" },
    { 500, "Internal Server Error" },
    { 501, "Not Implemented" },
    { 505, "HTTP Version Not Supported" },
};

static const std::unordered_map<std::string, std::string> s_mimeMap = {
    { ".js", "text/javascript" },
    { ".json", "application/json" },
    { ".html", "text/html" },
    { ".css", "text/css" },
    { ".xml", "text/xml" },
    { ".txt", "text/plain" },
};

static const std::string s_fallbackMimeType = "application/octet-stream";


//////////////////////////////////////////////////////////////////////
// For throwing errors.

struct WebError
{
	WebError(unsigned int code_, const char* const error_ = nullptr) :
		m_code(code_), m_error(error_ ? error_ : "")
    {
        if (!error_) {
            std::unordered_map<int, std::string>::const_iterator it = s_knownResponseCodes.find(m_code);
            
            if (it != s_knownResponseCodes.end()) {
                m_error = it->second;
            }
        }
    }

	unsigned int m_code;
	std::string m_error;
};

//////////////////////////////////////////////////////////////////////
// HTTP request parser
bool Teul::uWeb::HTTPRequestParser::parseMoreData(const std::string &data)
{
    ABORT_IF_EX(data.length() < 1, "Must have at least one byte of data in a buffer");
    
    m_parseBuffer += data;
    
    // A coroutine would be really nice here, but since we don't have those in
    // C++14, we go through a painful state machine instead and use the fields
    // of the request structure as an idempotency trick to avoid a state enum.
    
    #define yield_if(cond) do { \
        if ((cond)) { \
            return false; \
        } \
    } while (0)
    
    #define bad_request_if(cond) do { \
        if ((cond)) { \
            throw WebError(400); \
        } \
    } while (0)
    
    // Parse method
    if (m_request.method == "") {
        std::string::size_type nextWhitespace = m_parseBuffer.find_first_of(' ', m_parsePointer);
        
        yield_if(nextWhitespace == std::string::npos); // No whitespace found yet
        bad_request_if(nextWhitespace == m_parsePointer); // Whitespace is next character
        m_request.method = m_parseBuffer.substr(m_parsePointer, nextWhitespace - m_parsePointer);
        m_parsePointer = nextWhitespace + 1;
    }
    
    // Parse path
    if (m_request.path == "") {
        std::string::size_type nextWhitespace = m_parseBuffer.find_first_of(' ', m_parsePointer);
        
        yield_if(nextWhitespace == std::string::npos);
        bad_request_if(nextWhitespace == m_parsePointer);
        m_request.path = m_parseBuffer.substr(m_parsePointer, nextWhitespace - m_parsePointer);
        m_parsePointer = nextWhitespace + 1;
    }
    
    // Parse version
    if (m_request.version == "") {
        std::string::size_type nextCrlf = m_parseBuffer.find("\r\n", m_parsePointer);
        
        yield_if(nextCrlf == std::string::npos);
        bad_request_if(nextCrlf == m_parsePointer);
        m_request.version = m_parseBuffer.substr(m_parsePointer, nextCrlf - m_parsePointer);
        m_parsePointer = nextCrlf + 2;
    }
    
    // Parse headers - need this extra state variable here because "no headers" is a valid HTTP/1.0 request
    while (!m_seenHeaderTerminator) {
        std::string::size_type nextCrlf = m_parseBuffer.find("\r\n", m_parsePointer);
        
        yield_if(nextCrlf == std::string::npos);
        if (nextCrlf == m_parsePointer) {
            m_seenHeaderTerminator = true;
        }
        
        std::string rawHeader = m_parseBuffer.substr(m_parsePointer, nextCrlf - m_parsePointer);

        m_parsePointer = nextCrlf + 2;
        if (rawHeader.length() == 0) { // Putting this here avoids repeating the update of m_parsePointer
            break;
        }
        
        std::string::size_type separatorPos = rawHeader.find(": ");
        
        bad_request_if(separatorPos == std::string::npos || separatorPos == 0); // Missing or empty header name
        
        std::string headerName = rawHeader.substr(0, separatorPos);
        std::string headerValue = rawHeader.substr(separatorPos + 2);
        std::string lowerHeaderName;
        
        std::transform(headerName.begin(), headerName.end(), std::back_inserter(lowerHeaderName), [] (char c) { return tolower(c); });
        
        // If it's a content-length header, validate and parse it now instead of later because we'll be using it
        if (lowerHeaderName.compare("content-length") == 0) {
            bad_request_if(std::find_if(headerValue.begin(), headerValue.end(), [] (char c) { return !isdigit(c); }) != headerValue.end());
            m_knownContentLength = std::stoull(headerValue);
        }
        
        m_request.orderedHeaders.push_back(rawHeader);
        m_request.headers.insert({ lowerHeaderName, headerValue });
    }
    
    // Everything else is request body.
    std::string newRequestData = m_parseBuffer.substr(m_parsePointer);
    
    m_parsePointer += newRequestData.length();
    m_request.requestData += newRequestData;
    
    if (m_knownContentLength != UINT64_MAX) {
        // If a length was given, we're only done if that length has been reached.
        return m_request.requestData.length() >= m_knownContentLength;
    } else {
        // Otherwise, we're done if we've seen all the headers
        return m_seenHeaderTerminator;
    }

    #undef yield_if
    #undef bad_request_if
}

void Teul::uWeb::HTTPRequestParser::validateRequest() const
{
    // Well-formedness checks
    
    // Method must be purely alphabetical
    if (m_request.method.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz") != std::string::npos) {
        throw WebError(400);
    }
    // Path must be printable ASCII (c < 0 covers anything above ASCII 127)
    if (std::find_if(m_request.path.begin(), m_request.path.end(), [] (char c) { return c < 0 || !isprint(c); }) != m_request.path.end()) {
        throw WebError(400);
    }
    // Version must be of the form HTTP/m.n
    if (m_request.version.compare(0, 5, "HTTP/") != 0 || m_request.version.length() != 8 || m_request.version[6] != '.' ||
        !isdigit(m_request.version[5]) || !isdigit(m_request.version[7])
    ) {
        throw WebError(400);
    }
    for (const std::pair<std::string, std::string> &kv : m_request.headers) {
        // Header names must be alphabetical plus dash
        if (kv.first.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-") != std::string::npos) {
            throw WebError(400);
        }
        // Header values must be printable ASCII
        if (std::find_if(kv.second.begin(), kv.second.end(), [] (char c) { return c < 0 || !isprint(c); }) != kv.second.end()) {
            throw WebError(400);
        }
    }

    // Validity checks
    
    if (std::find_if(s_knownMethods.begin(), s_knownMethods.end(), [m = m_request.method] (const std::string &km) {
        return strcasecmp(km.c_str(), m.c_str()) == 0;
    }) == s_knownMethods.end()) {
        throw WebError(405);
    }
    if (m_request.version.compare("HTTP/1.0") != 0 && m_request.version.compare("HTTP/1.1") != 0) {
        throw WebError(505);
    }
    if (m_knownContentLength != UINT64_MAX && m_request.requestData.length() != m_knownContentLength) {
        // Content must be exactly as long as specified
        throw WebError(400);
    }
}

//////////////////////////////////////////////////////////////////////
// Response builder

Teul::uWeb::HTTPResponseBuilder::HTTPResponseBuilder()
    : m_response()
{
    m_response.version = "HTTP/1.1";
}

Teul::uWeb::HTTPResponseBuilder::HTTPResponseBuilder(const HTTPRequest &originalRequest)
    : HTTPResponseBuilder()
{
    m_response.version = originalRequest.version;
}

Teul::uWeb::HTTPResponseBuilder &Teul::uWeb::HTTPResponseBuilder::status(int responseCode, const std::string &statusMessage)
{
    m_response.responseCode = responseCode;
    if (statusMessage == "") {
        std::unordered_map<int, std::string>::const_iterator it = s_knownResponseCodes.find(responseCode);
        
        if (it != s_knownResponseCodes.end()) {
            m_response.statusMessage = it->second;
        } else {
            // What to do here?
        }
    } else {
        m_response.statusMessage = statusMessage;
    }
    return *this;
}

Teul::uWeb::HTTPResponseBuilder &Teul::uWeb::HTTPResponseBuilder::header(const std::string &headerName, const std::string &headerValue, bool replace)
{
    if (replace) {
        for (auto it = m_response.orderedHeaders.begin(); it != m_response.orderedHeaders.end(); ++it) {
            if (it->compare(0, headerName.size() + 2, headerName + ": ") == 0) {
                m_response.orderedHeaders.erase(it);
                break;
            }
        }
    }
    m_response.orderedHeaders.push_back(headerName + ": " + headerValue);
    return *this;
}

Teul::uWeb::HTTPResponseBuilder &Teul::uWeb::HTTPResponseBuilder::content(const std::string &data, bool suppressAutoContentLength, bool append)
{
    if (append) {
        m_response.responseData += data;
    } else {
        m_response.responseData = data;
    }
    
    if (!suppressAutoContentLength) {
        return this->header("Content-Length", std::to_string(m_response.responseData.length()), true);
    } else {
        return *this;
    }
}

Teul::uWeb::HTTPResponseBuilder &Teul::uWeb::HTTPResponseBuilder::JSON(const std::string &JSONData)
{
    return this->header("Content-Type", "application/json", true).content(JSONData, false, false);
}

Teul::uWeb::HTTPResponseBuilder &Teul::uWeb::HTTPResponseBuilder::file(const std::string &filePath, bool autodetectContentType)
{
    if (autodetectContentType) {
        std::string::size_type extLoc = filePath.find_last_of('.');

        if (extLoc != std::string::npos) {
            std::string ext = filePath.substr(extLoc);
            auto it = std::find_if(s_mimeMap.begin(), s_mimeMap.end(), [&ext] (const auto &kv) { return strcasecmp(kv.first.c_str(), ext.c_str()) == 0; });
            
            if (it != s_mimeMap.end()) {
                this->header("Content-Type", it->second);
            } else {
                this->header("Content-Type", s_fallbackMimeType);
            }
        } else {
            this->header("Content-Type", s_fallbackMimeType);
        }
    }
    m_response.responseVirtualFilePath = filePath;
    return *this;
}


const Teul::uWeb::HTTPResponse &Teul::uWeb::HTTPResponseBuilder::getResponse() const
{
    return m_response;
}

Teul::uWeb::HTTPResponse Teul::uWeb::HTTPResponseBuilder::copyResponse()
{
    return m_response;
}

void Teul::uWeb::HTTPResponseBuilder::build(Teul::uWeb::HTTPResponse &result)
{
    result = std::move(m_response);
}

//////////////////////////////////////////////////////////////////////
// URL decoder
static inline bool hexToBin(char in, uint8_t &out)
{
    if (in >= '0' && in <= '9') {
        out = in - '0';
    } else if (in >= 'A' && in <= 'F') {
        out = (in - 'A') + 10;
    } else if (in >= 'a' && in <= 'f') {
        out = (in - 'a') + 10;
    } else {
        return false;
    }
    return true;
}

std::string URLDecode(const std::string &input)
{
    std::string result = input;
    std::string::size_type loc = 0;

    while ((loc = result.find_first_of('%', loc)) != std::string::npos) {
        std::string escape = result.substr(loc + 1, 2);
        uint8_t highNibble = 0, lowNibble = 0;
        
        if (escape.length() == 2 && hexToBin(escape[0], highNibble) && hexToBin(escape[1], lowNibble)) {
            result.replace(loc, 3, std::string(1, ((highNibble & 0x0f) << 4) | (lowNibble & 0x0f)));
        } else {
            ++loc;
        }
    }
    return result;
}

//////////////////////////////////////////////////////////////////////
// Client thread
Teul::uWeb::Client::~Client()
{
    m_owner.client_unregisterClient(*this);
}

void Teul::uWeb::Client::onCancel()
{
    SOCKET socket = m_clientSock.exchange(INVALID_SOCKET);

    OS_CLOSESOCKET(socket);
    std::this_thread::yield();
}

void Teul::uWeb::Client::run()
{
    m_requestStart = std::chrono::steady_clock::now();
    
    pollfd pfd{ m_clientSock, POLLIN | POLLPRI | POLLERR | POLLHUP | POLLNVAL, 0 };
    
    while (!isCancelled()) {
        if ((std::chrono::steady_clock::now() - m_requestStart) > std::chrono::seconds(10)) { \
            dbugLog(_FLD, "uWebServerClient: request timeout (10 seconds) on " PRF_SOCKET " exceeded", static_cast<SOCKET>(m_clientSock));
            cancel();
            break;
        }
        switch (poll(&pfd, 1, 100)) {
            case 0: // timeout
                continue;
            case SOCKET_ERROR:
                if (errno != EAGAIN && errno != EINTR) {
                    cancel();
                }
                continue;
            default:
                break;
        }

        std::string byteBuffer(2048, '\0');
		int recvResult = recv(m_clientSock, &byteBuffer[0], byteBuffer.length(), 0);
        
        checkForCancellation();
        if (recvResult < 0) {
            dbugLog(_FLI, "uWebServerClient recv error on " PRF_SOCKET ": errno %d", static_cast<SOCKET>(m_clientSock), dbugGetErrno());
            if (dbugGetErrno() != EAGAIN && dbugGetErrno() != EINTR) {
                cancel();
                continue;
            }
        } else if (recvResult == 0) {
            dbugLog(_FLW, "uWebServerClient: recv on " PRF_SOCKET " had EOF/no data available after poll() returned?", static_cast<SOCKET>(m_clientSock));
            continue;
        }
        
        byteBuffer.resize(static_cast<std::string::size_type>(recvResult)); // we assume sizeof(size_type) >= sizeof(int)
        
        HTTPResponse response;
        
        try {
            if (m_requestParser.parseMoreData(byteBuffer)) {
                // Got a complete request
                m_requestParser.validateRequest(); // Will throw errors if any
                
                Server::VirtualFileHandler handler = m_owner.client_findHandlerForRequest(m_requestParser.getRequest());
                
                if (handler) {
                    handler(m_requestParser.getRequest(), response);
                } else {
                    throw WebError(404);
                }
            } else {
                continue; // Still waiting for more request pieces
            }

            checkForCancellation();
            sendResponse(response); // Sending a file-based response can throw errors
            dbugLog(_FLD, "uWebServerClient: Client %d sent %d for %s", static_cast<SOCKET>(m_clientSock), response.responseCode, m_requestParser.getRequest().path.c_str());
        } catch (WebError &e) {
            HTTPResponseBuilder()
                .status(e.m_code, e.m_error)
                .header("Access-Control-Allow-Origin", "*")
                .header("Content-Type", "text/html")
                .header("Content-Encoding", "utf-8")
                .content("<html><head><title>" + e.m_error + "</title></head><body><h1>" + e.m_error + "</h1></body></html>")
                .build(response);
            sendResponse(response); // Sending a 404 can never throw.
            dbugLog(_FLD, "uWebServerClient: Client %d sent %d for %s", static_cast<SOCKET>(m_clientSock), response.responseCode, m_requestParser.getRequest().path.c_str());
        }

        cancel(); // Close the socket and unregister this client
    }
}

static int _processAndOpenResponseFile(const std::string &rootPath, const std::string &virtualPath, off_t &fileSize)
{
    int filefd = -1;
    struct stat sbuf;
    
#if platOS == OS_WIN32
    // Ironically, the place where path length limits are the biggest
    // problem - Windows - is also the place where there're no *at() APIs to
    // deal with it. We could try to massage the issue with the \\?\ prefix,
    // but that should really happen in the specification of the root path,
    // so don't bother here
    filefd = open((rootPath + virtualPath).c_str(), O_RDONLY);
#else
    // Both MacOS (>= 10.10) and Linux have openat()
    int dirfd = open(rootPath.c_str(), O_RDONLY | O_DIRECTORY);
    
    if (dirfd < 0) {
        dbugLog(_FLW, "uWebServerClient: Failed to open server root directory %s: %d", rootPath.c_str(), errno);
        throw WebError(404);
    }
    filefd = openat(dirfd, virtualPath.substr(1).c_str(), O_RDONLY);
    close(dirfd);
#endif
    if (filefd < 0) {
        dbugLog(_FLI, "uWebServerClient: Failed to open virtual path %s: %d", virtualPath.c_str(), errno);
        throw WebError(404);
    }
    
    if (fstat(filefd, &sbuf) < 0) {
        dbugLog(_FLA, "uWebServerClient: Succeeded at opening file %s but couldn't stat it because %d??", virtualPath.c_str(), errno);
        close(filefd);
        throw WebError(500);
    }
    if (!S_ISREG(sbuf.st_mode)) {
        dbugLog(_FLW, "uWebServerClient: Only regular files are supported, %s is 0x%x", virtualPath.c_str(), sbuf.st_mode);
        close(filefd);
        throw WebError(403);
    }
    fileSize = sbuf.st_size;
    return filefd;
}

class SendFailureError : public std::exception {};

static void _tryClientSend(int sock, const void *buffer, size_t buflen)
{
    ABORT_IF_EX(buflen > SSIZE_MAX, "Can't send buffers larger than the send result size");
    
    ssize_t sendResult = send(sock, buffer, buflen, 0);
    
    // Blocking socket = no need to check for partial send (well, close enough)
    if (sendResult < static_cast<ssize_t>(buflen)) { // Already range-checked the length so this won't underflow
        throw SendFailureError();
    }
}

static void _tryClientSendFile(int sock, const Fio::FD &filefd, off_t filesize)
{
#if platOS == OS_WIN32
    // Oh, and TransmitFile() is useless because it doesn't exist on Windows 7.
    char transferBuffer[4096];
    int readBytes = 0;
    
    // Upside-down loop!
    do {
        if (readBytes > 0) {
            _tryClientSend(sock, transferBuffer, readBytes);
        }
        readBytes = read(filefd, transferBuffer, sizeof(transferBuffer));
    } while (readBytes > 0);
#elif platOS == OS_LINUX
    if (sendfile(sock, filefd, nullptr, filesize) < filesize) {
        throw SendFailureError();
    }
#elif platOS == OS_MACOS
    off_t sent = filesize;
    
    // Annoying BSD and its completely different signature and semantics for sendfile()...
    // I mean come on, the fd and sock arguments in flipped order!?
    if (sendfile(filefd, sock, 0, &sent, nullptr, 0) < 0 && sent < filesize) {
        throw SendFailureError();
    }
#endif
}

// API guarantee: A 404 response with no virtual file path will never throw an exception.
void Teul::uWeb::Client::sendResponse(const Teul::uWeb::HTTPResponse &response)
{
    if (response.version.substr(0, 5).compare("HTTP/") != 0) {
        dbugLog(_FLA, "uWebServerClient: Repsonse has nonsense version %s", response.version.c_str());
        throw WebError(500);
    }
    if (response.responseCode < 100 || response.responseCode > 599) {
        dbugLog(_FLA, "uWebServerClient: Response has invalid response code %d", response.responseCode);
        throw WebError(500);
    }
    if (response.statusMessage == "") {
        dbugLog(_FLA, "uWebServerClient: Response has no status message");
        throw WebError(500);
    }
    
    try {
        Fio::FD filefd;
        off_t fileSize = -1;
        
        if (response.responseVirtualFilePath.length() > 0) {
            // Lots of special handling for file sends
            if (response.responseVirtualFilePath[0] != PATH_SEPARATOR[0]) {
                dbugLog(_FLA, "uWebServerClient: Virtual file paths must always start with the path separator");
                throw WebError(500);
            }
            filefd.assign(_processAndOpenResponseFile(m_owner.rootPath(), response.responseVirtualFilePath, fileSize));
        }
        
        // This would be a great place for llvm::Twine(), 'cause doing this with raw
        // std::strings is a bit inefficient.
        std::string responseStr = response.version + " " + std::to_string(response.responseCode) + " " + response.statusMessage + "\r\n";
        
        for (const std::string &hdr : response.orderedHeaders) {
            if (strncasecmp(hdr.c_str(), "Content-Length:", sizeof("Content-Length:") - 1) == 0 && filefd) {
            	continue;
            }
            responseStr += hdr + "\r\n";
        }
        if (filefd) {
            // If we're sending a file, always synthesize a content length
            // header instead of using one that was provided.
            responseStr += "Content-Length: " + std::to_string(response.responseData.length() + fileSize) + "\r\n";
        }
        responseStr += "\r\n";
        _tryClientSend(m_clientSock, responseStr.c_str(), responseStr.length());
        
        if (response.responseData.length() > 0) {
            _tryClientSend(m_clientSock, response.responseData.c_str(), response.responseData.length());
        }
        if (filefd) {
            _tryClientSendFile(m_clientSock, filefd, fileSize);
        }
    } catch (SendFailureError &) {
        dbugLog(_FLM, "uWebServerClient: Failed to send all data to client, giving up");
        return;
    }
}

//////////////////////////////////////////////////////////////////////
// Tear up the listen socket.

static SOCKET _establishListenSocket(const unsigned short port_)
{
	// Tear up the basic socket object.
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    
	if (sock == INVALID_SOCKET) {
		dbugLog(_FLA, "uWebServer terminated: unable to open listenSock [" PRF_SOCKET ", %d]", sock, errno);
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
		dbugLog(_FLA, "uWebServer terminated: unable to bind listenSock on port %hu [" PRF_SOCKET ", %d]", port_, sock, errno);
		OS_CLOSESOCKET(sock);
		return bindResult;
	}

	int listenResult = listen(sock, 16);
    
	if (listenResult < 0) {
		dbugLog(_FLA, "uWebServer terminated: unable to set listen socket to listening on port %hu [" PRF_SOCKET ", %d]", port_, sock, bindResult);
		OS_CLOSESOCKET(sock);
		return listenResult;
	}

	dbugLog(_FLN, "uWebServer socket:" PRF_SOCKET " listening on port %u", sock, port_);

	return sock;
}

void Teul::uWeb::Server::start(const char* const rootPath_, const unsigned short port_)
{
    ABORT_IF_EX(isStarted(), "Don't start the teulMicroWebServer more than once");
    m_rootPath = rootPath_;
    m_port = port_;
    addVirtualFile("Basic alive test", "/server-check.json", [] (const HTTPRequest &request, HTTPResponse &response) {
        return HTTPResponseBuilder(request)
            .status(200)
            .header("Access-Control-Allow-Origin", "*")
            .JSON("{\"working\":\"yes\"}")
            .build(response);
    });
    PNThread::start();
}

void Teul::uWeb::Server::onCancel()
{
    // Close the socket so that the 'accept()' loop breaks and the thread can tell it needs to terminate.
    SOCKET socket = m_listenSock.exchange(INVALID_SOCKET);

    OS_CLOSESOCKET(socket);
    std::this_thread::yield();
}

void Teul::uWeb::Server::run()
{
	m_listenSock = _establishListenSocket(port());
	if (m_listenSock == INVALID_SOCKET) {
		return;
    }

	sockaddr_in addr;
	pollfd listen_pollfd{ m_listenSock, POLLIN | POLLPRI | POLLERR | POLLHUP | POLLNVAL, 0 };

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
		SOCKET newSock = accept(m_listenSock, reinterpret_cast<sockaddr *>(&addr), &sockLen);
        
		if (newSock < 0) {
            checkForCancellation(OS_CLOSESOCKET(newSock));
            dbugLog(_FLI, "uWebServer error on accept: %d (%d)", newSock, errno);
			continue;
		}
        
        std::unordered_set<Client>::iterator it;
        {
            std::lock_guard<std::mutex> lock(m_clientList_mutex);
            std::pair<std::unordered_set<Client>::iterator, bool> itp = m_clientList.emplace(*this, newSock);
            
            ABORT_IF_EX(itp.second != true, "Emplacing a new client in the set failed?");
            it = itp.first;
        }
        const_cast<Client &>(*it).start();
	}
}

void Teul::uWeb::Server::client_unregisterClient(Teul::uWeb::Client &client)
{
    std::lock_guard<std::mutex> lock(m_clientList_mutex);
    
    m_clientList.erase(client);
}

void Teul::uWeb::Server::indexHandler(const HTTPRequest &request, HTTPResponse &response)
{
    HTTPResponseBuilder(request)
        .status(200)
        .header("Access-Control-Allow-Origin", "*")
        .JSON(m_resourceList)
        .build(response);
}

void Teul::uWeb::Server::physicalHandler(const HTTPRequest &request, HTTPResponse &response)
{
    ABORT_IF_EX(request.path.length() < 1, "Request path should never be empty, something's wrong in the request parser");
    
    if (request.path[0] != '/') {
        throw WebError(400);
    }
    
    std::string::size_type separatorLoc = request.path.find_first_of("#?");
    std::string rawVirtualPath = request.path.substr(0, separatorLoc); // substr(0,npos) returns whole string
    
    // Disallow encoded path separator in path
    if (rawVirtualPath.find("%2F") != std::string::npos || rawVirtualPath.find("%2f") != std::string::npos) {
        throw WebError(400);
    }
    
    std::vector<std::string> componentizedVirtualPath = split_string(rawVirtualPath, "/");
    std::vector<std::string> decodedVirtualPathComponents;
    std::transform(componentizedVirtualPath.begin(), componentizedVirtualPath.end(), std::back_inserter(decodedVirtualPathComponents),
        [] (const std::string &p) { return URLDecode(p); }
    );
    std::string decodedVirtualPath = "/" + std::accumulate(decodedVirtualPathComponents.begin(), decodedVirtualPathComponents.end(),
        std::string(), [] (const std::string &a, const std::string &b) { return a + (a.length() ? "/" : "") + b; }
    );
    std::string sanitizedVirtualPath = string_replace(decodedVirtualPath, "/../", "");
    
    if (sanitizedVirtualPath.rfind("/..") == sanitizedVirtualPath.length() - 3) {
        sanitizedVirtualPath = sanitizedVirtualPath.substr(0, sanitizedVirtualPath.length() - 3);
    }
    
    std::string platformVirtualPath = string_replace(sanitizedVirtualPath, "/", PATH_SEPARATOR);
    
    // The actual check for a 404 won't happen until the sender routines attempt
    // to send the response.
    return HTTPResponseBuilder(request)
        .status(200)
        .header("Access-Control-Allow-Origin", "*")
        .file(platformVirtualPath)
        .build(response);
}

Teul::uWeb::Server::VirtualFileHandler Teul::uWeb::Server::client_findHandlerForRequest(const Teul::uWeb::HTTPRequest &request)
{
    {
        std::shared_lock<std::shared_timed_mutex> lock(m_config_mutex);
        auto it = m_virtualFileHandlers.find(request.path);

        if (it != m_virtualFileHandlers.end()) {
            return it->second.first;
        }
    }
    
    if (request.path.compare("/index.json") == 0 || request.path.compare("/") == 0) {
        return [this] (const HTTPRequest &req, HTTPResponse &response) { indexHandler(req, response); };
    } else {
        return [this] (const HTTPRequest &req, HTTPResponse &response) { physicalHandler(req, response); };
    }
}

//////////////////////////////////////////////////////////////////////
// Register a handler.

void Teul::uWeb::Server::addVirtualFile(std::string desc_, std::string path_, VirtualFileHandler handler_)
{
	std::unique_lock<std::shared_timed_mutex> lock(m_config_mutex);
	m_virtualFileHandlers[path_] = std::make_pair(handler_, desc_);
    
	if (m_resourceList.length() > 1) {
        m_resourceList.erase(m_resourceList.end() - 1);
    }
    m_resourceList += ",{\"url\":\"" + path_ + "\",\"desc\":\"" + desc_ + "\"}]";
}

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

//////////////////////////////////////////////////////////////////////
namespace Teul {
    namespace uWeb {

//////////////////////////////////////////////////////////////////////
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

class SendFailureError : public std::exception
{
};

//////////////////////////////////////////////////////////////////////
// HTTP headers parser
class HTTPRequestHeaderReader
{
    public:
        HTTPRequestHeaderReader(HTTPRequest &request);
    
        bool readMoreData(const std::string &data); // return true if found terminating \r\n\r\n, pass empty data for EOF, throw exception on error
        size_t getBytesUsed() const;
        void validateRequest() const; // Throw exception on error
    
    private:
        HTTPRequest &m_request;
        std::string m_parseBuffer;
        std::string::size_type m_parsePointer;
        bool m_seenHeaderTerminator; // This just bypasses extra logic when an EOF shows up
};

HTTPRequestHeaderReader::HTTPRequestHeaderReader(HTTPRequest &request) :
    m_request(request), m_parseBuffer(), m_parsePointer(), m_seenHeaderTerminator(false)
{
}

bool HTTPRequestHeaderReader::readMoreData(const std::string &data)
{
    #define bad_request_if(cond) do { \
        if ((cond)) { \
            throw WebError(400); \
        } \
    } while (0)
    
    // If we got EOF, there's nothing to yield for, throw bad request immediately
    #define yield_if(cond) do { \
        bad_request_if(data.length() == 0); \
        if ((cond)) { \
            return false; \
        } \
    } while (0)
    
    m_parseBuffer += data; // Yes this is an "unnecessary" data copy, but it makes things a lot easier.
    
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
    // (therefore orderedHeaders.size() == 0 is not a sufficient check)
    while (!m_seenHeaderTerminator) {
        std::string::size_type nextCrlf = m_parseBuffer.find("\r\n", m_parsePointer);
        
        yield_if(nextCrlf == std::string::npos);
        if (nextCrlf == m_parsePointer) {
            m_seenHeaderTerminator = true;
        }
        
        std::string rawHeader = m_parseBuffer.substr(m_parsePointer, nextCrlf - m_parsePointer);

        m_parsePointer = nextCrlf + 2;
        if (rawHeader.length() == 0) { // Putting this here avoids repeating the update of m_parsePointer
            ABORT_IF_EX(!m_seenHeaderTerminator, "How can we have an empty header (next CRLF was right there) but not have marked it as terminated?");
            break;
        }
        ABORT_IF_EX(m_seenHeaderTerminator, "How can we have marked headers as terminated but have a non-empty header?");
        
        std::string::size_type separatorPos = rawHeader.find(": ");
        
        bad_request_if(separatorPos == std::string::npos || separatorPos == 0); // Missing or empty header name
        
        std::string headerName = rawHeader.substr(0, separatorPos);
        std::string headerValue = rawHeader.substr(separatorPos + 2);
        std::string lowerHeaderName;
        
        std::transform(headerName.begin(), headerName.end(), std::back_inserter(lowerHeaderName), [] (char c) { return tolower(c); });
        m_request.orderedHeaders.push_back(rawHeader);
        m_request.headers.insert({ lowerHeaderName, headerValue });
    }
    
    return m_seenHeaderTerminator;
    
    #undef yield_if
    #undef bad_request_if
}

size_t HTTPRequestHeaderReader::getBytesUsed() const
{
    // It just so happens that on successful parse stages, we always update the
    // parse pointer to the next byte we'd check for the next parse. Which means
    // that at the end of a parse, it also tells us exactly how many bytes were
    // consumed during said parsing. Remember, this is bytes, not characters.
    // Unicode stuff comes WAY further down the line of processing than this.
    return m_parsePointer;
}

void HTTPRequestHeaderReader::validateRequest() const
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
    auto cl = m_request.headers.find("content-length");
    auto te = m_request.headers.find("transfer-encoding");

    // Method must be recognized
    if (std::find(s_knownMethods.begin(), s_knownMethods.end(), strtoupper(m_request.method)) == s_knownMethods.end()) {
        throw WebError(405);
    }
    // Version must be 1.0 or 1.1
    if (m_request.version.compare("HTTP/1.0") != 0 && m_request.version.compare("HTTP/1.1") != 0) {
        throw WebError(505);
    }
    // HTTP/1.1 must include a Host header
    if (m_request.version == "HTTP/1.1" && m_request.headers.find("host") == m_request.headers.end()) {
        throw WebError(400);
    }
    // Transfer-Encoding is not valid in HTTP/1.0
    if (te != m_request.headers.end() && m_request.version == "HTTP/1.0") {
        throw WebError(400);
    }
    // Transfer-Encoding & Content-Length both existing is an error
    if (cl != m_request.headers.end() && te != m_request.headers.end()) {
        throw WebError(400);
    }
    // Content-Length must be purely numeric
    if (cl != m_request.headers.end() && std::find_if(cl->second.begin(), cl->second.end(), [] (char c) { return !isdigit(c); }) != cl->second.end()) {
        throw WebError(400);
    }
}

//////////////////////////////////////////////////////////////////////
// HTTP body parser
class HTTPRequestBodyReader
{
    public:
        HTTPRequestBodyReader() : m_bufferedBody() {}
    
        virtual bool readMoreData(const std::string &data) = 0; // return true if body complete, pass empty data for "EOF received", throw exception on error
        std::string &getFinalRequestBody() { return m_bufferedBody; } // Purposely not const so main request parser can move instead of copy
        virtual size_t getUsedBytes() const = 0;
    protected:
        std::string m_bufferedBody;
};

// For requests with no Content-Length and no chunked encoding.
class HTTPBlindRequestBodyReader : public HTTPRequestBodyReader
{
    public:
        HTTPBlindRequestBodyReader() : HTTPRequestBodyReader(), m_seenBytes(0) {}
        virtual bool readMoreData(const std::string &data) override {
            m_bufferedBody += data;
            m_seenBytes += data.length();
            return data.length() == 0;
        }
        virtual size_t getUsedBytes() const override { return m_seenBytes; }
    private:
        size_t m_seenBytes;
};

// For requests with Content-Length
class HTTPDefinedRequestBodyReader : public HTTPRequestBodyReader
{
    public:
        HTTPDefinedRequestBodyReader(size_t definedLength) : HTTPRequestBodyReader(), m_definedLength(definedLength) {}
        virtual bool readMoreData(const std::string &data) override {
            if (data.length() > 0) {
                size_t meaningfulLength = m_definedLength - m_bufferedBody.length();
                
                m_bufferedBody += data.substr(0, std::min(meaningfulLength, data.length()));
            }
            ABORT_IF_EX(m_bufferedBody.length() > m_definedLength, "Someone made a math error, actual buffered data should never exceed desired amount");
            return m_bufferedBody.length() == m_definedLength;
        }
        virtual size_t getUsedBytes() const override { return m_definedLength; }
    private:
        size_t m_definedLength;
};

// For chunked transfer-encoding requests
class HTTPChunkedRequestBodyReader : public HTTPRequestBodyReader
{
    public:
        HTTPChunkedRequestBodyReader() :
            HTTPRequestBodyReader(),
            m_state(State::ReadingChunkSize), m_parseBuffer(), m_parsePointer(0), m_currentChunkLength(0) {}
        virtual bool readMoreData(const std::string &data) override;
        virtual size_t getUsedBytes() const override { return m_parsePointer; } // used bytes != result length in chunked encoding
    private:
        enum class State : uint32_t {
            ReadingChunkSize,
            ReadingChunkExtensionName,
            ReadingChunkExtensionValue,
            ReadingChunkQuotedExtensionValue,
            ReadingChunkSizeDivider,
            ReadingChunkData,
            ReadingChunkTerminator,
            ReadingChunkTrailers,
            ReadingChunkEnding, // trailing CRLF
        };
        State m_state;
        std::string m_parseBuffer;
        size_t m_parsePointer;
        size_t m_currentChunkLength;
};

// Kids watching at home; Do not implement a lexical parser this way.
// This is terrible.
bool HTTPChunkedRequestBodyReader::readMoreData(const std::string &data)
{
    // A coroutine would be really nice here, but since we don't have those in
    // C++14, we go through a painful state machine instead. I was rather
    // tempted to use what Tom Duff, inventor of Duff's device, called a "trick
    // for interrupt-driven state machines that is too horrible to go into".
    // Simon Tatham, inventor of NASM and developer of PuTTY, claims his use of
    // it therein as "the worst piece of C hackery ever seen in serious
    // production code". Don't believe me? Here's the bibliography:
    // http://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
    // http://brainwagon.org/2005/03/05/coroutines-in-c/#comment-1878
    
	#define bad_request_if(cond) do { \
		if ((cond)) { \
			throw WebError(400); \
		} \
	} while (0)
	
	// If we got EOF, there's nothing to yield for, throw bad request immediately
	#define yield_if(cond) do { \
		bad_request_if(data.length() == 0); \
		if ((cond)) { \
			return false; \
		} \
	} while (0)
	
    static const std::string HTTPHexDigChars = "ABCDEFabcdef0123456789"; // "HEXDIG" ABNF
	static const std::string HTTPTokenChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!#$%^'*+-.^_`|-"; // "tchar" ABNF
	static const std::string HTTPQDChars = "\t !#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^_`abcdefghijklmnopqrstuvwxyz{|}"; // "qdchar" ABNF
	
	m_parseBuffer += data;
	while (m_parsePointer < m_parseBuffer.length()) {
		switch (m_state) {
			case State::ReadingChunkSize: { //1*HEXDIG
				std::string::size_type nextNonhex = m_parseBuffer.find_first_not_of(HTTPHexDigChars, m_parsePointer);
				
				yield_if(nextNonhex == std::string::npos);
				bad_request_if(nextNonhex == m_parsePointer);
				bad_request_if(m_parseBuffer[nextNonhex] != ';' && m_parseBuffer[nextNonhex] != '\r');
				ABORT_IF_EX(nextNonhex < m_parsePointer, "Can't seek backwards!");
				
				m_currentChunkLength = std::stoull(m_parseBuffer.substr(m_parsePointer, nextNonhex - m_parsePointer), nullptr, 16);
				m_parsePointer = nextNonhex;
				if (m_parseBuffer[nextNonhex] == ';') {
					m_state = State::ReadingChunkExtensionName;
				} else if (m_parseBuffer[nextNonhex] == '\r') {
					m_state = State::ReadingChunkSizeDivider;
				}
				break;
			}
			case State::ReadingChunkExtensionName: { //*( ";" chunk-ext-name [ "=" chunk-ext-val ] )
				ABORT_IF_EX(m_parseBuffer[m_parsePointer] != ';', "State machine shouldn't get here without a ; showing");
				std::string::size_type nextToken = m_parseBuffer.find_first_not_of(HTTPTokenChars, m_parsePointer + 1);
				
				yield_if(nextToken == std::string::npos);
				bad_request_if(nextToken == m_parsePointer + 1);
				bad_request_if(m_parseBuffer[nextToken] != '=' && m_parseBuffer[nextToken] != ';' && m_parseBuffer[nextToken] != '\r');
				ABORT_IF_EX(nextToken < m_parsePointer + 1, "Can't seek backwards!");
				
				// We don't recognize any chunk extensions, spec says ignore them
				dbugLog(_FLD, "uWebServerClient: Ignoring chunk-extension with name %s", m_parseBuffer.substr(m_parsePointer + 1, nextToken - m_parsePointer - 1).c_str());
				m_parsePointer = nextToken;
				if (m_parseBuffer[nextToken] == '=') {
					m_state = State::ReadingChunkExtensionValue;
				} else if (m_parseBuffer[nextToken] == ';') {
					m_state = State::ReadingChunkExtensionName; // no change
				} else if (m_parseBuffer[nextToken] == '\r') {
					m_state = State::ReadingChunkSizeDivider;
				}
				break;
			}
			case State::ReadingChunkExtensionValue: { //token / quoted-string
				ABORT_IF_EX(m_parseBuffer[m_parsePointer] != '=', "State machine shouldn't get here without an = showing");
                std::string::size_type nextToken = m_parseBuffer.find_first_not_of(HTTPTokenChars, m_parsePointer + 1);
                
                yield_if(nextToken == std::string::npos);
                bad_request_if(m_parseBuffer[nextToken] != '"' && m_parseBuffer[nextToken] != ';' && m_parseBuffer[nextToken] != '\r');
                bad_request_if(m_parseBuffer[nextToken] == '"' && nextToken != m_parsePointer + 1);
                ABORT_IF_EX(nextToken < m_parsePointer + 1, "Can't seek backwards");
                
                // We don't recognize any chunk extensions, spec says ignore them
                if (m_parseBuffer[nextToken] != '"') {
                    dbugLog(_FLD, "uWebServerClient: Ignore chunk-ext with value %s", m_parseBuffer.substr(m_parsePointer + 1, nextToken - m_parsePointer - 1).c_str());
                }
                m_parsePointer = nextToken;
                if (m_parseBuffer[nextToken] == '"') {
                    m_state = State::ReadingChunkQuotedExtensionValue;
                } else if (m_parseBuffer[nextToken] == ';') {
                    m_state = State::ReadingChunkExtensionName;
                } else if (m_parseBuffer[nextToken] == '\r') {
                    m_state = State::ReadingChunkSizeDivider;
                }
				break;
			}
			case State::ReadingChunkQuotedExtensionValue: { //quoted-string
				ABORT_IF_EX(m_parseBuffer[m_parsePointer] != '"', "State machine shouldn't get here without a \" showing");
				yield_if(m_parsePointer == m_parseBuffer.length() - 1);
				std::string::size_type nextNonQD = m_parsePointer;
				
				do {
					nextNonQD = m_parseBuffer.find_first_not_of(HTTPQDChars, nextNonQD + 1);
					yield_if(nextNonQD == std::string::npos);
					bad_request_if(m_parseBuffer[nextNonQD] != '"' && m_parseBuffer[nextNonQD] != '\\');
					yield_if(nextNonQD == m_parseBuffer.length() - 1); // Need something to follow the escape or lookahead to follow closing quote
					if (m_parseBuffer[nextNonQD] == '\\') {
						nextNonQD += 2; // skip escape and whatever it excaped, go around again
					} else if (m_parseBuffer[nextNonQD] == '"') {
						dbugLog(_FLD, "uWebServerClient: Ignore chunk-ext with value \"%s\"", m_parseBuffer.substr(m_parsePointer + 1, nextNonQD - m_parsePointer - 1).c_str());
						m_parsePointer = nextNonQD + 1;
						bad_request_if(m_parseBuffer[m_parsePointer] != ';' && m_parseBuffer[m_parsePointer] != '\r');
						if (m_parseBuffer[m_parsePointer] == ';') {
							m_state = State::ReadingChunkExtensionName;
						} else if (m_parseBuffer[m_parsePointer] == '\r') {
							m_state = State::ReadingChunkSizeDivider;
						}
					}
				} while (nextNonQD >= m_parsePointer); // Stop once we've advanced the parse pointer
				break;
			}
			case State::ReadingChunkSizeDivider: { //CRLF
				ABORT_IF_EX(m_parseBuffer[m_parsePointer] != '\r', "State machine shouldn't get here without a \\r showing");
				yield_if(m_parsePointer == m_parseBuffer.length() - 1);
				bad_request_if(m_parseBuffer[m_parsePointer + 1] != '\n');
				m_parsePointer += 2;
				if (m_currentChunkLength == 0) {
					m_state = State::ReadingChunkTrailers;
				} else {
					m_state = State::ReadingChunkData;
				}
				break;
			}
			case State::ReadingChunkData: { //1*OCTET ; a sequence of chunk-size octets
				yield_if(m_parseBuffer.length() - m_parsePointer < m_currentChunkLength);
				m_bufferedBody += m_parseBuffer.substr(m_parsePointer, m_currentChunkLength);
				m_parsePointer += m_currentChunkLength;
				m_state = State::ReadingChunkTerminator;
				break;
			}
			case State::ReadingChunkTerminator: { //CRLF
				yield_if(m_parseBuffer.length() - m_parsePointer < 2);
				bad_request_if(m_parseBuffer[m_parsePointer] != '\r' || m_parseBuffer[m_parsePointer + 1] != '\n');
				m_parsePointer += 2;
				if (m_currentChunkLength == 0) {
					m_state = State::ReadingChunkEnding;
				} else {
					m_state = State::ReadingChunkSize;
				}
				break;
			}
			case State::ReadingChunkTrailers: { //*( header-field CRLF )
				std::string::size_type nextCrlf = m_parseBuffer.find("\r\n", m_parsePointer);
				
				yield_if(nextCrlf == std::string::npos);
				if (nextCrlf == m_parsePointer) {
					m_state = State::ReadingChunkEnding;
				} else {
					// Spec says implementations MAY ignore trailing headers. So we do.
					dbugLog(_FLD, "uWebServerClient: Ignoring trailer header %s", m_parseBuffer.substr(m_parsePointer, nextCrlf - m_parsePointer).c_str());
					m_parsePointer = nextCrlf + 2;
				}
				break;
			}
			case State::ReadingChunkEnding: { //CRLF
				yield_if(m_parseBuffer.length() - m_parsePointer < 2);
				bad_request_if(m_parseBuffer[m_parsePointer] != '\r' || m_parseBuffer[m_parsePointer + 1] != '\n');
				m_parsePointer += 2;
				// Done parsing!
				return true;
			}
			default: // exhaustive
				LogAbort("Invalid FSM state %u in chunked body reader", m_state);
		}
	}
	return false;
	#undef yield_if
	#undef bad_request_if
}

//////////////////////////////////////////////////////////////////////
// HTTP complete request parser
HTTPRequestParser::HTTPRequestParser() :
    m_bytesSoFar(),
    m_request(),
    m_headersParser(),
    m_bodyParser(),
    m_state(State::Ready)
{
    reset();
}

bool HTTPRequestParser::parseMoreData(const std::string &data)
{
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
    
    m_bytesSoFar += data.length();
    dbugLog(_FLD, "uWebServerClient: Parser parsing %zu bytes data", data.length());
    
    std::string::size_type dataStart = 0;
    
    // All state cases are fallthrough - the only purpose of the state machine
    // is to compensate for the lack of a true "yield" primitive (this not being
    // a coroutine). This is a very simple linear progression.
    switch (m_state) {
        case State::Ready:
            m_headersParser = std::make_unique<HTTPRequestHeaderReader>(m_request);
            m_state = State::ParsingHeaders;
            FALLTHROUGH;
        case State::ParsingHeaders: {
            yield_if(!m_headersParser->readMoreData(data));
            m_headersParser->validateRequest();
            
            auto transferEncoding = m_request.headers.find("transfer-encoding");
            auto contentLength = m_request.headers.find("content-length");
            
            if (transferEncoding != m_request.headers.end()) {
                bad_request_if(contentLength != m_request.headers.end());
                bad_request_if(transferEncoding->second != "chunked"); // we should eventually support gzip,compress,deflate and multiple TEs
                m_bodyParser = std::make_unique<HTTPChunkedRequestBodyReader>();
            } else if (contentLength != m_request.headers.end()) {
                bad_request_if(contentLength->second.find_first_not_of("0123456789") != std::string::npos);
                m_bodyParser = std::make_unique<HTTPDefinedRequestBodyReader>(std::stoull(contentLength->second, nullptr, 10));
            } else {
                m_bodyParser = std::make_unique<HTTPBlindRequestBodyReader>();
            }
            
            ABORT_IF_EX(m_headersParser->getBytesUsed() > m_bytesSoFar, "Headers parsing can't use more bytes than have been received!");
            std::string::size_type leftoverBytes = m_bytesSoFar - m_headersParser->getBytesUsed();
            ABORT_IF_EX(leftoverBytes > data.length(), "It should not be possible to have more bytes leftover from parsing headers than the most recent buffer.");
            
            dataStart = data.length() - leftoverBytes;
            m_state = State::ParsingBody;
            FALLTHROUGH;
        }
        case State::ParsingBody:
            yield_if(!m_bodyParser->readMoreData(dataStart == 0 ? data : data.substr(dataStart)));
            m_request.requestData = std::move(m_bodyParser->getFinalRequestBody());
            m_state = State::Complete;
            return true; // falling through here would hit the "throw for excess data" logic, see below
        case State::Complete:
            // Nothing to do. We treat this as a bad request because the body
            // reader should have returned true to get us to this state and any
            // data read after an explicit end of body makes no sense.
            throw WebError(400);
        default:
            LogAbort("Invalid request parser state %u", static_cast<uint8_t>(m_state));
    }
    #undef yield_if
    #undef bad_request_if
}

size_t HTTPRequestParser::getBytesUnused() const
{
    switch (m_state) {
        case State::Ready:
            return m_bytesSoFar;
        case State::ParsingHeaders:
            return m_bytesSoFar - m_headersParser->getBytesUsed();
        case State::ParsingBody:
            return m_bytesSoFar - (m_headersParser->getBytesUsed() + m_bodyParser->getUsedBytes());
        case State::Complete:
            return m_bytesSoFar - (m_headersParser->getBytesUsed() + m_bodyParser->getUsedBytes());
    }
}

void HTTPRequestParser::reset()
{
    m_state = State::Ready;
    m_headersParser.reset();
    m_bodyParser.reset();
    m_bytesSoFar = 0;
    m_request = HTTPRequest();
}

//////////////////////////////////////////////////////////////////////
// Response builder

HTTPResponseBuilder::HTTPResponseBuilder()
    : m_response()
{
    m_response.version = "HTTP/1.1";
}

HTTPResponseBuilder::HTTPResponseBuilder(const HTTPRequest &originalRequest)
    : HTTPResponseBuilder()
{
    m_response.version = originalRequest.version;
}

HTTPResponseBuilder &HTTPResponseBuilder::status(int responseCode, const std::string &statusMessage)
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

HTTPResponseBuilder &HTTPResponseBuilder::header(const std::string &headerName, const std::string &headerValue, bool replace)
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

HTTPResponseBuilder &HTTPResponseBuilder::content(const std::string &data, bool suppressAutoContentLength, bool append)
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

HTTPResponseBuilder &HTTPResponseBuilder::JSON(const std::string &JSONData)
{
    return this->header("Content-Type", "application/json", true).content(JSONData, false, false);
}

HTTPResponseBuilder &HTTPResponseBuilder::file(const std::string &filePath, bool autodetectContentType)
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


const HTTPResponse &HTTPResponseBuilder::getResponse() const
{
    return m_response;
}

HTTPResponse HTTPResponseBuilder::copyResponse()
{
    return m_response;
}

void HTTPResponseBuilder::build(HTTPResponse &result)
{
    result = std::move(m_response);
}

//////////////////////////////////////////////////////////////////////
// Client thread
Client::Client(Server &owner, SOCKET fd) :
    m_owner(owner),
    m_myIterator(),
    m_clientSock(fd),
    m_requestParser(),
    m_currentRequestStart(),
    m_transferBuffer(2048, '\0')
{
}

Client::~Client()
{
    dbugLog(_FLD, "destroy client %p", this);
}

void Client::start(ClientList::iterator &myIterator)
{
    m_myIterator = myIterator;
    PNThread::start();
}

void Client::onCancel()
{
    SOCKET socket = m_clientSock.exchange(INVALID_SOCKET);

    OS_CLOSESOCKET(socket);
    std::this_thread::yield();
}

bool Client::pollSocket()
{
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms741669(v=vs.85).aspx says:
    // "If the POLLPRI flag is set on a socket for the Microsoft Winsock provider, the WSAPoll function will fail."
    pollfd pfd{ m_clientSock, POLLIN, 0 };
    int pollResult = poll(&pfd, 1, 100);
    
    if (pollResult == SOCKET_ERROR) {
        // If the error was "try again" or "oops, syscall interrupt", just poll again
        if (dbugGetErrno() != EAGAIN && dbugGetErrno() != EINTR) {
            dbugLog(_FLN, "uWebServerClient: got error %d on " PRF_SOCKET ", closing", dbugGetErrno(), static_cast<SOCKET>(m_clientSock));
            cancel();
        }
        return false;
    } else if (pollResult == 0) {
        return false;
    }
    
    // This is really just here for yucks. No one uses URG. What do you think
    // this is, FTP? Telnet? Also we didn't set POLLPRI above because Windows.
    if ((pfd.revents & POLLPRI) != 0) {
        dbugLog(_FLW, "uWebServerClient: Got priority(?) data on " PRF_SOCKET ", this is not HTTP, closing", static_cast<SOCKET>(m_clientSock));
        cancel();
        return false;
    }
    if ((pfd.revents & POLLRDBAND) != 0) {
        dbugLog(_FLW, "uWebServerClient: Got OOB/URG data on " PRF_SOCKET ", this is not HTTP, closing", static_cast<SOCKET>(m_clientSock));
        cancel();
        return false;
    }
    
    if ((pfd.revents & POLLNVAL) != 0) {
        dbugLog(_FLW, "uWebServerClient: " PRF_SOCKET " is invalid? Giving up.", static_cast<SOCKET>(m_clientSock));
        cancel();
        return false;
    }
    
    if ((pfd.revents & POLLERR) != 0) {
        dbugLog(_FLW, "uWebServerClient: Received socket-level error notification on " PRF_SOCKET ", closing", static_cast<SOCKET>(m_clientSock));
        cancel();
        return false;
    }
    
    if ((pfd.revents & POLLHUP) != 0) {
        dbugLog(_FLD, "uWebServerClient: Received transport-layer disconnect on " PRF_SOCKET, static_cast<SOCKET>(m_clientSock));
        return true; // DON'T cancel here! EOF is not an immediate shutdown condition, especially in Connection: close HTTP/1.0, return true to process an EOF buffer
    }
    
    // Oh, and just for good measure...
    if ((pfd.revents & POLLRDNORM) == 0 && (pfd.revents & POLLIN) == 0) {
        dbugLog(_FLM, "uWebServerClient: poll() on " PRF_SOCKET " returned with no data and no timeout? What?", static_cast<SOCKET>(m_clientSock));
        cancel();
        return false;
    }
    
    return true;
}

size_t Client::readData()
{
    ssize_t recvResult = recv(m_clientSock, &m_transferBuffer[0], m_transferBuffer.length(), 0);
    
    if (recvResult < 0) {
        dbugLog(_FLI, "uWebServerClient: recv error %d on " PRF_SOCKET ", closing", dbugGetErrno(), static_cast<SOCKET>(m_clientSock));
        cancel();
        return 0;
    }
    if (recvResult == 0) {
        dbugLog(_FLD, "uWebServerClient: received EOF on " PRF_SOCKET, static_cast<SOCKET>(m_clientSock));
        return 0;
    }
    
    // We already checked that the result is greater than zero, and it is
    // guaranteed that sizeof(size_t) >= sizeof(ssize_t), so this cast is safe.
    return static_cast<size_t>(recvResult);
}

void Client::handleData(size_t nbytes)
{
    HTTPResponse response;
    
    try {
        if (!m_requestParser.parseMoreData(m_transferBuffer)) {
            if (nbytes == 0) {
                cancel(); // Not gonna get any more, give up
            }
            return; // Still waiting for more request pieces
        }

        // Got a complete request
        Server::VirtualHandlerEntry handler = m_owner.client_findHandlerForRequest(m_requestParser.getRequest());
        
        ABORT_IF_EX(!handler.handler, "No handler, not even the default one?");
        handler.handler(m_requestParser.getRequest(), response);
        
        checkForCancellation();
        sendResponse(response); // Sending a file-based response can throw errors
        dbugLog(_FLD, "uWebServerClient: Client %d sent %d for %s %s",
            static_cast<SOCKET>(m_clientSock), response.responseCode,
            m_requestParser.getRequest().method.c_str(), m_requestParser.getRequest().path.c_str());
        
        if (m_requestParser.getRequest().version == "HTTP/1.1") {
            // HTTP/1.1 defaults to keep-alive and closes with Connection: close or blind request body
            auto it = m_requestParser.getRequest().headers.find("connection");
            if (it != m_requestParser.getRequest().headers.end() && strtolower(it->second) == "close") {
                cancel();
            } else if (std::find(response.orderedHeaders.begin(), response.orderedHeaders.end(), "Connection: close") != response.orderedHeaders.end()) {
                cancel(); // Close if the response explicitly closed the connection
            } else if (response.responseCode > 399) {
                cancel(); // Close for any response code in the 4xx or 5xx ranges
            } else {
                // There may be more data that we didn't use in processing the
                // current completed request. Only in the case of a keepalive
                // connection does such data matter.
                size_t unusedBytes = m_requestParser.getBytesUnused();

                ABORT_IF_EX(unusedBytes > nbytes, "Remaining unused bytes should never be larger than the last buffer");
                m_requestParser.reset();
                // Recurse - this is a potential DoS vulnerability where too
                // many pipelined requests in one buffer would cause a stack
                // overflow, but even with the shortest possible requests the
                // various protocol buffers should be too small to permit this.
                if (unusedBytes > 0) {
                    m_transferBuffer.erase(0, nbytes - unusedBytes);
                    handleData(unusedBytes);
                }
            }
        } else if (m_requestParser.getRequest().version == "HTTP/1.0") {
            // HTTP/1.0 defaults to close. There is Connection: keep-alive, but its semantics are funny.
            cancel();
        }
    } catch (WebError &e) {
        if (nbytes > 0) { // don't try to send anything if we got EOF
            HTTPResponseBuilder()
                .status(e.m_code, e.m_error)
                .header("Access-Control-Allow-Origin", "*")
                .header("Content-Type", "text/html")
                .header("Content-Encoding", "utf-8")
                .content("<html><head><title>" + e.m_error + "</title></head><body><h1>" + e.m_error + "</h1></body></html>")
                .build(response);
            try {
                sendResponse(response);
            } catch (WebError &e) {
                // If it does for some reason throw, give up.
                dbugLog(_FLA, "uWebServerClient: Sending error response after throw threw again, giving up");
            }
            dbugLog(_FLD, "uWebServerClient: Client %d sent %d for %s %s (error %d %s)",
                static_cast<SOCKET>(m_clientSock), response.responseCode,
                m_requestParser.getRequest().method.c_str(), m_requestParser.getRequest().path.c_str(),
                e.m_code, e.m_error.c_str());
        }
        cancel(); // Always close after error
    }
}

void Client::run()
{
    m_currentRequestStart = std::chrono::steady_clock::now();
    
    while (!isCancelled()) {
        if ((std::chrono::steady_clock::now() - m_currentRequestStart) > std::chrono::seconds(10)) { \
            dbugLog(_FLD, "uWebServerClient: request timeout (10 seconds) on " PRF_SOCKET " exceeded", static_cast<SOCKET>(m_clientSock));
            cancel();
            break;
        }
        
        if (!pollSocket()) {
            continue;
        }
        checkForCancellation();
        m_transferBuffer.resize(2048, '\0');
        
        size_t nbytes = readData();
        
        checkForCancellation();
        m_transferBuffer.resize(nbytes);
        
        if (nbytes > 0) {
            m_currentRequestStart = std::chrono::steady_clock::now(); // we got data, reset timeout
        }
        handleData(nbytes);
    }
    m_owner.client_unregisterClient(m_myIterator); // Nothing else external is called on this object, so invocation of d-tor is safe here.
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

void Client::sendResponse(const HTTPResponse &response)
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

void Server::start(const char* const rootPath_, const unsigned short port_)
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
    addVirtualFile("Index", "/index.json", [this] (const HTTPRequest &request, HTTPResponse &response) { return indexHandler(request, response); });
    addVirtualFile("Index", "/", [this] (const HTTPRequest &request, HTTPResponse &response) { return indexHandler(request, response); });
    PNThread::start();
}

void Server::onCancel()
{
    // Close the socket so that the 'accept()' loop breaks and the thread can tell it needs to terminate.
    SOCKET socket = m_listenSock.exchange(INVALID_SOCKET);

    OS_CLOSESOCKET(socket);
    std::this_thread::yield();
}

void Server::run()
{
	m_listenSock = _establishListenSocket(port());
	if (m_listenSock == INVALID_SOCKET) {
		return;
    }

	sockaddr_in addr;
	pollfd listen_pollfd{ m_listenSock, POLLIN | POLLPRI | POLLERR | POLLHUP | POLLNVAL, 0 };

    while (!isCancelled()) {
        reap();
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
        
        std::unique_ptr<Client> client = std::make_unique<Client>(*this, newSock);
        {
            std::lock_guard<std::mutex> lock(m_clientList_mutex);
            auto it = m_clientList.insert(m_clientList.end(), std::move(client));
            
            (*it)->start(it);
        }
	}
    
    // Destroy all clients
    {
        std::lock_guard<std::mutex> lock(m_clientList_mutex);
        m_defunctClientList.splice(m_defunctClientList.end(), m_clientList);
        reap_unlocked();
    }
}

void Server::reap()
{
    std::lock_guard<std::mutex> lock(m_clientList_mutex);
    
    reap_unlocked();
}

void Server::reap_unlocked()
{
    m_defunctClientList.clear(); // This is all that's needed!
}

void Server::client_unregisterClient(ClientList::iterator &client)
{
    std::lock_guard<std::mutex> lock(m_clientList_mutex);
    
    // Defunct list is required because otherwise a client would try to delete
    // itself while still running on its own thread.
    m_defunctClientList.splice(m_defunctClientList.end(), m_clientList, client);
}

void Server::indexHandler(const HTTPRequest &request, HTTPResponse &response)
{
    if (request.method != "GET") {
        throw WebError(405);
    }
    return HTTPResponseBuilder(request)
        .status(200)
        .header("Access-Control-Allow-Origin", "*")
        .JSON(m_resourceList)
        .build(response);
}

void Server::physicalHandler(const HTTPRequest &request, HTTPResponse &response)
{
    ABORT_IF_EX(request.path.length() < 1, "Request path should never be empty, something's wrong in the request parser");
    
    if (request.path[0] != '/') {
        throw WebError(400);
    }
    if (request.method != "GET") {
        throw WebError(405);
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

Server::VirtualHandlerEntry Server::client_findHandlerForRequest(const HTTPRequest &request)
{
    {
        std::shared_lock<std::shared_timed_mutex> lock(m_config_mutex);
        
        // Try exact path match first, since that's a simple hash lookup. My
        // kingdom for C++17's if (auto foo = ...) construct.
        for (auto it = m_virtualFileHandlers.find(request.path); it != m_virtualFileHandlers.end(); ) {
            return it->second;
        }
        
        // No match by exact path, iterate looking for longest matching prefix.
        // There is a 100% chance there's a more efficient data structure for
        // dealing with this lookup (think trie), but that's more trouble than
        // it's worth for this usage.
        VirtualHandlers::iterator longestMatch = m_virtualFileHandlers.end();
        std::string::const_iterator::difference_type longestMatchLength = 0;
        
        for (auto it = m_virtualFileHandlers.begin(), end = m_virtualFileHandlers.end(); it != end; ++it) {
            if (!it->second.prefixMatch) {
                continue; // Not a prefix match handler, don't even try
            }
            if (it->first.length() > request.path.length()) {
                continue; // The request path is shorter than the prefix to match; this check also makes it safe to use std::mismatch() in C++11 mode
            }
            
            auto mismatchLoc = std::mismatch(it->first.begin(), it->first.end(), request.path.begin(), request.path.end());
            auto matchLength = std::distance(it->first.begin(), mismatchLoc.first);
            
#ifndef NDEBUG
            ABORT_IF_EX(static_cast<std::string::size_type>(matchLength) >= request.path.length(), "Exact match not found by hash lookup?");
#endif
            if (mismatchLoc.first == it->first.end() && matchLength > longestMatchLength) { // If loc in the prefix isn't the end, no match at all
                longestMatchLength = matchLength;
                longestMatch = it;
            }
        }
        if (longestMatch != m_virtualFileHandlers.end()) {
            return longestMatch->second;
        }
    }
    return VirtualHandlerEntry{ [this] (const HTTPRequest &req, HTTPResponse &response) { physicalHandler(req, response); }, false, "physical file lookup" };
}

//////////////////////////////////////////////////////////////////////
// Register a handler.

void Server::addVirtualFile(std::string desc_, std::string path_, VirtualFileHandler handler_)
{
	std::unique_lock<std::shared_timed_mutex> lock(m_config_mutex);
	m_virtualFileHandlers[path_] = VirtualHandlerEntry{ handler_, false, desc_ };
    
    m_resourceList.erase(m_resourceList.end() - 1);
	if (m_resourceList.length() > 2) {
        m_resourceList += ",";
    }
    m_resourceList += "{\"url\":\"" + path_ + "\",\"desc\":\"" + desc_ + "\"}]";
}

void Server::addVirtualPrefix(std::string desc_, std::string prefix_, VirtualFileHandler handler_)
{
	std::unique_lock<std::shared_timed_mutex> lock(m_config_mutex);
	m_virtualFileHandlers[prefix_] = VirtualHandlerEntry{ handler_, true, desc_ };
    
    m_resourceList.erase(m_resourceList.end() - 1);
	if (m_resourceList.length() > 2) {
        m_resourceList += ",";
    }
    m_resourceList += "{\"url\":\"" + prefix_ + "*\",\"desc\":\"" + desc_ + "\"}]";
}

//////////////////////////////////////////////////////////////////////
    } // namespace uWeb
} // namespace Teul

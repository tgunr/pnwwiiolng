#include <syscfg.h> // Path is project specific.
#include <windows.h>
#include <shellapi.h>
#include <direct.h> // _getcwd
#include <string>
#include "sys/sysLaunchUrl.h"
#include "include/vars.h"
#include "dbug/dbug.h"
#include <WinInet.h>
#include <sstream>

static const std::wstring WW2PCClient_userAgent = L"WW2PCClient/0.1 (Windows custom)";

extern HWND	myhwnd;

#define PATH_SIZE 255
static const int MAX_HEADER_SIZE = 1024;
static const int MAX_RESPONSE_LENGTH = 4096;

LOGICAL syslaunchurl(const char* url, LOGICAL local)
{
	std::string destination;

	// if we're local we'll lookup the working directory and
	// make the url relative to that
	if(local)
	{
		char path[PATH_SIZE + 1] = { 0 };
		if(_getcwd(path, PATH_SIZE))
		{
			destination = "file://";

			destination += path;
			destination += "\\";
			destination += url;
		}
	}
	else
	{
		destination = url;
	}

	if(ShellExecuteA(nullptr, "open", destination.c_str(), nullptr, nullptr, SW_SHOWNORMAL))
	{
		ShowWindow(myhwnd, SW_MINIMIZE);
		return 1;
	}

	return 0;
}

// Some inline utilities for trimming
static inline void ltrim(std::string &s) 
{
	s.erase(s.begin(), 
		std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
}

static inline void rtrim(std::string &s) 
{
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), 
		s.end());
}

static inline void trim(std::string &s) 
{
	ltrim(s);
	rtrim(s);
}

bool sysExecuteHTTPRequest(const sysBasicHTTPRequest &request, sysBasicHTTPResponse &response)
{
	// local storage for breaking down the request URL
	char hostName[100];
	char path[200];
	char scheme[40];

	URL_COMPONENTSA components;
	memset(&components, 0, sizeof(components));
	components.dwStructSize = sizeof(components);
	components.lpszHostName = hostName;
	components.lpszUrlPath = path;
	components.dwHostNameLength = sizeof(hostName);
	components.dwUrlPathLength = sizeof(path);
	components.lpszScheme = scheme;
	components.dwSchemeLength = sizeof(scheme);
	HINTERNET base = NULL, conn = NULL, req = NULL;
	bool success = false;
	DWORD lastError = 0;

	// organize all calls in a try-catch so open handles can be cleaned up without duplicating too much code
	try {
		if (!InternetCrackUrlA(request.url.c_str(), 0, ICU_DECODE, &components))
		{
			lastError = GetLastError();
			throw std::exception("Unable to parse URL");
		}

		HINTERNET base = InternetOpen(WW2PCClient_userAgent.c_str(), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		if (base == NULL)
		{
			throw std::exception("Unable to get out of the local network");
		}

		HINTERNET conn = InternetConnectA(base, components.lpszHostName, components.nPort, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);
		if (conn == NULL)
		{
			throw std::exception("Unable to establish a connection");
		}

		DWORD flags =
			INTERNET_FLAG_KEEP_CONNECTION |
			INTERNET_FLAG_NO_CACHE_WRITE |
			INTERNET_FLAG_NO_COOKIES |
			INTERNET_FLAG_NO_UI |
			INTERNET_FLAG_PRAGMA_NOCACHE |
			INTERNET_FLAG_RELOAD;

		if (strncmp(scheme, "https", sizeof(scheme))==0)
			flags |= INTERNET_FLAG_SECURE;

		const char *acceptTypes[] = { "*/*", nullptr };
		HINTERNET req = HttpOpenRequestA(conn, request.method.c_str(), components.lpszUrlPath, nullptr, nullptr, acceptTypes, flags, 0);
		if (req == NULL)
		{
			throw std::exception("Unable to open a Http request");
		}

		// compile the headers into a format usable by Wininet
		std::string compiledHeaders;
		for (std::pair<std::string, std::string> element : request.headers)
		{
			compiledHeaders.append(element.first);
			compiledHeaders.append(": ");
			compiledHeaders.append(element.second);
			compiledHeaders.append("\r\n");
		}

		// Use ASCII variant so we don't have to do an extra conversion on the multipart boundary just for the header here.
		if (!HttpSendRequestA(req, compiledHeaders.c_str(), -1, const_cast<char *>(request.content.c_str()), request.content.length())) {
			lastError = GetLastError();
			throw std::exception("Send request failed");
		}

		DWORD status_code = 0, status_code_len = sizeof(status_code);
		if (!HttpQueryInfo(req, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status_code, &status_code_len, 0)) {
			throw std::exception("Unable to get status code");
		}
		response.responseCode = static_cast<int>(status_code);

		DWORD dwHeaderSize = MAX_HEADER_SIZE;
		char tchHeader[MAX_HEADER_SIZE] = { '\0' };
		if (!HttpQueryInfoA(req,
			HTTP_QUERY_RAW_HEADERS_CRLF, tchHeader, &dwHeaderSize, 0))
		{
			throw std::exception("unable to get response headers");
		}
		
		// convert response headers to map
		std::stringstream ss(reinterpret_cast<const char *>(tchHeader));
		std::string nextLine;
		while (std::getline(ss, nextLine))
		{
			std::stringstream sl(nextLine);
			std::string hdrName, hdrVal;
			std::getline(sl, hdrName, ':');
			std::getline(sl, hdrVal, ':'); 
			trim(hdrName);
			trim(hdrVal);

			// make sure we have two keys - otherwise ignore
			if (hdrName.length() && hdrVal.length()) {
				response.headers.insert({ strtolower(hdrName), hdrVal });
			}
		}

		// Only parse the content if there is no error
		if (response.responseCode < 300)
		{
			DWORD dwLength = 0;
			DWORD dwLengthSize = sizeof(DWORD);

			// Get the content length
			if (!HttpQueryInfo(req, HTTP_QUERY_CONTENT_LENGTH |
				HTTP_QUERY_FLAG_NUMBER, &dwLength, &dwLengthSize, 0))
			{
				lastError = GetLastError();
				throw std::exception("unable to get size of content");
			}

			static char rawResponseBuffer[MAX_RESPONSE_LENGTH];
			DWORD dwRead;
			do {
				dwRead = 0;
				if (!InternetReadFile(req, rawResponseBuffer, MAX_RESPONSE_LENGTH, &dwRead) || dwRead == 0)
				{
					break;
				}

				// Do something with the buffer, and get more data
				success = true;
				response.responseMessage.append(rawResponseBuffer);
				rawResponseBuffer[0] = 0;

			} while (dwRead > 0);
		}
	}
	catch (const std::exception& e)
	{
		dbugLog(_FLW, "Exception while executing http request: %s", e.what());
		if (lastError != 0)
		{
			LPVOID lpMsgBuf;
			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				lastError,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR)&lpMsgBuf,
				0, NULL);
			dbugLog(_FLW, "LastError: %s", lpMsgBuf);
			LocalFree(lpMsgBuf);

		}

	}

	if (base != NULL) InternetCloseHandle(base);
	if (conn != NULL) InternetCloseHandle(conn);
	if (req != NULL) InternetCloseHandle(req);

	return success;
}

#if defined(UNIT_TESTING)
void testHttpRequests()
{
	sysBasicHTTPRequest req;
	sysBasicHTTPResponse resp;

	// To basic case
	req.url = "https://www.apple.com";
	req.method = "GET";
	req.content = "";
	resp.responseMessage = "";
	if (sysExecuteHTTPRequest(req, resp))
	{
		dbugLog(_FLI, "UNIT_TEST: testHttpRequests: Basic: Passed");
		dbugLog(_FLI, "    Code: %d", resp.responseCode);
		dbugLog(_FLI, "    Data: %s", resp.responseMessage);
	}
	else
		dbugLog(_FLI, "UNIT_TEST: testHttpRequests: Basic: FAILED, response code: %d", resp.responseCode);

	req.url = "https://jsonplaceholder.typicode.com/posts/1";
	req.method = "GET";
	req.content = "";
	resp.responseMessage = "";
	if (sysExecuteHTTPRequest(req, resp))
	{
		dbugLog(_FLI, "UNIT_TEST: testHttpRequests: jsonplaceholder GET: Passed");
		dbugLog(_FLI, "    Code: %d", resp.responseCode);
		dbugLog(_FLI, "    Data: %s", resp.responseMessage);
	}
	else
		dbugLog(_FLI, "UNIT_TEST: testHttpRequests: jsonplaceholder GET: FAILED");

	req.url = "https://jsonplaceholder.typicode.com/posts";
	req.method = "POST";
	req.content = "{\"title\":\"foo\",\"body\":\"bar\",\"userId\": 1}";
	resp.responseMessage = "";
	req.headers.insert({ "Content-Type", "application/json; charset=utf-8" });
	if (sysExecuteHTTPRequest(req, resp))
	{
		dbugLog(_FLI, "UNIT_TEST: testHttpRequests: jsonplaceholder POST: Passed");
		dbugLog(_FLI, "    Code: %d", resp.responseCode);
		dbugLog(_FLI, "    Data: %s", resp.responseMessage);
	}
	else
		dbugLog(_FLI, "UNIT_TEST: testHttpRequests: jsonplaceholder POST: FAILED");
}
#endif
#pragma once

#include "systypes.h"
#include <string>
#include <unordered_map>

// launches the default web browser
// returns true if it successfully opened the browser
// if local == true, we mangle the path in a OS dependent way as needed
LOGICAL syslaunchurl(const char* url, LOGICAL local = false);

// Returns the raw response from contacting the given URL with the given HTTP
// method. There are much better nicely-encapsulated types for defining an HTTP
// request's full parameters and response data in macOS code, but since this
// needs to work on Windows as well, the types of URLs and requests this
// interface can handle are pretty limited.
struct sysBasicHTTPRequest {
    std::string method;
    std::string url; // full URL, not just path
    std::unordered_map<std::string, std::string> headers;
    std::string content;
};

struct sysBasicHTTPResponse {
    int responseCode;
    std::string responseMessage;
    std::unordered_map<std::string, std::string> headers;
    std::string content;
};

// The errorCode parameter is unmodified if this function returns true. Otherwise,
// it contains an operating system-specific error code.
bool sysExecuteHTTPRequest(const sysBasicHTTPRequest &request, sysBasicHTTPResponse &response, uint64_t &errorCode);

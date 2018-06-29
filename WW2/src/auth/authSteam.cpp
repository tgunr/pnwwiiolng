//
//  authSteam.cpp
//  WW2OL
//
//  Created by Gwynne Raskind on 5/19/17.
//  Copyright © 2017 Cornered Rat Software - All rights reserved.
//

#include "syscfg.h"
#include "authSteam.h"
#include "sys/sysLaunchUrl.h"
#include "build_info.hpp"
#include "json.hpp"

static const std::string authSteamRESTPrefix = "/v2/";

static inline SteamLoginResponse _loginResponseFromResultCode(uint32_t resultCode)
{
    switch (resultCode) {
        case static_cast<uint32_t>(SteamLoginResponse::LoginSuccess):
        case static_cast<uint32_t>(SteamLoginResponse::LoginNotValidUser):
        case static_cast<uint32_t>(SteamLoginResponse::LoginNewUser):
        case static_cast<uint32_t>(SteamLoginResponse::LoginCallsignInvalid):
        case static_cast<uint32_t>(SteamLoginResponse::LoginCallsignDisallowed):
        case static_cast<uint32_t>(SteamLoginResponse::LoginCallsignInUse):
        case static_cast<uint32_t>(SteamLoginResponse::LoginUnknownError):
        case static_cast<uint32_t>(SteamLoginResponse::LoginEmailRequired):
        case static_cast<uint32_t>(SteamLoginResponse::LoginEmailInvalid):
        case static_cast<uint32_t>(SteamLoginResponse::LoginEmailInUse):
        case static_cast<uint32_t>(SteamLoginResponse::LoginEmailNotValidated):
        case static_cast<uint32_t>(SteamLoginResponse::LoginExistingUser):
            return static_cast<SteamLoginResponse>(resultCode);
        default:
            return SteamLoginResponse::INVALID_RESPONSE_CODE;
    }
}

static inline std::string _getAuthRelayURL(const std::string &endpoint)
{
    // Only allow override on Development/Debug builds
    if (gAuthRelayURL[0] != 0) {
        return gAuthRelayURL + endpoint;
    }

    return std::string("https://secure.wwiionline.com/relay") + authSteamRESTPrefix + endpoint;
}

static std::string _encodeSteamTicket(const void *authTicket, size_t authTicketLen)
{
    ABORT_IF_EX(authTicket == nullptr || authTicketLen < 1, "Steam auth ticket is required");

    const uint8_t *pAuthTicket = static_cast<const uint8_t *>(authTicket), *pAuthEnd = pAuthTicket + authTicketLen;
    std::string result(authTicketLen << 1, 0); // two hex digits per byte
    
    for (size_t i = 0; pAuthTicket < pAuthEnd; i += 2, ++pAuthTicket) {
        result[i + 0] = "0123456789abcdef"[*pAuthTicket >>  4];
        result[i + 1] = "0123456789abcdef"[*pAuthTicket & 0xf];
    }
    
    return result;
}

static inline std::string FORMAT_FN(1, 2) _cxxfmt(FORMAT_ARG(0) const char * const format, ...)
{
    va_list args;
    char *tmp = nullptr;
    std::string result;
    
    va_start(args, format);
    vasprintf(&tmp, format, args);
    va_end(args);
    result.assign(tmp);
    free(tmp);
    return result;
}

static SteamLoginResult _doRequestWithParameters(const std::string &endpoint, nlohmann::json params, bool successReadsData = false) // params is *purposely* not a const&
{
    // Can't make this a static inline function because there's no dbugLogv()!
    #define _requestError(msg, code, ...) do { dbugLog(_FLD, msg, ## __VA_ARGS__); return { SteamLoginResponse::LoginUnknownError, code }; } while (0)
    
    sysBasicHTTPRequest request;
    sysBasicHTTPResponse response;
    
    request.method = "POST";
    request.url = _getAuthRelayURL(endpoint);
    request.headers["Content-Type"] = "application/json";
    request.headers["X-WWIIOnline-Client-Version"] = PN_product_version_string; // Not required, just for good measure
    request.content = params.dump();
    
    if (!sysExecuteHTTPRequest(request, response)) {
        _requestError("Failed to execute HTTP request with URL %s", request.url.c_str());
    }
    if (response.responseCode != 200) {
        _requestError("Server returned non-200 response code %d with URL %s", _cxxfmt("20000000-0000-0000-0000-%012x", response.responseCode), response.responseCode, request.url.c_str());
    }
    if (response.headers.find("x-request-id") == response.headers.end()) {
        _requestError("Server's response has no context ID for URL %s", "30000000-0000-0000-0000-000000000000", request.url.c_str());
    }
    
    std::string requestId = response.headers["x-request-id"];
    
    if (response.headers.find("content-type") == response.headers.end() || !has_prefix(response.headers["content-type"], "application/json")) {
        _requestError("Server returned non-JSON content-type with URL %s", requestId, request.url.c_str());
    }
    
    auto responseJson = nlohmann::json::parse(response.content, nullptr, /*allow_exceptions:*/false);
    if (responseJson.is_discarded() || !responseJson.is_object()) {
        _requestError("Server returned invalid or non-object JSON with URL %s content %s", requestId, request.url.c_str(), response.content.c_str());
    }

    auto resultPtr = responseJson.find("result");
    if (resultPtr == responseJson.end() || !resultPtr->is_number_unsigned()) {
        _requestError("Server returned JSON with no or invalid result code with URL %s content %s", requestId, request.url.c_str(), response.content.c_str());
    }
    SteamLoginResponse resultCode = _loginResponseFromResultCode(resultPtr->get<uint32_t>());
    
    switch (resultCode) {
        case SteamLoginResponse::INVALID_RESPONSE_CODE:
            _requestError("Server returned JSON with unknown result code %u with URL %s content %s", requestId, responseJson["result"].get<uint32_t>(), request.url.c_str(), response.content.c_str());
            
        case SteamLoginResponse::LoginSuccess: {
            if (!successReadsData)  {
                return { resultCode, requestId };
            }
            
            auto arenaIDPtr = responseJson.find("arenaID");
            auto otpPtr = responseJson.find("otp");
            auto serverIPPtr = responseJson.find("serverIP");
            auto serverPortPtr = responseJson.find("serverPort");

            if (arenaIDPtr == responseJson.end() || !arenaIDPtr->is_number_unsigned()) {
                _requestError("Server returned JSON with no or invalid arena ID with URL %s content %s", requestId, request.url.c_str(), response.content.c_str());
            }
            if (otpPtr == responseJson.end() || !otpPtr->is_number_unsigned()) {
                _requestError("Server returned JSON with no or invalid OTP with URL %s content %s", requestId, request.url.c_str(), response.content.c_str());
            }
            if (serverIPPtr == responseJson.end() || !serverIPPtr->is_number_unsigned()) {
                _requestError("Server returned JSON with no or invalid server IP with URL %s content %s", requestId, request.url.c_str(), response.content.c_str());
            }
            if (serverPortPtr == responseJson.end() || !serverPortPtr->is_number_unsigned()) {
                _requestError("Server returned JSON with no or invalid server port with URL %s content %s", requestId, request.url.c_str(), response.content.c_str());
            }

            return { arenaIDPtr->get<int32_t>(), otpPtr->get<uint32_t>(), serverIPPtr->get<uint32_t>(), serverPortPtr->get<uint32_t>() };
        }
        
        default:
            return { resultCode, requestId };
    }
}

SteamLoginResult authRequestSteamLogin(const void *authTicket, size_t authTicketLen, uint32_t arenaID)
{
    uint32_t appVersion = static_cast<uint32_t>(PN_build_version_value);
    
    if (gClientV[0]) {
        teulGetHost(gClientV, &appVersion); // TODO: Anything other than this stupid function
    }
    
    return _doRequestWithParameters("login", {
        { "steam_ticket", _encodeSteamTicket(authTicket, authTicketLen) },
        { "arena_id", arenaID },
        { "app_version", appVersion },
        { "app_platform", platOS },
    }, true);
}

SteamLoginResult authRequestSteamSignup(const void *authTicket, size_t authTicketLen, uint32_t arenaID, const std::string &callsign, const std::string &email)
{
    return _doRequestWithParameters("signup", {
        { "steam_ticket", _encodeSteamTicket(authTicket, authTicketLen) },
        { "callsign", callsign },
        { "email", email },
    });
}

SteamLoginResult authRequestSteamEmailChange(const void *authTicket, size_t authTicketLen, uint32_t arenaID, const std::string &email)
{
    return _doRequestWithParameters("email", {
        { "steam_ticket", _encodeSteamTicket(authTicket, authTicketLen) },
        { "email", email },
    });
}

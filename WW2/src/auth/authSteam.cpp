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

static const std::string authSteamLoginURL = "https://auth.playnet.com/v1/login";

SteamLoginResponse authRequestSteamLogin(const void *authTicket, size_t authTicketLen, uint32_t arenaID, const std::string &requestedCallsign)
{
    ABORT_IF_EX(authTicket == nullptr || authTicketLen < 1, "Steam auth ticket is required");
    
    sysBasicHTTPRequest request;
    sysBasicHTTPResponse response;
    
    request.method = "POST";
	if (!gAuthServer[0])
		request.url = authSteamLoginURL;
	else
	{
		request.url = std::string("https://") + gAuthServer +
			std::string("/v1/login");
	}
    request.headers["Content-Type"] = "application/json";
    request.headers["X-WWIIOnline-Client-Version"] = PN_product_version_string; // Not required, just for good measure
    request.content = std::string("{") + // TODO: Do a real JSON encode here instead of just stringing strings together
        "\"steam_ticket\":" + ""/* hex-encode authTicket */ + ","
        "\"arena_id\":" + std::to_string(arenaID) +
        (requestedCallsign.empty() ? "" : std::string("\"callsign\":\"") + requestedCallsign + "\"") +
    "}";
    
    if (!sysExecuteHTTPRequest(request, response)) {
        // TODO: Better error handling
        return SteamLoginResponse::LoginUnknownError;
    }
    
    if (response.responseCode != 200) {
        // TODO: Logging, better error handling
        return SteamLoginResponse::LoginUnknownError;
    }
    
    if (response.headers.find("content-type") == response.headers.end() || response.headers["Content-Type"] != "application/json") {
        // TODO: Logging, better error handling
        return SteamLoginResponse::LoginUnknownError;
    }
    
    // TODO: Decode response.content JSON and return response code based on it.
    
    return SteamLoginResponse::LoginUnknownError;
}

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

static const std::string authSteamLoginProtocol = "http";
static const std::string authSteamLoginResource = "/v1/login";

SteamLoginResult authRequestSteamLogin(const void *authTicket, size_t authTicketLen, uint32_t arenaID, const std::string &requestedCallsign)
{
    ABORT_IF_EX(authTicket == nullptr || authTicketLen < 1, "Steam auth ticket is required");
    
    sysBasicHTTPRequest request;
    sysBasicHTTPResponse response;
    
    request.method = "POST";
    request.url = authSteamLoginProtocol + "://" + gAuthServer + authSteamLoginResource;
    request.headers["Content-Type"] = "application/json";
    request.headers["X-WWIIOnline-Client-Version"] = PN_product_version_string; // Not required, just for good measure
    
    const unsigned char *pAuthTicket = reinterpret_cast<const unsigned char *>(authTicket);
    std::stringstream steamTicketEncoded;
    for(size_t i=0; i < authTicketLen; i++) {
        int decVal = pAuthTicket[i];
        steamTicketEncoded << std::hex << std::setfill('0') << std::setw(2) << decVal;
    }
    
    request.content = std::string("{") + // TODO: Do a real JSON encode here instead of just stringing strings together
        "\"steam_ticket\":" + "\"" + steamTicketEncoded.str() + "\","
        "\"arena_id\":" + std::to_string(arenaID) + "," +
        "\"app_version\":" + std::to_string(PN_build_version_value) + "," +
        "\"app_platform\":" + std::to_string(platOS - 1) + // HACK!! OS_WIN32 == 1, OS_MACOS == 2, auth platform values are 0 and 1
        (requestedCallsign.empty() ? "" : std::string(",\"callsign\":\"") + requestedCallsign + "\"") +
    "}";
    
    if (!sysExecuteHTTPRequest(request, response)) {
        // TODO: Better error handling
        dbugLog(_FLD, "Failed to execute HTTP request with URL %s", request.url.c_str());
        return { SteamLoginResponse::LoginUnknownError, 0, 0, 0, 0 };
    }
    
    if (response.responseCode != 200) {
        // TODO: Better error handling
        dbugLog(_FLD, "Server returned non-200 response code %d with URL %s", response.responseCode, request.url.c_str());
        return { SteamLoginResponse::LoginUnknownError, 0, 0, 0, 0 };
    }
    
    if (response.headers.find("content-type") == response.headers.end() || response.headers["content-type"] != "application/json") {
        // TODO: Better error handling
        dbugLog(_FLD, "Server returned non-JSON content-type with URL %s", request.url.c_str());
        return { SteamLoginResponse::LoginUnknownError, 0, 0, 0, 0 };
    }
    
    try {
        auto responseJson = nlohmann::json::parse(response.content);
        
        if (!responseJson.is_object()) {
            // TODO: Better error handling
            dbugLog(_FLD, "Server returned non-object JSON with URL %s content %s", request.url.c_str(), response.content.c_str());
            return { SteamLoginResponse::LoginUnknownError, 0, 0, 0, 0 };
        }
        if (responseJson.find("result") == responseJson.end() || !responseJson["result"].is_number_unsigned()) {
            // TODO: Better error handling
            dbugLog(_FLD, "Server returned JSON with no or invalid result code with URL %s content %s", request.url.c_str(), response.content.c_str());
            return { SteamLoginResponse::LoginUnknownError, 0, 0, 0, 0 };
        }
        
        uint64_t resultCode = responseJson["result"];
        
        // TODO: This is a bad API design - it relies on the auth relay using
        // the same numeric values as the client-side enum. In the future this
        // should be improved.
        if (resultCode == static_cast<uint32_t>(SteamLoginResponse::LoginSuccess)) {
            if (responseJson.find("arenaID") == responseJson.end() || !responseJson["arenaID"].is_number_unsigned()) {
                // TODO: Better error handling
                dbugLog(_FLD, "Server returned JSON with no or invalid arena ID with URL %s content %s", request.url.c_str(), response.content.c_str());
                return { SteamLoginResponse::LoginUnknownError, 0, 0, 0, 0 };
            }
            if (responseJson.find("otp") == responseJson.end() || !responseJson["otp"].is_number_unsigned()) {
                // TODO: Better error handling
                dbugLog(_FLD, "Server returned JSON with no or invalid OTP with URL %s content %s", request.url.c_str(), response.content.c_str());
                return { SteamLoginResponse::LoginUnknownError, 0, 0, 0, 0 };
            }
            if (responseJson.find("serverIP") == responseJson.end() || !responseJson["serverIP"].is_number_unsigned()) {
                // TODO: Better error handling
                dbugLog(_FLD, "Server returned JSON with no or invalid server IP with URL %s content %s", request.url.c_str(), response.content.c_str());
                return { SteamLoginResponse::LoginUnknownError, 0, 0, 0, 0 };
            }
            if (responseJson.find("serverPort") == responseJson.end() || !responseJson["serverPort"].is_number_unsigned()) {
                // TODO: Better error handling
                dbugLog(_FLD, "Server returned JSON with no or invalid server port with URL %s content %s", request.url.c_str(), response.content.c_str());
                return { SteamLoginResponse::LoginUnknownError, 0, 0, 0, 0 };
            }
            
            int32_t resultArenaID = responseJson["arenaID"];
            uint32_t resultOTP = responseJson["otp"];
            uint32_t resultServerIP = responseJson["serverIP"];
            uint32_t resultServerPort = responseJson["serverPort"];

            return { SteamLoginResponse::LoginSuccess, resultArenaID, resultOTP, resultServerIP, resultServerPort };
        } else if (resultCode == static_cast<uint32_t>(SteamLoginResponse::LoginNotValidUser)) {
            return { SteamLoginResponse::LoginNotValidUser, 0, 0, 0, 0 };
        } else if (resultCode == static_cast<uint32_t>(SteamLoginResponse::LoginCallsignRequired)) {
            return { SteamLoginResponse::LoginCallsignRequired, 0, 0, 0, 0 };
        } else {
            // TODO: Better error handling
            dbugLog(_FLD, "Server returned JSON with unknown result code with URL %s content %s", request.url.c_str(), response.content.c_str());
            return { SteamLoginResponse::LoginUnknownError, 0, 0, 0, 0 };
        }
    } catch (std::invalid_argument &e) {
        // TODO: Better error handling
        dbugLog(_FLD, "Server returned invalid JSON with URL %s content %s", request.url.c_str(), response.content.c_str());
        return { SteamLoginResponse::LoginUnknownError, 0, 0, 0, 0 };
    } catch (...) {
        // TODO: Narrow this down!
        // TODO: Better error handling
        dbugLog(_FLD, "Server returned something it failed to deal with with URL %s content %s", request.url.c_str(), response.content.c_str());
        return { SteamLoginResponse::LoginUnknownError, 0, 0, 0, 0 };
    }
    
    // TODO: Logging, better error handling
    return { SteamLoginResponse::LoginUnknownError, 0, 0, 0, 0 };
}

/*
	SINT32	arenaID;
	UINT32	sessionID;		//The sessionID is used by the chat server, as
							//	well as any game service that wants sessions
	SINT32	appID;			//Application ID
	SINT32	appVersion;		//Version of the app required
	UINT32	otp;			//One time password, provided authentication was
							//	successful
	UINT32	serverIP;		//IP address of the server to connect to
	UINT32	serverPort;		//port of the game server
	PROTOCOL protocol;		//Protocol ID
	SINT32	status;			// server status < 0 is an error, > 0 is # of players
	SINT32	dataSize;
	SCHAR	appleID[4];
	char	data[MAX_PARAM];		//Application data.  Like parameters and such.

	APPDL_ACTION action;	// what to do with a file that we download
	SINT32	dlAppID;		//Application ID
*/

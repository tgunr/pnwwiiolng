//
//  authSteam.h
//  WW2OL
//
//  Created by Gwynne Raskind on 5/19/17.
//  Copyright © 2017 Cornered Rat Software - All rights reserved.
//

#pragma once

#include <cstdint>
#include <string>

enum class SteamLoginResponse : uint32_t {
    LoginSuccess = 0,
    LoginNotValidUser = 1,
    LoginCallsignRequired = 2, // Callsign is required but was not provided
    LoginCallsignInvalid = 3, // Callsign has invalid characters or is too long etc.
    LoginCallsignDisallowed = 4, // Callsign is on "bad names" list
    LoginCallsignInUse = 5, // Callsign is already in use by someone else
    LoginUnknownError = 6,
};

// Some of these fields loosely correspond to those of auth's APP_LAUNCH packet
struct SteamLoginResult {
    SteamLoginResponse status;
    int32_t arenaID;
    uint32_t otp;
    uint32_t serverIP;
    uint32_t serverPort;
};

// Operates synchronously (your thread will block until the request completes, and this may take awhile)
// It is SAFE to call this API on a secondary thread
// Do not pass a callsign unless you know one is required (i.e. the API returned LoginCallsignRequired in a previous call)
// Passing a callsign when one is not required will cause the API to return a login failure.
SteamLoginResult authRequestSteamLogin(const void *authTicket, size_t authTicketLen, uint32_t arenaID, const std::string &requestedCallsign = "");

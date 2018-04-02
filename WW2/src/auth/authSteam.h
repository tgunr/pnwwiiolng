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

// IMPORTANT: Keep these in sync with authRelay.php!
enum class SteamLoginResponse : uint32_t {
    LoginSuccess = 0,
    LoginNotValidUser = 1,
    LoginNewUser = 2, // The user is new and needs to provide their email, etc.
    LoginCallsignInvalid = 3, // Callsign has invalid characters or is too long etc.
    LoginCallsignDisallowed = 4, // Callsign is on "bad names" list
    LoginCallsignInUse = 5, // Callsign is already in use by someone else
    LoginUnknownError = 6,
    LoginEmailRequired = 7, // User is known but has no email
    LoginEmailInvalid = 8, // Provided email address looks so little like an email that we won't accept it
    LoginEmailInUse = 9, // Email is already in use by someone else
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
// Do not pass a callsign unless you know one is required (i.e. the API returned LoginNewUser in a previous call)
// Passing a callsign when one is not required will cause the API to return a login failure.
// Do not pass an email unless you know one is required (i.e. the API returned LoginNewUser or LoginEmailRequired in a previous call)
// Passing an email when one is not required will cause the API to return a login failure.
// Both email and callsign are REQUIRED for new users. LoginNewUser implies and includes LoginEmailRequired.
SteamLoginResult authRequestSteamLogin(const void *authTicket, size_t authTicketLen, uint32_t arenaID, const std::string &email = "", const std::string &requestedCallsign = "");

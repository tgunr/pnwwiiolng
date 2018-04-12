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
#include <numeric>
#include <limits>

// IMPORTANT: Keep these in sync with authRelay.php
// IMPORTANT: DO NOT REORDER OR RENUMBER THESE - only add new ones
enum class SteamLoginResponse : uint32_t {
    LoginSuccess = 0,
    LoginNotValidUser = 1, // Steam user doesn't own the game
    LoginNewUser = 2, // The user is new and needs to provide both callsign and email
    LoginCallsignInvalid = 3, // Callsign has invalid characters or is too long etc.
    LoginCallsignDisallowed = 4, // Callsign is on "bad names" list
    LoginCallsignInUse = 5, // Callsign is already in use by someone else
    LoginUnknownError = 6,
    LoginEmailRequired = 7, // User is known but has no email
    LoginEmailInvalid = 8, // Provided email address looks so little like an email that we won't accept it
    LoginEmailInUse = 9, // Email is already in use by someone else
    LoginEmailNotValidated = 10, // Email is not yet validated
    LoginExistingUser = 11, // Signup and email change not allowed on existing accounts
    
    INVALID_RESPONSE_CODE = std::numeric_limits<uint32_t>::max(), // Do not use this. Never returned from any API
};

// Some of these fields loosely correspond to those of auth's APP_LAUNCH packet
struct SteamLoginResult {
    SteamLoginResponse status;
    int32_t arenaID;
    uint32_t otp;
    uint32_t serverIP;
    uint32_t serverPort;
    
    SteamLoginResult(SteamLoginResponse error) : status(error), arenaID(0), otp(0), serverIP(0), serverPort(0) {}
    SteamLoginResult(int32_t arenaID, uint32_t otp, uint32_t serverIP, uint32_t serverPort) : status(SteamLoginResponse::LoginSuccess), arenaID(arenaID), otp(otp), serverIP(serverIP), serverPort(serverPort) {}
};

// All below APIs operate synchronously (your thread will block until the request completes, and this may take awhile)
// It is SAFE to call these APIs on a secondary thread
// All APIs may return any of the possible response codes

// Request a login when you don't know whether the user is already in the system or not.
// Also use this API when polling for completion of email verification.
SteamLoginResult authRequestSteamLogin(const void *authTicket, size_t authTicketLen, uint32_t arenaID);

// Request new user signup given a callsign and email. Both are required.
// This API will error if called for a user who already has a callsign, whether they have an email or not.
// If all validation passes, a verification email WILL be sent by this call.
// This API will never return LoginSuccess.
SteamLoginResult authRequestSteamSignup(const void *authTicket, size_t authTicketLen, uint32_t arenaID, const std::string &callsign, const std::string &email);

// Request new user email change.
// This API will error if called for a user who has no callsign or has already verified their email.
// Use this API for when the user requests to change the email address they want to verify (such as due to typos)
// Calling this API will result in a new verification email being sent, EVEN IF THE EMAIL HASN'T CHANGED. It can therefore be used to implement a "resend" button.
// This API will NOT work to change an email that is already verified!!!
SteamLoginResult authRequestSteamEmailChange(const void *authTicket, size_t authTicketLen, uint32_t arenaID, const std::string &email);

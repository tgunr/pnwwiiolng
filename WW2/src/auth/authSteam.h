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

// IMPORTANT: Keep these in sync with the auth relay
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
    LoginArenaDown = 12, // Login succeeded, but the requested arena is offline
    LoginVersionMismatch = 13, // Login succeeded, but the client is not a version supported by the requested arena
    LoginArenaDenied = 14, // Login succeeded, but the user does not have access to the requested arena

    INVALID_RESPONSE_CODE = std::numeric_limits<uint32_t>::max(), // Do not use this. Never returned from any API
};

// Some of these fields loosely correspond to those of auth's APP_LAUNCH packet.
// N.B.: Having a structure that effectively has three different layouts all squished
// together like this without even being a union is bad form, but the redesign necessary
// to improve on it is not presently worth the trouble.
struct SteamLoginResult {
    // The response code from the relay API. See functions below for expected results of various calls.
    SteamLoginResponse status;
    // The authHost's APP_LAUNCH response (effectively). Valid ONLY when returned from authRequestSteamLogin() AND status == LoginSuccess
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
// Responses from this API:
//  - LoginNotValidUser: Steam user invalid or banned
//  - LoginNewUser: This user is not registered; present UI and call `authRequestSteamSignup()` next.
//  - LoginEmailRequired: This user is an existing Steam user with no email address. Present UI and call `authRequestSteamEmailChange()` next.
//  - LoginEmailNotValidated: The user needs to verify their email address. Call `authRequestSteamLogin()` to poll verification status or
//      `authRequestSteamEmailChange()` to request new email send or address change
//  - LoginArenaDown: The cluster is offline or full. Once arena listing is implemented, this will mean the state changed since the last
//      time the list was pulled, but for now it's impossible to know whether the arena's up until you've logged in successfully.
//  - LoginVersionMismatch: This client is too old or too new, did Steam not get a new version yet?
//  - LoginArenaDenied: This user doesn't have access to the cluster, probably means access was revoked since the arena list was retrieved
//      (once that's implemented) or they're trying to hack their way into a cluster they shouldn't be accessing.
//  - LoginSuccess: Login SUCCEEDED. Result object contains server address, OTP, etc.
SteamLoginResult authRequestSteamLogin(const void *authTicket, size_t authTicketLen, uint32_t arenaID);

// Request new user signup given a callsign and email. Both are required.
// This API will error if called for a user who already has a callsign, whether they have an email or not.
// If all validation passes, a verification email WILL be sent by this call.
// Responses from this API:
//  - LoginNotValidUser: Steam user invalid or banned
//  - LoginCallsignInvalid, LoginCallsignDisallowed, LoginCallsignInUse, LoginEmailInvalid, LoginEmailInUse: Obvious
//  - LoginEmailRequired: This user is an existing Steam user with no email address. Present UI and call `authRequestSteamEmailChange()` next.
//  - LoginEmailNotValidated: The user needs to verify their email address. Call `authRequestSteamLogin()` to poll verification status or
//      `authRequestSteamEmailChange()` to request new email send or address change
//  - LoginExistingUser: This user is already fully signed up and verified. Call `authRequestSteamLogin()` next.
//  - LoginSuccess: Signup SUCCEEDED. Call `authRequestSteamLogin()` after user verifies their email (or `authRequestSteamEmailChange()` if user requests it).
//      THE RESULT OBJECT DOES NOT CONTAIN ARENA OR OTP INFORMATION!!!! The user MUST verify their email BEFORE logging in.
SteamLoginResult authRequestSteamSignup(const void *authTicket, size_t authTicketLen, uint32_t arenaID, const std::string &callsign, const std::string &email);

// Request new user email change.
// This API will error if called for a user who has no callsign or has already verified their email.
// Use this API for when the user requests to change the email address they want to verify (such as due to typos)
// Calling this API will result in a new verification email being sent, EVEN IF THE EMAIL HASN'T CHANGED. It can therefore be used to implement a "resend" button.
// This API will NOT work to change an email that is already verified!!!
//  - LoginNotValidUser: Steam user invalid or banned
//  - LoginNewUser: This user is not registered; present UI and call `authRequestSteamSignup()` next.
//  - LoginEmailInvalid, LoginEmailInUse: Obvious
//  - LoginExistingUser: This user is already fully signed up and verified; email can not be changed through this interface. Call `authRequestSteamLogin()` next.
//  - LoginSuccess: Email send/update SUCCEEDED. Call `authRequestSteamLogin()` after user verifies their email (or `authRequestSteamEmailChange()` if user requests it).
//      THE RESULT OBJECT DOES NOT CONTAIN ARENA OR OTP INFORMATION!!!! The user MUST verify their email BEFORE logging in.
SteamLoginResult authRequestSteamEmailChange(const void *authTicket, size_t authTicketLen, uint32_t arenaID, const std::string &email);

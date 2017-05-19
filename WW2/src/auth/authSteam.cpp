//
//  authSteam.cpp
//  WW2OL
//
//  Created by Gwynne Raskind on 5/19/17.
//  Copyright © 2017 Cornered Rat Software - All rights reserved.
//

#include "syscfg.h"
#include "authSteam.h"

SteamLoginResponse authRequestSteamLogin(const void *authTicket, size_t authTicketLen, uint32_t arenaID, const std::string &requestedCallsign)
{
    ABORT_IF_EX(authTicket == nullptr || authTicketLen < 1, "Steam auth ticket is required");
    
    return SteamLoginResponse::LoginUnknownError;
}

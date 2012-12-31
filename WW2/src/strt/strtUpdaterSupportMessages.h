// Date: Mon May 21 20:15:05 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

//This function installs all the host callbacks
extern SINT32 strtInstallUpdaterSupportCallbacks(void);

//This function sends a move user to the strat system
extern SINT32 strtSendMoveUser(SINT32 teulIndex, const struct OCTET_CELL& octet, const struct WEAPON_SERIAL& weapon, UINT32 playerID);

//This function sends a delete user
extern SINT32 strtSendDeleteUser(SINT32 teulIndex);

//Sends the strat address to the strat updater
extern SINT32 strtSendStratAddress(UINT32 stratAddress);

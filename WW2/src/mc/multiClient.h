#pragma once

#include <vector>
#include "client/ww2Client.h"

/************************************************************************************************
	multiClient
*************************************************************************************************/
#ifdef MULTI_CLIENT
extern int multiClientExecuteCommand(char* line, bool checkQ = false);
extern int multiClientExecuteCommand(int argc, char* argv[], bool checkQ = false);

extern void multiClientSetPlayerIdStart(int);
extern void multiClientSetPlayers(std::vector<int> p);
extern void multiClientSetHost(const char*);
extern void multiClientSetStartPosition(float,float,float);

extern void multiClientInit();
extern void multiClientUpdate(int wait = 0, bool force = false);
extern void multiClientKill(bool exit = false);
#endif


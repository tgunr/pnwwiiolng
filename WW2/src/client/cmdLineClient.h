#pragma once

#include "client/ww2Client.h"

/************************************************************************************************
	cmdLineClient
*************************************************************************************************/

extern int cmdLineClientExecuteCommand(char*);
extern int cmdLineClientExecuteCommand(int argc, char* argv[]);

extern void cmdLineClientSetPlayerIdStart(int);
extern void cmdLineClientSetHost(const char*);
extern void cmdLineClientSetStartPosition(float,float,float);

extern void cmdLineClientUpdate();

extern void cmdLineClientKill();

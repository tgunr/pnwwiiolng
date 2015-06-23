#pragma once

#include "client/ww2Client.h"

/************************************************************************************************
	cmdLineClient
*************************************************************************************************/

int cmdLineClientExecuteCommand(char*);
int cmdLineClientExecuteCommand(int argc, char* argv[]);

void cmdLineClientSetPlayerIdStart(int);
void cmdLineClientSetHost(const char*);
void cmdLineClientSetStartPosition(float,float,float);

void cmdLineClientUpdate();

void cmdLineClientKill();

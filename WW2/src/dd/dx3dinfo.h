#pragma once

#define DIRECT3D_VERSION 0x0900
#include <windows.h>
#include <d3d9types.h>
#include <d3d9caps.h>
#include <d3d9.h>

#include <string>
#include <vector>

struct DSOUNDDRIVERS
{
	GUID* guid;
	std::string description;
	std::string driverName;

};

struct ADAPTERMODES
{
	D3DADAPTER_IDENTIFIER9	identifier;
	DWORD					nummodes;
	std::vector<D3DDISPLAYMODE *> _modes;

};

char* dx3dinfoDeviceTypeToString(DWORD type);
char* dx3dinfoAdapterToString(DWORD adapter);

bool dx3dinfoEnumAdaptersAndModes(void);
bool dx3dinfoGetAdaptersAndModes(DWORD* retnumadapters, std::vector<ADAPTERMODES*>& adaptermodes);

int dx3dGetBPP(D3DFORMAT format);

DSOUNDDRIVERS* dx3dGetSoundInfo(int& numDrivers);

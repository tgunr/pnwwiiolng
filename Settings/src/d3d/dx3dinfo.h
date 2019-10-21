// dx3dinfo.h

#ifndef dx3dinfo_HEADER
#define dx3dinfo_HEADER

#define DIRECT3D_VERSION 0x0900
#include <d3d9types.h>
#include <d3d9caps.h>
#include <d3d9.h>

#include <string>

typedef struct
{
    GUID *guid;
    std::wstring description;
    std::wstring driverName;

} DSOUNDDRIVERS;

typedef struct tagADAPTERMODES
{
    D3DADAPTER_IDENTIFIER9	identifier;
	DWORD					nummodes;
    std::vector<D3DDISPLAYMODE *> _modes;

} ADAPTERMODES, *LPADAPTERMODES;

char *dx3dinfoDeviceTypeToString(DWORD type);
char *dx3dinfoAdapterToString(DWORD adapter);

bool dx3dinfoEnumAdaptersAndModes(void);
bool dx3dinfoGetAdaptersAndModes(DWORD *retnumadapters, std::vector<LPADAPTERMODES>& adaptermodes);

int dx3dGetBPP(D3DFORMAT format);

DSOUNDDRIVERS *dx3dGetSoundInfo(int& numDrivers);

#endif

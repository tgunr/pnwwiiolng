#pragma once
#include <vector>
#include <string>

#define MAX_DEVICE_ID_LENGTH 64

enum PI_OS
{
	PI_OS_PC,
	PI_OS_MAC
};

// platform independent device description
// used for both video and sound cards
struct PIDeviceInfo
{
	std::string description;
	std::string id; // unique id of the device
};

typedef std::vector<PIDeviceInfo> DeviceList;
typedef std::vector<std::string> InfoList;

extern bool piinfoIsMinOS(PI_OS os, unsigned int minMajorVersion = 0, unsigned int minMinorVersion = 0);
extern bool piinfoIsMaxOS(PI_OS os, unsigned int maxMajorVersion = 0xffffffff, unsigned int maxMinorVersion = 0xffffffff);
extern bool piinfoGetVideoCards(DeviceList* list);
extern bool piinfoGetSoundCards(DeviceList* list);
extern bool piinfoGetSupportedResolutions(InfoList* list, int adaptor = 0);
extern bool piinfoIsSupportedPref(int type);
extern bool piinfoIsSupportedSystem();

// pass in a native id type and it converts it into a
// string respresenting the id, for windows it will be
// a guid structure and for a mac it will be an index
// windows = GUID; mac = device index
extern const char* piinfoGetDeviceIDAsString(const void* id);

// clipboard accessor
extern const wchar_t* piinfoClipboard(const wchar_t* in);

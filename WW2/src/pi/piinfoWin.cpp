#include <syscfg.h> // Path is project specific.
#include "pi/piinfo.h"

#include <locale>
#include <codecvt>

#include "dd/dx3dinfo.h" // windows only
#include "cfg/cfgCodes.h"

#include "osgImport/FrameBufferObject.h"
#include "gl/Glew.h"

#include "ui/uiMain.h"

bool piinfoGetVideoCards(DeviceList* list)
{
	struct videoDisplay {
		DWORD indexNumber;
		std::wstring deviceName;
		std::wstring deviceString;
		std::wstring displayName;
	};

	videoDisplay tmpDisplay;

	DISPLAY_DEVICE device; device.cb = sizeof(device);
	for ( DWORD i = 0 ; EnumDisplayDevices(NULL, i, &device, 0) != 0 ; ++i ) {
		DEVMODE dm; dm.dmSize = sizeof(dm);
		if ( EnumDisplaySettings(device.DeviceName, 0, &dm) ) {
			//  std::string name(std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(device.DeviceName));
			// list->push_back({name, name});

			tmpDisplay.indexNumber = i;
			tmpDisplay.deviceName = device.DeviceName;
			tmpDisplay.deviceString = device.DeviceString;
			tmpDisplay.displayName = L"Display " + std::to_wstring(tmpDisplay.indexNumber + 1) + L" - " + tmpDisplay.deviceString;

			std::string deviceName(std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(tmpDisplay.deviceName));
			std::string displayName(std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(tmpDisplay.displayName));

			list->push_back({ displayName, deviceName });
		}
	}

	return (list->size()) ? true: false;
}

bool piinfoGetSoundCards(DeviceList* list)
{
    int numDrivers = 0;
    DSOUNDDRIVERS* drivers = dx3dGetSoundInfo(numDrivers);

    for(int i = 0; i < numDrivers; i++)
	{
		PIDeviceInfo device;

		device.description = drivers[i].description;
		device.id = piinfoGetDeviceIDAsString(drivers[i].guid);

		if(!device.id.size()) // primary sound driver has no guid
			device.id = "Primary";

		list->push_back(device);
	}

	return (list->size()) ? true: false;
}

bool piinfoGetSupportedResolutions(InfoList* list, int adapter)
{
	DeviceList devices;
	piinfoGetVideoCards(&devices);

	std::wstring devname(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(devices[adapter].id));

	DEVMODE dm; dm.dmSize = sizeof(dm);
	for ( DWORD i = 0 ; EnumDisplaySettings(devname.c_str(), i, &dm) ; ++i ) {
		if ( dm.dmPelsWidth >= 800 && dm.dmPelsHeight >= 600 && dm.dmBitsPerPel >= 32 ) {
			char buffer[128];
			sprintf(buffer, "%dx%dx%d", dm.dmPelsWidth, dm.dmPelsHeight, dm.dmBitsPerPel);
			if ( std::find(list->begin(), list->end(), buffer) == list->end() )
				list->push_back(buffer);
		}
	}

	return (list->size()) ? true: false;
}

bool piinfoIsSupportedPref(int type)
{
	bool supported = false;

	switch((_cfgPARAMS)type)
	{
		case cfgH2O_REFLECTIONS:
            {
                FBOExtensions* ext = FBOExtensions::instance(0, true);

                if(ext->isSupported() && piinfoIsSupportedPref(cfgENABLE_H2O_SHADERS))
                {
                    supported = true;
                }
			    break;
            }
		case cfgH2O_REFRACTIONS:
            {
                FBOExtensions* ext = FBOExtensions::instance(0, true);

                if(ext->isSupported() && piinfoIsSupportedPref(cfgENABLE_H2O_SHADERS))
                {
                    supported = true;
                }
			    break;
            }
		case cfgPOST_RENDER_FILTER:
            {
                FBOExtensions* ext = FBOExtensions::instance(0, true);

                if(ext->isSupported() && GLEW_ARB_fragment_program && GL_ARB_fragment_shader && GLEW_ARB_vertex_program)
                {
                    supported = true;
                }

                break;
            }
		case cfgENABLE_H2O_SHADERS:
		case cfgNORMAL_MAPS:
			supported = GLEW_ARB_fragment_program &&
                        GLEW_ARB_vertex_program &&
                        GL_ARB_fragment_shader;
			break;
		case cfgGRASS_SHADER:
			supported = GLEW_ARB_vertex_program &&
                        GLEW_ARB_fragment_program;
			break;
		case cfgWANT_SSE2:
		{
			__try
			{
				__asm
				{
					xorpd xmm0, xmm0        // executing SSE2 instruction
				}
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				// sse2 not supported
				// This code is neccessary to clear the exception code
				supported = (_exception_code() == STATUS_ILLEGAL_INSTRUCTION && false); // always return false
			}

			// sse2 supported
			supported = true;

    		break;
		}
	}

	return supported;
}


static bool _continueChecks(const char* failed)
{
	std::string key = "spec.bad.";
	key += failed;

	std::wstring title = uiLocString("spec.bad");
	std::wstring feedback = uiLocString(key.c_str());
	std::wstring con = uiLocString("spec.bad.continue");

	feedback += L" ";
	feedback += con;

	if(MessageBox(nullptr, feedback.c_str(), title.c_str(), MB_YESNO | MB_TOPMOST | MB_ICONEXCLAMATION) == IDYES)
		return true;

	return false;
}

bool piinfoIsSupportedSystem()
{
    const char* glver = (const char*)glGetString(GL_VERSION);
	int major = (glver) ? atoi(&glver[0]) : 0;
	int minor = 0;

	if(major < 2)
	{
		Write_Debug("Minimum Spec not met: opengl min version %s", (glver) ? glver : "na");

		if(!_continueChecks("openglversion"))
			return false;
	}

    const char* glslver = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
	major = (glslver) ? atoi(&glslver[0]) : 0;
	minor = (glslver) ? atoi(&glslver[2]) : 0;

	if(major < 1 || (major == 1 && minor < 20))
	{
		Write_Debug("Minimum Spec not met: shader model %s", (glslver) ? glslver : "na");

		if(!_continueChecks("shadermodel"))
			return false;
	}

    MEMORYSTATUSEX memstat;
    memset(&memstat, 0, sizeof(MEMORYSTATUSEX));
    memstat.dwLength = sizeof(MEMORYSTATUSEX);

    GlobalMemoryStatusEx(&memstat);

    float totalGB = memstat.ullTotalPhys / 1024.0f / 1024.0f / 1024.0f;

    if(totalGB < 1.8f)
	{
		Write_Debug("Minimum Spec not met: not enough system memory: %.2f", totalGB);

		if(!_continueChecks("systemmemory"))
			return false;
	}

	return true;
}

const char* piinfoGetDeviceIDAsString(const void* id)
{
	wchar_t buffer[MAX_DEVICE_ID_LENGTH+1] = {0};

	if(id) StringFromGUID2(*((GUID*)id), buffer, MAX_DEVICE_ID_LENGTH);

	return uiUTF16toUTF8(buffer);

}

const TCHAR* piinfoClipboard(const TCHAR* in)
{
	const TCHAR* out = nullptr;

	if(OpenClipboard(GetForegroundWindow()))
	{
		if(in)
		{
			// cutting or copying to clipboard

			// empty the clipboard first
			 EmptyClipboard();

			 int size = (wcslen(in)+1) * sizeof(TCHAR);

			 // get a memory object to copy the data into
			 HGLOBAL object = GlobalAlloc(GMEM_MOVEABLE, size);

			 if(object)
			 {
				 // lock it
				 LPVOID ptr = GlobalLock(object);

				 // copy the data
				 memcpy(ptr, in, size);

				 // unlock it
				 GlobalUnlock(object);

				 // set our clipboard data
				 SetClipboardData(CF_UNICODETEXT, object);
			 }
		}
		else
		{
			// pasting from clipboard
			if(IsClipboardFormatAvailable(CF_UNICODETEXT))
			{
				HANDLE object = GetClipboardData(CF_UNICODETEXT);

				 if(object)
				 {
					 static std::wstring buffer;
					 buffer = L"";

					 // lock it
					 LPVOID ptr = GlobalLock(object);

					 // copy the data
					 buffer = (const TCHAR*)ptr;

					 // unlock it
					 GlobalUnlock(object);

					 // set the out pointer to the newly grabbed clipboard data
					 out = buffer.c_str();
				 }
			}
		}

		CloseClipboard();
	}

	return out;
}

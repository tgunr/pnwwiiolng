#include <syscfg.h> // Path is project specific.
#include "pi/piinfo.h"
#include "dd/dx3dinfo.h" // windows only
#include "cfg/cfgCodes.h"

#include "osgImport/FrameBufferObject.h"
#include "gl/Glew.h"

#include "ui/uiMain.h"

bool piinfoGetVideoCards(DeviceList* list)
{
    DWORD numAdapters;
    std::vector<ADAPTERMODES*> adapterModes;

	if(dx3dinfoGetAdaptersAndModes(&numAdapters, adapterModes))
	{
		for(DWORD i = 0; i < numAdapters; i++)
		{
			PIDeviceInfo device;

			device.description = adapterModes[i]->identifier.Description;
			device.id = piinfoGetDeviceIDAsString(&adapterModes[i]->identifier.DeviceIdentifier);

			list->push_back(device);
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
	DWORD numAdapters;
    std::vector<ADAPTERMODES*> adapterModes;

	if(dx3dinfoGetAdaptersAndModes(&numAdapters, adapterModes) && (DWORD)adapter < numAdapters)
	{
		char buffer[128+1];
		UINT w, h;

        for(unsigned int i = 0; i < adapterModes[adapter]->_modes.size(); i++)
		{
			w = adapterModes[adapter]->_modes[i]->Width;
			h = adapterModes[adapter]->_modes[i]->Height;

            int bpp = dx3dGetBPP(adapterModes[adapter]->_modes[i]->Format);

			if(w >= 800 && h >= 600 && bpp >= 32)
			{
				snprintf(buffer, 128+1, "%dx%dx%d", w, h, bpp);

				list->push_back(buffer);
			}
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

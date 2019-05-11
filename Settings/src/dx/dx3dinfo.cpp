// dx3dinfo.c

#include <windows.h>

#include <dsound.h>
#include <vector>

#include "dx3dinfo.h"

DWORD					numadapters;

static std::vector<ADAPTERMODES *> _adaptermodes;

static LPDIRECT3D9 _d3d9 = NULL;

static int numberDrivers = 0;
static DSOUNDDRIVERS soundDrivers[64];

static BOOL CALLBACK EnumDriverCallback(LPGUID guid, LPCWSTR description, LPCWSTR name, LPVOID context)
{
	if(numberDrivers >= 64)
	{
		return FALSE;
	}

    soundDrivers[numberDrivers].guid = guid;
    soundDrivers[numberDrivers].description = description;
    soundDrivers[numberDrivers].driverName = name;

	numberDrivers++;

	return TRUE;
}

DSOUNDDRIVERS *dx3dGetSoundInfo(int& numDrivers)
{
    if(numberDrivers == 0)
    {
	    DirectSoundEnumerate(EnumDriverCallback, NULL);
    }
    else
    {
        // Already done
    }

    numDrivers = numberDrivers;

    return soundDrivers;
};

int dx3dGetBPP(D3DFORMAT format)
{
    switch(format)
    {
    case D3DFMT_UNKNOWN: return 0;
    case D3DFMT_R8G8B8: return 24;
    case D3DFMT_A8R8G8B8: return 32;
    case D3DFMT_X8R8G8B8: return 32;
    case D3DFMT_R5G6B5: return 16;
    case D3DFMT_X1R5G5B5: return 16;
    case D3DFMT_A1R5G5B5: return 16;
    case D3DFMT_A4R4G4B4: return 16;
    case D3DFMT_R3G3B2: return 8;
    case D3DFMT_A8: return 8;
    case D3DFMT_A8R3G3B2: return 16;
    case D3DFMT_X4R4G4B4: return 16;
    case D3DFMT_A2B10G10R10: return 32;
    case D3DFMT_G16R16: return 32;
    case D3DFMT_A8P8: return 16;
    case D3DFMT_P8: return 8;
    case D3DFMT_L8: return 8;
    case D3DFMT_A8L8: return 16;
    case D3DFMT_A4L4: return 8;
    case D3DFMT_V8U8: return 16;
    case D3DFMT_L6V5U5: return 16;
    case D3DFMT_X8L8V8U8: return 32;
    case D3DFMT_Q8W8V8U8: return 32;
    case D3DFMT_V16U16: return 32;
    case D3DFMT_A2W10V10U10: return 32;
    default:
        {
            return 0;
        }
    }

    return 0;
}

// Return a text pointer to a string that represents the device type
char *dx3dinfoDeviceTypeToString(DWORD type)
{
	switch (type)
	{
    case D3DDEVTYPE_HAL:
	{
		static char *ptr="D3DDEVTYPE_HAL";
		return(ptr);
	}
	case D3DDEVTYPE_REF:
	{
		static char *ptr="D3DDEVTYPE_REF";
		return(ptr);
	}
	case D3DDEVTYPE_SW:
	{
		static char *ptr="D3DDEVTYPE_SW";
		return(ptr);
	}
	default:
	{
		static char *ptr="Default UNKNOWN";
		return(ptr);
	}
	}
}

/*------------------------------------------------------------
Function: dx3dinfoAdapterToString
Description:
-------------------------------------------------------------*/
char *dx3dinfoAdapterToString(DWORD adapter)
{
    return(_adaptermodes[adapter]->identifier.Description);
}

/*------------------------------------------------------------
Function: dx3dinfoEnumAdaptersAndModes
Description:
-------------------------------------------------------------*/
bool dx3dinfoEnumAdaptersAndModes()
{
    int i, j;
    D3DDISPLAYMODE *mode = NULL;
    D3DADAPTER_IDENTIFIER9 *identifier = NULL;

    if(!_adaptermodes.empty())
    {
        return true;
    }

    const D3DFORMAT allowedAdapterFormats[] = 
    {   
        D3DFMT_A1R5G5B5,
        D3DFMT_A2R10G10B10,
        D3DFMT_A8R8G8B8,
        D3DFMT_R5G6B5, 
        D3DFMT_X1R5G5B5, 
        D3DFMT_X8R8G8B8
    };

    D3DDISPLAYMODE currentmode;
    _d3d9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &currentmode);

    const UINT allowedAdapterFormatArrayCount  = sizeof(allowedAdapterFormats) / sizeof(allowedAdapterFormats[0]);
   
    for(i = 0; i < (int)_d3d9->GetAdapterCount(); i++)
	{
        ADAPTERMODES *adaptermode = new ADAPTERMODES;

    	if(D3D_OK != _d3d9->GetAdapterIdentifier(i, 0, &adaptermode->identifier))
		{
            delete adaptermode;
			return false;
		}
        
        for(int q = 0; q < allowedAdapterFormatArrayCount; q++)
        {
		    int nummodes = _d3d9->GetAdapterModeCount(i, allowedAdapterFormats[q]);

            for(j = 0; j < nummodes; j++)
            {
                D3DDISPLAYMODE *mode = new D3DDISPLAYMODE;

                if (D3D_OK != _d3d9->EnumAdapterModes(i, allowedAdapterFormats[q], j, mode))
			    {
                    delete mode;
				    return false;
			    }

                if (mode->Width < 800 || 
                    mode->Height < 600 || 
                    mode->RefreshRate != currentmode.RefreshRate)
 			    {
                    delete mode;
                    continue;
			    }

                adaptermode->_modes.push_back(mode);
		    }
        }

        if(adaptermode->_modes.empty())
        {
            delete adaptermode;
        }
        else
        {
            _adaptermodes.push_back(adaptermode);
        }
	}

	return true;
}

bool dx3dinfoGetAdaptersAndModes(DWORD *numadapters, std::vector<LPADAPTERMODES>& adaptermodes)
{
    if(!_d3d9)
    {
        _d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
		
        if(!_d3d9)
        {
            return false;
        }

 		if(!dx3dinfoEnumAdaptersAndModes())
		{
			return false;
		}

		if(_d3d9 != NULL)
		{
			_d3d9->Release();
			_d3d9 = NULL;
		}
	}

    *numadapters = (DWORD)_adaptermodes.size();
    adaptermodes = _adaptermodes;

	return true;
}


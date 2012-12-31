#include <string>
#include "platforms.h"

#include "systypes.h"
#include "piinfo.h"
#include "gl/glew.h"
#include "cfgCodes.h"

#include <Cocoa/Cocoa.h>
//#include <Carbon/Carbon.h>

bool piinfoIsSupportedSystem()
{
  return TRUE;
}

bool piinfoGetVideoCards(DeviceList* list)
{
	CGDisplayCount 		displayCount;
	CGDirectDisplayID	displayIDs[16];

	if(CGGetActiveDisplayList(16, displayIDs, &displayCount) != kCGErrorSuccess) return false;
    
    for (unsigned int i = 0; i < displayCount; i++) 
    {
		PIDeviceInfo devinfo;
		char s[32];
		snprintf(s, 31, "%d", CGDisplayIDToOpenGLDisplayMask(displayIDs[i]));
		devinfo.id = s;
		snprintf(s, 31, "Monitor %d",i);		 
		devinfo.description = s;
		list->push_back(devinfo);
	}

	return true;
}

bool piinfoGetSoundCards(DeviceList* list)
{
	return false;
}

bool piinfoGetSupportedResolutions(InfoList* list, int adaptor)
{
	CGDisplayCount 		displayCount;
	CGDirectDisplayID	displayIDs[16];
	CGGetActiveDisplayList(16, displayIDs, &displayCount);
	if(adaptor >= (int)displayCount || adaptor < 0) return false;

	NSArray* modes = (NSArray*)CGDisplayAvailableModes(displayIDs[adaptor]);
	if(modes == 0) return false;

	for (unsigned int i = 0; i < [modes count]; i++ )
	{
		NSDictionary* mode = [modes objectAtIndex:i];
		
		if (([[mode objectForKey:(NSString*)kCGDisplayModeIsSafeForHardware] boolValue] == YES) &&	//include only safe modes
			([[mode objectForKey:(NSString*)kCGDisplayModeIsTelevisionOutput] boolValue] == NO) && 	//exclude TV
			([[mode objectForKey:(NSString*)kCGDisplayModeIsStretched] boolValue] == NO)) 			//exclude streched (for now)
		{

			int bpp = [[mode objectForKey:(NSString*)kCGDisplayBitsPerPixel] intValue];
			
			if (bpp == 32) 
			{

				int width = [[mode objectForKey:(NSString*)kCGDisplayWidth] intValue];
				int height = [[mode objectForKey:(NSString*)kCGDisplayHeight] intValue];
				int refreshRate = [[mode objectForKey:(NSString*)kCGDisplayRefreshRate] intValue];

				list->push_back([[NSString stringWithFormat:@"%dx%dx%dx%d", width,height, bpp, refreshRate] cStringUsingEncoding: NSASCIIStringEncoding]);
			}
		}
	}

	return true;
}


bool piinfoIsSupportedPref(int type)
{
	bool supported = true;

	switch((_cfgPARAMS)type)
	{
		case cfgH2O_REFLECTIONS:
			supported = piinfoIsSupportedPref(cfgENABLE_H2O_SHADERS) &&
						GLEW_ARB_texture_cube_map;
			break;
		case cfgENABLE_H2O_SHADERS:
		case cfgPOST_RENDER_FILTER:
		case cfgNORMAL_MAPS:
			supported = GLEW_ARB_fragment_program && 
                        GLEW_ARB_vertex_program;
			break;
		case cfgGRASS_SHADER:
			supported = GLEW_ARB_vertex_program;
			break;
		case cfgWANT_SSE2:
			supported = false;
		break;
        default: break ;
	}

	return supported;
}

static const wchar_t* _paste()
{
	// Return the contents of the PasteBoard/ScrapBook/Clipboard
	return L"" ;
}

static const wchar_t* _copy(const wchar_t* input)
{
	return input ;
}

const wchar_t* piinfoClipboard(const wchar_t* input)
{
	if ( input == NULL )
	{
		return _paste() ;
	}
	else
	{
		return _copy(input) ;
	}
}

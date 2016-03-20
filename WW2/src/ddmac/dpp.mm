
#include "sysmac/syscfg.h"
#include "ddp.h"
#include "sysmain.h"
#include <sys/stat.h>
#include "cfgPrefs.h"
#import <Cocoa/Cocoa.h>
#include "endianess.h"

// CGLDescribeRenderer

CGLContextObj cgl_ctx;
static NSOpenGLContext *nsOpenGlContext; //, *win_ctx, *full_ctx;
static NSRect ddpRect,winRect,fullRect;
static NSWindow* ddpWin=NULL;
static NSOpenGLView* ddpView=NULL;
static CGDirectDisplayID displayID;
static CGDisplayModeRef defaultMode, fullScreenMode;
static NSScreen* ddpScreen=NULL;

static DDWND_DDRENDERFUNC	ddrenderfunc;
static DDWND_D3DRENDERFUNC	d3drenderfunc;
static LOGICAL	resetfps;
static int		capture;
static UINT32	totpaints;
static UINT32	tottime;
static int bpp;
static bool canWindow = true;
bool appHidden;

#define BASE_GL_ATTRIBUTES \
NSOpenGLPFADepthSize, (NSOpenGLPixelFormatAttribute)0,       \
NSOpenGLPFAAccelerated,                                      \
NSOpenGLPFADoubleBuffer,                                     \
NSOpenGLPFANoRecovery,                                       \
NSOpenGLPFAStencilSize, (NSOpenGLPixelFormatAttribute)8      

#define FULLSCREEN_GL_ATTRIBUTES \
NSOpenGLPFAFullScreen,                                       \
NSOpenGLPFAScreenMask,(NSOpenGLPixelFormatAttribute)0        

#define MULTI_GL_ATTRIBUTES \
NSOpenGLPFASampleBuffers, 1,                                     \
NSOpenGLPFASamples,0x7fffffff,                                           \
NSOpenGLPFAMultisample

static NSOpenGLPixelFormatAttribute fullScreenAttrs[] = 
{	
	BASE_GL_ATTRIBUTES,
	FULLSCREEN_GL_ATTRIBUTES,
	0,0xffffffff
LOGICAL cursorHidden = false;

};

static NSOpenGLPixelFormatAttribute windowedAttrs[] = 
{	
	BASE_GL_ATTRIBUTES,
	0,0xffffffff
};

static NSOpenGLPixelFormatAttribute fullScreenAttrs_S[] = 
{	
	BASE_GL_ATTRIBUTES,
	FULLSCREEN_GL_ATTRIBUTES,
    MULTI_GL_ATTRIBUTES,
	0,0xffffffff
};

static NSOpenGLPixelFormatAttribute windowedAttrs_S[] = 
{	
	BASE_GL_ATTRIBUTES,
    MULTI_GL_ATTRIBUTES,
	0,0xffffffff
};

static NSOpenGLPixelFormatAttribute* fullScreenAttributes;
static NSOpenGLPixelFormatAttribute* windowedAttributes;

// hack
@interface NSScreen (NSScreenAccess)
- (void) setFrame:(NSRect)frame;
@end

@implementation NSScreen (NSScreenAccess)
- (void) setFrame:(NSRect)frame;
{
	_frame = frame;
}
@end 

static void examineRenderer(int oglDisplayMask)
{
    CGLRendererInfoObj info;
    GLint nrend;
    GLint value;
    
    fullScreenAttributes = fullScreenAttrs;
    windowedAttributes = windowedAttrs;
    
    int fsaa = 0;
    cfgPrefsGetInt("app.fsaasamples",&fsaa);
    
    CGLError err = CGLQueryRendererInfo(oglDisplayMask, &info, &nrend );
    if ( err == kCGLNoError )
    {
        err = CGLDescribeRenderer(info, 0, kCGLRPSampleModes, &value);
        if ( err == kCGLNoError )
        {
            if ( (value & kCGLMultisampleBit) && (fsaa!=0)  )
            {
                fullScreenAttributes = fullScreenAttrs_S;
                windowedAttributes = windowedAttrs_S;
                
                NSOpenGLPixelFormatAttribute *p = fullScreenAttributes;
                while (*p != 0xffffffff )
                {
                    if ( *p == 0x7fffffff )
                    {
                        *p = fsaa;
                        break;
                    }
                    p++;
                }
                p = windowedAttributes;
                while (*p != 0xffffffff )
                {
                    if ( *p == 0x7fffffff )
                    {
                        *p = fsaa;
                        break;
                    }
                    p++;
                }
            }
        }
    }
}

static NSScreen* ddpGetScreen(int oglDisplayMask)
{
	displayID = CGOpenGLDisplayMaskToDisplayID(oglDisplayMask);
	
	for(unsigned int i = 0; i < [[NSScreen screens] count]; i++)
	{
		NSScreen* screen = [[NSScreen screens] objectAtIndex: i];
		if(displayID == (CGDirectDisplayID)[[[screen deviceDescription] objectForKey:@"NSScreenNumber"] longValue])
			return screen;
	}
	
	return [NSScreen mainScreen];
}

void ddpHandleVsync()
{
	GLint params[1] = {(GLint)g_enableVSync};
    if (nsOpenGlContext!=NULL)
    {
        CGLSetParameter((CGLContextObj)[nsOpenGlContext CGLContextObj], kCGLCPSwapInterval, params);	
    }
}

LOGICAL ddpCreateWnd(int x, int y, int width, int height, int inBpp, int fullscreen, int monitor, int refreshrate)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    NSOpenGLContext* context;
    
	ddpScreen = ddpGetScreen(monitor);
	bpp = inBpp;
    
    examineRenderer(monitor);
    
	CGDirectDisplayID displayID = (CGDirectDisplayID)[[[ddpScreen deviceDescription] objectForKey:@"NSScreenNumber"] longValue];
    
    defaultMode = CGDisplayCopyDisplayMode ( displayID );

	fullScreenAttributes[NSOpenGLPFAColorSize] = (NSOpenGLPixelFormatAttribute)bpp;
	fullScreenAttributes[NSOpenGLPFAScreenMask] = (NSOpenGLPixelFormatAttribute)CGDisplayIDToOpenGLDisplayMask(displayID);
    fullScreenAttributes[NSOpenGLPFAFullScreen] = (NSOpenGLPixelFormatAttribute) true;
    
	windowedAttributes[NSOpenGLPFAColorSize] = (NSOpenGLPixelFormatAttribute)bpp;
    
    ddpView = [[NSOpenGLView alloc] initWithFrame:NSMakeRect(0, 0, width, height)
                                      pixelFormat:[[[NSOpenGLPixelFormat alloc] initWithAttributes:windowedAttributes] autorelease]];
    [ddpView retain];
    context = [ddpView openGLContext];
    if(!context) throw "Unable To Allocate OpenGL Window Context";
    
    NSRect screenRect = [ddpScreen frame];

    if ( ! fullscreen )
    {
        winRect = NSMakeRect(screenRect.origin.x, screenRect.origin.y, screenRect.size.width, screenRect.size.height);
        ddpWin = [[NSWindow alloc] initWithContentRect: winRect styleMask: NSBorderlessWindowMask backing: NSBackingStoreNonretained defer: NO];
    } else {
        
        winRect = NSMakeRect(0, 0, width, height);
        ddpWin = [[NSWindow alloc] initWithContentRect: winRect styleMask: NSTitledWindowMask backing: NSBackingStoreNonretained defer: NO];
    }
        //winRect.origin.x = screenRect.origin.x + (screenRect.size.width - winRect.size.width) / 2;
        //winRect.origin.y = screenRect.origin.y + (screenRect.size.height - winRect.size.height) / 2;
        
    [ddpWin setReleasedWhenClosed:NO];
    [ddpWin setAcceptsMouseMovedEvents:NO];
    
    nsOpenGlContext = context;
	
	[NSMenu setMenuBarVisible:NO];
    
	ddpSetFullScreen(fullscreen);
	
	ddpHandleVsync();
	
	[pool release];
    
	return TRUE;
}

void ddpSetupGL(int w, int h)
{
	[nsOpenGlContext makeCurrentContext];
    
    *oglGetProjectionMatrix() = IdentityMatrix;
    
	piSetViewportDimensions(0, 0, w, h, 0.0f, 1.0f);
    
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
    
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glewInit();
}

LOGICAL ddpSetDDRender(DDWND_DDRENDERFUNC newfunc)
{
	ddrenderfunc = newfunc;
	return TRUE;
}

LOGICAL ddpSetD3DRender(DDWND_D3DRENDERFUNC newfunc)
{
	d3drenderfunc = newfunc;
	return TRUE;
}

UINT32 ddpEngine(void* param)
{
    //	piResetRenderStats();
	ddpUpdate();
    
	return 0;
}

LOGICAL	ddpFlip()
{
	[nsOpenGlContext flushBuffer];
	return TRUE;
}

BOOL cursorHidden = FALSE;

void ddpShowCursor(void)
{
    //  [ddpWin setAcceptsMouseMovedEvents:YES];
	[NSCursor unhide];
    cursorHidden = FALSE;
}

void ddpHideCursor(void)
{
    //  [ddpWin setAcceptsMouseMovedEvents:NO];
	[NSCursor hide];
    cursorHidden = TRUE;
}

void ddpKill()
{
	CGReleaseAllDisplays();
	ddpShowCursor();
}

long ddpGetTotalPaints(void)
{
	return totpaints;
}

int	ddpGetBPP()
{
	return bpp;
}

UINT32 ddpGetClockTime(UINT32 * Time)
{
	*Time = GetTickCount();
	return 0;
}

UINT32 ddpGetTotTime(UINT32 * TotTime)
{
	*TotTime = tottime;
	return 0;
}

void ddpResetFPS()
{
	resetfps = TRUE;
}

float ddpGetFPS()
{
	UINT32   ttime;
	float   fps=0.0f;
    
	ttime = (frametime - tottime);
	if (ttime)
	{
		fps = (float)totpaints * 1000.0f / (float)ttime;
	}
    
	return(fps);
}


LOGICAL ddcapStart()
{
	UCHAR* bitmap = (UCHAR*)malloc(ddpGetWidth() * ddpGetHeight() * 3);
	
	if(bitmap)
	{
		char picFolder[128],path[256],filename[64];
		FSRef ref;
		FSFindFolder(kUserDomain, kPictureDocumentsFolderType, kCreateFolder, &ref);		 
		FSRefMakePath(&ref, (unsigned char*)picFolder, 127);
		time_t t = time(NULL);
		strftime(filename,63, "%F %H.%M.%S", localtime(&t));
        
		snprintf(path,255,"%s/WW2OL Screenshots", picFolder);
		mkdir(path, S_IRWXU);
		
		snprintf(path,255,"%s/WW2OL Screenshots/%s.bmp", picFolder,filename);
		
		glReadPixels(0, 0, ddpGetWidth(), ddpGetHeight(), GL_RGB, GL_UNSIGNED_BYTE, bitmap);
		ddcapSave(bitmap, ddpGetWidth(), ddpGetHeight(), path);
        
		free(bitmap);
	}
	return TRUE;
}

LOGICAL	ddpUpdate()
{
	piResetRenderStats();
	
	totpaints++;
    
	CGLSetCurrentContext(cgl_ctx);
	
	if(ddrenderfunc) ddrenderfunc();
	if(d3drenderfunc) d3drenderfunc();

#ifdef MAKE_HOST_TERRAIN
    if ( GeneratingHostTerrain )
    {
        updateGeneratingHostTerrain();
    }
#endif
    
#ifdef ENABLE_FLOATERS
	logUpdateProfiling();
#endif	// #ifdef BETA_RELEASE
    
	if (resetfps)
	{
		tottime = frametime;
		totpaints = 0;
		ft1000mscount = ft1000msticks = 0;
		ft500mscount = ft500msticks = 0;
		ft100mscount = ft100msticks = 0;
		ft50mscount = ft50msticks = 0;
		ft10mscount = ft10msticks = 0;
		resetfps = FALSE;
        
	}
    
	ddpFlip();
    
	if (capture > 0)
	{
		ddcapStart();
		capture--;
	}
    
    UINT32 points;
    UINT32 lines;
    UINT32 triangles;
    UINT32 vertices;
    
    piGetRenderStats(&points, &lines, &triangles, &vertices);
    
    //printf("%8d: %8d %8d %8d %8d\n",totpaints, points,lines,triangles,vertices);
    
	return TRUE;
}

LOGICAL	ddpDrawToScreen(LPVOID lpdds)
{
	return FALSE;
}

void ddpCapture()
{
	capture++;
}

void ScreenCapture(const char* name, int sx, int sy, int w, int h)
{
    int shotCount=0;
    UCHAR* bitmap = (UCHAR*)memAlloc(w * h * 3);
	char buff[128];
    
	if(bitmap)
	{
        sprintf(buff, "%s.bmp", name);    
		glReadPixels(sx, sy, w, h, GL_RGB, GL_UNSIGNED_BYTE, bitmap);
        GLenum err = glGetError();
		ddcapSave(bitmap, w, h, buff);
        
		memFree(bitmap);
	}
}

LOGICAL ddcapSave(UCHAR *bptr, UINT32 width, UINT32 height, char *bname)
{
    
    UCHAR				*bits, *ptrin, *ptrout;
    UINT32				i, j;
    UINT32				size, bwidth, twidth, tsize;
    piBITMAPINFO	*dib;
    
    
	bwidth = width * 3;		// 3 bytes per pixel
	size = bwidth * height;
    
	// DIBs must be DWORD aligned
	if (3 & bwidth)
	{
		twidth = (bwidth + 3) & (~3);
		tsize = twidth * height;
	}
	else
	{
		twidth = bwidth;
		tsize = size;
	}
    
	dib = (piBITMAPINFO*)malloc(tsize + sizeof(piBITMAPINFO));
	if (!dib)
	{
		return(FALSE);
	}
    
	dib->bmiHeader.biSize = sizeof(dib->bmiHeader);
	dib->bmiHeader.biWidth = width;
	dib->bmiHeader.biHeight = height;	//	top down bitmap
	dib->bmiHeader.biPlanes = 1;
	dib->bmiHeader.biBitCount = 24;
	dib->bmiHeader.biCompression = BI_RGB;
	dib->bmiHeader.biSizeImage = tsize;
	dib->bmiHeader.biXPelsPerMeter = 0;
	dib->bmiHeader.biYPelsPerMeter = 0;
	dib->bmiHeader.biClrUsed = 0;
	dib->bmiHeader.biClrImportant = 0;
    
	bits = (UCHAR*)dib + dib->bmiHeader.biSize;
    
	ptrin 	= bptr;
	ptrout 	= bits;
	for (i=0 ; i < height ; i++)
	{
		// copy a row
		for (j=0 ; j < bwidth ; j += 3, ptrin += 3)
		{
			*ptrout++ = *(ptrin+2);
			*ptrout++ = *(ptrin+1);
			*ptrout++ = *(ptrin+0);
		}
        
		// pad rest of DWORD with zeros
		for ( ; j < twidth ; j++)
		{
			*ptrout++ = 0;
		}
	}
    
	// create a file from the DIB
	if (NULL != bname)
	{
        piBITMAPFILEHEADER	bmfh;
        FILE	*cf;
        
		bmfh.bfType = (UINT16)('B' + ('M' << 8));
		bmfh.bfSize = sizeof(bmfh) + dib->bmiHeader.biSize + twidth * height;
		bmfh.bfReserved1 = 0;
		bmfh.bfReserved2 = 0;
		bmfh.bfOffBits = sizeof(bmfh) + dib->bmiHeader.biSize;
		
		size = dib->bmiHeader.biSize + dib->bmiHeader.biSizeImage;
		
		NATIVE_TO_LITTLE_ENDIAN_16(bmfh.bfType);
		NATIVE_TO_LITTLE_ENDIAN_32(bmfh.bfSize);
		NATIVE_TO_LITTLE_ENDIAN_16(bmfh.bfReserved1);
		NATIVE_TO_LITTLE_ENDIAN_16(bmfh.bfReserved2);
		NATIVE_TO_LITTLE_ENDIAN_32(bmfh.bfOffBits);
		
		NATIVE_TO_LITTLE_ENDIAN_32(dib->bmiHeader.biSize);
		NATIVE_TO_LITTLE_ENDIAN_32(dib->bmiHeader.biWidth);
		NATIVE_TO_LITTLE_ENDIAN_32(dib->bmiHeader.biHeight);
		NATIVE_TO_LITTLE_ENDIAN_16(dib->bmiHeader.biPlanes);
		NATIVE_TO_LITTLE_ENDIAN_16(dib->bmiHeader.biBitCount);
		NATIVE_TO_LITTLE_ENDIAN_32(dib->bmiHeader.biCompression);
		NATIVE_TO_LITTLE_ENDIAN_32(dib->bmiHeader.biSizeImage);
		NATIVE_TO_LITTLE_ENDIAN_32(dib->bmiHeader.biXPelsPerMeter);
		NATIVE_TO_LITTLE_ENDIAN_32(dib->bmiHeader.biYPelsPerMeter);
		NATIVE_TO_LITTLE_ENDIAN_32(dib->bmiHeader.biClrUsed);
		NATIVE_TO_LITTLE_ENDIAN_32(dib->bmiHeader.biClrImportant);
		
		cf = fopen(bname, "wb+");
		if(!cf) return FALSE;
		
		fwrite(&bmfh, 1, sizeof(bmfh), cf);
		fwrite(dib, 1, size, cf);
        
		fclose(cf);
	}
    
	free(dib);
    
	return(TRUE);
}

#include <Carbon/Carbon.h>

void sysSetMousePosition(int x, int y)
{
}

void sysGetMousePosition(SINT32* x, SINT32* y)
{
	if(WantedWindow)
	{
		NSPoint pt = [ddpWin mouseLocationOutsideOfEventStream];
		*x = pt.x;
		*y = ddpRect.size.height - pt.y;
	}else
	{
        NSPoint pt = [NSEvent mouseLocation];
//		Point pt;
//		GetMouse(&pt);
		*x = pt.x - ddpRect.origin.x;
		*y = pt.y - ddpRect.origin.y;
	}
}

int ddpGetWidth()
{
	return ddpRect.size.width;
}

int ddpGetHeight()
{
	return ddpRect.size.height;
}

void ddpHideApplication()
{
	CGReleaseAllDisplays();
    CGLClearDrawable( cgl_ctx );
    
    if ( ddpWin )
    {
        [ddpWin orderOut:ddpWin];	
    }
	[NSApp deactivate];
}

bool ddpAppHidden()
{
	return appHidden;
}

void ddpToggleWindowedMode()
{
	ddpSetFullScreen(WantedWindow);
}

void ddpShowApplication()
{
	ddpSetFullScreen(!WantedWindow);
}

void ddpSetFullScreen(LOGICAL fullscreen)
{
	if(!canWindow && !fullscreen) return;
	
    CGError result;
    bool cap = true;
    cfgPrefsGetBool("app.capturealldisplays",&cap);
    
	WantedWindow = !fullscreen;
	if(fullscreen)
	{
        if ( cap )
		{
            CGCaptureAllDisplaysWithOptions(kCGCaptureNoOptions);
        }
        else 
        {
            CGDisplayCaptureWithOptions(displayID,kCGCaptureNoOptions);
        }

        result = CGDisplaySetDisplayMode ( displayID, fullScreenMode, NULL );
		CGRect displayBounds = CGDisplayBounds(displayID);
		fullRect = NSMakeRect(displayBounds.origin.x,displayBounds.origin.y, displayBounds.size.width, displayBounds.size.height);
        
		[ddpWin orderOut: ddpWin];
		[ddpWin setContentView: 0];	
		ddpRect = fullRect;
		[nsOpenGlContext setFullScreen];
		[ddpScreen setFrame: fullRect];
	}
    else
	{
//		[nsOpenGlContext clearDrawable];
        result = CGDisplaySetDisplayMode ( displayID, defaultMode, NULL );
		CGReleaseAllDisplays();
        
		[ddpWin setContentView: ddpView];
		[ddpWin makeKeyAndOrderFront:ddpWin];
		ddpRect = winRect;
	}
	cgl_ctx = (CGLContextObj)[nsOpenGlContext CGLContextObj];
    
//    CGDisplayModeRef mode;
//    CFStringRef modeStr = CGDisplayModeCopyPixelEncoding(mode);
    WantedBpp = bpp = TexBpp = 32;
	WantedWidth = ddpRect.size.width;
	WantedHeight =  ddpRect.size.height;
    
	[nsOpenGlContext makeCurrentContext];
}

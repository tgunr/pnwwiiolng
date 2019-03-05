
#include "sysmac/syscfg.h"
#include "ddp.h"
#include "sysmain.h"
#include <sys/stat.h>
#include "cfgPrefs.h"
#import <Cocoa/Cocoa.h>

LOGICAL cursorHidden = false;

// From glplat.h (why)
CGLContextObj cgl_ctx = nullptr;

static NSOpenGLContext *nsOpenGlContext = nil; //, *win_ctx, *full_ctx;
static NSRect ddpRect = NSZeroRect, winRect = NSZeroRect, fullRect = NSZeroRect;
static NSWindow *ddpWin = nil;
static NSOpenGLView *ddpView = nil;
static CGDirectDisplayID displayID = kCGNullDirectDisplay;
static CGDisplayModeRef defaultMode = nullptr, fullScreenMode = nullptr;
static NSScreen *ddpScreen = nullptr;

static DDWND_DDRENDERFUNC ddrenderfunc = nullptr;
static DDWND_D3DRENDERFUNC d3drenderfunc = nullptr;
static LOGICAL resetfps = false;
static UINT32 capture = 0;
static UINT32 totpaints = 0;
static UINT32 tottime = 0;

static LOGICAL ddcapGrabScreen();

////////////////////////////////////////////////////////////////////////////////
#pragma mark - SETUP -
////////////////////////////////////////////////////////////////////////////////

static constexpr const NSOpenGLPixelFormatAttribute placeholderAttributeValue = std::numeric_limits<NSOpenGLPixelFormatAttribute>::max();

static NSOpenGLPixelFormatAttribute oglAttributes[] =
{
    // Explicitly request non-Core profile
    NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersionLegacy,
    // Request <to be determined> color buffer
    NSOpenGLPFAColorSize, placeholderAttributeValue,
    // Request 32-bit depth buffer
    NSOpenGLPFADepthSize, 32,
    // Request double-buffering
    NSOpenGLPFADoubleBuffer,
    // Request 8-bit stencil buffer
    NSOpenGLPFAStencilSize, 8,

    0,

    // Request a single FSAA buffer
    NSOpenGLPFASampleBuffers, 1,
    // Request <to be determined> FSAA samples
    NSOpenGLPFASamples, placeholderAttributeValue,
    // Request multisampling enable
    NSOpenGLPFAMultisample,

    0
};

static void resetOGLAttributeValue(NSOpenGLPixelFormatAttribute *attributes,
                                   NSOpenGLPixelFormatAttribute attrKey,
                                   NSOpenGLPixelFormatAttribute attrValue)
{
    for (NSOpenGLPixelFormatAttribute *attr = attributes; *attr; ++attr) {
        if (*attr == attrKey && *(attr + 1) == placeholderAttributeValue) {
            *(attr + 1) = attrValue;
            return;
        }
    }
#if DEBUG
    throw "Didn't find placeholder value to reset OGL attribute";
#endif
}

static bool rendererSupportsFSAA(CGDirectDisplayID displayID) // returns false on error
{
    CGLError err = kCGLNoError;
    CGLRendererInfoObj info = nullptr;
    GLint nrend = 0, value = 0;

    if ((err = CGLQueryRendererInfo(CGDisplayIDToOpenGLDisplayMask(displayID), &info, &nrend)) != kCGLNoError) {
        // Failed to get renderer
        return false;
    }

    err = CGLDescribeRenderer(info, 0, kCGLRPSampleModes, &value);
    CGLDestroyRendererInfo(info); // Don't need it anymore.

    if (err != kCGLNoError || // Failed to get info.
        nrend < 1 // Fail if no renderers are described.
    ) {
        return false;
    }

    return (value & kCGLMultisampleBit) != 0;
}

static bool enableFSAAIfRequested(NSOpenGLPixelFormatAttribute *attributes, CGDirectDisplayID displayID)
{
    int fsaa_ = 0;

    if (!cfgPrefsGetInt("app.fsaasamples", &fsaa_) || fsaa_ <= 0) {
        // If value not set or is zero, don't even bother. Not an error.
        return true;
    }

    unsigned int fsaa = static_cast<unsigned int>(fsaa_);

    if (!rendererSupportsFSAA(displayID)) {
        return false;
    }

    // Wipe out the placeholder that keeps the FSAA attributes from being used by default.
    // This is a bit of a stupid hack, but it makes a bunch of code elsewhere cleaner.
    NSOpenGLPixelFormatAttribute *fsaaDisablePlaceholder = attributes, *finalTerminator = nullptr;

    while (*fsaaDisablePlaceholder++) ; // Stop after first 0
    finalTerminator = fsaaDisablePlaceholder--; // Back up to point to first 0
    while (*finalTerminator++) ; // Stop after next 0
    --finalTerminator; // Back up to point to last 0.

    // No need to re-terminate - the final terminator is included in the memory copy.
    memmove(fsaaDisablePlaceholder, // Destination = overwrite the disable placeholder
            fsaaDisablePlaceholder + 1, // Source = first entry after the disable placeholder
            static_cast<size_t>(finalTerminator - fsaaDisablePlaceholder) * sizeof(NSOpenGLPixelFormatAttribute)
            // ^ length = number of entries covered by the distance between the source and the end
    );

    // Finally, set the sample value.
    resetOGLAttributeValue(attributes, NSOpenGLPFASamples, fsaa);
    return true;
}

static NSScreen *ddpGetScreen(int oglDisplayMask)
{
	displayID = CGOpenGLDisplayMaskToDisplayID(static_cast<CGDirectDisplayID>(oglDisplayMask));

    for (NSScreen *screen in [NSScreen screens]) {
		if (displayID == (CGDirectDisplayID)[[screen deviceDescription][@"NSScreenNumber"] longValue])
			return screen;
	}
	return [NSScreen mainScreen];
}

void ddpHandleVsync()
{
    if (nsOpenGlContext) {
        CGLSetParameter(nsOpenGlContext.CGLContextObj, kCGLCPSwapInterval, (GLint[1]){ (GLint)g_enableVSync });
    }
}

LOGICAL ddpCreateWnd(int x, int y, int width, int height, int bpp, int fullscreen, int displayMask, int refreshrate)
{
    NSOpenGLContext *context = nil;

	if (!(ddpScreen = ddpGetScreen(static_cast<int>(displayMask)))) {
        fprintf(stderr, "Failed to get screen for chosen display\n");
        return false;
    }

    CGDirectDisplayID monitor = static_cast<CGDirectDisplayID>([[ddpScreen deviceDescription][@"NSScreenNumber"] longValue]);

    defaultMode = CGDisplayCopyDisplayMode(monitor);
    // Apple says not to change the display mode. TODO: Make the back buffer differently-sized instead.
    fullScreenMode = CGDisplayCopyDisplayMode(monitor);// CopyBestDisplayMode(monitor, bpp, width, height, refreshrate);

    if (!enableFSAAIfRequested(oglAttributes, monitor)) {
        fprintf(stderr, "Failed to enable FSAA\n");
        return false;
    }
    resetOGLAttributeValue(oglAttributes, NSOpenGLPFAColorSize, static_cast<NSOpenGLPixelFormatAttribute>(bpp));

    ddpView = [[NSOpenGLView alloc] initWithFrame:NSMakeRect(0, 0, width, height)
                                      pixelFormat:[[NSOpenGLPixelFormat alloc] initWithAttributes:oglAttributes]];
    context = ddpView.openGLContext;
    if (!context) {
        throw "Unable To Allocate OpenGL Window Context";
    }

    fullRect = ddpScreen.frame;
    winRect = NSMakeRect((ddpScreen.frame.size.width) / 2 - (width / 2), (ddpScreen.frame.size.height) / 2 - (height / 2), width, height);
    if (fullscreen) {
        ddpRect = fullRect;
    } else {
        ddpRect = winRect;
    }
    ddpRect = winRect;

    ddpWin = [[NSWindow alloc] initWithContentRect:ddpRect
                               styleMask:fullscreen ? NSBorderlessWindowMask : NSTitledWindowMask
                               backing:NSBackingStoreBuffered
                               defer:NO];
    ddpWin.releasedWhenClosed = NO;
    ddpWin.acceptsMouseMovedEvents = NO;
    ddpWin.contentView = ddpView;
    nsOpenGlContext = context;
	ddpSetFullScreen(!!fullscreen);
	ddpHandleVsync();
	return true;
}

static void ddpRefreshGLViewport(int w, int h)
{
    *oglGetProjectionMatrix() = IdentityMatrix;
	piSetViewportDimensions(0, 0, w, h, 0.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// IMPORTANT: This is not just a setup routine! This is called every time the
// application is activated or deactivated.
void ddpSetFullScreen(LOGICAL fullscreen)
{
	WantedWindow = !fullscreen;
    ddpWin.level = fullscreen ? NSMainMenuWindowLevel + 1 : NSNormalWindowLevel; // per Apple docs
    ddpWin.hidesOnDeactivate = !!fullscreen;
    [ddpWin setFrame:fullscreen ? fullRect : winRect display:YES animate:NO];
    ddpRect = ddpWin.frame;
    [ddpWin makeKeyAndOrderFront:nil];
	cgl_ctx = nsOpenGlContext.CGLContextObj;
    WantedBpp = TexBpp = 32;
	WantedWidth = static_cast<SINT32>(ddpRect.size.width);
	WantedHeight = static_cast<SINT32>(ddpRect.size.height);
	[nsOpenGlContext makeCurrentContext];
    ddpRefreshGLViewport(WantedWidth, WantedHeight);
}

void ddpTemporarilyEscapeFullscreen(bool begin)
{
    // This is a TERRIBLE solution, for the record. It's just a quick hack that works for now.
    // TODO: Clean this up with something sane.
    static NSRect oldWinRect;

    if (begin) {
        oldWinRect = winRect;
        winRect = fullRect;
        ddpSetFullScreen(false);
    } else {
        winRect = oldWinRect;
        ddpSetFullScreen(true);
    }
}

GLuint ddpDefaultFramebuffer() {
	return 0;
}

void ddpSetupGL(int w, int h)
{
	[nsOpenGlContext makeCurrentContext];
	glewInit();
    ddpRefreshGLViewport(w, h);
}

////////////////////////////////////////////////////////////////////////////////
#pragma mark - RENDER -
////////////////////////////////////////////////////////////////////////////////

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
	ddpUpdate();
	return 0;
}

LOGICAL	ddpUpdate()
{
	piResetRenderStats();

	++totpaints;

	CGLSetCurrentContext(cgl_ctx);

	if (ddrenderfunc) {
        ddrenderfunc();
    }
	if (d3drenderfunc) {
        d3drenderfunc();
    }

#ifdef MAKE_HOST_TERRAIN
    if (GeneratingHostTerrain) {
        updateGeneratingHostTerrain();
    }
#endif

	if (resetfps)
	{
		tottime = static_cast<UINT32>(frametime);
		totpaints = 0;
		ft1000mscount = ft1000msticks = 0;
		ft500mscount = ft500msticks = 0;
		ft100mscount = ft100msticks = 0;
		ft50mscount = ft50msticks = 0;
		ft10mscount = ft10msticks = 0;
		resetfps = FALSE;

	}

	ddpFlip();

	if (capture > 0) {
		if (!ddcapGrabScreen()) {
            dbugLog(_FLN, "Saving screenshot failed");
        }
		capture--;
	}

    UINT32 points = 0, lines = 0, triangles = 0, vertices = 0;

    piGetRenderStats(&points, &lines, &triangles, &vertices);
    //printf("%8d: %8d %8d %8d %8d\n",totpaints, points,lines,triangles,vertices);
	return TRUE;
}

LOGICAL	ddpFlip()
{
	[nsOpenGlContext flushBuffer];
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
#pragma mark - MOUSE -
////////////////////////////////////////////////////////////////////////////////

void ddpShowCursor(void)
{
	[NSCursor unhide];
    cursorHidden = FALSE;
}

void ddpHideCursor(void)
{
	[NSCursor hide];
    cursorHidden = TRUE;
}

void sysSetMousePosition(int x, int y)
{
    dbugLog(_FLD, "Ignoring request to set mouse position to %d, %d", x, y);
}

void sysGetMousePosition(SINT32 *x, SINT32 *y)
{
    NSPoint pt = [ddpWin mouseLocationOutsideOfEventStream];

    *x = static_cast<SINT32>(pt.x);
    *y = static_cast<SINT32>(ddpRect.size.height - pt.y);
}

////////////////////////////////////////////////////////////////////////////////
#pragma mark - GETTERS AND STATS -
////////////////////////////////////////////////////////////////////////////////

int ddpGetCenterX()
{
    return static_cast<int>([[ddpWin screen] frame].origin.x + ([[ddpWin screen] frame].size.width / 2));
}

int ddpGetCenterY()
{
    return static_cast<int>([[ddpWin screen] frame].origin.y + ([[ddpWin screen] frame].size.height / 2));
}

int ddpGetWidth()
{
	return static_cast<int>(ddpRect.size.width);
}

int ddpGetHeight()
{
	return static_cast<int>(ddpRect.size.height);
}

long ddpGetTotalPaints(void)
{
	return static_cast<long>(totpaints);
}

UINT32 ddpGetClockTime(UINT32 * Time)
{
	*Time = timeGetTime();
	return 0;
}

UINT32 ddpGetTotTime(UINT32 * TotTime)
{
	*TotTime = tottime;
	return 0;
}

float ddpGetFPS()
{
	float ttime = static_cast<float>(frametime) - static_cast<float>(tottime), fps = 0.0f;

	if (ttime > 0.0f) {
		fps = static_cast<float>(ddpGetTotalPaints()) * 1000.0f / ttime;
	}
	return fps;
}

void ddpResetFPS()
{
	resetfps = TRUE;
}

////////////////////////////////////////////////////////////////////////////////
#pragma mark - MODE MANAGEMENT -
////////////////////////////////////////////////////////////////////////////////

void ddpHideApplication()
{
}

void ddpShowApplication()
{
	ddpSetFullScreen(!WantedWindow);
}

void ddpToggleWindowedMode()
{
	ddpSetFullScreen(WantedWindow);
}

void ddpKill()
{
	//CGReleaseAllDisplays();
	ddpShowCursor();
    [ddpWin orderOut:nil];
    [ddpWin close];
}

////////////////////////////////////////////////////////////////////////////////
#pragma mark - SCREEN CAPTURE -
////////////////////////////////////////////////////////////////////////////////

void ddpCapture()
{
	++capture;
}

// Note: The following code takes advantage of the semantics of messaging nil in
// Objective-C in several places. Do not mistake failures to check for nil for
// lack of consideration of it's effect. For example, in the code below, only
// "screenshotsDir" is checked for nil, even though "picturesDir" is used to
// create it. This is okay because a nil "picturesDir" will always return nil
// from any message; therefore checking "screenshotsDir" counts as checking
// both at once. (Additionally, messages to nil are very very fast, so no speed
// penalty is taken.)
static NSURL *ddcapGenerateScreenshotURL(NSString *providedName)
{
    static dispatch_once_t predicate = 0;
    static NSURL *screenshotsDirectory = nil;
    static NSDateFormatter *dateFormatter = nil;

    // Only need to do all this setup and configuration once.
    dispatch_once(&predicate, ^ {
        // Find ~/Pictures
        NSURL *picturesDir = [[NSFileManager defaultManager]
            URLForDirectory:NSPicturesDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:nullptr];
        // Add our custom subdirectory to the URLs - TODO: Add a setting for users to configure this
        NSURL *screenshotsDir = [picturesDir URLByAppendingPathComponent:@"WW2OL Screenshots"];
        NSError *error = nil;

        if (!screenshotsDir) {
            dbugLog(_FLW, "Failed to find screenshot directory");
            return;
        }
        // Create the custom subdirectory if it doesn't already exist.
        if (![[NSFileManager defaultManager] createDirectoryAtURL:screenshotsDir withIntermediateDirectories:NO attributes:nil error:&error]) {
            // Only error if it's not 'cause the dir already exists. Checking first would be a race condition.
            // No, it's not likely we'll encounter that race condition. That's no reason to write bad code.
            if (![error.domain isEqualToString:NSCocoaErrorDomain] || error.code != NSFileWriteFileExistsError) {
                dbugLog(_FLW, "Failed to create screenshot directory");
                return;
            }
        }
        screenshotsDirectory = screenshotsDir;

        // Create a date formatter that includes full date and time info,
        // doesn't return "today" etc., and uses the user's formatting settings.
        dateFormatter = [[NSDateFormatter alloc] init];
        [dateFormatter setDateFormat:[NSDateFormatter dateFormatFromTemplate:@"YYYY-MM-dd HH.mm.ss.SSSS" options:0 locale:[NSLocale currentLocale]]];
        dateFormatter.doesRelativeDateFormatting = NO;
        dateFormatter.locale = [NSLocale currentLocale];
    });

    // The filename will be "<providedName> - <datetime>.png" ("WW2OL" is the default if no name is provided)
    NSString *screenshotName = [NSString stringWithFormat:@"%@ - %@", providedName ?: @"WW2OL", [dateFormatter stringFromDate:[NSDate date]]];

    // Because NSDateFormatter's locale template may generate invalid characters
    // for filenames, we sanitize the string. Unfortunately, Cocoa does not
    // provide a built-in function for ensuring this, so we hardcode knowledge
    // of OS X's filesystem peculiarities.
    screenshotName = [screenshotName stringByReplacingOccurrencesOfString:@"/" withString:@"-"];
    screenshotName = [screenshotName stringByReplacingOccurrencesOfString:@":" withString:@"."];

    return [screenshotsDirectory URLByAppendingPathComponent:[screenshotName stringByAppendingPathExtension:@"png"]];
}

static void ddcapFlipBuffer(NSMutableData *bitmap, GLsizei w, GLsizei h)
{
    // OpenGL's coordinate system is vertically flipped relative to AppKit's. There are 3 obvious ways to handle this:
    //  1) Apply glScalef() to the OGL render, requiring another frame of rendering before calling glReadPixels()
    //  2) Flip the NSBitmapImageRep by creating an NSImage from it and applying the appropriate scale transformation to that.
    //  3) Manually perform a simple bytewise vertical flip in-place on the buffer read from glReadPixels()
    // We choose option 3 as by far the fastest, as well as less error-prone than flipping the entire renderer.
    uint32_t *basePtr = static_cast<uint32_t *>(bitmap.mutableBytes);

    // The data is conceptually an array of rows, each representing an array of pixels. We can therefore swap h/2 rows.
    // For h = 4, h / 2 = 2, we will swap(0, 3), swap(1, 2)
    // For h = 5, h / 2 = 2, we will swap(0, 4), swap(1, 3)
    // For h = 6, h / 2 = 3, we will swap(0, 5), swap(1, 4), swap(2, 3)
    // These loops are massively parallelizable, so I let Clang do the dirty work for me.
    for (GLsizei row = 0; row < (h / 2); ++row) {
        uint32_t *tRow = basePtr + (row * w);
        uint32_t *bRow = basePtr + ((h - row - 1) * w);

        #pragma clang loop vectorize(enable) vectorize_width(8)
        for (GLsizei col = 0; col < w; ++col) {
            std::swap(tRow[col], bRow[col]);
        }
    }
}

// Note: It is STRONGLY recommended to wrap an autorelease pool around this call.
static bool ddcapSaveScreenRegion(GLint x, GLint y, GLsizei w, GLsizei h, NSString *name = nil)
{
    NSURL *screenshotURL = ddcapGenerateScreenshotURL(name); // default is just based on current time

    if (!screenshotURL) {
        fprintf(stderr, "Error finding place to save screenshot\n");
        return false;
    }

    NSMutableData *bitmap = [NSMutableData dataWithLength:static_cast<NSUInteger>(4/*RGBA*/ * ddpGetWidth() * ddpGetHeight())];

	if (!bitmap) {
        dbugLog(_FLA, "MEMORY ALLOCATION FAILURE!!");
        return false;
    }

    glReadPixels(std::min<GLint>  (x, ddpGetWidth()), std::min<GLint>  (y, ddpGetHeight()),
                 std::min<GLsizei>(w, ddpGetWidth()), std::min<GLsizei>(h, ddpGetHeight()),
                 GL_RGBA, GL_UNSIGNED_BYTE, bitmap.mutableBytes
    );

    ddcapFlipBuffer(bitmap, w, h);

    NSBitmapImageRep *imageRep = [[NSBitmapImageRep alloc]
        initWithBitmapDataPlanes:(unsigned char *[]){ static_cast<unsigned char *>(bitmap.mutableBytes) }
        pixelsWide:ddpGetWidth() pixelsHigh:ddpGetHeight()
        bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace
        bitmapFormat:NS32BitLittleEndianBitmapFormat bytesPerRow:4 * ddpGetWidth() bitsPerPixel:32
    ];

    if (!imageRep) {
        dbugLog(_FLW, "Failed to read screenshot data");
        return false;
    }

    NSData *pngData = [imageRep representationUsingType:NSPNGFileType properties:@{ NSImageInterlaced: @NO, /* do NOT specify a gamma */ }];

    if (![pngData writeToURL:screenshotURL options:NSDataWritingWithoutOverwriting error:nullptr]) {
        dbugLog(_FLW, "Failed to write PNG to %s", screenshotURL.path.UTF8String);
        return false;
    }
    return true;
}

static LOGICAL ddcapGrabScreen()
{
    // Unfortunately, this is one case where we *definitely* want an
    // autorelease pool, especially since NSApplicationMain() is not running and
    // we therefore can't depend on the system to provide one for us.
    @autoreleasepool {
        return ddcapSaveScreenRegion(0, 0, ddpGetWidth(), ddpGetHeight());
    }
}

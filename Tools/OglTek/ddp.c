// ddp.c

#include "syscfg.h"
#include "vars.h"

BOOL ddpChooseScreenFormat(HDC hdc, PIXELFORMATDESCRIPTOR *pfd)
{
int					i, pixelformat, depth, accum, stencil;

	memset(pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

	pfd->nSize				= sizeof(PIXELFORMATDESCRIPTOR);
	pfd->nVersion			= 1;
	pfd->dwFlags			= PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
	pfd->iPixelType			= PFD_TYPE_RGBA;
	pfd->cColorBits			= WantedBpp;
	pfd->cRedBits			= 0;
	pfd->cRedShift			= 0;
	pfd->cGreenBits			= 0;
	pfd->cGreenShift		= 0;
	pfd->cBlueBits			= 0;
	pfd->cBlueShift			= 0;
	pfd->cAlphaBits			= 0;
	pfd->cAlphaShift		= 0;
	pfd->cAccumBits			= 0;
	pfd->cAccumRedBits		= 0;
	pfd->cAccumGreenBits	= 0;
	pfd->cAccumBlueBits		= 0;
	pfd->cAccumAlphaBits	= 0;
	pfd->cDepthBits			= 32;
	pfd->cStencilBits		= 0;
	pfd->cAuxBuffers		= 0;
	pfd->iLayerType			= PFD_MAIN_PLANE;
	pfd->bReserved			= 0;
	pfd->dwLayerMask		= 0;
	pfd->dwVisibleMask		= 0;
	pfd->dwDamageMask		= 0;

	pixelformat = ChoosePixelFormat(hdc, pfd);
	if (pixelformat == 0)
	{
		Write_Debug("Unable to choose a compatible pixel format");
		return(FALSE);
	}

	Write_Debug("ChoosePixelFormat recommended #%d", pixelformat);

	depth = 0;
	accum = 0;
	stencil = 0;
	for (i=1 ; ; i++)
	{
		if (!DescribePixelFormat(hdc, i, sizeof(PIXELFORMATDESCRIPTOR), pfd))
		{
			Write_Debug("There were %d pixel formats", i - 1);
			break;
		}

		if ((WantedWindow ? (pfd->dwFlags & PFD_DRAW_TO_WINDOW) : TRUE) &&
			(pfd->dwFlags & PFD_SUPPORT_OPENGL) &&
			(pfd->dwFlags & PFD_DOUBLEBUFFER) &&
			!(pfd->dwFlags & PFD_GENERIC_FORMAT) &&
			pfd->iLayerType == PFD_MAIN_PLANE &&
			pfd->iPixelType == PFD_TYPE_RGBA &&
			pfd->cDepthBits >= depth &&		// Largest bit depth
			pfd->cAccumBits >= accum &&		// Largest accumulators
			pfd->cStencilBits >= stencil &&	// Largest stencil buffer
			pfd->cColorBits == WantedBpp)
		{
			Write_Debug(" Format #%2d,"
						" Version=%d,"
						" Flags=0x%04x,"
						" PixelType=%s,"
						" ColorBits=%2d,"
						" R=%d,"
						" G=%d,"
						" B=%d,"
						" A=%d,"
						" AccumBits=%2d,"
						" R=%2d,"
						" G=%2d,"
						" B=%2d,"
						" A=%2d,"
						" DepthBits=%2d,"
						" StencilBits=%2d",
						i,
						pfd->nVersion,
						pfd->dwFlags,
						pfd->iPixelType == PFD_TYPE_RGBA ? "RGB" : "Pal",
						pfd->cColorBits,
						pfd->cRedBits,
						pfd->cGreenBits,
						pfd->cBlueBits,
						pfd->cAlphaBits,
						pfd->cAccumBits,
						pfd->cAccumRedBits,
						pfd->cAccumGreenBits,
						pfd->cAccumBlueBits,
						pfd->cAccumAlphaBits,
						pfd->cDepthBits,
						pfd->cStencilBits);

			// Just show all the flags that are set
			if (pfd->dwFlags & PFD_SUPPORT_DIRECTDRAW)
			{
				Write_Debug(" Supports DirectDraw");
			}

			if (pfd->dwFlags & PFD_GENERIC_ACCELERATED)
			{
				Write_Debug(" Hardware accelerated");
			}

			if (pfd->dwFlags & PFD_SWAP_LAYER_BUFFERS)
			{
				Write_Debug(" Swap layer buffers");
			}

			if (pfd->dwFlags & PFD_SWAP_COPY)
			{
				Write_Debug(" Swap Copy");
			}

			if (pfd->dwFlags & PFD_SWAP_EXCHANGE)
			{
				Write_Debug(" Swap exchange");
			}

			if (pfd->dwFlags & PFD_NEED_SYSTEM_PALETTE)
			{
				Write_Debug(" Need system palette");
			}

			if (pfd->dwFlags & PFD_NEED_PALETTE)
			{
				Write_Debug(" Need palette");
			}

			if (pfd->dwFlags & PFD_GENERIC_FORMAT)
			{
				Write_Debug(" Generic format (unaccelerated software)");
			}

			if (pfd->dwFlags & PFD_SUPPORT_OPENGL)
			{
				Write_Debug(" Supports OpenGL");
			}

			if (pfd->dwFlags & PFD_SUPPORT_GDI)
			{
				Write_Debug(" Supports GDI");
			}

			if (pfd->dwFlags & PFD_DRAW_TO_BITMAP)
			{
				Write_Debug(" Can draw to bitmap");
			}

			if (pfd->dwFlags & PFD_DRAW_TO_WINDOW)
			{
				Write_Debug(" Can draw to window");
			}

			if (pfd->dwFlags & PFD_STEREO)
			{
				Write_Debug(" Supports stereoscopic display");
			}

			if (pfd->dwFlags & PFD_DOUBLEBUFFER)
			{
				Write_Debug(" Double buffered");
			}

			depth = pfd->cDepthBits;
			accum = pfd->cAccumBits;
			stencil = pfd->cStencilBits;
			pixelformat = i;
		}
	}

	Write_Debug("Deduced pixel format was #%d", pixelformat);

	// Set the pixel format selected
	if (!SetPixelFormat(hdc, pixelformat, pfd))
	{
		Write_Debug("Unable to set the desired pixel format");
		return(FALSE);
	}

	return(TRUE);
}

BOOL ddpSetDisplay(UINT32 flags)
{
BOOL			found;
UINT32			retval, modenum;
DEVMODE			presentmode;
DEVMODE			devmode;

	do
	{
		EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &presentmode);
		Write_Debug("Present mode: w=%d, h=%d, bpp=%d, ref=%d",
			presentmode.dmPelsWidth,
			presentmode.dmPelsHeight,
			presentmode.dmBitsPerPel,
			presentmode.dmDisplayFrequency);

		found = FALSE;
		modenum = 0;
		Write_Debug("Enumerated modes:");
		do
		{
			if (!EnumDisplaySettings(NULL, modenum, &devmode))
			{
				break;
			}

			Write_Debug("                  w=%d, h=%d, bpp=%d, ref=%d",
				devmode.dmPelsWidth,
				devmode.dmPelsHeight,
				devmode.dmBitsPerPel,
				devmode.dmDisplayFrequency);

			if (devmode.dmBitsPerPel == (UINT32)WantedBpp &&
				devmode.dmPelsWidth == (UINT32)WantedWidth &&
				devmode.dmPelsHeight == (UINT32)WantedHeight &&
				(devmode.dmDisplayFrequency == 0 ||
				(devmode.dmDisplayFrequency >= 60 &&
				devmode.dmDisplayFrequency <= presentmode.dmDisplayFrequency)))
			{
				found = TRUE;
				break;
			}

			modenum += 1;
		}
		while (TRUE);

		if (!found)
		{
			Write_Debug("No compatible display mode exists");
			return(FALSE);
		}

		retval = ChangeDisplaySettingsEx(NULL, &devmode, NULL, CDS_TEST, NULL);
		switch (retval)
		{
		case DISP_CHANGE_BADFLAGS:
			Write_Debug("DISP_CHANGE_BADFLAGS");
			break;
		case DISP_CHANGE_BADMODE:
			Write_Debug("DISP_CHANGE_BADMODE");
			break;
		case DISP_CHANGE_BADPARAM:
			Write_Debug("DISP_CHANGE_BADPARAM");
			break;
		case DISP_CHANGE_FAILED:
			Write_Debug("DISP_CHANGE_FAILED");
			break;
		case DISP_CHANGE_NOTUPDATED:
			Write_Debug("DISP_CHANGE_NOTUPDATED");
			break;
		case DISP_CHANGE_RESTART:
			Write_Debug("DISP_CHANGE_RESTART");
			break;
		case DISP_CHANGE_SUCCESSFUL:
			Write_Debug("DISP_CHANGE_SUCCESSFUL");
			ChangeDisplaySettingsEx(NULL, &devmode, NULL, CDS_FULLSCREEN, NULL);

			return(TRUE);
		}

		return(FALSE);
	}
	while (FALSE);

	return(TRUE);
}

void ddpResetDisplay(void)
{
	ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL);
}

#include <syscfg.h> // Path is project specific.
//	 Copyright 2008 Playnet Inc, All rights reserved
//	 Not to be distributed in API form except as part of a compiled library
//	 under License or NDA

#include "pi/pi.h"
#include "include/vars.h"

#include "imap/imaphack.h"
#include "dip.h"
#include "trn/trntod.h"
#include "locl/loclMain.h"
#include "veh/vehcomms.h"
#include "veh/vehmain.h"
#include "rn/rnmain.h"
#include "pers/persmain.h"
#include "ui/uiMain.h"
#include "trn/trnsky.h"
#include "ui/uiChat.h"
#include "osgImport/ShadowMap.h"

//#define USE_VTUNE

#ifdef USE_VTUNE
#include "vtuneapi.h"

int profile;
#endif

#include "imap/imap.h"
#include "imap/ww2func.h"

static UINT32 _TimeOfDay;
static UINT32 _RateOfTimeTravel;

char buffer[MAX_KEYS];
char lastbuffer[MAX_KEYS];

LOGICAL ProcessKeyboardInput();

UINT32 imaphackDoKeyboardInputs()
{
	if (!ProcessKeyboardInput())
	{
		return(0);
	}

/*
#if platOS == OS_WIN32
	// if we're alt tabbing, we've handled input
	if((dipKEYDOWN(buffer,DIK_TAB) && !dipKEYDOWN(lastbuffer,DIK_TAB)) && dipKEYDOWN(buffer,DIK_LMENU))
		return 1;
#endif
*/

	if(!uiInGame())
	{
		if (dipKEYDOWN(buffer,DIK_F9) && !dipKEYDOWN(lastbuffer,DIK_F9))
			piCapture(1, IMAP_MOM_TAKE_SCREEN_SHOT, 0);
	}

#if RELEASE_CLIENT == 0
	if (dipKEYDOWN(buffer,DIK_PAUSE) && !dipKEYDOWN(lastbuffer,DIK_PAUSE))
	{
		FillMode = (FillMode == piFILL_SOLID) ? piFILL_WIREFRAME : piFILL_SOLID;
		piSetRenderState(piRENDERSTATE_FILLMODE, FillMode);
	}
#endif

// V-TUNE profiling toggle
#ifdef USE_VTUNE
{
	if (dipKEYDOWN(buffer,DIK_TAB) && !dipKEYDOWN(lastbuffer,DIK_TAB))
	{
		profile = !profile;
		if(profile)
			VTResume();
		else
			VTPause();
	}
}
#endif

/* temporarily disable alt-f4 for release clients to help with some socring issue; see Oli or Doc
#if platOS == OS_WIN32
#if RELEASE_CLIENT != 0
	if ((dipKEYDOWN(buffer,DIK_F4) && !dipKEYDOWN(lastbuffer,DIK_F4)) &&
		(dipKEYDOWN(buffer,DIK_LMENU) || dipKEYDOWN(buffer,DIK_RMENU)))
	{
		piCloseProgram();
	}
#endif
#endif
*/

	if (Online == 0)
	{
		if (dipKEYDOWN(buffer,DIK_F11) && !dipKEYDOWN(lastbuffer,DIK_F11))
		{
            trnskyResetFade();

            // Do not change this!
#if(0)// RELEASE_CLIENT != 0
			if (_TimeOfDay == 0)
			{
				_TimeOfDay = 1;
				rnmainListText(loclGetLocalizationStringByID(loclAFTERNOON_LIGHTING_MSG));
				trnTODSetTimeOfDay(59400);	// 1630
			}
			else if (_TimeOfDay == 1)
			{
				_TimeOfDay = 2;
				rnmainListText(loclGetLocalizationStringByID(loclDUSK_LIGHTING_MSG));
				trnTODSetTimeOfDay(64000);	// 1547

			}
			else if (_TimeOfDay == 2)
			{
				_TimeOfDay = 3;
				rnmainListText(loclGetLocalizationStringByID(loclNIGHT_LIGHTING_MSG));
				trnTODSetTimeOfDay(0);		// 0000
			}
			else if (_TimeOfDay == 3)
			{
				_TimeOfDay = 4;
				rnmainListText(loclGetLocalizationStringByID(loclSUNRISE_LIGHTING_MSG));
				trnTODSetTimeOfDay(21000);	// 0550
			}
			else if (_TimeOfDay == 4)
			{
				_TimeOfDay = 0;
				rnmainListText(loclGetLocalizationStringByID(loclNOON_LIGHTING_MSG));
				trnTODSetTimeOfDay(43200);	// 1200
			}
#else	// RELEASE_LIENT != 0
			// This code was added to non-release clients (Debug and Optimization)
			// to allow better TOD resolution to help troubleshoot bump mapped surfaces

            static SINT32 times[] =
            {
                0,      //0000
                3600,   //0100
                7200,   //0200
                10800,  //0300
                14400,  //0400
                18000,  //0500
                21600,  //0600
                25200,  //0700
                28800,  //0800
                32400,  //0900
                36000,  //1000
                39600,  //1100
                43200,  //1200
                46800,  //1300
                50400,  //1400
                54000,  //1500
                57600,  //1600
                61200,  //1700
                64800,  //1800
                68400,  //1900
                72000,  //2000
                75600,  //2100
                79200,  //2200
                82800   //2300
            };

            static UINT32 size = sizeof(times) / sizeof(SINT32);
            static bool first = true;
            static UINT32 time = 0;

            if(first)
            {
                first = false;
                trnTODGetTimeOfDay(&time);

                _TimeOfDay = time / 3600;
                _TimeOfDay++;
            }

            _TimeOfDay = _TimeOfDay >= size ? 0 : _TimeOfDay;

            time = times[_TimeOfDay++];
            trnTODSetTimeOfDay(time);

            SINT32 hour = trnTODGetHourFromTimeOfDay(time);

            char buffer[64] = {0};

            strprintf(buffer, hour < 10 ? "0%d00" : "%d00", hour);
            const wchar_t* str = uiUTF8toUTF16(buffer);

            uiChatPrintLine(L"Time set to %s", str);

            ShadowMap::GetInstance().Reset();
#endif	// RELEASE_LIENT != 0
		}

		if (dipKEYDOWN(buffer,DIK_F12) && !dipKEYDOWN(lastbuffer,DIK_F12))
		{
			if (_RateOfTimeTravel == 0)
			{
				_RateOfTimeTravel = 1;
				rnmainListText(loclGetLocalizationStringByID(loclACCELERATED_SUN_MOVEMENT_MSG));
				trnTODSetTimeMultiplier(100.0);
			}
			else
			{
				_RateOfTimeTravel = 0;
				rnmainListText(loclGetLocalizationStringByID(loclNORMAL_SUN_MOVEMENT_MSG));
				trnTODSetTimeMultiplier(1.0);
			}
		}
	}

	return 0;
}

LOGICAL ProcessKeyboardInput()
{
UINT32 i;

	for (i=0;i<MAX_KEYS;i++)
	{
		lastbuffer[i] = buffer[i];
	}

	if (!dipGetKeyboardState(sizeof(buffer), buffer))
	{
        memset(buffer, 0, sizeof(buffer));

		return(FALSE);
	}

	return(TRUE);
}

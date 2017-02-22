//

#include <syscfg.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <string>
#include "pi/pi.h"
#include "include/vars.h"
#include "fio/fio.h"
#include "fio/fiod.h"
#include "rn/rnmain.h"
#include "rn/rntext.h"
//#include "rn/rntextloc.h"
#include "rn/rnlogo.h"
#include "rn/rnvis.h"
#include "ld/ldpix.h"
#include "ob/obc.h"
#include "ob/obcd.h"
#include "ob/obdraw.h"
#include "ob/obmain.h"
#include "ob/obplyr.h"
#include "ob/oblf.h"
#include "ob/obwld.h"
#include "ob/obcs.h"
#include "trn/trnmain.h"
#include "trn/trnsky.h"
#include "trn/trnwtr.h"
#include "trn/trnop.h"
#include "trn/trnvars.h"
#include "trn/trntod.h"
#include "draw/draw2d.h"
#include "cfg/cfgLoad.h"
#include "sn/sndsSoundControl.h"
#include "dbug/dbugExtErrors.h"
#include "pers/pershud.h"
#include "ml/mlCompon.h"
#include "locl/loclLang.h"
#include "veh/vehIcon.h"
#include "trn/trnRadClutter.h"
#include "spt/sptmain.h"
#include "client/GameClient.h"
#include "ui/uiChat.h"
#include "ui/uiSound.h"
#include "dimain.h"
#include "client/clientNetError.h"
#include "auth/authBypassOTP.h"
#include "trn/speedgrass.h"
#include "fx/fxWeather.h"
#include "sdm/BuildTerrain.h"
#include "pi/piinfo.h"
#include "ui/uiDotCommands.h"
#include "map/mapArenaDescription.h"

#include "shader/glsl.h"

#if platOS == OS_WIN32
#include <direct.h>
#endif
#include "math/math3d.h"
#include "comm/commClientMessages.h"
#include "chat/chatClientMessages.h"
#include "strt/strtClientMessages.h"
#include "mm/mmSpawnClientMessages.h"
#include "teul/teulKit.h"
#include "fx/FXInterface.h"
#include "map/mapClientMessages.h"
#include "miss/missionClientMessages.h"
#include "wwii/wwiiClientMessages.h"
#include "gmlp/gmlpmain.h"
#include "pers/persmain.h"
#include "fx/fxMgr.h"
#include "ml/mltypes.h"
#include "weap/weapammo.h"
#include "locl/loclMain.h"
#include "sysmain.h"
#include "sdm/sdm.h"
#include "sdm/StratDB.h"
#include "js/jsClient.h"
#include "sysLaunchUrl.h"
#include "physics/physicsMain.h"
#include "ui/uiMain.h"
#include "cfg/cfgPrefs.h"
#include "fx/TransformEffect.h"
#include "imap/ww2imap.h"
#include "wwii/wwiiNetPorts.h"
#include "gr2/gr2Anim.h"

#if USE_STATESET
#include "ogl/States.h"
#endif

#ifdef MULTI_CLIENT
#include "mc/multiClient.h"
#endif

//#define USE_VTUNE

#define _DUMP_STRAT_TRANSLATIONS
#undef _DUMP_STRAT_TRANSLATIONS
#define _WORLD_VIEW_SCALE_FACTOR 1.0 // scaling factor for the world
#define _COCKPIT_VIEW_SCALE_FACTOR 1.0 // scaling factor for cockpit

#define FRAMEFACTOR_FPS		20
#define FRAMEFACTOR_MINFPS	15
#define FRAMEFACTOR_MINVAL	0.1f

static void PrintDateTime();

static LOGICAL _ParseTicketFile(UINT32* host, UINT16* port, UINT16* protocol, UINT32* cookie);
static LOGICAL _ParseCommandArgs(UINT32* host, UINT16* port, UINT16* protocol, UINT32* otp);

static FLOAT32 _ViewScaleFactor;
static FLOAT32 _ZBufferNearVal;

static UINT32 _DefaultFogColor;
static FLOAT32 _DefaultFogStart;
static FLOAT32 _DefaultFogEnd;
static UINT32	BypassID;

// Globals
extern SINT32 DefaultMaterialId;

static piVECTOR			lastpos, lastrot;

LOGICAL rnmainSetDefaultRenderStates()
{
#if USE_STATESET
    States::InitDefaultAttributes();

    States::SetGlobalDefaultModeValue(GL_CULL_FACE, 1);
    States::SetGlobalDefaultModeValue(GL_LIGHTING, 1);
    States::SetGlobalDefaultModeValue(GL_BLEND, 0);
    States::SetGlobalDefaultModeValue(GL_ALPHA_TEST, 0);
    States::SetGlobalDefaultModeValue(GL_FOG, 1);
    States::SetGlobalDefaultModeValue(GL_DEPTH_TEST, 1);

    States::ApplyRenderModes(nullptr);
#else
    piSetRenderState(piRENDERSTATE_SRCBLEND, SrcBlend);
    piSetRenderState(piRENDERSTATE_DESTBLEND, DestBlend);
    piSetRenderState(piRENDERSTATE_ALPHAREF, piALPHAREF_DEFAULT);
    piSetRenderState(piRENDERSTATE_ALPHAFUNC, piALPHAFUNC_DEFAULT);
    piSetRenderState(piRENDERSTATE_ALPHATESTENABLE, piALPHATESTENABLE_DEFAULT);
    piSetRenderState(piRENDERSTATE_COLORMASK, piRGBA_MAKE(1, 1, 1, 1));
    piSetRenderState(piRENDERSTATE_CULLFACE, GL_FRONT);
    piSetRenderState(piRENDERSTATE_CULLFACEENABLE, piCULLFACEENABLE_DEFAULT);
#endif

    return TRUE;

#if(0)
LOGICAL		retval=FALSE;

	piBeginScene();
    piSetRenderState(piRENDERSTATE_ZENABLE, bZEnable);
    piSetRenderState(piRENDERSTATE_FILLMODE, FillMode);
    piSetRenderState(piRENDERSTATE_SHADEMODE, ShadeMode);
    piSetRenderState(piRENDERSTATE_ZWRITEENABLE, bZWrite);

    piSetRenderState(piRENDERSTATE_CULLMODE, piCULLMODE_DEFAULT);
    piSetRenderState(piRENDERSTATE_ZFUNC, ZFunction);

    piSetRenderState(piRENDERSTATE_ALPHAREF, piALPHAREF_DEFAULT);
    piSetRenderState(piRENDERSTATE_ALPHAFUNC, piALPHAFUNC_DEFAULT);
    piSetRenderState(piRENDERSTATE_DITHERENABLE, bDithering);

    piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
    piSetRenderState(piRENDERSTATE_FOGENABLE, bFogEnabled);
    piSetRenderState(piRENDERSTATE_SPECULARENABLE, bSpecular);
    piSetRenderState(piRENDERSTATE_FOGCOLOR, FogColor);
    piSetRenderState(piRENDERSTATE_CLEARCOLOR, FogColor);
    piSetRenderState(piRENDERSTATE_FOGTABLEMODE, piFOGTABLEMODE_DEFAULT);
    piSetRenderState(piRENDERSTATE_FOGSTART, FogStart);
    piSetRenderState(piRENDERSTATE_FOGEND, FogEnd);
    piSetRenderState(piRENDERSTATE_FOGDENSITY, FogDensity);
    piSetRenderState(piRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
    piSetRenderState(piRENDERSTATE_AMBIENT, piAMBIENT_DEFAULT);
    piSetRenderState(piRENDERSTATE_COLORMASK, piRGBA_MAKE(1, 1, 1, 1));

    piSetRenderStageState(0, piTSS_MINFILTER, TextureMinFilter);
	piSetRenderStageState(0, piTSS_MAGFILTER, TextureMagFilter);

    GLint state[1];

    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, state);
    piSetRenderStageState(0, piTSS_ADDRESSU, state[0]);

    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, state);
    piSetRenderStageState(0, piTSS_ADDRESSV, state[0]);

    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, state);
    piSetRenderStageState(0, piTSS_ADDRESSW, state[0]);

    glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, state);
    piSetRenderStageState(0, piTSS_TEXTURE_ENV_MODE, state[0]);

    glGetTexEnviv(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, state);
    piSetRenderStageState(0, piTSS_SOURCE0_RGB, state[0]);

    glGetTexEnviv(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, state);
    piSetRenderStageState(0, piTSS_OPERAND0_RGB, state[0]);

    glGetTexEnviv(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, state);
    piSetRenderStageState(0, piTSS_SOURCE0_ALPHA, state[0]);

    glGetTexEnviv(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, state);
    piSetRenderStageState(0, piTSS_OPERAND0_ALPHA, state[0]);

    glGetTexEnviv(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, state);
    piSetRenderStageState(0, piTSS_COMBINE_RGB, state[0]);

    glGetTexEnviv(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, state);
    piSetRenderStageState(0, piTSS_COMBINE_ALPHA, state[0]);

	piEndScene();

	if (retval)
	{
		Write_Debug("Encountered an error while setting states in rnSetDefaultStates");
		return(FALSE);
	}
#endif

	return(TRUE);
}

static LOGICAL SetupVariables()
{
	FogStart = piFloatLikeLong(piLongLikeFloat(FogStart) * ScaleFactor * (FLOAT32)_WORLD_VIEW_SCALE_FACTOR);
	FogEnd   = piFloatLikeLong(piLongLikeFloat(FogEnd) * ScaleFactor * (FLOAT32)_WORLD_VIEW_SCALE_FACTOR);

	if (!rnmainSetDefaultRenderStates())
	{
		return(FALSE);
	}

	rnlogoInit();

	piSetAmbLight(piRGBA_MAKE(51, 51, 51, 255));

	PrintDateTime();

	return(TRUE);
}

static int _GetStratData(int scx, int scy, sdm_strat_data* sdmdata)
{
	char filename[64];
	snprintf(filename, 63, "terrain/%04x_%04x.sdm", (scx < 0) ? scx & 0x0000ffff : scx, (scy < 0) ? scy & 0x0000ffff : scy);

	int fd = fioOpen(filename);
	if(fd < 0) return -1;

	sdm_seek_replacement_data(fd, fioRead, fioCallbacks.seek);
	sdm_seek_strat_data(fd, fioRead, fioCallbacks.seek);
	sdm_read_strat_data(fd, fioRead, sdmdata);
	fioClose(fd);

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Find out whether this is going to be an online session or not.

static UINT32 GameHost, GameOTP;
static UINT16 GamePort;

Covert<bool> ProtectedOnlineFlag = false;

bool rnmainGetOnlineSetting()
{
	UINT16 CommType;

	// temp hack until command line params set for going online
	if (BypassString[0] != '\0')
	{
		if (BypassString[0] == 'R')
			BypassID = strtoul(&BypassString[1], nullptr, 10);
		else
			BypassID = strtoul(&BypassString[0], nullptr, 10);
	}
	else
		BypassID = 0;

	if (BypassID != 0 )
	{
		Online = 1;

		if (teulGetHost(GameServerIP, &GameHost) == 0)
		{
			Write_Debug("Unable to translate host address");
			return false;
		}

		GamePort = WWII::Net::Ports::MapHost;
		CommType = PROTOCOL_TCP;
		GameOTP = strtoul(GameServerOTP, nullptr, 10);

#if defined(BETA_RELEASE)
		if(GameOTP == 0)
		{
			GameOTP = AUTH_BYPASS_OTP;
			snprintf(GameServerOTP, sizeof(GameServerOTP)-1, "%u", GameOTP);
		}
#endif
	}
	else
	{
#if defined(AUTH_BYPASS)
		Online = 1;

		#pragma message("AUTH_BYPASS using Host " AUTH_BYPASS)
		if (teulGetHost((UCHAR*) AUTH_BYPASS, &GameHost) == 0)
		{
			Write_Debug("Unable to translate host address");
			return false;
		}

		GamePort = WWII::Net::Ports::MapHost;
		CommType = TCP;
		GameOTP = AUTH_BYPASS_OTP;
#else
		if (TicketFile[0] != 0)
		{
			Online = 1;

			if (!_ParseTicketFile(&GameHost, &GamePort, &CommType, &GameOTP))
			{
				Online = 0;
				return false;
			}
		}
		else if (GameServerIP[0] != 0)
		{
			Online = 1;

			if (!_ParseCommandArgs(&GameHost, &GamePort, &CommType, &GameOTP))
			{
				Online = 0;
				return false;
			}
		}
#endif
	}

  ProtectedOnlineFlag = Online;

	return true;
}

static bool _connectClientToGameNetwork()
{
	try
	{
		UINT32 appversion = 0;

		rnlogoUpdateStatus("loading.network");

        if (gClientV[0] != 0) {
            teulGetHost(gClientV, &appversion);
        } else {
            teulGetHost(PN_product_version_string, &appversion);
        }

		if ( teulMaskNetworkAddress(0) == 0 )
			teulSetNetworkMask(GameHost);

		if ( GameClientLogin(GameHost, BypassID, GameOTP, appversion, GamePort) < 0 )
			throw ClientNetError(-1, "connect.fail");

		if ( PlayerClient()->ChatConnection() < 0 )
			throw ClientNetError(-1, "connect.chat.fail");
	}
	catch (ClientNetError& e)
	{
		const char* key = e.What();

		rnlogoUpdateStatus(key);

#if platOS == OS_WIN32
		MessageBox(nullptr, uiLocString(key).c_str(), uiLocString("connect.popup.title").c_str(), MB_OK | MB_TOPMOST);
#elif platOS == OS_MACOS
        PostModalErrorDialog(uiLocString2("connect.popup.title").c_str(), uiLocString2(key).c_str());
#else
		fprintf(stderr, "NETWORK ERROR: %s", UTF8(message.c_str()));
#endif
		//piExitProgram(_FL, UTF8(message.c_str()));
        return false;
	}

	// Wipe these variables out.
	GameHost = static_cast<UINT32>(random());
	GameOTP = static_cast<UINT32>(random());
	GamePort = static_cast<UINT16>(random());

	return true;
}

// Init everything for rendering
LOGICAL rnmainInit()
{
#if platOS == OS_WIN32
    // OS X and Linux auto-seed the PRNG. Windows, being a backwards OS in need
    // of assistance, doesn't. Therefore, we borrow Windows' equivalent of
    // arc4random() to seed the useless LCRG until we can replace all uses of
    // rand() in the code outright.
    //
    // Oh, and by the way, seeding rand() with the current time is the worst
    // idea ever conceived even if you're NOT looking for cryptographically
    // secure values. Just don't do that. Please.
    unsigned int value;
    rand_s(&value);
    srand(value);
#endif

	rnlogoSplash();

    // Must be first
    glslInit();

	StratDB::Init(_GetStratData);

    uiSoundInit();
	uiInit();
	dimainInit();

#if platOS == OS_WIN32
	// We have to do this here again because a dll loaded with dimainInit() resets the FPU exception mask
	// and we do want to get FPU exceptions for all the other things done here
	// OGS: Not on the Mac, you don't :)
	SetupFPU(true);
#endif

	// Setup variables
	if (!SetupVariables())
	{
		return(FALSE);
	}

	// Initialize the frame factors
	FrameFactor = 1.0f;
	InvFrameFactor = 1.0f;

	/*
	//call for language
	if(!loclLoadLocalizationStringTable(0))
    {
        Write_Debug("[rnmainInit] loclLoadLocalizationStringTable** FAILED**");
        return FALSE;
    }
	*/

	if (!rntextInit())
	{
		return(FALSE);
	}

    if(Online)
    {
		// Time to connect the game to the servers.
		if(_connectClientToGameNetwork() == false)
			return FALSE;

		// send prefs that are shared between the host and client
		// these are originated from the client
		// includes vis player limit
		cfgPrefsSendSharedPrefs();
    }

	// lets check to make sure we have the minimum spec to run the game
	if(!piinfoIsSupportedSystem())
		return FALSE;

	rnlogoUpdateStatus("loading.compress");

	// Default values unless not specified in WOR file
	if (1)
	{
	piVECTOR	pos, rot;

		rot.x = 0.0f;//(float)PI + PI / 2.0f;//1.75f;
		rot.y = 0.0f;
		rot.z = 0.0f;
		rnmainRotatePlayer(&rot);

		pos.x = 0.0f;
		pos.y = 0.0f;
		pos.z = 0.0f;
		rnmainMovePlayer(&pos);

		rnmainResetPlayer();
	}

#ifdef USE_GAME_ADVERTISING
	rnlogoUpdateStatus("loading.extresource");
	if(!adInit())
    {
        // Not an error
        Write_Log(LOG_OLD, LOG_INFO, "[rnmainInit] adInit FAILED");
    }
#endif

	rnlogoUpdateStatus("loading.sounds.init");
	if (!sndsInit())
	{
        Write_Debug("Initializing Sounds FAILED. Disabling sounds");
        // Keep playing without sounds
        bSoundEnable = FALSE;
	}

	rnlogoUpdateStatus("loading.objects");
	if (!obmainInit())
	{
		return(FALSE);
	}

	rnlogoUpdateStatus("loading.trees");
    if(!sptmainInit())
    {
        return FALSE;
    }

	if(!rnmainSetFogAttributes())
	{
		return FALSE;
	}

	if (!piSetFieldOfView(FieldOfView, 0.1f, piLongLikeFloat(FogEnd)))
	{
		return(FALSE);
	}

#if(0)
	rnlogoUpdateStatus("loading.fx");
	if(!FXInitialize())
    {
        Write_Debug("[rnmainInit] FXInitialized** FAILED**");
        return FALSE;
    }
#endif

    InitializeFxSystem();
    fxMgrInit();
	rnlogoUpdateStatus("loading.gameloop");

    if (gmlpMainInit())
	{
		return FALSE;
	}

	/* tutorial testing by ramp
	if(Online)
	{
		try
		{
			StratDB::StratObject* strat = StratDB::FindStratObject("Antwerp");
			HCUnit* hcUnit = new HCUnit(WWII_ENGLAND, HCU_BRIGADE, 0, WWII_ENGLAND, BRANCH_LAND, BRANCH_LAND, 9, 0, "English Training Brigade", "1.uk", 1, (strat) ? *strat->FullObjID(): nullObjID, 0, 0);
			hcUnit->addToRegistry();

			strat = StratDB::FindStratObject("Lier");
			hcUnit = new HCUnit(WWII_FRANCE, HCU_BRIGADE, 0, WWII_FRANCE, BRANCH_LAND, BRANCH_LAND, 9, 0, "French Training Brigade", "1.fr", 1, (strat) ? *strat->FullObjID(): nullObjID, 0, 0);
			hcUnit->addToRegistry();

			strat = StratDB::FindStratObject("Schilde");
			hcUnit = new HCUnit(WWII_GERMANY, HCU_BRIGADE, 0, WWII_GERMANY, BRANCH_LAND, BRANCH_LAND, 9, 0, "German Training Brigade", "1.de", 1, (strat) ? *strat->FullObjID(): nullObjID, 0, 0);
			hcUnit->addToRegistry();
		}
		catch(std::runtime_error)
		{
			// do nothing
		}
	}
	*/

	// Should be the last function called in Init!
	piReleaseTextureHandles();

	_ViewScaleFactor = _WORLD_VIEW_SCALE_FACTOR;

	uiPostInit();

	if(!Online)
	{
		uiLoadView("offline");
	}
	else
	{
		// a bit of conditional logic to determine online entry page
		// default to campaign mode only
		SINT32 playMode = 1;

		ARENA_VARS::iterator found = g_arenaDescription.find("play_mode");

		if(found != g_arenaDescription.end())
			playMode = atoi(found->second.c_str());

		if(playMode == 2)
			uiLoadView("scenarios/lobby");
		else if(playMode == 1)
			uiLoadView("persona");
		else
			uiLoadView("online");
	}

	uiChatShow();

	// make sure devices get detected
	// testing for now
	dimainKill();
	dimainInit();
	imapLoadPresets();
	// end testing

#ifdef MULTI_CLIENT
	multiClientInit();
#endif

	return(TRUE);
}

LOGICAL rnmainSetFogAttributes()
{
	FLOAT32 fHorizon;
	FLOAT32 fStart;
	FLOAT32 fEnd;

	fHorizon = 0.0f;
	trnmainGetHorizon(&fHorizon);

	// Set FogStart and FogEnd as a percentage of the Horizon.
	// These percentages are WAGs.
	fStart = fHorizon * 0.3f;
	fEnd = fHorizon * 0.95f;

	_DefaultFogColor = FogColor;
	_DefaultFogStart = fStart;
	_DefaultFogEnd = fEnd;

	FogStart = piFloatLikeLong(fStart);
	FogEnd = piFloatLikeLong(fEnd);

	piBeginScene();

    piSetRenderState(piRENDERSTATE_FOGENABLE, TRUE);
    piSetRenderState(piRENDERSTATE_FOGCOLOR, FogColor);
    piSetRenderState(piRENDERSTATE_CLEARCOLOR, FogColor);
    piSetRenderState(piRENDERSTATE_FOGSTART, FogStart);
    piSetRenderState(piRENDERSTATE_FOGEND, FogEnd);
    piSetRenderState(piRENDERSTATE_FOGDENSITY, FogDensity);
    piSetRenderState(piRENDERSTATE_FOGTABLEMODE, piFOGTABLEMODE_DEFAULT);
    piSetRenderState(piRENDERSTATE_FOGVERTEXMODE, piFOGVERTEXMODE_DEFAULT);
    piSetRenderState(piRENDERSTATE_RANGEFOGENABLE, piRANGEFOGENABLE_DEFAULT);

	piEndScene();

	return TRUE;
}

LOGICAL rnmainAdjustFogAttributes(UINT32 FogColor,FLOAT32 FogStartFloat,FLOAT32 FogEndFloat)
{
	FogStart = piFloatLikeLong(FogStartFloat);
	FogEnd = piFloatLikeLong(FogEndFloat);

    piSetRenderState(piRENDERSTATE_FOGCOLOR, FogColor);
    piSetRenderState(piRENDERSTATE_CLEARCOLOR, FogColor);
    piSetRenderState(piRENDERSTATE_FOGSTART, FogStart);
    piSetRenderState(piRENDERSTATE_FOGEND, FogEnd);

	return TRUE;
}

void rnmainSetDefaultFogStart(FLOAT32 start)
{
    _DefaultFogStart = start;
}

void rnmainSetDefaultFogEnd(FLOAT32 end)
{
    _DefaultFogEnd = end;
}

void rnmainSetDefaultFogColor(UINT32 color)
{
    _DefaultFogColor = color;
}

FLOAT32 rnmainGetDefaultFogStart()
{
    return _DefaultFogStart;
}

FLOAT32 rnmainGetDefaultFogEnd()
{
    return _DefaultFogEnd;
}

UINT32 rnmainGetDefaultFogColor()
{
    return _DefaultFogColor;
}

LOGICAL rnmainGetDefaultFogAttributes(UINT32 * OutFogColor, FLOAT32 * OutFogStart, FLOAT32 * OutFogEnd)
{
	*OutFogColor = _DefaultFogColor;
	*OutFogStart = _DefaultFogStart;
	*OutFogEnd = _DefaultFogEnd;

	return TRUE;
}

LOGICAL rnmainFOVChange()
{
	if (FieldOfView < 15.0f)
	{
		FieldOfView = 15.0f;
	}
	else
	if (FieldOfView < 30.0f)
	{
		FieldOfView = 30.0f;
	}
	else
	if (FieldOfView < 45.0f)
	{
		FieldOfView = 45.0f;
	}
	else
	if (FieldOfView < 60.0f)
	{
		FieldOfView = 60.0f;
	}
	else
	if (FieldOfView < 75.0f)
	{
		FieldOfView = 75.0f;
	}
	else
	if (FieldOfView < 90.0f)
	{
		FieldOfView = 90.0f;
	}
	else
	if (FieldOfView == 90.0f)
	{
		FieldOfView = 15.0f;
	}

	{
		FLOAT32	horizon;

		trnmainGetHorizon(&horizon);
		if (!piSetFieldOfView(FieldOfView, 0.1f, piLongLikeFloat(FogEnd)))
		{
			return(FALSE);
		}
	}

	if (!rnmainListText(loclGetLocalizationStringByID(loclFIELD_OF_VIEW_MSG), FieldOfView))
	{
		return(FALSE);
	}

	return(TRUE);
}

// Kill the render engine
void rnmainKill()
{
#ifdef MULTI_CLIENT
	multiClientKill(true);
#endif

	if (Online == 1)
	{
		Online = 0;
		ww2::Client::Kill();
	}

#ifdef USE_GAME_ADVERTISING
    adKill();
#endif

	sptExitApp();

    ///////////////////////////////////////////
    // Placed here in case of an "Alt F4" exit.
    ///////////////////////////////////////////
	gmlpMainKill();

	//FXKillAll();

	if (ScaleFactor != 0.0f)
	{
		Write_Debug("Player position: x=%f, y=%f, z=%f", playpos.x/ScaleFactor, playpos.y/ScaleFactor, playpos.z/ScaleFactor);
		Write_Debug("Player rotation: x=%f, y=%f, z=%f", playrot.x, playrot.y, playrot.z);
	}

	obmainKill();
	physicsKill();
	rntextKill();
    GameClientKill();

	sndsKill();
    glslKill();

    Write_Log(LOG_OLD, LOG_INFO, "[rnmainKill]");
}

void rnmainSetPlayerInfo(piVECTOR* lppos, piVECTOR* lprot)
{
	playrot = *lprot;
	playpos = *lppos;
}

/*
 * Updates the matrix, which is equivalent to having the camera at a
 * specified rotation.
 * lpmat    Matrix to update.
 */
static void RotatePlayer(piMATRIX* lpmat, piVECTOR* lprot)
{
	// rotate the player on his axis
	// (must be done before translation)
	math3dMatrixRotate(lpmat, lprot);
}

/*
 * Updates the matrix, which is equivalent to having the camera at a
 * specified position.
 * lpmat    Matrix to update.
 */
static void TranslatePlayer(piMATRIX* lpmat, piVECTOR* lppos)
{
#ifdef WANT_WASTED_CPU
piMATRIX	tmp;

	// translate the player to his new position
	tmp = IdentityMatrix;
	tmp._41 = lppos->x;
	tmp._42 = lppos->y;
	tmp._43 = lppos->z;

	math3dMatrixMultiply(lpmat, &tmp, lpmat);
#else
    // Reduce 64 multiplies down to 9
    lpmat->_41 = lppos->x * lpmat->_11 + lppos->y * lpmat->_21 + lppos->z * lpmat->_31 + lpmat->_41;
    lpmat->_42 = lppos->x * lpmat->_12 + lppos->y * lpmat->_22 + lppos->z * lpmat->_32 + lpmat->_42;
    lpmat->_43 = lppos->x * lpmat->_13 + lppos->y * lpmat->_23 + lppos->z * lpmat->_33 + lpmat->_43;
#endif
}

void rnmainSetViewMatrix(piVECTOR* pos, piVECTOR* att)
{
    piMATRIX mat, mat1, mat2, mat4;
    piMATRIX DistortionMat;

	mat = IdentityMatrix;

	RotatePlayer(&mat, att);
	TranslatePlayer(&mat, pos);

	playrot = *att;

	playright.x = mat._11;
	playright.y = mat._21;
	playright.z = mat._31;
	playup.x = mat._12;
	playup.y = mat._22;
	playup.z = mat._32;
	playdir.x = mat._13;
	playdir.y = mat._23;
	playdir.z = mat._33;
	playtrans.x = mat._41;
	playtrans.y = mat._42;
	playtrans.z = mat._43;

	playpos.x = mat._11 * mat._41 + mat._12 * mat._42 + mat._13 * mat._43;
	playpos.y = mat._21 * mat._41 + mat._22 * mat._42 + mat._23 * mat._43;
	playpos.z = mat._31 * mat._41 + mat._32 * mat._42 + mat._33 * mat._43;

	rnvisSetupClipPlanes();

	// scale the view matrix
	mat1 = IdentityMatrix;

	mat1._11 = _ViewScaleFactor;
	mat1._22 = _ViewScaleFactor;
	mat1._33 = _ViewScaleFactor;

	mat2 = mat;

	// g distortion effects
	persGetGDistortion(&DistortionMat);
	mat4 = IdentityMatrix;
	mat4._41 = playpos.x;
	mat4._42 = playpos.y;
	mat4._43 = playpos.z;
	math3dMatrixMultiply(&DistortionMat,&mat4,&DistortionMat);
	mat4._41 = -mat4._41;
	mat4._42 = -mat4._42;
	mat4._43 = -mat4._43;
	math3dMatrixMultiply(&DistortionMat,&DistortionMat,&mat4);

	math3dMatrixMultiply(&mat, &DistortionMat, &mat);

	// update the view matrix
	piSetViewMatrix(&mat);
}

void rnmainSetViewMatrix2(piMATRIX* view)
{
    piMATRIX mat = *view;

    playright.x = mat._11;
    playright.y = mat._21;
    playright.z = mat._31;

    playup.x = mat._12;
    playup.y = mat._22;
    playup.z = mat._32;

    playdir.x = mat._13;
    playdir.y = mat._23;
    playdir.z = mat._33;

    playtrans.x = mat._41;
    playtrans.y = mat._42;
    playtrans.z = mat._43;

	playpos.x = mat._11 * mat._41 + mat._12 * mat._42 + mat._13 * mat._43;
	playpos.y = mat._21 * mat._41 + mat._22 * mat._42 + mat._23 * mat._43;
	playpos.z = mat._31 * mat._41 + mat._32 * mat._42 + mat._33 * mat._43;

    rnvisSetupClipPlanes();

    piSetUnscaledViewMatrix(&mat);

    // scale the view matrix
    piMATRIX mat1 = IdentityMatrix;

    mat1._11 = _ViewScaleFactor;
    mat1._22 = _ViewScaleFactor;
    mat1._33 = _ViewScaleFactor;

    piMATRIX mat2 = mat;

    piMATRIX DistortionMat = IdentityMatrix;

    // g distortion effects
    persGetGDistortion(&DistortionMat);

    piMATRIX mat4 = IdentityMatrix;

    mat4._41 = playpos.x;
    mat4._42 = playpos.y;
    mat4._43 = playpos.z;

    math3dMatrixMultiply(&DistortionMat, &mat4, &DistortionMat);

    mat4._41 = -mat4._41;
    mat4._42 = -mat4._42;
    mat4._43 = -mat4._43;

    math3dMatrixMultiply(&DistortionMat, &DistortionMat, &mat4);

    math3dMatrixMultiply(&mat, &DistortionMat, &mat);

    // update the view matrix
    piSetViewMatrix(&mat);

    // apply the scaling factor
    // and the g distortion effect
    mat1 = IdentityMatrix;

    if (bZEnable != piZB_USEW)
    {
        mat1._11 = _COCKPIT_VIEW_SCALE_FACTOR;
        mat1._22 = _COCKPIT_VIEW_SCALE_FACTOR;
        mat1._33 = _COCKPIT_VIEW_SCALE_FACTOR;
    }

    math3dMatrixMultiply(&mat2, &DistortionMat, &mat2);
    math3dMatrixMultiply(&mat, &mat2, &mat1);

    piSetCockpitViewMatrix(&mat);
}

static void RotateTranslatePlayer()
{
piMATRIX    mat,mat1,mat2,mat4;
piMATRIX	DistortionMat;
piVECTOR	Pos;
piVECTOR	Att;

	// correct the camera if necessary
	persGetCameraPosAndAtt(&Pos, &Att);
	persShakeCamera(-1); // reset shake

	mat = IdentityMatrix;

	RotatePlayer(&mat, &Att);
	TranslatePlayer(&mat, &Pos);

	//math3dOrientAngles(&playrot, &Att);
	playrot = Att;

	//math3dCalcNormalVector(&playdir, &playrot);
	playright.x = mat._11;
	playright.y = mat._21;
	playright.z = mat._31;
	playup.x = mat._12;
	playup.y = mat._22;
	playup.z = mat._32;
	playdir.x = mat._13;
	playdir.y = mat._23;
	playdir.z = mat._33;
	playtrans.x = mat._41;
	playtrans.y = mat._42;
	playtrans.z = mat._43;

	playpos.x = mat._11 * mat._41 + mat._12 * mat._42 + mat._13 * mat._43;
	playpos.y = mat._21 * mat._41 + mat._22 * mat._42 + mat._23 * mat._43;
	playpos.z = mat._31 * mat._41 + mat._32 * mat._42 + mat._33 * mat._43;

	rnvisSetupClipPlanes();

	piSetUnscaledViewMatrix(&mat);

	// scale the view matrix
	mat1 = IdentityMatrix;

	mat1._11 = _ViewScaleFactor;
	mat1._22 = _ViewScaleFactor;
	mat1._33 = _ViewScaleFactor;

	mat2 = mat;

	// g distortion effects
	persGetGDistortion(&DistortionMat);
	mat4 = IdentityMatrix;
	mat4._41 = playpos.x;
	mat4._42 = playpos.y;
	mat4._43 = playpos.z;
	math3dMatrixMultiply(&DistortionMat,&mat4,&DistortionMat);
	mat4._41 = -mat4._41;
	mat4._42 = -mat4._42;
	mat4._43 = -mat4._43;
	math3dMatrixMultiply(&DistortionMat,&DistortionMat,&mat4);

	math3dMatrixMultiply(&mat,&DistortionMat,&mat);

	// update the view matrix
	piSetViewMatrix(&mat);

	weapAmmoSetRoundPos(&Pos);

	// apply the scaling factor
	// and the g distortion effect
	mat1 = IdentityMatrix;
	if (bZEnable != piZB_USEW)
	{
		mat1._11 = _COCKPIT_VIEW_SCALE_FACTOR;
		mat1._22 = _COCKPIT_VIEW_SCALE_FACTOR;
		mat1._33 = _COCKPIT_VIEW_SCALE_FACTOR;
	}
	math3dMatrixMultiply(&mat2,&DistortionMat,&mat2);
	math3dMatrixMultiply(&mat,&mat2,&mat1);

	piSetCockpitViewMatrix(&mat);
}

void rnmainResetPlayer()
{
piMATRIX    mat;

	// rotate the view
	playrot.x = 0.0f;//lastrot.x;
	playrot.y = 0.0f;//lastrot.y;
	playrot.z = 0.0f;//lastrot.z;
	deltarot.x = lastrot.x;
	deltarot.y = lastrot.y;
	deltarot.z = lastrot.z;

	// translate the view
	playpos.x = 0.0f;//lastpos.x;
	playpos.y = 0.0f;//lastpos.y;
	playpos.z = 0.0f;//lastpos.z;
	deltapos.x = lastpos.x;
	deltapos.y = lastpos.y;
	deltapos.z = lastpos.z;

	mat = IdentityMatrix;
	piSetViewMatrix(&mat);

	RotateTranslatePlayer();
}

void rnmainRotatePlayer(piVECTOR* lprot)
{
	lastrot = *lprot;
}

void rnmainMovePlayer(piVECTOR* lppos)
{
	lastpos.x = lppos->x * ScaleFactor;
	lastpos.y = lppos->y * ScaleFactor;
	lastpos.z = lppos->z * ScaleFactor;
}

void rnUpdateView()
{
	if (ft50msticks != 0)
	{
	piVECTOR  oldpos;

		{
			deltapos.x = 0.0f;
			deltapos.y = 0.0f;
			deltapos.z = 0.0f;
			deltarot.x = 0.0f;
			deltarot.y = 0.0f;
			deltarot.z = 0.0f;

			oldpos = playpos;

			if (deltapos.x != 0.0f ||
				deltapos.y != 0.0f ||
				deltapos.z != 0.0f)
			{
				playpos.x -= playright.x * deltapos.x;
				playpos.x -= playup.x * deltapos.z;
				playpos.x -= playdir.x * deltapos.y;

				playpos.y -= playright.y * deltapos.x;
				playpos.y -= playup.y * deltapos.z;
				playpos.y -= playdir.y * deltapos.y;

				playpos.z -= playright.z * deltapos.x;
				playpos.z -= playup.z * deltapos.z;
				playpos.z -= playdir.z * deltapos.y;
				obcdPlayer(&oldpos, &playpos);
			}
			deltapos.x = playpos.x - oldpos.x;
			deltapos.y = playpos.y - oldpos.y;
			deltapos.z = playpos.z - oldpos.z;
		}

//		RotateTranslatePlayer();
	}

	RotateTranslatePlayer();
}

LOGICAL rnmainMoveView(FLOAT64 PosX, FLOAT64 PosY)
{
	mlVehicleData*	VehicleData = persGetPersonaModelDataPtr();
	VehicleData->VehiclePos.x = PosX;
	VehicleData->VehiclePos.y = PosY;
	//VehicleData->VehiclePos.z = 0;

	return(TRUE);
}

static void PrintDateTime()
{
    char buf[256] = {0};
    struct tm t;
    time_t p = time(nullptr);

    localtime_r(&p, &t);
    strftime(buf, 255, "Date and time: %F %T %z", &t);
    Write_Debug("%s", buf);
}


UINT32 rnmainGetViewScaleFactor(FLOAT32 * ScaleFactor)
{
	if (ScaleFactor != nullptr)
	{
		*ScaleFactor = _ViewScaleFactor;
	}

	return 0;
}

UINT32 rnmainSetViewScaleFactor(FLOAT32 ScaleFactor)
{
	_ViewScaleFactor = ScaleFactor;

	return 0;
}

float rnmainGetZBufferNearVal()
{
    return _ZBufferNearVal;
}

UINT32 rnmainSetZBufferNearVal(FLOAT32 ZBufferNearVal)
{
	_ZBufferNearVal = ZBufferNearVal;

	return 0;
}

UINT32 rnmainGetZBufferNearVal(FLOAT32* ZBufferNearVal)
{
	if (ZBufferNearVal != nullptr)
	{
		*ZBufferNearVal = _ZBufferNearVal;
	}

	return 0;
}

#if(0)
static void OutputStatusMessageToFile(SINT8* File, SINT32 Line, SINT8* MessageString)
{
	FILE* ExitLog = Fio::OpenLogFile();

	Write_Debug("Status handler information...");

	if (MessageString == nullptr)
	{
		if ( ExitLog )
			fprintf(ExitLog, "No exit message!\n");
		Write_Debug("No exit message!");
	}
	else
	{
		if ( ExitLog )
			fprintf(ExitLog, "%s\n", MessageString);
		Write_Debug("%s", MessageString);
	}

	if ( ExitLog )
		fprintf(ExitLog,"File: %s, Line: %d\n", File, Line);
	Write_Debug("File: %s, Line: %d", File, Line);

	if ( ExitLog )
	{
		fflush(ExitLog);
		fclose(ExitLog);
	}
}
#endif

/*
	TicketFile looks like this:

apID: 1
data: Login Accepted
serverIP: 0xd0868f85
port: 31337
protocol: 2
OTP: 602544593
*/

inline std::string& rtrim(std::string& s, const char* t = " \t\n\r\f\v")
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

static LOGICAL _ParseTicketFile(UINT32* host, UINT16* port, UINT16* protocol, UINT32* cookie)
{
    LOGICAL	retval=TRUE;
    std::string apID, data, serverIP, serverport, serverprotocol, otp, bypass;

    try {
        std::ifstream  ticketFileStream (TicketFile);
        std::getline(ticketFileStream, apID); // ignore
        std::getline(ticketFileStream, data); // ignore
        std::getline(ticketFileStream, serverIP);
        std::string temp =serverIP.substr(serverIP.find(" ")+1);
        std::string ipAddress = rtrim(temp);
        if (!ipAddress.empty())
        {
            *host = strtoul(ipAddress.c_str(), nullptr, 10);
        }
        std::getline(ticketFileStream, serverport);
        std::string ipPort = serverport.substr(serverport.find(" ")+1);
        if (!ipPort.empty())
        {
            *port = static_cast<UINT16>(strtoul(ipPort.c_str(), nullptr, 10));
        }
        std::getline(ticketFileStream, serverprotocol);
        std::string ipProt = serverprotocol.substr(serverprotocol.find(" ")+1);
        if (!ipProt.empty())
        {
            *protocol = static_cast<UINT16>(strtoul(ipProt.c_str(), nullptr, 10));
        }
        std::getline(ticketFileStream, otp); // cookie
        std::string otpText = otp.substr(otp.find(" ")+1);
        if (!otpText.empty())
        {
            *cookie = static_cast<UINT32>(strtoul(otpText.c_str(), nullptr, 10));
        }

        std::getline(ticketFileStream, bypass);
        if (!bypass.empty())
        {
            if (teulGetHost(ipAddress.c_str(), host) == 0)
            {
                Write_Debug("Unable to translate host address");
                return (FALSE);
            }

            std::string userId = bypass.substr(bypass.find(" ")+1);
            if (!userId.empty())
            {
                strncpy(BypassString,userId.c_str(),32);

                if (BypassString[0] != '\0')
                {
                    if (BypassString[0] == 'R')
                        BypassID = strtoul(&BypassString[1], nullptr, 10);
                    else
                        BypassID = strtoul(&BypassString[0], nullptr, 10);
                }
                else
                    BypassID = 0;
            }

        }
    } catch (std::ifstream::failure e) {
        Write_Debug( e.what());
        retval = FALSE;
        Online = 0;
    }


	return(retval);
}

static LOGICAL _ParseCommandArgs(UINT32* host, UINT16* port, UINT16* protocol, UINT32* otp)
{

	if (GameServerIP[0] == 0)
		return FALSE;
	if (GameServerPort[0] == 0)
		return FALSE;
	if (GameServerOTP[0] == 0)
		return FALSE;

	if (teulGetHost(GameServerIP, host) == 0)
	{
		Write_Debug("Unable to translate host address");
		return (FALSE);
	}

	*port = (UINT16)strtoul(GameServerPort, nullptr, 10);
	*protocol = PROTOCOL_TCP;

	*otp = strtoul(GameServerOTP, nullptr, 10);

	return TRUE;

}

#if DEBUG_SQRT
#undef sqrt
double callSqrt(const char* file, int line, const double& n)
{
	double value = sqrt(n);

	if(_isnan(value) || !_finite(value) || n < 0.0)
	{
		rnmainListText("%s %d value=%f n=%f", file, line, value, n);
	}

	return value;
}

float callSqrt(const char* file, int line, const float& n)
{
	float value = sqrt(n);

	if(_isnan(value) || !_finite(value) || n < 0.0)
	{
		rnmainListText("%s %d value=%f n=%f", file, line, value, n);
	}

	return value;
}

#endif

#if DEBUG_POW
#undef pow
double callPow(const char* file, int line, const double& x, const double& y)
{
	double value = pow(x, y);

	if(_isnan(value) || !_finite(value))
	{
		rnmainListText("%s %d value=%f x=%f y=%f", file, line, value, x, y);
	}

	return value;
}

float callPow(const char* file, int line, const float& x, const float& y)
{
	float value = pow(x, y);

	if(_isnan(value) || !_finite(value))
	{
		rnmainListText("%s %d value=%f x=%f y=%f", file, line, value, x, y);
	}

	return value;
}

#if DEBUG_ATAN2
#undef atan2
double callAtan2(const char* file, int line, const double& x, const double& y)
{
	double value = atan2(x, y);

	if(_isnan(value) || !_finite(value))
	{
		rnmainListText("%s %d value=%f x=%f y=%f", file, line, value, x, y);
	}

	return value;
}

float callAtan2(const char* file, int line, const float& x, const float& y)
{
	float value = atan2(x, y);

	if(_isnan(value) || !_finite(value))
	{
		rnmainListText("%s %d value=%f x=%f y=%f", file, line, value, x, y);
	}

	return value;
}
#endif

#ifdef DEBUG_MATH3DNORMALIZE
#undef math3dNormalize

piVECTOR* callMath3Dnormalize(const char* file, int line, piVECTOR* v)
{
    float inv_mod;

	inv_mod = v->x * v->x + v->y * v->y + v->z * v->z;
	if (inv_mod)
	{
		inv_mod = (float)(1.0 / sqrt(inv_mod));

		v->x *= inv_mod;
		v->y *= inv_mod;
		v->z *= inv_mod;
	}

    if(_isnan(v->x) || _isnan(v->y) || _isnan(v->z))
    {
        rnmainListText("math3dNormalize %s %d", file, line);
    }

    return v;
}
#endif

#endif

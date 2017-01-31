#include <syscfg.h> // Path is project specific.

#include <math.h>

#include "pi/pi.h"
#include "include/vars.h"

#include "dip.h"
#include "ob/obmain.h"
#include "ob/obdraw.h"
#include "ob/obwld.h"
#include "ob/oblf.h"
#include "ob/obpool.h"
#include "rn/rnmain.h"
#include "trn/trnmain.h"
#include "trn/trnsky.h"
#include "trn/trnwtr.h"
#include "trn/trntod.h"
#include "trn/trnautogun.h"
#include "fx/FXInterface.h"
#include "fx/fxMgr.h"
#include "rn/rnlogo.h"
#include "dimain.h"
#include "ld/ldmdl.h"
#include "gmlp/gmlpmain.h"
#include "draw/draw3d.h"
#include "shader/glslWater.h"
#include "shader/glslSpecular.h"
#include "shader/glslNormal.h"
#include "veh/vehload.h"
#include "sdm/StratDB.h"
#include "ui/uiMain.h"
#include "spt/sptmain.h"
#include "pers/persmain.h"
#include "pers/persgunsight.h"
#include "pers/persbombsight.h"
#include "pers/persmsg.h"
#include "pers/pershud.h"
#include "veh/vehmain.h"
#include "veh/vehupdate.h"
#include "veh/vehcomms.h"
#include "ml/mltypes.h"
#include "ml/mlgauges.h"
#include "imap/imaphack.h"
#include "weap/weapmain.h"
#include "weap/weapammo.h"
#include "rn/rnmain.h"
#include "math/math3d.h"
#include "math/mathnum.h"
#include "comm/commClientMessages.h"
#include "strt/strtClientMessages.h"
#include "wwii/wwiiClientMessages.h"
#include "locl/loclMain.h"
#include "trn/trnclutter.h"
#include "trn/trncloud.h"
#include "gmlp/gmlpmain.h"
#include "imap/imap.h"
#include "imap/ww2imap.h"
#include "fp/fpFPS.h"
#include "fp/fpFPSdata.h"
#include "veh/vehIcon.h"
#include "sdm/sdm.h"
#include "ui/uiSound.h"
#include "cfg/cfgPrefs.h"
#include "client/GameClient.h"
#include "trn/trngrass.h"
#include "ob/obdshad.h"
#include "client/clientServerTrackedObjects.h"
#include "weap/weapList.h"
#include "wwii/wwiiServerTrackedObject.h"
#include "physics/physicsMain.h"
#include "ddp.h"
#include "trn/trnvars.h"
#include "shader/glsl.h"
#include "fx/TransformEffect.h"
#include "osgImport/ShadowMap.h"
#include "fx/fxWeather.h"
#include "trn/trnRadClutter.h"
#include "trn/speedgrass.h"
#include "fx/postRender.h"
#include "gr2/gr2Anim.h"
#include "fx/fxMisc.h"
#include "scen/ScenarioManager.h"
#include "ogl/GLCounter.h"
#include "maptools/GenerateIcons.h"
#include "sdm/BuildTerrain.h"
#include "wwii/wwiiPPOLoader.h"

#ifdef MULTI_CLIENT
#include "mc/multiClient.h"
#endif

#ifdef GENICONS
#include "GenerateIcons.h"
#endif

static void _UpdateInit();
static void _UpdateWorld();
static void _PreRenderUpdateDeprecated();
static void _RenderDeprecated();
static void _PostRenderUpdateDeprecated();
static void _UpdateKill();
void UpdateGameClocks();
static LOGICAL _PostExternalDrawFunc(LOGICAL IsBeforeStage);
static LOGICAL _PostInternalDrawFunc(LOGICAL IsBeforeStage);
static LOGICAL _PostUnsortedDrawFunc(LOGICAL IsBeforeStage);
static void _CalcFPS();

PPOSpecLoader specLoader;

#define AUTO_AFK_PERIOD 300000	// after 5 minutes, auto-afk turns on
#define AUTO_AFK_CHECK 5000		// auto-afk checks every 5 seconds
#define AUTO_AFK_BUFFER 1000	// little extra time between manually turning on afk and checking

#ifdef BETA_RELEASE
#include "ob/obc.h"
FLOAT32 logTimerFPS;
#endif

#ifdef SPATIAL_DB
#include "SpatialDB.h"
#endif

static void preload_sdms()
{
// whole map -26, 210, 79 wide 72 high
	for(int sy = 210; sy < 210+72; sy++)
	{
		rnUpdateLoadingScreen();
		for(int sx = -26; sx < -26+79; sx++)
		{
			StratDB::LoadSuperCell(sx, sy);
		}
	}

#if defined(_DUMP_STRAT_TRANSLATIONS)
	StratDB::StratObjList stratlist;

	FILE* report = nullptr;
	errno_t err = fopen_s(&report, "c:\\projects\\stratlist.txt", "w");

	if(!err)
	{
		StratDB::AllCPsInWorld(stratlist);
		StratDB::StratObjList::iterator it = stratlist.begin();

		for(; it != stratlist.end(); ++it)
		{
			StratDB::StratObject* s = *it;

			fprintf(report, "strat.%lu = %s\n", (unsigned long)s->FullObjID()->Key(), s->Name());

			StratDB::StratObject::ChildList& children = s->ChildStratObjects();
			for(int i = 0; i < (int)children.size(); i++)
			{
				StratDB::StratObject* c = children[i];
				fprintf(report, "strat.%lu = %s\n", (unsigned long)c->FullObjID()->Key(), c->Name());
			}
		}

		stratlist.clear();

		StratDB::FindStratObjects(stratlist, SDM_CP_BRIDGE);
		it = stratlist.begin();

		for(; it != stratlist.end(); ++it)
		{
			StratDB::StratObject* s = *it;
			fprintf(report, "strat.%lu = %s\n", (unsigned long)s->FullObjID()->Key(), s->Name());
		}

		fclose(report);
	}
#endif
}

LOGICAL gmlpMainInit()
{
    ShadowMap::GetInstance().init();
    ShadowMap::GetInstance().setPolygonOffset(Vec2f(8, 4));

	ww2imapInit();

	// initialize the PPO mapping, eliminated lazy load
	specLoader.loadSpecMap();

	rnlogoUpdateStatus("loading.gameloop");

    mathSeedRandomNumberGenerator();

	rnlogoUpdateStatus("loading.blingmeters");
	mlGaugeInit();

	rnlogoUpdateStatus("loading.fps");
	persInit();

	rnlogoUpdateStatus("loading.weapons");
	weapInit();
	trnInitAutogunSystem(); // must occur after weapInit

	rnlogoUpdateStatus("loading.vehicles.init");
	vehInit();

	rnlogoUpdateStatus("loading.beefjerky");
	mlInit();

	rnlogoUpdateStatus("loading.mspaint");
	piResetRenderBuckets();

	obdrawInstallDrawCallback(_PostExternalDrawFunc,OBDRAW_SORTED_EXTERNAL_DRAW);
	obdrawInstallDrawCallback(_PostInternalDrawFunc,OBDRAW_SORTED_INTERNAL_DRAW);
    obdrawInstallDrawCallback(_PostUnsortedDrawFunc, OBDRAW_UNSORTED_EXTERNAL_DRAW);

	rnlogoUpdateStatus("loading.stuff.on.the.ground");
	trnClutterInit();

	rnlogoUpdateStatus("loading.eyepatches");
	persGunsightInit();

	rnlogoUpdateStatus("loading.uglies");
	persBombsightInit();

	rnlogoUpdateStatus("loading.nukes");
	physicsInit();

	rnlogoUpdateStatus("loading.grass");
    speedgrassInit();

	rnlogoUpdateStatus("loading.weather");
    fxWeatherInit();

	// dropped "Online" requirement because the offline map cp finder was breaking
	if(g_buildTerrain)
	{
		rnlogoUpdateStatus("loading.bsdms");
		BuildTerrain(true, 0, 0);
	}

	if(!LowMemorySystem && !NoBlitzWorPreload)
	{
		rnlogoUpdateStatus("loading.sdms");
		preload_sdms();
	}

    PostRenderFilter::GetInstance().init();

	rnlogoUpdateStatus("loading.ninetynine.ninenine.percent");
    rnUpdateLoadingEnd();

	rnlogoKill();

	uiSetCursor(1);

    return 0;
}

#if USE_STATESET
/*----------------------------------------------------------------------
Function: PostDrawCallbacks
Description:
-----------------------------------------------------------------------*/
void PostDrawCallbacks()
{
    bool render = uiInGame() && (Online ? g_bVehicleAttached : TRUE);

    if(!render)
        return;

    fxMiscDrawEffects();
    PostRenderFilter::GetInstance().apply();
    persDrawRedoutBlackout();
}
#else
/*----------------------------------------------------------------------
Function: PostDrawCallbacks
Description:
-----------------------------------------------------------------------*/
void PostDrawCallbacks()
{
    bool render = uiInGame() && (Online ? g_bVehicleAttached : TRUE);

    if(!render)
        return;

    logTimerCallVoid(fxMiscDrawEffects);
    PostRenderFilter::GetInstance().apply();
    logTimerCallVoid(persDrawRedoutBlackout);
}
#endif

/*----------------------------------------------------------------------
Function: PreRenderUpdate
Description: Non GL deprecated pre render path
-----------------------------------------------------------------------*/
static void PreRenderUpdate()
{
    logTimerBeginMacro();

    sptUpdate();
    fxWeatherUpdate();
    fxMgrUpdate();

    if(persIsPersonaVehicleType(VEH_TYPE_AIR))
        radclutterEnable(g_bRadialWhenFlying);
    else
        radclutterEnable(g_bRadialClutter);

	if(bClearViewport || bClearZBuffer)
	{
		UINT32 targets;

		targets =  bClearViewport ? piCLEAR_VIEWPORT : 0;
		targets |= bClearZBuffer  ? piCLEAR_ZBUFFER  : 0;

        piClearTargets(targets);
    }

	logTimerCallVoid(trnmainUpdate);
	logTimerCallVoid(mlCheckForCollisionListUpdate);
	logTimerCallVoid(trnUpdateAutoGuns);

#ifdef BETA_RELEASE
    obcUpdate();

    if(!VehicleObjectAnalysis)
    {
#endif

#ifdef GENICONS
        if(!GetIconGenerator().IsEnabled())
            rnUpdateView();
#else
	    logTimerCallVoid(rnUpdateView);
#endif

#ifdef BETA_RELEASE
    }
#endif

	logTimerCallVoid(obmainUpdate);
	logTimerCallVoid(trnClutterUpdate);

#ifdef GENICONS
    if(!GetIconGenerator().IsEnabled())
#endif
    {
        logTimerCallVoid(trnskyUpdate);
    }

	logTimerCallVoid(persUpdateCockpitSmoke);
    logTimerCallVoid(obpoolUpdate);
	logTimerCallVoid(uiUpdate);
    logTimerCallVoid(glslPreRenderUpdate);

    logTimerEndMacro();
}

/*----------------------------------------------------------------------
Function: Render
Description: Non GL deprecated render path
-----------------------------------------------------------------------*/
static void Render()
{
    logTimerCallVoid(gmlpHandleFieldOfView);

    const bool render = uiInGame() && (Online ? g_bVehicleAttached : TRUE);

	if(render)
	{
        oglSetRenderState(oglRENDERSTATE_LIGHTING, FALSE);
        oglLockState(oglRENDERSTATE_LIGHTING, 1);

        glslWaterClearBin();
        glslNormalClearBin();
        glslSpecularClearBin();

#ifdef BETA_RELEASE
	    draw3dDrawDebug();
#endif
        logTimerCallVoid(rnvisSetupFrustum);
        logTimerCallVoid(oglDisableRenderStates);

#ifdef GENICONS
    if(!GetIconGenerator().IsEnabled())
#endif
/*
#ifdef BETA_RELEASE
		draw3dDebugLineUpdate(FALSE);
#endif
*/
        logTimerCallVoid(obdrawObjects);
        logTimerCallVoid(PostDrawCallbacks);
        /*
        logTimerCallVoid(persmsgDraw);
        */

        oglLockState(oglRENDERSTATE_LIGHTING, 0);
        oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
	}

	logTimerCallVoid(uiDraw);

#ifdef BETA_RELEASE
    draw3dDebugLineUpdate(TRUE);

    if(VehicleObjectAnalysis)
    {
	    vehUpdate(); // update all vehicles
	    persUpdate(); // must occur after vehUpdate for view syncing reasons
	    weapUpdate(); // when analyzing spall/shrapnel, must be here to see lines

	    rnUpdateView();
    }
#endif

#ifdef MULTI_CLIENT
	multiClientUpdate();
#endif

	_CalcFPS();
}

/*----------------------------------------------------------------------
Function: PostRenderUpdate
Description: Non GL deprecated render path
-----------------------------------------------------------------------*/
static void PostRenderUpdate()
{
    piVECTOR pos, att;

	vehAdjustAnimationTime();

	// determine if the world origin offset needs to be moved
    persGetCameraPosAndAtt(&pos, &att);
    trnCheckAndAdjustCameraPositionWithOriginOffset(&pos);

	// move the world origin offset if necessary
	trnUpdateTerrainOriginOffset(FALSE);

	piUpdateTextures();

    uiSoundUpdate();

	mlSetNormalReadout();

    if(!SCEN_MGR->IsActive())
    {
        static SINT32 then = 0;

		if(!Online)
			teulSyncClock();

        if(frametime - then > 2000)
        {
            then = frametime;

            wwiiExpireSTOs();
        }
    }

    UpdateFxSystem();
    glslPostRenderUpdate();

#ifdef BETA_RELEASE
#ifdef GENICONS
    if(GetIconGenerator().IsEnabled())
    {
        GetIconGenerator().Capture();
    }
#endif

#if platOS == OS_WIN32
    GenerateScenarioMap::GetInstance().Apply();
#endif

#endif
}

LOGICAL gmlpMainUpdate()
{
//	static UINT32 First = 1;

#ifdef BETA_RELEASE
	static SINT32 timerid = -1;
	logTimerBegin(__FUNCTION__, "", &timerid);
#endif

    GLCounter::instance().startFrame();

	// prep the game loop, including any message processing
	logTimerCallVoid(_UpdateInit);

	// update the world state
	logTimerCallVoid(_UpdateWorld);

	// actual render stage
    if(ogl::UseDeprecatedPath())
    {
	    logTimerCallVoid(_PreRenderUpdateDeprecated);
        logTimerCallVoid(obdrawCull);
	    logTimerCallVoid(_RenderDeprecated);
	    logTimerCallVoid(_PostRenderUpdateDeprecated);
    }
    else
    {
	    logTimerCallVoid(PreRenderUpdate);
        logTimerCallVoid(obdrawCull);
        logTimerCallVoid(Render);
    	logTimerCallVoid(PostRenderUpdate);
    }

	clientSendPendingSTOJoules();		// send any pending STO damage messages

	// close the game loop
	_UpdateKill();

#ifdef BETA_RELEASE
	logTimerEnd(timerid);
{
	static FLOAT32 fps[10];
	static int lasttotpaints;
	int i;
	int totpaints = piGetTotalPaints();

	memmove(&fps[1], &fps[0], 9 * sizeof(FLOAT32));
	fps[0] = gFPS;

	if(logTimerFPS > 0 && fps[4] < logTimerFPS)
	{
		Enable_Log(LOG_TIMER, TRUE);

		WriteLog(LOG_TIMER, LOG_INFO, "***Frame Profile Begin***");
		WriteLog(LOG_TIMER, LOG_INFO, "logTimerFPS: %.2f", logTimerFPS);
		for(i = 9; i >= 1; i--)
		{
			WriteLog(LOG_TIMER, LOG_INFO, "Frame %d", 5 - i);
			logTimerPrint(timerid, i);
		}

		WriteLog(LOG_TIMER, LOG_INFO, "***Frame Profile End***");
		logTimerFPS = 0;
		rnmainListText("Logged frame profile");
		rnmainListText("Frame profiling off");
	}
	lasttotpaints = totpaints;
}
	logTimerReset();
#endif

    // Ticket #2298
    // Removes loading screen once per spawn. Returns if no work necessary.
    uiUIToGameEnd();

    GLCounter::instance().endFrame();

	std::this_thread::yield();

	return TRUE;
}

LOGICAL gmlpMainKill()
{
    WriteDebug("[gmlpMainKill] Begin");

	//cfgPrefsSave();

//	physicsKill(); needs to go after obmainkill
	uiKill();

	persBombsightKill();
	persGunsightKill();

	trnClutterKill();
	radclutterKill();
    speedgrassKill();
	fxMgrKill();
    KillFxSystem();
    PostRenderKill();

#ifdef EVENT_LOG
	piDisplayRenderBuckets();
	piDisplayRenderIndexBuckets();
#endif

#ifdef USE_SHADOWS
    obdshadKill();
#endif // USE_SHADOWS

	locl_Message_Table_Kill();

	mlKill();

	vehKill();
	weapKill();
	persKill();

	ww2imapKill();

	weapListKill();

	parseWorKill();

	StratDB::Kill();

    cfgPrefsKill();

    WriteDebug("[gmlpMainKill] End");

	return 0;
}

//extern void mapProcessSuperCellVislevels();

// initializes stuff for the main update loop
void _UpdateInit()
{
	UpdateGameClocks(); // adjust the internal game clocks

	if (Online == 1)
	{
		PlayerClient()->Update();

		teulUpdateTransport(); // process any new messages

		//mapProcessSuperCellVislevels();
	}

	//if(!imaphackDoKeyboardInputs())
	//	gmlpProcessInputs();

	gmlpProcessInputs();
	imaphackDoKeyboardInputs();
}

#include "client/clientStrtQueue.h"

void _UpdateWorld()
{
#ifdef BETA_RELEASE
#ifdef GENICONS

  if(GetIconGenerator().IsEnabled())
  {
    GetIconGenerator().Apply();
    GetIconGenerator().LoadModel();
  }
#endif
#endif

	//clientStrtQueueUpdate();
	trnTODUpdate(); // updates time of day, always make this the first world update

//	if(uiMgrIsGameInProcess())
	{
		// needs to be before vehUpdate because vehUpdate goes ahead and shoots my gun.  can't have
		// that as it relies on state being correct .. so .. set my state here
		fpFPSUpdate();

		physicsUpdate(frametime);
		gr2AnimUpdate();

#ifdef BETA_RELEASE
	if(!VehicleObjectAnalysis)
	{
#endif
		vehUpdate(); // update all vehicles
		persUpdate(); // must occur after vehUpdate for view syncing reasons
		weapUpdate(); // must occur after persUpdate otherwise bullets might not be visible
#ifdef BETA_RELEASE
	}
#endif
		vehupdateUpdate(0); // sends out updates
		persGunsightUpdate();
		trnCloudUpdate();
	}
}

#if USE_STATESET
void _PreRenderUpdateDeprecated()
{
#ifdef USE_SHADOWS
	obwldPredrawUpdate();
#endif

    sptUpdate();
    fxWeatherUpdate();
    fxMgrUpdate();

    bool render = uiInGame() && (Online ? g_bVehicleAttached : TRUE);

    if(persIsPersonaVehicleType(VEH_TYPE_AIR))
        ShadowMap::GetInstance().Enable(ShadowMapFlags & SHADOW_PLANE_DISABLE);
    else
        ShadowMap::GetInstance().Enable(ShadowMapFlags != 0);

    if(persIsPersonaVehicleType(VEH_TYPE_AIR))
        radclutterEnable(g_bRadialWhenFlying);
    else
        radclutterEnable(g_bRadialClutter);

    if(ShadowMap::GetInstance().Enabled() && render)
    {
        if(gCustomTerrain)
        {
            ShadowMap::GetInstance().RenderShadowCasters();
        }
        else
        {
            ShadowMap::GetInstance().RenderStaticShadowCasters();
            ShadowMap::GetInstance().RenderDynamicShadowCasters();
        }
    }

    static ref_ptr<ClearNode> clear = new ClearNode(GL_DEPTH_BUFFER_BIT, true, Vec4f(0, 0, 0, 1));

    clear->apply();

    trnmainUpdate();
    mlCheckForCollisionListUpdate();
    trnUpdateAutoGuns();

#ifdef BETA_RELEASE
    obcUpdate();
#endif

#ifdef BETA_RELEASE
    if(!VehicleObjectAnalysis)
    {
#endif

	rnUpdateView();

#ifdef BETA_RELEASE
    }
#endif

    obmainUpdate();
    trnClutterUpdate();
    trnskyUpdate();
    obpoolUpdate();
	uiUpdate();

    glslPreRenderUpdate();

    gmlpHandleFieldOfView();

    trnskyPreDrawFlare();
    trnskyReadControlPixels();
}
#else
// update stuff to occur before the actual scene render
void _PreRenderUpdateDeprecated()
{
#ifdef USE_SHADOWS
	logTimerCallVoid(obwldPredrawUpdate);
#endif
    logTimerCallVoid(sptUpdate);
    logTimerCallVoid(fxWeatherUpdate);
    logTimerCallVoid(fxMgrUpdate);

	//logTimerCallVoid(fxmirRender);

    bool render = uiInGame() && (Online ? g_bVehicleAttached : TRUE);

    if(persIsPersonaVehicleType(VEH_TYPE_AIR))
        ShadowMap::GetInstance().Enable(ShadowMapFlags & SHADOW_PLANE_DISABLE);
    else
        ShadowMap::GetInstance().Enable(ShadowMapFlags != 0);

    if(persIsPersonaVehicleType(VEH_TYPE_AIR))
        radclutterEnable(g_bRadialWhenFlying);
    else
        radclutterEnable(g_bRadialClutter);

    if(ShadowMap::GetInstance().Enabled() && render)
    {
        if(gCustomTerrain)
        {
            ShadowMap::GetInstance().RenderShadowCasters();
        }
        else
        {
            ShadowMap::GetInstance().RenderStaticShadowCasters();
            ShadowMap::GetInstance().RenderDynamicShadowCasters();
        }
    }

	if (bClearViewport || bClearZBuffer)
	{
		UINT32 targets;

		targets =  bClearViewport ? piCLEAR_VIEWPORT : 0;
		targets |= bClearZBuffer  ? piCLEAR_ZBUFFER  : 0;

        piClearTargets(targets);
    }

	logTimerCallVoid(trnmainUpdate);
	logTimerCallVoid(mlCheckForCollisionListUpdate);
	logTimerCallVoid(trnUpdateAutoGuns);

#ifdef BETA_RELEASE
    obcUpdate();
    if(!VehicleObjectAnalysis)
    {
#endif

#ifdef GENICONS
  if(!GetIconGenerator().IsEnabled())
    rnUpdateView();
#endif

#ifdef BETA_RELEASE
    }
#endif

    logTimerCallVoid(rnUpdateView);

	logTimerCallVoid(obmainUpdate);
	logTimerCallVoid(trnClutterUpdate);
#ifdef GENICONS
  if(!GetIconGenerator().IsEnabled())
#endif
  {
  logTimerCallVoid(trnskyUpdate);
  }
	logTimerCallVoid(persUpdateCockpitSmoke);
//	logTimerCallVoid(FXUpdate);
    logTimerCallVoid(obpoolUpdate);
	logTimerCallVoid(uiUpdate);
    UpdateFxSystem();

    glslPreRenderUpdate();
}
#endif

// renders the image
#include "rn/rnvis.h"
#if USE_STATESET
void _RenderDeprecated()
{
    const bool render = uiInGame() && (Online ? g_bVehicleAttached : TRUE);

	if(render)
	{
        // Once per frame frustum setup for culling
        rnvisSetupFrustum();
        obdrawObjectsDeprecated();
        PostDrawCallbacks();
    }

    uiDraw();

	_CalcFPS();
}
#else
void _RenderDeprecated()
{
	// since we have a variable fogging outer distance
	// we want to adjust the field of view for every frame
	// for z-buffer reasons
    gmlpHandleFieldOfView();

    const bool bRender = uiInGame() && (Online ? g_bVehicleAttached : TRUE);

	if(bRender)
	{
        glslWaterClearBin();
        glslNormalClearBin();
        glslSpecularClearBin();

#if DEBUG_SHADOW_MAP
        if(ShadowMap::GetInstance().Enabled())
            ShadowMap::GetInstance().Debug();
#endif

#ifdef BETA_RELEASE
	    draw3dDrawDebug();
#endif
        // Once per frame frustum setup for culling
        rnvisSetupFrustum();

        piDisableRenderStates();

#ifdef GENICONS
    if(! GetIconGenerator().IsEnabled())
#endif
        trnskyDrawDeprecated();

#ifdef USE_LENS_FLARE
        trnskyPreDrawFlare();
        trnskyReadControlPixels();
#endif

#ifdef BETA_RELEASE
		draw3dDebugLineUpdate(FALSE);
#endif

        trnmainDraw();
        obdrawObjectsDeprecated();

        PostDrawCallbacks();

        persmsgDraw();

#ifdef SPATIAL_DB
        SDB_DrawDebug();
#endif
    }

	oblfUpdate();

	uiDraw();

#ifdef BETA_RELEASE
    draw3dDebugLineUpdate(TRUE);

    if(VehicleObjectAnalysis)
    {
	    vehUpdate(); // update all vehicles
	    persUpdate(); // must occur after vehUpdate for view syncing reasons
	    weapUpdate(); // when analyzing spall/shrapnel, must be here to see lines

	    rnUpdateView();
    }
#endif


#ifdef MULTI_CLIENT
	multiClientUpdate();
#endif

	_CalcFPS();
}
#endif

// update stuff to occure after the actual scene render
void _PostRenderUpdateDeprecated()
{
	piVECTOR	Pos;
	piVECTOR	Att;

	// reset animation time
	vehAdjustAnimationTime();

	// determine if the world origin offset needs to be moved
	persGetCameraPosAndAtt(&Pos, &Att);
	trnCheckAndAdjustCameraPositionWithOriginOffset(&Pos);

	// move the world origin offset if necessary
	trnUpdateTerrainOriginOffset(FALSE);

    uiSoundUpdate();

	mlSetNormalReadout();

    if(!SCEN_MGR->IsActive())
    {
        static SINT32 then = 0;

		if(!Online)
			teulSyncClock();

        if(frametime - then > 2000)
        {
            then = frametime;

            wwiiExpireSTOs();
        }
    }

    glslPostRenderUpdate();
    ShadowMap::GetInstance().PostRenderUpdate();

#ifdef BETA_RELEASE
#ifdef GENICONS

  if(GetIconGenerator().IsEnabled())
  {
    GetIconGenerator().Capture();
  }
#endif
#if platOS == OS_WIN32
    GenerateScenarioMap::GetInstance().Apply();
#endif
#endif
}

// any kill functions necessary at end of cycle
void _UpdateKill()
{
}

//=============================================

// updates the millisecond game clock
void UpdateGameClocks()
{
	static UINT64 starttime = GetMillisecondTick() - 1;
	const UINT64 newFrametime = GetMillisecondTick();

	lastframetime = frametime;
	frametime = (SINT32)(newFrametime - starttime);

	UINT32 ticks = ft1000mscount;
	ft1000mscount = frametime / 1000;
	ft1000msticks = ft1000mscount - ticks;
	ticks = ft500mscount;
	ft500mscount = frametime / 500;
	ft500msticks = ft500mscount - ticks;
	ticks = ft200mscount;
	ft200mscount = frametime / 200;
	ft200msticks = ft200mscount - ticks;
	ticks = ft100mscount;
	ft100mscount = frametime / 100;
	ft100msticks = ft100mscount - ticks;
	ticks = ft50mscount;
	ft50mscount = frametime / 50;
	ft50msticks = ft50mscount - ticks;
	ft10mscount = frametime / 10;
}


UINT32 gmlpHandleFieldOfView()
{
FLOAT32 ZBufferStart;
//FLOAT32 ZBufferEnd;

	// z buffer start depends completely on the ratio of the FOV to
	// 90 degrees
	// this is done in order to increase z-buffer resolution when it really matters
	// at the expense of near-z buffer position

	if (bZEnable == piZB_USEW)
	{
		ZBufferStart = 0.05f;
	}
	else
	{
		rnmainGetZBufferNearVal(&ZBufferStart);
// PMM - removed this.  we really need to audit our whole FOV system
//		if (FieldOfView < 45.0f)
//		{
//			ZBufferStart *= 45.0f / FieldOfView;
//		}
	}


	// z buffer end is the fogend range, adjusted for the corners of the frustrum
//	ZBufferEnd = (FLOAT32)(FogEnd / cos(mlDEG_TO_RAD(FieldOfView / 2.0)));
//	ZBufferEnd = piLongLikeFloat(FogEnd) * 2.0f;
	//trnmainGetHorizon(&ZBufferEnd);
//	ZBufferEnd = piLongLikeFloat(FogEnd) * 2.0f;

//	if (persGetFogScaleMultiplier() > 0.0f)
	{
//		piSetFieldOfView(FieldOfView, ZBufferStart, ZBufferEnd * 2.0f / persGetFogScaleMultiplier());
		piSetFieldOfView(FieldOfView, ZBufferStart, piLongLikeFloat(FogEnd));
	}

    return 0;
}

static LOGICAL _PostExternalDrawFunc(LOGICAL beforesorted)
{
	if (!beforesorted)
	{
       // FXDisplayParticles(0);
	}
	else
	{
		vehIconDraw();
		weapDrawTracers();
	}

	return TRUE;
}

// Called after all unsorted objects are drawn
static LOGICAL _PostUnsortedDrawFunc(LOGICAL notused)
{
    return TRUE;
}

static LOGICAL _PostInternalDrawFunc(LOGICAL IsBeforeStage)
{

	if (!IsBeforeStage)
	{
        oglDisableTexturesAboveAndIncluding(0);
        persDrawGunsight();
	}

	return TRUE;
}

imap_queue_entry* imapqueue;
UINT32 imapqueuesize;

void gmlpProcessInputs()
{
	CHAR_INPUT* input;
	SINT32 count;

	static SINT32 inputtime = frametime;
	static SINT32 afkcheck	= frametime;

#if platOS == OS_MACOS
    checkIfInFront();
#endif

	dimainGetInputs(&imapqueue, &imapqueuesize);
    dimainGetTextInputs(&input, &count);

	// record the last time input was seen
	if(imapqueuesize)
		inputtime = frametime;

	// auto-afk code, checks every AUTO_AFK_CHECK milliseconds
	if(frametime > afkcheck)
	{
		SINT32 isafk = uiIsAFK();

		// "/afk" is host controlled and is a toggle
		if(!isafk && frametime > inputtime + AUTO_AFK_PERIOD) // not currently afk, lets autoafk them
		{
			uiSetAutoAFK(true); // let the ui system know this was an autoafk
			PlayerClient()->SendChatMessage(0, "/afk");
		}
		else if(isafk && uiIsAutoAFK() && inputtime > isafk + AUTO_AFK_BUFFER) // only auto-unafk if we originally autoafk'ed them
		{
			PlayerClient()->SendChatMessage(0, "/afk");
		}

		afkcheck = frametime + AUTO_AFK_CHECK;
	}

	uiInput(imapqueue, imapqueuesize, input, count);

	if(uiFocused() || disableButtonInput)
    {
        imapButtonInputState(0);
    }

	imapAxisInputState(disableAxisInput ? 0 : 1);

	if(uiCursorState() != 0)
	{
		// remove mouse axis entries
		imap_queue_entry oldQueue[NUM_IAC_QUEUE_ENTRIES];
		UINT32 oldNumEntries = imapqueuesize;
		memcpy(oldQueue, imapqueue, imapqueuesize* sizeof(imap_queue_entry));
		imapqueuesize = 0;

		for(UINT32 i = 0; i < oldNumEntries; i++)
		{
			if(!IAC_IS_MOUSE(oldQueue[i].iac))
			{
				imapqueue[imapqueuesize++] = oldQueue[i];
			}
		}
	}
	imapProcessInputs(imapqueue, imapqueuesize);

    if(uiInGame() && !uiFocused() && !disableButtonInput)
    {
        imapButtonInputState(1);
    }
}

#if platOS == OS_WIN32
#include "timer.h"
#endif

static void _CalcFPS()
{
#if platOS == OS_WIN32

    static timer fpsTimer(2);

    fpsTimer.frame();

    if(ft1000msticks)
    {
        gFPS = fpsTimer.get_fps();
    }

#ifdef BETA_RELEASE
	gLastFrameFPS = fpsTimer.get_fps();
#endif

    piResetRenderStats();

    return;
#endif

	static long lasttotpaints;
	long    totpaints;
	UINT32	points, lines, triangles, vertices;
	SINT32	StatTris;

	piGetRenderStats(&points, &lines, &triangles, &vertices);

	StatTris = 0;
	totpaints = piGetTotalPaints();

	if(ft1000msticks)
		gFPS = (FLOAT32)(totpaints - lasttotpaints) * 1000.0f / (FLOAT32)(frametime - lastframetime);

#ifdef BETA_RELEASE
	gLastFrameFPS = (FLOAT32)(totpaints - lasttotpaints) * 1000.0f / (FLOAT32)(frametime - lastframetime);
#endif

	//lastframetime = frametime;
	lasttotpaints = totpaints;

	//piResetRenderStats();
}

#include <syscfg.h> // Path is project specific.
/*-----------------------------------------------------------------------------
Module: fxWeather
Description:
-----------------------------------------------------------------------------*/
#include "fx/fxWeather.h"
#include "fx/TransformEffect.h"
#include "trn/trnvars.h"
#include "ld/ldtexture.h"
#include "math/math3d.h"
#include "trn/trnsky.h"
#include "fx/fxSnd.h"
#include "physics/colliderMain.h"
#include "physics/physicsmain.h"
#include "trn/trntod.h"
#include "ui/uiChat.h"
#include "ob/obMgr.h"
#include "spt/Random.h"
#include "pers/persmain.h"
#include "math/mathUnitConversion.h"
#include "ui/uiMain.h"
#include "math/mathnum.h"
#include "scen/ScenarioManager.h"

#define RainRow         7
#define RainColumn      7
#define RainGridSize    49
#define LowPrime        11

#define USE_ODE 0

#ifdef DEBUG_WW2
#define TEST_OFFLINE 0
#endif

#if USE_ODE
#define USE_SINGLE_COLLISION    0
#define USE_RAY 0
static colliderQuery* RainCollider = nullptr;
#endif

static bool dorain = false;
static bool rainparticleemitted = false;

static int gridids[RainGridSize];
static int thunderupdate = -1, nextthunderupdate = 0;
static int rainsoundid = 0;
static int NextRainTime = -1;

static unsigned int weatherstate = WXSTATE_CLEAR_DAY;
static unsigned int weatherseed = 0;
static unsigned int lasttod = 0;
static unsigned int oldweatherstate;
static unsigned int StartRainTime = 0;
static unsigned int EndRainTime = 0;

static std::vector<unsigned int> raintimes;
static std::vector<unsigned int> rainlength;
static std::vector<unsigned int> thundertimes;

static float speed = 0.0f;

#if(0)
static std::set<OBJECT*> _bounds;
#endif

static const unsigned int NumRainPatterns = 16;
static unsigned int seeds[NumRainPatterns] =
{
    1259, 1489, 1823, 1811,
    2083, 3163, 4969, 5261,
    5743, 6151, 6569, 6793,
    7027, 7451, 7681, 7879
};

static inline int WeatherRandom(int nMin, int nMax)
{
    return PNRandom::random(nMin, nMax);
}

static inline float WeatherRandom(float fMin, float fMax)
{
    return PNRandom::random(fMin, fMax);
}

static void WeatherRandomSeed(unsigned int seed)
{
    PNRandom::seedrandom(seed);
}

/*----------------------------------------------------------------------------
Function: LogRainTimes
Description:
-----------------------------------------------------------------------------*/
#if defined(BETA_RELEASE)
static void LogRainTimes()
{
    for(unsigned int i = 0; i < raintimes.size(); i++)
    {
        unsigned int time = raintimes[i];

        int hr = time / 3600;

        int t = time % 3600;

        int mn = t / 60;
        int sc = t % 60;

        char hour[16] = {0};
        char minute[16] = {0};
        char second[16] = {0};

        strprintf(hour, hr < 10 ? "0%d:" : "%d:", hr);
        strprintf(minute, mn < 10 ? "0%d:" : "%d:", mn);
        strprintf(second, sc < 10 ? "0%d" : "%d", sc);

        std::string str = "Rain Time: ";
        str += hour;
        str += minute;
        str += second;
        char buff[80] = {0};

        t = rainlength[i];

        mn = t / 60;
        sc = t - mn * 60;

        strprintf(buff, " lasting for %d minutes and %d seconds.", mn, sc);
        str += buff;

        Write_Debug(str.c_str());
    }
}
#endif

/*----------------------------------------------------------------------------
Function: SetTodaysRainTimes
Description:
-----------------------------------------------------------------------------*/
static void SetTodaysRainTimes(const int seedidx)
{
	static signed int lastidx = -1;

	if(lastidx == seedidx && !raintimes.empty())
	{
		// Same patern, don't mess with it.
		return;
	}

	lastidx = seedidx;
	thunderupdate = -1;
	nextthunderupdate = 0;

    raintimes.clear();
    rainlength.clear();

	if(seedidx >= NumRainPatterns)
	{
		// If the pattern is greater than the number of
		// rain patterns, then there's no rain, just
		// clouds.
		return;
	}

    Write_Debug("Rain for index %d", seedidx);

	// OGS: Since 'seedidx' is coming down from the host,
	// we need to re-seed the RNG with something after
	// we are done using 'seedidx' as the seed.
//	unsigned int reseed = rand(); // mpt used with Weather

    unsigned int seed = seeds[seedidx];

    WeatherRandomSeed(seed);

    // RAND_MAX = 32767
    // 86399 = 23:59 hours
	static const int daySeconds = 24 * 60 * 60;
    float time = 0.0f;
	float duration = 0.0f;
    int num = WeatherRandom(4, 16);

    for(int i = 0; i < num; i++)
    {
        time = WeatherRandom(0.0f, 24.0f);

		// Between 6pm and 6am, time runs fast.
		bool accelerated = (time < 6.0f || time > 18.0f);

        time *= 3600.0f;

        raintimes.push_back((int)time);

		if(accelerated == false)
		{
			// 5 to 30 minute rain periods while the
			// clock is running slower
			duration = WeatherRandom(5.0f, 30.0f);
		}
		else
		{
			// OGS: Have to make it rain longer when the
			// clock is running faster, otherwise
			// the showers look a bit silly.
			duration = WeatherRandom(20.0f, 55.0f);
		}

        duration *= 60.0f; // to seconds

        rainlength.push_back((int)duration);
    }

    sort(raintimes.begin(), raintimes.end());

	// Should we have thunder? (Lets not thunder every rainy day)
	if(WeatherRandom(0, 16) > 4 )
	{
		// Generate thunder offsets.
		static const unsigned int windowerPrime = 3797;	// Any prime between 3600 and 7200 will do.
		static const unsigned int divisor = 5;			// Divide by X to prevent overlapping thunders.

		// Start by plugging some thunder into the rain windows.
		for(unsigned int i = 0; i < raintimes.size(); ++i)
		{
			rainlength[i] += WeatherRandom(3, 57);

			// 2/3 showers will generate no thunder.
			if(WeatherRandom(0, 3) != 0)
				continue;

			// Generate some thunder before or after the shower.
			unsigned int offset = WeatherRandom(-30, rainlength[i] + 30);
			int start = (int)raintimes[i] + offset;

			if(start > 0 || start < (int)daySeconds)
				thundertimes.push_back((unsigned int)start / divisor);

			unsigned int hour = raintimes[i] / 3600;
			unsigned int thunders = (hour >= 6 && hour <= 18) ? 16 : 3;
			for(unsigned int t = 1; t < thunders; ++t)
			{
				if ( rainlength[i] < 120 )
					continue;
				// Chance of an additional rumbles the rain; increasingly less likely.
				if(WeatherRandom(0, t) == 0)
				{
					start = raintimes[i] + WeatherRandom(57, rainlength[i] - 57);
					thundertimes.push_back((unsigned int)start / divisor);
				}
			}
		}

		// Now generate rain-independent thunder.
		for(unsigned int hour = 0; hour < 24; hour++)
		{
			// Because of accelerated time, we want less
			// chance of thunder during the fast times.
			unsigned int maxThundersThisHour = (hour >= 6 && hour <= 18) ? 3 : 1;

			unsigned int thundersThisHour = WeatherRandom(0, maxThundersThisHour);
			if(thundersThisHour == 0)
				continue;

			// Start of the hour in seconds.
			unsigned int hourTime = hour * 3600;
			unsigned int incrementor = WeatherRandom(61, 3541);	// Primes at the edge of +60s and -60s on the hour.
			for(unsigned int thunder = 0; thunder < thundersThisHour; ++thunder)
			{
				unsigned int thundertime = hourTime + (((incrementor + windowerPrime) * thunder) % 3600);
				while(std::find(thundertimes.begin(), thundertimes.end(), thundertime / divisor) != thundertimes.end())
				{
					static const signed int ArbitraryPrime1 = 37;
					static const signed int ArbitraryPrime2 = 89;
					thundertime = hourTime + (thundertime + (unsigned int)WeatherRandom(ArbitraryPrime1, ArbitraryPrime2)) % 3600;
				}
				thundertimes.push_back(thundertime / divisor);
			}
		}

		for(unsigned int i = 0; i < thundertimes.size(); ++i)
		{
			thundertimes[i] *= divisor + WeatherRandom(0, divisor);
		}

		sort(thundertimes.begin(), thundertimes.end());
	}

#ifdef BETA_RELEASE
    LogRainTimes();
#endif

//	// OGS: Resume randomness.
//	srand(reseed);
}

/*----------------------------------------------------------------------------
Function: fxWeatherIsCloudy
Description:
-----------------------------------------------------------------------------*/
bool fxWeatherIsCloudy()
{
    return weatherstate & (WXSTATE_CLOUDS | WXSTATE_RAIN);
}

/*----------------------------------------------------------------------------
Function: fxWeatherIsRaining
Description:
-----------------------------------------------------------------------------*/
bool fxWeatherIsRaining()
{
    return weatherstate & WXSTATE_RAIN;
}

/*----------------------------------------------------------------------------
Function: GetGridSlot
Description: Provides unique id 0 <= id < RainGridSize (wraps)
-----------------------------------------------------------------------------*/
static int GetGridSlot()
{
    static int idx = 0;

    idx++;
    idx = idx >= RainGridSize ? 0 : idx;

    return gridids[idx];
}

/*----------------------------------------------------------------------------
Function: fxWeatherInit
Description:
-----------------------------------------------------------------------------*/
void fxWeatherInit()
{
    if(!Online)
        SetTodaysRainTimes(WeatherRandom(0, 15));

#if DEBUG_WW2
#if(0)
    for(int i = 0; i < max_seeds; i++)
        SetTodaysRainTimes(i);
#endif
#endif

    oldweatherstate = weatherstate;

    int id = 0;

    // Initialize 'random' grid access
    for(int i = 0; i < RainGridSize; i++)
    {
        id = (id + LowPrime) % RainGridSize;
        gridids[i] = id;
    }

#if(0)
    glslProgram* pgm = GLSL.loadProgram("raindrop", nullptr, nullptr);

    if(pgm)
    {
        ref_ptr<glslShader> s1 = GLSL.loadVertexShader("fx/shaders/GLSL/particle/rain.vert");
        ref_ptr<glslShader> s2 = GLSL.loadFragmentShader("fx/shaders/GLSL/particle/rain.frag");

        pgm->AddShader(s1.get());
        pgm->AddShader(s2.get());

        pgm->GetOrCreateUniform("src1", glslUniform::SAMPLER_2D);
    }

    normal = ldTextureLoad("terrain/to/water/water1_normal.dds", LDPIX_FLIP);
#endif
}

#ifdef BETA_RELEASE
/*-----------------------------------------------------------------------------
Function: PrintTimeLine
Description:
-----------------------------------------------------------------------------*/
static void PrintTimeLine()
{
	char time[32];
	std::string output = "";

	unsigned int tod;
	trnTODGetTimeOfDay(&tod);
	snprintf(time, 31, "[%02u:%02u] ", tod / 3600, (tod / 60) % 60);
	output += time;
	if(raintimes.size() == 0)
	{
		output += "No rain today.";
	}
	else
	{
		output += "Rain Times: ";

		for ( unsigned int i = 0; i < raintimes.size(); ++i )
		{
			if(i > 0)
				output += ", ";

			if(i == NextRainTime)
			{
				output += "->";
			}
			snprintf(time, 31, "[%02u:%02u]", raintimes[i] / 3600, (raintimes[i] / 60) % 60);
			output += time;
			if(i == NextRainTime)
			{
				snprintf(time, 31, " for %um & %0us<-", rainlength[i] / 60, rainlength[i] % 60);
				output += time;
			}
		}
	}

	uiChatPrintLine(output.c_str());
}
#endif

/*-----------------------------------------------------------------------------
Function: fxWeatherSetState
Description:
-----------------------------------------------------------------------------*/
void fxWeatherSetState(int weather, int seedindex)
{
	unsigned int tod;
	trnTODGetTimeOfDay(&tod);

	// Translate the weather from host variants to client variants.
    switch(weather)
    {
    case 1:
        {
            weather = WXSTATE_CLOUDS;
			seedindex = 0;
            break;
        }
    case 2:
        {
            weather = WXSTATE_RAIN;
			break;
        }
    default:
        {
            weather = WXSTATE_CLEAR_DAY;
			seedindex = 0;

            break;
        }
    }

	if(!Online || weatherstate != weather || weatherseed != seedindex || tod < lasttod || SCEN_MGR->IsActive())
	{
#ifdef BETA_RELEASE
		uiChatPrintLine("[%02u:%02u] Setting weather state sky=%u, wthr=%u (was %u:%u)"
						, tod/3600, (tod/60)%60
						, weather, seedindex
						, weatherstate, weatherseed
						);
#endif

	    raintimes.clear();
	    rainlength.clear();
		thunderupdate = -1;
		nextthunderupdate = 0;
	}

	weatherstate = weather;
	weatherseed = seedindex;
	lasttod = tod;

	if(weather == WXSTATE_RAIN)
	{
		SetTodaysRainTimes(seedindex);
#if BETA_RELEASE
		PrintTimeLine();
#endif
	}

#if TEST_OFFLINE
#else
    if(!Online || gCustomTerrain)
#endif
	{
        dorain = (weather == WXSTATE_RAIN);
	}
}

/*-----------------------------------------------------------------------------
Function: fxWeatherDrawWindow
Description:
-----------------------------------------------------------------------------*/
void fxWeatherDrawWindow(OBJECT* o)
{
#if(0)
    oglSetWorldMatrix(&o->mat);
    oglSetActiveMaterial(o->matid);

    static glslProgram* rain = nullptr;

    if(!rain)
        rain = GLSL.getProgram("raindrop");

    if(!rain)
        return;

    rain->Apply();
    oglSetActiveTexture(0, normal);
    rain->ApplyUniformMap();

    obdrawDrawMesh(o);

    rain->UnApply();
#endif
}

/*-----------------------------------------------------------------------------
Function: _killcallback
Description:
-----------------------------------------------------------------------------*/
static void _killcallback(Particle2* p)
{
    // Expired rain did not pass 'endpos', so we are probably elevated too high
    // to see any rain splashes.
    if(p->expired())
        return;

    Vec3f n(0, 0, -1);
    TransformEffect* te = TransformEffect::AddEffect("raingroundhit", n, 0, nullptr);

    if(te)
    {
        te->enableemitters(true);;
        te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
        te->setmatrix(Matrixf::translate(p->_endpos));
    }
}

#if USE_ODE
/*-----------------------------------------------------------------------------
Function: _updatecallback
Description: Checks if particle is in building bound box
-----------------------------------------------------------------------------*/
static bool _updatecallback(Particle2* p)
{
    if(p->expired())
        return false;

#ifdef BETA_RELEASE
    static int timerid = -1;
    logTimerBegin(__FUNCTION__, "", &timerid);
#endif

    Vec3d pos = physicsGameToPhysical(p->_position);
    pos.z() -= 1000.0f;

    colliderQuery::dGeomList& geoms = RainCollider->geoms();
#if USE_RAY
    dGeomRaySet(geoms[0], pos.x() - p->_currentsize, pos.y() - p->_currentsize, pos.z(), 0, 0, 1);
    dGeomRaySet(geoms[1], pos.x() - p->_currentsize, pos.y() + p->_currentsize, pos.z(), 0, 0, 1);
    dGeomRaySet(geoms[2], pos.x() + p->_currentsize, pos.y() + p->_currentsize, pos.z(), 0, 0, 1);
    dGeomRaySet(geoms[3], pos.x() + p->_currentsize, pos.y() - p->_currentsize, pos.z(), 0, 0, 1);
#else
    dGeomSetPosition(geoms[0], pos.x(), pos.y(), pos.z());
#endif

    dGeomID current = nullptr;
    RainCollider->setDirty(true);

    if(!RainCollider->findCollisions(true))
    {
        p->_hasendpos = false;
#ifdef BETA_RELEASE
        logTimerEnd(timerid);
#endif
        return true;
    }

    colliderQuery::CollisionList& collisions = RainCollider->getCollisions();
    piVECTOR opos;

    for(int j = (int)(collisions.size() - 1); j >= 0; j--)
    {
        struct colliderQuery::Collision& c = collisions[j];

        if(current == c.g1)
            continue;

        current = c.g1;

        OBJECT* o = (OBJECT*)dGeomGetData(c.g2);

        if(!o)
            return false;

        if(piIsMaterialInterior(o->matid))
        {
#ifdef BETA_RELEASE
            logTimerEnd(timerid);
#endif
            return false;
        }

        if(o->flag3 & OBJECT_IS_INTERIOR)
        {
#ifdef BETA_RELEASE
            logTimerEnd(timerid);
#endif
            return false;
        }

#if USE_RAY
        Vec3d start, end;
        osgGeomRayGet(c.g1, start, end);
        float z = -(float)(start.z() + c.depth);
#else
        float z = -float(c.pos.z());
#endif
        math3dVectorMultiply(&opos, &o->pos, &o->mat);

        p->_endpos.z() = z;

        // Is collidable above me?
        if(z < -playpos.z)
        {
#ifdef BETA_RELEASE
            logTimerEnd(timerid);
#endif
            return false;
        }
    }

#ifdef BETA_RELEASE
    logTimerEnd(timerid);
#endif

    return true;
}
#else
/*-----------------------------------------------------------------------------
Function: _updatecallback
Description: Checks if particle is in building bound box
-----------------------------------------------------------------------------*/
static bool _updatecallback(Particle2* p)
{
    p->_currentalpha = 0.5f;

    static piVECTOR up = {0, 0, -1};

    float alpha1 = 1.0f - math3dDotProduct(&playdir, &up);
    alpha1 = alpha1 > 0.5f ? 0.5f : alpha1 < 0.001f ? 0.0f : alpha1;
    p->_currentalpha = alpha1;

    /*
    float alpha2 = 1.0f - (speed / 10.0f);
    alpha2 = alpha2 < 0.0f ? 0.0f : alpha2 > 0.5f ? 0.5f : alpha2;

    p->_currentalpha = std::min(alpha1, alpha2);
    */

    return true;
}
#endif

#if USE_ODE
#if USE_SINGLE_COLLISION
/*-----------------------------------------------------------------------------
Function: _createrain
Description:
-----------------------------------------------------------------------------*/
static void _createrain(int numemissions)
{
    float height = trnskyGetAltitude();

    rainparticleemitted = false;

    // Turn off rain above this height
    if(height >= 200.0f)
        return;

    rainparticleemitted = true;

    ParticleMgr* pm = TransformEffect::GetParticleMgr();

    unsigned int flags = 0;

    int delayoffset = 0;

    mlVehicleData* data = persGetPersonaModelDataPtr();

    if(!data)
        return;

    Vec3d player(-playpos.x, -playpos.y, -playpos.z);
    player.x() += WorldOriginOffset.x;
    player.y() += WorldOriginOffset.y;

    Particle2* p;

    static char* part = "rain";

    static int numparts = 4;

    // Place in physics world
    Vec3d pos = player;

    for(int q = 0; q < 1; q++)
    {
        for(int i = 0; i < 1; i++)
        {
            int slot = GetGridSlot();

            int r = slot / RainRow;
            int c = slot % RainColumn;

            double x = pos.x() + double(c - 3) * 3.0;
            double y = pos.y() + double(r - 3) * 3.0;
            double z = pos.z() - 10.0f;

            p = pm->createparticle(part);

            if(!p)
                continue;

            p->_alpha[0] = 0.5f;
            p->_alpha[1] = 0.5f;

            p->_size[0] = 1.5f;
            p->_size[1] = 1.5f;

            p->_time[0] = frametime + delayoffset;
            p->_time[3] = p->_time[0] + 3000;

            p->_currentsize = p->_size[0];
            p->_currentalpha = p->_alpha[0];

            p->_position = Vec3d(x, y, z);
            p->_lastposition = p->_position;
            p->_originalposition = p->_position;

            p->_hasendpos = true;
            p->_endpos = p->_position;

            p->_gravcoef = 9.8f;
            p->_stretchlength = 2.0f;

            p->_flags |= part_has_gravity;
            p->_flags |= part_has_fixstretch;
            p->_flags |= part_always_stretches;
            p->_flags |= part_follows_player_xy;

            p->_velocity = Vec3f(0, 0, 0.1f);//Vec3f(WeatherRandom(-1.0f, 1.0f), WeatherRandom(-1.0f, 1.0f), 16.0f);

            p->_killcallback = _killcallback;
            p->_updatecallback = _updatecallback;

            delayoffset += 15;
        }
    }
}
#else // USE_SINGLE_COLLISION
/*-----------------------------------------------------------------------------
Function: _createrain
Description:
-----------------------------------------------------------------------------*/
static void _createrain(int numemissions)
{
    float height = trnskyGetAltitude();

    rainparticleemitted = false;

    // Turn off rain above this height
    if(height >= 200.0f)
        return;

    rainparticleemitted = true;

    ParticleMgr* pm = TransformEffect::GetParticleMgr();

    unsigned int flags = 0;

    int delayoffset = 0;

    mlVehicleData* data = persGetPersonaModelDataPtr();

    if(!data)
        return;

    Vec3d player(-playpos.x, -playpos.y, -playpos.z);
    player.x() += WorldOriginOffset.x;
    player.y() += WorldOriginOffset.y;

    Particle2* p;

    static char* part = "rain";

    static int numparts = 4;

    float minheight = -1;

    colliderQuery::dGeomList& geoms = RainCollider->geoms();
    int q = 0;

    for(q = 0; q < RainGridSize; q++)
    {
        dGeomDisable(geoms[q]);
    }

    // Place in physics world
    Vec3d pos = physicsGameToPhysical(player);
    pos.z() -= 1000.0f;

    for(q = 0; q < numemissions; q++)
    {
        for(int i = 0; i < numparts; i++)
        {
            int slot = GetGridSlot();

            dGeomEnable(geoms[slot]);

            int r = slot / RainRow;
            int c = slot % RainColumn;

            double x = pos.x() + double(c - 3) * 3.0;
            double y = pos.y() + double(r - 3) * 3.0;
            double z = pos.z();

            dGeomRaySet(geoms[slot], x, y, z, 0, 0, 1);
        }
    }

    dGeomID current = nullptr;
    RainCollider->setDirty(true);

    if(!RainCollider->findCollisions(true))
    {
        return;
    }

    colliderQuery::CollisionList& collisions = RainCollider->getCollisions();

    for(int j = (int)(collisions.size() - 1); j >= 0; j--)
    {
        struct colliderQuery::Collision& c = collisions[j];

        if(current == c.g1)
            continue;

        current = c.g1;

        OBJECT* o = (OBJECT*)dGeomGetData(c.g2);

        if(!o)
            continue;

        if(piIsMaterialInterior(o->matid))
            continue;

        if(o->flag3 & OBJECT_IS_INTERIOR)
            continue;

        Vec3d start, end;
        osgGeomRayGet(c.g1, start, end);
        float z = -(float)(start.z() + c.depth);

        // Is collidable above me?
        // Less is more, with our stupid coord system
        if(z < -playpos.z)
        {
            continue;
        }

        minheight = std::min(z, minheight);

        pos = physicsPhysicalToGame(start);

        pos.z() = player.z() - 10.0f;

        p = pm->createparticle(part);

        if(!p)
            continue;

        p->_alpha[0] = 0.5f;
        p->_alpha[1] = 0.5f;

        p->_size[0] = 1.5f;
        p->_size[1] = 1.5f;

        p->_time[0] = frametime + delayoffset;
        p->_time[3] = p->_time[0] + 3000;

        p->_currentsize = p->_size[0];
        p->_currentalpha = p->_alpha[0];

        p->_position = pos;
        p->_lastposition = pos;
        p->_originalposition = pos;

        p->_gravcoef = 9.8f;
        p->_stretchlength = 2.0f;

        p->_flags |= part_has_gravity;
        p->_flags |= part_has_fixstretch;
        p->_flags |= part_always_stretches;
        p->_flags |= part_follows_player_xy;

        p->_velocity = Vec3f(WeatherRandom(-1.0f, 1.0f), WeatherRandom(-1.0f, 1.0f), 16.0f);
        p->_killcallback = _killcallback;

        p->_hasendpos = true;
        p->_endpos = pos;
        p->_endpos.z() = z;

        delayoffset += 15;
    }

    // Adjust sound volume based on height
    if(rainsoundid > 0)
    {
        // 10 meters high is minimum for max sound volume
        minheight = -playpos.z - minheight - 10.0f;

        minheight = minheight > 0 ? 0 : minheight < -40.0f ? -40.0f : minheight;
        float t = minheight / -40.0f;
        t = 1.0f - t;

        float vol = t * 0.005f;

        if(vol < 0.00001f)
            vol = 0.0f;

        fxSndSetVolume(rainsoundid, vol);
    }
}
#endif // USE_SINGLE_COLLISION
#else // USE_ODE
static void _addparticle(piVECTOR& pos, OBJECT* o)
{
    // Is collidable above me?
    // Less is more, with our stupid coord system
    if(pos.z < -playpos.z)
    {
#if(0)
        if(o)
        {
            if(_bounds.find(o) == _bounds.end())
                _bounds.insert(o);
        }
#endif
        return;
    }

    static const char* part = "rain";
    int delayoffset = 0;

    ParticleMgr* pm = TransformEffect::GetParticleMgr();
    Particle2* p = pm->createparticle(part);

    if(!p)
        return;

    p->setname("rain");

    p->_size[0] = 1.5f;
    p->_size[1] = 1.5f;

    p->_time[0] = frametime + delayoffset;
    p->_time[3] = p->_time[0] + 3000;

    p->_currentsize = p->_size[0];

    p->_position = Vec3d(pos.x + WorldOriginOffset.x, pos.y + WorldOriginOffset.y, pos.z);
    p->_position.z() = -playpos.z - 10.0f;

    p->_lastposition = p->_position;
    p->_originalposition = p->_position;

    p->_gravcoef = 9.8f;
    p->_stretchlength = 2.0f;

    p->_flags |= part_has_gravity;
    p->_flags |= part_has_fixstretch;
    p->_flags |= part_always_stretches;
    p->_flags |= part_follows_player_xy;

    p->_velocity.set(WeatherRandom(-0.2f, 0.2f), WeatherRandom(-0.2f, 0.2f), 16.0f);

    float t = speed / 20.0f;
    t = t > 1.0f ? 1.0f : t;

    piVECTOR vel = {0, 0, 0};

    if(speed >= 1.0f)
        persGetPlayerVelocity(&vel);

    if(persIsPersonaVehicleType(VEH_TYPE_PERSON))
        vel.x = -vel.x;

    vel.z = 0.0f;

    math3dNormalize(&vel);

    // static for debugging
    static float maxscale = 0.0f;

    if(persIsPersonaVehicleType(VEH_TYPE_AIR))
        maxscale = -20.0f;
    else if(persIsPersonaVehicleType(VEH_TYPE_GROUND))
        maxscale = -10.0f;

    float scale = maxscale * t;

    p->_velocity.x() += vel.x * scale;
    p->_velocity.y() += vel.y * scale;

    p->_killcallback = _killcallback;
    p->_updatecallback = _updatecallback;
    p->_hasendpos = true;
    p->_endpos = pos;
    p->_endpos.x() += (float)WorldOriginOffset.x;
    p->_endpos.y() += (float)WorldOriginOffset.y;
    p->_endpos.z(); // Add back 10 meters from above start point

    delayoffset += 15;
}

/*-----------------------------------------------------------------------------
Function: _createrain
Description:
-----------------------------------------------------------------------------*/
static void _createrain(int numemissions)
{
    rainparticleemitted = false;

    // Turn off rain above this height
    if(playpos.z >= RAIN_CLOUD_HEIGHT - 50.0f)
        return;

    rainparticleemitted = true;

    mlVehicleData* data = persGetPersonaModelDataPtr();

    if(!data)
        return;

    piVECTOR player;
    player.set(-playpos.x, -playpos.y, -playpos.z);

    static int numparts = 4;

    float minheight = -1;

    COLLISION collision;
    memset(&collision, 0, sizeof(COLLISION));

    FASTCOLLIDE list[32];
    unsigned int numcollides = 0;

#if(0)
    _bounds.clear();
#endif

    float t = speed / 20.0f;
    t = t > 1.0f ? 1.0f : t;

    // Move position slightly ahead if vehicle is moving
    static float posoffset = 12.0f; // WAG
    float offset = posoffset * t;

    if(persIsPersonaVehicleType(VEH_TYPE_SEA) ||
       persIsPersonaVehicleType(VEH_TYPE_PERSON))
    {
        offset = 0;
    }

    piVECTOR vel;
    persGetPlayerVelocity(&vel);
    math3dNormalize(&vel);

    trncdIgnoreTreeCollide(TRUE);

    for(int q = 0; q < numemissions; q++)
    {
        for(int i = 0; i < numparts; i++)
        {
            int slot = GetGridSlot();

            int r = slot / RainRow;
            int c = slot % RainColumn;

            // Ticket #3587: Add random variance. Reduce size of cell from 4 to 3.5
            collision.oldpos.x = player.x + (c - 4) * 3.5f + WeatherRandom(-1.0f, 1.0f);
            collision.oldpos.y = player.y + (r - 4) * 3.5f + WeatherRandom(-1.0f, 1.0f);
            collision.oldpos.z = player.z - 1000.0f;

            collision.newpos = collision.oldpos;
            collision.newpos.z = player.z + 1000.0f;

            if(trncdCollideDownShortList(collision, list, numcollides))
            {
                if(!list[0].CollideObject)
                    continue;

                if(piIsMaterialInterior(list[0].CollideObject->matid))
                    continue;

                if(list[0].CollideObject->flag3 & OBJECT_IS_INTERIOR)
                    continue;

                piVECTOR rainpos;
                math3dVectorMultiply(&rainpos, &list[0].CollidePnt, &list[0].CollideObject->mat);

                rainpos.x = collision.oldpos.x;
                rainpos.y = collision.oldpos.y;

                rainpos.x += vel.x * offset;
                rainpos.y += vel.y * offset;

                minheight = std::min(minheight, rainpos.z);

                _addparticle(rainpos, list[0].TopLevelObject);
            }
            else
            {
                if(trncdCheckTerrain(&collision))
                {
                    piVECTOR rainpos;

                    math3dVectorMultiply(&rainpos, &collision.colpos, &collision.colobj->mat);
                    minheight = std::min(minheight, collision.colpos.z);

                    rainpos.x = collision.oldpos.x;
                    rainpos.y = collision.oldpos.y;

                    rainpos.x += vel.x * offset;
                    rainpos.y += vel.y * offset;

                    _addparticle(rainpos, nullptr);
                }
            }
        }
    }

    trncdIgnoreTreeCollide(FALSE);

    // Adjust sound volume based on height
    if(rainsoundid > 0)
    {
#if XAUDIO
        static float damper = 0.25f;
        float inside = 1.0f;

        static colliderQuery collider;
        static bool firsttime = true;

        const mlVehicleData* data = persGetPersonaModelDataPtr();

        if(firsttime)
        {
            firsttime = false;
            collider.addGeomToCollideWith((dGeomID)colliderWorldSpace());
            dGeomID ray = dCreateRay(collider.space(), 10000.0f);
            dGeomRaySet(ray, 0, 0, 0, 0, 0, -1); // Updated when we collide
            collider.addGeom(ray, 0, COLLIDER_HARDTERRAIN_MASK);
        }

        colliderQuery::dGeomList& geoms = collider.geoms();
        Vec3d pos(data->VehiclePos);
        pos = physicsGameToPhysical(pos);
        pos.z() += 1000.0f;

        dGeomRaySet(geoms[0], pos.x(), pos.y(), pos.z(), 0, 0, -1);

        collider.setDirty(true);

        if(!collider.findCollisions(true))
        {
            return;
        }

        colliderQuery::CollisionList& collisions = collider.getCollisions();
        struct colliderQuery::Collision& c = collisions[0];

        double distance = fabs(playpos.z - c.pos.z());

        if(persGetPersonaVehiclePtr())
        {
            if(persGetPersonaVehiclePtr()->isInside)
                inside = 0.5f;
        }

        float min2 = 10.0f;
        float max2 = 40.0f;

        float vol = 0.0f;

        // Calculate attenuation (volume is halved each min distance)
        float numhalves = 0;

        // Get distance from min distance.
        distance -= min2;

        if(distance < 0.0f)
            distance = 0.0f;

        if(min2 > 0.0f)
        {
            numhalves = (float)distance / min2;
        }

        vol = 1.0f / pow(2.0f, numhalves);

        fxSndSetVolume(rainsoundid, vol * damper * inside);
#else
        // 10 meters high is minimum for max sound volume
        minheight = -playpos.z - minheight - 10.0f;

        minheight = minheight > 0 ? 0 : minheight < -40.0f ? -40.0f : minheight;
        float t = minheight / -40.0f;
        t = 1.0f - t;

        float vol = t * 0.005f;

        if(vol < 0.00001f)
            vol = 0.0f;

        fxSndSetVolume(rainsoundid, vol);
#endif
    }
}
#endif // USE_ODE

/*-----------------------------------------------------------------------------
Function: killrain
Description:
-----------------------------------------------------------------------------*/
static void killrain(void* data)
{
    rainsoundid = 0;
}

/*-----------------------------------------------------------------------------
Function: _doThunder
Description: Apply the next thunder sound
-----------------------------------------------------------------------------*/
static void _doThunder(unsigned int tod)
{
	if(thundertimes.size() == 0)
		return;

	if(thunderupdate == -1)
	{
		for(thunderupdate = nextthunderupdate; thunderupdate < (int)thundertimes.size(); ++thunderupdate)
		{
			if(thundertimes[thunderupdate] > tod)
			{
#ifdef BETA_RELEASE
				uiChatPrintLine("[%02u:%02u] I shall smite thee with thunder at [%02u:%02u]."
								, tod / 3600, (tod / 60) % 60
								, thundertimes[thunderupdate] / 3600, (thundertimes[thunderupdate] / 60) % 60);
#endif
				break;
			}
		}
#ifdef BETA_RELEASE
		if(thunderupdate >= (int)thundertimes.size())
			uiChatPrintLine("[%02u:%02u] I shall smite thee with thunder no more today.", tod / 3600, (tod / 60) % 60);
#endif
		// Don't play a thundersound on first pass.
		return;
	}

	// No thunders left to play, or not time to play the next...
	if(thunderupdate >= (int)thundertimes.size() || tod < thundertimes[thunderupdate])
		return;

	// We only play a thunder time when we have one pending,
	// and the tod for it to play has been reached. This
	// ensures some synchronization of thunders.
	float pitch = WeatherRandom(0.4f, 1.0f);
	float volume = WeatherRandom(0.05f, 0.5f);
	fxSndAddFiniteHeadSound("sounds/thunder.wav", IDE_FP_PRIORITY_LOW, volume, pitch);

	++thunderupdate;

#ifdef BETA_RELEASE
	if(thunderupdate < (int)thundertimes.size())
		uiChatPrintLine("[%02u:%02u] {distant rumble} ... and thy next thunderous smiting shalt be at [%02u:%02u]."
						, tod / 3600, (tod / 60) % 60
						, thundertimes[thunderupdate] / 3600, (thundertimes[thunderupdate] / 60) % 60);
#endif

	nextthunderupdate = thunderupdate;
}

/*-----------------------------------------------------------------------------
Function: _doRain
Description:
-----------------------------------------------------------------------------*/
inline static void _doRain()
{
    static int lastupdate = 0;

    if(rainparticleemitted && rainsoundid == 0)
    {
        rainsoundid = fxSndAddInfiniteHeadSound("sounds/rain.wav", IDE_FP_PRIORITY_LOW, 0.0f, 1.0f);

        if(rainsoundid > 0)
        {
            fxSndSetKillCallback(rainsoundid, killrain);
        }
    }
    else if(!rainparticleemitted && rainsoundid > 0)
    {
        fxSndStopSound(rainsoundid);
    }

    if(lastupdate == 0)
        lastupdate = frametime;

    float time = (frametime - lastupdate) * 0.001f;

    // RainGridSize per second
    int numemissions = FloatToInt(time * RainGridSize);

    if(numemissions <= 0)
        return;

    if(numemissions >= RainGridSize)
        numemissions = RainGridSize - 1;

    _createrain(numemissions);

    lastupdate = frametime;
}

#if USE_ODE
#if USE_SINGLE_COLLISION
/*-----------------------------------------------------------------------------
Function: CreateCollider
Description:
-----------------------------------------------------------------------------*/
static void CreateCollider()
{
    RainCollider = new colliderQuery(0, false);
    RainCollider->addGeomToCollideWith((dGeomID)colliderTerrainSpace());
    RainCollider->setMaxContactsPerHit(1);
	//RainCollider->setNormalFilter(0.0f, 0.0f, 1.0f);

#if USE_RAY
    for(int i = 0; i < 4; i++)
    {
	    dGeomID ray = dCreateRay(RainCollider->space(), 10000.0f);
	    dGeomRaySet(ray, 0, 0, 0, 0, 0, 1);
        RainCollider->addGeom(ray, 0, COLLIDER_TRN_GROUND);
    }
   // dGeomID ray = dCreateRay(RainCollider->space(), 10000.0f);
  //  dGeomRaySet(ray, 0, 0, 0, 0, 0, 1);
  //  RainCollider->addGeom(ray, 0, COLLIDER_TRN_GROUND);
#else
    dGeomID cylinder = dCreateCapsule(RainCollider->space(), 3.0f, 10000.0f);
    RainCollider->addGeom(cylinder, 0, COLLIDER_TRN_GROUND);
#endif
}
#else // USE_SINGLE_COLLISION
/*-----------------------------------------------------------------------------
Function: CreateCollider
Description:
-----------------------------------------------------------------------------*/
static void CreateCollider()
{
    RainCollider = new colliderQuery(0, false);
    RainCollider->addGeomToCollideWith((dGeomID)colliderTerrainSpace());
    RainCollider->setMaxContactsPerHit(1);
	//RainCollider->setNormalFilter(0.0f, 0.0f, 1.0f);

    for(int i = 0; i < RainGridSize; i++)
    {
	    dGeomID ray = dCreateRay(RainCollider->space(), 10000.0f);
	    dGeomRaySet(ray, 0, 0, 0, 0, 0, 1);
        RainCollider->addGeom(ray, 0, COLLIDER_TRN_GROUND);
    }
}
#endif // USE_SINGLE_COLLISION
#endif // USE_ODE

/*-----------------------------------------------------------------------------
Function: ChangeWeaterState
Description:
-----------------------------------------------------------------------------*/
static void ChangeWeaterState()
{
    if(weatherstate & WXSTATE_CLEAR_DAY)
    {
        trnskyChangeSetting(0, trnskyGetFogOption(), true);
    }

    if((weatherstate & WXSTATE_CLOUDS) || (weatherstate & WXSTATE_RAIN))
    {
        trnskyChangeSetting(2, trnskyGetFogOption(), true);
    }
}

/*-----------------------------------------------------------------------------
Function: fxWeatherUpdate
Description:
-----------------------------------------------------------------------------*/
void fxWeatherUpdate()
{
    if(!uiInGame())
    {
        return;
    }

    static bool firsttime = true;

    if(firsttime)
    {
        firsttime = false;

#if USE_ODE
        CreateCollider();
#endif
    }

    bool rainupdate = uiInGame() && (Online ? g_bVehicleAttached : true);

	unsigned int tod;
	trnTODGetTimeOfDay(&tod);
    if(weatherstate != oldweatherstate || tod < lasttod)
    {
        ChangeWeaterState();
        oldweatherstate = weatherstate;
    }
	lasttod = tod;

	if(weatherstate != WXSTATE_CLEAR_DAY)
		_doThunder(tod);

    if((weatherstate & WXSTATE_RAIN) && rainupdate)
    {
        piVECTOR vel;
        persGetPlayerVelocity(&vel);

        speed = math3dFastSqrt(math3dDotProduct(&vel, &vel));

#if USE_ODE
#if USE_SINGLE_COLLISION
        // Adjust sound volume based on height
        if(rainsoundid > 0)
        {
            Vec3d pos(-playpos.x, -playpos.y, -playpos.z);
            pos.x() += WorldOriginOffset.x;
            pos.y() += WorldOriginOffset.y;

            pos = physicsGameToPhysical(pos);
            pos.z() -= 1000.0f;

            colliderQuery::dGeomList& geoms = RainCollider->geoms();
#if USE_RAY
            dGeomRaySet(geoms[0], pos.x(), pos.y(), pos.z(), 0, 0, 1);
#else
            dGeomSetPosition(geoms[0], pos.x(), pos.y(), pos.z());
#endif

            RainCollider->setDirty(true);

            if(RainCollider->findCollisions(true))
            {
                colliderQuery::CollisionList& collisions = RainCollider->getCollisions();
                struct colliderQuery::Collision& c = collisions[collisions.size() - 1];

#if USE_RAY
                Vec3d start, end;
                osgGeomRayGet(c.g1, start, end);
                float z = -(float)(start.z() + c.depth);
#else
                float z = -float(c.pos.z());
#endif

                // 10 meters high is minimum for max sound volume
                float minheight = -(z + playpos.z);

                minheight = minheight > 0 ? 0 : minheight < -40.0f ? -40.0f : minheight;
                float t = minheight / -40.0f;
                t = 1.0f - t;

                float vol = t * 0.005f;

                if(vol < 0.00001f)
                    vol = 0.0f;

                fxSndSetVolume(rainsoundid, vol);
            }
        }
#endif // USE_SINGLE_COLLISION
#endif // USE_ODE

#if TEST_OFFLINE
#else
        if(Online)
#endif
        {
            if(!dorain)
            {
                NextRainTime = -1;

                for(unsigned int i = 0; i < raintimes.size(); i++)
                {
                    if((tod >= raintimes[i]) && tod <= (raintimes[i] + rainlength[i]))
                    {
                        NextRainTime = i;
                        break;
                    }
                }

#ifdef BETA_RELEASE
                static int nexttime = -1;

                if(NextRainTime != -1 && nexttime != NextRainTime)
                {
                    PrintTimeLine();
                    nexttime = NextRainTime;
                }
#endif // BETA_RELEASE

                if(NextRainTime != -1)
                {
                    // Overlaps possible...but who cares.
                    if(NextRainTime < (int)raintimes.size() && tod >= raintimes[NextRainTime])
                    {
                        StartRainTime = tod;
                        EndRainTime = StartRainTime + rainlength[NextRainTime];

                        dorain = true;
                    }
                }
            }
        }

        if(dorain)
        {
            _doRain();

#if TEST_OFFLINE
#else
            if(Online && !gCustomTerrain)
#endif
            {
                if((unsigned int)tod >= EndRainTime)
                {
                    if(rainsoundid > 0)
                        fxSndStopSound(rainsoundid);

                    dorain = false;
                }
            }
        }
    }
    else
    {
        if(rainsoundid > 0)
        {
            fxSndStopSound(rainsoundid);
        }
    }
}

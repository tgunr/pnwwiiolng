#include <syscfg.h> // Path is project specific.
// Date: Tue Apr 10 10:22:23 2001
//
// Time components of the weather system
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson
// Supporting Programmer:  Eric "Meat" Hall

#include "include/wthrTime.h"
#include "wwii/wwiiConfig.h"

static UINT32 _lastRealTime, _lastGameTime;

// How many game seconds pass for 1 elapsed wall second based on hours
UINT32 wthrTimeFactors[HOURS_PER_DAY] =
{
    20                  // 00:00
,   20                  // 01:00
,   20                  // 02:00
,   20                  // 03:00
,   20                  // 04:00
,   20                  // 05:00
,   3                   // 06:00
,   3                   // 07:00
,   3                   // 08:00
,   3                   // 09:00
,   3                   // 10:00
,   3                   // 11:00
,   3                   // 12:00
,   3                   // 13:00
,   3                   // 14:00
,   3                   // 15:00
,   3                   // 16:00
,   3                   // 17:00
,   20                  // 18:00
,   20                  // 19:00
,   20                  // 20:00
,   20                  // 21:00
,   20                  // 22:00
,   20                  // 23:00
};

//This function initializes all the time components
void
wthrInitTime(void)
{
	//Set lastGameTime to midnight
	_lastGameTime = 0;
	//Sets current time to the clock
	_lastRealTime = (UINT32) teulTime();
}

//This function returns the current time in the world
UINT32
wthrGetGameTime(void)
{
	//Get the realTime
	UINT32 currentRealTime = (UINT32) teulTime();
	if ( currentRealTime == _lastRealTime )
		return _lastGameTime;

	if ( host_config.arena.freezeClock )
	{
		UINT32 hours = host_config.arena.freezeClock / 100;
		UINT32 minutes = (host_config.arena.freezeClock % 100) % 60;
		_lastRealTime = teulTime();
		_lastGameTime = ((hours * 60) + minutes) * 60;
		return _lastGameTime;
	}

	//Calculate the game time off of the current real time
	UINT32 newTime = _lastGameTime + (currentRealTime - _lastRealTime) * wthrGetTimeFactor();

	newTime %= SECONDS_PER_DAY;

	//Save these values for the next time around
	_lastRealTime = currentRealTime;
	_lastGameTime = newTime;

	return newTime;
}

void
wthrSetGameTime(UINT32 gameTime)
{
	_lastRealTime = (UINT32) teulTime();
	_lastGameTime = (gameTime % SECONDS_PER_DAY);
}

class WeatherForecast
{
	public: WeatherForecast(const char* name, const UINT32 numOptions, const UINT16& lockTo);
	public: ~WeatherForecast();

	public: void GenerateNextForecast();

	// We return 1-based valued but store 0-based internally
	public: UINT32 GetCurrent() const { return m_today + 1; }
	public: UINT32 GetForecast() const { return m_tomorrow + 1; }
	public: void SetForecast(const UINT32 selection);

	private: const char* m_name;						// What we are describing
	private: const UINT32 m_numOptions;					// Range of options (fixed)
	private: const UINT16& m_lockTo;					// Dynamic value to lock to
	private: UINT32 m_yesterday, m_today, m_tomorrow;	// Histogram

	private: WeatherForecast(const WeatherForecast&);
};

WeatherForecast::WeatherForecast(const char* name, const UINT32 numOptions, const UINT16& lockTo)
	: m_name(name)
	, m_numOptions(numOptions), m_lockTo(lockTo)
	, m_yesterday(0), m_today(0), m_tomorrow(9999)
{
}

WeatherForecast::~WeatherForecast()
{
}

static UINT32
_random(UINT32 numOptions)
{
	if ( numOptions > 3 )
		return rand() % numOptions;

	static const UINT32 modulo = 1024;	// Must be a power of 2 to provide even rand() distribution.
	// Transform the chance into a factor of the modulo.
	const UINT32 chance = std::min((UINT32)host_config.weather.chanceOf, (UINT32)100) * modulo / 100;

	UINT32 roll = rand() % modulo;		// Roll of the dice.
	if ( roll < chance )
		return 2;

	return 0;
}

void
WeatherForecast::GenerateNextForecast()
{
	if ( m_tomorrow == 9999 )
		m_tomorrow = _random(m_numOptions);

	const char* method;
	UINT32 prediction;
	if ( m_numOptions <= 1 )
	{
		m_tomorrow = prediction = 0;
		method = "defaulting to";
	}
	else if ( m_lockTo > 0 && m_lockTo < m_numOptions )
	{
		m_tomorrow = prediction = m_lockTo - 1;
		method = "locking to";
	}
	else
	{
		prediction = _random(m_numOptions);
		if ( prediction > 0 && prediction == m_tomorrow )
		{
			prediction = (m_numOptions <= 3 ? 2 - prediction : (prediction+1) % m_numOptions);
			method = "adjusting";
		}
		else
			method = "predicting";
	}

	dbugLog(_FLI, "%s %s %u, setting today to %u", method, m_name, prediction + 1, m_tomorrow + 1);

	m_yesterday = m_today;
	m_today = m_tomorrow;
	m_tomorrow = prediction;
}

void
WeatherForecast::SetForecast(const UINT32 selection)
{
	wthrGetGameTime();

	if ( selection > 0 )
	{
		if ( selection > 1 && selection > m_numOptions )
		{
			dbugLog(_FLW, "Attempted to set %s to %u (max %u), ignoring", m_name, selection, m_numOptions);
			return;
		}
		m_today = (selection - 1);
	}
	else
		GenerateNextForecast();
}

static WeatherForecast weatherForecast("weather forecast", 3, host_config.weather.lockToPattern);
static WeatherForecast fogForecast("fog forecast", 16, host_config.weather.lockToFog);

void
wthrSetWeatherPattern(UINT32 pattern)
{
	weatherForecast.SetForecast(pattern);
}

void
wthrSetFogPattern(UINT32 pattern)
{
	fogForecast.SetForecast(pattern);
}

//Gets the current time factor
UINT32
wthrGetTimeFactor(void)
{
	if ( host_config.arena.fixedTimeFactor > 0 )
		return host_config.arena.fixedTimeFactor;

	return wthrTimeFactors[(_lastGameTime % SECONDS_PER_DAY) / SECONDS_PER_HOUR];
}

// Determine the number of real seconds left until the end of the current hour
UINT32
wthrRealSecondsToHour(void)
{
	UINT32 gameSecondsLeft = (SECONDS_PER_HOUR - (_lastGameTime % SECONDS_PER_HOUR));
	return gameSecondsLeft / wthrTimeFactors[(_lastGameTime % SECONDS_PER_DAY) / SECONDS_PER_HOUR];
}

// wthrConvertTimeToString()
//
// returns the time in a string with the format HH:MM
// ie 06:17  or 12:08, always 5 digits including ":"
const char*
wthrConvertTimeToString(UINT32 _time)
{
	if(_time >= SECONDS_PER_DAY) //no such time
		return NULL;

	UINT32 hours =_time/(SECONDS_PER_HOUR);
	if ( hours > 23 )
		return NULL;
	UINT32 mins =(_time - (hours*SECONDS_PER_HOUR))/60;
	if ( mins > 59 )
		return NULL;

	static char timeStr[5+1] = "00:00";
	// Very simple placement of corresponding time characters
	timeStr[0] = static_cast<char>(hours / 10 + '0');
	timeStr[1] = static_cast<char>(hours % 10 + '0');
	// timeStr[2] never changes from ':'
	timeStr[3] = static_cast<char>(mins / 10 + '0');
	timeStr[4] = static_cast<char>(mins % 10 + '0');

	return timeStr;
}

//////////////////////////////////////////////////////////////////////
// Get the current weather pattern

UINT32
wthrGetWeatherPattern()
{
	return weatherForecast.GetCurrent();
}

//////////////////////////////////////////////////////////////////////
// Get the current fog pattern

UINT32
wthrGetFogPattern()
{
	return fogForecast.GetCurrent();
}

//////////////////////////////////////////////////////////////////////
// Get tomorrow's weather pattern

UINT32
wthrGetWeatherForecast()
{
	return weatherForecast.GetForecast();
}

//////////////////////////////////////////////////////////////////////
// Get tomorrow's fog pattern

UINT32
wthrGetFogForecast()
{
	return fogForecast.GetForecast();
}

//////////////////////////////////////////////////////////////////////
// Calculate the in-game duration of a day

FLOAT64
wthrGetGameDayDuration()
{
	static FLOAT64 dayDuration = 0.0f;
	if ( !dayDuration )
	{
		for ( UINT32 i = 0; i < HOURS_PER_DAY; ++i )
		{
			dayDuration += (3600.0 / wthrTimeFactors[i]);
		}
	}

	return dayDuration;
}

//////////////////////////////////////////////////////////////////////
// Set new fog and cloud patterns

void
wthrStartNewDay()
{
	wthrGetGameTime();

	weatherForecast.GenerateNextForecast();
	fogForecast.GenerateNextForecast();
}



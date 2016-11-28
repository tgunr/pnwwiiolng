#include <syscfg.h> // Path is project specific.
//

#include "pi/pi.h"
#include "include/vars.h"
#include <math.h>

#include "trn/trntod.h"
#include "scen/ScenarioManager.h"

static UINT32 _TimeOfDay; // 0 to 86399, from 12:00 midnight to 11:59:59... next evening
static UINT32 _DayOfYear; // 1 to 365, 1 being Jan 1 for weather/time of year purposes
static UINT32 _DayOfMonth; // 1 to 28, 1 being new moon, 14 being full moon

static FLOAT32 _MSCurrentSecond; // Millisecond Counter

// the multiplier works like this:
// a value of 2 makes 2 seconds pass in the game for each real world second
// a value of .5 makes .5 seconds pass
// flight models are totally unaffected by this multiplier
static FLOAT32 _DaytimeTimeMultiplier; // time rate accelerator

SINT32 trntodTimeOfDayHasChanged; // flag to state that the time of day has changed

UINT32 trnTODInit()
{
	_TimeOfDay = 43200; // midday
	_DaytimeTimeMultiplier = 1.0;

	_DayOfYear = 1;
	_DayOfMonth = 1;

	_MSCurrentSecond = 0.0;

	return 0;
}

UINT32 trnTODUpdate()
{
    UINT32 TempNum;
    static UINT32 _Lastframetime;

    if (_Lastframetime == 0)
    {
	    _Lastframetime = frametime;
	    return 0;
    }

    _MSCurrentSecond += (frametime - _Lastframetime) * _DaytimeTimeMultiplier;
    _Lastframetime = frametime;

    if (_MSCurrentSecond > 1000.0)
    {
	    TempNum = (UINT32)(floor(_MSCurrentSecond / 1000.0));
	    _MSCurrentSecond -= (FLOAT32)(1000.0 * TempNum);

	    _TimeOfDay += TempNum;
    }

    if (_TimeOfDay > 86399)
    {
	    TempNum = _TimeOfDay / 86400;
	    _TimeOfDay -= TempNum * 86400;

	    _DayOfYear += TempNum;
	    _DayOfMonth += TempNum;

	    _DayOfYear = (_DayOfYear - 1) % 365 + 1;
	    _DayOfMonth = (_DayOfMonth - 1) % 28 + 1;
    }

    if (trntodTimeOfDayHasChanged > 0)
    {
	    trntodTimeOfDayHasChanged--;
    }

	return 0;
}

UINT32 trnTODGetTimeOfDay(UINT32 * TimeOfDay)
{
	if (TimeOfDay != nullptr)
	{
		*TimeOfDay = _TimeOfDay;
	}

	return 0;
}

UINT32 trnTODGetDayOfYear(UINT32 * DayOfYear)
{
	if (DayOfYear != nullptr)
	{
		*DayOfYear = _DayOfYear;
	}

	return 0;
}

UINT32 trnTODGetDayOfMonth(UINT32 * DayOfMonth)
{
	if (DayOfMonth != nullptr)
	{
		*DayOfMonth = _DayOfMonth;
	}

	return 0;
}

UINT32 trnTODSetTimeOfDay(UINT32 TimeOfDay)
{
	_TimeOfDay = TimeOfDay;

	// we set this to 2 because the updater, which clears this, gets called
	// very early in the main loop, thus by using a decrementer, we
	// guarantee the entire loop sees the change at least once
	trntodTimeOfDayHasChanged = 2;

	return 0;
}

UINT32 trnTODSetDayOfYear(UINT32 DayOfYear)
{
	_DayOfYear = DayOfYear;

	return 0;
}

UINT32 trnTODSetDayOfMonth(UINT32 DayOfMonth)
{
	_DayOfMonth = DayOfMonth;

	return 0;
}

UINT32 trnTODGetTimeMultiplier(FLOAT32 * Multiplier)
{
	if (Multiplier == nullptr)
	{
		return 0;
	}

	*Multiplier = _DaytimeTimeMultiplier;

	return 0;
}

UINT32 trnTODSetTimeMultiplier(FLOAT32 Multiplier)
{
	_DaytimeTimeMultiplier = Multiplier;

	return 0;
}

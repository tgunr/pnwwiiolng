#pragma once

#define trnTODGetHourFromTimeOfDay(A) (A / 3600)
#define trnTODGetMinuteFromTimeOfDay(A) ((A - (A / 3600 * 3600)) / 60)
#define trnTODGetSecondFromTimeOfDay(A) (A % 60)

UINT32 trnTODInit();
UINT32 trnTODUpdate();

UINT32 trnTODGetTimeOfDay(UINT32 * TimeOfDay);
UINT32 trnTODGetDayOfYear(UINT32 * DayOfYear);
UINT32 trnTODGetDayOfMonth(UINT32 * DayOfMonth);
UINT32 trnTODSetTimeOfDay(UINT32 TimeOfDay);
UINT32 trnTODSetDayOfYear(UINT32 DayOfYear);
UINT32 trnTODSetDayOfMonth(UINT32 DayOfMonth);

// accelerate time by the multiplier function
// the multiplier works like this:
// a value of 2 makes 2 seconds pass in the game for each real world second
// a value of .5 makes .5 seconds pass
// flight models are totally unaffected by this multiplier
UINT32 trnTODGetTimeMultiplier(FLOAT32 * Multiplier);
UINT32 trnTODSetTimeMultiplier(FLOAT32 Multiplier);

extern SINT32 trntodTimeOfDayHasChanged;

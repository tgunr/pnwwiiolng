#pragma once
//
// Created: Mon Apr  9 18:15:11 2001
//
// Time components of the weather system
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#define HOURS_PER_DAY 24
#define SECONDS_PER_DAY 86400
#define SECONDS_PER_HOUR 3600

//Time is expressed in the number of seconds that have elapsed since
//	midnight, which gives it a range of 0-86399

//This function initializes all the time components
extern void wthrInitTime();

//tolua_begin
//This function returns the current time in the world
extern UINT32 wthrGetGameTime();
extern void wthrSetGameTime(UINT32 gameTime);

//Gets the current time factor
extern UINT32 wthrGetTimeFactor();
//convert time to a string HH:MM
extern const char* wthrConvertTimeToString(UINT32 _time);

extern UINT32 wthrRealSecondsToHour();

extern UINT32 wthrGetWeatherPattern();
extern void wthrSetWeatherPattern(UINT32 pattern);

extern UINT32 wthrGetFogPattern();
extern void wthrSetFogPattern(UINT32 pattern);

extern UINT32 wthrGetWeatherForecast();
extern UINT32 wthrGetFogForecast();

extern void wthrStartNewDay();

// Return the number of seconds that a game-day lasts.
extern FLOAT64 wthrGetGameDayDuration();
//tolua_end


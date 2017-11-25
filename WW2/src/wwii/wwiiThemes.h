//
//  wwiiThemes.h
//  WW2OL
//
//  Copyright (C) PlayNet, Inc 1999-2017 - All rights reserved.
//  Not for redistribution.
//
//  Created by Ian Frisbie on 11/23/17.
//

#ifndef wwiiThemes_h
#define wwiiThemes_h

#include <string>

extern std::string optionalFileSuffix;

namespace WWII
{
	enum THEME {
		THEME_NONE = 0,
		
		THEME_HOLIDAY_SEASON,
		THEME_VETERANS,
		THEME_HALLOWEEN,
		
		THEME_MAX
	};
	
	class Theme
	{
		
	private:
		static std::string toString(THEME theme) {
			static std::string themeList[] = { "", "_xmas", "_vets", "_spooky" };
			return (isValid(theme)?themeList[theme]:"");
		}
		static THEME m_currentTheme;
		static bool isValid(UINT32 themeRef) { return themeRef < THEME_MAX; }
	public:
		static THEME theme() { return m_currentTheme; }
		static void setTheme(UINT32 theme) {
			if (isValid(theme)) {
				m_currentTheme =static_cast<THEME>(theme);
				optionalFileSuffix = toString(m_currentTheme);
			}
		}
	};
}

#endif /* wwiiThemes_h */

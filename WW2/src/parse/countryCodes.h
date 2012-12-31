#pragma once

namespace Parse
{

	enum COUNTRY_CODE
	{
		ND_=0, // Not Defined
		UK_,   // United Kingdom
		US_,   // United States
		FR_,   // France
		DE_,   // Germany
		IT_,   // Italy
		FY_, // friendly country
		EY_, // enemy country
		MAX_COUNTRY_CODES
	} ;

}

extern Parse::COUNTRY_CODE findCountryCode(const char* country);


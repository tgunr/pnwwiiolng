#pragma once

enum
{
	ERR_WRONG_SUPERCELL
,	ERR_CANT_FIND_PARENT_FACILITY
,	ERR_LINK_START_MULTIPLE_LINKS
,	ERR_NO_CAPTURE_TABLE
,	ERR_ZERO_LINKS
,	ERR_ZERO_ILINKS
,	ERR_ZERO_OLINKS
,	ERR_MISMATCH_LINKS
,	ERR_NULL_ILINK
,	ERR_NULL_OLINK
,	ERR_FB_BOTH_CHECK
,	ERR_FB_DAMAGEABLE_SPAWN
,	ERR_FB_DIVISOR
,	ERR_FB_NO_LINK
,	ERR_FB_NO_MATE
,	ERR_AB_BOTH_CHECK
,	ERR_AF_NO_SPAWN
,	ERR_CP_NO_OWNABLES
,	ERR_DEFINED_CONTESTED
,	ERR_OBJ_NOT_IN_BLITZWOR
,	ERR_CANT_FIND_LINK_BUILDING
,	ERR_LINK_START_DEPOT_LINK
,	ERR_LINK_END_DEPOT_LINK
,	ERR_LINK_FIREBASE
,	ERR_FB_ZERO_OBJECTS
,	ERR_MULT_CAP_OBJECTS

,	MAX_ERR_CODES
};


struct ErrorInfo
{
	const char*	name;				// Name for blitzWor
	const char* text;				// Descriptive text.
	UINT32		level;				// Error level
	bool		canIgnore;			// Can ignore minor ones
};

class DataErrors
{
	using ErrorMap = std::map<std::string, std::string>;
	using ErrorList = std::array<ErrorMap, MAX_ERR_CODES>;
	ErrorList m_errorList;

public:
	void emplace(UINT32 code, const char* key, std::string&& value)
	{
		m_errorList[code].emplace(key, std::move(value));
	}

	const ErrorMap& operator[](size_t index) const noexcept { return m_errorList[index]; }
	ErrorMap& operator[](size_t index) noexcept { return m_errorList[index]; }
};

extern DataErrors g_dataErrors;

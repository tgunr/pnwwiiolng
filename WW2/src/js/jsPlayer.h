#pragma once

#include "phui/Javascript.h"

/************************************************************************************************
	jsPlayer
*************************************************************************************************/

class jsPosition;
class jsClient;
#ifdef DOROTHY
class jsGR2Object;
#endif

class jsPlayer : public phui::js::Object
{
public:
	jsPlayer();

	JS_CLASS_DATA;

enum
{
	POSITION_PROP,ATPLEVEL_PROP,BLOODLEVEL_PROP,
	PRIMARYAMMOCOUNT_PROP, SECONDARYAMMOCOUNT_PROP,
	PRIMARYCLIPCOUNT_PROP, SECONDARYCLIPCOUNT_PROP,
	PRIMARYAMMONAME_PROP, SECONDARYAMMONAME_PROP,
	STATE_PROP, WALKRUN_PROP, VEHICLETYPE_PROP,
	NEAREST_CP_PROP, RTB_PROP, CLIENT_PROP, RTBFAC_PROP,
	PRIMARYOVERSUPPLY_PROP, SECONDARYOVERSUPPLY_PROP,
	PRIMARYCLIPSIZE_PROP, SECONDARYCLIPSIZE_PROP,
	RGAMMO_PROP, RGLOADED_PROP, ISOVERSUPPLIED_PROP,
	PRIMARYCLIPTYPE_PROP, SECONDARYCLIPTYPE_PROP,
	PRIMARYRANGE_PROP, SECONDARYRANGE_PROP,
	NOFIREZONE_PROP, CAPTUREZONE_PROP, HULKED_PROP,
	DEATHCAM_PROP, COUNTRY_PROP
#ifdef DOROTHY
	,CROSSHAIRSX_PROP, CROSSHAIRSY_PROP,
	GR2VEHICLE_PROP
#endif
};

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);

	phui::js::root_ptr<jsClient> jsclient;
//	jsClient* jsclient;
	phui::js::root_ptr<jsPosition> jsposition;
#ifdef DOROTHY
	phui::js::root_ptr<jsGR2Object> jsgr2vehicle;
#endif

protected:
	~jsPlayer();
};


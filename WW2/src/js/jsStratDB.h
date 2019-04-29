#pragma once

#include "phui/Javascript.h"

#include "js/jsClient.h"
#include "js/jsPosition.h"
#include "sdm/StratDB.h"

class jsCountry;
class jsPosition;
class jsBoundingBox;

/************************************************************************************************
	jsStratObject
*************************************************************************************************/

class jsStratObject : public phui::js::Object
{
public:
//	jsStratObject(StratDB::StratObject const*);FULL_OBJ_ID const*
    jsStratObject(FULL_OBJ_ID const*);

	JS_CLASS_DATA;
	JS_FUNCTION(attack);
	JS_FUNCTION(defend);
	JS_FUNCTION(clear);
	JS_FUNCTION(validmissionorigin);
	JS_FUNCTION(changegarrisoncountry);

	enum
	{
		PARENT_PROP, CHILDREN_PROP, NAME_PROP, ASCII_NAME,
		COUNTRY_PROP, POSITION_PROP, BBOX_PROP, BRIGADES_PROP,
		CANATTACK_PROP, CANDEFEND_PROP, CANCLEAR_PROP,
		SIDE_PROP, CONTESTED_PROP, HASAO_PROP
	};

	enum
	{
		MISSION_ORIGIN_INVALID = 0,
		MISSION_ORIGIN_OK,
		MISSION_ORIGIN_NOTFACILITY,
		MISSION_ORIGIN_NOTSPAWNABLE,
		MISSION_ORIGIN_NOTOWNED,
		MISSION_ORIGIN_BADFACILITY
	};

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);

	StratDB::StratObject const* stratobj;

protected:
	phui::js::root_ptr<jsStratObject> parent;
	phui::js::array_root_ptr children;
	phui::js::array_root_ptr brigades;
// rather than adding the properties in the constructor we do it the old way
// to reduce the number of js objects allocated in the global cp list
	phui::js::root_ptr<jsCountry> country;
	phui::js::root_ptr<jsPosition> position;
	phui::js::root_ptr<jsBoundingBox> bbox;
	phui::js::jsval_root asciiname;
	phui::js::jsval_root name;

	~jsStratObject(){}
};


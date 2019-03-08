#include "syscfg.h" // Path is project specific.
#include <string.h>
#include <math.h>

#include "sdm/sdm.h"
#include "wwii/wwiiTypes.h"
#include "js/jsStratDB.h"
#include "client/GameClient.h"
#include "strt/strtCPFlags.h"
#include "weap/weapList.h"
#include "wwii/wwiiHCUnits.h"

/************************************************************************************************
	jsStratObject
*************************************************************************************************/

static const char* _stratclasses[SDM_MAX_STRAT_CLASSES] = {"none", "cp", "facility", "building", "link", "waypoint"};
static const char* _cptypes[SDM_MAX_CP_TYPES] = {"none", "generic", "small", "medium", "large", "bridge" };
static const char* _factypes[SDM_MAX_FACILITY_TYPES] = { "none", "city", "factory", "base", "depot", "omt", "shipyard", "firebase", "airbase", "armybase","navalbase" };

JSClass jsStratObject::jsclass = JS_CLASS_MACRO("stratobject");
JSFunctionSpec jsStratObject::jsfunctions[] =
{
	JS_FUNCTION_SPEC(jsStratObject, attack, 0),
	JS_FUNCTION_SPEC(jsStratObject, defend, 0),
	JS_FUNCTION_SPEC(jsStratObject, clear, 0),
	JS_FUNCTION_SPEC(jsStratObject, validmissionorigin, 0),
	JS_FUNCTION_SPEC(jsStratObject, changegarrisoncountry, 1),
	{0}
};

JSPropertySpec jsStratObject::jsproperties[] =
{
	{"parent", 		PARENT_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"children", 	CHILDREN_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"name",		NAME_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"ascii",		ASCII_NAME,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"country",		COUNTRY_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"position",	POSITION_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"bbox",		BBOX_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"brigades",	BRIGADES_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"canattack",	CANATTACK_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"candefend",	CANDEFEND_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"canclear",	CANCLEAR_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"side",	SIDE_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"contested",	CONTESTED_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"hasao",	HASAO_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{0}
};

/************************************************************************************************
	jsStratObject
*************************************************************************************************/

jsStratObject::jsStratObject(FULL_OBJ_ID const*id)
			: phui::js::Object(&jsStratObject::jsclass, jsfunctions,jsproperties)
{
	// we do this because map objects (which often times create these objects)
	// use a cached version of the the underlying strat object and the map
	// object can go away, leaving this stratobj trashed; this ensures we
	// always have a good stratobj
	stratobj = StratDB::FindStratObject(*id);

	JS_CONSTRUCTOR_LOCK;

	DefineProperty("originalcountry", new jsCountry((WWII_COUNTRY)stratobj->OriginalCountry()));
	DefineProperty("originalside", phui::js::ConstString((stratobj->OriginalSide() == AXIS) ? "axis" : "allies"));
	DefineProperty("side", phui::js::ConstString((stratobj->Side() == AXIS) ? "axis" : "allies"));
	DefineProperty("stratclass", phui::js::ConstString(
		(stratobj->StratClass() < SDM_MAX_STRAT_CLASSES) ? _stratclasses[stratobj->StratClass()] : ""));

	int spawnType = 0;

	switch(stratobj->StratClass())
	{
		case SDM_CLASS_CP:
			DefineProperty("strattype", phui::js::ConstString(
				(stratobj->StratType() < SDM_MAX_CP_TYPES) ? _cptypes[stratobj->StratType()] : ""));
		break;
		case SDM_CLASS_FACILITY:
		{
			DefineProperty("strattype", phui::js::ConstString(
				(stratobj->StratType() < SDM_MAX_FACILITY_TYPES) ? _factypes[stratobj->StratType()] : ""));

			switch(stratobj->StratType())
			{
				case SDM_FACILITY_ARMYBASE:
				case SDM_FACILITY_FIREBASE:
				case SDM_FACILITY_DEPOT:
					spawnType = 1;
				break;
				case SDM_FACILITY_AIRBASE:
					spawnType = 2;
				break;
				case SDM_FACILITY_NAVALBASE:
					spawnType = 3;
				break;
			}
		}
		break;
	}

	DefineProperty("spawnable", BOOLEAN_TO_JSVAL(stratobj->IsSpawnable()));
	DefineProperty("spawntype", INT_TO_JSVAL(spawnType));
	DefineProperty("deepwaterport", BOOLEAN_TO_JSVAL(stratobj->IsDeepWaterPort()));
}

/************************************************************************************************
	attack
*************************************************************************************************/

JS_FUNCTION_DEF(jsStratObject, attack)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	jsStratObject* target = phui::js::Cast<jsStratObject*>(ctx, jsobj);
	if(!target) return JS_FALSE;

	BrigadeControlRequest req;

	req.requestAttack(
		PlayerClient()->ChatConnection(),
		*target->stratobj->FullObjID());

	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	return JS_TRUE;
}

/************************************************************************************************
	defend
*************************************************************************************************/

JS_FUNCTION_DEF(jsStratObject, defend)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	jsStratObject* target = phui::js::Cast<jsStratObject*>(ctx, jsobj);
	if(!target) return JS_FALSE;

	BrigadeControlRequest req;

	req.requestDefend(
		PlayerClient()->ChatConnection(),
		*target->stratobj->FullObjID());

	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	return JS_TRUE;
}

/************************************************************************************************
	clear
*************************************************************************************************/

JS_FUNCTION_DEF(jsStratObject, clear)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	jsStratObject* target = phui::js::Cast<jsStratObject*>(ctx, jsobj);
	if(!target) return JS_FALSE;

	BrigadeControlRequest req;

	req.requestClear(
		PlayerClient()->ChatConnection(),
		*target->stratobj->FullObjID());

	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	return JS_TRUE;
}

/************************************************************************************************
	validmissionorigin
*************************************************************************************************/

JS_FUNCTION_DEF(jsStratObject, validmissionorigin)
{
	*rval = INT_TO_JSVAL(jsStratObject::MISSION_ORIGIN_INVALID);

	// must be a valid strat object
	jsStratObject* origin = phui::js::Cast<jsStratObject*>(ctx, jsobj);
	if(!origin || !origin->stratobj) return JS_TRUE;

	// must be a facility
	if(origin->stratobj->StratClass() != SDM_CLASS_FACILITY)
	{
		*rval = INT_TO_JSVAL(jsStratObject::MISSION_ORIGIN_NOTFACILITY);
		return JS_TRUE;
	}

	// must be same side
	const PersonaSelection& persona = PlayerClient()->PersonaSelected();
	if(origin->stratobj->Side() != wwiiSide(persona.m_countryID))
	{
		*rval = INT_TO_JSVAL(jsStratObject::MISSION_ORIGIN_NOTOWNED);
		return JS_TRUE;
	}

	// must be a spawnable facility
	if(!origin->stratobj->IsSpawnable())
	{
		*rval = INT_TO_JSVAL(jsStratObject::MISSION_ORIGIN_NOTSPAWNABLE);
		 return JS_TRUE;
	}

	// must be in my brigade's cp or my division's cp or
	UINT32 brigadeID	= PlayerClient()->BrigadeSelected().m_detail.unitID();
	UINT32 divisionID	= PlayerClient()->BrigadeSelected().m_detail.parentID();
	bool nearBrigade	= origin->stratobj->IsUnitHere(brigadeID);
	bool nearDivision	= origin->stratobj->IsUnitHere(divisionID);
	bool validLocation	= nearBrigade || nearDivision;

	if(!validLocation)
	{
		// this facility is not in my brigade or my division's cp
		// only other valid place is in a friendly depot in an enemy cp linked to my brigade's depot
		if(origin->stratobj->StratType() == SDM_FACILITY_DEPOT)
		{
			StratDB::StratObject* linkedDepot = origin->stratobj->GetLinkedDepot();

			if(linkedDepot && linkedDepot->IsUnitHere(brigadeID))
				validLocation = true;
		}
	}

	// if its a firebase, it must be linked to my brigade
	if(validLocation && origin->stratobj->StratType() == SDM_FACILITY_FIREBASE && !nearBrigade)
		validLocation = false;

	if(!validLocation)
	{
		*rval = INT_TO_JSVAL(jsStratObject::MISSION_ORIGIN_BADFACILITY);
		 return JS_TRUE;
	}

	// if we made it here, it must be a valid mission origin
	// althought the host could still deny it
	*rval = INT_TO_JSVAL(jsStratObject::MISSION_ORIGIN_OK);

	return JS_TRUE;
}

/************************************************************************************************
changegarrisoncountry
*************************************************************************************************/

JS_FUNCTION_DEF(jsStratObject, changegarrisoncountry)
{
	if (argc < 1) return JS_FALSE;

	// must be a valid strat object
	jsStratObject* origin = phui::js::Cast<jsStratObject*>(ctx, jsobj);

	if (!origin || !origin->stratobj) return JS_FALSE;

	int32 country = 0;

	if (argc > 0) JS_ValueToInt32(ctx, argv[0], &country);

	FULL_OBJ_ID cp = *origin->stratobj->FullObjID();
	UINT32 brigadeID = HCUnit::GetArmyGarrison(cp); // TODO lookup brigade/garrison based on CP
	if (brigadeID == 0)
	{
		return JS_FALSE;
	}

	BrigadeControlRequest req;
	req.requestBrigadeCountryChange(
				PlayerClient()->ChatConnection(),
				brigadeID,
				cp,		// CP to change
				(WWII_COUNTRY)country);				// target country

	return JS_TRUE;
}

JSBool jsStratObject::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	if(!stratobj) return JS_TRUE;

	switch(propid)
	{
		case PARENT_PROP:
			if(!parent.valid() && stratobj->ParentStratObject())
				parent = new jsStratObject(stratobj->ParentStratObject()->FullObjID());

			*val = parent.valid() ? parent->jsVal() : JSVAL_NULL;
		break;
		case CHILDREN_PROP:
			if(!children.valid())
			{
				children = new phui::js::Array;

				StratDB::StratObject::ChildList childlist = stratobj->ChildStratObjects();
				StratDB::StratObject::ChildList::iterator it = childlist.begin();
				for(unsigned int i = 0; it != childlist.end(); ++it, i++)
					children->SetElement(new jsStratObject((*it)->FullObjID()), i);
			}
			*val = children->jsVal();
		break;
		case BRIGADES_PROP:
		{
			if(!brigades.valid())
				brigades = new phui::js::Array;

			brigades->clear();

			const StratDB::StratObject::UnitList& units = stratobj->GetUnits();
			StratDB::StratObject::UnitList::const_iterator it = units.begin();

			for(unsigned int i = 0; it != units.end(); ++it, i++)
				brigades->SetElement(new jsBrigade(*it), i);

			*val = brigades->jsVal();
		}
		break;
		case COUNTRY_PROP:
			if(!country.valid())
				country = new jsCountry((WWII_COUNTRY)stratobj->Country());
			*val = country->jsVal();
		break;
		case POSITION_PROP:
			if(!position.valid())
				position = new jsPosition((float)stratobj->PosX(), (float)stratobj->PosY());
			*val = position->jsVal();
		break;
		case BBOX_PROP:
			if(!bbox.valid())
				bbox = new jsBoundingBox(stratobj);
			*val = bbox->jsVal();
		break;
		case NAME_PROP:
			if(!name.jsVal())
				name.set(_jsctx, phui::js::NewString(stratobj->DisplayName()));
			*val = name.jsVal();
		break;
		case ASCII_NAME:
			if(!asciiname.jsVal())
			{
				const char* stratname = stratobj->Name();
				const UINT32 maxsize = 128;
				char buffer[maxsize];

				strncpy(buffer, stratname, maxsize);

				for(UINT32 i = 0; i < maxsize; ++i)
				{
					// we only check for o and u becauses we know the strat names don't use other characters
					switch((UINT8)buffer[i])
					{
						case 252: // ü
							buffer[i] = 'u';
						break;
						case 246: // ö
							buffer[i] = 'o';
						break;
					}
				}
				asciiname.set(_jsctx, phui::js::NewString(buffer));
			}
			*val = asciiname.jsVal();
		break;
		case CANATTACK_PROP:
		{
			bool canAttack = false;

			// need to attack's on brigades
			if(stratobj->StratClass() == SDM_CLASS_CP)
			{
				WWII_SIDE mySide = wwiiSide(PlayerClient()->PersonaSelected().m_countryID);

				if(stratobj->StratType() == SDM_CP_BRIDGE)
				{
					if((mySide == ALLIED && !(stratobj->StratFlags(true) & CP_ALLIED_OBJECTIVES)) ||
					   (mySide == AXIS && !(stratobj->StratFlags(true) & CP_AXIS_OBJECTIVES)))
						canAttack = true;
				}
				else
				{
					WWII_SIDE cpSide = wwiiSide(stratobj->Country());

					if((stratobj->StratFlags(true) & (CP_STAT_ENEMY_LINKED|CP_STAT_CONTENTION)) &&
					  !(stratobj->StratFlags(true) & CP_OBJECTIVES) &&
					    mySide != cpSide)
					{
						canAttack = true;
					}
				}
			}

			*val = BOOLEAN_TO_JSVAL(canAttack);
		}
		break;
		case CANDEFEND_PROP:
		{
			bool canDefend = false;

			if(stratobj->StratClass() == SDM_CLASS_CP && stratobj->StratType() == SDM_CP_BRIDGE)
			{
				WWII_SIDE mySide = wwiiSide(PlayerClient()->PersonaSelected().m_countryID);

				if((mySide == ALLIED && !(stratobj->StratFlags(true) & CP_ALLIED_OBJECTIVES)) ||
					(mySide == AXIS && !(stratobj->StratFlags(true) & CP_AXIS_OBJECTIVES)))
					canDefend = true;
			}

			*val = BOOLEAN_TO_JSVAL(canDefend);
		}
		break;
		case CANCLEAR_PROP:
		{
			bool canClear = false;

			if(stratobj->StratClass() == SDM_CLASS_CP)
			{
				WWII_SIDE mySide = wwiiSide(PlayerClient()->PersonaSelected().m_countryID);

				if((mySide == ALLIED && stratobj->StratFlags(true) & CP_ALLIED_OBJECTIVES) ||
				   (mySide == AXIS && stratobj->StratFlags(true) & CP_AXIS_OBJECTIVES))
					canClear = true;
			}

			*val = BOOLEAN_TO_JSVAL(canClear);
		}
		break;
		case SIDE_PROP:
		{
			*val = phui::js::ConstString((stratobj->Side() == AXIS) ? "axis" : "allies");
		}
		break;
		case CONTESTED_PROP:
		{
			StratDB::StratObject const* cp = stratobj;

			while(cp && cp->StratClass() != SDM_CLASS_CP)
				cp = cp->ParentStratObject();

			if(cp && (cp->StratFlags(true) & CP_STAT_CONTENTION))
				*val = BOOLEAN_TO_JSVAL(true);
			else
				*val = BOOLEAN_TO_JSVAL(false);
		}
		break;
		case HASAO_PROP:
		{
			StratDB::StratObject const* cp = stratobj;

			while(cp && cp->StratClass() != SDM_CLASS_CP)
				cp = cp->ParentStratObject();

			if(cp && (cp->StratFlags(true) & CP_OBJECTIVES))
				*val = BOOLEAN_TO_JSVAL(true);
			else
				*val = BOOLEAN_TO_JSVAL(false);
		}
		break;
		//case CANCHANGECOUNTRY_PROP:
		//{
		//	bool canChangeCountry = false;

		//	// need to attack's on brigades
		//	if (stratobj->StratClass() == SDM_CLASS_CP)
		//	{
		//		
		//		if (stratobj->Side() == wwiiSide(PlayerClient()->PersonaSelected().m_countryID))
		//		{
		//			if (stratobj->Side() == ALLIED)
		//			{

		//			}
		//		}
		//	
		//	}
		//}
		//break;
	}
	return JS_TRUE;
}

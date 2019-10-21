#include <syscfg.h> // Path is project specific.

#include <osg/Math>

#include <string>
#include <map>

#include "libxml/tree.h"
#include "libxml/parser.h"

#include "fio/fio.h"
#include "weap/weapType.h"
#include "osg/osgModelMgr.h"

static std::map<std::string, weaponType*> _weaponTypes;
static std::map<std::string, weapAmmoType*> _ammoTypes;

/************************************************************************************************
	weapType
*************************************************************************************************/

static void weaponParseWeaponNode(xmlNode* weaponNode)
{
	weaponType* weapType = new weaponType;
	memset(weapType, 0x0, sizeof(weaponType));

	xmlChar* xmlstr;

	xmlstr = xmlGetProp(weaponNode, (xmlChar*)"id");
	if(xmlstr)
	{
		strncpy((char*)weapType->Name, (char*)xmlstr, 31);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(weaponNode, (xmlChar*)"mass");
	if(xmlstr)
	{
		weapType->Mass = (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(weaponNode, (xmlChar*)"firetype");
	if(xmlstr)
	{
		if(strcmp("singleshot", (char*)xmlstr) == 0)
			weapType->FireType = WEAP_FIRETYPE_SINGLESHOT;
		else if(strcmp("syncauto", (char*)xmlstr) == 0)
			weapType->FireType = WEAP_FIRETYPE_AUTO_SYNC;
		else if(strcmp("auto", (char*)xmlstr) == 0)
			weapType->FireType = WEAP_FIRETYPE_AUTO_NORMAL;
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(weaponNode, (xmlChar*)"rateoffire");
	if(xmlstr)
	{
		weapType->RateOfFire = 60.0f / (float)atof((char*)xmlstr) ;
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(weaponNode, (xmlChar*)"ammotype1");
	if(xmlstr)
	{
		weapType->AmmoType[0] = weapAmmoTypeFindByName((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(weaponNode, (xmlChar*)"ammotype2");
	if(xmlstr)
	{
		weapType->AmmoType[1] = weapAmmoTypeFindByName((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(weaponNode, (xmlChar*)"ammotype3");
	if(xmlstr)
	{
		weapType->AmmoType[2] = weapAmmoTypeFindByName((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(weaponNode, (xmlChar*)"ammotype4");
	if(xmlstr)
	{
		weapType->AmmoType[3] = weapAmmoTypeFindByName((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(weaponNode, (xmlChar*)"ammotype5");
	if(xmlstr)
	{
		weapType->AmmoType[4] = weapAmmoTypeFindByName((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(weaponNode, (xmlChar*)"ammotype6");
	if(xmlstr)
	{
		weapType->AmmoType[5] = weapAmmoTypeFindByName((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(weaponNode, (xmlChar*)"ammotype7");
	if(xmlstr)
	{
		weapType->AmmoType[6] = weapAmmoTypeFindByName((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(weaponNode, (xmlChar*)"ammotype8");
	if(xmlstr)
	{
		weapType->AmmoType[7] = weapAmmoTypeFindByName((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(weaponNode, (xmlChar*)"dispersion");
	if(xmlstr)
	{
		weapType->Dispersion = (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(weaponNode, (xmlChar*)"ghostrof");
	if(xmlstr)
	{
		weapType->GhostRateOfFire =  60.0f /  (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(weaponNode, (xmlChar*)"ghostammotype");
	if(xmlstr)
	{
		weapType->GhostAmmoType = weapAmmoTypeFindByName((char*)xmlstr);;
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(weaponNode, (xmlChar*)"hideonlastdrop");
	if(xmlstr)
	{
		weapType->HideOnLastDrop = atoi((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(weaponNode, (xmlChar*)"cliprounds");
	if(xmlstr)
	{
		weapType->NumRoundsInClip = atoi((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(weaponNode, (xmlChar*)"cliptype");
	if(xmlstr)
	{
		// defaults to clip (0)
		if(strcmp("na", (char*)xmlstr) == 0)
			weapType->ClipType = WEAPAMMO_CLIPTYPE_NA;
		else if(strcmp("clip", (char*)xmlstr) == 0)
			weapType->ClipType = WEAPAMMO_CLIPTYPE_CLIP;
		else if(strcmp("magazine", (char*)xmlstr) == 0)
			weapType->ClipType = WEAPAMMO_CLIPTYPE_MAGAZINE;
		else if(strcmp("belt", (char*)xmlstr) == 0)
			weapType->ClipType = WEAPAMMO_CLIPTYPE_BELT;
		else if(strcmp("reload", (char*)xmlstr) == 0)
			weapType->ClipType = WEAPAMMO_CLIPTYPE_RELOAD;
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(weaponNode, (xmlChar*)"reloadtime");
	if(xmlstr)
	{
		weapType->ReloadTime = (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(weaponNode, (xmlChar*)"isplaced");
	if(xmlstr)
	{
		weapType->IsPlacedWeapon = atoi((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(weaponNode, (xmlChar*)"nomuzzlesmoke");
	if(xmlstr)
	{
		weapType->NoMuzzleSmoke = atoi((char*)xmlstr);
		xmlFree(xmlstr);
	}
	// shock dust needs to be in weapon file or moved to a field in mount

	_weaponTypes[(char*)weapType->Name] = weapType;
}

static void weaponParseAmmoNode(xmlNode* ammoNode)
{
	weapAmmoType* ammoType = new weapAmmoType;
	memset(ammoType, 0x0, sizeof(weapAmmoType));

	xmlChar* xmlstr;

	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"id");
	if(xmlstr)
	{
		strncpy((char*)ammoType->Name, (char*)xmlstr, 31);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"type");
	if(xmlstr)
	{
		if(strcmp("ballistic", (char*)xmlstr) == 0)
			ammoType->SubType = WEAPAMMO_SUBTYPE_BALLISTIC;
		else if(strcmp("grenade", (char*)xmlstr) == 0)
			ammoType->SubType = WEAPAMMO_SUBTYPE_GRENADE;
		else if(strcmp("airburst", (char*)xmlstr) == 0)
			ammoType->SubType = WEAPAMMO_SUBTYPE_AIRBURST;
		else if(strcmp("bomb", (char*)xmlstr) == 0)
			ammoType->SubType = WEAPAMMO_SUBTYPE_BOMB;
		else if(strcmp("torpedo", (char*)xmlstr) == 0)
			ammoType->SubType = WEAPAMMO_SUBTYPE_TORPEDO;
		else if(strcmp("rocket", (char*)xmlstr) == 0)
			ammoType->SubType = WEAPAMMO_SUBTYPE_ROCKET;
		else if(strcmp("placed", (char*)xmlstr) == 0)
			ammoType->SubType = WEAPAMMO_SUBTYPE_PLACED;
		else if(strcmp("nuke", (char*)xmlstr) == 0)
			ammoType->SubType = WEAPAMMO_SUBTYPE_NUKE;
		else if(strcmp("knife", (char*)xmlstr) == 0)
			ammoType->SubType = WEAPAMMO_SUBTYPE_KNIFE;
		else if(strcmp("mortar", (char*)xmlstr) == 0)
			ammoType->SubType = WEAPAMMO_SUBTYPE_MORTAR;

		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"diameter");
	if(xmlstr)
	{
		ammoType->Diameter = (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"weight");
	if(xmlstr)
	{
		ammoType->ProjectileWeight = (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"ammoweight");
	if(xmlstr)
	{
		ammoType->AmmoWeight = (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}

	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"velocity");
	if(xmlstr)
	{
		ammoType->InitVelocity = (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"tracertime");
	if(xmlstr)
	{
		ammoType->TracerTime = (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"object");
	if(xmlstr)
	{
		strncpy(ammoType->ObjectName,(char*)xmlstr,31);
		std::string path = "wo/";
		path += ammoType->ObjectName;
		ammoType->Object = osgGetModel(path.c_str());
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"explosive");
	if(xmlstr)
	{
		ammoType->ExplosiveEnergy = (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"fuzetime");
	if(xmlstr)
	{
		ammoType->FuzeTime = (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"armingtime");
	if(xmlstr)
	{
		ammoType->ArmingTime = (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"bc");
	if(xmlstr)
	{
		ammoType->BallisticCoefficient = (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"lifetime");
	if(xmlstr)
	{
		ammoType->LifeTime = (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"stretchobject");
	if(xmlstr)
	{
		ammoType->StretchObject = atoi((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"dontdisplayfirstframe");
	if(xmlstr)
	{
		ammoType->IgnoreFirstFrame = atoi((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"penetrationconstant");
	if(xmlstr)
	{
		ammoType->PenetrationConstant = (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"shrapnelpenetration");
	if(xmlstr)
	{
		ammoType->ShrapnelPenetrationConstant = (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"ricochetangle");
	if(xmlstr)
	{
		ammoType->RichochetAngle = (float)osg::DegreesToRadians(atof((char*)xmlstr));
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"fuzestartspeedloss");
	if(xmlstr)
	{
		ammoType->FuseStartSpeedLoss = (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"modelspall");
	if(xmlstr)
	{
		ammoType->ModelSpall = atoi((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"numshrapnelpieces");
	if(xmlstr)
	{
		ammoType->NumShrapnelPieces = atoi((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"totalshrapnelmass");
	if(xmlstr)
	{
		ammoType->TotalShrapnelMass = (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"explosionsoundnear");
	if(xmlstr)
	{
		strncpy(ammoType->ExplosionNameNear, (char*)xmlstr, 63);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"explosionsoundfar");
	if(xmlstr)
	{
		strncpy(ammoType->ExplosionNameFar, (char*)xmlstr, 63);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"nearfardistance");
	if(xmlstr)
	{
		ammoType->NearFarSoundDist = (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"selfdestructtime");
	if(xmlstr)
	{
		ammoType->SelfDestructTime = (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"isplaced");
	if(xmlstr)
	{
		ammoType->IsPlaced = atoi((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"grenaderotaterate");
	if(xmlstr)
	{
		ammoType->GrenadeRotateRate = (float)(atof((char*)xmlstr) * 2 * osg::PI);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"shapedchargepenmm");
	if(xmlstr)
	{
		ammoType->ShapedChargePen = (float)(atof((char*)xmlstr) * .001);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"torpedorundepth");
	if(xmlstr)
	{
		ammoType->TorpedoRunDepth = (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"torpedorunspeed");
	if(xmlstr)
	{
		ammoType->TorpedoRunSpeed = (float)atof((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"fuzetype");
	if(xmlstr)
	{
		// fix me
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"hassmoke");
	if(xmlstr)
	{
		ammoType->HasSmoke = atoi((char*)xmlstr);
		xmlFree(xmlstr);
	}
	xmlstr = xmlGetProp(ammoNode, (xmlChar*)"muzzleflash");
	if(xmlstr)
	{
		ammoType->iMuzzleFlashType = atoi((char*)xmlstr);
		xmlFree(xmlstr);
	}
    xmlstr = xmlGetProp(ammoNode, (xmlChar*)"hiteffect");
    if(xmlstr)
    {
        strncpy(ammoType->hiteffect, (char*)xmlstr, 31);
        xmlFree(xmlstr);
    }
/*
	"use_new_he",
	"nhe_diameter",
	"nhe_weight",
	"nhe_min_pen_const",
	"nhe_ric_angle",
	"nhe_energy_mult",
	"nhe_min_pieces",
	"nhe_max_pieces",
	"nhe_max_per_veh",
	"nhe_add_speed",
	"nhe_timeout",
	"nhe_max_mass_mult",
*/
	_ammoTypes[(char*)ammoType->Name] = ammoType;

}

static void weaponLoadWeapons()
{
	int fd = fioOpen("vehicles/weapons.txt");
	if(!fd) return;

	char weapname[64];
	while(fioGetLine(fd, weapname, 255) > 0)
	{
		if(weapname[0] == '#' || weapname[0] == '\0' || weapname[0] == ' ') continue;
		char path[128];
		snprintf(path, 127, "vehicles/weapons/%s.xml", weapname);

		xmlDocPtr doc = xmlReadFile(path, NULL, XML_PARSE_RECOVER);
		if(!doc) continue;

		xmlNodePtr rootnode = xmlDocGetRootElement(doc);
		if(rootnode)
			weaponParseWeaponNode(rootnode);

		xmlFreeDoc(doc);
	}

	fioClose(fd);
}

static void weaponLoadAmmo()
{
	int fd = fioOpen("vehicles/ammo.txt");
	if(!fd) return;

	char ammoname[64];
	while(fioGetLine(fd, ammoname, 255) > 0)
	{
		if(ammoname[0] == '#' || ammoname[0] == '\0' || ammoname[0] == ' ') continue;
		char path[128];
		snprintf(path, 127, "vehicles/ammo/%s.xml", ammoname);

		xmlDocPtr doc = xmlReadFile(path, NULL, XML_PARSE_RECOVER);
		if(!doc) continue;

		xmlNodePtr rootnode = xmlDocGetRootElement(doc);
		if(rootnode)
			weaponParseAmmoNode(rootnode);

		xmlFreeDoc(doc);
	}

	fioClose(fd);
}

weaponType const* weapTypeFindByName(const char* weapname)
{
	std::map<std::string, weaponType*>::iterator it = _weaponTypes.find(weapname);
	if(it == _weaponTypes.end()) return 0;
	return it->second;
}

weapAmmoType const* weapAmmoTypeFindByName(const char* ammoName)
{
	std::map<std::string, weapAmmoType*>::iterator it = _ammoTypes.find(ammoName);
	if(it == _ammoTypes.end()) return 0;
	return it->second;
}

void weapTypeInit()
{
	fioXmlRegisterInputCallbacks();

	weaponLoadAmmo();
	weaponLoadWeapons();
}

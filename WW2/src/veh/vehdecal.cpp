#include "syscfg.h" // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: vehdecal.cpp
// Description: Vehicle decals
//////////////////////////////////////////////////////////////////////
#include "veh/vehdecal.h"
#include "ogrp/ogrptype.h"
#include "pi/pi.h"
#include "ld/ldtex.h"
#include "ld/lddds.h"
#include "ld/ldpix.h"
#include "include/vars.h"
#include "fio/fio.h"
#include "veh/vehmain.h"
#include "weap/weapList.h"
#include "veh/vehdecalLoader.h"
#include "ogl/oglMaterial.h"
#include "client/GameClient.h"
#include "math/SuperFastHash.h"

#if USE_STATESET
#include "ogl/States.h"
#endif

#include <vector>
static std::vector<int> troop_personal_id;

#define PERSONAL_DIVISOR    8       // Number of textures in row and column
#define PERSONAL_UV         0.125f  // 128 / 512 (each texture is 128x128 on a 512x512

#define INDIVIDUAL_DIVISOR 8        // Number of texture in row and column
#define INDIVIDUAL_UV      0.125f   // 128 / 1024 (each texture is 128x128 on a 1024x1024)

#define UNIT_DIVISOR        8       // Number of texture in row and column
#define UNIT_UV             0.125f  // 128 / 1024 (each texture is 128x128 on a 1024x1024)

#define DE_ARMOR_IND_MOD    64      // Number of textures
#define UK_ARMOR_IND_MOD    45      // Number of textures
#define FR_ARMOR_IND_MOD    60      // Number of textures
#define DE_PLANE_IND_MOD    39      // Number of textures
#define FR_PLANE_IND_MOD    26      // Number of textures
#define UK_PLANE_IND_MOD    26      // Number of textures
#define US_ARMOR_IND_MOD    12      // Number of textures
#define US_PLANE_IND_MOD    12      // Number of textures

#define BRIGADE_DECALS_DB "vehicles/brigadedecals.xml"

//static std::map<int, int> _unitMap;
static BrigadeDecalMap _decalMap;

static int personal_id;
static int de_fr_uk_unit1_id;
static int de_fr_uk_unit2_id;
static int de_armor_individual_id;
static int de_plane_individual_id;
static int fr_plane_individual_id;
static int uk_plane_individual_id;
static int fr_armor_individual_id;
static int uk_armor_individual_id;
static int us_armor_individual_id;
static int us_plane_individual_id;

static WEAPON_SPEC _armorcategory;
static WEAPON_SPEC _aircategory;
static WEAPON_SPEC _infcategory;
static WEAPON_SPEC _munitionscategory;
static WEAPON_SPEC _seacategory;

//////////////////////////////////////////////////////////////////////
// Function: vehdecalInit
// Description:
//////////////////////////////////////////////////////////////////////
int vehdecalInit()
{
	de_armor_individual_id = ldddsLoadDDSTexture("vehicles/decals/de_armor_individual.dds", GL_TEXTURE_2D, 0);
	fr_armor_individual_id = ldddsLoadDDSTexture("vehicles/decals/fr_armor_individual.dds", GL_TEXTURE_2D, 0);
	uk_armor_individual_id = ldddsLoadDDSTexture("vehicles/decals/uk_armor_individual.dds", GL_TEXTURE_2D, 0);
	de_plane_individual_id = ldddsLoadDDSTexture("vehicles/decals/de_plane_individual.dds", GL_TEXTURE_2D, 0);
	fr_plane_individual_id = ldddsLoadDDSTexture("vehicles/decals/fr_plane_individual.dds", GL_TEXTURE_2D, 0);
	uk_plane_individual_id = ldddsLoadDDSTexture("vehicles/decals/uk_plane_individual.dds", GL_TEXTURE_2D, 0);
	us_armor_individual_id = ldddsLoadDDSTexture("vehicles/decals/us_armor_individual.dds", GL_TEXTURE_2D, 0);
	us_plane_individual_id = ldddsLoadDDSTexture("vehicles/decals/us_plane_individual.dds", GL_TEXTURE_2D, 0);

	de_fr_uk_unit1_id = ldddsLoadDDSTexture("vehicles/decals/de_fr_uk_unit1.dds", GL_TEXTURE_2D, 0);
	de_fr_uk_unit2_id = ldddsLoadDDSTexture("vehicles/decals/de_fr_uk_unit2.dds", GL_TEXTURE_2D, 0);

	personal_id = ldddsLoadDDSTexture("vehicles/decals/personal.dds", GL_TEXTURE_2D, 0);

	// Granny stinks! Hopefully we can use the above someday for granny troopers.
	int count = 1;

	while(1)
	{
		char buff[64];
		strprintf(buff, "vehicles/decals/personal/%d.dds", count++);
		int id = ldddsLoadDDSTexture(buff, GL_TEXTURE_2D, 0);

		// Load textures old way for troopers.
		if(!piIsTexture(id, 0))
		{
			break;
		}

		troop_personal_id.push_back(id);
	}

	_armorcategory = weapFindCategory("GV_");
	_aircategory = weapFindCategory("AC_");
	_infcategory = weapFindCategory("IN_");
	_munitionscategory = weapFindCategory("MN_");
	_seacategory = weapFindCategory("SV_");

	// load in the brigade decal specs
	BrigadeDecalLoader loader(BRIGADE_DECALS_DB);

	if(!loader.Load(&_decalMap))
	{
		WriteLog(LOG_OLD, LOG_INFO, "%s** FAILED** to load", BRIGADE_DECALS_DB);
		return 1;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Function: vehdecalCreateUnitMap
// Description:
//////////////////////////////////////////////////////////////////////
void vehdecalCreateUnitMap(HCUnit* unit)
{
#if(0)
	static int deArmorUnitID = 0;
	static int deAirUnitID = 0;
	static int frArmorUnitID = 0;
	static int frAirUnitID = 0;
	static int ukArmorUnitID = 0;
	static int ukAirUnitID = 0;

	HCUNIT_INDEX::iterator itr = hcUnits.begin();

	UINT32 id = unit->unitID();
/*
	-------------
	CLEANUP later!
	-------------

	FILE* fp = fopen("brigade.txt", "a+");

	if(unit->branch() == BRANCH_SEA)
	{
		return;
	}

	fprintf(fp, "%s", "<brigade id=");
	fprintf(fp, "\"%d\"", id);
	fprintf(fp, " name=\"%s\"", unit->shortName());
	fprintf(fp, " branch=\"%s\"", unit->branch() == BRANCH_AIR ? "air" : unit->branch() == BRANCH_LAND ? "ground" : unit->branch() == BRANCH_SEA ? "sea" : "unknown");
	fprintf(fp, " country=\"%s\"", unit->country() == WWII_ENGLAND ? "uk" : unit->country() == WWII_FRANCE ? "fr" : unit->country() == WWII_GERMANY ? "de" : "unknown");
	fprintf(fp, "%s", ">\n");
	fprintf(fp, "\t");
	fprintf(fp, "%s", "<decal category=\"");
	fprintf(fp, "%s", unit->movementType() == BRANCH_LAND ? "ground" : unit->movementType() == BRANCH_AIR ? "air" : unit->movementType() == BRANCH_SEA ? "sea" : "unknown");
	fprintf(fp, "\"");
	fprintf(fp, " texture=\"");
	fprintf(fp, "%s", unit->movementType() == BRANCH_LAND ? "de_fr_uk_armor_unit.dds" : "de_fr_uk_plane_unit.dds");
	fprintf(fp, "\"");
	fprintf(fp, " row=\"0\"");
	fprintf(fp, " col=\"0\"");
	fprintf(fp, "/>\n");

	if(unit->branch() != unit->movementType())
	{
		fprintf(fp, "\t");
		fprintf(fp, "%s", "<decal category=\"");
		fprintf(fp, "%s", unit->branch() == BRANCH_LAND ? "ground" : unit->branch() == BRANCH_AIR ? "air" : unit->branch() == BRANCH_SEA ? "sea" : "unknown");
		fprintf(fp, "\"");
		fprintf(fp, " texture=\"");
		fprintf(fp, "%s", unit->branch() == BRANCH_LAND ? "de_fr_uk_armor_unit.dds" : "de_fr_uk_plane_unit.dds");
		fprintf(fp, "\"");
		fprintf(fp, " row=\"0\"");
		fprintf(fp, " col=\"0\"");
		fprintf(fp, "/>\n");
	}

	fprintf(fp, "%s", "</brigade>\n");

	fclose(fp);
*/
	// Determine which brigade icon to display
	switch(unit->movementType())
	{
	case BRANCH_LAND:
		{
			switch(unit->country())
			{
			case WWII_ENGLAND:
				{
					_unitMap[id] = ukArmorUnitID++;
					break;
				}
			case WWII_USA:
				{
					_unitMap[id] = usArmorUnitID++;
					break;
				}
			case WWII_FRANCE:
				{
					_unitMap[id] = frArmorUnitID++;
					break;
				}
			case WWII_GERMANY:
				{
					_unitMap[id] = deArmorUnitID++;
					break;
				}
			}

			break;
		}
	case BRANCH_AIR:
		{
			switch(unit->country())
			{
			case WWII_ENGLAND:
				{
					_unitMap[id]= ukAirUnitID++;
					break;
				}
			case WWII_USA:
				{
					_unitMap[id]= usAirUnitID++;
					break;
				}
			case WWII_FRANCE:
				{
					_unitMap[id] = frAirUnitID++;
					break;
				}
			case WWII_GERMANY:
				{
					_unitMap[id] = deAirUnitID++;
					break;
				}
			}

			break;
		}
	}
#endif
}

//////////////////////////////////////////////////////////////////////
// Function: vehdecalKill
// Description:
//////////////////////////////////////////////////////////////////////
void vehdecalKill()
{
	if(piIsTexture(de_armor_individual_id, 0))
	{
		piFreeMipTextures(de_armor_individual_id);
	}

	troop_personal_id.clear();
}

//////////////////////////////////////////////////////////////////////
// Function: _remapUV
// Description:
//////////////////////////////////////////////////////////////////////
static void _remapUV(OBJECT* o, int texid, float u, float v, float divisor)
{
	OBJECT* attached = o;

	do
	{
		//Ticket# 3865: Hack for sorting errors with fx (so artists don't have to revisit each decal)
		attached->layer = 1;

		piFreeMipTextures(attached->texid);

		attached->texid = texid;

		if(attached->material)
		{
			if(attached->material->IsPool())
			{
#if USE_STATESET
                attached->material = new RenderStateSet(*attached->material);
                attached->material->SetIsPool(false);
#else
				// Cannot use pool object for this. // Ticket #4774
				attached->material = new oglMaterial(*attached->material);
				attached->material->SetIsPoolMaterial(false);
#endif
			}

#if USE_STATESET
            TextureUnit* tu = dynamic_cast<TextureUnit*>(attached->material->GetRenderState(RS_TEXTURE_UNIT0));
            tu->SetTexture(attached->texid);
#else
			attached->material->AddTexture(0, attached->texid);
#endif
		}

		piAddTexRef(texid);

		piTexCoordPtr tex = attached->MutableTexCoords();

		// Remap to new texture
		for(int r = 0; r < attached->numverts; r++)
		{
			tex[r].u /= divisor;
			tex[r].v /= divisor;

			tex[r].u += u;
			tex[r].v += v;
		}

		attached = attached->attached;
	}
	while(attached);
}

//////////////////////////////////////////////////////////////////////
// Function: _setPersonalTexture
// Description:
//////////////////////////////////////////////////////////////////////
static void _setPersonalTexture(vehVehicle* veh)
{
	OBJECT* lod1;
	OBJECT* lod2;
	OBJECT* lod3;

	OBJECT* o = veh->VehicleObjectData->PlaneObject;

	if(veh->persDecal < 1)
	{
		ogrpFindSubObjectByName(&lod1, (OBJECT*)o, "per");

		if(lod1)
			obmainRemoveObjectTree(lod1);

		ogrpFindSubObjectByName(&lod2, (OBJECT*)o, "per_1");

		if(lod2)
			obmainRemoveObjectTree(lod2);

		ogrpFindSubObjectByName(&lod3, (OBJECT*)o, "per_2");

		if(lod3)
			obmainRemoveObjectTree(lod3);

		return;
	}

	ogrpFindSubObjectByName(&lod1, (OBJECT*)o, "per");

	if(!lod1)
	{
		return;
	}

	int row = (veh->persDecal - 1) / PERSONAL_DIVISOR;
	int col = (veh->persDecal - 1) % PERSONAL_DIVISOR;

	float u = (float)col * PERSONAL_UV;
	float v = (float)row * PERSONAL_UV;

	piTexCoordPtr tex = lod1->MutableTexCoords();

	piFreeMipTextures(lod1->texid);
	lod1->texid = personal_id;

	if(lod1->material)
	{
		if(lod1->material->IsPool())
		{
#if USE_STATESET
            lod1->material = new RenderStateSet(*lod1->material);
            lod1->material->SetIsPool(false);
#else
			// Cannot use pool object for this. // Ticket #4774
			lod1->material = new oglMaterial(*lod1->material);
			lod1->material->SetIsPoolMaterial(false);
#endif
		}

#if USE_STATESET
        TextureUnit* tu = dynamic_cast<TextureUnit*>(lod1->material->GetRenderState(RS_TEXTURE_UNIT0));
        tu->SetTexture(lod1->texid);
#else
		lod1->material->AddTexture(0, lod1->texid);
#endif
	}

	piAddTexRef(personal_id);

	// Remap to new texture
	for(int r = 0; r < lod1->numverts; r++)
	{
		tex[r].u /= (float)PERSONAL_DIVISOR;
		tex[r].v /= (float)PERSONAL_DIVISOR;

		tex[r].u += u;
		tex[r].v += v;
	}

	ogrpFindSubObjectByName(&lod2, (OBJECT*)o, "per_1");

	if(!lod2)
	{
		return;
	}

	tex = lod2->MutableTexCoords();

	piFreeMipTextures(lod2->texid);
	lod2->texid = personal_id;

	if(lod2->material)
	{
		if(lod2->material->IsPool())
		{
#if USE_STATESET
			// Cannot use pool object for this. // Ticket #4774
			lod2->material = new RenderStateSet(*lod2->material);
			lod2->material->SetIsPool(false);
#else
			// Cannot use pool object for this. // Ticket #4774
			lod2->material = new oglMaterial(*lod2->material);
			lod2->material->SetIsPoolMaterial(false);
#endif
		}

#if USE_STATESET
        TextureUnit* tu = dynamic_cast<TextureUnit*>(lod2->material->GetRenderState(RS_TEXTURE_UNIT0));
        tu->SetTexture(lod2->texid);
#else
		lod2->material->AddTexture(0, lod2->texid);
#endif
	}

	piAddTexRef(personal_id);

	// Remap to new texture
	for(int r = 0; r < lod2->numverts; r++)
	{
		tex[r].u /= (float)PERSONAL_DIVISOR;
		tex[r].v /= (float)PERSONAL_DIVISOR;

		tex[r].u += u;
		tex[r].v += v;
	}

	ogrpFindSubObjectByName(&lod3, (OBJECT*)o, "per_2");

	if(!lod3)
	{
		return;
	}

	tex = lod3->MutableTexCoords();

	piFreeMipTextures(lod3->texid);
	lod3->texid = personal_id;

	if(lod3->material)
	{
		if(lod3->material->IsPool())
		{
#if USE_STATESET
			// Cannot use pool object for this. // Ticket #4774
			lod3->material = new RenderStateSet(*lod3->material);
			lod3->material->SetIsPool(false);
#else
			// Cannot use pool object for this. // Ticket #4774
			lod3->material = new oglMaterial(*lod3->material);
			lod3->material->SetIsPoolMaterial(false);
#endif
		}

#if USE_STATESET
        TextureUnit* tu = dynamic_cast<TextureUnit*>(lod3->material->GetRenderState(RS_TEXTURE_UNIT0));
        tu->SetTexture(lod3->texid);
#else
		lod3->material->AddTexture(0, lod3->texid);
#endif
	}

	piAddTexRef(personal_id);

	// Remap to new texture
	for(int r = 0; r < lod3->numverts; r++)
	{
		tex[r].u /= (float)PERSONAL_DIVISOR;
		tex[r].v /= (float)PERSONAL_DIVISOR;

		tex[r].u += u;
		tex[r].v += v;
	}
}

//////////////////////////////////////////////////////////////////////
// Function: _setSquadTexture
// Description: TESTING ONLY
//////////////////////////////////////////////////////////////////////
#if(0)
static void _setSquadTexture(vehVehicle* veh)
{
	OBJECT* lod1;
	OBJECT* lod2;
	OBJECT* lod3;

	OBJECT* o = veh->VehicleObjectData->PlaneObject;

	if(veh->persDecal < 1)
	{
		ogrpFindSubObjectByName(&lod1, (OBJECT*)o, "squad");

		if(lod1)
			obmainRemoveObjectTree(lod1);

		ogrpFindSubObjectByName(&lod2, (OBJECT*)o, "squad_1");

		if(lod2)
			obmainRemoveObjectTree(lod2);

		ogrpFindSubObjectByName(&lod3, (OBJECT*)o, "squad_2");

		if(lod3)
			obmainRemoveObjectTree(lod3);

		return;
	}

	// For testing only
	int texid = 8;

	ogrpFindSubObjectByName(&lod1, (OBJECT*)o, "squad");

	if(!lod1)
	{
		return;
	}

	// USE personal texture for testing only
	int row = texid / PERSONAL_DIVISOR;
	int col = texid % PERSONAL_DIVISOR;

	float u = (float)col * PERSONAL_UV;
	float v = (float)row * PERSONAL_UV;

	piTexCoordPtr tex = lod1->MutableTexCoords();

	piFreeMipTextures(lod1->texid);
	lod1->texid = personal_id;

	if(lod1->material)
	{
		if(lod1->material->IsPool())
		{
#if USE_STATESET
            lod1->material = new RenderStateSet(*lod1->material);
            lod1->material->SetIsPool(false);
#else
			// Cannot use pool object for this. // Ticket #4774
			lod1->material = new oglMaterial(*lod1->material);
			lod1->material->SetIsPoolMaterial(false);
#endif
		}

#if USE_STATESET
        TextureUnit* tu = dynamic_cast<TextureUnit*>(lod1->material->GetRenderState(RS_TEXTURE_UNIT0));
        tu->SetTexture(lod1->texid);
#else
		lod1->material->AddTexture(0, lod1->texid);
#endif
	}

	piAddTexRef(personal_id);

	// Remap to new texture
	for(int r = 0; r < lod1->numverts; r++)
	{
		tex[r].u /= (float)PERSONAL_DIVISOR;
		tex[r].v /= (float)PERSONAL_DIVISOR;

		tex[r].u += u;
		tex[r].v += v;
	}

	ogrpFindSubObjectByName(&lod2, (OBJECT*)o, "squad_1");

	if(!lod2)
	{
		return;
	}

	tex = lod2->MutableTexCoords();

	piFreeMipTextures(lod2->texid);
	lod2->texid = personal_id;

	if(lod2->material)
	{
		if(lod2->material->IsPool())
		{
#if USE_STATESET
            lod2->material = new RenderStateSet(*lod2->material);
            lod2->material->SetIsPool(false);
#else
			// Cannot use pool object for this. // Ticket #4774
			lod2->material = new oglMaterial(*lod2->material);
			lod2->material->SetIsPoolMaterial(false);
#endif
		}

#if USE_STATESET
        TextureUnit* tu = dynamic_cast<TextureUnit*>(lod2->material->GetRenderState(RS_TEXTURE_UNIT0));
        tu->SetTexture(lod2->texid);
#else
		lod2->material->AddTexture(0, lod2->texid);
#endif
	}

	piAddTexRef(personal_id);

	// Remap to new texture
	for(int r = 0; r < lod2->numverts; r++)
	{
		tex[r].u /= (float)PERSONAL_DIVISOR;
		tex[r].v /= (float)PERSONAL_DIVISOR;

		tex[r].u += u;
		tex[r].v += v;
	}

	ogrpFindSubObjectByName(&lod3, (OBJECT*)o, "squad_2");

	if(!lod3)
	{
		return;
	}

	tex = lod3->MutableTexCoords();

	piFreeMipTextures(lod3->texid);
	lod3->texid = personal_id;

	if(lod3->material)
	{
		if(lod3->material->IsPool())
		{
#if USE_STATESET
            lod3->material = new RenderStateSet(*lod3->material);
            lod3->material->SetIsPool(false);
#else
			// Cannot use pool object for this. // Ticket #4774
			lod3->material = new oglMaterial(*lod3->material);
			lod3->material->SetIsPoolMaterial(false);
#endif
		}

#if USE_STATESET
        TextureUnit* tu = dynamic_cast<TextureUnit*>(lod3->material->GetRenderState(RS_TEXTURE_UNIT0));
        tu->SetTexture(lod3->texid);
#else
		lod3->material->AddTexture(0, lod3->texid);
#endif
	}

	piAddTexRef(personal_id);

	// Remap to new texture
	for(int r = 0; r < lod3->numverts; r++)
	{
		tex[r].u /= (float)PERSONAL_DIVISOR;
		tex[r].v /= (float)PERSONAL_DIVISOR;

		tex[r].u += u;
		tex[r].v += v;
	}
}
#endif

//////////////////////////////////////////////////////////////////////
// Function: _setIndividualTexture
// Description:
//////////////////////////////////////////////////////////////////////
static void _setIndividualTexture(vehVehicle* veh)
{
	int texid = -1;
	int modvalue = 64;

	const WEAPON_SPEC& ws = veh->VehicleType->WeaponSpec;
	OBJECT* o = veh->VehicleObjectData->PlaneObject;

	switch(ws.country())
	{
	case WWII_ENGLAND:
		{
			// Armor
			if(ws.wCategory == _armorcategory.wCategory)
			{
				texid = uk_armor_individual_id;
				modvalue = UK_ARMOR_IND_MOD;
			}
			// Air
			else if(ws.wCategory == _aircategory.wCategory)
			{
				texid = uk_plane_individual_id;
				modvalue = UK_PLANE_IND_MOD;
			}

			// TODO: complete with new textures

			break;
		}
	case WWII_FRANCE:
		{
			// Armor
			if(ws.wCategory == _armorcategory.wCategory)
			{
				texid = fr_armor_individual_id;
				modvalue = FR_ARMOR_IND_MOD;
			}
			// Air
			else if(ws.wCategory == _aircategory.wCategory)
			{
				texid = fr_plane_individual_id;
				modvalue = FR_PLANE_IND_MOD;
			}

			// TODO: complete with new textures

			break;
		}
	case WWII_GERMANY:
		{
			// Armor
			if(ws.wCategory == _armorcategory.wCategory)
			{
				texid = de_armor_individual_id;
				modvalue = DE_ARMOR_IND_MOD;
			}
			// Air
			else if(ws.wCategory == _aircategory.wCategory)
			{
				texid = de_plane_individual_id;
				modvalue = DE_PLANE_IND_MOD;
			}

			// TODO: complete with new textures

			break;
		}
	case WWII_USA:
		{
#if US_INDIVIDUAL_DECALS_ENABLED // ticket 5370 (US does not have these decals)
			// Armor
			if(ws.wCategory == _armorcategory.wCategory)
			{
				texid = us_armor_individual_id;
				modvalue = US_ARMOR_IND_MOD;
			}
			// Air
			else if(ws.wCategory == _aircategory.wCategory)
			{
				texid = us_plane_individual_id;
				modvalue = US_PLANE_IND_MOD;
			}
#endif
			// TODO: complete with new textures
            break;
		}
	case WWII_ITALY:
	case WWII_CHINA:
	case WWII_RUSSIA:
		{
			break;
		}
    default:
        // no other currently valid countries
        break;
	}

    if(!oglIsTexture(texid, 0))
        return;

	OBJECT* lod1;
	OBJECT* lod2;
	OBJECT* lod3;

	ogrpFindSubObjectByName(&lod1, o, "ind");

	if(!lod1)
	{
		return;
	}

    UINT64 handle = 0;

    // Ticket #6067 Revert to old way. New was using SuperFastHashCode
    for(unsigned int i = 0; i < strlen((char*)veh->Handle); i++)
    {
        handle += (i + 1) * veh->Handle[i] * 1301;
    }

	// modvalue = number of textures
	handle %= modvalue;

	UINT64 col = handle % INDIVIDUAL_DIVISOR; // 8 texture per row/column of 128 resolution
	UINT64 row = handle / INDIVIDUAL_DIVISOR;

	float u = (float)col * INDIVIDUAL_UV;
	float v = (float)row * INDIVIDUAL_UV;

	if(lod1)
	{
		_remapUV(lod1, texid, u, v, INDIVIDUAL_DIVISOR);
	}

	ogrpFindSubObjectByName(&lod2, o, "ind_1");

	if(lod2)
	{
		_remapUV(lod2, texid, u, v, INDIVIDUAL_DIVISOR);
	}

	ogrpFindSubObjectByName(&lod3, o, "ind_2");

	if(lod3)
	{
		_remapUV(lod3, texid, u, v, INDIVIDUAL_DIVISOR);
	}
}

//////////////////////////////////////////////////////////////////////
// Function: RemoveUnitObjects
// Description:
//////////////////////////////////////////////////////////////////////
static void RemoveUnitObjects(OBJECT* o)
{
    OBJECT* lod = nullptr;

    ogrpFindSubObjectByName(&lod, (OBJECT*)o, "unit");

    if(lod)
	    obmainRemoveObjectTree(lod);

    ogrpFindSubObjectByName(&lod, (OBJECT*)o, "unit_1");

    if(lod)
	    obmainRemoveObjectTree(lod);

    ogrpFindSubObjectByName(&lod, (OBJECT*)o, "unit_2");

    if(lod)
	    obmainRemoveObjectTree(lod);
}

//////////////////////////////////////////////////////////////////////
// Function: _setUnitTexture
// Description:
//////////////////////////////////////////////////////////////////////
#include "wwii/wwiiHCUnits.h"
static void _setUnitTexture(vehVehicle* veh)
{
	// row / column for brigades specified in vehicles/brigadedecals.xml

	int texid = -1;

	OBJECT* lod1;
	OBJECT* lod2;
	OBJECT* lod3;

	OBJECT* o = veh->VehicleObjectData->PlaneObject;

    const HCUnit* hc = nullptr;

    if(Online)
        hc = HCUnit::Lookup((UINT32)veh->unitDecal);

    if(!hc)
    {
        if(Online)
        {
            RemoveUnitObjects(o);
            return;
        }
    }

    BrigadeDecalMap::iterator itr;

    if(!hc)
        itr = _decalMap.begin();
    else
        itr = _decalMap.find(hc->m_callsign);

	if(itr == _decalMap.end())
	{
        RemoveUnitObjects(o);
		return;
	}

	ogrpFindSubObjectByName(&lod1, o, "unit");
	ogrpFindSubObjectByName(&lod2, o, "unit_1");
	ogrpFindSubObjectByName(&lod3, o, "unit_2");

	BRIGADE_DECAL_SPEC spec = itr->second;
	int col = -1;
	int row = -1;

	for(unsigned int i = 0; i < spec.decals.size(); i++)
	{
		if(veh->VehicleType->WeaponSpec.wCategory == spec.decals[i].category)
		{
			texid = spec.decals[i].texture;

			row = spec.decals[i].row;
			col = spec.decals[i].col;

			break;
		}
	}

	if(!piIsTexture(texid, 0))
	{
		return;
	}

	float u = (float)col * UNIT_UV;
	float v = (float)row * UNIT_UV;

	if(lod1)
	{
		_remapUV(lod1, texid, u, v, UNIT_DIVISOR);
	}

	if(lod2)
	{
		_remapUV(lod2, texid, u, v, UNIT_DIVISOR);
	}

	if(lod3)
	{
		_remapUV(lod3, texid, u, v, UNIT_DIVISOR);
	}
}

//////////////////////////////////////////////////////////////////////
// Function: vehdecalGetUnitTextureForUI
// Description:
//////////////////////////////////////////////////////////////////////
bool vehdecalGetUnitTextureForUI(HCUnit* unit, int* texid, float* u1, float* v1, float* u2, float* v2)
{
    BrigadeDecalMap::iterator itr = _decalMap.find(unit->callsign());

	if(itr == _decalMap.end())
		return false; // no map found

	int category = 0;

	// determine which category to use
	switch(unit->branch())
	{
		case BRANCH_LAND:
			category = 2;
			break;
		case BRANCH_AIR:
			category = 1;
			break;
		case BRANCH_SEA:
			category = 3;
			break;
		default:
			// no good match
			break;
	}

	if(category)
	{
		BRIGADE_DECAL_SPEC spec = itr->second;

		for(unsigned int i = 0; i < spec.decals.size(); i++)
		{
			if(category == spec.decals[i].category)
			{
				*texid	= spec.decals[i].texture;
				*u1		= (float)spec.decals[i].col * UNIT_UV;
				*v1		= (float)spec.decals[i].row * UNIT_UV;
				*u2		= *u1 + UNIT_UV;
				*v2		= *v1 + UNIT_UV;

				return true;
			}
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Function: vehdecalSetDecals
// Description:
//////////////////////////////////////////////////////////////////////
void vehdecalSetDecals(vehVehicle* vehicle)
{
#ifdef BETA_RELEASE
	// For testing offline only
	if(!Online)
	{
		if(gPersDecalOverride)
			vehicle->persDecal = gPersDecalOverride;

		if(gUnitDecalOverride)
			vehicle->unitDecal = gUnitDecalOverride;
	}
#endif

	// Individual textures based on handle and vehicle
	_setIndividualTexture(vehicle);

	// Unit textures based on brigade
	_setUnitTexture(vehicle);

	// Personal textures based on permissions. Host sets.
	_setPersonalTexture(vehicle);

#if(0)
	// Squad textures based on whatever...
	_setSquadTexture(vehicle);
#endif
}

//////////////////////////////////////////////////////////////////////
// Function: vehdecalGetTrooperPersonalDecal
// Description:
//////////////////////////////////////////////////////////////////////
int vehdecalGetTrooperPersonalDecal(unsigned int decal)
{
	// Old way to handle this. This will change whenever we can figure out a way to
	// access granny texture coordinate data in memory. Their documentation is nothing
	// but a circuitous bunch of crap.
	if(decal - 1 >= troop_personal_id.size())
	{
		return -1;
	}

	return troop_personal_id[decal - 1];
}

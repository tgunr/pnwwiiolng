#pragma once

#include "xml/XMLLoader2.h"

struct DECAL_SPEC
{
	int category;
	int texture;
	int row;
	int col;

	bool IsValid()
	{
		return (category > 0 && category < 6 &&
				texture >= 0 &&
				row >= 0 &&
				col >= 0);
	}
};

struct BRIGADE_DECAL_SPEC
{
	std::string shortname;
	std::vector<DECAL_SPEC> decals;
};

typedef std::map<std::string, BRIGADE_DECAL_SPEC> BrigadeDecalMap;

class BrigadeDecalLoader : public XMLLoader2<BrigadeDecalLoader>
{
public:

	BrigadeDecalLoader(const char* path) : XMLLoader2<BrigadeDecalLoader>(path)
	{
		AddHandler("brigade", &BrigadeDecalLoader::DoBrigade, this);
		AddHandler("decal", &BrigadeDecalLoader::DoDecal, this);
	}

	virtual ~BrigadeDecalLoader()
	{
	}

	// tag handlers
	XML_TAG_HANDLER(DoBrigade)
	{
		if(first && userData)
		{
			BrigadeDecalMap* decalMap = (BrigadeDecalMap*)userData;
			BRIGADE_DECAL_SPEC spec;

			spec.shortname.clear();

            const char* p = NULL;
			xmlGetNodeValue(node, XML_DATA_STRING, &p, "id");
            spec.shortname = p;

			// only add the unit if it has a unit id and we found some decals for it
			if(!spec.shortname.empty() && ProcessChildren(node, &spec) && spec.decals.size())
				(*decalMap)[spec.shortname] = spec;
		}

		return false;
	}

	XML_TAG_HANDLER(DoDecal)
	{
		if(first && userData)
		{
			// passed in from DoBrigade
			BRIGADE_DECAL_SPEC* spec = (BRIGADE_DECAL_SPEC*)userData;

			// new decal
			DECAL_SPEC decal;

			// set some defaults
			decal.category	= -1;
			decal.texture	= -1;
			decal.row		= -1;
			decal.col		= -1;

			// grab the string data
			std::string category	= xmlGetStringFromNode(node, "category");
			std::string texture		= "vehicles/decals/";

			texture += xmlGetStringFromNode(node, "texture");

			// grab the row and col
			xmlGetNodeValue(node, XML_DATA_SINT, &decal.row, "row");
			xmlGetNodeValue(node, XML_DATA_SINT, &decal.col, "col");

			// determine the category
			if(!stricmp(category.c_str(), "ground"))
            {
                decal.category = weapFindCategory("gv_").wCategory;
            }
			else if(!stricmp(category.c_str(), "air"))
            {
                decal.category = weapFindCategory("ac_").wCategory;
            }
			else if(!stricmp(category.c_str(), "sea"))
            {
                decal.category = weapFindCategory("sv_").wCategory;
            }
			else if(!stricmp(category.c_str(), "troop"))
            {
                decal.category = weapFindCategory("in_").wCategory;
            }

			// lookup the texture id
			decal.texture = ldddsLoadDDSTexture(texture.c_str(), GL_TEXTURE_2D, 0);
			// only use it if it looks valid
			if(decal.IsValid())
				spec->decals.push_back(decal);
		}

		return true;
	}
};

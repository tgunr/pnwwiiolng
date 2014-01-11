#include <syscfg.h> // Path is project specific.
#include "pi/pi.h"
#include "str/strpool.h"

#include "fp/fpTPSKits.h"
#include "fp/fpFPSdata.h"
#include "fp/fpTPSdata.h"
#include "fp/fpTPS.h"
#include "fio/fio.h"
#include "sys/sysWinISO.h"
#include "str/stringLib.h"
#include "xml/XMLLoader2.h"
#include "gr2/gr2Anim.h"
#include "spt/Random.h"

#define	KITS_SPEC "vehicles\\ground\\groundman\\kits.xml"

typedef std::vector<std::string> KitNameList;
typedef std::pair<std::string, UINT32> KitModelLOD;
typedef std::vector<KitModelLOD> KitModelLODList;

// lowest level; visual model
struct KitModel
{
	UINT32 attachToBone;
	KitModelLODList lods;
};

typedef std::map<std::string,KitModel> KitModelMap;
typedef std::vector<KitModel*> KitModelPtrList;

// logical grouping of visual models for selection; possibly random
struct KitObject
{
	KitModelPtrList models;
};

typedef std::map<std::string,KitObject> KitObjectMap;
typedef std::vector<KitObject*> KitObjectPtrList;

// highest level object
// represents a kit spec and is used to populate a gr2AnimTrooper
// with kit items
struct Kit
{
	KitObjectPtrList objects;
};

typedef std::map<std::string,Kit> KitMap;
typedef std::vector<Kit*> KitPtrList;

// represents a vehicle in kit terms...by vehicle id
struct KitTrooper
{
	KitPtrList kits;
};

typedef std::map<std::string,KitTrooper> KitTrooperMap;

struct Kits
{
	KitModelMap models;
	KitObjectMap objects;
	KitMap kits;
	KitTrooperMap troopers;

	void Reset();

	KitModel* FindModel(const char* name);
	KitObject* FindObject(const char* name);
	Kit* FindKit(const char* name);
	KitTrooper* FindTrooper(const char* id);
};

// this is our kit spec loading class
class KitLoader : public XMLLoader2<KitLoader>
{
public:

	KitLoader(const char* path);
	virtual ~KitLoader(){}

	// tag handlers
	XML_TAG_HANDLER(DoModels);
	XML_TAG_HANDLER(DoModel);
	XML_TAG_HANDLER(DoLOD);
	XML_TAG_HANDLER(DoObject);
	XML_TAG_HANDLER(DoKit);
	XML_TAG_HANDLER(DoTrooper);

protected:

	int _lod;
};

static Kits _kits;

static void _applyKit(gr2AnimTrooper* anim, Kit* kit)
{
	if(anim && kit)
	{
		for(size_t k = 0; k < kit->objects.size(); ++k)
		{
			KitObject* object = kit->objects.at(k);

			if(object->models.empty())
				continue;

			UINT32 modelCount	= object->models.size();
			KitModel* model		= object->models.at(GetRandom(0, modelCount - 1));

			// need to make it so lods can be used
			KitModelLOD& lod	= model->lods.at(0);

			// load the attachment
			gr2Model* attachment = gr2Model::Load(lod.first.c_str(), nullptr, nullptr, 1);

			// if we found one, add it
			// true indicates we're letting the skeleton manage the attachment (draw, updated, etc...)
			if(attachment)
			{
				// set our max visible range
				attachment->maxVis = lod.second;

				// add our attachment to our skeleton
				anim->skeleton->AddAttachment(attachment, 0, model->attachToBone, true);
				//attachment->UpdateAttachment();
				//attachment->Update(true);
			}
		}

		anim->kit = kit;
	}
}

Kit* _getRandomKit(KitTrooper* trooper)
{
	if(trooper && !trooper->kits.empty())
		return trooper->kits.at(GetRandom(0, trooper->kits.size() - 1));

	return nullptr;
}

// prototypes
void fpTPSKitsInit(bool force)
{
	static bool init = false;
	if(init && !force) return;

	// if we made it here via a forced init, cleanup resources first
	if(init)
		fpTPSKitsKill();

	// set us to inited
	init = true;

	// load up our spec
	KitLoader loader(KITS_SPEC);

	// read the xml file and load in the kit specs loadouts
	loader.Load();
}

void fpTPSKitsKill()
{
	_kits.Reset();
}

// clears any kit related attachments
void fpTPSKitsReset(gr2AnimTrooper* anim)
{
	if(anim && anim->kit)
	{
		anim->kit = nullptr;
		anim->skeleton->FreeAttachments();
	}
}

// sets a trooper to a certain kit
// will defailt to the first kit for the trooper's vehicle id
void fpTPSKitsSetByVehicle(gr2AnimTrooper* anim, SINT32 index)
{
	if(anim && anim->myVehicle && anim->myVehicle->VehicleType)
	{
		KitTrooper* spec = _kits.FindTrooper((const char*)anim->myVehicle->VehicleType->Id);
		Kit* kit = nullptr;

		if(spec)
		{
			UINT32 kitCount = spec->kits.size();

			if(index >= 0 && index < (int)kitCount)
			{
				kit = spec->kits.at(index);
			}
			else if(kitCount == 1)
			{
				kit = spec->kits.at(0);
			}
			else
			{
				kit = _getRandomKit(spec);
			}
		}
		else
		{
			kit = _kits.FindKit((const char*)anim->myVehicle->VehicleType->Id);
		}

		_applyKit(anim, kit);
	}
}

void fpTPSKitsSetByName(gr2AnimTrooper* anim, const char* kitName)
{
	_applyKit(anim, _kits.FindKit(kitName));
}

KitLoader::KitLoader(const char* path) : XMLLoader2<KitLoader>(path), _lod(0)
{
	AddHandler("models", &KitLoader::DoModels, this);
	AddHandler("model", &KitLoader::DoModel, this);
	AddHandler("lod", &KitLoader::DoLOD, this);
	AddHandler("object", &KitLoader::DoObject, this);
	AddHandler("kit", &KitLoader::DoKit, this);
	AddHandler("trooper", &KitLoader::DoTrooper, this);
}

bool KitLoader::DoModels(xmlNode* node, bool first, void* userData)
{
	if(first)
		xmlGetNodeValue(node, XML_DATA_UINT, &_lod, "lod");

	return true;
}

bool KitLoader::DoModel(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		// this is a model lookup
		if(userData)
		{
			KitObject* object		= (KitObject*)userData;
			const char* modelName	= xmlGetStringFromNode(node, "name");

			if(modelName != nullptr)
			{
				KitModel* model = _kits.FindModel(modelName);

				if(model)
					object->models.push_back(model);
			}
		}
		else
		{
			KitModel model;

			model.attachToBone		= 0;
			std::string name		= xmlGetStringFromNode(node, "name");
			const char* path		= xmlGetStringFromNode(node, "path");

			// cache this kit object model
			gr2Model::Load(path, nullptr, nullptr, 0);

			xmlGetNodeValue(node, XML_DATA_UINT, &model.attachToBone, "attach");

			if(path != nullptr)
			{
				UINT32 distance = _lod;
				xmlGetNodeValue(node, XML_DATA_UINT, &distance, "lod");
				model.lods.push_back(std::make_pair(path, distance));
			}

			ProcessChildren(node, &model);

			// save our model spec
			_kits.models[name] = model;
		}
	}

	return false;
}

bool KitLoader::DoLOD(xmlNode* node, bool first, void* userData)
{
	if(first && userData)
	{
		KitModel* model = (KitModel*)userData;

		const char* path = xmlGetStringFromNode(node, "path");

		if(path != nullptr)
		{
			UINT32 distance = _lod;
			xmlGetNodeValue(node, XML_DATA_UINT, &distance, "lod");
			model->lods.push_back(std::make_pair(path, distance));
		}
	}

	return false;
}

bool KitLoader::DoObject(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		// this is a model lookup
		if(userData)
		{
			Kit* kit				= (Kit*)userData;
			const char* objectName	= xmlGetStringFromNode(node, "name");

			if(objectName != nullptr)
			{
				KitObject* object = _kits.FindObject(objectName);

				if(object)
					kit->objects.push_back(object);
			}
		}
		else
		{
			KitObject object;

			std::string name		= xmlGetStringFromNode(node, "name");
			const char* modelName	= xmlGetStringFromNode(node, "model");

			if(modelName != nullptr)
			{
				KitModel* model = _kits.FindModel(modelName);

				if(model)
					object.models.push_back(model);
			}

			ProcessChildren(node, &object);

			// save our model spec
			_kits.objects[name] = object;
		}
	}

	return false;
}

bool KitLoader::DoKit(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		// this is a model lookup
		if(userData)
		{
			KitTrooper* trooper		= (KitTrooper*)userData;
			const char* kitName		= xmlGetStringFromNode(node, "name");

			if(kitName != nullptr)
			{
				Kit* kit = _kits.FindKit(kitName);

				if(kit)
					trooper->kits.push_back(kit);
			}
		}
		else
		{
			Kit kit;

			std::string name		= xmlGetStringFromNode(node, "name");
			const char* objectName	= xmlGetStringFromNode(node, "object");

			if(objectName != nullptr)
			{
				KitObject* object = _kits.FindObject(objectName);

				if(object)
					kit.objects.push_back(object);
			}

			ProcessChildren(node, &kit);

			// save our model spec
			_kits.kits[name] = kit;
		}
	}

	return false;
}

bool KitLoader::DoTrooper(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		// this is a model lookup
		KitTrooper trooper;

		std::string id		= xmlGetStringFromNode(node, "id");
		const char* kitName	= xmlGetStringFromNode(node, "kit");

		if(kitName != nullptr)
		{
			Kit* kit = _kits.FindKit(kitName);

			if(kit)
				trooper.kits.push_back(kit);
		}

		ProcessChildren(node, &trooper);

		// save our model spec
		_kits.troopers[id] = trooper;
	}

	return false;
}

void Kits::Reset()
{
	models.clear();
	objects.clear();
	kits.clear();
	troopers.clear();
}

KitModel* Kits::FindModel(const char* name)
{
	KitModelMap::iterator found = models.find(name);
	return (found != models.end()) ? &found->second : nullptr;
}

KitObject* Kits::FindObject(const char* name)
{
	KitObjectMap::iterator found = objects.find(name);
	return (found != objects.end()) ? &found->second : nullptr;
}

Kit* Kits::FindKit(const char* name)
{
	KitMap::iterator found = kits.find(name);
	return (found != kits.end()) ? &found->second : nullptr;
}

KitTrooper* Kits::FindTrooper(const char* id)
{
	KitTrooperMap::iterator found = troopers.find(id);
	return (found != troopers.end()) ? &found->second : nullptr;
}

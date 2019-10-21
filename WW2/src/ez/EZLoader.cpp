#include "syscfg.h" // Path is project specific.
#include "pi/pi.h"
#include "str/strpool.h"

#include "ez/EZLoader.h"
#include "ez/EZManager.h"
#include "veh/vehcntrl.h"
#include "imap/ww2func.h"

static const char* _ezBaseFile(const vehType* vtype)
{
	if(!vtype)
		return "";

	/* from the game database
	1 | bomber        | 2007-09-24 19:35:34 | 2007-09-24 19:35:34 |
	2 | fighter       | 2007-09-24 19:35:34 | 2007-09-24 19:35:34 |
	3 | gm            | 2007-09-24 19:35:34 | 2007-09-24 19:35:34 |
	4 | tank          | 2007-09-24 19:35:34 | 2007-09-24 19:35:34 |
	5 | artillery     | 2007-09-24 19:35:34 | 2007-09-24 19:35:34 |
	6 | truck         | 2007-09-24 19:35:34 | 2007-09-24 19:35:34 |
	7 | gun           | 2007-09-24 19:35:34 | 2007-09-24 19:35:34 |
	8 | crew          | 2007-09-24 19:35:34 | 2007-09-24 19:35:34 |
	10 | airBomb       | 2007-09-24 19:35:34 | 2007-09-24 19:35:34 |
	11 | heavyInfantry | 2007-09-24 19:35:34 | 2007-09-24 19:35:34 |
	12 | riverine      | 2007-09-24 19:35:34 | 2007-09-24 19:35:34 |
	13 | sea           | 2007-09-24 19:35:34 | 2007-09-24 19:35:34 |
	9 | troop         | 2007-09-24 19:35:34 | 2007-09-24 19:35:34 |
	14 | para          | 2007-11-06 15:02:23 | 2007-11-06 00:00:00 |
	*/

	switch(vtype->WeaponSpec.wClass)
	{
		case 1: // bomber
		case 2: // fighter
			return "vehicles/ez/plane.ez";
		break;
		case 4: // tank
			return "vehicles/ez/tank.ez";
		break;
		case 5: // artillery
		case 7: // gun
			return "vehicles/ez/gun.ez";
		break;
		case 6: // truck
			return "vehicles/ez/truck.ez";
		break;
		case 9: // troops
		case 14: // paras
			return "vehicles/ez/trooper.ez";
		break;
		case 12: // riverine
		case 13: // ship
			return "vehicles/ez/boat.ez";
		break;
	}

	return "";
}

EZLoader::EZLoader(const char* path) :
	XMLLoader2<EZLoader>(path),
	currentLevel(1),
	maxLevelLoaded(0),
	rootLoader(nullptr)
{
	AddHandler("vehicle", &EZLoader::DoVehicle, this);
	AddHandler("include", &EZLoader::DoInclude, this);
	AddHandler("ez", &EZLoader::DoEZ, this);
	AddHandler("param", &EZLoader::DoConfig, this);
	AddHandler("control", &EZLoader::DoControl, this);
	AddHandler("position", &EZLoader::DoPosition, this);
	AddHandler("view", &EZLoader::DoView, this);
	AddHandler("camera", &EZLoader::DoCamera, this);
	AddHandler("traverse", &EZLoader::DoTraverse, this);
	AddHandler("elevation", &EZLoader::DoElevation, this);
	AddHandler("blocks", &EZLoader::DoBlocks, this);
	AddHandler("turret", &EZLoader::DoTurret, this);

	// default us to being the root loader
	rootLoader = (EZLoader*)this;
}

bool EZLoader::DoVehicle(xmlNode* node, bool first, void* userData)
{
	if(!EZ_MGR)
		return false;

	if(first)
	{
		// if another load was passed in, make it the root loader
		if(userData)
		{
			rootLoader = (EZLoader*)userData;
		}
		else
		{
			// otherwise load this stuff
			const char* reticle = xmlGetStringFromNode(node, "reticle");
			if(reticle) EZ_MGR->SetReticle(reticle);

			SINT32 mainTurret = -1;
			if(xmlGetNodeValue(node, XML_DATA_SINT, &mainTurret, "turret"))
				EZ_MGR->SetTurret(mainTurret);
		}
	}

	return true;
}

bool EZLoader::DoInclude(xmlNode* node, bool first, void* userData)
{
	if(first && EZ_MGR)
	{
		std::string path = xmlGetStringFromNode(node, "path");

		if(!path.empty())
		{
			EZLoader loader(path.c_str());
			loader.Load(this);
		}
	}

	return true;
}

bool EZLoader::DoEZ(xmlNode* node, bool first, void* userData)
{
	if(first && EZ_MGR)
	{
		rootLoader->currentLevel = 1;

		xmlGetNodeValue(node, XML_DATA_SINT, &rootLoader->currentLevel, "level");

		if(rootLoader->currentLevel > rootLoader->maxLevelLoaded)
			rootLoader->maxLevelLoaded = rootLoader->currentLevel;

		// init this level; could overwrite this level if a previous
		// include had populated this ez level but thats ok and expected
		// functionality
		rootLoader->levels[rootLoader->currentLevel] = EZLoader::EZLevel();

		// now we see if this level is doubling as a position
		SINT32 position = -1;

		if(xmlGetNodeValue(node, XML_DATA_SINT, &position, "position"))
			DoPosition(node, true, nullptr);
	}

	return true;
}

bool EZLoader::DoConfig(xmlNode* node, bool first, void* userData)
{
	if(first && EZ_MGR)
	{
		std::string name = xmlGetStringFromNode(node, "name");
		std::string value = xmlGetStringFromNode(node, "value");

		if(!rootLoader->levels.empty())
		{
			rootLoader->levels[rootLoader->currentLevel].config.push_back(std::make_pair(name, value));
		}
		else
		{
			EZ_MGR->SetConfig(name.c_str(), value.c_str());
		}
	}

	return true;
}

bool EZLoader::DoControl(xmlNode* node, bool first, void* userData)
{
	if(first && EZ_MGR)
	{
		SINT32 position = 0;

		if(xmlGetNodeValue(node, XML_DATA_SINT, &position, "position"))
		{
			std::string name = xmlGetStringFromNode(node, "name");

			if(!name.empty())
				rootLoader->levels[rootLoader->currentLevel].controls.push_back(std::make_pair(position, name));
		}
	}

	return true;
}

bool EZLoader::DoPosition(xmlNode* node, bool first, void* userData)
{
	if(first && EZ_MGR)
	{
		EZManager::EZPosition position;

		position.name = "command";

		const char* name = xmlGetStringFromNode(node, "name");
		if(name) position.name = name;

		const char* reticle	= xmlGetStringFromNode(node, "reticle");
		if(reticle) position.reticle = reticle;

		// see if the position node defined any view data
		DoView(node, true, &position);

		if(!position.views.empty())
		{
			// get our current view
			EZManager::EZView& view = position.GetView();

			// create a new turret for this position
			EZManager::EZTurret turret;

			// set our turret position index
			turret.position = view.position;

			// make it the first turret
			view.turrets.push_back(turret);
		}

		// other turrets might get added here and more than one
		// game position is synced to this ez position
		ProcessChildren(node, &position);

		// must have at least 1 view for this to be a valid position
		if(!position.views.empty())
		{
			if(rootLoader->currentLevel > rootLoader->maxLevelLoaded)
				rootLoader->maxLevelLoaded = rootLoader->currentLevel;

			// save our ez position
			rootLoader->levels[rootLoader->currentLevel].positions.push_back(position);
		}
	}

	return false;
}

bool EZLoader::DoView(xmlNode* node, bool first, void* userData)
{
	if(first && EZ_MGR && userData)
	{
		EZManager::EZPosition* position = (EZManager::EZPosition*)userData;
		EZManager::EZView view;

		view.position	= 0;
		view.view 		= EZManager::VIEW_EXTERNAL;
		view.zoom 		= 0;
		view.hatch		= EZManager::EZView::EVENT_NONE;
		view.deploy		= EZManager::EZView::EVENT_NONE;

		if(xmlGetNodeValue(node, XML_DATA_SINT, &view.position, "position"))
		{
			if(xmlGetNodeValue(node, XML_DATA_SINT, &view.view, "view"))
				view.view--;

			xmlGetNodeValue(node, XML_DATA_SINT, &view.zoom, "zoom");

			const char* hatch = xmlGetStringFromNode(node, "hatch");

			if(strcasecmp(hatch, "onspawn") == 0)
				view.hatch = EZManager::EZView::EVENT_ONSPAWN;
			else if(strcasecmp(hatch, "onload") == 0)
				view.hatch = EZManager::EZView::EVENT_ONLOAD;

			const char* deploy = xmlGetStringFromNode(node, "deploy");

			if(strcasecmp(deploy, "onspawn") == 0)
				view.deploy = EZManager::EZView::EVENT_ONSPAWN;
			else if(strcasecmp(deploy, "onload") == 0)
				view.deploy = EZManager::EZView::EVENT_ONLOAD;

			// command views automatically block these things
			if(view.view < 0)
			{
				view.blocks[IMAP_ASX_TURRET_TRAVERSE] = TRUE;
				view.blocks[IMAP_ASX_TURRET_ELEVATE] = TRUE;
			}

			position->views.push_back(view);
		}
	}

	return true;
}

bool EZLoader::DoCamera(xmlNode* node, bool first, void* userData)
{
	if(first && EZ_MGR && userData)
	{
		EZManager::EZPosition* position = (EZManager::EZPosition*)userData;

		// grab our last view; we'll attach the camera to this view
		EZManager::EZView& view = position->views.at(position->views.size() - 1);

		FLOAT32 offsetX		= 0;
		FLOAT32 offsetY		= 0;
		FLOAT32 offsetZ		= 0;

		xmlGetNodeValue(node, XML_DATA_DOUBLE, &view.camera.focus, "focus");
		xmlGetNodeValue(node, XML_DATA_BOOL, &view.camera.pov, "pov");

		const char* offset = xmlGetStringFromNode(node, "offset");

		if(offset && sscanf(offset, "%f,%f,%f", &offsetX, &offsetY, &offsetZ) == 3)
		{
			// so camera orbiting works correctly
			if(!view.camera.pov)
				offsetY = -offsetY;

			view.camera.offset.set((FLOAT64)offsetX, (FLOAT64)offsetY, (FLOAT64)offsetZ);
		}

		ProcessChildren(node, &view.camera);
	}

	return false;
}

bool EZLoader::DoTraverse(xmlNode* node, bool first, void* userData)
{
	if(first && EZ_MGR && userData)
	{
		EZManager::EZCamera* camera = (EZManager::EZCamera*)userData;

		xmlGetNodeValue(node, XML_DATA_DOUBLE, &camera->traverse.rate, "rate");
		xmlGetNodeValue(node, XML_DATA_DOUBLE, &camera->traverse.minAngle, "left");
		xmlGetNodeValue(node, XML_DATA_DOUBLE, &camera->traverse.maxAngle, "right");
	}

	return false;
}

bool EZLoader::DoElevation(xmlNode* node, bool first, void* userData)
{
	if(first && EZ_MGR && userData)
	{
		EZManager::EZCamera* camera = (EZManager::EZCamera*)userData;

		xmlGetNodeValue(node, XML_DATA_DOUBLE, &camera->elevation.rate, "rate");
		xmlGetNodeValue(node, XML_DATA_DOUBLE, &camera->elevation.minAngle, "down");
		xmlGetNodeValue(node, XML_DATA_DOUBLE, &camera->elevation.maxAngle, "up");
	}

	return false;
}

bool EZLoader::DoBlocks(xmlNode* node, bool first, void* userData)
{
	if(first && EZ_MGR && userData)
	{
		EZManager::EZPosition* position = (EZManager::EZPosition*)userData;

		const char* input = xmlGetStringFromNode(node, "input");

		if(input && !position->views.empty())
		{
			SINT32 id = vehcntrlGetFunctionIndex((SCHAR*)input);

			if(id >= 0)
			{
				// grab our last view
				EZManager::EZView& view = position->views.at(position->views.size() - 1);

				// add this block to the last view
				view.blocks[id] = TRUE;
			}
		}
	}

	return false;
}

bool EZLoader::DoTurret(xmlNode* node, bool first, void* userData)
{
	if(first && EZ_MGR && userData)
	{
		EZManager::EZPosition* position = (EZManager::EZPosition*)userData;
		EZManager::EZTurret turret;

		if(!position->views.empty() && xmlGetNodeValue(node, XML_DATA_SINT, &turret.position, "position"))
		{
			// grab our last view
			EZManager::EZView& view = position->views.at(position->views.size() - 1);

			// add this turret to the last view
			view.turrets.push_back(turret);
		}
	}

	return false;
}

// partially loads the ez file for a vehicle to determine which manager to use for its control
class EZManagerType : public XMLLoader2<EZManagerType>
{
public:

	EZManagerType(const vehType* vtype) : XMLLoader2<EZManagerType>("")
	{
		AddHandler("vehicle", &EZManagerType::DoVehicle, this);

        std::string modelPath = std::string("vehicles/") + reinterpret_cast<const char *>(&vtype->ModelDirectory[0]);
        std::string::size_type sep = modelPath.find_last_of("/");

        ezPath = modelPath + "/" + modelPath.substr(sep + 1) + ".ez"; // HACK: npos + 1 == 0
        ezFallbackPath = _ezBaseFile(vtype);
        SetPath(ezPath.c_str());
	}

	virtual bool Load(void *userData = nullptr)
	{
        if (!XMLLoader2::Load(userData)) {
            // Fallback on failure instead of using the dangerous access(2) call
            ezPath = ezFallbackPath;
    		SetPath(ezPath.c_str());
    		return XMLLoader2::Load(userData);
    	}
    	return true;
	}

	virtual ~EZManagerType(){}

	// tag handlers
	bool DoVehicle(xmlNode* node, bool first, void* userData)
	{
		if(first)
		{
			const char* mtype = xmlGetStringFromNode(node, "manager");
			if(mtype) ezManager = mtype;
		}

		return false;
	}

	// file we used to determine the manager from
	std::string ezPath;
	std::string ezFallbackPath;
	std::string ezManager;
};

bool ezManagerType(const vehType* vtype, std::string& manager, std::string& path)
{
	if(vtype)
	{
		EZManagerType typeLoader(vtype);
		typeLoader.Load(&manager);

		if(!typeLoader.ezManager.empty())
		{
			manager = typeLoader.ezManager;
			path = typeLoader.ezPath;

			return true;
		}
	}

	return false;
}

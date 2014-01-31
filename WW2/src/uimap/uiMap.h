#pragma once

#include "phui/View.h"
#include "phui/DOM.h"
#include "phui/UIXML.h"

class SuperCellMapView;

enum MAP_PLAYER_EVENT
{
	MAP_PLAYER_EVENT_INVALID = 0,
	MAP_PLAYER_EVENT_EMOTE,
	MAP_PLAYER_EVENT_DEPLOY,
	MAP_PLAYER_EVENT_UNDEPLOY,
	MAP_PLAYER_EVENT_AMMO,
	MAP_PLAYER_EVENT_NUM_TYPES
};

/************************************************************************************************
	uiMap
*************************************************************************************************/

phui::View* uiMapCreate(_xmlNode*, phui::View* parent);
//phui::View* uiMapIconCreate(_xmlNode*, phui::View* parent);
phui::DOM::ElementDOM* uiMapCreateDOM(const char* classname, phui::View* view, phui::DOM::ElementDOM* parent);

// tolua_begin
UINT32 uiMapToggleIngameMap(SINT32 keyState,  UINT32 functionNum, UINT32 data);
void uiMapHideIngameMap(bool close=false);
bool uiMapIngameMapUp();
void uiMapLock(bool l);

void uiMapActivateWaypoint(const char* name);
void uiMapAddWaypoint(const char* name, int type, int subtype, int x, int y, const char* notes);
void uiMapRemoveWaypoint(const char* name);
void uiMapClearWaypoints();
void uiMapAddLayer(SuperCellMapView* mapview, _xmlNode* node);
namespace StratDB { class StratObject; };
void uiMapStratUpdate(StratDB::StratObject* strat, int country = -1, int vislevel = -1);

SuperCellMapView* uiMapFind(const char* name);
// tolua_end
void uiMapSetMissionOriginAndTarget(FULL_OBJ_ID const& origin, FULL_OBJ_ID const& target);

UINT32 uiMapScaleAllMaps(SINT32 keyState,  UINT32 functionNum, UINT32 data);
void uiMapProcessPlayerEventToAllMaps(ChatHandle handle, MAP_PLAYER_EVENT event, SINT32 data = -1, SINT32 expires = 0);
void uiMapProcessPlayerEventToMap(const char* mapname, ChatHandle handle, MAP_PLAYER_EVENT event, SINT32 data = -1, SINT32 expires = 0);
void uiMapClearPlayerEvents();
void uiMapSaveMinimapScale();

//bool uiMapGetTrnobjTextureData(const char* name, int* texid, int* x, int* y, int* width, int* height, float uv[4]);
bool uiMapGetSdmData(int scx, int scy, struct sdm_dtl_data* dtl, struct sdm_replacement_octet_data* replace);


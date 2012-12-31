#pragma once

#include "phui/View.h"
#include "phui/DOM.h"
#include "phui/UIXML.h"

class SuperCellMapView;
struct FULL_OBJ_ID;

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

extern phui::View* uiMapCreate(_xmlNode*, phui::View* parent);
//phui::View* uiMapIconCreate(_xmlNode*, phui::View* parent);
extern phui::DOM::ElementDOM* uiMapCreateDOM(const char* classname, phui::View* view, phui::DOM::ElementDOM* parent);

// tolua_begin
extern UINT32 uiMapToggleIngameMap(SINT32 keyState,  UINT32 functionNum, UINT32 data);
extern void uiMapHideIngameMap(bool close=false);
extern bool uiMapIngameMapUp();
extern void uiMapLock(bool l);

extern void uiMapActivateWaypoint(const char* name);
extern void uiMapAddWaypoint(const char* name, int type, int subtype, int x, int y, const char* notes);
extern void uiMapRemoveWaypoint(const char* name);
extern void uiMapClearWaypoints();
extern void uiMapAddLayer(SuperCellMapView* mapview, _xmlNode* node);
namespace StratDB { class StratObject ; } ;
extern void uiMapStratUpdate(StratDB::StratObject* strat, int country = -1, int vislevel = -1);

extern SuperCellMapView* uiMapFind(const char* name);
// tolua_end
extern void uiMapSetMissionOriginAndTarget(FULL_OBJ_ID const& origin, FULL_OBJ_ID const& target);

extern UINT32 uiMapScaleAllMaps(SINT32 keyState,  UINT32 functionNum, UINT32 data);
extern void uiMapProcessPlayerEventToAllMaps(ChatHandle handle, MAP_PLAYER_EVENT event, SINT32 data = -1, SINT32 expires = 0);
extern void uiMapProcessPlayerEventToMap(const char* mapname, ChatHandle handle, MAP_PLAYER_EVENT event, SINT32 data = -1, SINT32 expires = 0);
extern void uiMapClearPlayerEvents();
extern void uiMapSaveMinimapScale();

//bool uiMapGetTrnobjTextureData(const char* name, int* texid, int* x, int* y, int* width, int* height, float uv[4]);
extern bool uiMapGetSdmData(int scx, int scy, struct sdm_dtl_data* dtl, struct sdm_replacement_octet_data* replace);

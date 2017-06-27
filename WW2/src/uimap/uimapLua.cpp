#include "syscfg.h" // Path is project specific.
/*
** Lua binding: uimap
** Generated automatically by tolua++-1.0.92 on 01/04/14 22:00:09.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_uimap_open (lua_State* tolua_S);

#include "ui/uiMain.h"
#include "uimap/uiMap.h"
#include "uimap/SuperCellMapView.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"phui::View");
 tolua_usertype(tolua_S,"SuperCellMapView");
 tolua_usertype(tolua_S,"_xmlNode");
 tolua_usertype(tolua_S,"StratDB::StratObject");
}

/* function: uiMapToggleIngameMap */
#ifndef TOLUA_DISABLE_tolua_uimap_map_uiMapToggleIngameMap00
static int tolua_uimap_map_uiMapToggleIngameMap00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
   int keyState = ((  int)  tolua_tonumber(tolua_S,1,0));
  unsigned int functionNum = (( unsigned int)  tolua_tonumber(tolua_S,2,0));
  unsigned int data = (( unsigned int)  tolua_tonumber(tolua_S,3,0));
  {
   unsigned int tolua_ret = ( unsigned int)  uiMapToggleIngameMap(keyState,functionNum,data);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiMapToggleIngameMap'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiMapHideIngameMap */
#ifndef TOLUA_DISABLE_tolua_uimap_map_uiMapHideIngameMap00
static int tolua_uimap_map_uiMapHideIngameMap00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isboolean(tolua_S,1,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  bool close = ((bool)  tolua_toboolean(tolua_S,1,false));
  {
   uiMapHideIngameMap(close);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiMapHideIngameMap'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiMapIngameMapUp */
#ifndef TOLUA_DISABLE_tolua_uimap_map_uiMapIngameMapUp00
static int tolua_uimap_map_uiMapIngameMapUp00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isnoobj(tolua_S,1,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  {
   bool tolua_ret = (bool)  uiMapIngameMapUp();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiMapIngameMapUp'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiMapLock */
#ifndef TOLUA_DISABLE_tolua_uimap_map_uiMapLock00
static int tolua_uimap_map_uiMapLock00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isboolean(tolua_S,1,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  bool l = ((bool)  tolua_toboolean(tolua_S,1,0));
  {
   uiMapLock(l);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiMapLock'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiMapActivateWaypoint */
#ifndef TOLUA_DISABLE_tolua_uimap_map_uiMapActivateWaypoint00
static int tolua_uimap_map_uiMapActivateWaypoint00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isstring(tolua_S,1,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const char* name = ((const char*)  tolua_tostring(tolua_S,1,0));
  {
   uiMapActivateWaypoint(name);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiMapActivateWaypoint'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiMapAddWaypoint */
#ifndef TOLUA_DISABLE_tolua_uimap_map_uiMapAddWaypoint00
static int tolua_uimap_map_uiMapAddWaypoint00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isstring(tolua_S,1,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,4,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,5,0,&tolua_err) ||
     !tolua_isstring(tolua_S,6,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,7,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const char* name = ((const char*)  tolua_tostring(tolua_S,1,0));
  int type = ((int)  tolua_tonumber(tolua_S,2,0));
  int subtype = ((int)  tolua_tonumber(tolua_S,3,0));
  int x = ((int)  tolua_tonumber(tolua_S,4,0));
  int y = ((int)  tolua_tonumber(tolua_S,5,0));
  const char* notes = ((const char*)  tolua_tostring(tolua_S,6,0));
  {
   uiMapAddWaypoint(name,type,subtype,x,y,notes);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiMapAddWaypoint'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiMapRemoveWaypoint */
#ifndef TOLUA_DISABLE_tolua_uimap_map_uiMapRemoveWaypoint00
static int tolua_uimap_map_uiMapRemoveWaypoint00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isstring(tolua_S,1,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const char* name = ((const char*)  tolua_tostring(tolua_S,1,0));
  {
   uiMapRemoveWaypoint(name);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiMapRemoveWaypoint'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiMapClearWaypoints */
#ifndef TOLUA_DISABLE_tolua_uimap_map_uiMapClearWaypoints00
static int tolua_uimap_map_uiMapClearWaypoints00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isnoobj(tolua_S,1,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  {
   uiMapClearWaypoints();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiMapClearWaypoints'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiMapAddLayer */
#ifndef TOLUA_DISABLE_tolua_uimap_map_uiMapAddLayer00
static int tolua_uimap_map_uiMapAddLayer00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"SuperCellMapView",0,&tolua_err) ||
     !tolua_isusertype(tolua_S,2,"_xmlNode",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  SuperCellMapView* mapview = ((SuperCellMapView*)  tolua_tousertype(tolua_S,1,0));
  _xmlNode* node = ((_xmlNode*)  tolua_tousertype(tolua_S,2,0));
  {
   uiMapAddLayer(mapview,node);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiMapAddLayer'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiMapStratUpdate */
#ifndef TOLUA_DISABLE_tolua_uimap_map_uiMapStratUpdate00
static int tolua_uimap_map_uiMapStratUpdate00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,1,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* strat = ((StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0));
  int country = ((int)  tolua_tonumber(tolua_S,2,-1));
  int vislevel = ((int)  tolua_tonumber(tolua_S,3,-1));
  {
   uiMapStratUpdate(strat,country,vislevel);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiMapStratUpdate'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiMapFind */
#ifndef TOLUA_DISABLE_tolua_uimap_map_uiMapFind00
static int tolua_uimap_map_uiMapFind00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isstring(tolua_S,1,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const char* name = ((const char*)  tolua_tostring(tolua_S,1,0));
  {
   SuperCellMapView* tolua_ret = (SuperCellMapView*)  uiMapFind(name);
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"SuperCellMapView");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiMapFind'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: KeyDown of class  SuperCellMapView */
#ifndef TOLUA_DISABLE_tolua_uimap_map_SuperCellMapView_KeyDown00
static int tolua_uimap_map_SuperCellMapView_KeyDown00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"SuperCellMapView",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  SuperCellMapView* self = (SuperCellMapView*)  tolua_tousertype(tolua_S,1,0);
  int keycode = ((int)  tolua_tonumber(tolua_S,2,0));
  unsigned short unicode = ((unsigned short)  tolua_tonumber(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'KeyDown'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->KeyDown(keycode,unicode);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'KeyDown'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: KeyUp of class  SuperCellMapView */
#ifndef TOLUA_DISABLE_tolua_uimap_map_SuperCellMapView_KeyUp00
static int tolua_uimap_map_SuperCellMapView_KeyUp00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"SuperCellMapView",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  SuperCellMapView* self = (SuperCellMapView*)  tolua_tousertype(tolua_S,1,0);
  int keycode = ((int)  tolua_tonumber(tolua_S,2,0));
  unsigned short unicode = ((unsigned short)  tolua_tonumber(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'KeyUp'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->KeyUp(keycode,unicode);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'KeyUp'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: MouseDown of class  SuperCellMapView */
#ifndef TOLUA_DISABLE_tolua_uimap_map_SuperCellMapView_MouseDown00
static int tolua_uimap_map_SuperCellMapView_MouseDown00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"SuperCellMapView",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,4,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,5,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  SuperCellMapView* self = (SuperCellMapView*)  tolua_tousertype(tolua_S,1,0);
  float x = ((float)  tolua_tonumber(tolua_S,2,0));
  float y = ((float)  tolua_tonumber(tolua_S,3,0));
  int button = ((int)  tolua_tonumber(tolua_S,4,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'MouseDown'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->MouseDown(x,y,button);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'MouseDown'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: MouseMove of class  SuperCellMapView */
#ifndef TOLUA_DISABLE_tolua_uimap_map_SuperCellMapView_MouseMove00
static int tolua_uimap_map_SuperCellMapView_MouseMove00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"SuperCellMapView",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,4,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,5,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  SuperCellMapView* self = (SuperCellMapView*)  tolua_tousertype(tolua_S,1,0);
  float x = ((float)  tolua_tonumber(tolua_S,2,0));
  float y = ((float)  tolua_tonumber(tolua_S,3,0));
  int state = ((int)  tolua_tonumber(tolua_S,4,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'MouseMove'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->MouseMove(x,y,state);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'MouseMove'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: MouseScroll of class  SuperCellMapView */
#ifndef TOLUA_DISABLE_tolua_uimap_map_SuperCellMapView_MouseScroll00
static int tolua_uimap_map_SuperCellMapView_MouseScroll00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"SuperCellMapView",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,4,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,5,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  SuperCellMapView* self = (SuperCellMapView*)  tolua_tousertype(tolua_S,1,0);
  float x = ((float)  tolua_tonumber(tolua_S,2,0));
  float y = ((float)  tolua_tonumber(tolua_S,3,0));
  int dir = ((int)  tolua_tonumber(tolua_S,4,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'MouseScroll'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->MouseScroll(x,y,dir);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'MouseScroll'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: MouseDrag of class  SuperCellMapView */
#ifndef TOLUA_DISABLE_tolua_uimap_map_SuperCellMapView_MouseDrag00
static int tolua_uimap_map_SuperCellMapView_MouseDrag00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"SuperCellMapView",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,4,0,&tolua_err) ||
     !tolua_isboolean(tolua_S,5,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,6,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  SuperCellMapView* self = (SuperCellMapView*)  tolua_tousertype(tolua_S,1,0);
  float x = ((float)  tolua_tonumber(tolua_S,2,0));
  float y = ((float)  tolua_tonumber(tolua_S,3,0));
  int button = ((int)  tolua_tonumber(tolua_S,4,0));
  bool start = ((bool)  tolua_toboolean(tolua_S,5,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'MouseDrag'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->MouseDrag(x,y,button,start);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'MouseDrag'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetPositionXY of class  SuperCellMapView */
#ifndef TOLUA_DISABLE_tolua_uimap_map_SuperCellMapView_SetPositionXY00
static int tolua_uimap_map_SuperCellMapView_SetPositionXY00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"SuperCellMapView",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isboolean(tolua_S,4,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,5,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  SuperCellMapView* self = (SuperCellMapView*)  tolua_tousertype(tolua_S,1,0);
  float x = ((float)  tolua_tonumber(tolua_S,2,0));
  float y = ((float)  tolua_tonumber(tolua_S,3,0));
  bool recalcPan = ((bool)  tolua_toboolean(tolua_S,4,true));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetPositionXY'",NULL);
#endif
  {
   self->SetPositionXY(x,y,recalcPan);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetPositionXY'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetPositionLatLon of class  SuperCellMapView */
#ifndef TOLUA_DISABLE_tolua_uimap_map_SuperCellMapView_SetPositionLatLon00
static int tolua_uimap_map_SuperCellMapView_SetPositionLatLon00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"SuperCellMapView",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  SuperCellMapView* self = (SuperCellMapView*)  tolua_tousertype(tolua_S,1,0);
  float lat = ((float)  tolua_tonumber(tolua_S,2,0));
  float lon = ((float)  tolua_tonumber(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetPositionLatLon'",NULL);
#endif
  {
   self->SetPositionLatLon(lat,lon);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetPositionLatLon'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetPositionSuperCell of class  SuperCellMapView */
#ifndef TOLUA_DISABLE_tolua_uimap_map_SuperCellMapView_SetPositionSuperCell00
static int tolua_uimap_map_SuperCellMapView_SetPositionSuperCell00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"SuperCellMapView",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  SuperCellMapView* self = (SuperCellMapView*)  tolua_tousertype(tolua_S,1,0);
  float scx = ((float)  tolua_tonumber(tolua_S,2,0));
  float scy = ((float)  tolua_tonumber(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetPositionSuperCell'",NULL);
#endif
  {
   self->SetPositionSuperCell(scx,scy);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetPositionSuperCell'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetScale of class  SuperCellMapView */
#ifndef TOLUA_DISABLE_tolua_uimap_map_SuperCellMapView_SetScale00
static int tolua_uimap_map_SuperCellMapView_SetScale00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"SuperCellMapView",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  SuperCellMapView* self = (SuperCellMapView*)  tolua_tousertype(tolua_S,1,0);
  float numsc = ((float)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetScale'",NULL);
#endif
  {
   self->SetScale(numsc);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetScale'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetPanPositionXY of class  SuperCellMapView */
#ifndef TOLUA_DISABLE_tolua_uimap_map_SuperCellMapView_SetPanPositionXY00
static int tolua_uimap_map_SuperCellMapView_SetPanPositionXY00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"SuperCellMapView",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,4,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,5,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  SuperCellMapView* self = (SuperCellMapView*)  tolua_tousertype(tolua_S,1,0);
  float x = ((float)  tolua_tonumber(tolua_S,2,0));
  float y = ((float)  tolua_tonumber(tolua_S,3,0));
  float s = ((float)  tolua_tonumber(tolua_S,4,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetPanPositionXY'",NULL);
#endif
  {
   self->SetPanPositionXY(x,y,s);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetPanPositionXY'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetAlpha of class  SuperCellMapView */
#ifndef TOLUA_DISABLE_tolua_uimap_map_SuperCellMapView_SetAlpha00
static int tolua_uimap_map_SuperCellMapView_SetAlpha00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"SuperCellMapView",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  SuperCellMapView* self = (SuperCellMapView*)  tolua_tousertype(tolua_S,1,0);
  float a = ((float)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetAlpha'",NULL);
#endif
  {
   self->SetAlpha(a);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetAlpha'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetNavScale of class  SuperCellMapView */
#ifndef TOLUA_DISABLE_tolua_uimap_map_SuperCellMapView_SetNavScale00
static int tolua_uimap_map_SuperCellMapView_SetNavScale00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"SuperCellMapView",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  SuperCellMapView* self = (SuperCellMapView*)  tolua_tousertype(tolua_S,1,0);
  float ns = ((float)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetNavScale'",NULL);
#endif
  {
   self->SetNavScale(ns);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetNavScale'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: MapScale of class  SuperCellMapView */
#ifndef TOLUA_DISABLE_tolua_uimap_map_SuperCellMapView_MapScale00
static int tolua_uimap_map_SuperCellMapView_MapScale00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"SuperCellMapView",0,&tolua_err) ||
     !tolua_isboolean(tolua_S,2,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  SuperCellMapView* self = (SuperCellMapView*)  tolua_tousertype(tolua_S,1,0);
  bool force = ((bool)  tolua_toboolean(tolua_S,2,false));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'MapScale'",NULL);
#endif
  {
   float tolua_ret = (float)  self->MapScale(force);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'MapScale'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: NavScale of class  SuperCellMapView */
#ifndef TOLUA_DISABLE_tolua_uimap_map_SuperCellMapView_NavScale00
static int tolua_uimap_map_SuperCellMapView_NavScale00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const SuperCellMapView",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const SuperCellMapView* self = (const SuperCellMapView*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'NavScale'",NULL);
#endif
  {
   float tolua_ret = (float)  self->NavScale();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'NavScale'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Scale of class  SuperCellMapView */
#ifndef TOLUA_DISABLE_tolua_uimap_map_SuperCellMapView_Scale00
static int tolua_uimap_map_SuperCellMapView_Scale00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const SuperCellMapView",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const SuperCellMapView* self = (const SuperCellMapView*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Scale'",NULL);
#endif
  {
   float tolua_ret = (float)  self->Scale();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Scale'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: PosX of class  SuperCellMapView */
#ifndef TOLUA_DISABLE_tolua_uimap_map_SuperCellMapView_PosX00
static int tolua_uimap_map_SuperCellMapView_PosX00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const SuperCellMapView",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const SuperCellMapView* self = (const SuperCellMapView*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'PosX'",NULL);
#endif
  {
   float tolua_ret = (float)  self->PosX();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'PosX'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: PosY of class  SuperCellMapView */
#ifndef TOLUA_DISABLE_tolua_uimap_map_SuperCellMapView_PosY00
static int tolua_uimap_map_SuperCellMapView_PosY00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const SuperCellMapView",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const SuperCellMapView* self = (const SuperCellMapView*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'PosY'",NULL);
#endif
  {
   float tolua_ret = (float)  self->PosY();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'PosY'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_uimap_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_module(tolua_S,"map",0);
  tolua_beginmodule(tolua_S,"map");
   tolua_function(tolua_S,"uiMapToggleIngameMap",tolua_uimap_map_uiMapToggleIngameMap00);
   tolua_function(tolua_S,"uiMapHideIngameMap",tolua_uimap_map_uiMapHideIngameMap00);
   tolua_function(tolua_S,"uiMapIngameMapUp",tolua_uimap_map_uiMapIngameMapUp00);
   tolua_function(tolua_S,"uiMapLock",tolua_uimap_map_uiMapLock00);
   tolua_function(tolua_S,"uiMapActivateWaypoint",tolua_uimap_map_uiMapActivateWaypoint00);
   tolua_function(tolua_S,"uiMapAddWaypoint",tolua_uimap_map_uiMapAddWaypoint00);
   tolua_function(tolua_S,"uiMapRemoveWaypoint",tolua_uimap_map_uiMapRemoveWaypoint00);
   tolua_function(tolua_S,"uiMapClearWaypoints",tolua_uimap_map_uiMapClearWaypoints00);
   tolua_function(tolua_S,"uiMapAddLayer",tolua_uimap_map_uiMapAddLayer00);
   tolua_module(tolua_S,"StratDB",0);
   tolua_beginmodule(tolua_S,"StratDB");
    tolua_cclass(tolua_S,"StratObject","StratDB::StratObject","",NULL);
    tolua_beginmodule(tolua_S,"StratObject");
    tolua_endmodule(tolua_S);
   tolua_endmodule(tolua_S);
   tolua_function(tolua_S,"uiMapStratUpdate",tolua_uimap_map_uiMapStratUpdate00);
   tolua_function(tolua_S,"uiMapFind",tolua_uimap_map_uiMapFind00);
   tolua_cclass(tolua_S,"SuperCellMapView","SuperCellMapView","phui::View",NULL);
   tolua_beginmodule(tolua_S,"SuperCellMapView");
    tolua_function(tolua_S,"KeyDown",tolua_uimap_map_SuperCellMapView_KeyDown00);
    tolua_function(tolua_S,"KeyUp",tolua_uimap_map_SuperCellMapView_KeyUp00);
    tolua_function(tolua_S,"MouseDown",tolua_uimap_map_SuperCellMapView_MouseDown00);
    tolua_function(tolua_S,"MouseMove",tolua_uimap_map_SuperCellMapView_MouseMove00);
    tolua_function(tolua_S,"MouseScroll",tolua_uimap_map_SuperCellMapView_MouseScroll00);
    tolua_function(tolua_S,"MouseDrag",tolua_uimap_map_SuperCellMapView_MouseDrag00);
    tolua_function(tolua_S,"SetPositionXY",tolua_uimap_map_SuperCellMapView_SetPositionXY00);
    tolua_function(tolua_S,"SetPositionLatLon",tolua_uimap_map_SuperCellMapView_SetPositionLatLon00);
    tolua_function(tolua_S,"SetPositionSuperCell",tolua_uimap_map_SuperCellMapView_SetPositionSuperCell00);
    tolua_function(tolua_S,"SetScale",tolua_uimap_map_SuperCellMapView_SetScale00);
    tolua_function(tolua_S,"SetPanPositionXY",tolua_uimap_map_SuperCellMapView_SetPanPositionXY00);
    tolua_function(tolua_S,"SetAlpha",tolua_uimap_map_SuperCellMapView_SetAlpha00);
    tolua_function(tolua_S,"SetNavScale",tolua_uimap_map_SuperCellMapView_SetNavScale00);
    tolua_function(tolua_S,"MapScale",tolua_uimap_map_SuperCellMapView_MapScale00);
    tolua_function(tolua_S,"NavScale",tolua_uimap_map_SuperCellMapView_NavScale00);
    tolua_function(tolua_S,"Scale",tolua_uimap_map_SuperCellMapView_Scale00);
    tolua_function(tolua_S,"PosX",tolua_uimap_map_SuperCellMapView_PosX00);
    tolua_function(tolua_S,"PosY",tolua_uimap_map_SuperCellMapView_PosY00);
   tolua_endmodule(tolua_S);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_uimap (lua_State* tolua_S) {
 return tolua_uimap_open(tolua_S);
};
#endif


#include <syscfg.h> // Path is project specific.
/*
** Lua binding: trn
** Generated automatically by tolua++-1.0.92 on 01/04/14 22:00:09.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_trn_open (lua_State* tolua_S);

#include "trn/trncd.h"
#include "sdm/StratDB.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"trnCDTerrainObject");
 tolua_usertype(tolua_S,"StratDB::StratObject");
}

/* method: StratObj of class  trnCDTerrainObject */
#ifndef TOLUA_DISABLE_tolua_trn_terrain_trnCDTerrainObject_StratObj00
static int tolua_trn_terrain_trnCDTerrainObject_StratObj00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"trnCDTerrainObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  trnCDTerrainObject* self = (trnCDTerrainObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'StratObj'",NULL);
#endif
  {
   StratDB::StratObject* tolua_ret = (StratDB::StratObject*)  self->StratObj();
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"StratDB::StratObject");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'StratObj'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ObjName of class  trnCDTerrainObject */
#ifndef TOLUA_DISABLE_tolua_trn_terrain_trnCDTerrainObject_ObjName00
static int tolua_trn_terrain_trnCDTerrainObject_ObjName00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"trnCDTerrainObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  trnCDTerrainObject* self = (trnCDTerrainObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ObjName'",NULL);
#endif
  {
   const char* tolua_ret = (const char*)  self->ObjName();
   tolua_pushstring(tolua_S,(const char*)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ObjName'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ObjFlags of class  trnCDTerrainObject */
#ifndef TOLUA_DISABLE_tolua_trn_terrain_trnCDTerrainObject_ObjFlags00
static int tolua_trn_terrain_trnCDTerrainObject_ObjFlags00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"trnCDTerrainObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  trnCDTerrainObject* self = (trnCDTerrainObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ObjFlags'",NULL);
#endif
  {
   unsigned char tolua_ret = (unsigned char)  self->ObjFlags();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ObjFlags'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SupercellX of class  trnCDTerrainObject */
#ifndef TOLUA_DISABLE_tolua_trn_terrain_trnCDTerrainObject_SupercellX00
static int tolua_trn_terrain_trnCDTerrainObject_SupercellX00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"trnCDTerrainObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  trnCDTerrainObject* self = (trnCDTerrainObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SupercellX'",NULL);
#endif
  {
   int tolua_ret = (int)  self->SupercellX();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SupercellX'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SupercellY of class  trnCDTerrainObject */
#ifndef TOLUA_DISABLE_tolua_trn_terrain_trnCDTerrainObject_SupercellY00
static int tolua_trn_terrain_trnCDTerrainObject_SupercellY00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"trnCDTerrainObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  trnCDTerrainObject* self = (trnCDTerrainObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SupercellY'",NULL);
#endif
  {
   int tolua_ret = (int)  self->SupercellY();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SupercellY'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_trn_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_module(tolua_S,"terrain",0);
  tolua_beginmodule(tolua_S,"terrain");
   tolua_cclass(tolua_S,"trnCDTerrainObject","trnCDTerrainObject","",NULL);
   tolua_beginmodule(tolua_S,"trnCDTerrainObject");
    tolua_function(tolua_S,"StratObj",tolua_trn_terrain_trnCDTerrainObject_StratObj00);
    tolua_function(tolua_S,"ObjName",tolua_trn_terrain_trnCDTerrainObject_ObjName00);
    tolua_function(tolua_S,"ObjFlags",tolua_trn_terrain_trnCDTerrainObject_ObjFlags00);
    tolua_function(tolua_S,"SupercellX",tolua_trn_terrain_trnCDTerrainObject_SupercellX00);
    tolua_function(tolua_S,"SupercellY",tolua_trn_terrain_trnCDTerrainObject_SupercellY00);
   tolua_endmodule(tolua_S);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_trn (lua_State* tolua_S) {
 return tolua_trn_open(tolua_S);
};
#endif


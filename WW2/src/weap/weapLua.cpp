#include <syscfg.h> // Path is project specific.
/*
** Lua binding: weap
** Generated automatically by tolua++-1.0.92 on 01/04/14 22:00:09.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_weap_open (lua_State* tolua_S);

#include "include/systypes.h"
#include "weap/weapType.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"weaponType");
}

/* get function: Name of class  weaponType */
#ifndef TOLUA_DISABLE_tolua_get_weaponType_Name
static int tolua_get_weaponType_Name(lua_State* tolua_S)
{
  weaponType* self = (weaponType*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'Name'",NULL);
#endif
  tolua_pushstring(tolua_S,(const char*)self->Name);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: FireType of class  weaponType */
#ifndef TOLUA_DISABLE_tolua_get_weaponType_unsigned_FireType
static int tolua_get_weaponType_unsigned_FireType(lua_State* tolua_S)
{
  weaponType* self = (weaponType*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'FireType'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->FireType);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: RateOfFire of class  weaponType */
#ifndef TOLUA_DISABLE_tolua_get_weaponType_RateOfFire
static int tolua_get_weaponType_RateOfFire(lua_State* tolua_S)
{
  weaponType* self = (weaponType*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'RateOfFire'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->RateOfFire);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: Dispersion of class  weaponType */
#ifndef TOLUA_DISABLE_tolua_get_weaponType_Dispersion
static int tolua_get_weaponType_Dispersion(lua_State* tolua_S)
{
  weaponType* self = (weaponType*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'Dispersion'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->Dispersion);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: NumRoundsInClip of class  weaponType */
#ifndef TOLUA_DISABLE_tolua_get_weaponType_unsigned_NumRoundsInClip
static int tolua_get_weaponType_unsigned_NumRoundsInClip(lua_State* tolua_S)
{
  weaponType* self = (weaponType*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'NumRoundsInClip'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->NumRoundsInClip);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: ReloadTime of class  weaponType */
#ifndef TOLUA_DISABLE_tolua_get_weaponType_ReloadTime
static int tolua_get_weaponType_ReloadTime(lua_State* tolua_S)
{
  weaponType* self = (weaponType*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'ReloadTime'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->ReloadTime);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: IsPlacedWeapon of class  weaponType */
#ifndef TOLUA_DISABLE_tolua_get_weaponType_unsigned_IsPlacedWeapon
static int tolua_get_weaponType_unsigned_IsPlacedWeapon(lua_State* tolua_S)
{
  weaponType* self = (weaponType*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'IsPlacedWeapon'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->IsPlacedWeapon);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: MaxOverSupply of class  weaponType */
#ifndef TOLUA_DISABLE_tolua_get_weaponType_unsigned_MaxOverSupply
static int tolua_get_weaponType_unsigned_MaxOverSupply(lua_State* tolua_S)
{
  weaponType* self = (weaponType*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'MaxOverSupply'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->MaxOverSupply);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: InfResupply of class  weaponType */
#ifndef TOLUA_DISABLE_tolua_get_weaponType_InfResupply
static int tolua_get_weaponType_InfResupply(lua_State* tolua_S)
{
  weaponType* self = (weaponType*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'InfResupply'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->InfResupply);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: ClipType of class  weaponType */
#ifndef TOLUA_DISABLE_tolua_get_weaponType_ClipType
static int tolua_get_weaponType_ClipType(lua_State* tolua_S)
{
  weaponType* self = (weaponType*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'ClipType'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->ClipType);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_weap_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_module(tolua_S,"weapon",0);
  tolua_beginmodule(tolua_S,"weapon");
   tolua_cclass(tolua_S,"weaponType","weaponType","",NULL);
   tolua_beginmodule(tolua_S,"weaponType");
    tolua_variable(tolua_S,"Name",tolua_get_weaponType_Name,NULL);
    tolua_variable(tolua_S,"FireType",tolua_get_weaponType_unsigned_FireType,NULL);
    tolua_variable(tolua_S,"RateOfFire",tolua_get_weaponType_RateOfFire,NULL);
    tolua_variable(tolua_S,"Dispersion",tolua_get_weaponType_Dispersion,NULL);
    tolua_variable(tolua_S,"NumRoundsInClip",tolua_get_weaponType_unsigned_NumRoundsInClip,NULL);
    tolua_variable(tolua_S,"ReloadTime",tolua_get_weaponType_ReloadTime,NULL);
    tolua_variable(tolua_S,"IsPlacedWeapon",tolua_get_weaponType_unsigned_IsPlacedWeapon,NULL);
    tolua_variable(tolua_S,"MaxOverSupply",tolua_get_weaponType_unsigned_MaxOverSupply,NULL);
    tolua_variable(tolua_S,"InfResupply",tolua_get_weaponType_InfResupply,NULL);
    tolua_variable(tolua_S,"ClipType",tolua_get_weaponType_ClipType,NULL);
   tolua_endmodule(tolua_S);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_weap (lua_State* tolua_S) {
 return tolua_weap_open(tolua_S);
};
#endif


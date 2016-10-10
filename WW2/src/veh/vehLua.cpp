#include <syscfg.h> // Path is project specific.
/*
** Lua binding: veh
** Generated automatically by tolua++-1.0.92 on 01/04/14 22:00:09.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_veh_open (lua_State* tolua_S);

#include "veh/vehmain.h"
#include "veh/vehType.h"
#include "weap/weapSpec.h"
#include "pers/persmain.h"

/* function to release collected object via destructor */
#ifdef __cplusplus

static int tolua_collect_WEAPON_SPEC (lua_State* tolua_S)
{
 WEAPON_SPEC* self = (WEAPON_SPEC*) tolua_tousertype(tolua_S,1,0);
	delete self;
	return 0;
}
#endif


/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"WEAPON_SPEC");
 tolua_usertype(tolua_S,"vehType");
 tolua_usertype(tolua_S,"vehVehicle");
}

/* get function: VehicleHostIndex of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_get_vehVehicle_unsigned_VehicleHostIndex
static int tolua_get_vehVehicle_unsigned_VehicleHostIndex(lua_State* tolua_S)
{
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'VehicleHostIndex'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->VehicleHostIndex);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetPosition of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_SetPosition00
static int tolua_veh_vehicle_vehVehicle_SetPosition00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,4,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,5,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
  double x = ((double)  tolua_tonumber(tolua_S,2,0));
  double y = ((double)  tolua_tonumber(tolua_S,3,0));
  double z = ((double)  tolua_tonumber(tolua_S,4,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetPosition'",NULL);
#endif
  {
   self->SetPosition(x,y,z);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetPosition'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetYaw of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_SetYaw00
static int tolua_veh_vehicle_vehVehicle_SetYaw00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
  double y = ((double)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetYaw'",NULL);
#endif
  {
   self->SetYaw(y);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetYaw'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetType of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_GetType00
static int tolua_veh_vehicle_vehVehicle_GetType00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetType'",NULL);
#endif
  {
   vehType* tolua_ret = (vehType*)  self->GetType();
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"vehType");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetType'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetHandle of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_GetHandle00
static int tolua_veh_vehicle_vehVehicle_GetHandle00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetHandle'",NULL);
#endif
  {
   std::string tolua_ret = (std::string)  self->GetHandle();
   tolua_pushcppstring(tolua_S,(const char*)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetHandle'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetChatHandle of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_GetChatHandle00
static int tolua_veh_vehicle_vehVehicle_GetChatHandle00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetChatHandle'",NULL);
#endif
  {
   unsigned long long tolua_ret = (unsigned long long)  self->GetChatHandle();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetChatHandle'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetRank of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_GetRank00
static int tolua_veh_vehicle_vehVehicle_GetRank00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetRank'",NULL);
#endif
  {
   unsigned int tolua_ret = (unsigned int)  self->GetRank();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetRank'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetSide of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_GetSide00
static int tolua_veh_vehicle_vehVehicle_GetSide00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetSide'",NULL);
#endif
  {
   unsigned int tolua_ret = (unsigned int)  self->GetSide();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetSide'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetCountry of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_GetCountry00
static int tolua_veh_vehicle_vehVehicle_GetCountry00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetCountry'",NULL);
#endif
  {
   unsigned int tolua_ret = (unsigned int)  self->GetCountry();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetCountry'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetDecal of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_GetDecal00
static int tolua_veh_vehicle_vehVehicle_GetDecal00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetDecal'",NULL);
#endif
  {
   int tolua_ret = (int)  self->GetDecal();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetDecal'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: DistanceTo of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_DistanceTo00
static int tolua_veh_vehicle_vehVehicle_DistanceTo00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,4,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,5,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
  double x = ((double)  tolua_tonumber(tolua_S,2,0));
  double y = ((double)  tolua_tonumber(tolua_S,3,0));
  double z = ((double)  tolua_tonumber(tolua_S,4,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'DistanceTo'",NULL);
#endif
  {
   double tolua_ret = (double)  self->DistanceTo(x,y,z);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'DistanceTo'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetYaw of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_GetYaw00
static int tolua_veh_vehicle_vehVehicle_GetYaw00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetYaw'",NULL);
#endif
  {
   double tolua_ret = (double)  self->GetYaw();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetYaw'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetPos of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_GetPos00
static int tolua_veh_vehicle_vehVehicle_GetPos00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
  int axis = ((int)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetPos'",NULL);
#endif
  {
   double tolua_ret = (double)  self->GetPos(axis);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetPos'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetTraverse of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_GetTraverse00
static int tolua_veh_vehicle_vehVehicle_GetTraverse00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isboolean(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
  int turret = ((int)  tolua_tonumber(tolua_S,2,0));
  bool world = ((bool)  tolua_toboolean(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetTraverse'",NULL);
#endif
  {
   double tolua_ret = (double)  self->GetTraverse(turret,world);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetTraverse'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetElevation of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_GetElevation00
static int tolua_veh_vehicle_vehVehicle_GetElevation00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isboolean(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
  int turret = ((int)  tolua_tonumber(tolua_S,2,0));
  bool world = ((bool)  tolua_toboolean(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetElevation'",NULL);
#endif
  {
   double tolua_ret = (double)  self->GetElevation(turret,world);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetElevation'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: IsMyMissLeader of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_IsMyMissLeader00
static int tolua_veh_vehicle_vehVehicle_IsMyMissLeader00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'IsMyMissLeader'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->IsMyMissLeader();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'IsMyMissLeader'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: IsMissLeader of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_IsMissLeader00
static int tolua_veh_vehicle_vehVehicle_IsMissLeader00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'IsMissLeader'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->IsMissLeader();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'IsMissLeader'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: IsMissMate of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_IsMissMate00
static int tolua_veh_vehicle_vehVehicle_IsMissMate00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'IsMissMate'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->IsMissMate();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'IsMissMate'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: IsSquadMate of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_IsSquadMate00
static int tolua_veh_vehicle_vehVehicle_IsSquadMate00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'IsSquadMate'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->IsSquadMate();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'IsSquadMate'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: IsBrigMate of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_IsBrigMate00
static int tolua_veh_vehicle_vehVehicle_IsBrigMate00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'IsBrigMate'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->IsBrigMate();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'IsBrigMate'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: IsBldr of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_IsBldr00
static int tolua_veh_vehicle_vehVehicle_IsBldr00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'IsBldr'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->IsBldr();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'IsBldr'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: IsTrnr of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_IsTrnr00
static int tolua_veh_vehicle_vehVehicle_IsTrnr00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'IsTrnr'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->IsTrnr();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'IsTrnr'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: IsHC of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_IsHC00
static int tolua_veh_vehicle_vehVehicle_IsHC00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'IsHC'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->IsHC();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'IsHC'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: IsMe of class  vehVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehVehicle_IsMe00
static int tolua_veh_vehicle_vehVehicle_IsMe00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"vehVehicle",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  vehVehicle* self = (vehVehicle*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'IsMe'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->IsMe();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'IsMe'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* get function: VehicleTypeNum of class  vehType */
#ifndef TOLUA_DISABLE_tolua_get_vehType_unsigned_VehicleTypeNum
static int tolua_get_vehType_unsigned_VehicleTypeNum(lua_State* tolua_S)
{
  vehType* self = (vehType*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'VehicleTypeNum'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->VehicleTypeNum);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: Type of class  vehType */
#ifndef TOLUA_DISABLE_tolua_get_vehType_unsigned_Type
static int tolua_get_vehType_unsigned_Type(lua_State* tolua_S)
{
  vehType* self = (vehType*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'Type'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->Type);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: WeaponSpec of class  vehType */
#ifndef TOLUA_DISABLE_tolua_get_vehType_WeaponSpec
static int tolua_get_vehType_WeaponSpec(lua_State* tolua_S)
{
  vehType* self = (vehType*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'WeaponSpec'",NULL);
#endif
  tolua_pushusertype(tolua_S,(void*)&self->WeaponSpec,"WEAPON_SPEC");
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: VehicleIsMultiCrew of class  vehType */
#ifndef TOLUA_DISABLE_tolua_get_vehType_VehicleIsMultiCrew
static int tolua_get_vehType_VehicleIsMultiCrew(lua_State* tolua_S)
{
  vehType* self = (vehType*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'VehicleIsMultiCrew'",NULL);
#endif
  tolua_pushboolean(tolua_S,(bool)self->VehicleIsMultiCrew);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: OnlineOnly of class  vehType */
#ifndef TOLUA_DISABLE_tolua_get_vehType_OnlineOnly
static int tolua_get_vehType_OnlineOnly(lua_State* tolua_S)
{
  vehType* self = (vehType*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'OnlineOnly'",NULL);
#endif
  tolua_pushboolean(tolua_S,(bool)self->OnlineOnly);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: Id of class  vehType */
#ifndef TOLUA_DISABLE_tolua_get_vehType_Id
static int tolua_get_vehType_Id(lua_State* tolua_S)
{
  vehType* self = (vehType*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'Id'",NULL);
#endif
  tolua_pushstring(tolua_S,(const char*)self->Id);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* function: vehFindVehicleByChatHandle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehFindVehicleByChatHandle00
static int tolua_veh_vehicle_vehFindVehicleByChatHandle00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  unsigned int tolua_var_1 = ((unsigned int)  tolua_tonumber(tolua_S,1,0));
  {
   vehVehicle* tolua_ret = (vehVehicle*)  vehFindVehicleByChatHandle(tolua_var_1);
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"vehVehicle");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'vehFindVehicleByChatHandle'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: vehFindVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehFindVehicle00
static int tolua_veh_vehicle_vehFindVehicle00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  unsigned int tolua_var_2 = ((unsigned int)  tolua_tonumber(tolua_S,1,0));
  {
   vehVehicle* tolua_ret = (vehVehicle*)  vehFindVehicle(tolua_var_2);
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"vehVehicle");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'vehFindVehicle'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: vehTypeGetBySpec */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehTypeGetBySpec00
static int tolua_veh_vehicle_vehTypeGetBySpec00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"WEAPON_SPEC",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  WEAPON_SPEC tolua_var_3 = *((WEAPON_SPEC*)  tolua_tousertype(tolua_S,1,0));
  {
   const vehType* tolua_ret = (const vehType*)  vehTypeGetBySpec(tolua_var_3);
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"const vehType");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'vehTypeGetBySpec'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: vehtypeGetByIndex */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehtypeGetByIndex00
static int tolua_veh_vehicle_vehtypeGetByIndex00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  unsigned int tolua_var_4 = ((unsigned int)  tolua_tonumber(tolua_S,1,0));
  {
   const vehType* tolua_ret = (const vehType*)  vehtypeGetByIndex(tolua_var_4);
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"const vehType");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'vehtypeGetByIndex'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: vehTypeGetById */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehTypeGetById00
static int tolua_veh_vehicle_vehTypeGetById00(lua_State* tolua_S)
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
  const char* tolua_var_5 = ((const char*)  tolua_tostring(tolua_S,1,0));
  {
   const vehType* tolua_ret = (const vehType*)  vehTypeGetById(tolua_var_5);
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"const vehType");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'vehTypeGetById'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: playerGetVehicle */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehPlayerVehicle00
static int tolua_veh_vehicle_vehPlayerVehicle00(lua_State* tolua_S)
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
   vehVehicle* tolua_ret = (vehVehicle*)  playerGetVehicle();
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"vehVehicle");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'vehPlayerVehicle'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: vehmainAddSTO */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehmainAddSTO00
static int tolua_veh_vehicle_vehmainAddSTO00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const vehType",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,4,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,5,0,&tolua_err) ||
     !tolua_isboolean(tolua_S,6,0,&tolua_err) ||
     !tolua_isstring(tolua_S,7,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,8,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const vehType* type = ((const vehType*)  tolua_tousertype(tolua_S,1,0));
  double x = ((double)  tolua_tonumber(tolua_S,2,0));
  double y = ((double)  tolua_tonumber(tolua_S,3,0));
  double z = ((double)  tolua_tonumber(tolua_S,4,0));
  float att = ((float)  tolua_tonumber(tolua_S,5,0));
  bool relative = ((bool)  tolua_toboolean(tolua_S,6,0));
  const char* icon = ((const char*)  tolua_tostring(tolua_S,7,0));
  {
   unsigned int tolua_ret = (unsigned int)  vehmainAddSTO(type,x,y,z,att,relative,icon);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'vehmainAddSTO'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: vehmainAddSTO */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehmainAddSTO01
static int tolua_veh_vehicle_vehmainAddSTO01(lua_State* tolua_S)
{
 tolua_Error tolua_err;
 if (
     !tolua_isstring(tolua_S,1,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,4,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,5,0,&tolua_err) ||
     !tolua_isboolean(tolua_S,6,0,&tolua_err) ||
     !tolua_isstring(tolua_S,7,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,8,&tolua_err)
 )
  goto tolua_lerror;
 else
 {
  const char* type = ((const char*)  tolua_tostring(tolua_S,1,0));
  double x = ((double)  tolua_tonumber(tolua_S,2,0));
  double y = ((double)  tolua_tonumber(tolua_S,3,0));
  double z = ((double)  tolua_tonumber(tolua_S,4,0));
  float att = ((float)  tolua_tonumber(tolua_S,5,0));
  bool relative = ((bool)  tolua_toboolean(tolua_S,6,0));
  const char* icon = ((const char*)  tolua_tostring(tolua_S,7,0));
  {
   unsigned int tolua_ret = (unsigned int)  vehmainAddSTO(type,x,y,z,att,relative,icon);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
tolua_lerror:
 return tolua_veh_vehicle_vehmainAddSTO00(tolua_S);
}
#endif //#ifndef TOLUA_DISABLE

/* function: vehmainRemoveSTO */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_vehmainRemoveSTO00
static int tolua_veh_vehicle_vehmainRemoveSTO00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  unsigned int id = ((unsigned int)  tolua_tonumber(tolua_S,1,0));
  {
   bool tolua_ret = (bool)  vehmainRemoveSTO(id);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'vehmainRemoveSTO'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* get function: wCountry of class  WEAPON_SPEC */
#ifndef TOLUA_DISABLE_tolua_get_WEAPON_SPEC_unsigned_wCountry
static int tolua_get_WEAPON_SPEC_unsigned_wCountry(lua_State* tolua_S)
{
  WEAPON_SPEC* self = (WEAPON_SPEC*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'wCountry'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->wCountry);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: wCategory of class  WEAPON_SPEC */
#ifndef TOLUA_DISABLE_tolua_get_WEAPON_SPEC_unsigned_wCategory
static int tolua_get_WEAPON_SPEC_unsigned_wCategory(lua_State* tolua_S)
{
  WEAPON_SPEC* self = (WEAPON_SPEC*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'wCategory'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->wCategory);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: wClass of class  WEAPON_SPEC */
#ifndef TOLUA_DISABLE_tolua_get_WEAPON_SPEC_unsigned_wClass
static int tolua_get_WEAPON_SPEC_unsigned_wClass(lua_State* tolua_S)
{
  WEAPON_SPEC* self = (WEAPON_SPEC*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'wClass'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->wClass);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: wType of class  WEAPON_SPEC */
#ifndef TOLUA_DISABLE_tolua_get_WEAPON_SPEC_unsigned_wType
static int tolua_get_WEAPON_SPEC_unsigned_wType(lua_State* tolua_S)
{
  WEAPON_SPEC* self = (WEAPON_SPEC*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'wType'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->wType);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* method: isNull of class  WEAPON_SPEC */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_WEAPON_SPEC_isNull00
static int tolua_veh_vehicle_WEAPON_SPEC_isNull00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"WEAPON_SPEC",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  WEAPON_SPEC* self = (WEAPON_SPEC*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'isNull'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->isNull();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'isNull'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: country of class  WEAPON_SPEC */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_WEAPON_SPEC_country00
static int tolua_veh_vehicle_WEAPON_SPEC_country00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"WEAPON_SPEC",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  WEAPON_SPEC* self = (WEAPON_SPEC*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'country'",NULL);
#endif
  {
   int tolua_ret = (int)  self->country();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'country'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: side of class  WEAPON_SPEC */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_WEAPON_SPEC_side00
static int tolua_veh_vehicle_WEAPON_SPEC_side00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"WEAPON_SPEC",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  WEAPON_SPEC* self = (WEAPON_SPEC*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'side'",NULL);
#endif
  {
   int tolua_ret = (int)  self->side();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'side'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: weapCreateSpec */
#ifndef TOLUA_DISABLE_tolua_veh_vehicle_weapCreateSpec00
static int tolua_veh_vehicle_weapCreateSpec00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isnumber(tolua_S, 1, 0, &tolua_err) ||
     !tolua_isnumber(tolua_S, 2, 0, &tolua_err) ||
     !tolua_isnumber(tolua_S, 3, 0, &tolua_err) ||
     !tolua_isnumber(tolua_S, 4, 0, &tolua_err) ||
     !tolua_isnoobj(tolua_S, 5, &tolua_err)
     )
  goto tolua_lerror;
 else
#endif
 {
    int wcountry  = ((int)tolua_tonumber(tolua_S, 1, 0));
    int wcategory = ((int)tolua_tonumber(tolua_S, 2, 0));
    int wclass    = ((int)tolua_tonumber(tolua_S, 3, 0));
    int wtype     = ((int)tolua_tonumber(tolua_S, 4, 0));
  {
    WEAPON_SPEC tolua_ret = (WEAPON_SPEC)weapCreateSpec(wcountry, wcategory, wclass, wtype);
   {
#ifdef __cplusplus
    void* tolua_obj = new WEAPON_SPEC(tolua_ret);
    tolua_pushusertype_and_takeownership(tolua_S,tolua_obj,"WEAPON_SPEC");
#else
    void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(WEAPON_SPEC));
    tolua_pushusertype_and_takeownership(tolua_S,tolua_obj,"WEAPON_SPEC");
#endif
   }
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'weapCreateSpec'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_veh_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_module(tolua_S,"vehicle",0);
  tolua_beginmodule(tolua_S,"vehicle");
   tolua_cclass(tolua_S,"vehVehicle","vehVehicle","",NULL);
   tolua_beginmodule(tolua_S,"vehVehicle");
    tolua_variable(tolua_S,"VehicleHostIndex",tolua_get_vehVehicle_unsigned_VehicleHostIndex,NULL);
    tolua_function(tolua_S,"SetPosition",tolua_veh_vehicle_vehVehicle_SetPosition00);
    tolua_function(tolua_S,"SetYaw",tolua_veh_vehicle_vehVehicle_SetYaw00);
    tolua_function(tolua_S,"GetType",tolua_veh_vehicle_vehVehicle_GetType00);
    tolua_function(tolua_S,"GetHandle",tolua_veh_vehicle_vehVehicle_GetHandle00);
    tolua_function(tolua_S,"GetChatHandle",tolua_veh_vehicle_vehVehicle_GetChatHandle00);
    tolua_function(tolua_S,"GetRank",tolua_veh_vehicle_vehVehicle_GetRank00);
    tolua_function(tolua_S,"GetSide",tolua_veh_vehicle_vehVehicle_GetSide00);
    tolua_function(tolua_S,"GetCountry",tolua_veh_vehicle_vehVehicle_GetCountry00);
    tolua_function(tolua_S,"GetDecal",tolua_veh_vehicle_vehVehicle_GetDecal00);
    tolua_function(tolua_S,"DistanceTo",tolua_veh_vehicle_vehVehicle_DistanceTo00);
    tolua_function(tolua_S,"GetYaw",tolua_veh_vehicle_vehVehicle_GetYaw00);
    tolua_function(tolua_S,"GetPos",tolua_veh_vehicle_vehVehicle_GetPos00);
    tolua_function(tolua_S,"GetTraverse",tolua_veh_vehicle_vehVehicle_GetTraverse00);
    tolua_function(tolua_S,"GetElevation",tolua_veh_vehicle_vehVehicle_GetElevation00);
    tolua_function(tolua_S,"IsMyMissLeader",tolua_veh_vehicle_vehVehicle_IsMyMissLeader00);
    tolua_function(tolua_S,"IsMissLeader",tolua_veh_vehicle_vehVehicle_IsMissLeader00);
    tolua_function(tolua_S,"IsMissMate",tolua_veh_vehicle_vehVehicle_IsMissMate00);
    tolua_function(tolua_S,"IsSquadMate",tolua_veh_vehicle_vehVehicle_IsSquadMate00);
    tolua_function(tolua_S,"IsBrigMate",tolua_veh_vehicle_vehVehicle_IsBrigMate00);
    tolua_function(tolua_S,"IsBldr",tolua_veh_vehicle_vehVehicle_IsBldr00);
    tolua_function(tolua_S,"IsTrnr",tolua_veh_vehicle_vehVehicle_IsTrnr00);
    tolua_function(tolua_S,"IsHC",tolua_veh_vehicle_vehVehicle_IsHC00);
    tolua_function(tolua_S,"IsMe",tolua_veh_vehicle_vehVehicle_IsMe00);
   tolua_endmodule(tolua_S);
   tolua_constant(tolua_S,"VEH_TYPE_NONE",VEH_TYPE_NONE);
   tolua_constant(tolua_S,"VEH_TYPE_FLOATER",VEH_TYPE_FLOATER);
   tolua_constant(tolua_S,"VEH_TYPE_PERSON",VEH_TYPE_PERSON);
   tolua_constant(tolua_S,"VEH_TYPE_AIR",VEH_TYPE_AIR);
   tolua_constant(tolua_S,"VEH_TYPE_GROUND",VEH_TYPE_GROUND);
   tolua_constant(tolua_S,"VEH_TYPE_SEA",VEH_TYPE_SEA);
   tolua_constant(tolua_S,"VEH_MAXTYPE",VEH_MAXTYPE);
   tolua_cclass(tolua_S,"vehType","vehType","",NULL);
   tolua_beginmodule(tolua_S,"vehType");
    tolua_variable(tolua_S,"VehicleTypeNum",tolua_get_vehType_unsigned_VehicleTypeNum,NULL);
    tolua_variable(tolua_S,"Type",tolua_get_vehType_unsigned_Type,NULL);
    tolua_variable(tolua_S,"WeaponSpec",tolua_get_vehType_WeaponSpec,NULL);
    tolua_variable(tolua_S,"VehicleIsMultiCrew",tolua_get_vehType_VehicleIsMultiCrew,NULL);
    tolua_variable(tolua_S,"OnlineOnly",tolua_get_vehType_OnlineOnly,NULL);
    tolua_variable(tolua_S,"Id",tolua_get_vehType_Id,NULL);
   tolua_endmodule(tolua_S);
   tolua_function(tolua_S,"vehFindVehicleByChatHandle",tolua_veh_vehicle_vehFindVehicleByChatHandle00);
   tolua_function(tolua_S,"vehFindVehicle",tolua_veh_vehicle_vehFindVehicle00);
   tolua_function(tolua_S,"vehTypeGetBySpec",tolua_veh_vehicle_vehTypeGetBySpec00);
   tolua_function(tolua_S,"vehtypeGetByIndex",tolua_veh_vehicle_vehtypeGetByIndex00);
   tolua_function(tolua_S,"vehTypeGetById",tolua_veh_vehicle_vehTypeGetById00);
   tolua_function(tolua_S,"vehPlayerVehicle",tolua_veh_vehicle_vehPlayerVehicle00);
   tolua_function(tolua_S,"vehmainAddSTO",tolua_veh_vehicle_vehmainAddSTO00);
   tolua_function(tolua_S,"vehmainAddSTO",tolua_veh_vehicle_vehmainAddSTO01);
   tolua_function(tolua_S,"vehmainRemoveSTO",tolua_veh_vehicle_vehmainRemoveSTO00);
   #ifdef __cplusplus
   tolua_cclass(tolua_S,"WEAPON_SPEC","WEAPON_SPEC","",tolua_collect_WEAPON_SPEC);
   #else
   tolua_cclass(tolua_S,"WEAPON_SPEC","WEAPON_SPEC","",NULL);
   #endif
   tolua_beginmodule(tolua_S,"WEAPON_SPEC");
    tolua_variable(tolua_S,"wCountry",tolua_get_WEAPON_SPEC_unsigned_wCountry,NULL);
    tolua_variable(tolua_S,"wCategory",tolua_get_WEAPON_SPEC_unsigned_wCategory,NULL);
    tolua_variable(tolua_S,"wClass",tolua_get_WEAPON_SPEC_unsigned_wClass,NULL);
    tolua_variable(tolua_S,"wType",tolua_get_WEAPON_SPEC_unsigned_wType,NULL);
    tolua_function(tolua_S,"isNull",tolua_veh_vehicle_WEAPON_SPEC_isNull00);
    tolua_function(tolua_S,"country",tolua_veh_vehicle_WEAPON_SPEC_country00);
    tolua_function(tolua_S,"side",tolua_veh_vehicle_WEAPON_SPEC_side00);
   tolua_endmodule(tolua_S);
   tolua_function(tolua_S,"weapCreateSpec",tolua_veh_vehicle_weapCreateSpec00);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_veh (lua_State* tolua_S) {
 return tolua_veh_open(tolua_S);
};
#endif


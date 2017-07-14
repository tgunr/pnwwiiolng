#include "syscfg.h" // Path is project specific.
/*
** Lua binding: pers
** Generated automatically by tolua++-1.0.92 on 01/04/14 22:00:08.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_pers_open (lua_State* tolua_S);

#include "veh/vehmain.h"
#include "pers/persmain.h"
#include "pers/pershud.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"vehVehicle");
 tolua_usertype(tolua_S,"Player");
}

/* function: persHUDToggleHUD */
#ifndef TOLUA_DISABLE_tolua_pers_player_persHUDToggleHUD00
static int tolua_pers_player_persHUDToggleHUD00(lua_State* tolua_S)
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
  unsigned int alwaysZero = (( unsigned int)  tolua_tonumber(tolua_S,3,0));
  {
   unsigned int tolua_ret = ( unsigned int)  persHUDToggleHUD(keyState,functionNum,alwaysZero);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'persHUDToggleHUD'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: persHUDToggleGameInfo */
#ifndef TOLUA_DISABLE_tolua_pers_player_persHUDToggleGameInfo00
static int tolua_pers_player_persHUDToggleGameInfo00(lua_State* tolua_S)
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
  unsigned int alwaysZero = (( unsigned int)  tolua_tonumber(tolua_S,3,0));
  {
   unsigned int tolua_ret = ( unsigned int)  persHUDToggleGameInfo(keyState,functionNum,alwaysZero);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'persHUDToggleGameInfo'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: persHUDToggleReadout */
#ifndef TOLUA_DISABLE_tolua_pers_player_persHUDToggleReadout00
static int tolua_pers_player_persHUDToggleReadout00(lua_State* tolua_S)
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
  unsigned int alwaysZero = (( unsigned int)  tolua_tonumber(tolua_S,3,0));
  {
   unsigned int tolua_ret = ( unsigned int)  persHUDToggleReadout(keyState,functionNum,alwaysZero);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'persHUDToggleReadout'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: persHUDLock */
#ifndef TOLUA_DISABLE_tolua_pers_player_persHUDLock00
static int tolua_pers_player_persHUDLock00(lua_State* tolua_S)
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
   persHUDLock(l);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'persHUDLock'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: persHUDResetHUD */
#ifndef TOLUA_DISABLE_tolua_pers_player_persHUDResetHUD00
static int tolua_pers_player_persHUDResetHUD00(lua_State* tolua_S)
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
   int param = ((  int)  tolua_tonumber(tolua_S,1,0));
  unsigned int functionNum = (( unsigned int)  tolua_tonumber(tolua_S,2,0));
  unsigned int alwaysZero = (( unsigned int)  tolua_tonumber(tolua_S,3,0));
  {
   unsigned int tolua_ret = ( unsigned int)  persHUDResetHUD(param,functionNum,alwaysZero);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'persHUDResetHUD'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: persHUDShowHUD */
#ifndef TOLUA_DISABLE_tolua_pers_player_persHUDShowHUD00
static int tolua_pers_player_persHUDShowHUD00(lua_State* tolua_S)
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
   persHUDShowHUD();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'persHUDShowHUD'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: persHUDHideHUD */
#ifndef TOLUA_DISABLE_tolua_pers_player_persHUDHideHUD00
static int tolua_pers_player_persHUDHideHUD00(lua_State* tolua_S)
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
   persHUDHideHUD(close);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'persHUDHideHUD'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetVehicle of class  Player */
#ifndef TOLUA_DISABLE_tolua_pers_player_Player_GetVehicle00
static int tolua_pers_player_Player_GetVehicle00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  Player* self = (Player*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetVehicle'",NULL);
#endif
  {
   vehVehicle* tolua_ret = (vehVehicle*)  self->GetVehicle();
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"vehVehicle");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetVehicle'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ShakeHead of class  Player */
#ifndef TOLUA_DISABLE_tolua_pers_player_Player_ShakeHead00
static int tolua_pers_player_Player_ShakeHead00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  Player* self = (Player*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ShakeHead'",NULL);
#endif
  {
   self->ShakeHead();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ShakeHead'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetEnergy of class  Player */
#ifndef TOLUA_DISABLE_tolua_pers_player_Player_GetEnergy00
static int tolua_pers_player_Player_GetEnergy00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  Player* self = (Player*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetEnergy'",NULL);
#endif
  {
   float tolua_ret = (float)  self->GetEnergy();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetEnergy'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetHealth of class  Player */
#ifndef TOLUA_DISABLE_tolua_pers_player_Player_GetHealth00
static int tolua_pers_player_Player_GetHealth00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  Player* self = (Player*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetHealth'",NULL);
#endif
  {
   float tolua_ret = (float)  self->GetHealth();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetHealth'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetSide of class  Player */
#ifndef TOLUA_DISABLE_tolua_pers_player_Player_GetSide00
static int tolua_pers_player_Player_GetSide00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  Player* self = (Player*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetSide'",NULL);
#endif
  {
   int tolua_ret = (int)  self->GetSide();
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

/* method: GetCountry of class  Player */
#ifndef TOLUA_DISABLE_tolua_pers_player_Player_GetCountry00
static int tolua_pers_player_Player_GetCountry00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  Player* self = (Player*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetCountry'",NULL);
#endif
  {
   int tolua_ret = (int)  self->GetCountry();
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

/* method: GetRangeSetting of class  Player */
#ifndef TOLUA_DISABLE_tolua_pers_player_Player_GetRangeSetting00
static int tolua_pers_player_Player_GetRangeSetting00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  Player* self = (Player*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetRangeSetting'",NULL);
#endif
  {
   int tolua_ret = (int)  self->GetRangeSetting();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetRangeSetting'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: playerGetPlayer */
#ifndef TOLUA_DISABLE_tolua_pers_player_playerGetPlayer00
static int tolua_pers_player_playerGetPlayer00(lua_State* tolua_S)
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
   Player* tolua_ret = (Player*)  playerGetPlayer();
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"Player");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'playerGetPlayer'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: playerGetVehicle */
#ifndef TOLUA_DISABLE_tolua_pers_player_playerGetVehicle00
static int tolua_pers_player_playerGetVehicle00(lua_State* tolua_S)
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
 tolua_error(tolua_S,"#ferror in function 'playerGetVehicle'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: persApplyDamage */
#ifndef TOLUA_DISABLE_tolua_pers_player_persApplyDamage00
static int tolua_pers_player_persApplyDamage00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,4,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,5,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  float Damage = ((float)  tolua_tonumber(tolua_S,1,0));
  unsigned int Position = ((unsigned int)  tolua_tonumber(tolua_S,2,0));
  unsigned int Location = ((unsigned int)  tolua_tonumber(tolua_S,3,0));
  int ATPOnly = ((int)  tolua_tonumber(tolua_S,4,0));
  {
   unsigned int tolua_ret = (unsigned int)  persApplyDamage(Damage,Position,Location,ATPOnly);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'persApplyDamage'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_pers_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_module(tolua_S,"player",0);
  tolua_beginmodule(tolua_S,"player");
   tolua_constant(tolua_S,"HUD_CYCLE_OFF",HUD_CYCLE_OFF);
   tolua_constant(tolua_S,"HUD_CYCLE_RESET",HUD_CYCLE_RESET);
   tolua_constant(tolua_S,"HUD_CYCLE_ON",HUD_CYCLE_ON);
   tolua_constant(tolua_S,"HUD_CYCLE_INVALID",HUD_CYCLE_INVALID);
   tolua_function(tolua_S,"persHUDToggleHUD",tolua_pers_player_persHUDToggleHUD00);
   tolua_function(tolua_S,"persHUDToggleGameInfo",tolua_pers_player_persHUDToggleGameInfo00);
   tolua_function(tolua_S,"persHUDToggleReadout",tolua_pers_player_persHUDToggleReadout00);
   tolua_function(tolua_S,"persHUDLock",tolua_pers_player_persHUDLock00);
   tolua_function(tolua_S,"persHUDResetHUD",tolua_pers_player_persHUDResetHUD00);
   tolua_function(tolua_S,"persHUDShowHUD",tolua_pers_player_persHUDShowHUD00);
   tolua_function(tolua_S,"persHUDHideHUD",tolua_pers_player_persHUDHideHUD00);
   tolua_cclass(tolua_S,"Player","Player","",NULL);
   tolua_beginmodule(tolua_S,"Player");
    tolua_function(tolua_S,"GetVehicle",tolua_pers_player_Player_GetVehicle00);
    tolua_function(tolua_S,"ShakeHead",tolua_pers_player_Player_ShakeHead00);
    tolua_function(tolua_S,"GetEnergy",tolua_pers_player_Player_GetEnergy00);
    tolua_function(tolua_S,"GetHealth",tolua_pers_player_Player_GetHealth00);
    tolua_function(tolua_S,"GetSide",tolua_pers_player_Player_GetSide00);
    tolua_function(tolua_S,"GetCountry",tolua_pers_player_Player_GetCountry00);
    tolua_function(tolua_S,"GetRangeSetting",tolua_pers_player_Player_GetRangeSetting00);
   tolua_endmodule(tolua_S);
   tolua_function(tolua_S,"playerGetPlayer",tolua_pers_player_playerGetPlayer00);
   tolua_function(tolua_S,"playerGetVehicle",tolua_pers_player_playerGetVehicle00);
   tolua_function(tolua_S,"persApplyDamage",tolua_pers_player_persApplyDamage00);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_pers (lua_State* tolua_S) {
 return tolua_pers_open(tolua_S);
};
#endif


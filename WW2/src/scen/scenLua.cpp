#include <syscfg.h> // Path is project specific.
/*
** Lua binding: scen
** Generated automatically by tolua++-1.0.92 on 01/04/14 22:00:09.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_scen_open (lua_State* tolua_S);

#include "scen/ScenarioManager.h"

/* function to release collected object via destructor */
#ifdef __cplusplus

static int tolua_collect_ScenarioManager (lua_State* tolua_S)
{
 ScenarioManager* self = (ScenarioManager*) tolua_tousertype(tolua_S,1,0);
	delete self;
	return 0;
}
#endif


/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"ScenarioObject");
 tolua_usertype(tolua_S,"ScenarioManager");
 tolua_usertype(tolua_S,"ScenarioObject::TYPE");
}

/* method: new of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_new00
static int tolua_scen_scenario_ScenarioManager_new00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertable(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  {
   ScenarioManager* tolua_ret = (ScenarioManager*)  new ScenarioManager();
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"ScenarioManager");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'new'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: new_local of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_new00_local
static int tolua_scen_scenario_ScenarioManager_new00_local(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertable(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  {
   ScenarioManager* tolua_ret = (ScenarioManager*)  new ScenarioManager();
   tolua_pushusertype_and_takeownership(tolua_S,(void *)tolua_ret,"ScenarioManager");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'new'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: delete of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_delete00
static int tolua_scen_scenario_ScenarioManager_delete00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'delete'",NULL);
#endif
  delete self;
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'delete'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Init of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_Init00
static int tolua_scen_scenario_ScenarioManager_Init00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Init'",NULL);
#endif
  {
   self->Init();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Init'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Update of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_Update00
static int tolua_scen_scenario_ScenarioManager_Update00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Update'",NULL);
#endif
  {
   self->Update();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Update'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: UpdatePlayer of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_UpdatePlayer00
static int tolua_scen_scenario_ScenarioManager_UpdatePlayer00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isstring(tolua_S,3,0,&tolua_err) ||
     !tolua_isstring(tolua_S,4,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,5,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
  const char* callsign = ((const char*)  tolua_tostring(tolua_S,2,0));
  const char* state = ((const char*)  tolua_tostring(tolua_S,3,0));
  const char* value = ((const char*)  tolua_tostring(tolua_S,4,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'UpdatePlayer'",NULL);
#endif
  {
   self->UpdatePlayer(callsign,state,value);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'UpdatePlayer'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: IsActive of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_IsActive00
static int tolua_scen_scenario_ScenarioManager_IsActive00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const ScenarioManager",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const ScenarioManager* self = (const ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'IsActive'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->IsActive();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'IsActive'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: IsScenarioComplete of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_IsScenarioComplete00
static int tolua_scen_scenario_ScenarioManager_IsScenarioComplete00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const ScenarioManager",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const ScenarioManager* self = (const ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'IsScenarioComplete'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->IsScenarioComplete();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'IsScenarioComplete'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetFlag of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_SetFlag00
static int tolua_scen_scenario_ScenarioManager_SetFlag00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isboolean(tolua_S,3,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
   int flag = ((  int)  tolua_tonumber(tolua_S,2,0));
  bool on = ((bool)  tolua_toboolean(tolua_S,3,true));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetFlag'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->SetFlag(flag,on);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetFlag'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: HasFlag of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_HasFlag00
static int tolua_scen_scenario_ScenarioManager_HasFlag00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
   int flag = ((  int)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'HasFlag'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->HasFlag(flag);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'HasFlag'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetPlayerCount of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_GetPlayerCount00
static int tolua_scen_scenario_ScenarioManager_GetPlayerCount00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
  int team = ((int)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetPlayerCount'",NULL);
#endif
  {
   int tolua_ret = (int)  self->GetPlayerCount(team);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetPlayerCount'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetReadyCount of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_GetReadyCount00
static int tolua_scen_scenario_ScenarioManager_GetReadyCount00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
  int team = ((int)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetReadyCount'",NULL);
#endif
  {
   int tolua_ret = (int)  self->GetReadyCount(team);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetReadyCount'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetRound of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_GetRound00
static int tolua_scen_scenario_ScenarioManager_GetRound00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetRound'",NULL);
#endif
  {
   int tolua_ret = (int)  self->GetRound();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetRound'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetStage of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_GetStage00
static int tolua_scen_scenario_ScenarioManager_GetStage00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetStage'",NULL);
#endif
  {
   int tolua_ret = (int)  self->GetStage();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetStage'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetRound of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_SetRound00
static int tolua_scen_scenario_ScenarioManager_SetRound00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
  int r = ((int)  tolua_tonumber(tolua_S,2,-1));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetRound'",NULL);
#endif
  {
   int tolua_ret = (int)  self->SetRound(r);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetRound'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetStage of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_SetStage00
static int tolua_scen_scenario_ScenarioManager_SetStage00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
  int s = ((int)  tolua_tonumber(tolua_S,2,-1));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetStage'",NULL);
#endif
  {
   int tolua_ret = (int)  self->SetStage(s);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetStage'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: PlayerRelevant of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_PlayerRelevant00
static int tolua_scen_scenario_ScenarioManager_PlayerRelevant00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
  const char* name = ((const char*)  tolua_tostring(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'PlayerRelevant'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->PlayerRelevant(name);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'PlayerRelevant'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: PlayerRelevant of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_PlayerRelevant01
static int tolua_scen_scenario_ScenarioManager_PlayerRelevant01(lua_State* tolua_S)
{
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isusertype(tolua_S,2,"ScenarioObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
  ScenarioObject* ob = ((ScenarioObject*)  tolua_tousertype(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'PlayerRelevant'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->PlayerRelevant(ob);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
tolua_lerror:
 return tolua_scen_scenario_ScenarioManager_PlayerRelevant00(tolua_S);
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetTeam of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_GetTeam00
static int tolua_scen_scenario_ScenarioManager_GetTeam00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetTeam'",NULL);
#endif
  {
   int tolua_ret = (int)  self->GetTeam();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetTeam'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetOtherTeam of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_GetOtherTeam00
static int tolua_scen_scenario_ScenarioManager_GetOtherTeam00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetOtherTeam'",NULL);
#endif
  {
   int tolua_ret = (int)  self->GetOtherTeam();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetOtherTeam'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: RunObject of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_RunObject00
static int tolua_scen_scenario_ScenarioManager_RunObject00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
  const char* name = ((const char*)  tolua_tostring(tolua_S,2,0));
  int load = ((int)  tolua_tonumber(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'RunObject'",NULL);
#endif
  {
   self->RunObject(name,load);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'RunObject'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: RunObject of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_RunObject01
static int tolua_scen_scenario_ScenarioManager_RunObject01(lua_State* tolua_S)
{
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isusertype(tolua_S,2,"ScenarioObject::TYPE",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
  ScenarioObject::TYPE type = *((ScenarioObject::TYPE*)  tolua_tousertype(tolua_S,2,0));
  int load = ((int)  tolua_tonumber(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'RunObject'",NULL);
#endif
  {
   self->RunObject(type,load);
  }
 }
 return 0;
tolua_lerror:
 return tolua_scen_scenario_ScenarioManager_RunObject00(tolua_S);
}
#endif //#ifndef TOLUA_DISABLE

/* method: RunObject of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_RunObject02
static int tolua_scen_scenario_ScenarioManager_RunObject02(lua_State* tolua_S)
{
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isstring(tolua_S,4,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,5,&tolua_err)
 )
  goto tolua_lerror;
 else
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
  const char* name = ((const char*)  tolua_tostring(tolua_S,2,0));
  int load = ((int)  tolua_tonumber(tolua_S,3,0));
  const char* data = ((const char*)  tolua_tostring(tolua_S,4,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'RunObject'",NULL);
#endif
  {
   self->RunObject(name,load,data);
  }
 }
 return 0;
tolua_lerror:
 return tolua_scen_scenario_ScenarioManager_RunObject01(tolua_S);
}
#endif //#ifndef TOLUA_DISABLE

/* method: RunObject of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_RunObject03
static int tolua_scen_scenario_ScenarioManager_RunObject03(lua_State* tolua_S)
{
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isusertype(tolua_S,2,"ScenarioObject::TYPE",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isstring(tolua_S,4,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,5,&tolua_err)
 )
  goto tolua_lerror;
 else
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
  ScenarioObject::TYPE type = *((ScenarioObject::TYPE*)  tolua_tousertype(tolua_S,2,0));
  int load = ((int)  tolua_tonumber(tolua_S,3,0));
  const char* data = ((const char*)  tolua_tostring(tolua_S,4,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'RunObject'",NULL);
#endif
  {
   self->RunObject(type,load,data);
  }
 }
 return 0;
tolua_lerror:
 return tolua_scen_scenario_ScenarioManager_RunObject02(tolua_S);
}
#endif //#ifndef TOLUA_DISABLE

/* method: LoadObjects of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_LoadObjects00
static int tolua_scen_scenario_ScenarioManager_LoadObjects00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
  unsigned int loadFlags = (( unsigned int)  tolua_tonumber(tolua_S,2,ScenarioObject::ON_START));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'LoadObjects'",NULL);
#endif
  {
   self->LoadObjects(loadFlags);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'LoadObjects'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: LoadContent of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_LoadContent00
static int tolua_scen_scenario_ScenarioManager_LoadContent00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'LoadContent'",NULL);
#endif
  {
   self->LoadContent();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'LoadContent'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: UnloadContent of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_UnloadContent00
static int tolua_scen_scenario_ScenarioManager_UnloadContent00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'UnloadContent'",NULL);
#endif
  {
   self->UnloadContent();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'UnloadContent'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: RunEvent of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_RunEvent00
static int tolua_scen_scenario_ScenarioManager_RunEvent00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
  const char* name = ((const char*)  tolua_tostring(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'RunEvent'",NULL);
#endif
  {
   self->RunEvent(name);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'RunEvent'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: AddContent of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_AddContent00
static int tolua_scen_scenario_ScenarioManager_AddContent00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ScenarioManager* self = (ScenarioManager*)  tolua_tousertype(tolua_S,1,0);
  const char* line = ((const char*)  tolua_tostring(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'AddContent'",NULL);
#endif
  {
   int tolua_ret = (int)  self->AddContent(line);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'AddContent'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetInstance of class  ScenarioManager */
#ifndef TOLUA_DISABLE_tolua_scen_scenario_ScenarioManager_GetInstance00
static int tolua_scen_scenario_ScenarioManager_GetInstance00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertable(tolua_S,1,"ScenarioManager",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  {
   ScenarioManager* tolua_ret = (ScenarioManager*)  ScenarioManager::GetInstance();
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"ScenarioManager");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetInstance'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_scen_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_module(tolua_S,"scenario",0);
  tolua_beginmodule(tolua_S,"scenario");
   #ifdef __cplusplus
   tolua_cclass(tolua_S,"ScenarioManager","ScenarioManager","",tolua_collect_ScenarioManager);
   #else
   tolua_cclass(tolua_S,"ScenarioManager","ScenarioManager","",NULL);
   #endif
   tolua_beginmodule(tolua_S,"ScenarioManager");
    tolua_constant(tolua_S,"ALL_OFF",ScenarioManager::ALL_OFF);
    tolua_constant(tolua_S,"WHO_KILLED_ME",ScenarioManager::WHO_KILLED_ME);
    tolua_constant(tolua_S,"ENEMY_DEATHS",ScenarioManager::ENEMY_DEATHS);
    tolua_constant(tolua_S,"ENEMY_ICONS",ScenarioManager::ENEMY_ICONS);
    tolua_constant(tolua_S,"FAST_ENEMY_ICONS",ScenarioManager::FAST_ENEMY_ICONS);
    tolua_constant(tolua_S,"FIRING_ENEMY_ICONS",ScenarioManager::FIRING_ENEMY_ICONS);
    tolua_constant(tolua_S,"HIDE_PRONE_ENEMY_ICONS",ScenarioManager::HIDE_PRONE_ENEMY_ICONS);
    tolua_constant(tolua_S,"ENEMY_ICON_TYPES_ONLY",ScenarioManager::ENEMY_ICON_TYPES_ONLY);
    tolua_constant(tolua_S,"RUN_EVENTS",ScenarioManager::RUN_EVENTS);
    tolua_constant(tolua_S,"ENEMY_INTEL",ScenarioManager::ENEMY_INTEL);
    tolua_constant(tolua_S,"RESUPPLY_ME",ScenarioManager::RESUPPLY_ME);
    tolua_constant(tolua_S,"RESPAWN_ME",ScenarioManager::RESPAWN_ME);
    tolua_constant(tolua_S,"GLOBAL_NONE",ScenarioManager::GLOBAL_NONE);
    tolua_constant(tolua_S,"GLOBAL_GENERATE_MAP",ScenarioManager::GLOBAL_GENERATE_MAP);
    tolua_function(tolua_S,"new",tolua_scen_scenario_ScenarioManager_new00);
    tolua_function(tolua_S,"new_local",tolua_scen_scenario_ScenarioManager_new00_local);
    tolua_function(tolua_S,".call",tolua_scen_scenario_ScenarioManager_new00_local);
    tolua_function(tolua_S,"delete",tolua_scen_scenario_ScenarioManager_delete00);
    tolua_function(tolua_S,"Init",tolua_scen_scenario_ScenarioManager_Init00);
    tolua_function(tolua_S,"Update",tolua_scen_scenario_ScenarioManager_Update00);
    tolua_function(tolua_S,"UpdatePlayer",tolua_scen_scenario_ScenarioManager_UpdatePlayer00);
    tolua_function(tolua_S,"IsActive",tolua_scen_scenario_ScenarioManager_IsActive00);
    tolua_function(tolua_S,"IsScenarioComplete",tolua_scen_scenario_ScenarioManager_IsScenarioComplete00);
    tolua_function(tolua_S,"SetFlag",tolua_scen_scenario_ScenarioManager_SetFlag00);
    tolua_function(tolua_S,"HasFlag",tolua_scen_scenario_ScenarioManager_HasFlag00);
    tolua_function(tolua_S,"GetPlayerCount",tolua_scen_scenario_ScenarioManager_GetPlayerCount00);
    tolua_function(tolua_S,"GetReadyCount",tolua_scen_scenario_ScenarioManager_GetReadyCount00);
    tolua_function(tolua_S,"GetRound",tolua_scen_scenario_ScenarioManager_GetRound00);
    tolua_function(tolua_S,"GetStage",tolua_scen_scenario_ScenarioManager_GetStage00);
    tolua_function(tolua_S,"SetRound",tolua_scen_scenario_ScenarioManager_SetRound00);
    tolua_function(tolua_S,"SetStage",tolua_scen_scenario_ScenarioManager_SetStage00);
    tolua_function(tolua_S,"PlayerRelevant",tolua_scen_scenario_ScenarioManager_PlayerRelevant00);
    tolua_function(tolua_S,"PlayerRelevant",tolua_scen_scenario_ScenarioManager_PlayerRelevant01);
    tolua_function(tolua_S,"GetTeam",tolua_scen_scenario_ScenarioManager_GetTeam00);
    tolua_function(tolua_S,"GetOtherTeam",tolua_scen_scenario_ScenarioManager_GetOtherTeam00);
    tolua_function(tolua_S,"RunObject",tolua_scen_scenario_ScenarioManager_RunObject00);
    tolua_function(tolua_S,"RunObject",tolua_scen_scenario_ScenarioManager_RunObject01);
    tolua_function(tolua_S,"RunObject",tolua_scen_scenario_ScenarioManager_RunObject02);
    tolua_function(tolua_S,"RunObject",tolua_scen_scenario_ScenarioManager_RunObject03);
    tolua_function(tolua_S,"LoadObjects",tolua_scen_scenario_ScenarioManager_LoadObjects00);
    tolua_function(tolua_S,"LoadContent",tolua_scen_scenario_ScenarioManager_LoadContent00);
    tolua_function(tolua_S,"UnloadContent",tolua_scen_scenario_ScenarioManager_UnloadContent00);
    tolua_function(tolua_S,"RunEvent",tolua_scen_scenario_ScenarioManager_RunEvent00);
    tolua_function(tolua_S,"AddContent",tolua_scen_scenario_ScenarioManager_AddContent00);
    tolua_function(tolua_S,"GetInstance",tolua_scen_scenario_ScenarioManager_GetInstance00);
   tolua_endmodule(tolua_S);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_scen (lua_State* tolua_S) {
 return tolua_scen_open(tolua_S);
};
#endif


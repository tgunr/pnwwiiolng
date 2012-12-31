#include <syscfg.h> // Path is project specific.
/*
** Lua binding: tut
** Generated automatically by tolua++-1.0.92 on 09/03/11 11:16:31.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_tut_open (lua_State* tolua_S);

#include "TutorialManager.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"TutorialHandler");
 tolua_usertype(tolua_S,"TutorialManager");
}

/* get function: name of class  TutorialHandler */
#ifndef TOLUA_DISABLE_tolua_get_TutorialHandler_name
static int tolua_get_TutorialHandler_name(lua_State* tolua_S)
{
  TutorialHandler* self = (TutorialHandler*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'name'",NULL);
#endif
  tolua_pushcppstring(tolua_S,(const char*)self->name);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* set function: name of class  TutorialHandler */
#ifndef TOLUA_DISABLE_tolua_set_TutorialHandler_name
static int tolua_set_TutorialHandler_name(lua_State* tolua_S)
{
  TutorialHandler* self = (TutorialHandler*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  tolua_Error tolua_err;
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'name'",NULL);
  if (!tolua_iscppstring(tolua_S,2,0,&tolua_err))
   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->name = ((std::string)  tolua_tocppstring(tolua_S,2,0))
;
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: scope of class  TutorialHandler */
#ifndef TOLUA_DISABLE_tolua_get_TutorialHandler_scope
static int tolua_get_TutorialHandler_scope(lua_State* tolua_S)
{
  TutorialHandler* self = (TutorialHandler*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'scope'",NULL);
#endif
  tolua_pushcppstring(tolua_S,(const char*)self->scope);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* set function: scope of class  TutorialHandler */
#ifndef TOLUA_DISABLE_tolua_set_TutorialHandler_scope
static int tolua_set_TutorialHandler_scope(lua_State* tolua_S)
{
  TutorialHandler* self = (TutorialHandler*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  tolua_Error tolua_err;
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'scope'",NULL);
  if (!tolua_iscppstring(tolua_S,2,0,&tolua_err))
   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->scope = ((std::string)  tolua_tocppstring(tolua_S,2,0))
;
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: callback of class  TutorialHandler */
#ifndef TOLUA_DISABLE_tolua_get_TutorialHandler_callback
static int tolua_get_TutorialHandler_callback(lua_State* tolua_S)
{
  TutorialHandler* self = (TutorialHandler*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'callback'",NULL);
#endif
  tolua_pushcppstring(tolua_S,(const char*)self->callback);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* set function: callback of class  TutorialHandler */
#ifndef TOLUA_DISABLE_tolua_set_TutorialHandler_callback
static int tolua_set_TutorialHandler_callback(lua_State* tolua_S)
{
  TutorialHandler* self = (TutorialHandler*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  tolua_Error tolua_err;
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'callback'",NULL);
  if (!tolua_iscppstring(tolua_S,2,0,&tolua_err))
   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->callback = ((std::string)  tolua_tocppstring(tolua_S,2,0))
;
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: eventType of class  TutorialHandler */
#ifndef TOLUA_DISABLE_tolua_get_TutorialHandler_eventType
static int tolua_get_TutorialHandler_eventType(lua_State* tolua_S)
{
  TutorialHandler* self = (TutorialHandler*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'eventType'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->eventType);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* set function: eventType of class  TutorialHandler */
#ifndef TOLUA_DISABLE_tolua_set_TutorialHandler_eventType
static int tolua_set_TutorialHandler_eventType(lua_State* tolua_S)
{
  TutorialHandler* self = (TutorialHandler*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  tolua_Error tolua_err;
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'eventType'",NULL);
  if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->eventType = ((TUT_EVENT_TYPE) (int)  tolua_tonumber(tolua_S,2,0))
;
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: state of class  TutorialHandler */
#ifndef TOLUA_DISABLE_tolua_get_TutorialHandler_state
static int tolua_get_TutorialHandler_state(lua_State* tolua_S)
{
  TutorialHandler* self = (TutorialHandler*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'state'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->state);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* set function: state of class  TutorialHandler */
#ifndef TOLUA_DISABLE_tolua_set_TutorialHandler_state
static int tolua_set_TutorialHandler_state(lua_State* tolua_S)
{
  TutorialHandler* self = (TutorialHandler*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  tolua_Error tolua_err;
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'state'",NULL);
  if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->state = ((TUT_HANDLER_STATE) (int)  tolua_tonumber(tolua_S,2,0))
;
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: called of class  TutorialHandler */
#ifndef TOLUA_DISABLE_tolua_get_TutorialHandler_called
static int tolua_get_TutorialHandler_called(lua_State* tolua_S)
{
  TutorialHandler* self = (TutorialHandler*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'called'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->called);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* set function: called of class  TutorialHandler */
#ifndef TOLUA_DISABLE_tolua_set_TutorialHandler_called
static int tolua_set_TutorialHandler_called(lua_State* tolua_S)
{
  TutorialHandler* self = (TutorialHandler*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  tolua_Error tolua_err;
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'called'",NULL);
  if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->called = ((int)  tolua_tonumber(tolua_S,2,0))
;
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* method: Start of class  TutorialManager */
#ifndef TOLUA_DISABLE_tolua_tut_tutorial_TutorialManager_Start00
static int tolua_tut_tutorial_TutorialManager_Start00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"TutorialManager",0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isstring(tolua_S,3,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  TutorialManager* self = (TutorialManager*)  tolua_tousertype(tolua_S,1,0);
  const char* id = ((const char*)  tolua_tostring(tolua_S,2,0));
  const char* script = ((const char*)  tolua_tostring(tolua_S,3,NULL));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Start'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->Start(id,script);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Start'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Stop of class  TutorialManager */
#ifndef TOLUA_DISABLE_tolua_tut_tutorial_TutorialManager_Stop00
static int tolua_tut_tutorial_TutorialManager_Stop00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"TutorialManager",0,&tolua_err) ||
     !tolua_isboolean(tolua_S,2,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  TutorialManager* self = (TutorialManager*)  tolua_tousertype(tolua_S,1,0);
  bool delayed = ((bool)  tolua_toboolean(tolua_S,2,true));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Stop'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->Stop(delayed);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Stop'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: AddHandler of class  TutorialManager */
#ifndef TOLUA_DISABLE_tolua_tut_tutorial_TutorialManager_AddHandler00
static int tolua_tut_tutorial_TutorialManager_AddHandler00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"TutorialManager",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isstring(tolua_S,3,0,&tolua_err) ||
     !tolua_isstring(tolua_S,4,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,5,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  TutorialManager* self = (TutorialManager*)  tolua_tousertype(tolua_S,1,0);
  unsigned int eventType = (( unsigned int)  tolua_tonumber(tolua_S,2,0));
  const char* callback = ((const char*)  tolua_tostring(tolua_S,3,0));
  const char* name = ((const char*)  tolua_tostring(tolua_S,4,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'AddHandler'",NULL);
#endif
  {
   TutorialHandler* tolua_ret = (TutorialHandler*)  self->AddHandler(eventType,callback,name);
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"TutorialHandler");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'AddHandler'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: AddTimedHandler of class  TutorialManager */
#ifndef TOLUA_DISABLE_tolua_tut_tutorial_TutorialManager_AddTimedHandler00
static int tolua_tut_tutorial_TutorialManager_AddTimedHandler00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"TutorialManager",0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isstring(tolua_S,3,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,4,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,5,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  TutorialManager* self = (TutorialManager*)  tolua_tousertype(tolua_S,1,0);
  const char* callback = ((const char*)  tolua_tostring(tolua_S,2,0));
  const char* name = ((const char*)  tolua_tostring(tolua_S,3,0));
  int duration = ((int)  tolua_tonumber(tolua_S,4,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'AddTimedHandler'",NULL);
#endif
  {
   TutorialHandler* tolua_ret = (TutorialHandler*)  self->AddTimedHandler(callback,name,duration);
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"TutorialHandler");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'AddTimedHandler'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: AddLocationHandler of class  TutorialManager */
#ifndef TOLUA_DISABLE_tolua_tut_tutorial_TutorialManager_AddLocationHandler00
static int tolua_tut_tutorial_TutorialManager_AddLocationHandler00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"TutorialManager",0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isstring(tolua_S,3,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,4,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,5,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,6,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,7,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,8,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  TutorialManager* self = (TutorialManager*)  tolua_tousertype(tolua_S,1,0);
  const char* callback = ((const char*)  tolua_tostring(tolua_S,2,0));
  const char* name = ((const char*)  tolua_tostring(tolua_S,3,0));
  double x = ((double)  tolua_tonumber(tolua_S,4,0));
  double y = ((double)  tolua_tonumber(tolua_S,5,0));
  double z = ((double)  tolua_tonumber(tolua_S,6,0));
  float d = ((float)  tolua_tonumber(tolua_S,7,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'AddLocationHandler'",NULL);
#endif
  {
   TutorialHandler* tolua_ret = (TutorialHandler*)  self->AddLocationHandler(callback,name,x,y,z,d);
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"TutorialHandler");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'AddLocationHandler'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: AddAreaHandler of class  TutorialManager */
#ifndef TOLUA_DISABLE_tolua_tut_tutorial_TutorialManager_AddAreaHandler00
static int tolua_tut_tutorial_TutorialManager_AddAreaHandler00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"TutorialManager",0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isstring(tolua_S,3,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,4,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,5,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,6,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,7,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,8,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,9,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,10,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  TutorialManager* self = (TutorialManager*)  tolua_tousertype(tolua_S,1,0);
  const char* callback = ((const char*)  tolua_tostring(tolua_S,2,0));
  const char* name = ((const char*)  tolua_tostring(tolua_S,3,0));
  double x = ((double)  tolua_tonumber(tolua_S,4,0));
  double y = ((double)  tolua_tonumber(tolua_S,5,0));
  double z = ((double)  tolua_tonumber(tolua_S,6,0));
  double x2 = ((double)  tolua_tonumber(tolua_S,7,0));
  double y2 = ((double)  tolua_tonumber(tolua_S,8,0));
  double z2 = ((double)  tolua_tonumber(tolua_S,9,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'AddAreaHandler'",NULL);
#endif
  {
   TutorialHandler* tolua_ret = (TutorialHandler*)  self->AddAreaHandler(callback,name,x,y,z,x2,y2,z2);
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"TutorialHandler");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'AddAreaHandler'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: RemoveHandler of class  TutorialManager */
#ifndef TOLUA_DISABLE_tolua_tut_tutorial_TutorialManager_RemoveHandler00
static int tolua_tut_tutorial_TutorialManager_RemoveHandler00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"TutorialManager",0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  TutorialManager* self = (TutorialManager*)  tolua_tousertype(tolua_S,1,0);
  const char* name = ((const char*)  tolua_tostring(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'RemoveHandler'",NULL);
#endif
  {
   self->RemoveHandler(name);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'RemoveHandler'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: RemoveHandlers of class  TutorialManager */
#ifndef TOLUA_DISABLE_tolua_tut_tutorial_TutorialManager_RemoveHandlers00
static int tolua_tut_tutorial_TutorialManager_RemoveHandlers00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"TutorialManager",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  TutorialManager* self = (TutorialManager*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'RemoveHandlers'",NULL);
#endif
  {
   self->RemoveHandlers();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'RemoveHandlers'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetComplete of class  TutorialManager */
#ifndef TOLUA_DISABLE_tolua_tut_tutorial_TutorialManager_SetComplete00
static int tolua_tut_tutorial_TutorialManager_SetComplete00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"TutorialManager",0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isboolean(tolua_S,4,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,5,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  TutorialManager* self = (TutorialManager*)  tolua_tousertype(tolua_S,1,0);
  const char* id = ((const char*)  tolua_tostring(tolua_S,2,0));
  int key = ((int)  tolua_tonumber(tolua_S,3,0));
  bool completed = ((bool)  tolua_toboolean(tolua_S,4,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetComplete'",NULL);
#endif
  {
   self->SetComplete(id,key,completed);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetComplete'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: IsComplete of class  TutorialManager */
#ifndef TOLUA_DISABLE_tolua_tut_tutorial_TutorialManager_IsComplete00
static int tolua_tut_tutorial_TutorialManager_IsComplete00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"TutorialManager",0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  TutorialManager* self = (TutorialManager*)  tolua_tousertype(tolua_S,1,0);
  const char* id = ((const char*)  tolua_tostring(tolua_S,2,0));
  int key = ((int)  tolua_tonumber(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'IsComplete'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->IsComplete(id,key);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'IsComplete'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: AllowStrat of class  TutorialManager */
#ifndef TOLUA_DISABLE_tolua_tut_tutorial_TutorialManager_AllowStrat00
static int tolua_tut_tutorial_TutorialManager_AllowStrat00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const TutorialManager",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const TutorialManager* self = (const TutorialManager*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'AllowStrat'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->AllowStrat();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'AllowStrat'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetStratAllow of class  TutorialManager */
#ifndef TOLUA_DISABLE_tolua_tut_tutorial_TutorialManager_SetStratAllow00
static int tolua_tut_tutorial_TutorialManager_SetStratAllow00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"TutorialManager",0,&tolua_err) ||
     !tolua_isboolean(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  TutorialManager* self = (TutorialManager*)  tolua_tousertype(tolua_S,1,0);
  bool allow = ((bool)  tolua_toboolean(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetStratAllow'",NULL);
#endif
  {
   self->SetStratAllow(allow);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetStratAllow'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetInstance of class  TutorialManager */
#ifndef TOLUA_DISABLE_tolua_tut_tutorial_TutorialManager_GetInstance00
static int tolua_tut_tutorial_TutorialManager_GetInstance00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertable(tolua_S,1,"TutorialManager",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  {
   TutorialManager* tolua_ret = (TutorialManager*)  TutorialManager::GetInstance();
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"TutorialManager");
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
TOLUA_API int tolua_tut_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_module(tolua_S,"tutorial",0);
  tolua_beginmodule(tolua_S,"tutorial");
   tolua_constant(tolua_S,"TUT_EVENT_GENERAL",TUT_EVENT_GENERAL);
   tolua_constant(tolua_S,"TUT_EVENT_START",TUT_EVENT_START);
   tolua_constant(tolua_S,"TUT_EVENT_STOP",TUT_EVENT_STOP);
   tolua_constant(tolua_S,"TUT_EVENT_SPAWN",TUT_EVENT_SPAWN);
   tolua_constant(tolua_S,"TUT_EVENT_DESPAWN",TUT_EVENT_DESPAWN);
   tolua_constant(tolua_S,"TUT_EVENT_INPUT",TUT_EVENT_INPUT);
   tolua_constant(tolua_S,"TUT_EVENT_STATE_TROOP",TUT_EVENT_STATE_TROOP);
   tolua_constant(tolua_S,"TUT_EVENT_STATE_VEHICLE",TUT_EVENT_STATE_VEHICLE);
   tolua_constant(tolua_S,"TUT_EVENT_WEAPON_FIRE",TUT_EVENT_WEAPON_FIRE);
   tolua_constant(tolua_S,"TUT_EVENT_WEAPON_SWITCH",TUT_EVENT_WEAPON_SWITCH);
   tolua_constant(tolua_S,"TUT_EVENT_LOCATION",TUT_EVENT_LOCATION);
   tolua_constant(tolua_S,"TUT_EVENT_COLLISION",TUT_EVENT_COLLISION);
   tolua_constant(tolua_S,"TUT_EVENT_HIT",TUT_EVENT_HIT);
   tolua_constant(tolua_S,"TUT_EVENT_TIMED",TUT_EVENT_TIMED);
   tolua_constant(tolua_S,"TUT_EVENT_ENTER",TUT_EVENT_ENTER);
   tolua_constant(tolua_S,"TUT_EVENT_LEAVE",TUT_EVENT_LEAVE);
   tolua_constant(tolua_S,"TUT_EVENT_UPDATE",TUT_EVENT_UPDATE);
   tolua_constant(tolua_S,"TUT_EVENT_FPS_POSE",TUT_EVENT_FPS_POSE);
   tolua_constant(tolua_S,"TUT_EVENT_UI_POPUP",TUT_EVENT_UI_POPUP);
   tolua_constant(tolua_S,"TUT_EVENT_UI_DESTROYED",TUT_EVENT_UI_DESTROYED);
   tolua_constant(tolua_S,"TUT_EVENT_UI_READY",TUT_EVENT_UI_READY);
   tolua_constant(tolua_S,"TUT_EVENT_KEY",TUT_EVENT_KEY);
   tolua_constant(tolua_S,"TUT_EVENT_MAP",TUT_EVENT_MAP);
   tolua_constant(tolua_S,"TUT_EVENT_CHANNEL",TUT_EVENT_CHANNEL);
   tolua_constant(tolua_S,"TUT_EVENT_CHAT",TUT_EVENT_CHAT);
   tolua_constant(tolua_S,"TUT_EVENT_DIED",TUT_EVENT_DIED);
   tolua_constant(tolua_S,"TUT_EVENT_AREA",TUT_EVENT_AREA);
   tolua_constant(tolua_S,"TUT_EVENT_DAMAGE",TUT_EVENT_DAMAGE);
   tolua_constant(tolua_S,"TUT_EVENT_CAMERA",TUT_EVENT_CAMERA);
   tolua_constant(tolua_S,"TUT_EVENT_MESSAGE",TUT_EVENT_MESSAGE);
   tolua_constant(tolua_S,"TUT_EVENT_SET",TUT_EVENT_SET);
   tolua_constant(tolua_S,"TUT_EVENT_NUM_TYPES",TUT_EVENT_NUM_TYPES);
   tolua_constant(tolua_S,"TUT_MAP_INVALID",TUT_MAP_INVALID);
   tolua_constant(tolua_S,"TUT_MAP_CENTERME",TUT_MAP_CENTERME);
   tolua_constant(tolua_S,"TUT_MAP_ZOOMIN",TUT_MAP_ZOOMIN);
   tolua_constant(tolua_S,"TUT_MAP_ZOOMOUT",TUT_MAP_ZOOMOUT);
   tolua_constant(tolua_S,"TUT_MAP_DRAG",TUT_MAP_DRAG);
   tolua_constant(tolua_S,"TUT_MAP_WAYPOINT",TUT_MAP_WAYPOINT);
   tolua_constant(tolua_S,"TUT_MAP_REPORT",TUT_MAP_REPORT);
   tolua_constant(tolua_S,"TUT_MAP_TOGGLE",TUT_MAP_TOGGLE);
   tolua_constant(tolua_S,"TUT_MAP_ACTIVATE",TUT_MAP_ACTIVATE);
   tolua_constant(tolua_S,"TUT_MAP_NUM",TUT_MAP_NUM);
   tolua_constant(tolua_S,"TUTORIAL_UI_EVENTS",TUTORIAL_UI_EVENTS);
   tolua_constant(tolua_S,"TUTORIAL_UPDATE_EVENTS",TUTORIAL_UPDATE_EVENTS);
   tolua_constant(tolua_S,"THS_GARBAGE",THS_GARBAGE);
   tolua_constant(tolua_S,"THS_START",THS_START);
   tolua_constant(tolua_S,"THS_EVENT_IN",THS_EVENT_IN);
   tolua_constant(tolua_S,"THS_EVENT_OUT",THS_EVENT_OUT);
   tolua_cclass(tolua_S,"TutorialHandler","TutorialHandler","",NULL);
   tolua_beginmodule(tolua_S,"TutorialHandler");
    tolua_variable(tolua_S,"name",tolua_get_TutorialHandler_name,tolua_set_TutorialHandler_name);
    tolua_variable(tolua_S,"scope",tolua_get_TutorialHandler_scope,tolua_set_TutorialHandler_scope);
    tolua_variable(tolua_S,"callback",tolua_get_TutorialHandler_callback,tolua_set_TutorialHandler_callback);
    tolua_variable(tolua_S,"eventType",tolua_get_TutorialHandler_eventType,tolua_set_TutorialHandler_eventType);
    tolua_variable(tolua_S,"state",tolua_get_TutorialHandler_state,tolua_set_TutorialHandler_state);
    tolua_variable(tolua_S,"called",tolua_get_TutorialHandler_called,tolua_set_TutorialHandler_called);
   tolua_endmodule(tolua_S);
   tolua_cclass(tolua_S,"TutorialManager","TutorialManager","",NULL);
   tolua_beginmodule(tolua_S,"TutorialManager");
    tolua_function(tolua_S,"Start",tolua_tut_tutorial_TutorialManager_Start00);
    tolua_function(tolua_S,"Stop",tolua_tut_tutorial_TutorialManager_Stop00);
    tolua_function(tolua_S,"AddHandler",tolua_tut_tutorial_TutorialManager_AddHandler00);
    tolua_function(tolua_S,"AddTimedHandler",tolua_tut_tutorial_TutorialManager_AddTimedHandler00);
    tolua_function(tolua_S,"AddLocationHandler",tolua_tut_tutorial_TutorialManager_AddLocationHandler00);
    tolua_function(tolua_S,"AddAreaHandler",tolua_tut_tutorial_TutorialManager_AddAreaHandler00);
    tolua_function(tolua_S,"RemoveHandler",tolua_tut_tutorial_TutorialManager_RemoveHandler00);
    tolua_function(tolua_S,"RemoveHandlers",tolua_tut_tutorial_TutorialManager_RemoveHandlers00);
    tolua_function(tolua_S,"SetComplete",tolua_tut_tutorial_TutorialManager_SetComplete00);
    tolua_function(tolua_S,"IsComplete",tolua_tut_tutorial_TutorialManager_IsComplete00);
    tolua_function(tolua_S,"AllowStrat",tolua_tut_tutorial_TutorialManager_AllowStrat00);
    tolua_function(tolua_S,"SetStratAllow",tolua_tut_tutorial_TutorialManager_SetStratAllow00);
    tolua_function(tolua_S,"GetInstance",tolua_tut_tutorial_TutorialManager_GetInstance00);
   tolua_endmodule(tolua_S);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_tut (lua_State* tolua_S) {
 return tolua_tut_open(tolua_S);
};
#endif


#include "syscfg.h" // Path is project specific.
/*
** Lua binding: ob
** Generated automatically by tolua++-1.0.92 on 08/22/08 16:08:07.
*/

#ifndef __cplusplus
#include <stdlib.h>
#endif
#include <string.h>

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_ob_open (lua_State* tolua_S);

#include "ob/obmain.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"OBJECT");
}

/* get function: name of class  OBJECT */
#ifndef TOLUA_DISABLE_tolua_get_OBJECT_name
static int tolua_get_OBJECT_name(lua_State* tolua_S)
{
  OBJECT* self = (OBJECT*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'name'",NULL);
#endif
  tolua_pushstring(tolua_S,(const char*)self->name);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: matname of class  OBJECT */
#ifndef TOLUA_DISABLE_tolua_get_OBJECT_matname
static int tolua_get_OBJECT_matname(lua_State* tolua_S)
{
  OBJECT* self = (OBJECT*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'matname'",NULL);
#endif
  tolua_pushstring(tolua_S,(const char*)self->matname);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: texname of class  OBJECT */
#ifndef TOLUA_DISABLE_tolua_get_OBJECT_texname
static int tolua_get_OBJECT_texname(lua_State* tolua_S)
{
  OBJECT* self = (OBJECT*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'texname'",NULL);
#endif
  tolua_pushstring(tolua_S,(const char*)self->texname);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_ob_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_module(tolua_S,"ob",0);
  tolua_beginmodule(tolua_S,"ob");
   tolua_cclass(tolua_S,"OBJECT","OBJECT","",NULL);
   tolua_beginmodule(tolua_S,"OBJECT");
    tolua_variable(tolua_S,"name",tolua_get_OBJECT_name,NULL);
    tolua_variable(tolua_S,"matname",tolua_get_OBJECT_matname,NULL);
    tolua_variable(tolua_S,"texname",tolua_get_OBJECT_texname,NULL);
   tolua_endmodule(tolua_S);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_ob (lua_State* tolua_S) {
 return tolua_ob_open(tolua_S);
};
#endif


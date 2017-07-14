#include "syscfg.h" // Path is project specific.
/*
** Lua binding: cfg
** Generated automatically by tolua++-1.0.92 on 01/04/14 22:00:08.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_cfg_open (lua_State* tolua_S);

#include "cfg/cfgPrefs.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
}

/* function: cfgPrefsSave */
#ifndef TOLUA_DISABLE_tolua_cfg_cfg_cfgPrefsSave00
static int tolua_cfg_cfg_cfgPrefsSave00(lua_State* tolua_S)
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
   cfgPrefsSave();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'cfgPrefsSave'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_cfg_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_module(tolua_S,"cfg",0);
  tolua_beginmodule(tolua_S,"cfg");
   tolua_function(tolua_S,"cfgPrefsSave",tolua_cfg_cfg_cfgPrefsSave00);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_cfg (lua_State* tolua_S) {
 return tolua_cfg_open(tolua_S);
};
#endif


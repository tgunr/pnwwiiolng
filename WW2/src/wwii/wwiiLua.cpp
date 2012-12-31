#include <syscfg.h> // Path is project specific.
/*
** Lua binding: wwii
** Generated automatically by tolua++-1.0.92 on 06/10/08 19:47:10.
*/

#ifndef __cplusplus
#include <stdlib.h>
#endif
#include <string.h>

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_wwii_open (lua_State* tolua_S);

#include "wwii/wwiiOwnershipEnums.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
}

/* function: wwiiSide */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_wwiiSide00
static int tolua_wwii_wwii_wwiiSide00(lua_State* tolua_S)
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
  unsigned int country = (( unsigned int)  tolua_tonumber(tolua_S,1,0));
  {
   WWII_SIDE tolua_ret = (WWII_SIDE)  wwiiSide(country);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'wwiiSide'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: wwiiOtherSide */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_wwiiOtherSide00
static int tolua_wwii_wwii_wwiiOtherSide00(lua_State* tolua_S)
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
  unsigned int side = (( unsigned int)  tolua_tonumber(tolua_S,1,0));
  {
   WWII_SIDE tolua_ret = (WWII_SIDE)  wwiiOtherSide(side);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'wwiiOtherSide'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_wwii_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_module(tolua_S,"wwii",0);
  tolua_beginmodule(tolua_S,"wwii");
   tolua_constant(tolua_S,"WWII_NO_COUNTRY",WWII_NO_COUNTRY);
   tolua_constant(tolua_S,"WWII_ENGLAND",WWII_ENGLAND);
   tolua_constant(tolua_S,"WWII_USA",WWII_USA);
   tolua_constant(tolua_S,"WWII_FRANCE",WWII_FRANCE);
   tolua_constant(tolua_S,"WWII_GERMANY",WWII_GERMANY);
   tolua_constant(tolua_S,"WWII_ITALY",WWII_ITALY);
   tolua_constant(tolua_S,"WWII_CHINA",WWII_CHINA);
   tolua_constant(tolua_S,"WWII_RUSSIA",WWII_RUSSIA);
   tolua_constant(tolua_S,"MAX_COUNTRIES",MAX_COUNTRIES);
   tolua_constant(tolua_S,"WWII_NO_SIDE",WWII_NO_SIDE);
   tolua_constant(tolua_S,"ALLIED",ALLIED);
   tolua_constant(tolua_S,"AXIS",AXIS);
   tolua_constant(tolua_S,"NEUTRAL",NEUTRAL);
   tolua_constant(tolua_S,"MAX_SIDES",MAX_SIDES);
   tolua_function(tolua_S,"wwiiSide",tolua_wwii_wwii_wwiiSide00);
   tolua_function(tolua_S,"wwiiOtherSide",tolua_wwii_wwii_wwiiOtherSide00);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_wwii (lua_State* tolua_S) {
 return tolua_wwii_open(tolua_S);
};
#endif


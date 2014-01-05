#include <syscfg.h> // Path is project specific.
/*
** Lua binding: ui
** Generated automatically by tolua++-1.0.92 on 01/04/14 22:00:09.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_ui_open (lua_State* tolua_S);

#include "ui/uiSound.h"
#include "ui/uiChat.h"
#include "chat/chatChannelTypes.h"
#include "ui/uiGameEvents.h"
#include "phui/View.h"
#include "ui/uiMain.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"std::wstring");
 tolua_usertype(tolua_S,"phui::View");
}

/* function: uiSetDespawnInfo */
#ifndef TOLUA_DISABLE_tolua_ui_ui_uiSetDespawnInfo00
static int tolua_ui_ui_uiSetDespawnInfo00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isstring(tolua_S,1,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,1,&tolua_err) ||
     !tolua_isboolean(tolua_S,3,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const char* screen = ((const char*)  tolua_tostring(tolua_S,1,0));
  int tab = ((int)  tolua_tonumber(tolua_S,2,0));
  bool reset = ((bool)  tolua_toboolean(tolua_S,3,true));
  {
   uiSetDespawnInfo(screen,tab,reset);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiSetDespawnInfo'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiInGame */
#ifndef TOLUA_DISABLE_tolua_ui_ui_uiInGame00
static int tolua_ui_ui_uiInGame00(lua_State* tolua_S)
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
   bool tolua_ret = (bool)  uiInGame();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiInGame'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiRunScript */
#ifndef TOLUA_DISABLE_tolua_ui_ui_uiRunScript00
static int tolua_ui_ui_uiRunScript00(lua_State* tolua_S)
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
  const char* script = ((const char*)  tolua_tostring(tolua_S,1,0));
  {
   uiRunScript(script);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiRunScript'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiRunScript */
#ifndef TOLUA_DISABLE_tolua_ui_ui_uiRunScript01
static int tolua_ui_ui_uiRunScript01(lua_State* tolua_S)
{
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
 {
  phui::View* target = ((phui::View*)  tolua_tousertype(tolua_S,1,0));
  const char* script = ((const char*)  tolua_tostring(tolua_S,2,0));
  {
   uiRunScript(target,script);
  }
 }
 return 0;
tolua_lerror:
 return tolua_ui_ui_uiRunScript00(tolua_S);
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiSetCursor */
#ifndef TOLUA_DISABLE_tolua_ui_ui_uiSetCursor00
static int tolua_ui_ui_uiSetCursor00(lua_State* tolua_S)
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
  int tolua_var_1 = ((int)  tolua_tonumber(tolua_S,1,0));
  {
   uiSetCursor(tolua_var_1);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiSetCursor'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiLocString */
#ifndef TOLUA_DISABLE_tolua_ui_ui_uiLocString00
static int tolua_ui_ui_uiLocString00(lua_State* tolua_S)
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
  const char* tolua_var_2 = ((const char*)  tolua_tostring(tolua_S,1,0));
  {
   const std::wstring& tolua_ret = (const std::wstring&)  uiLocString(tolua_var_2);
   tolua_pushusertype(tolua_S,(void*)&tolua_ret,"const std::wstring");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiLocString'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiLocString2 */
#ifndef TOLUA_DISABLE_tolua_ui_ui_uiLocString200
static int tolua_ui_ui_uiLocString200(lua_State* tolua_S)
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
  const char* tolua_var_3 = ((const char*)  tolua_tostring(tolua_S,1,0));
  {
   const std::string tolua_ret = (const std::string)  uiLocString2(tolua_var_3);
   tolua_pushcppstring(tolua_S,(const char*)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiLocString2'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiLocString2 */
#ifndef TOLUA_DISABLE_tolua_ui_ui_uiLocString201
static int tolua_ui_ui_uiLocString201(lua_State* tolua_S)
{
 tolua_Error tolua_err;
 if (
     !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
 {
  int tolua_var_4 = ((int)  tolua_tonumber(tolua_S,1,0));
  int tolua_var_5 = ((int)  tolua_tonumber(tolua_S,2,0));
  {
   const std::string tolua_ret = (const std::string)  uiLocString2(tolua_var_4,tolua_var_5);
   tolua_pushcppstring(tolua_S,(const char*)tolua_ret);
  }
 }
 return 1;
tolua_lerror:
 return tolua_ui_ui_uiLocString200(tolua_S);
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiHasLocString */
#ifndef TOLUA_DISABLE_tolua_ui_ui_uiHasLocString00
static int tolua_ui_ui_uiHasLocString00(lua_State* tolua_S)
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
  const char* tolua_var_6 = ((const char*)  tolua_tostring(tolua_S,1,0));
  {
   bool tolua_ret = (bool)  uiHasLocString(tolua_var_6);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiHasLocString'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiGameEvent */
#ifndef TOLUA_DISABLE_tolua_ui_ui_uiGameEvent00
static int tolua_ui_ui_uiGameEvent00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  int event = ((int)  tolua_tonumber(tolua_S,1,0));
  const char* message = ((const char*)  tolua_tostring(tolua_S,2,0));
  {
   int tolua_ret = (int)  uiGameEvent(event,message);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiGameEvent'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiPlaySound */
#ifndef TOLUA_DISABLE_tolua_ui_ui_uiPlaySound00
static int tolua_ui_ui_uiPlaySound00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isstring(tolua_S,1,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const char* file = ((const char*)  tolua_tostring(tolua_S,1,0));
  unsigned int finite = ((unsigned int)  tolua_tonumber(tolua_S,2,0));
  double volume = ((double)  tolua_tonumber(tolua_S,3,0));
  {
   int tolua_ret = (int)  uiPlaySound(file,finite,volume);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiPlaySound'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiChatPrintLine */
#ifndef TOLUA_DISABLE_tolua_ui_ui_uiChatPrintLine00
static int tolua_ui_ui_uiChatPrintLine00(lua_State* tolua_S)
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
  const char* message = ((const char*)  tolua_tostring(tolua_S,1,0));
  {
   int tolua_ret = (int)  uiChatPrintLine(message);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiChatPrintLine'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiConsoleprintf */
#ifndef TOLUA_DISABLE_tolua_ui_ui_uiConsoleprintf00
static int tolua_ui_ui_uiConsoleprintf00(lua_State* tolua_S)
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
  const char* text = ((const char*)  tolua_tostring(tolua_S,1,0));
  {
   int tolua_ret = (int)  uiConsoleprintf(text);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiConsoleprintf'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiLoadView */
#ifndef TOLUA_DISABLE_tolua_ui_ui_uiLoadView00
static int tolua_ui_ui_uiLoadView00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isstring(tolua_S,1,0,&tolua_err) ||
     !tolua_isboolean(tolua_S,2,0,&tolua_err) ||
     !tolua_isboolean(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const char* filename = ((const char*)  tolua_tostring(tolua_S,1,0));
  bool addtoviewport = ((bool)  tolua_toboolean(tolua_S,2,0));
  bool show = ((bool)  tolua_toboolean(tolua_S,3,0));
  {
   phui::View* tolua_ret = (phui::View*)  uiLoadView(filename,addtoviewport,show);
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"phui::View");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiLoadView'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiFindView */
#ifndef TOLUA_DISABLE_tolua_ui_ui_uiFindView00
static int tolua_ui_ui_uiFindView00(lua_State* tolua_S)
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
   phui::View* tolua_ret = (phui::View*)  uiFindView(name);
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"phui::View");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiFindView'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiChatSetRadio */
#ifndef TOLUA_DISABLE_tolua_ui_ui_uiChatSetRadio00
static int tolua_ui_ui_uiChatSetRadio00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  unsigned int radioIndex = ((unsigned int)  tolua_tonumber(tolua_S,1,0));
  unsigned int type = ((unsigned int)  tolua_tonumber(tolua_S,2,0));
  {
   uiChatSetRadio(radioIndex,type);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiChatSetRadio'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: uiChatGetRadio */
#ifndef TOLUA_DISABLE_tolua_ui_ui_uiChatGetRadio00
static int tolua_ui_ui_uiChatGetRadio00(lua_State* tolua_S)
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
  unsigned int radioIndex = ((unsigned int)  tolua_tonumber(tolua_S,1,0));
  {
   unsigned int tolua_ret = (unsigned int)  uiChatGetRadio(radioIndex);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'uiChatGetRadio'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Parent of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_Parent00
static int tolua_ui_ui_phui_View_Parent00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const phui::View",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const phui::View* self = (const phui::View*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Parent'",NULL);
#endif
  {
   phui::View* tolua_ret = (phui::View*)  self->Parent();
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"phui::View");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Parent'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ChildAt of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_ChildAt00
static int tolua_ui_ui_phui_View_ChildAt00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const phui::View",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const phui::View* self = (const phui::View*)  tolua_tousertype(tolua_S,1,0);
  unsigned int tolua_var_7 = ((unsigned int)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ChildAt'",NULL);
#endif
  {
   phui::View* tolua_ret = (phui::View*)  self->ChildAt(tolua_var_7);
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"phui::View");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ChildAt'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: FindView of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_FindView00
static int tolua_ui_ui_phui_View_FindView00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const phui::View",0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const phui::View* self = (const phui::View*)  tolua_tousertype(tolua_S,1,0);
  const char* name = ((const char*)  tolua_tostring(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'FindView'",NULL);
#endif
  {
   phui::View* tolua_ret = (phui::View*)  self->FindView(name);
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"phui::View");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'FindView'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: FindSibling of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_FindSibling00
static int tolua_ui_ui_phui_View_FindSibling00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const phui::View",0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const phui::View* self = (const phui::View*)  tolua_tousertype(tolua_S,1,0);
  const char* name = ((const char*)  tolua_tostring(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'FindSibling'",NULL);
#endif
  {
   phui::View* tolua_ret = (phui::View*)  self->FindSibling(name);
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"phui::View");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'FindSibling'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Width of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_Width00
static int tolua_ui_ui_phui_View_Width00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const phui::View",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const phui::View* self = (const phui::View*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Width'",NULL);
#endif
  {
   float tolua_ret = (float)  self->Width();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Width'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Height of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_Height00
static int tolua_ui_ui_phui_View_Height00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const phui::View",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const phui::View* self = (const phui::View*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Height'",NULL);
#endif
  {
   float tolua_ret = (float)  self->Height();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Height'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: X of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_X00
static int tolua_ui_ui_phui_View_X00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const phui::View",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const phui::View* self = (const phui::View*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'X'",NULL);
#endif
  {
   float tolua_ret = (float)  self->X();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'X'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Y of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_Y00
static int tolua_ui_ui_phui_View_Y00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const phui::View",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const phui::View* self = (const phui::View*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Y'",NULL);
#endif
  {
   float tolua_ret = (float)  self->Y();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Y'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Left of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_Left00
static int tolua_ui_ui_phui_View_Left00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Left'",NULL);
#endif
  {
   float tolua_ret = (float)  self->Left();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Left'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Right of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_Right00
static int tolua_ui_ui_phui_View_Right00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Right'",NULL);
#endif
  {
   float tolua_ret = (float)  self->Right();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Right'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Bottom of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_Bottom00
static int tolua_ui_ui_phui_View_Bottom00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Bottom'",NULL);
#endif
  {
   float tolua_ret = (float)  self->Bottom();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Bottom'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Top of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_Top00
static int tolua_ui_ui_phui_View_Top00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Top'",NULL);
#endif
  {
   float tolua_ret = (float)  self->Top();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Top'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetLeft of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_SetLeft00
static int tolua_ui_ui_phui_View_SetLeft00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
  float left = ((float)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetLeft'",NULL);
#endif
  {
   self->SetLeft(left);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetLeft'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetRight of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_SetRight00
static int tolua_ui_ui_phui_View_SetRight00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
  float right = ((float)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetRight'",NULL);
#endif
  {
   self->SetRight(right);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetRight'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetBottom of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_SetBottom00
static int tolua_ui_ui_phui_View_SetBottom00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
  float bottom = ((float)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetBottom'",NULL);
#endif
  {
   self->SetBottom(bottom);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetBottom'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetTop of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_SetTop00
static int tolua_ui_ui_phui_View_SetTop00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
  float top = ((float)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetTop'",NULL);
#endif
  {
   self->SetTop(top);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetTop'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ResizeToAspect of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_ResizeToAspect00
static int tolua_ui_ui_phui_View_ResizeToAspect00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
  int follow = ((int)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ResizeToAspect'",NULL);
#endif
  {
   self->ResizeToAspect(follow);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ResizeToAspect'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ResizeTo of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_ResizeTo00
static int tolua_ui_ui_phui_View_ResizeTo00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
  float width = ((float)  tolua_tonumber(tolua_S,2,0));
  float height = ((float)  tolua_tonumber(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ResizeTo'",NULL);
#endif
  {
   self->ResizeTo(width,height);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ResizeTo'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ResizeBy of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_ResizeBy00
static int tolua_ui_ui_phui_View_ResizeBy00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
  float dw = ((float)  tolua_tonumber(tolua_S,2,0));
  float dh = ((float)  tolua_tonumber(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ResizeBy'",NULL);
#endif
  {
   self->ResizeBy(dw,dh);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ResizeBy'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: MoveTo of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_MoveTo00
static int tolua_ui_ui_phui_View_MoveTo00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
  float x = ((float)  tolua_tonumber(tolua_S,2,0));
  float y = ((float)  tolua_tonumber(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'MoveTo'",NULL);
#endif
  {
   self->MoveTo(x,y);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'MoveTo'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: MoveBy of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_MoveBy00
static int tolua_ui_ui_phui_View_MoveBy00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
  float dx = ((float)  tolua_tonumber(tolua_S,2,0));
  float dy = ((float)  tolua_tonumber(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'MoveBy'",NULL);
#endif
  {
   self->MoveBy(dx,dy);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'MoveBy'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ScrollTo of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_ScrollTo00
static int tolua_ui_ui_phui_View_ScrollTo00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
  float x = ((float)  tolua_tonumber(tolua_S,2,0));
  float y = ((float)  tolua_tonumber(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ScrollTo'",NULL);
#endif
  {
   self->ScrollTo(x,y);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ScrollTo'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ScrollBy of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_ScrollBy00
static int tolua_ui_ui_phui_View_ScrollBy00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
  float dx = ((float)  tolua_tonumber(tolua_S,2,0));
  float dy = ((float)  tolua_tonumber(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ScrollBy'",NULL);
#endif
  {
   self->ScrollBy(dx,dy);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ScrollBy'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetBackgroundColor of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_SetBackgroundColor00
static int tolua_ui_ui_phui_View_SetBackgroundColor00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,4,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,5,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,6,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
  float r = ((float)  tolua_tonumber(tolua_S,2,0));
  float g = ((float)  tolua_tonumber(tolua_S,3,0));
  float b = ((float)  tolua_tonumber(tolua_S,4,0));
  float a = ((float)  tolua_tonumber(tolua_S,5,1.0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetBackgroundColor'",NULL);
#endif
  {
   self->SetBackgroundColor(r,g,b,a);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetBackgroundColor'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetBorderColor of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_SetBorderColor00
static int tolua_ui_ui_phui_View_SetBorderColor00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,4,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,5,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,6,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
  float r = ((float)  tolua_tonumber(tolua_S,2,0));
  float g = ((float)  tolua_tonumber(tolua_S,3,0));
  float b = ((float)  tolua_tonumber(tolua_S,4,0));
  float a = ((float)  tolua_tonumber(tolua_S,5,1.0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetBorderColor'",NULL);
#endif
  {
   self->SetBorderColor(r,g,b,a);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetBorderColor'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetTextColor of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_SetTextColor00
static int tolua_ui_ui_phui_View_SetTextColor00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,4,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,5,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,6,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
  float r = ((float)  tolua_tonumber(tolua_S,2,0));
  float g = ((float)  tolua_tonumber(tolua_S,3,0));
  float b = ((float)  tolua_tonumber(tolua_S,4,0));
  float a = ((float)  tolua_tonumber(tolua_S,5,1.0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetTextColor'",NULL);
#endif
  {
   self->SetTextColor(r,g,b,a);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetTextColor'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Show of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_Show00
static int tolua_ui_ui_phui_View_Show00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Show'",NULL);
#endif
  {
   self->Show();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Show'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Hide of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_Hide00
static int tolua_ui_ui_phui_View_Hide00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Hide'",NULL);
#endif
  {
   self->Hide();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Hide'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: IsVisible of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_IsVisible00
static int tolua_ui_ui_phui_View_IsVisible00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const phui::View",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const phui::View* self = (const phui::View*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'IsVisible'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->IsVisible();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'IsVisible'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetEnable of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_SetEnable00
static int tolua_ui_ui_phui_View_SetEnable00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isboolean(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
  bool enable = ((bool)  tolua_toboolean(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetEnable'",NULL);
#endif
  {
   self->SetEnable(enable);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetEnable'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Enabled of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_Enabled00
static int tolua_ui_ui_phui_View_Enabled00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const phui::View",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const phui::View* self = (const phui::View*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Enabled'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->Enabled();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Enabled'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetMoveOnDrag of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_SetMoveOnDrag00
static int tolua_ui_ui_phui_View_SetMoveOnDrag00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isboolean(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
  bool b = ((bool)  tolua_toboolean(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetMoveOnDrag'",NULL);
#endif
  {
   self->SetMoveOnDrag(b);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetMoveOnDrag'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Close of class  phui::View */
#ifndef TOLUA_DISABLE_tolua_ui_ui_phui_View_Close00
static int tolua_ui_ui_phui_View_Close00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"phui::View",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  phui::View* self = (phui::View*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Close'",NULL);
#endif
  {
   self->Close();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Close'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_ui_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_module(tolua_S,"ui",0);
  tolua_beginmodule(tolua_S,"ui");
   tolua_function(tolua_S,"uiSetDespawnInfo",tolua_ui_ui_uiSetDespawnInfo00);
   tolua_function(tolua_S,"uiInGame",tolua_ui_ui_uiInGame00);
   tolua_function(tolua_S,"uiRunScript",tolua_ui_ui_uiRunScript00);
   tolua_function(tolua_S,"uiRunScript",tolua_ui_ui_uiRunScript01);
   tolua_function(tolua_S,"uiSetCursor",tolua_ui_ui_uiSetCursor00);
   tolua_function(tolua_S,"uiLocString",tolua_ui_ui_uiLocString00);
   tolua_function(tolua_S,"uiLocString2",tolua_ui_ui_uiLocString200);
   tolua_function(tolua_S,"uiLocString2",tolua_ui_ui_uiLocString201);
   tolua_function(tolua_S,"uiHasLocString",tolua_ui_ui_uiHasLocString00);
   tolua_function(tolua_S,"uiGameEvent",tolua_ui_ui_uiGameEvent00);
   tolua_function(tolua_S,"uiPlaySound",tolua_ui_ui_uiPlaySound00);
   tolua_function(tolua_S,"uiChatPrintLine",tolua_ui_ui_uiChatPrintLine00);
   tolua_function(tolua_S,"uiConsoleprintf",tolua_ui_ui_uiConsoleprintf00);
   tolua_function(tolua_S,"uiLoadView",tolua_ui_ui_uiLoadView00);
   tolua_function(tolua_S,"uiFindView",tolua_ui_ui_uiFindView00);
   tolua_function(tolua_S,"uiChatSetRadio",tolua_ui_ui_uiChatSetRadio00);
   tolua_function(tolua_S,"uiChatGetRadio",tolua_ui_ui_uiChatGetRadio00);
   tolua_constant(tolua_S,"NO_CHANNEL",NO_CHANNEL);
   tolua_constant(tolua_S,"BRIGADE_CHANNEL",BRIGADE_CHANNEL);
   tolua_constant(tolua_S,"MISSION_CHANNEL",MISSION_CHANNEL);
   tolua_constant(tolua_S,"SQUAD_CHANNEL",SQUAD_CHANNEL);
   tolua_constant(tolua_S,"GCI_CHANNEL",GCI_CHANNEL);
   tolua_constant(tolua_S,"CP_CHANNEL",CP_CHANNEL);
   tolua_constant(tolua_S,"NUMBERED_CHANNEL",NUMBERED_CHANNEL);
   tolua_constant(tolua_S,"PRIVATE_CHANNEL",PRIVATE_CHANNEL);
   tolua_constant(tolua_S,"ADMIN_CHANNEL",ADMIN_CHANNEL);
   tolua_constant(tolua_S,"ORIGIN_CP_CHANNEL",ORIGIN_CP_CHANNEL);
   tolua_constant(tolua_S,"OBJECTIVE_CP_CHANNEL",OBJECTIVE_CP_CHANNEL);
   tolua_constant(tolua_S,"SIDE_GENERAL_CHANNEL",SIDE_GENERAL_CHANNEL);
   tolua_constant(tolua_S,"SIDE_HELP_CHANNEL",SIDE_HELP_CHANNEL);
   tolua_constant(tolua_S,"SIDE_OPS_CHANNEL",SIDE_OPS_CHANNEL);
   tolua_constant(tolua_S,"MAX_CHANNEL_TYPES",MAX_CHANNEL_TYPES);
   tolua_module(tolua_S,"phui",0);
   tolua_beginmodule(tolua_S,"phui");
    tolua_cclass(tolua_S,"View","phui::View","",NULL);
    tolua_beginmodule(tolua_S,"View");
     tolua_function(tolua_S,"Parent",tolua_ui_ui_phui_View_Parent00);
     tolua_function(tolua_S,"ChildAt",tolua_ui_ui_phui_View_ChildAt00);
     tolua_function(tolua_S,"FindView",tolua_ui_ui_phui_View_FindView00);
     tolua_function(tolua_S,"FindSibling",tolua_ui_ui_phui_View_FindSibling00);
     tolua_function(tolua_S,"Width",tolua_ui_ui_phui_View_Width00);
     tolua_function(tolua_S,"Height",tolua_ui_ui_phui_View_Height00);
     tolua_function(tolua_S,"X",tolua_ui_ui_phui_View_X00);
     tolua_function(tolua_S,"Y",tolua_ui_ui_phui_View_Y00);
     tolua_function(tolua_S,"Left",tolua_ui_ui_phui_View_Left00);
     tolua_function(tolua_S,"Right",tolua_ui_ui_phui_View_Right00);
     tolua_function(tolua_S,"Bottom",tolua_ui_ui_phui_View_Bottom00);
     tolua_function(tolua_S,"Top",tolua_ui_ui_phui_View_Top00);
     tolua_function(tolua_S,"SetLeft",tolua_ui_ui_phui_View_SetLeft00);
     tolua_function(tolua_S,"SetRight",tolua_ui_ui_phui_View_SetRight00);
     tolua_function(tolua_S,"SetBottom",tolua_ui_ui_phui_View_SetBottom00);
     tolua_function(tolua_S,"SetTop",tolua_ui_ui_phui_View_SetTop00);
     tolua_function(tolua_S,"ResizeToAspect",tolua_ui_ui_phui_View_ResizeToAspect00);
     tolua_function(tolua_S,"ResizeTo",tolua_ui_ui_phui_View_ResizeTo00);
     tolua_function(tolua_S,"ResizeBy",tolua_ui_ui_phui_View_ResizeBy00);
     tolua_function(tolua_S,"MoveTo",tolua_ui_ui_phui_View_MoveTo00);
     tolua_function(tolua_S,"MoveBy",tolua_ui_ui_phui_View_MoveBy00);
     tolua_function(tolua_S,"ScrollTo",tolua_ui_ui_phui_View_ScrollTo00);
     tolua_function(tolua_S,"ScrollBy",tolua_ui_ui_phui_View_ScrollBy00);
     tolua_function(tolua_S,"SetBackgroundColor",tolua_ui_ui_phui_View_SetBackgroundColor00);
     tolua_function(tolua_S,"SetBorderColor",tolua_ui_ui_phui_View_SetBorderColor00);
     tolua_function(tolua_S,"SetTextColor",tolua_ui_ui_phui_View_SetTextColor00);
     tolua_function(tolua_S,"Show",tolua_ui_ui_phui_View_Show00);
     tolua_function(tolua_S,"Hide",tolua_ui_ui_phui_View_Hide00);
     tolua_function(tolua_S,"IsVisible",tolua_ui_ui_phui_View_IsVisible00);
     tolua_function(tolua_S,"SetEnable",tolua_ui_ui_phui_View_SetEnable00);
     tolua_function(tolua_S,"Enabled",tolua_ui_ui_phui_View_Enabled00);
     tolua_function(tolua_S,"SetMoveOnDrag",tolua_ui_ui_phui_View_SetMoveOnDrag00);
     tolua_function(tolua_S,"Close",tolua_ui_ui_phui_View_Close00);
    tolua_endmodule(tolua_S);
   tolua_endmodule(tolua_S);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_ui (lua_State* tolua_S) {
 return tolua_ui_open(tolua_S);
};
#endif


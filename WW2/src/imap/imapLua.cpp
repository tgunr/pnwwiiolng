#include "syscfg.h" // Path is project specific.
/*
** Lua binding: imap
** Generated automatically by tolua++-1.0.92 on 01/04/14 22:00:08.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_imap_open (lua_State* tolua_S);

#include "imap/iac.h"
#include "imap/ww2func.h"
#include "imap/ww2imap.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
}

/* function: ww2GetFuncName */
#ifndef TOLUA_DISABLE_tolua_imap_input_ww2GetFuncName00
static int tolua_imap_input_ww2GetFuncName00(lua_State* tolua_S)
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
  unsigned int functionCode = ((unsigned int)  tolua_tonumber(tolua_S,1,0));
  {
   const char* tolua_ret = (const char*)  ww2GetFuncName(functionCode);
   tolua_pushstring(tolua_S,(const char*)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ww2GetFuncName'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: ww2funcHeader */
#ifndef TOLUA_DISABLE_tolua_imap_input_ww2funcHeader00
static int tolua_imap_input_ww2funcHeader00(lua_State* tolua_S)
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
  unsigned int functionCode = ((unsigned int)  tolua_tonumber(tolua_S,1,0));
  {
   const char* tolua_ret = (const char*)  ww2funcHeader(functionCode);
   tolua_pushstring(tolua_S,(const char*)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ww2funcHeader'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: ww2FunctionControlName */
#ifndef TOLUA_DISABLE_tolua_imap_input_ww2FunctionControlName00
static int tolua_imap_input_ww2FunctionControlName00(lua_State* tolua_S)
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
  unsigned int functionCode = ((unsigned int)  tolua_tonumber(tolua_S,1,0));
  {
   const char* tolua_ret = (const char*)  ww2FunctionControlName(functionCode);
   tolua_pushstring(tolua_S,(const char*)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ww2FunctionControlName'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: ww2KeymapInit */
#ifndef TOLUA_DISABLE_tolua_imap_input_ww2KeymapInit00
static int tolua_imap_input_ww2KeymapInit00(lua_State* tolua_S)
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
  int defaultcfml = ((int)  tolua_tonumber(tolua_S,1,0));
  {
   int tolua_ret = (int)  ww2KeymapInit(defaultcfml);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ww2KeymapInit'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: ww2KeymapKill */
#ifndef TOLUA_DISABLE_tolua_imap_input_ww2KeymapKill00
static int tolua_imap_input_ww2KeymapKill00(lua_State* tolua_S)
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
   ww2KeymapKill();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ww2KeymapKill'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: ww2imapGetFuncKeyname */
#ifndef TOLUA_DISABLE_tolua_imap_input_ww2imapGetFuncKeyname00
static int tolua_imap_input_ww2imapGetFuncKeyname00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
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
  int cat = ((int)  tolua_tonumber(tolua_S,1,0));
  int veh = ((int)  tolua_tonumber(tolua_S,2,0));
  unsigned int func = ((unsigned int)  tolua_tonumber(tolua_S,3,0));
  int key = ((int)  tolua_tonumber(tolua_S,4,0));
  int type = ((int)  tolua_tonumber(tolua_S,5,-1));
  {
   const char* tolua_ret = (const char*)  ww2imapGetFuncKeyname(cat,veh,func,key,type);
   tolua_pushstring(tolua_S,(const char*)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ww2imapGetFuncKeyname'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_imap_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_module(tolua_S,"input",0);
  tolua_beginmodule(tolua_S,"input");
   tolua_constant(tolua_S,"IAC_US_KEYBOARD_BEGIN",IAC_US_KEYBOARD_BEGIN);
   tolua_constant(tolua_S,"IAC_ESCAPE",IAC_ESCAPE);
   tolua_constant(tolua_S,"IAC_F1",IAC_F1);
   tolua_constant(tolua_S,"IAC_F2",IAC_F2);
   tolua_constant(tolua_S,"IAC_F3",IAC_F3);
   tolua_constant(tolua_S,"IAC_F4",IAC_F4);
   tolua_constant(tolua_S,"IAC_F5",IAC_F5);
   tolua_constant(tolua_S,"IAC_F6",IAC_F6);
   tolua_constant(tolua_S,"IAC_F7",IAC_F7);
   tolua_constant(tolua_S,"IAC_F8",IAC_F8);
   tolua_constant(tolua_S,"IAC_F9",IAC_F9);
   tolua_constant(tolua_S,"IAC_F10",IAC_F10);
   tolua_constant(tolua_S,"IAC_F11",IAC_F11);
   tolua_constant(tolua_S,"IAC_F12",IAC_F12);
   tolua_constant(tolua_S,"IAC_SYSRQ",IAC_SYSRQ);
   tolua_constant(tolua_S,"IAC_SCROLL",IAC_SCROLL);
   tolua_constant(tolua_S,"IAC_PAUSE",IAC_PAUSE);
   tolua_constant(tolua_S,"IAC_GRAVE",IAC_GRAVE);
   tolua_constant(tolua_S,"IAC_1",IAC_1);
   tolua_constant(tolua_S,"IAC_2",IAC_2);
   tolua_constant(tolua_S,"IAC_3",IAC_3);
   tolua_constant(tolua_S,"IAC_4",IAC_4);
   tolua_constant(tolua_S,"IAC_5",IAC_5);
   tolua_constant(tolua_S,"IAC_6",IAC_6);
   tolua_constant(tolua_S,"IAC_7",IAC_7);
   tolua_constant(tolua_S,"IAC_8",IAC_8);
   tolua_constant(tolua_S,"IAC_9",IAC_9);
   tolua_constant(tolua_S,"IAC_0",IAC_0);
   tolua_constant(tolua_S,"IAC_MINUS",IAC_MINUS);
   tolua_constant(tolua_S,"IAC_EQUALS",IAC_EQUALS);
   tolua_constant(tolua_S,"IAC_BACKSPACE",IAC_BACKSPACE);
   tolua_constant(tolua_S,"IAC_INSERT",IAC_INSERT);
   tolua_constant(tolua_S,"IAC_HOME",IAC_HOME);
   tolua_constant(tolua_S,"IAC_PAGEUP",IAC_PAGEUP);
   tolua_constant(tolua_S,"IAC_NUMLOCK",IAC_NUMLOCK);
   tolua_constant(tolua_S,"IAC_NUMPADDIVIDE",IAC_NUMPADDIVIDE);
   tolua_constant(tolua_S,"IAC_NUMPADMULTIPLY",IAC_NUMPADMULTIPLY);
   tolua_constant(tolua_S,"IAC_NUMPADSUBTRACT",IAC_NUMPADSUBTRACT);
   tolua_constant(tolua_S,"IAC_TAB",IAC_TAB);
   tolua_constant(tolua_S,"IAC_Q",IAC_Q);
   tolua_constant(tolua_S,"IAC_W",IAC_W);
   tolua_constant(tolua_S,"IAC_E",IAC_E);
   tolua_constant(tolua_S,"IAC_R",IAC_R);
   tolua_constant(tolua_S,"IAC_T",IAC_T);
   tolua_constant(tolua_S,"IAC_Y",IAC_Y);
   tolua_constant(tolua_S,"IAC_U",IAC_U);
   tolua_constant(tolua_S,"IAC_I",IAC_I);
   tolua_constant(tolua_S,"IAC_O",IAC_O);
   tolua_constant(tolua_S,"IAC_P",IAC_P);
   tolua_constant(tolua_S,"IAC_LEFTBRACKET",IAC_LEFTBRACKET);
   tolua_constant(tolua_S,"IAC_RIGHTBRACKET",IAC_RIGHTBRACKET);
   tolua_constant(tolua_S,"IAC_BACKSLASH",IAC_BACKSLASH);
   tolua_constant(tolua_S,"IAC_DELETE",IAC_DELETE);
   tolua_constant(tolua_S,"IAC_END",IAC_END);
   tolua_constant(tolua_S,"IAC_PAGEDOWN",IAC_PAGEDOWN);
   tolua_constant(tolua_S,"IAC_NUMPAD7",IAC_NUMPAD7);
   tolua_constant(tolua_S,"IAC_NUMPAD8",IAC_NUMPAD8);
   tolua_constant(tolua_S,"IAC_NUMPAD9",IAC_NUMPAD9);
   tolua_constant(tolua_S,"IAC_NUMPADADD",IAC_NUMPADADD);
   tolua_constant(tolua_S,"IAC_CAPSLOCK",IAC_CAPSLOCK);
   tolua_constant(tolua_S,"IAC_A",IAC_A);
   tolua_constant(tolua_S,"IAC_S",IAC_S);
   tolua_constant(tolua_S,"IAC_D",IAC_D);
   tolua_constant(tolua_S,"IAC_F",IAC_F);
   tolua_constant(tolua_S,"IAC_G",IAC_G);
   tolua_constant(tolua_S,"IAC_H",IAC_H);
   tolua_constant(tolua_S,"IAC_J",IAC_J);
   tolua_constant(tolua_S,"IAC_K",IAC_K);
   tolua_constant(tolua_S,"IAC_L",IAC_L);
   tolua_constant(tolua_S,"IAC_SEMICOLON",IAC_SEMICOLON);
   tolua_constant(tolua_S,"IAC_APOSTROPHE",IAC_APOSTROPHE);
   tolua_constant(tolua_S,"IAC_ENTER",IAC_ENTER);
   tolua_constant(tolua_S,"IAC_NUMPAD4",IAC_NUMPAD4);
   tolua_constant(tolua_S,"IAC_NUMPAD5",IAC_NUMPAD5);
   tolua_constant(tolua_S,"IAC_NUMPAD6",IAC_NUMPAD6);
   tolua_constant(tolua_S,"IAC_LEFTSHIFT",IAC_LEFTSHIFT);
   tolua_constant(tolua_S,"IAC_Z",IAC_Z);
   tolua_constant(tolua_S,"IAC_X",IAC_X);
   tolua_constant(tolua_S,"IAC_C",IAC_C);
   tolua_constant(tolua_S,"IAC_V",IAC_V);
   tolua_constant(tolua_S,"IAC_B",IAC_B);
   tolua_constant(tolua_S,"IAC_N",IAC_N);
   tolua_constant(tolua_S,"IAC_M",IAC_M);
   tolua_constant(tolua_S,"IAC_COMMA",IAC_COMMA);
   tolua_constant(tolua_S,"IAC_PERIOD",IAC_PERIOD);
   tolua_constant(tolua_S,"IAC_SLASH",IAC_SLASH);
   tolua_constant(tolua_S,"IAC_RIGHTSHIFT",IAC_RIGHTSHIFT);
   tolua_constant(tolua_S,"IAC_UP",IAC_UP);
   tolua_constant(tolua_S,"IAC_NUMPAD1",IAC_NUMPAD1);
   tolua_constant(tolua_S,"IAC_NUMPAD2",IAC_NUMPAD2);
   tolua_constant(tolua_S,"IAC_NUMPAD3",IAC_NUMPAD3);
   tolua_constant(tolua_S,"IAC_NUMPADENTER",IAC_NUMPADENTER);
   tolua_constant(tolua_S,"IAC_LEFTCONTROL",IAC_LEFTCONTROL);
   tolua_constant(tolua_S,"IAC_LEFTALT",IAC_LEFTALT);
   tolua_constant(tolua_S,"IAC_SPACE",IAC_SPACE);
   tolua_constant(tolua_S,"IAC_RIGHTALT",IAC_RIGHTALT);
   tolua_constant(tolua_S,"IAC_RIGHTCONTROL",IAC_RIGHTCONTROL);
   tolua_constant(tolua_S,"IAC_LEFT",IAC_LEFT);
   tolua_constant(tolua_S,"IAC_DOWN",IAC_DOWN);
   tolua_constant(tolua_S,"IAC_RIGHT",IAC_RIGHT);
   tolua_constant(tolua_S,"IAC_NUMPAD0",IAC_NUMPAD0);
   tolua_constant(tolua_S,"IAC_NUMPADDECIMAL",IAC_NUMPADDECIMAL);
   tolua_constant(tolua_S,"IAC_NUMPADEQUALS",IAC_NUMPADEQUALS);
   tolua_constant(tolua_S,"IAC_LEFTMODKEY",IAC_LEFTMODKEY);
   tolua_constant(tolua_S,"IAC_RIGHTMODKEY",IAC_RIGHTMODKEY);
   tolua_constant(tolua_S,"IAC_OEM_8",IAC_OEM_8);
   tolua_constant(tolua_S,"IAC_OEM_102",IAC_OEM_102);
   tolua_constant(tolua_S,"IAC_US_KEYBOARD_END",IAC_US_KEYBOARD_END);
   tolua_constant(tolua_S,"IAC_US_MAX_KEYS",IAC_US_MAX_KEYS);
   tolua_constant(tolua_S,"IAC_MOUSE_AXIS_BEGIN",IAC_MOUSE_AXIS_BEGIN);
   tolua_constant(tolua_S,"IAC_MOUSEX",IAC_MOUSEX);
   tolua_constant(tolua_S,"IAC_MOUSEY",IAC_MOUSEY);
   tolua_constant(tolua_S,"IAC_MOUSEZ",IAC_MOUSEZ);
   tolua_constant(tolua_S,"IAC_MOUSE_AXIS_END",IAC_MOUSE_AXIS_END);
   tolua_constant(tolua_S,"IAC_MOUSE_MAX_AXES",IAC_MOUSE_MAX_AXES);
   tolua_constant(tolua_S,"IAC_MOUSE_BUTTON_BEGIN",IAC_MOUSE_BUTTON_BEGIN);
   tolua_constant(tolua_S,"IAC_MOUSEBUTTON1",IAC_MOUSEBUTTON1);
   tolua_constant(tolua_S,"IAC_MOUSEBUTTON2",IAC_MOUSEBUTTON2);
   tolua_constant(tolua_S,"IAC_MOUSEBUTTON3",IAC_MOUSEBUTTON3);
   tolua_constant(tolua_S,"IAC_MOUSEBUTTON4",IAC_MOUSEBUTTON4);
   tolua_constant(tolua_S,"IAC_MOUSEBUTTON5",IAC_MOUSEBUTTON5);
   tolua_constant(tolua_S,"IAC_MOUSE_BUTTON_END",IAC_MOUSE_BUTTON_END);
   tolua_constant(tolua_S,"IAC_MOUSE_MAX_BUTTONS",IAC_MOUSE_MAX_BUTTONS);
   tolua_constant(tolua_S,"IAC_MOUSE_WHEEL_BEGIN",IAC_MOUSE_WHEEL_BEGIN);
   tolua_constant(tolua_S,"IAC_MOUSE_WHEEL_UP",IAC_MOUSE_WHEEL_UP);
   tolua_constant(tolua_S,"IAC_MOUSE_WHEEL_DOWN",IAC_MOUSE_WHEEL_DOWN);
   tolua_constant(tolua_S,"IAC_MOUSE_WHEEL_END",IAC_MOUSE_WHEEL_END);
   tolua_constant(tolua_S,"IAC_MAX_JOYSTICKS",IAC_MAX_JOYSTICKS);
   tolua_constant(tolua_S,"IAC_JOYSTICK_AXIS_BEGIN",IAC_JOYSTICK_AXIS_BEGIN);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_AXIS_BEGIN",IAC_JOYSTICK1_AXIS_BEGIN);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POSX",IAC_JOYSTICK1_POSX);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POSY",IAC_JOYSTICK1_POSY);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POSZ",IAC_JOYSTICK1_POSZ);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_ROTX",IAC_JOYSTICK1_ROTX);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_ROTY",IAC_JOYSTICK1_ROTY);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_ROTZ",IAC_JOYSTICK1_ROTZ);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_SLIDER1",IAC_JOYSTICK1_SLIDER1);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_SLIDER2",IAC_JOYSTICK1_SLIDER2);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_AXIS_END",IAC_JOYSTICK1_AXIS_END);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_AXIS_BEGIN",IAC_JOYSTICK2_AXIS_BEGIN);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POSX",IAC_JOYSTICK2_POSX);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POSY",IAC_JOYSTICK2_POSY);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POSZ",IAC_JOYSTICK2_POSZ);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_ROTX",IAC_JOYSTICK2_ROTX);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_ROTY",IAC_JOYSTICK2_ROTY);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_ROTZ",IAC_JOYSTICK2_ROTZ);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_SLIDER1",IAC_JOYSTICK2_SLIDER1);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_SLIDER2",IAC_JOYSTICK2_SLIDER2);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_AXIS_END",IAC_JOYSTICK2_AXIS_END);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_AXIS_BEGIN",IAC_JOYSTICK3_AXIS_BEGIN);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POSX",IAC_JOYSTICK3_POSX);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POSY",IAC_JOYSTICK3_POSY);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POSZ",IAC_JOYSTICK3_POSZ);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_ROTX",IAC_JOYSTICK3_ROTX);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_ROTY",IAC_JOYSTICK3_ROTY);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_ROTZ",IAC_JOYSTICK3_ROTZ);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_SLIDER1",IAC_JOYSTICK3_SLIDER1);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_SLIDER2",IAC_JOYSTICK3_SLIDER2);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_AXIS_END",IAC_JOYSTICK3_AXIS_END);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_AXIS_BEGIN",IAC_JOYSTICK4_AXIS_BEGIN);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POSX",IAC_JOYSTICK4_POSX);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POSY",IAC_JOYSTICK4_POSY);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POSZ",IAC_JOYSTICK4_POSZ);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_ROTX",IAC_JOYSTICK4_ROTX);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_ROTY",IAC_JOYSTICK4_ROTY);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_ROTZ",IAC_JOYSTICK4_ROTZ);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_SLIDER1",IAC_JOYSTICK4_SLIDER1);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_SLIDER2",IAC_JOYSTICK4_SLIDER2);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_AXIS_END",IAC_JOYSTICK4_AXIS_END);
   tolua_constant(tolua_S,"IAC_JOYSTICK_TRACK_IR_BEGIN",IAC_JOYSTICK_TRACK_IR_BEGIN);
   tolua_constant(tolua_S,"IAC_JOYSTICK_TRACK_IR_X",IAC_JOYSTICK_TRACK_IR_X);
   tolua_constant(tolua_S,"IAC_JOYSTICK_TRACK_IR_Y",IAC_JOYSTICK_TRACK_IR_Y);
   tolua_constant(tolua_S,"IAC_JOYSTICK_TRACK_IR_Z",IAC_JOYSTICK_TRACK_IR_Z);
   tolua_constant(tolua_S,"IAC_JOYSTICK_TRACK_IR_TX",IAC_JOYSTICK_TRACK_IR_TX);
   tolua_constant(tolua_S,"IAC_JOYSTICK_TRACK_IR_TY",IAC_JOYSTICK_TRACK_IR_TY);
   tolua_constant(tolua_S,"IAC_JOYSTICK_TRACK_IR_TZ",IAC_JOYSTICK_TRACK_IR_TZ);
   tolua_constant(tolua_S,"IAC_JOYSTICK_TRACK_IR_END",IAC_JOYSTICK_TRACK_IR_END);
   tolua_constant(tolua_S,"IAC_JOYSTICK_AXIS_END",IAC_JOYSTICK_AXIS_END);
   tolua_constant(tolua_S,"IAC_JOYSTICK_MAX_AXES",IAC_JOYSTICK_MAX_AXES);
   tolua_constant(tolua_S,"IAC_JOYSTICK_BUTTON_BEGIN",IAC_JOYSTICK_BUTTON_BEGIN);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON_BEGIN",IAC_JOYSTICK1_BUTTON_BEGIN);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON1",IAC_JOYSTICK1_BUTTON1);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON2",IAC_JOYSTICK1_BUTTON2);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON3",IAC_JOYSTICK1_BUTTON3);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON4",IAC_JOYSTICK1_BUTTON4);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON5",IAC_JOYSTICK1_BUTTON5);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON6",IAC_JOYSTICK1_BUTTON6);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON7",IAC_JOYSTICK1_BUTTON7);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON8",IAC_JOYSTICK1_BUTTON8);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON9",IAC_JOYSTICK1_BUTTON9);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON10",IAC_JOYSTICK1_BUTTON10);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON11",IAC_JOYSTICK1_BUTTON11);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON12",IAC_JOYSTICK1_BUTTON12);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON13",IAC_JOYSTICK1_BUTTON13);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON14",IAC_JOYSTICK1_BUTTON14);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON15",IAC_JOYSTICK1_BUTTON15);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON16",IAC_JOYSTICK1_BUTTON16);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON17",IAC_JOYSTICK1_BUTTON17);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON18",IAC_JOYSTICK1_BUTTON18);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON19",IAC_JOYSTICK1_BUTTON19);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON20",IAC_JOYSTICK1_BUTTON20);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON21",IAC_JOYSTICK1_BUTTON21);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON22",IAC_JOYSTICK1_BUTTON22);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON23",IAC_JOYSTICK1_BUTTON23);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON24",IAC_JOYSTICK1_BUTTON24);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON25",IAC_JOYSTICK1_BUTTON25);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON26",IAC_JOYSTICK1_BUTTON26);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON27",IAC_JOYSTICK1_BUTTON27);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON28",IAC_JOYSTICK1_BUTTON28);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON29",IAC_JOYSTICK1_BUTTON29);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON30",IAC_JOYSTICK1_BUTTON30);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON31",IAC_JOYSTICK1_BUTTON31);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON32",IAC_JOYSTICK1_BUTTON32);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_BUTTON_END",IAC_JOYSTICK1_BUTTON_END);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON_BEGIN",IAC_JOYSTICK2_BUTTON_BEGIN);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON1",IAC_JOYSTICK2_BUTTON1);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON2",IAC_JOYSTICK2_BUTTON2);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON3",IAC_JOYSTICK2_BUTTON3);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON4",IAC_JOYSTICK2_BUTTON4);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON5",IAC_JOYSTICK2_BUTTON5);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON6",IAC_JOYSTICK2_BUTTON6);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON7",IAC_JOYSTICK2_BUTTON7);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON8",IAC_JOYSTICK2_BUTTON8);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON9",IAC_JOYSTICK2_BUTTON9);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON10",IAC_JOYSTICK2_BUTTON10);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON11",IAC_JOYSTICK2_BUTTON11);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON12",IAC_JOYSTICK2_BUTTON12);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON13",IAC_JOYSTICK2_BUTTON13);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON14",IAC_JOYSTICK2_BUTTON14);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON15",IAC_JOYSTICK2_BUTTON15);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON16",IAC_JOYSTICK2_BUTTON16);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON17",IAC_JOYSTICK2_BUTTON17);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON18",IAC_JOYSTICK2_BUTTON18);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON19",IAC_JOYSTICK2_BUTTON19);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON20",IAC_JOYSTICK2_BUTTON20);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON21",IAC_JOYSTICK2_BUTTON21);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON22",IAC_JOYSTICK2_BUTTON22);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON23",IAC_JOYSTICK2_BUTTON23);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON24",IAC_JOYSTICK2_BUTTON24);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON25",IAC_JOYSTICK2_BUTTON25);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON26",IAC_JOYSTICK2_BUTTON26);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON27",IAC_JOYSTICK2_BUTTON27);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON28",IAC_JOYSTICK2_BUTTON28);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON29",IAC_JOYSTICK2_BUTTON29);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON30",IAC_JOYSTICK2_BUTTON30);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON31",IAC_JOYSTICK2_BUTTON31);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON32",IAC_JOYSTICK2_BUTTON32);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_BUTTON_END",IAC_JOYSTICK2_BUTTON_END);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON_BEGIN",IAC_JOYSTICK3_BUTTON_BEGIN);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON1",IAC_JOYSTICK3_BUTTON1);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON2",IAC_JOYSTICK3_BUTTON2);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON3",IAC_JOYSTICK3_BUTTON3);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON4",IAC_JOYSTICK3_BUTTON4);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON5",IAC_JOYSTICK3_BUTTON5);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON6",IAC_JOYSTICK3_BUTTON6);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON7",IAC_JOYSTICK3_BUTTON7);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON8",IAC_JOYSTICK3_BUTTON8);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON9",IAC_JOYSTICK3_BUTTON9);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON10",IAC_JOYSTICK3_BUTTON10);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON11",IAC_JOYSTICK3_BUTTON11);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON12",IAC_JOYSTICK3_BUTTON12);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON13",IAC_JOYSTICK3_BUTTON13);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON14",IAC_JOYSTICK3_BUTTON14);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON15",IAC_JOYSTICK3_BUTTON15);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON16",IAC_JOYSTICK3_BUTTON16);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON17",IAC_JOYSTICK3_BUTTON17);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON18",IAC_JOYSTICK3_BUTTON18);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON19",IAC_JOYSTICK3_BUTTON19);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON20",IAC_JOYSTICK3_BUTTON20);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON21",IAC_JOYSTICK3_BUTTON21);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON22",IAC_JOYSTICK3_BUTTON22);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON23",IAC_JOYSTICK3_BUTTON23);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON24",IAC_JOYSTICK3_BUTTON24);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON25",IAC_JOYSTICK3_BUTTON25);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON26",IAC_JOYSTICK3_BUTTON26);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON27",IAC_JOYSTICK3_BUTTON27);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON28",IAC_JOYSTICK3_BUTTON28);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON29",IAC_JOYSTICK3_BUTTON29);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON30",IAC_JOYSTICK3_BUTTON30);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON31",IAC_JOYSTICK3_BUTTON31);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON32",IAC_JOYSTICK3_BUTTON32);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_BUTTON_END",IAC_JOYSTICK3_BUTTON_END);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON_BEGIN",IAC_JOYSTICK4_BUTTON_BEGIN);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON1",IAC_JOYSTICK4_BUTTON1);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON2",IAC_JOYSTICK4_BUTTON2);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON3",IAC_JOYSTICK4_BUTTON3);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON4",IAC_JOYSTICK4_BUTTON4);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON5",IAC_JOYSTICK4_BUTTON5);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON6",IAC_JOYSTICK4_BUTTON6);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON7",IAC_JOYSTICK4_BUTTON7);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON8",IAC_JOYSTICK4_BUTTON8);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON9",IAC_JOYSTICK4_BUTTON9);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON10",IAC_JOYSTICK4_BUTTON10);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON11",IAC_JOYSTICK4_BUTTON11);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON12",IAC_JOYSTICK4_BUTTON12);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON13",IAC_JOYSTICK4_BUTTON13);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON14",IAC_JOYSTICK4_BUTTON14);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON15",IAC_JOYSTICK4_BUTTON15);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON16",IAC_JOYSTICK4_BUTTON16);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON17",IAC_JOYSTICK4_BUTTON17);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON18",IAC_JOYSTICK4_BUTTON18);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON19",IAC_JOYSTICK4_BUTTON19);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON20",IAC_JOYSTICK4_BUTTON20);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON21",IAC_JOYSTICK4_BUTTON21);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON22",IAC_JOYSTICK4_BUTTON22);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON23",IAC_JOYSTICK4_BUTTON23);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON24",IAC_JOYSTICK4_BUTTON24);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON25",IAC_JOYSTICK4_BUTTON25);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON26",IAC_JOYSTICK4_BUTTON26);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON27",IAC_JOYSTICK4_BUTTON27);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON28",IAC_JOYSTICK4_BUTTON28);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON29",IAC_JOYSTICK4_BUTTON29);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON30",IAC_JOYSTICK4_BUTTON30);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON31",IAC_JOYSTICK4_BUTTON31);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON32",IAC_JOYSTICK4_BUTTON32);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_BUTTON_END",IAC_JOYSTICK4_BUTTON_END);
   tolua_constant(tolua_S,"IAC_JOYSTICK_BUTTON_END",IAC_JOYSTICK_BUTTON_END);
   tolua_constant(tolua_S,"IAC_JOYSTICK_MAX_BUTTONS",IAC_JOYSTICK_MAX_BUTTONS);
   tolua_constant(tolua_S,"IAC_JOYSTICK_POV_MAX",IAC_JOYSTICK_POV_MAX);
   tolua_constant(tolua_S,"IAC_JOYSTICK_POV_POS",IAC_JOYSTICK_POV_POS);
   tolua_constant(tolua_S,"IAC_JOYSTICK_POV_BEGIN",IAC_JOYSTICK_POV_BEGIN);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV_BEGIN",IAC_JOYSTICK1_POV_BEGIN);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV1_N",IAC_JOYSTICK1_POV1_N);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV1_NE",IAC_JOYSTICK1_POV1_NE);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV1_E",IAC_JOYSTICK1_POV1_E);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV1_SE",IAC_JOYSTICK1_POV1_SE);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV1_S",IAC_JOYSTICK1_POV1_S);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV1_SW",IAC_JOYSTICK1_POV1_SW);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV1_W",IAC_JOYSTICK1_POV1_W);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV1_NW",IAC_JOYSTICK1_POV1_NW);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV2_N",IAC_JOYSTICK1_POV2_N);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV2_NE",IAC_JOYSTICK1_POV2_NE);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV2_E",IAC_JOYSTICK1_POV2_E);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV2_SE",IAC_JOYSTICK1_POV2_SE);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV2_S",IAC_JOYSTICK1_POV2_S);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV2_SW",IAC_JOYSTICK1_POV2_SW);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV2_W",IAC_JOYSTICK1_POV2_W);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV2_NW",IAC_JOYSTICK1_POV2_NW);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV3_N",IAC_JOYSTICK1_POV3_N);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV3_NE",IAC_JOYSTICK1_POV3_NE);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV3_E",IAC_JOYSTICK1_POV3_E);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV3_SE",IAC_JOYSTICK1_POV3_SE);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV3_S",IAC_JOYSTICK1_POV3_S);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV3_SW",IAC_JOYSTICK1_POV3_SW);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV3_W",IAC_JOYSTICK1_POV3_W);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV3_NW",IAC_JOYSTICK1_POV3_NW);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV4_N",IAC_JOYSTICK1_POV4_N);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV4_NE",IAC_JOYSTICK1_POV4_NE);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV4_E",IAC_JOYSTICK1_POV4_E);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV4_SE",IAC_JOYSTICK1_POV4_SE);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV4_S",IAC_JOYSTICK1_POV4_S);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV4_SW",IAC_JOYSTICK1_POV4_SW);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV4_W",IAC_JOYSTICK1_POV4_W);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV4_NW",IAC_JOYSTICK1_POV4_NW);
   tolua_constant(tolua_S,"IAC_JOYSTICK1_POV_END",IAC_JOYSTICK1_POV_END);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV_BEGIN",IAC_JOYSTICK2_POV_BEGIN);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV1_N",IAC_JOYSTICK2_POV1_N);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV1_NE",IAC_JOYSTICK2_POV1_NE);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV1_E",IAC_JOYSTICK2_POV1_E);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV1_SE",IAC_JOYSTICK2_POV1_SE);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV1_S",IAC_JOYSTICK2_POV1_S);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV1_SW",IAC_JOYSTICK2_POV1_SW);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV1_W",IAC_JOYSTICK2_POV1_W);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV1_NW",IAC_JOYSTICK2_POV1_NW);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV2_N",IAC_JOYSTICK2_POV2_N);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV2_NE",IAC_JOYSTICK2_POV2_NE);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV2_E",IAC_JOYSTICK2_POV2_E);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV2_SE",IAC_JOYSTICK2_POV2_SE);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV2_S",IAC_JOYSTICK2_POV2_S);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV2_SW",IAC_JOYSTICK2_POV2_SW);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV2_W",IAC_JOYSTICK2_POV2_W);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV2_NW",IAC_JOYSTICK2_POV2_NW);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV3_N",IAC_JOYSTICK2_POV3_N);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV3_NE",IAC_JOYSTICK2_POV3_NE);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV3_E",IAC_JOYSTICK2_POV3_E);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV3_SE",IAC_JOYSTICK2_POV3_SE);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV3_S",IAC_JOYSTICK2_POV3_S);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV3_SW",IAC_JOYSTICK2_POV3_SW);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV3_W",IAC_JOYSTICK2_POV3_W);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV3_NW",IAC_JOYSTICK2_POV3_NW);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV4_N",IAC_JOYSTICK2_POV4_N);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV4_NE",IAC_JOYSTICK2_POV4_NE);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV4_E",IAC_JOYSTICK2_POV4_E);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV4_SE",IAC_JOYSTICK2_POV4_SE);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV4_S",IAC_JOYSTICK2_POV4_S);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV4_SW",IAC_JOYSTICK2_POV4_SW);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV4_W",IAC_JOYSTICK2_POV4_W);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV4_NW",IAC_JOYSTICK2_POV4_NW);
   tolua_constant(tolua_S,"IAC_JOYSTICK2_POV_END",IAC_JOYSTICK2_POV_END);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV_BEGIN",IAC_JOYSTICK3_POV_BEGIN);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV1_N",IAC_JOYSTICK3_POV1_N);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV1_NE",IAC_JOYSTICK3_POV1_NE);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV1_E",IAC_JOYSTICK3_POV1_E);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV1_SE",IAC_JOYSTICK3_POV1_SE);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV1_S",IAC_JOYSTICK3_POV1_S);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV1_SW",IAC_JOYSTICK3_POV1_SW);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV1_W",IAC_JOYSTICK3_POV1_W);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV1_NW",IAC_JOYSTICK3_POV1_NW);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV2_N",IAC_JOYSTICK3_POV2_N);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV2_NE",IAC_JOYSTICK3_POV2_NE);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV2_E",IAC_JOYSTICK3_POV2_E);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV2_SE",IAC_JOYSTICK3_POV2_SE);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV2_S",IAC_JOYSTICK3_POV2_S);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV2_SW",IAC_JOYSTICK3_POV2_SW);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV2_W",IAC_JOYSTICK3_POV2_W);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV2_NW",IAC_JOYSTICK3_POV2_NW);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV3_N",IAC_JOYSTICK3_POV3_N);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV3_NE",IAC_JOYSTICK3_POV3_NE);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV3_E",IAC_JOYSTICK3_POV3_E);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV3_SE",IAC_JOYSTICK3_POV3_SE);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV3_S",IAC_JOYSTICK3_POV3_S);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV3_SW",IAC_JOYSTICK3_POV3_SW);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV3_W",IAC_JOYSTICK3_POV3_W);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV3_NW",IAC_JOYSTICK3_POV3_NW);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV4_N",IAC_JOYSTICK3_POV4_N);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV4_NE",IAC_JOYSTICK3_POV4_NE);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV4_E",IAC_JOYSTICK3_POV4_E);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV4_SE",IAC_JOYSTICK3_POV4_SE);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV4_S",IAC_JOYSTICK3_POV4_S);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV4_SW",IAC_JOYSTICK3_POV4_SW);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV4_W",IAC_JOYSTICK3_POV4_W);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV4_NW",IAC_JOYSTICK3_POV4_NW);
   tolua_constant(tolua_S,"IAC_JOYSTICK3_POV_END",IAC_JOYSTICK3_POV_END);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV_BEGIN",IAC_JOYSTICK4_POV_BEGIN);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV1_N",IAC_JOYSTICK4_POV1_N);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV1_NE",IAC_JOYSTICK4_POV1_NE);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV1_E",IAC_JOYSTICK4_POV1_E);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV1_SE",IAC_JOYSTICK4_POV1_SE);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV1_S",IAC_JOYSTICK4_POV1_S);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV1_SW",IAC_JOYSTICK4_POV1_SW);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV1_W",IAC_JOYSTICK4_POV1_W);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV1_NW",IAC_JOYSTICK4_POV1_NW);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV2_N",IAC_JOYSTICK4_POV2_N);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV2_NE",IAC_JOYSTICK4_POV2_NE);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV2_E",IAC_JOYSTICK4_POV2_E);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV2_SE",IAC_JOYSTICK4_POV2_SE);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV2_S",IAC_JOYSTICK4_POV2_S);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV2_SW",IAC_JOYSTICK4_POV2_SW);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV2_W",IAC_JOYSTICK4_POV2_W);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV2_NW",IAC_JOYSTICK4_POV2_NW);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV3_N",IAC_JOYSTICK4_POV3_N);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV3_NE",IAC_JOYSTICK4_POV3_NE);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV3_E",IAC_JOYSTICK4_POV3_E);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV3_SE",IAC_JOYSTICK4_POV3_SE);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV3_S",IAC_JOYSTICK4_POV3_S);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV3_SW",IAC_JOYSTICK4_POV3_SW);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV3_W",IAC_JOYSTICK4_POV3_W);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV3_NW",IAC_JOYSTICK4_POV3_NW);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV4_N",IAC_JOYSTICK4_POV4_N);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV4_NE",IAC_JOYSTICK4_POV4_NE);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV4_E",IAC_JOYSTICK4_POV4_E);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV4_SE",IAC_JOYSTICK4_POV4_SE);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV4_S",IAC_JOYSTICK4_POV4_S);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV4_SW",IAC_JOYSTICK4_POV4_SW);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV4_W",IAC_JOYSTICK4_POV4_W);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV4_NW",IAC_JOYSTICK4_POV4_NW);
   tolua_constant(tolua_S,"IAC_JOYSTICK4_POV_END",IAC_JOYSTICK4_POV_END);
   tolua_constant(tolua_S,"IAC_JOYSTICK_POV_END",IAC_JOYSTICK_POV_END);
   tolua_constant(tolua_S,"IMAP_NO_FUNCTION",IMAP_NO_FUNCTION);
   tolua_constant(tolua_S,"IMAP_MOM_GEARHANDLE",IMAP_MOM_GEARHANDLE);
   tolua_constant(tolua_S,"IMAP_MOM_MIXTURE_OFF",IMAP_MOM_MIXTURE_OFF);
   tolua_constant(tolua_S,"IMAP_MOM_MIXTURE_LEAN",IMAP_MOM_MIXTURE_LEAN);
   tolua_constant(tolua_S,"IMAP_MOM_MIXTURE_RICH",IMAP_MOM_MIXTURE_RICH);
   tolua_constant(tolua_S,"IMAP_MOM_PANELLIGHT",IMAP_MOM_PANELLIGHT);
   tolua_constant(tolua_S,"IMAP_MOM_HATCHCONTROL",IMAP_MOM_HATCHCONTROL);
   tolua_constant(tolua_S,"IMAP_MOM_CANOPYCONTROL",IMAP_MOM_CANOPYCONTROL);
   tolua_constant(tolua_S,"IMAP_MOM_DEPLOYDIVEBRAKES",IMAP_MOM_DEPLOYDIVEBRAKES);
   tolua_constant(tolua_S,"IMAP_MOM_STARTER_ON",IMAP_MOM_STARTER_ON);
   tolua_constant(tolua_S,"IMAP_MOM_STARTER_OFF",IMAP_MOM_STARTER_OFF);
   tolua_constant(tolua_S,"IMAP_MOM_TOGGLE_TAILLOCK",IMAP_MOM_TOGGLE_TAILLOCK);
   tolua_constant(tolua_S,"IMAP_MOM_TOGGLE_BOMBBAYDOOR",IMAP_MOM_TOGGLE_BOMBBAYDOOR);
   tolua_constant(tolua_S,"IMAP_MOM_TOGGLE_DEPLOYWEAPON",IMAP_MOM_TOGGLE_DEPLOYWEAPON);
   tolua_constant(tolua_S,"IMAP_MOM_TOGGLE_TOWBAR",IMAP_MOM_TOGGLE_TOWBAR);
   tolua_constant(tolua_S,"IMAP_MOM_SHIFTUP",IMAP_MOM_SHIFTUP);
   tolua_constant(tolua_S,"IMAP_MOM_SHIFTDOWN",IMAP_MOM_SHIFTDOWN);
   tolua_constant(tolua_S,"IMAP_MOM_REVERSE",IMAP_MOM_REVERSE);
   tolua_constant(tolua_S,"IMAP_MOM_TOGGLEWEP",IMAP_MOM_TOGGLEWEP);
   tolua_constant(tolua_S,"IMAP_TORPEDO_DEPTH_INCREASE",IMAP_TORPEDO_DEPTH_INCREASE);
   tolua_constant(tolua_S,"IMAP_TORPEDO_DEPTH_DECREASE",IMAP_TORPEDO_DEPTH_DECREASE);
   tolua_constant(tolua_S,"IMAP_RIDE_TRANSPORT",IMAP_RIDE_TRANSPORT);
   tolua_constant(tolua_S,"IMAP_AUTOPILOT",IMAP_AUTOPILOT);
   tolua_constant(tolua_S,"IMAP_MOBILE_SPAWN",IMAP_MOBILE_SPAWN);
   tolua_constant(tolua_S,"IMAP_MOM_TOGGLE_INTERNAL_EXTERNAL_VIEWS",IMAP_MOM_TOGGLE_INTERNAL_EXTERNAL_VIEWS);
   tolua_constant(tolua_S,"IMAP_MOM_INTERNAL_VIEW",IMAP_MOM_INTERNAL_VIEW);
   tolua_constant(tolua_S,"IMAP_MOM_EXTERNAL_VIEW",IMAP_MOM_EXTERNAL_VIEW);
   tolua_constant(tolua_S,"IMAP_MOM_EXTERNAL_LOCKED",IMAP_MOM_EXTERNAL_LOCKED);
   tolua_constant(tolua_S,"IMAP_MOM_EXTERNAL_RUBBERBAND",IMAP_MOM_EXTERNAL_RUBBERBAND);
   tolua_constant(tolua_S,"IMAP_MOM_EXTERNAL_FIXED",IMAP_MOM_EXTERNAL_FIXED);
   tolua_constant(tolua_S,"IMAP_HOLD_SLEWVIEWLEFT",IMAP_HOLD_SLEWVIEWLEFT);
   tolua_constant(tolua_S,"IMAP_HOLD_SLEWVIEWRIGHT",IMAP_HOLD_SLEWVIEWRIGHT);
   tolua_constant(tolua_S,"IMAP_HOLD_SLEWVIEWUP",IMAP_HOLD_SLEWVIEWUP);
   tolua_constant(tolua_S,"IMAP_HOLD_SLEWVIEWDOWN",IMAP_HOLD_SLEWVIEWDOWN);
   tolua_constant(tolua_S,"IMAP_HOLD_SLEWVIEWIN",IMAP_HOLD_SLEWVIEWIN);
   tolua_constant(tolua_S,"IMAP_HOLD_SLEWVIEWOUT",IMAP_HOLD_SLEWVIEWOUT);
   tolua_constant(tolua_S,"IMAP_HOLD_ROLLVIEWLEFT",IMAP_HOLD_ROLLVIEWLEFT);
   tolua_constant(tolua_S,"IMAP_HOLD_ROLLVIEWRIGHT",IMAP_HOLD_ROLLVIEWRIGHT);
   tolua_constant(tolua_S,"IMAP_MOM_SET_FIXED_CAMERA",IMAP_MOM_SET_FIXED_CAMERA);
   tolua_constant(tolua_S,"IMAP_TMP_CHANGE_FOV",IMAP_TMP_CHANGE_FOV);
   tolua_constant(tolua_S,"IMAP_TMP_CHANGE_FOV_UP",IMAP_TMP_CHANGE_FOV_UP);
   tolua_constant(tolua_S,"IMAP_TMP_CHANGE_FOV_DOWN",IMAP_TMP_CHANGE_FOV_DOWN);
   tolua_constant(tolua_S,"IMAP_HOLD_ZOOMIN",IMAP_HOLD_ZOOMIN);
   tolua_constant(tolua_S,"IMAP_HOLD_ZOOMOUT",IMAP_HOLD_ZOOMOUT);
   tolua_constant(tolua_S,"IMAP_MOM_SHOW_GUI",IMAP_MOM_SHOW_GUI);
   tolua_constant(tolua_S,"IMAP_MOM_SHOW_GUI_CURSOR",IMAP_MOM_SHOW_GUI_CURSOR);
   tolua_constant(tolua_S,"IMAP_MOM_SHOW_GUI_CHAT_WINDOW",IMAP_MOM_SHOW_GUI_CHAT_WINDOW);
   tolua_constant(tolua_S,"IMAP_MOM_SHOW_RADIO_BUFFER",IMAP_MOM_SHOW_RADIO_BUFFER);
   tolua_constant(tolua_S,"IMAP_MOM_UI_MAP_ZOOMIN",IMAP_MOM_UI_MAP_ZOOMIN);
   tolua_constant(tolua_S,"IMAP_MOM_UI_MAP_ZOOMOUT",IMAP_MOM_UI_MAP_ZOOMOUT);
   tolua_constant(tolua_S,"IMAP_CONTINUE_TUTORIAL",IMAP_CONTINUE_TUTORIAL);
   tolua_constant(tolua_S,"IMAP_MOM_SHOW_IN_GAME_MAP",IMAP_MOM_SHOW_IN_GAME_MAP);
   tolua_constant(tolua_S,"IMAP_MOM_SHOW_IN_GAME_KEYMAPPER",IMAP_MOM_SHOW_IN_GAME_KEYMAPPER);
   tolua_constant(tolua_S,"IMAP_MOM_TAKE_SCREEN_SHOT",IMAP_MOM_TAKE_SCREEN_SHOT);
   tolua_constant(tolua_S,"IMAP_MOM_TOGGLE_HUD",IMAP_MOM_TOGGLE_HUD);
   tolua_constant(tolua_S,"IMAP_MOM_RESET_HUD",IMAP_MOM_RESET_HUD);
   tolua_constant(tolua_S,"IMAP_TMP_EXIT_VEHICLE",IMAP_TMP_EXIT_VEHICLE);
   tolua_constant(tolua_S,"IMAP_SHOW_INFO",IMAP_SHOW_INFO);
   tolua_constant(tolua_S,"IMAP_SHOW_STATS",IMAP_SHOW_STATS);
   tolua_constant(tolua_S,"IMAP_CYCLE_ICON_MODE",IMAP_CYCLE_ICON_MODE);
   tolua_constant(tolua_S,"IMAP_MOM_TOGGLE_CHATHEIGHT",IMAP_MOM_TOGGLE_CHATHEIGHT);
   tolua_constant(tolua_S,"IMAP_CLONE_VEHICLE",IMAP_CLONE_VEHICLE);
   tolua_constant(tolua_S,"IMAP_TMP_INSTRUMENT_VIEW",IMAP_TMP_INSTRUMENT_VIEW);
   tolua_constant(tolua_S,"IMAP_TMP_GUNSIGHT_VIEW",IMAP_TMP_GUNSIGHT_VIEW);
   tolua_constant(tolua_S,"IMAP_TMP_CYCLE_AMMO",IMAP_TMP_CYCLE_AMMO);
   tolua_constant(tolua_S,"IMAP_TMP_RANGEFIND",IMAP_TMP_RANGEFIND);
   tolua_constant(tolua_S,"IMAP_HOLD_MAINTRIGGER",IMAP_HOLD_MAINTRIGGER);
   tolua_constant(tolua_S,"IMAP_HOLD_SECONDTRIGGER",IMAP_HOLD_SECONDTRIGGER);
   tolua_constant(tolua_S,"IMAP_JETTISON_ORDNANCE",IMAP_JETTISON_ORDNANCE);
   tolua_constant(tolua_S,"IMAP_HOLD_GUNSIGHT_RANGE_DEC",IMAP_HOLD_GUNSIGHT_RANGE_DEC);
   tolua_constant(tolua_S,"IMAP_HOLD_GUNSIGHT_RANGE_INC",IMAP_HOLD_GUNSIGHT_RANGE_INC);
   tolua_constant(tolua_S,"IMAP_HOLD_GVALUE_INC", IMAP_HOLD_GVALUE_INC);
   tolua_constant(tolua_S,"IMAP_HOLD_GVALUE_DEC", IMAP_HOLD_GVALUE_DEC);
   tolua_constant(tolua_S,"IMAP_HOLD_GXVALUE_INC",IMAP_HOLD_GXVALUE_INC);
   tolua_constant(tolua_S,"IMAP_HOLD_GXVALUE_DEC",IMAP_HOLD_GXVALUE_DEC);
   tolua_constant(tolua_S,"IMAP_HOLD_GYVALUE_INC",IMAP_HOLD_GYVALUE_INC);
   tolua_constant(tolua_S,"IMAP_HOLD_GYVALUE_DEC",IMAP_HOLD_GYVALUE_DEC);
   tolua_constant(tolua_S,"IMAP_HOLD_GZVALUE_INC",IMAP_HOLD_GZVALUE_INC);
   tolua_constant(tolua_S,"IMAP_HOLD_GZVALUE_DEC",IMAP_HOLD_GZVALUE_DEC);
   tolua_constant(tolua_S,"IMAP_TOGGLE", IMAP_TOGGLE);
   tolua_constant(tolua_S,"IMAP_TOGGLE1",IMAP_TOGGLE1);
   tolua_constant(tolua_S,"IMAP_TOGGLE2",IMAP_TOGGLE2);
   tolua_constant(tolua_S,"IMAP_TOGGLE3",IMAP_TOGGLE3);
   tolua_constant(tolua_S,"IMAP_HOLD_BOMBSIGHT_SPEED_DEC",IMAP_HOLD_BOMBSIGHT_SPEED_DEC);
   tolua_constant(tolua_S,"IMAP_HOLD_BOMBSIGHT_SPEED_INC",IMAP_HOLD_BOMBSIGHT_SPEED_INC);
   tolua_constant(tolua_S,"IMAP_HOLD_BOMBSIGHT_ALT_DEC",IMAP_HOLD_BOMBSIGHT_ALT_DEC);
   tolua_constant(tolua_S,"IMAP_HOLD_BOMBSIGHT_ALT_INC",IMAP_HOLD_BOMBSIGHT_ALT_INC);
   tolua_constant(tolua_S,"IMAP_TARGET_SPEED_INC",IMAP_TARGET_SPEED_INC);
   tolua_constant(tolua_S,"IMAP_TARGET_SPEED_DEC",IMAP_TARGET_SPEED_DEC);
   tolua_constant(tolua_S,"IMAP_TARGET_ANGLE_OFF_INC",IMAP_TARGET_ANGLE_OFF_INC);
   tolua_constant(tolua_S,"IMAP_TARGET_ANGLE_OFF_DEC",IMAP_TARGET_ANGLE_OFF_DEC);
   tolua_constant(tolua_S,"IMAP_TARGET_MARK_BEARING",IMAP_TARGET_MARK_BEARING);
   tolua_constant(tolua_S,"IMAP_TARGET_CALC_SOLUTION",IMAP_TARGET_CALC_SOLUTION);
   tolua_constant(tolua_S,"IMAP_GUNSIGHT_CONVERGENCE_INC",IMAP_GUNSIGHT_CONVERGENCE_INC);
   tolua_constant(tolua_S,"IMAP_GUNSIGHT_CONVERGENCE_DEC",IMAP_GUNSIGHT_CONVERGENCE_DEC);
   tolua_constant(tolua_S,"IMAP_MOM_POSITION_1",IMAP_MOM_POSITION_1);
   tolua_constant(tolua_S,"IMAP_MOM_POSITION_2",IMAP_MOM_POSITION_2);
   tolua_constant(tolua_S,"IMAP_MOM_POSITION_3",IMAP_MOM_POSITION_3);
   tolua_constant(tolua_S,"IMAP_MOM_POSITION_4",IMAP_MOM_POSITION_4);
   tolua_constant(tolua_S,"IMAP_MOM_POSITION_5",IMAP_MOM_POSITION_5);
   tolua_constant(tolua_S,"IMAP_MOM_POSITION_6",IMAP_MOM_POSITION_6);
   tolua_constant(tolua_S,"IMAP_MOM_POSITION_7",IMAP_MOM_POSITION_7);
   tolua_constant(tolua_S,"IMAP_MOM_POSITION_8",IMAP_MOM_POSITION_8);
   tolua_constant(tolua_S,"IMAP_MOM_POSITION_9",IMAP_MOM_POSITION_9);
   tolua_constant(tolua_S,"IMAP_MOM_POSITION_10",IMAP_MOM_POSITION_10);
   tolua_constant(tolua_S,"IMAP_LOOK_START",IMAP_LOOK_START);
   tolua_constant(tolua_S,"IMAP_LOOK_FORWARD",IMAP_LOOK_FORWARD);
   tolua_constant(tolua_S,"IMAP_LOOK_BACK",IMAP_LOOK_BACK);
   tolua_constant(tolua_S,"IMAP_LOOK_UP",IMAP_LOOK_UP);
   tolua_constant(tolua_S,"IMAP_LOOK_DOWN",IMAP_LOOK_DOWN);
   tolua_constant(tolua_S,"IMAP_LOOK_FORWARD_RIGHT",IMAP_LOOK_FORWARD_RIGHT);
   tolua_constant(tolua_S,"IMAP_LOOK_FORWARD_LEFT",IMAP_LOOK_FORWARD_LEFT);
   tolua_constant(tolua_S,"IMAP_LOOK_RIGHT",IMAP_LOOK_RIGHT);
   tolua_constant(tolua_S,"IMAP_LOOK_LEFT",IMAP_LOOK_LEFT);
   tolua_constant(tolua_S,"IMAP_LOOK_BACK_RIGHT",IMAP_LOOK_BACK_RIGHT);
   tolua_constant(tolua_S,"IMAP_LOOK_BACK_LEFT",IMAP_LOOK_BACK_LEFT);
   tolua_constant(tolua_S,"IMAP_LOOK_FORWARD_UP",IMAP_LOOK_FORWARD_UP);
   tolua_constant(tolua_S,"IMAP_LOOK_FORWARD_RIGHT_UP",IMAP_LOOK_FORWARD_RIGHT_UP);
   tolua_constant(tolua_S,"IMAP_LOOK_FORWARD_LEFT_UP",IMAP_LOOK_FORWARD_LEFT_UP);
   tolua_constant(tolua_S,"IMAP_LOOK_RIGHT_UP",IMAP_LOOK_RIGHT_UP);
   tolua_constant(tolua_S,"IMAP_LOOK_LEFT_UP",IMAP_LOOK_LEFT_UP);
   tolua_constant(tolua_S,"IMAP_LOOK_BACK_RIGHT_UP",IMAP_LOOK_BACK_RIGHT_UP);
   tolua_constant(tolua_S,"IMAP_LOOK_BACK_LEFT_UP",IMAP_LOOK_BACK_LEFT_UP);
   tolua_constant(tolua_S,"IMAP_LOOK_FORWARD_DOWN",IMAP_LOOK_FORWARD_DOWN);
   tolua_constant(tolua_S,"IMAP_LOOK_FORWARD_RIGHT_DOWN",IMAP_LOOK_FORWARD_RIGHT_DOWN);
   tolua_constant(tolua_S,"IMAP_LOOK_FORWARD_LEFT_DOWN",IMAP_LOOK_FORWARD_LEFT_DOWN);
   tolua_constant(tolua_S,"IMAP_LOOK_RIGHT_DOWN",IMAP_LOOK_RIGHT_DOWN);
   tolua_constant(tolua_S,"IMAP_LOOK_LEFT_DOWN",IMAP_LOOK_LEFT_DOWN);
   tolua_constant(tolua_S,"IMAP_LOOK_BACK_RIGHT_DOWN",IMAP_LOOK_BACK_RIGHT_DOWN);
   tolua_constant(tolua_S,"IMAP_LOOK_BACK_LEFT_DOWN",IMAP_LOOK_BACK_LEFT_DOWN);
   tolua_constant(tolua_S,"IMAP_LOOK_BACK_DOWN",IMAP_LOOK_BACK_DOWN);
   tolua_constant(tolua_S,"IMAP_LOOK_BACK_UP",IMAP_LOOK_BACK_UP);
   tolua_constant(tolua_S,"IMAP_LOOK_END",IMAP_LOOK_END);
   tolua_constant(tolua_S,"IMAP_TOGGLE_MOUSE_LOOK",IMAP_TOGGLE_MOUSE_LOOK);
   tolua_constant(tolua_S,"IMAP_MOUSE_LOOK_X",IMAP_MOUSE_LOOK_X);
   tolua_constant(tolua_S,"IMAP_MOUSE_LOOK_Y",IMAP_MOUSE_LOOK_Y);
   tolua_constant(tolua_S,"IMAP_MOUSE_LOOK_Z",IMAP_MOUSE_LOOK_Z);
   tolua_constant(tolua_S,"IMAP_HEAD_TRANSLATE_X",IMAP_HEAD_TRANSLATE_X);
   tolua_constant(tolua_S,"IMAP_HEAD_TRANSLATE_Y",IMAP_HEAD_TRANSLATE_Y);
   tolua_constant(tolua_S,"IMAP_HEAD_TRANSLATE_Z",IMAP_HEAD_TRANSLATE_Z);
   tolua_constant(tolua_S,"IMAP_TOGGLE_HEAD_LOOK",IMAP_TOGGLE_HEAD_LOOK);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_SPRINT",IMAP_HOLD_FP_SPRINT);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_AUTORUN",IMAP_HOLD_FP_AUTORUN);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_CROUCH",IMAP_HOLD_FP_CROUCH);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_PRONE",IMAP_HOLD_FP_PRONE);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_FORWARD",IMAP_HOLD_FP_FORWARD);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_BACKWARD",IMAP_HOLD_FP_BACKWARD);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_STRAFE_RIGHT",IMAP_HOLD_FP_STRAFE_RIGHT);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_STRAFE_LEFT",IMAP_HOLD_FP_STRAFE_LEFT);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_TURN_RIGHT",IMAP_HOLD_FP_TURN_RIGHT);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_TURN_LEFT",IMAP_HOLD_FP_TURN_LEFT);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_LOOK_LEFT",IMAP_HOLD_FP_LOOK_LEFT);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_LOOK_RIGHT",IMAP_HOLD_FP_LOOK_RIGHT);
   tolua_constant(tolua_S,"IMAP_MOM_FP_RIDE_UNRIDE_VEHICLE",IMAP_MOM_FP_RIDE_UNRIDE_VEHICLE);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_RELOAD",IMAP_HOLD_FP_RELOAD);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_DEPLOY",IMAP_HOLD_FP_DEPLOY);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_GUNSIGHT",IMAP_HOLD_FP_GUNSIGHT);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_FIRE",IMAP_HOLD_FP_FIRE);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_RELAXED",IMAP_HOLD_FP_RELAXED);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_WALK",IMAP_HOLD_FP_WALK);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_LEAN_LEFT",IMAP_HOLD_FP_LEAN_LEFT);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_LEAN_RIGHT",IMAP_HOLD_FP_LEAN_RIGHT);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_NEXT_WEAPON",IMAP_HOLD_FP_NEXT_WEAPON);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_PREV_WEAPON",IMAP_HOLD_FP_PREV_WEAPON);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_WEAPON_1",IMAP_HOLD_FP_WEAPON_1);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_WEAPON_2",IMAP_HOLD_FP_WEAPON_2);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_WEAPON_3",IMAP_HOLD_FP_WEAPON_3);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_WEAPON_4",IMAP_HOLD_FP_WEAPON_4);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_WEAPON_5",IMAP_HOLD_FP_WEAPON_5);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_WEAPON_6",IMAP_HOLD_FP_WEAPON_6);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_WEAPON_7",IMAP_HOLD_FP_WEAPON_7);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_WEAPON_8",IMAP_HOLD_FP_WEAPON_8);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_WEAPON_9",IMAP_HOLD_FP_WEAPON_9);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_WEAPON_10",IMAP_HOLD_FP_WEAPON_10);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_CE_DOUBLE",IMAP_HOLD_FP_CE_DOUBLE);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_CE_GETDOWN",IMAP_HOLD_FP_CE_GETDOWN);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_CE_HALT",IMAP_HOLD_FP_CE_HALT);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_CE_MOVEOUT",IMAP_HOLD_FP_CE_MOVEOUT);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_CE_AMMO",IMAP_HOLD_FP_CE_AMMO);
   tolua_constant(tolua_S,"IMAP_HOLD_FP_CE_JUMP",IMAP_HOLD_FP_CE_JUMP);
   tolua_constant(tolua_S,"IMAP_ASX_FP_TORSO_VERTICAL_AXIS",IMAP_ASX_FP_TORSO_VERTICAL_AXIS);
   tolua_constant(tolua_S,"IMAP_ASX_FP_TORSO_HORIZONTAL_AXIS",IMAP_ASX_FP_TORSO_HORIZONTAL_AXIS);
   tolua_constant(tolua_S,"IMAP_ASX_ROLL",IMAP_ASX_ROLL);
   tolua_constant(tolua_S,"IMAP_ASX_PITCH",IMAP_ASX_PITCH);
   tolua_constant(tolua_S,"IMAP_ASX_YAW",IMAP_ASX_YAW);
   tolua_constant(tolua_S,"IMAP_ASX_THROTTLE",IMAP_ASX_THROTTLE);
   tolua_constant(tolua_S,"IMAP_ASX_GAS",IMAP_ASX_GAS);
   tolua_constant(tolua_S,"IMAP_ASX_CLUTCH",IMAP_ASX_CLUTCH);
   tolua_constant(tolua_S,"IMAP_ASX_LEFTTRACKCLUTCH",IMAP_ASX_LEFTTRACKCLUTCH);
   tolua_constant(tolua_S,"IMAP_ASX_RIGHTTRACKCLUTCH",IMAP_ASX_RIGHTTRACKCLUTCH);
   tolua_constant(tolua_S,"IMAP_ASX_FLAPCONTROL",IMAP_ASX_FLAPCONTROL);
   tolua_constant(tolua_S,"IMAP_ASX_LEFTBRAKE",IMAP_ASX_LEFTBRAKE);
   tolua_constant(tolua_S,"IMAP_ASX_RIGHTBRAKE",IMAP_ASX_RIGHTBRAKE);
   tolua_constant(tolua_S,"IMAP_ASX_CENTERBRAKE",IMAP_ASX_CENTERBRAKE);
   tolua_constant(tolua_S,"IMAP_ASX_STEERING",IMAP_ASX_STEERING);
   tolua_constant(tolua_S,"IMAP_ASX_TURRET_TRAVERSE",IMAP_ASX_TURRET_TRAVERSE);
   tolua_constant(tolua_S,"IMAP_ASX_TURRET_ELEVATE",IMAP_ASX_TURRET_ELEVATE);
   tolua_constant(tolua_S,"IMAP_ASX_ELEV_TRIM",IMAP_ASX_ELEV_TRIM);
   tolua_constant(tolua_S,"IMAP_ASX_AILERON_TRIM",IMAP_ASX_AILERON_TRIM);
   tolua_constant(tolua_S,"IMAP_ASX_RUDDER_TRIM",IMAP_ASX_RUDDER_TRIM);
   tolua_constant(tolua_S,"IMAP_ASX_ATGUN_MOVEMENT_LEFTRIGHT",IMAP_ASX_ATGUN_MOVEMENT_LEFTRIGHT);
   tolua_constant(tolua_S,"IMAP_ASX_ATGUN_MOVEMENT_FORWARDBACKWARDS",IMAP_ASX_ATGUN_MOVEMENT_FORWARDBACKWARDS);
   tolua_constant(tolua_S,"IMAP_ASX_WINCH",IMAP_ASX_WINCH);
   tolua_constant(tolua_S,"IMAP_CENTER_AXIS",IMAP_CENTER_AXIS);
   tolua_constant(tolua_S,"IMAP_MOM_STUKASIREN",IMAP_MOM_STUKASIREN);
   tolua_constant(tolua_S,"IMAP_MOM_SUPERCHARGER_STAGE1",IMAP_MOM_SUPERCHARGER_STAGE1);
   tolua_constant(tolua_S,"IMAP_MOM_SUPERCHARGER_STAGE2",IMAP_MOM_SUPERCHARGER_STAGE2);
   tolua_constant(tolua_S,"IMAP_MOM_SUPERCHARGER_NEUTRAL",IMAP_MOM_SUPERCHARGER_NEUTRAL);
   tolua_constant(tolua_S,"IMAP_MOM_LLSWITCH",IMAP_MOM_LLSWITCH);
   tolua_constant(tolua_S,"IMAP_MOM_FUELCOCK_NEXTTANK",IMAP_MOM_FUELCOCK_NEXTTANK);
   tolua_constant(tolua_S,"IMAP_MOM_COWLFLAPS",IMAP_MOM_COWLFLAPS);
   tolua_constant(tolua_S,"IMAP_MOM_AUXSIGHT",IMAP_MOM_AUXSIGHT);
   tolua_constant(tolua_S,"IMAP_MOM_RADIO",IMAP_MOM_RADIO);
   tolua_constant(tolua_S,"IMAP_MOM_ORDTRIGGER",IMAP_MOM_ORDTRIGGER);
   tolua_constant(tolua_S,"IMAP_MOM_ORDSELECTOR",IMAP_MOM_ORDSELECTOR);
   tolua_constant(tolua_S,"IMAP_MOM_AUTODIVE",IMAP_MOM_AUTODIVE);
   tolua_constant(tolua_S,"IMAP_MOM_MAP",IMAP_MOM_MAP);
   tolua_constant(tolua_S,"IMAP_MOM_FLAPEXECUTE",IMAP_MOM_FLAPEXECUTE);
   tolua_constant(tolua_S,"IMAP_MOM_ADJUST_PROP_UP",IMAP_MOM_ADJUST_PROP_UP);
   tolua_constant(tolua_S,"IMAP_MOM_ADJUST_PROP_DOWN",IMAP_MOM_ADJUST_PROP_DOWN);
   tolua_constant(tolua_S,"IMAP_ASX_TURRET_TRAVERSE_SECONDARY",IMAP_ASX_TURRET_TRAVERSE_SECONDARY);
   tolua_constant(tolua_S,"IMAP_ASX_TURRET_ELEVATE_SECONDARY",IMAP_ASX_TURRET_ELEVATE_SECONDARY);
   tolua_constant(tolua_S,"IMAP_ASX_RPMLEVER",IMAP_ASX_RPMLEVER);
   tolua_constant(tolua_S,"IMAP_ASX_PITCHLEVER",IMAP_ASX_PITCHLEVER);
   tolua_constant(tolua_S,"IMAP_ASX_FLAPLEVER",IMAP_ASX_FLAPLEVER);
   tolua_constant(tolua_S,"IMAP_BUZZARD_FORWARD",IMAP_BUZZARD_FORWARD);
   tolua_constant(tolua_S,"IMAP_BUZZARD_BACK",IMAP_BUZZARD_BACK);
   tolua_constant(tolua_S,"IMAP_BUZZARD_LEFT",IMAP_BUZZARD_LEFT);
   tolua_constant(tolua_S,"IMAP_BUZZARD_RIGHT",IMAP_BUZZARD_RIGHT);
   tolua_constant(tolua_S,"IMAP_BUZZARD_UP",IMAP_BUZZARD_UP);
   tolua_constant(tolua_S,"IMAP_BUZZARD_DOWN",IMAP_BUZZARD_DOWN);
   tolua_constant(tolua_S,"IMAP_BUZZARD_PITCH_UP",IMAP_BUZZARD_PITCH_UP);
   tolua_constant(tolua_S,"IMAP_BUZZARD_PITCH_DOWN",IMAP_BUZZARD_PITCH_DOWN);
   tolua_constant(tolua_S,"IMAP_BUZZARD_YAW_LEFT",IMAP_BUZZARD_YAW_LEFT);
   tolua_constant(tolua_S,"IMAP_BUZZARD_YAW_RIGHT",IMAP_BUZZARD_YAW_RIGHT);
   tolua_constant(tolua_S,"IMAP_BUZZARD_SPEED1",IMAP_BUZZARD_SPEED1);
   tolua_constant(tolua_S,"IMAP_BUZZARD_SPEED2",IMAP_BUZZARD_SPEED2);
   tolua_constant(tolua_S,"IMAP_BUZZARD_SPEED3",IMAP_BUZZARD_SPEED3);
   tolua_constant(tolua_S,"IMAP_EZ_MOVE",IMAP_EZ_MOVE);
   tolua_constant(tolua_S,"IMAP_EZ_TURN",IMAP_EZ_TURN);
   tolua_constant(tolua_S,"IMAP_EZ_ACTION1",IMAP_EZ_ACTION1);
   tolua_constant(tolua_S,"IMAP_EZ_ACTION2",IMAP_EZ_ACTION2);
   tolua_constant(tolua_S,"IMAP_EZ_FREELOOK",IMAP_EZ_FREELOOK);
   tolua_constant(tolua_S,"IMAP_EZ_CAMERA_X",IMAP_EZ_CAMERA_X);
   tolua_constant(tolua_S,"IMAP_EZ_CAMERA_Y",IMAP_EZ_CAMERA_Y);
   tolua_constant(tolua_S,"IMAP_EZ_NEXTPOS",IMAP_EZ_NEXTPOS);
   tolua_constant(tolua_S,"IMAP_EZ_PREVPOS",IMAP_EZ_PREVPOS);
   tolua_constant(tolua_S,"IMAP_EZ_ENGINE",IMAP_EZ_ENGINE);
   tolua_constant(tolua_S,"WW2_FUNC_CATEGORY_GENERAL",WW2_FUNC_CATEGORY_GENERAL);
   tolua_constant(tolua_S,"WW2_FUNC_CATEGORY_VIEW",WW2_FUNC_CATEGORY_VIEW);
   tolua_constant(tolua_S,"WW2_FUNC_CATEGORY_AIR",WW2_FUNC_CATEGORY_AIR);
   tolua_constant(tolua_S,"WW2_FUNC_CATEGORY_TANK",WW2_FUNC_CATEGORY_TANK);
   tolua_constant(tolua_S,"WW2_FUNC_CATEGORY_TRUCK",WW2_FUNC_CATEGORY_TRUCK);
   tolua_constant(tolua_S,"WW2_FUNC_CATEGORY_ATGUN",WW2_FUNC_CATEGORY_ATGUN);
   tolua_constant(tolua_S,"WW2_FUNC_CATEGORY_PERSON",WW2_FUNC_CATEGORY_PERSON);
   tolua_constant(tolua_S,"WW2_FUNC_CATEGORY_SEA",WW2_FUNC_CATEGORY_SEA);
   tolua_constant(tolua_S,"WW2_FUNC_CATEGORY_EZ",WW2_FUNC_CATEGORY_EZ);
   tolua_constant(tolua_S,"WW2_FUNC_NUM_CATEGORIES",WW2_FUNC_NUM_CATEGORIES);
   tolua_function(tolua_S,"ww2GetFuncName",tolua_imap_input_ww2GetFuncName00);
   tolua_function(tolua_S,"ww2funcHeader",tolua_imap_input_ww2funcHeader00);
   tolua_function(tolua_S,"ww2FunctionControlName",tolua_imap_input_ww2FunctionControlName00);
   tolua_function(tolua_S,"ww2KeymapInit",tolua_imap_input_ww2KeymapInit00);
   tolua_function(tolua_S,"ww2KeymapKill",tolua_imap_input_ww2KeymapKill00);
   tolua_function(tolua_S,"ww2imapGetFuncKeyname",tolua_imap_input_ww2imapGetFuncKeyname00);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_imap (lua_State* tolua_S) {
 return tolua_imap_open(tolua_S);
};
#endif


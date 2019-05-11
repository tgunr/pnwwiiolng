#include "syscfg.h" // Path is project specific.
/*
** Lua binding: fp
** Generated automatically by tolua++-1.0.92 on 01/04/14 22:00:08.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_fp_open (lua_State* tolua_S);

#include "fp/fpFPS.h"
#include "fp/fpFPSdata.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"ChatHandle");
}

/* function: fpFPSGetPlayerPose */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSGetPlayerPose00
static int tolua_fp_fp_fpFPSGetPlayerPose00(lua_State* tolua_S)
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
   fpFPSPoses tolua_ret = (fpFPSPoses)  fpFPSGetPlayerPose();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSGetPlayerPose'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: fpFPSControlInput */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSControlInput00
static int tolua_fp_fp_fpFPSControlInput00(lua_State* tolua_S)
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
   int Param = ((  int)  tolua_tonumber(tolua_S,1,0));
  unsigned int Id = (( unsigned int)  tolua_tonumber(tolua_S,2,0));
  unsigned int reserve = (( unsigned int)  tolua_tonumber(tolua_S,3,0));
  {
   unsigned int tolua_ret = ( unsigned int)  fpFPSControlInput(Param,Id,reserve);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSControlInput'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: fpFPSKillTrooper */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSKillTrooper00
static int tolua_fp_fp_fpFPSKillTrooper00(lua_State* tolua_S)
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
   fpFPSKillTrooper();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSKillTrooper'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: fpFPSGetCurrentWeapon */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSGetCurrentWeapon00
static int tolua_fp_fp_fpFPSGetCurrentWeapon00(lua_State* tolua_S)
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
   unsigned int tolua_ret = ( unsigned int)  fpFPSGetCurrentWeapon();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSGetCurrentWeapon'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: fpFPSIsReloading */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSIsReloading00
static int tolua_fp_fp_fpFPSIsReloading00(lua_State* tolua_S)
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
   unsigned int tolua_ret = ( unsigned int)  fpFPSIsReloading();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSIsReloading'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: fpFPSIsSwimming */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSIsSwimming00
static int tolua_fp_fp_fpFPSIsSwimming00(lua_State* tolua_S)
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
   unsigned int tolua_ret = ( unsigned int)  fpFPSIsSwimming();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSIsSwimming'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: fpFPSIsWading */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSIsWading00
static int tolua_fp_fp_fpFPSIsWading00(lua_State* tolua_S)
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
   unsigned int tolua_ret = ( unsigned int)  fpFPSIsWading();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSIsWading'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: fpFPSIsProne */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSIsProne00
static int tolua_fp_fp_fpFPSIsProne00(lua_State* tolua_S)
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
   unsigned int tolua_ret = ( unsigned int)  fpFPSIsProne();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSIsProne'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: fpFPSIsCrouch */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSIsCrouch00
static int tolua_fp_fp_fpFPSIsCrouch00(lua_State* tolua_S)
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
   unsigned int tolua_ret = ( unsigned int)  fpFPSIsCrouch();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSIsCrouch'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: fpFPSIsAim */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSIsAim00
static int tolua_fp_fp_fpFPSIsAim00(lua_State* tolua_S)
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
   unsigned int tolua_ret = ( unsigned int)  fpFPSIsAim();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSIsAim'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: fpFPSIsFast */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSIsFast00
static int tolua_fp_fp_fpFPSIsFast00(lua_State* tolua_S)
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
   unsigned int tolua_ret = ( unsigned int)  fpFPSIsFast();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSIsFast'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: fpFPSIsWalking */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSIsWalking00
static int tolua_fp_fp_fpFPSIsWalking00(lua_State* tolua_S)
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
   unsigned int tolua_ret = ( unsigned int)  fpFPSIsWalking();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSIsWalking'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: fpFPSIsDead */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSIsDead00
static int tolua_fp_fp_fpFPSIsDead00(lua_State* tolua_S)
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
   bool tolua_ret = (bool)  fpFPSIsDead();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSIsDead'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: fpFPSIsMoving */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSIsMoving00
static int tolua_fp_fp_fpFPSIsMoving00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isnumber(tolua_S,1,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  float threshold = ((float)  tolua_tonumber(tolua_S,1,0.0f));
  {
   unsigned int tolua_ret = ( unsigned int)  fpFPSIsMoving(threshold);
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSIsMoving'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: fpFPSResupplySelf */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSResupplySelf00
static int tolua_fp_fp_fpFPSResupplySelf00(lua_State* tolua_S)
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
  const char* from = ((const char*)  tolua_tostring(tolua_S,1,0));
  {
   fpFPSResupplySelf(from);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSResupplySelf'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: fpFPSSelectNextWeapon */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSSelectNextWeapon00
static int tolua_fp_fp_fpFPSSelectNextWeapon00(lua_State* tolua_S)
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
   fpFPSSelectNextWeapon();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSSelectNextWeapon'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: fpFPSSelectPreviousWeapon */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSSelectPreviousWeapon00
static int tolua_fp_fp_fpFPSSelectPreviousWeapon00(lua_State* tolua_S)
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
   fpFPSSelectPreviousWeapon();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSSelectPreviousWeapon'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: fpFPSProcessTPSEmote */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSProcessTPSEmote00
static int tolua_fp_fp_fpFPSProcessTPSEmote00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"ChatHandle",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  ChatHandle handle = *((ChatHandle*)  tolua_tousertype(tolua_S,1,0));
   int emoteID = ((  int)  tolua_tonumber(tolua_S,2,0));
  {
   fpFPSProcessTPSEmote(handle,emoteID);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSProcessTPSEmote'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: fpFPSControlBlocked */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSControlBlocked00
static int tolua_fp_fp_fpFPSControlBlocked00(lua_State* tolua_S)
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
  int flag = ((int)  tolua_tonumber(tolua_S,1,0));
  {
   bool tolua_ret = (bool)  fpFPSControlBlocked(flag);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSControlBlocked'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: fpFPSControlBlockSet */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSControlBlockSet00
static int tolua_fp_fp_fpFPSControlBlockSet00(lua_State* tolua_S)
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
  int flag = ((int)  tolua_tonumber(tolua_S,1,0));
  {
   fpFPSControlBlockSet(flag);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSControlBlockSet'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: fpFPSControlBlockClear */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSControlBlockClear00
static int tolua_fp_fp_fpFPSControlBlockClear00(lua_State* tolua_S)
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
  int flag = ((int)  tolua_tonumber(tolua_S,1,0));
  {
   fpFPSControlBlockClear(flag);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSControlBlockClear'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: fpFPSControlBlockReset */
#ifndef TOLUA_DISABLE_tolua_fp_fp_fpFPSControlBlockReset00
static int tolua_fp_fp_fpFPSControlBlockReset00(lua_State* tolua_S)
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
   fpFPSControlBlockReset();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'fpFPSControlBlockReset'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_fp_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_module(tolua_S,"fp",0);
  tolua_beginmodule(tolua_S,"fp");
   tolua_constant(tolua_S,"POSE_STAND",POSE_STAND);
   tolua_constant(tolua_S,"POSE_CROUCH",POSE_CROUCH);
   tolua_constant(tolua_S,"POSE_PRONE",POSE_PRONE);
   tolua_constant(tolua_S,"POSE_TRANSITION",POSE_TRANSITION);
   tolua_constant(tolua_S,"POSE_SIT",POSE_SIT);
   tolua_constant(tolua_S,"POSE_NUM_POSES",POSE_NUM_POSES);
   tolua_constant(tolua_S,"CB_NONE",CB_NONE);
   tolua_constant(tolua_S,"CB_MOVE",CB_MOVE);
   tolua_constant(tolua_S,"CB_POSE",CB_POSE);
   tolua_constant(tolua_S,"CB_LOOK",CB_LOOK);
   tolua_constant(tolua_S,"CB_FIRE",CB_FIRE);
   tolua_constant(tolua_S,"CB_SELECT",CB_SELECT);
   tolua_constant(tolua_S,"CB_DEPLOY",CB_DEPLOY);
   tolua_constant(tolua_S,"CB_NUM_BLOCKS",CB_NUM_BLOCKS);
   tolua_function(tolua_S,"fpFPSGetPlayerPose",tolua_fp_fp_fpFPSGetPlayerPose00);
   tolua_function(tolua_S,"fpFPSControlInput",tolua_fp_fp_fpFPSControlInput00);
   tolua_function(tolua_S,"fpFPSKillTrooper",tolua_fp_fp_fpFPSKillTrooper00);
   tolua_function(tolua_S,"fpFPSGetCurrentWeapon",tolua_fp_fp_fpFPSGetCurrentWeapon00);
   tolua_function(tolua_S,"fpFPSIsReloading",tolua_fp_fp_fpFPSIsReloading00);
   tolua_function(tolua_S,"fpFPSIsSwimming",tolua_fp_fp_fpFPSIsSwimming00);
   tolua_function(tolua_S,"fpFPSIsWading",tolua_fp_fp_fpFPSIsWading00);
   tolua_function(tolua_S,"fpFPSIsProne",tolua_fp_fp_fpFPSIsProne00);
   tolua_function(tolua_S,"fpFPSIsCrouch",tolua_fp_fp_fpFPSIsCrouch00);
   tolua_function(tolua_S,"fpFPSIsAim",tolua_fp_fp_fpFPSIsAim00);
   tolua_function(tolua_S,"fpFPSIsFast",tolua_fp_fp_fpFPSIsFast00);
   tolua_function(tolua_S,"fpFPSIsWalking",tolua_fp_fp_fpFPSIsWalking00);
   tolua_function(tolua_S,"fpFPSIsDead",tolua_fp_fp_fpFPSIsDead00);
   tolua_function(tolua_S,"fpFPSIsMoving",tolua_fp_fp_fpFPSIsMoving00);
   tolua_function(tolua_S,"fpFPSResupplySelf",tolua_fp_fp_fpFPSResupplySelf00);
   tolua_function(tolua_S,"fpFPSSelectNextWeapon",tolua_fp_fp_fpFPSSelectNextWeapon00);
   tolua_function(tolua_S,"fpFPSSelectPreviousWeapon",tolua_fp_fp_fpFPSSelectPreviousWeapon00);
   tolua_function(tolua_S,"fpFPSProcessTPSEmote",tolua_fp_fp_fpFPSProcessTPSEmote00);
   tolua_function(tolua_S,"fpFPSControlBlocked",tolua_fp_fp_fpFPSControlBlocked00);
   tolua_function(tolua_S,"fpFPSControlBlockSet",tolua_fp_fp_fpFPSControlBlockSet00);
   tolua_function(tolua_S,"fpFPSControlBlockClear",tolua_fp_fp_fpFPSControlBlockClear00);
   tolua_function(tolua_S,"fpFPSControlBlockReset",tolua_fp_fp_fpFPSControlBlockReset00);
   tolua_constant(tolua_S,"BRITISH_INFANTRY",BRITISH_INFANTRY);
   tolua_constant(tolua_S,"GERMAN_INFANTRY",GERMAN_INFANTRY);
   tolua_constant(tolua_S,"FRENCH_INFANTRY",FRENCH_INFANTRY);
   tolua_constant(tolua_S,"BRITISH_HC",BRITISH_HC);
   tolua_constant(tolua_S,"GERMAN_HC",GERMAN_HC);
   tolua_constant(tolua_S,"FRENCH_HC",FRENCH_HC);
   tolua_constant(tolua_S,"BRITISH_PARA_PACK",BRITISH_PARA_PACK);
   tolua_constant(tolua_S,"GERMAN_PARA_PACK",GERMAN_PARA_PACK);
   tolua_constant(tolua_S,"FRENCH_PARA_PACK",FRENCH_PARA_PACK);
   tolua_constant(tolua_S,"BRITISH_PARA_NOPACK",BRITISH_PARA_NOPACK);
   tolua_constant(tolua_S,"GERMAN_PARA_NOPACK",GERMAN_PARA_NOPACK);
   tolua_constant(tolua_S,"FRENCH_PARA_NOPACK",FRENCH_PARA_NOPACK);
   tolua_constant(tolua_S,"BRITISH_PARA_PACK_SELF",BRITISH_PARA_PACK_SELF);
   tolua_constant(tolua_S,"GERMAN_PARA_PACK_SELF",GERMAN_PARA_PACK_SELF);
   tolua_constant(tolua_S,"FRENCH_PARA_PACK_SELF",FRENCH_PARA_PACK_SELF);
   tolua_constant(tolua_S,"AMERICAN_PARA_PACK",AMERICAN_PARA_PACK);
   tolua_constant(tolua_S,"AMERICAN_PARA_NOPACK",AMERICAN_PARA_NOPACK);
   tolua_constant(tolua_S,"AMERICAN_PARA_PACK_SELF",AMERICAN_PARA_PACK_SELF);
   tolua_constant(tolua_S,"BRITISH_ML",BRITISH_ML);
   tolua_constant(tolua_S,"GERMAN_ML",GERMAN_ML);
   tolua_constant(tolua_S,"FRENCH_ML",FRENCH_ML);
   tolua_constant(tolua_S,"AMERICAN_INFANTRY",AMERICAN_INFANTRY);
   tolua_constant(tolua_S,"NUM_INFANTRY_TYPES",NUM_INFANTRY_TYPES);
   tolua_constant(tolua_S,"FPS_SPEED_STILL",FPS_SPEED_STILL);
   tolua_constant(tolua_S,"FPS_SPEED_SPRINT",FPS_SPEED_SPRINT);
   tolua_constant(tolua_S,"FPS_SPEED_TROT",FPS_SPEED_TROT);
   tolua_constant(tolua_S,"FPS_SPEED_QUICK",FPS_SPEED_QUICK);
   tolua_constant(tolua_S,"FPS_SPEED_WALK",FPS_SPEED_WALK);
   tolua_constant(tolua_S,"FPS_SPEED_CREEP",FPS_SPEED_CREEP);
   tolua_constant(tolua_S,"FPS_SPEED_CRAWL",FPS_SPEED_CRAWL);
   tolua_constant(tolua_S,"FPS_SPEED_SWIM",FPS_SPEED_SWIM);
   tolua_constant(tolua_S,"FPS_SPEED_BACKWARDS",FPS_SPEED_BACKWARDS);
   tolua_constant(tolua_S,"FPS_SPEED_BACKWARDS_CROUCH",FPS_SPEED_BACKWARDS_CROUCH);
   tolua_constant(tolua_S,"FPS_NUM_SPEEDS",FPS_NUM_SPEEDS);
   tolua_constant(tolua_S,"NO_WEAPON",NO_WEAPON);
   tolua_constant(tolua_S,"THOMPSON",THOMPSON);
   tolua_constant(tolua_S,"MP40",MP40);
   tolua_constant(tolua_S,"MAS",MAS);
   tolua_constant(tolua_S,"SATCHEL",SATCHEL);
   tolua_constant(tolua_S,"KAR98K",KAR98K);
   tolua_constant(tolua_S,"ENFIELD",ENFIELD);
   tolua_constant(tolua_S,"LEBEL",LEBEL);
   tolua_constant(tolua_S,"M1935A",M1935A);
   tolua_constant(tolua_S,"LUGER",LUGER);
   tolua_constant(tolua_S,"MILLS36MK1",MILLS36MK1);
   tolua_constant(tolua_S,"FRBANG",FRBANG);
   tolua_constant(tolua_S,"STIGR",STIGR);
   tolua_constant(tolua_S,"WEBLEY",WEBLEY);
   tolua_constant(tolua_S,"BINOCS",BINOCS);
   tolua_constant(tolua_S,"KNIFE_AXIS",KNIFE_AXIS);
   tolua_constant(tolua_S,"KNIFE_ALLIED",KNIFE_ALLIED);
   tolua_constant(tolua_S,"BREN",BREN);
   tolua_constant(tolua_S,"MG34",MG34);
   tolua_constant(tolua_S,"FM29",FM29);
   tolua_constant(tolua_S,"HGR39",HGR39);
   tolua_constant(tolua_S,"M18",M18);
   tolua_constant(tolua_S,"FRSMOKE",FRSMOKE);
   tolua_constant(tolua_S,"AMMO",AMMO);
   tolua_constant(tolua_S,"MAS36",MAS36);
   tolua_constant(tolua_S,"MAS40",MAS40);
   tolua_constant(tolua_S,"RG_G30",RG_G30);
   tolua_constant(tolua_S,"RG_VB",RG_VB);
   tolua_constant(tolua_S,"RG_MILLS",RG_MILLS);
   tolua_constant(tolua_S,"BOYSMK1",BOYSMK1);
   tolua_constant(tolua_S,"PZB39",PZB39);
   tolua_constant(tolua_S,"BRIDGE_REPAIR",BRIDGE_REPAIR);
   tolua_constant(tolua_S,"MAP",MAP);
   tolua_constant(tolua_S,"ENFIELDMK4",ENFIELDMK4);
   tolua_constant(tolua_S,"LEBELMLE",LEBELMLE);
   tolua_constant(tolua_S,"KAR98KS",KAR98KS);
   tolua_constant(tolua_S,"SATCHEL_AX",SATCHEL_AX);
   tolua_constant(tolua_S,"LTMORTAR_UK",LTMORTAR_UK);
   tolua_constant(tolua_S,"LTMORTAR_FR",LTMORTAR_FR);
   tolua_constant(tolua_S,"LTMORTAR_DE",LTMORTAR_DE);
   tolua_constant(tolua_S,"BUILD_TOOL",BUILD_TOOL);
   tolua_constant(tolua_S,"M1GARAND",M1GARAND);
   tolua_constant(tolua_S,"BAR",BAR);
   tolua_constant(tolua_S,"COLT1911",COLT1911);
   tolua_constant(tolua_S,"SPRINGFIELD1903",SPRINGFIELD1903);
   tolua_constant(tolua_S,"GEW43",GEW43);
   tolua_constant(tolua_S,"GEW41",GEW41);
   tolua_constant(tolua_S,"M1THOMPSON",M1THOMPSON);
   tolua_constant(tolua_S,"MARK2",MARK2);
   tolua_constant(tolua_S,"BAZOOKA",BAZOOKA);
   tolua_constant(tolua_S,"PZSCHRECK",PZSCHRECK);
   tolua_constant(tolua_S,"M1903A3",M1903A3);
   tolua_constant(tolua_S,"USSMOKE",USSMOKE);
   tolua_constant(tolua_S,"PIAT",PIAT);
   tolua_constant(tolua_S,"STENMK2",STENMK2);
   tolua_constant(tolua_S,"M3A1",M3A1);
   tolua_constant(tolua_S,"MP34",MP34);
   tolua_constant(tolua_S,"FG42G",FG42G);
   tolua_constant(tolua_S,"NUM_WEAPONS",NUM_WEAPONS);
   tolua_constant(tolua_S,"RELOAD",RELOAD);
   tolua_constant(tolua_S,"STAND_FIRE",STAND_FIRE);
   tolua_constant(tolua_S,"CROUCH_FIRE",CROUCH_FIRE);
   tolua_constant(tolua_S,"AIM_FIRE",AIM_FIRE);
   tolua_constant(tolua_S,"RECYCLE",RECYCLE);
   tolua_constant(tolua_S,"LOWER",LOWER);
   tolua_constant(tolua_S,"RAISE",RAISE);
   tolua_constant(tolua_S,"IDLE",IDLE);
   tolua_constant(tolua_S,"KNEEL",KNEEL);
   tolua_constant(tolua_S,"AIM",AIM);
   tolua_constant(tolua_S,"TROT",TROT);
   tolua_constant(tolua_S,"CREEP",CREEP);
   tolua_constant(tolua_S,"CRAWL",CRAWL);
   tolua_constant(tolua_S,"SPRINT",SPRINT);
   tolua_constant(tolua_S,"PRONE",PRONE);
   tolua_constant(tolua_S,"PRONE_FIRE",PRONE_FIRE);
   tolua_constant(tolua_S,"SIT",SIT);
   tolua_constant(tolua_S,"RELOAD2",RELOAD2);
   tolua_constant(tolua_S,"RELOAD3",RELOAD3);
   tolua_constant(tolua_S,"THROW",THROW);
   tolua_constant(tolua_S,"DEPLOYED",DEPLOYED);
   tolua_constant(tolua_S,"DEPLOYING",DEPLOYING);
   tolua_constant(tolua_S,"UNDEPLOYING",UNDEPLOYING);
   tolua_constant(tolua_S,"DEPLOYED_FIRE",DEPLOYED_FIRE);
   tolua_constant(tolua_S,"DEPLOYED_RELOAD",DEPLOYED_RELOAD);
   tolua_constant(tolua_S,"DEPLOYED_KNEEL",DEPLOYED_KNEEL);
   tolua_constant(tolua_S,"AIM_SMG",AIM_SMG);
   tolua_constant(tolua_S,"NUM_WEAP_STATES",NUM_WEAP_STATES);
   tolua_constant(tolua_S,"FPS_WEAPONTYPE_SMG",FPS_WEAPONTYPE_SMG);
   tolua_constant(tolua_S,"FPS_WEAPONTYPE_SATCHEL",FPS_WEAPONTYPE_SATCHEL);
   tolua_constant(tolua_S,"FPS_WEAPONTYPE_RIFLE",FPS_WEAPONTYPE_RIFLE);
   tolua_constant(tolua_S,"FPS_WEAPONTYPE_AUTO_PISTOL",FPS_WEAPONTYPE_AUTO_PISTOL);
   tolua_constant(tolua_S,"FPS_WEAPONTYPE_GRENADE",FPS_WEAPONTYPE_GRENADE);
   tolua_constant(tolua_S,"FPS_WEAPONTYPE_REVOLVER",FPS_WEAPONTYPE_REVOLVER);
   tolua_constant(tolua_S,"FPS_WEAPONTYPE_ITEM",FPS_WEAPONTYPE_ITEM);
   tolua_constant(tolua_S,"FPS_WEAPONTYPE_KNIFE",FPS_WEAPONTYPE_KNIFE);
   tolua_constant(tolua_S,"FPS_WEAPONTYPE_LMG",FPS_WEAPONTYPE_LMG);
   tolua_constant(tolua_S,"FPS_WEAPONTYPE_RIFLE_GRENADE",FPS_WEAPONTYPE_RIFLE_GRENADE);
   tolua_constant(tolua_S,"FPS_WEAPONTYPE_ATR",FPS_WEAPONTYPE_ATR);
   tolua_constant(tolua_S,"FPS_WEAPONTYPE_LTMORTAR",FPS_WEAPONTYPE_LTMORTAR);
   tolua_constant(tolua_S,"FPS_WEAPONTYPE_BUILD",FPS_WEAPONTYPE_BUILD);
   tolua_constant(tolua_S,"FPS_WEAPONTYPE_AUTO_RIFLE",FPS_WEAPONTYPE_AUTO_RIFLE);
   tolua_constant(tolua_S,"FPS_WEAPONTYPE_ROCKET",FPS_WEAPONTYPE_ROCKET);
   tolua_constant(tolua_S,"FPS_WEAPONTYPE_NUM_TYPES",FPS_WEAPONTYPE_NUM_TYPES);
   tolua_constant(tolua_S,"FPS_POSITION_STAND",FPS_POSITION_STAND);
   tolua_constant(tolua_S,"FPS_POSITION_CROUCH",FPS_POSITION_CROUCH);
   tolua_constant(tolua_S,"FPS_POSITION_PRONE",FPS_POSITION_PRONE);
   tolua_constant(tolua_S,"FPS_POSITION_SITTING",FPS_POSITION_SITTING);
   tolua_constant(tolua_S,"FPS_NUM_POSITIONS",FPS_NUM_POSITIONS);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_fp (lua_State* tolua_S) {
 return tolua_fp_open(tolua_S);
};
#endif


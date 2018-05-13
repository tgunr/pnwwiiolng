#include "syscfg.h" // Path is project specific.
/*
** Lua binding: strt
** Generated automatically by tolua++-1.0.92 on 01/04/14 22:00:09.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_strt_open (lua_State* tolua_S);

#include "sdm/StratDB.h"
#include "strt/strtDTL.h"
#include "strt/strtCPFlags.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"StratDB::StratObject");
 tolua_usertype(tolua_S,"FULL_OBJ_ID");
 tolua_usertype(tolua_S,"STRT_DTL_HEADER");
}

/* get function: numCPs of class  STRT_DTL_HEADER */
#ifndef TOLUA_DISABLE_tolua_get_STRT_DTL_HEADER_unsigned_numCPs
static int tolua_get_STRT_DTL_HEADER_unsigned_numCPs(lua_State* tolua_S)
{
  STRT_DTL_HEADER* self = (STRT_DTL_HEADER*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'numCPs'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->numCPs);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* set function: numCPs of class  STRT_DTL_HEADER */
#ifndef TOLUA_DISABLE_tolua_set_STRT_DTL_HEADER_unsigned_numCPs
static int tolua_set_STRT_DTL_HEADER_unsigned_numCPs(lua_State* tolua_S)
{
  STRT_DTL_HEADER* self = (STRT_DTL_HEADER*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  tolua_Error tolua_err;
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'numCPs'",NULL);
  if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->numCPs = (( unsigned short)  tolua_tonumber(tolua_S,2,0))
;
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: numFacilities of class  STRT_DTL_HEADER */
#ifndef TOLUA_DISABLE_tolua_get_STRT_DTL_HEADER_unsigned_numFacilities
static int tolua_get_STRT_DTL_HEADER_unsigned_numFacilities(lua_State* tolua_S)
{
  STRT_DTL_HEADER* self = (STRT_DTL_HEADER*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'numFacilities'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->numFacilities);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* set function: numFacilities of class  STRT_DTL_HEADER */
#ifndef TOLUA_DISABLE_tolua_set_STRT_DTL_HEADER_unsigned_numFacilities
static int tolua_set_STRT_DTL_HEADER_unsigned_numFacilities(lua_State* tolua_S)
{
  STRT_DTL_HEADER* self = (STRT_DTL_HEADER*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  tolua_Error tolua_err;
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'numFacilities'",NULL);
  if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->numFacilities = (( unsigned short)  tolua_tonumber(tolua_S,2,0))
;
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: numBuildings of class  STRT_DTL_HEADER */
#ifndef TOLUA_DISABLE_tolua_get_STRT_DTL_HEADER_unsigned_numBuildings
static int tolua_get_STRT_DTL_HEADER_unsigned_numBuildings(lua_State* tolua_S)
{
  STRT_DTL_HEADER* self = (STRT_DTL_HEADER*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'numBuildings'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->numBuildings);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* set function: numBuildings of class  STRT_DTL_HEADER */
#ifndef TOLUA_DISABLE_tolua_set_STRT_DTL_HEADER_unsigned_numBuildings
static int tolua_set_STRT_DTL_HEADER_unsigned_numBuildings(lua_State* tolua_S)
{
  STRT_DTL_HEADER* self = (STRT_DTL_HEADER*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  tolua_Error tolua_err;
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'numBuildings'",NULL);
  if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->numBuildings = (( unsigned short)  tolua_tonumber(tolua_S,2,0))
;
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: numLinks of class  STRT_DTL_HEADER */
#ifndef TOLUA_DISABLE_tolua_get_STRT_DTL_HEADER_unsigned_numLinks
static int tolua_get_STRT_DTL_HEADER_unsigned_numLinks(lua_State* tolua_S)
{
  STRT_DTL_HEADER* self = (STRT_DTL_HEADER*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'numLinks'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->numLinks);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* set function: numLinks of class  STRT_DTL_HEADER */
#ifndef TOLUA_DISABLE_tolua_set_STRT_DTL_HEADER_unsigned_numLinks
static int tolua_set_STRT_DTL_HEADER_unsigned_numLinks(lua_State* tolua_S)
{
  STRT_DTL_HEADER* self = (STRT_DTL_HEADER*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  tolua_Error tolua_err;
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'numLinks'",NULL);
  if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->numLinks = (( unsigned short)  tolua_tonumber(tolua_S,2,0))
;
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: numWaypoints of class  STRT_DTL_HEADER */
#ifndef TOLUA_DISABLE_tolua_get_STRT_DTL_HEADER_unsigned_numWaypoints
static int tolua_get_STRT_DTL_HEADER_unsigned_numWaypoints(lua_State* tolua_S)
{
  STRT_DTL_HEADER* self = (STRT_DTL_HEADER*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'numWaypoints'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->numWaypoints);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* set function: numWaypoints of class  STRT_DTL_HEADER */
#ifndef TOLUA_DISABLE_tolua_set_STRT_DTL_HEADER_unsigned_numWaypoints
static int tolua_set_STRT_DTL_HEADER_unsigned_numWaypoints(lua_State* tolua_S)
{
  STRT_DTL_HEADER* self = (STRT_DTL_HEADER*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  tolua_Error tolua_err;
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'numWaypoints'",NULL);
  if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->numWaypoints = (( unsigned short)  tolua_tonumber(tolua_S,2,0))
;
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: reserved1 of class  STRT_DTL_HEADER */
#ifndef TOLUA_DISABLE_tolua_get_STRT_DTL_HEADER_unsigned_reserved1
static int tolua_get_STRT_DTL_HEADER_unsigned_reserved1(lua_State* tolua_S)
{
  STRT_DTL_HEADER* self = (STRT_DTL_HEADER*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'reserved1'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->reserved1);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* set function: reserved1 of class  STRT_DTL_HEADER */
#ifndef TOLUA_DISABLE_tolua_set_STRT_DTL_HEADER_unsigned_reserved1
static int tolua_set_STRT_DTL_HEADER_unsigned_reserved1(lua_State* tolua_S)
{
  STRT_DTL_HEADER* self = (STRT_DTL_HEADER*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  tolua_Error tolua_err;
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'reserved1'",NULL);
  if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->reserved1 = (( unsigned short)  tolua_tonumber(tolua_S,2,0))
;
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: reserved2 of class  STRT_DTL_HEADER */
#ifndef TOLUA_DISABLE_tolua_get_STRT_DTL_HEADER_unsigned_reserved2
static int tolua_get_STRT_DTL_HEADER_unsigned_reserved2(lua_State* tolua_S)
{
  STRT_DTL_HEADER* self = (STRT_DTL_HEADER*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'reserved2'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->reserved2);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* set function: reserved2 of class  STRT_DTL_HEADER */
#ifndef TOLUA_DISABLE_tolua_set_STRT_DTL_HEADER_unsigned_reserved2
static int tolua_set_STRT_DTL_HEADER_unsigned_reserved2(lua_State* tolua_S)
{
  STRT_DTL_HEADER* self = (STRT_DTL_HEADER*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  tolua_Error tolua_err;
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'reserved2'",NULL);
  if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->reserved2 = (( unsigned short)  tolua_tonumber(tolua_S,2,0))
;
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: reserved3 of class  STRT_DTL_HEADER */
#ifndef TOLUA_DISABLE_tolua_get_STRT_DTL_HEADER_unsigned_reserved3
static int tolua_get_STRT_DTL_HEADER_unsigned_reserved3(lua_State* tolua_S)
{
  STRT_DTL_HEADER* self = (STRT_DTL_HEADER*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'reserved3'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->reserved3);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* set function: reserved3 of class  STRT_DTL_HEADER */
#ifndef TOLUA_DISABLE_tolua_set_STRT_DTL_HEADER_unsigned_reserved3
static int tolua_set_STRT_DTL_HEADER_unsigned_reserved3(lua_State* tolua_S)
{
  STRT_DTL_HEADER* self = (STRT_DTL_HEADER*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  tolua_Error tolua_err;
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'reserved3'",NULL);
  if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->reserved3 = (( unsigned short)  tolua_tonumber(tolua_S,2,0))
;
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* method: FullObjID of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_FullObjID00
static int tolua_strt_strat_StratDB_StratObject_FullObjID00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'FullObjID'",NULL);
#endif
  {
   const FULL_OBJ_ID* tolua_ret = (const FULL_OBJ_ID*)  self->FullObjID();
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"const FULL_OBJ_ID");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'FullObjID'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ParentStratObject of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_ParentStratObject00
static int tolua_strt_strat_StratDB_StratObject_ParentStratObject00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ParentStratObject'",NULL);
#endif
  {
   StratDB::StratObject* tolua_ret = (StratDB::StratObject*)  self->ParentStratObject();
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"StratDB::StratObject");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ParentStratObject'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SupercellX of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_SupercellX00
static int tolua_strt_strat_StratDB_StratObject_SupercellX00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SupercellX'",NULL);
#endif
  {
   int tolua_ret = (int)  self->SupercellX();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SupercellX'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SupercellY of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_SupercellY00
static int tolua_strt_strat_StratDB_StratObject_SupercellY00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SupercellY'",NULL);
#endif
  {
   int tolua_ret = (int)  self->SupercellY();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SupercellY'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Id of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_Id00
static int tolua_strt_strat_StratDB_StratObject_Id00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Id'",NULL);
#endif
  {
   int tolua_ret = (int)  self->Id();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Id'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: StratClass of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_StratClass00
static int tolua_strt_strat_StratDB_StratObject_StratClass00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'StratClass'",NULL);
#endif
  {
   int tolua_ret = (int)  self->StratClass();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'StratClass'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: StratType of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_StratType00
static int tolua_strt_strat_StratDB_StratObject_StratType00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'StratType'",NULL);
#endif
  {
   int tolua_ret = (int)  self->StratType();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'StratType'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: PosX of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_PosX00
static int tolua_strt_strat_StratDB_StratObject_PosX00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'PosX'",NULL);
#endif
  {
   int tolua_ret = (int)  self->PosX();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'PosX'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: PosY of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_PosY00
static int tolua_strt_strat_StratDB_StratObject_PosY00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'PosY'",NULL);
#endif
  {
   int tolua_ret = (int)  self->PosY();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'PosY'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: OriginalSide of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_OriginalSide00
static int tolua_strt_strat_StratDB_StratObject_OriginalSide00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'OriginalSide'",NULL);
#endif
  {
   int tolua_ret = (int)  self->OriginalSide();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'OriginalSide'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: OriginalCountry of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_OriginalCountry00
static int tolua_strt_strat_StratDB_StratObject_OriginalCountry00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'OriginalCountry'",NULL);
#endif
  {
   int tolua_ret = (int)  self->OriginalCountry();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'OriginalCountry'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SubType of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_SubType00
static int tolua_strt_strat_StratDB_StratObject_SubType00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SubType'",NULL);
#endif
  {
   int tolua_ret = (int)  self->SubType();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SubType'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Name of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_Name00
static int tolua_strt_strat_StratDB_StratObject_Name00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Name'",NULL);
#endif
  {
   const char* tolua_ret = (const char*)  self->Name();
   tolua_pushstring(tolua_S,(const char*)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Name'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Filename of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_Filename00
static int tolua_strt_strat_StratDB_StratObject_Filename00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Filename'",NULL);
#endif
  {
   const char* tolua_ret = (const char*)  self->Filename();
   tolua_pushstring(tolua_S,(const char*)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Filename'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Side of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_Side00
static int tolua_strt_strat_StratDB_StratObject_Side00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Side'",NULL);
#endif
  {
   int tolua_ret = (int)  self->Side();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Side'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Country of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_Country00
static int tolua_strt_strat_StratDB_StratObject_Country00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Country'",NULL);
#endif
  {
   int tolua_ret = (int)  self->Country();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Country'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: IsSpawnable of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_IsSpawnable00
static int tolua_strt_strat_StratDB_StratObject_IsSpawnable00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'IsSpawnable'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->IsSpawnable();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'IsSpawnable'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: IsDeepWaterPort of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_IsDeepWaterPort00
static int tolua_strt_strat_StratDB_StratObject_IsDeepWaterPort00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'IsDeepWaterPort'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->IsDeepWaterPort();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'IsDeepWaterPort'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetCountry of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_SetCountry00
static int tolua_strt_strat_StratDB_StratObject_SetCountry00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
  int country = ((int)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetCountry'",NULL);
#endif
  {
   self->SetCountry(country);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetCountry'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetUpdateTime of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_SetUpdateTime00
static int tolua_strt_strat_StratDB_StratObject_SetUpdateTime00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
  unsigned int t = ((unsigned int)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetUpdateTime'",NULL);
#endif
  {
   self->SetUpdateTime(t);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetUpdateTime'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetStratFlags of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_SetStratFlags00
static int tolua_strt_strat_StratDB_StratObject_SetStratFlags00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
  unsigned int flags = ((unsigned int)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetStratFlags'",NULL);
#endif
  {
   self->SetStratFlags(flags);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetStratFlags'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetVisLevel of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_SetVisLevel00
static int tolua_strt_strat_StratDB_StratObject_SetVisLevel00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
  int vislevel = ((int)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetVisLevel'",NULL);
#endif
  {
   self->SetVisLevel(vislevel);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetVisLevel'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: AddStratFlags of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_AddStratFlags00
static int tolua_strt_strat_StratDB_StratObject_AddStratFlags00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
  unsigned int flags = ((unsigned int)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'AddStratFlags'",NULL);
#endif
  {
   self->AddStratFlags(flags);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'AddStratFlags'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ClearStratFlags of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_ClearStratFlags00
static int tolua_strt_strat_StratDB_StratObject_ClearStratFlags00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
  unsigned int flags = ((unsigned int)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ClearStratFlags'",NULL);
#endif
  {
   self->ClearStratFlags(flags);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ClearStratFlags'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: AddUnit of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_AddUnit00
static int tolua_strt_strat_StratDB_StratObject_AddUnit00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
  unsigned int id = ((unsigned int)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'AddUnit'",NULL);
#endif
  {
   self->AddUnit(id);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'AddUnit'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: RemoveUnit of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_RemoveUnit00
static int tolua_strt_strat_StratDB_StratObject_RemoveUnit00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  StratDB::StratObject* self = (StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
  unsigned int id = ((unsigned int)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'RemoveUnit'",NULL);
#endif
  {
   self->RemoveUnit(id);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'RemoveUnit'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: IsUnitHere of class  StratDB::StratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_StratObject_IsUnitHere00
static int tolua_strt_strat_StratDB_StratObject_IsUnitHere00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const StratDB::StratObject",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const StratDB::StratObject* self = (const StratDB::StratObject*)  tolua_tousertype(tolua_S,1,0);
  unsigned int id = ((unsigned int)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'IsUnitHere'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->IsUnitHere(id);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'IsUnitHere'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: StratDB::FindStratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_FindStratObject00
static int tolua_strt_strat_StratDB_FindStratObject00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const FULL_OBJ_ID",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const FULL_OBJ_ID* id = ((const FULL_OBJ_ID*)  tolua_tousertype(tolua_S,1,0));
  {
   StratDB::StratObject* tolua_ret = (StratDB::StratObject*)  StratDB::FindStratObject(*id);
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"StratDB::StratObject");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'FindStratObject'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: StratDB::FindStratObject */
#ifndef TOLUA_DISABLE_tolua_strt_strat_StratDB_FindStratObject01
static int tolua_strt_strat_StratDB_FindStratObject01(lua_State* tolua_S)
{
 tolua_Error tolua_err;
 if (
     !tolua_isstring(tolua_S,1,0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
 {
  const char* cp = ((const char*)  tolua_tostring(tolua_S,1,0));
  const char* fac = ((const char*)  tolua_tostring(tolua_S,2,0));
  {
   StratDB::StratObject* tolua_ret = (StratDB::StratObject*)  StratDB::FindStratObject(cp,fac);
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"StratDB::StratObject");
  }
 }
 return 1;
tolua_lerror:
 return tolua_strt_strat_StratDB_FindStratObject00(tolua_S);
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_strt_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_module(tolua_S,"strat",0);
  tolua_beginmodule(tolua_S,"strat");
   tolua_constant(tolua_S,"MAX_STRT_NAME_LENGTH",MAX_STRT_NAME_LENGTH);
   tolua_constant(tolua_S,"MAX_OBJNAME_LENGTH",MAX_OBJNAME_LENGTH);
   tolua_constant(tolua_S,"GLOBAL_FLAG_NEW",GLOBAL_FLAG_NEW);
   tolua_constant(tolua_S,"GLOBAL_FLAG_CHANGED",GLOBAL_FLAG_CHANGED);
   tolua_constant(tolua_S,"LINK_FLAG_SETOPEN",LINK_FLAG_SETOPEN);
   tolua_constant(tolua_S,"LINK_FLAG_DISABLED",LINK_FLAG_DISABLED);
   tolua_constant(tolua_S,"LINK_FLAG_CONVOY",LINK_FLAG_CONVOY);
   tolua_constant(tolua_S,"CP_GENERIC",CP_GENERIC);
   tolua_constant(tolua_S,"CP_SMALL",CP_SMALL);
   tolua_constant(tolua_S,"CP_MEDIUM",CP_MEDIUM);
   tolua_constant(tolua_S,"CP_LARGE",CP_LARGE);
   tolua_constant(tolua_S,"CP_BRIDGE",CP_BRIDGE);
   tolua_constant(tolua_S,"MAX_CP_TYPES",MAX_CP_TYPES);
   tolua_constant(tolua_S,"FACILITY_CITY",FACILITY_CITY);
   tolua_constant(tolua_S,"FACILITY_FACTORY",FACILITY_FACTORY);
   tolua_constant(tolua_S,"FACILITY_BASE",FACILITY_BASE);
   tolua_constant(tolua_S,"FACILITY_DEPOT",FACILITY_DEPOT);
   tolua_constant(tolua_S,"FACILITY_OMT",FACILITY_OMT);
   tolua_constant(tolua_S,"FACILITY_SHIPYARD",FACILITY_SHIPYARD);
   tolua_constant(tolua_S,"FACILITY_FIREBASE",FACILITY_FIREBASE);
   tolua_constant(tolua_S,"FACILITY_AIRBASE",FACILITY_AIRBASE);
   tolua_constant(tolua_S,"FACILITY_ARMYBASE",FACILITY_ARMYBASE);
   tolua_constant(tolua_S,"FACILITY_NAVALBASE",FACILITY_NAVALBASE);
   tolua_constant(tolua_S,"MAX_FACILITY_TYPES",MAX_FACILITY_TYPES);
   tolua_constant(tolua_S,"FACTORY_GENERIC",FACTORY_GENERIC);
   tolua_constant(tolua_S,"FACTORY_MUNITIONS",FACTORY_MUNITIONS);
   tolua_constant(tolua_S,"FACTORY_AIRCRAFT",FACTORY_AIRCRAFT);
   tolua_constant(tolua_S,"FACTORY_OIL_REFINERY",FACTORY_OIL_REFINERY);
   tolua_constant(tolua_S,"FACTORY_CHEMICAL_PLANT",FACTORY_CHEMICAL_PLANT);
   tolua_constant(tolua_S,"FACTORY_MACHINERY",FACTORY_MACHINERY);
   tolua_constant(tolua_S,"FACTORY_VEHICLE",FACTORY_VEHICLE);
   tolua_constant(tolua_S,"MAX_FACTORY_SUBTYPES",MAX_FACTORY_SUBTYPES);
   tolua_constant(tolua_S,"DEPOT_GENERIC",DEPOT_GENERIC);
   tolua_constant(tolua_S,"DEPOT_TRAIN",DEPOT_TRAIN);
   tolua_constant(tolua_S,"DEPOT_TRUCK",DEPOT_TRUCK);
   tolua_constant(tolua_S,"DEPOT_BARGE",DEPOT_BARGE);
   tolua_constant(tolua_S,"DEPOT_CONVOY",DEPOT_CONVOY);
   tolua_constant(tolua_S,"DEPOT_PACK_ANIMAL",DEPOT_PACK_ANIMAL);
   tolua_constant(tolua_S,"DEPOT_WAGON",DEPOT_WAGON);
   tolua_constant(tolua_S,"DEPOT_PORTER",DEPOT_PORTER);
   tolua_constant(tolua_S,"MAX_DEPOT_SUBTYPES",MAX_DEPOT_SUBTYPES);
   tolua_constant(tolua_S,"OMT_GENERIC",OMT_GENERIC);
   tolua_constant(tolua_S,"OMT_RAIL",OMT_RAIL);
   tolua_constant(tolua_S,"OMT_ROAD",OMT_ROAD);
   tolua_constant(tolua_S,"OMT_SEA",OMT_SEA);
   tolua_constant(tolua_S,"MAX_OMT_SUBTYPES",MAX_OMT_SUBTYPES);
   tolua_constant(tolua_S,"BUILDING_GENERIC",BUILDING_GENERIC);
   tolua_constant(tolua_S,"BUILDING_RESEARCH",BUILDING_RESEARCH);
   tolua_constant(tolua_S,"BUILDING_SPAWN",BUILDING_SPAWN);
   tolua_constant(tolua_S,"BUILDING_AUTOGUNNER",BUILDING_AUTOGUNNER);
   tolua_constant(tolua_S,"BUILDING_CAPTURE",BUILDING_CAPTURE);
   tolua_constant(tolua_S,"BUILDING_BRIDGE",BUILDING_BRIDGE);
   tolua_constant(tolua_S,"MAX_BUILDING_TYPES",MAX_BUILDING_TYPES);
   tolua_constant(tolua_S,"AUTOGUNNER_MGNEST",AUTOGUNNER_MGNEST);
   tolua_constant(tolua_S,"AUTOGUNNER_RIFLEMAN",AUTOGUNNER_RIFLEMAN);
   tolua_constant(tolua_S,"AUTOGUNNER_RIFLEPIT",AUTOGUNNER_RIFLEPIT);
   tolua_constant(tolua_S,"AUTOGUNNER_MORTARPIT",AUTOGUNNER_MORTARPIT);
   tolua_constant(tolua_S,"AUTOGUNNER_ARTILLERYLIGHT",AUTOGUNNER_ARTILLERYLIGHT);
   tolua_constant(tolua_S,"AUTOGUNNER_ARTILLERYMEDIUM",AUTOGUNNER_ARTILLERYMEDIUM);
   tolua_constant(tolua_S,"AUTOGUNNER_ARTILLERYHEAVY",AUTOGUNNER_ARTILLERYHEAVY);
   tolua_constant(tolua_S,"AUTOGUNNER_COASTDEFENSE",AUTOGUNNER_COASTDEFENSE);
   tolua_constant(tolua_S,"AUTOGUNNER_FORTCUPOUT",AUTOGUNNER_FORTCUPOUT);
   tolua_constant(tolua_S,"AUTOGUNNER_AAALIGHT",AUTOGUNNER_AAALIGHT);
   tolua_constant(tolua_S,"AUTOGUNNER_AAAMEDIUM",AUTOGUNNER_AAAMEDIUM);
   tolua_constant(tolua_S,"AUTOGUNNER_AAAHEAVY",AUTOGUNNER_AAAHEAVY);
   tolua_constant(tolua_S,"AUTOGUNNER_TORPEDO",AUTOGUNNER_TORPEDO);
   tolua_constant(tolua_S,"AUTOGUNNER_ATLIGHT",AUTOGUNNER_ATLIGHT);
   tolua_constant(tolua_S,"AUTOGUNNER_ATMEDIUM",AUTOGUNNER_ATMEDIUM);
   tolua_constant(tolua_S,"AUTOGUNNER_ATHEAVY",AUTOGUNNER_ATHEAVY);
   tolua_constant(tolua_S,"MAX_AUTOGUNNER_SUBTYPES",MAX_AUTOGUNNER_SUBTYPES);
   tolua_constant(tolua_S,"STRAT_MODIFIED",STRAT_MODIFIED);
   tolua_constant(tolua_S,"STRAT_CONTROL",STRAT_CONTROL);
   tolua_constant(tolua_S,"MAX_STRAT_FLAGS",MAX_STRAT_FLAGS);
   tolua_cclass(tolua_S,"STRT_DTL_HEADER","STRT_DTL_HEADER","",NULL);
   tolua_beginmodule(tolua_S,"STRT_DTL_HEADER");
    tolua_variable(tolua_S,"numCPs",tolua_get_STRT_DTL_HEADER_unsigned_numCPs,tolua_set_STRT_DTL_HEADER_unsigned_numCPs);
    tolua_variable(tolua_S,"numFacilities",tolua_get_STRT_DTL_HEADER_unsigned_numFacilities,tolua_set_STRT_DTL_HEADER_unsigned_numFacilities);
    tolua_variable(tolua_S,"numBuildings",tolua_get_STRT_DTL_HEADER_unsigned_numBuildings,tolua_set_STRT_DTL_HEADER_unsigned_numBuildings);
    tolua_variable(tolua_S,"numLinks",tolua_get_STRT_DTL_HEADER_unsigned_numLinks,tolua_set_STRT_DTL_HEADER_unsigned_numLinks);
    tolua_variable(tolua_S,"numWaypoints",tolua_get_STRT_DTL_HEADER_unsigned_numWaypoints,tolua_set_STRT_DTL_HEADER_unsigned_numWaypoints);
    tolua_variable(tolua_S,"reserved1",tolua_get_STRT_DTL_HEADER_unsigned_reserved1,tolua_set_STRT_DTL_HEADER_unsigned_reserved1);
    tolua_variable(tolua_S,"reserved2",tolua_get_STRT_DTL_HEADER_unsigned_reserved2,tolua_set_STRT_DTL_HEADER_unsigned_reserved2);
    tolua_variable(tolua_S,"reserved3",tolua_get_STRT_DTL_HEADER_unsigned_reserved3,tolua_set_STRT_DTL_HEADER_unsigned_reserved3);
   tolua_endmodule(tolua_S);
   tolua_constant(tolua_S,"CP_STAT_NONE",CP_STAT_NONE);
   tolua_constant(tolua_S,"CP_STAT_OPEN",CP_STAT_OPEN);
   tolua_constant(tolua_S,"CP_STAT_IN_SUPPLY",CP_STAT_IN_SUPPLY);
   tolua_constant(tolua_S,"CP_STAT_HAS_NAVYBASE",CP_STAT_HAS_NAVYBASE);
   tolua_constant(tolua_S,"CP_STAT_SURRENDER_CANDIDATE",CP_STAT_SURRENDER_CANDIDATE);
   tolua_constant(tolua_S,"CP_STAT_HAS_AIRBASE",CP_STAT_HAS_AIRBASE);
   tolua_constant(tolua_S,"CP_STAT_CONTENTION",CP_STAT_CONTENTION);
   tolua_constant(tolua_S,"CP_STAT_ENEMY_ARMOR",CP_STAT_ENEMY_ARMOR);
   tolua_constant(tolua_S,"CP_STAT_ENEMY_INFANTRY",CP_STAT_ENEMY_INFANTRY);
   tolua_constant(tolua_S,"CP_STAT_ENEMY_SEACRAFT",CP_STAT_ENEMY_SEACRAFT);
   tolua_constant(tolua_S,"CP_STAT_ENEMY_AIRCRAFT",CP_STAT_ENEMY_AIRCRAFT);
   tolua_constant(tolua_S,"CP_STAT_HAS_RARE_INF",CP_STAT_HAS_RARE_INF);
   tolua_constant(tolua_S,"CP_STAT_HAS_RARE_GROUND",CP_STAT_HAS_RARE_GROUND);
   tolua_constant(tolua_S,"CP_STAT_HAS_RARE_NAVY",CP_STAT_HAS_RARE_NAVY);
   tolua_constant(tolua_S,"CP_STAT_HAS_RARE_AIR",CP_STAT_HAS_RARE_AIR);
   tolua_constant(tolua_S,"CP_STAT_ENEMY_LINKED",CP_STAT_ENEMY_LINKED);
   tolua_constant(tolua_S,"CP_STAT_OWN_HAAC",CP_STAT_OWN_HAAC);
   tolua_constant(tolua_S,"CP_STAT_FALLBACK",CP_STAT_FALLBACK);
   tolua_constant(tolua_S,"CP_STAT_OVERRUN",CP_STAT_OVERRUN);
   tolua_constant(tolua_S,"CP_STAT_ALLIED_ATTACK_OBJ",CP_STAT_ALLIED_ATTACK_OBJ);
   tolua_constant(tolua_S,"CP_STAT_ALLIED_DEFEND_OBJ",CP_STAT_ALLIED_DEFEND_OBJ);
   tolua_constant(tolua_S,"CP_STAT_AXIS_ATTACK_OBJ",CP_STAT_AXIS_ATTACK_OBJ);
   tolua_constant(tolua_S,"CP_STAT_AXIS_DEFEND_OBJ",CP_STAT_AXIS_DEFEND_OBJ);
   tolua_constant(tolua_S,"CP_STAT_HEAVY_ENEMY_ARMOR",CP_STAT_HEAVY_ENEMY_ARMOR);
   tolua_constant(tolua_S,"CP_STAT_HEAVY_ENEMY_INF",CP_STAT_HEAVY_ENEMY_INF);
   tolua_constant(tolua_S,"CP_STAT_HEAVY_ENEMY_SEA",CP_STAT_HEAVY_ENEMY_SEA);
   tolua_constant(tolua_S,"CP_STAT_HEAVY_ENEMY_AIR",CP_STAT_HEAVY_ENEMY_AIR);
   tolua_constant(tolua_S,"CP_STAT_MAKES_RDP",CP_STAT_MAKES_RDP);
   tolua_constant(tolua_S,"CP_STAT_RESERVED",CP_STAT_RESERVED);
   tolua_constant(tolua_S,"CP_NORM_EWS_FLAG_MASK",CP_NORM_EWS_FLAG_MASK);
   tolua_constant(tolua_S,"CP_HEAVY_EWS_FLAG_MASK",CP_HEAVY_EWS_FLAG_MASK);
   tolua_constant(tolua_S,"CP_EWS_FLAG_MASK",CP_EWS_FLAG_MASK);
   tolua_constant(tolua_S,"CP_ALLIED_OBJECTIVES",CP_ALLIED_OBJECTIVES);
   tolua_constant(tolua_S,"CP_AXIS_OBJECTIVES",CP_AXIS_OBJECTIVES);
   tolua_constant(tolua_S,"CP_OBJECTIVES",CP_OBJECTIVES);
   tolua_constant(tolua_S,"BRIDGE_STAT_MASK",BRIDGE_STAT_MASK);
   tolua_module(tolua_S,"StratDB",0);
   tolua_beginmodule(tolua_S,"StratDB");
    tolua_cclass(tolua_S,"StratObject","StratDB::StratObject","",NULL);
    tolua_beginmodule(tolua_S,"StratObject");
     tolua_function(tolua_S,"FullObjID",tolua_strt_strat_StratDB_StratObject_FullObjID00);
     tolua_function(tolua_S,"ParentStratObject",tolua_strt_strat_StratDB_StratObject_ParentStratObject00);
     tolua_function(tolua_S,"SupercellX",tolua_strt_strat_StratDB_StratObject_SupercellX00);
     tolua_function(tolua_S,"SupercellY",tolua_strt_strat_StratDB_StratObject_SupercellY00);
     tolua_function(tolua_S,"Id",tolua_strt_strat_StratDB_StratObject_Id00);
     tolua_function(tolua_S,"StratClass",tolua_strt_strat_StratDB_StratObject_StratClass00);
     tolua_function(tolua_S,"StratType",tolua_strt_strat_StratDB_StratObject_StratType00);
     tolua_function(tolua_S,"PosX",tolua_strt_strat_StratDB_StratObject_PosX00);
     tolua_function(tolua_S,"PosY",tolua_strt_strat_StratDB_StratObject_PosY00);
     tolua_function(tolua_S,"OriginalSide",tolua_strt_strat_StratDB_StratObject_OriginalSide00);
     tolua_function(tolua_S,"OriginalCountry",tolua_strt_strat_StratDB_StratObject_OriginalCountry00);
     tolua_function(tolua_S,"SubType",tolua_strt_strat_StratDB_StratObject_SubType00);
     tolua_function(tolua_S,"Name",tolua_strt_strat_StratDB_StratObject_Name00);
     tolua_function(tolua_S,"Filename",tolua_strt_strat_StratDB_StratObject_Filename00);
     tolua_function(tolua_S,"Side",tolua_strt_strat_StratDB_StratObject_Side00);
     tolua_function(tolua_S,"Country",tolua_strt_strat_StratDB_StratObject_Country00);
     tolua_function(tolua_S,"IsSpawnable",tolua_strt_strat_StratDB_StratObject_IsSpawnable00);
     tolua_function(tolua_S,"IsDeepWaterPort",tolua_strt_strat_StratDB_StratObject_IsDeepWaterPort00);
     tolua_function(tolua_S,"SetCountry",tolua_strt_strat_StratDB_StratObject_SetCountry00);
     tolua_function(tolua_S,"SetUpdateTime",tolua_strt_strat_StratDB_StratObject_SetUpdateTime00);
     tolua_function(tolua_S,"SetStratFlags",tolua_strt_strat_StratDB_StratObject_SetStratFlags00);
     tolua_function(tolua_S,"SetVisLevel",tolua_strt_strat_StratDB_StratObject_SetVisLevel00);
     tolua_function(tolua_S,"AddStratFlags",tolua_strt_strat_StratDB_StratObject_AddStratFlags00);
     tolua_function(tolua_S,"ClearStratFlags",tolua_strt_strat_StratDB_StratObject_ClearStratFlags00);
     tolua_function(tolua_S,"AddUnit",tolua_strt_strat_StratDB_StratObject_AddUnit00);
     tolua_function(tolua_S,"RemoveUnit",tolua_strt_strat_StratDB_StratObject_RemoveUnit00);
     tolua_function(tolua_S,"IsUnitHere",tolua_strt_strat_StratDB_StratObject_IsUnitHere00);
    tolua_endmodule(tolua_S);
    tolua_function(tolua_S,"FindStratObject",tolua_strt_strat_StratDB_FindStratObject00);
    tolua_function(tolua_S,"FindStratObject",tolua_strt_strat_StratDB_FindStratObject01);
   tolua_endmodule(tolua_S);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_strt (lua_State* tolua_S) {
 return tolua_strt_open(tolua_S);
};
#endif


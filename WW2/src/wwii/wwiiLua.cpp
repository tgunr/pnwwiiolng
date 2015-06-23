#include <syscfg.h> // Path is project specific.
/*
** Lua binding: wwii
** Generated automatically by tolua++-1.0.92 on 01/04/14 22:00:09.
*/

#ifndef __cplusplus
#include <stdlib.h>
#endif
#include <string.h>

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_wwii_open (lua_State* tolua_S);

#include "include/systypes.h"
#include "wwii/wwiiHCUnits.h"
#include "wwii/wwiiFullObjID.h"
#include "wwii/wwiiOwnershipEnums.h"
#include "mm/mmEnums.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"RANK_NO");
 tolua_usertype(tolua_S,"BRANCH_ENUM");
 tolua_usertype(tolua_S,"ChatHandle");
 tolua_usertype(tolua_S,"HCUnit");
 tolua_usertype(tolua_S,"OCTET_CELL");
 tolua_usertype(tolua_S,"FULL_OBJ_ID");
}

/* get function: ox of class  OCTET_CELL */
#ifndef TOLUA_DISABLE_tolua_get_OCTET_CELL_ox
static int tolua_get_OCTET_CELL_ox(lua_State* tolua_S)
{
  OCTET_CELL* self = (OCTET_CELL*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'ox'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->ox);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* set function: ox of class  OCTET_CELL */
#ifndef TOLUA_DISABLE_tolua_set_OCTET_CELL_ox
static int tolua_set_OCTET_CELL_ox(lua_State* tolua_S)
{
  OCTET_CELL* self = (OCTET_CELL*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  tolua_Error tolua_err;
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'ox'",NULL);
  if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->ox = ((  short)  tolua_tonumber(tolua_S,2,0))
;
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: oy of class  OCTET_CELL */
#ifndef TOLUA_DISABLE_tolua_get_OCTET_CELL_oy
static int tolua_get_OCTET_CELL_oy(lua_State* tolua_S)
{
  OCTET_CELL* self = (OCTET_CELL*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'oy'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->oy);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* set function: oy of class  OCTET_CELL */
#ifndef TOLUA_DISABLE_tolua_set_OCTET_CELL_oy
static int tolua_set_OCTET_CELL_oy(lua_State* tolua_S)
{
  OCTET_CELL* self = (OCTET_CELL*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  tolua_Error tolua_err;
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'oy'",NULL);
  if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->oy = ((  short)  tolua_tonumber(tolua_S,2,0))
;
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: cell of class  FULL_OBJ_ID */
#ifndef TOLUA_DISABLE_tolua_get_FULL_OBJ_ID_cell
static int tolua_get_FULL_OBJ_ID_cell(lua_State* tolua_S)
{
  FULL_OBJ_ID* self = (FULL_OBJ_ID*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'cell'",NULL);
#endif
  tolua_pushusertype(tolua_S,(void*)&self->cell,"OCTET_CELL");
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* set function: cell of class  FULL_OBJ_ID */
#ifndef TOLUA_DISABLE_tolua_set_FULL_OBJ_ID_cell
static int tolua_set_FULL_OBJ_ID_cell(lua_State* tolua_S)
{
  FULL_OBJ_ID* self = (FULL_OBJ_ID*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  tolua_Error tolua_err;
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'cell'",NULL);
  if (!tolua_isusertype(tolua_S,2,"OCTET_CELL",0,&tolua_err))
   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->cell = *((OCTET_CELL*)  tolua_tousertype(tolua_S,2,0))
;
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: id of class  FULL_OBJ_ID */
#ifndef TOLUA_DISABLE_tolua_get_FULL_OBJ_ID_unsigned_id
static int tolua_get_FULL_OBJ_ID_unsigned_id(lua_State* tolua_S)
{
  FULL_OBJ_ID* self = (FULL_OBJ_ID*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'id'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->id);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* set function: id of class  FULL_OBJ_ID */
#ifndef TOLUA_DISABLE_tolua_set_FULL_OBJ_ID_unsigned_id
static int tolua_set_FULL_OBJ_ID_unsigned_id(lua_State* tolua_S)
{
  FULL_OBJ_ID* self = (FULL_OBJ_ID*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  tolua_Error tolua_err;
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'id'",NULL);
  if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->id = (( unsigned short)  tolua_tonumber(tolua_S,2,0))
;
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: objClass of class  FULL_OBJ_ID */
#ifndef TOLUA_DISABLE_tolua_get_FULL_OBJ_ID_objClass
static int tolua_get_FULL_OBJ_ID_objClass(lua_State* tolua_S)
{
  FULL_OBJ_ID* self = (FULL_OBJ_ID*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'objClass'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->objClass);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* set function: objClass of class  FULL_OBJ_ID */
#ifndef TOLUA_DISABLE_tolua_set_FULL_OBJ_ID_objClass
static int tolua_set_FULL_OBJ_ID_objClass(lua_State* tolua_S)
{
  FULL_OBJ_ID* self = (FULL_OBJ_ID*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  tolua_Error tolua_err;
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'objClass'",NULL);
  if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->objClass = ((  char)  tolua_tonumber(tolua_S,2,0))
;
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* get function: objType of class  FULL_OBJ_ID */
#ifndef TOLUA_DISABLE_tolua_get_FULL_OBJ_ID_objType
static int tolua_get_FULL_OBJ_ID_objType(lua_State* tolua_S)
{
  FULL_OBJ_ID* self = (FULL_OBJ_ID*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'objType'",NULL);
#endif
  tolua_pushnumber(tolua_S,(lua_Number)self->objType);
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* set function: objType of class  FULL_OBJ_ID */
#ifndef TOLUA_DISABLE_tolua_set_FULL_OBJ_ID_objType
static int tolua_set_FULL_OBJ_ID_objType(lua_State* tolua_S)
{
  FULL_OBJ_ID* self = (FULL_OBJ_ID*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  tolua_Error tolua_err;
  if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'objType'",NULL);
  if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->objType = ((  char)  tolua_tonumber(tolua_S,2,0))
;
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* method: sameCell of class  FULL_OBJ_ID */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_FULL_OBJ_ID_sameCell00
static int tolua_wwii_wwii_FULL_OBJ_ID_sameCell00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const FULL_OBJ_ID",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const FULL_OBJ_ID* self = (const FULL_OBJ_ID*)  tolua_tousertype(tolua_S,1,0);
   short x = ((  short)  tolua_tonumber(tolua_S,2,0));
   short y = ((  short)  tolua_tonumber(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'sameCell'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->sameCell(x,y);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'sameCell'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: sameCell of class  FULL_OBJ_ID */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_FULL_OBJ_ID_sameCell01
static int tolua_wwii_wwii_FULL_OBJ_ID_sameCell01(lua_State* tolua_S)
{
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const FULL_OBJ_ID",0,&tolua_err) ||
     !tolua_isusertype(tolua_S,2,"const OCTET_CELL",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
 {
  const FULL_OBJ_ID* self = (const FULL_OBJ_ID*)  tolua_tousertype(tolua_S,1,0);
  const OCTET_CELL* rhs = ((const OCTET_CELL*)  tolua_tousertype(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'sameCell'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->sameCell(*rhs);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
tolua_lerror:
 return tolua_wwii_wwii_FULL_OBJ_ID_sameCell00(tolua_S);
}
#endif //#ifndef TOLUA_DISABLE

/* method: sameCell of class  FULL_OBJ_ID */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_FULL_OBJ_ID_sameCell02
static int tolua_wwii_wwii_FULL_OBJ_ID_sameCell02(lua_State* tolua_S)
{
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const FULL_OBJ_ID",0,&tolua_err) ||
     !tolua_isusertype(tolua_S,2,"const FULL_OBJ_ID",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
 {
  const FULL_OBJ_ID* self = (const FULL_OBJ_ID*)  tolua_tousertype(tolua_S,1,0);
  const FULL_OBJ_ID* rhs = ((const FULL_OBJ_ID*)  tolua_tousertype(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'sameCell'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->sameCell(*rhs);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
tolua_lerror:
 return tolua_wwii_wwii_FULL_OBJ_ID_sameCell01(tolua_S);
}
#endif //#ifndef TOLUA_DISABLE

/* method: operator== of class  FULL_OBJ_ID */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_FULL_OBJ_ID__eq00
static int tolua_wwii_wwii_FULL_OBJ_ID__eq00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const FULL_OBJ_ID",0,&tolua_err) ||
     !tolua_isusertype(tolua_S,2,"const FULL_OBJ_ID",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const FULL_OBJ_ID* self = (const FULL_OBJ_ID*)  tolua_tousertype(tolua_S,1,0);
  const FULL_OBJ_ID* rhs = ((const FULL_OBJ_ID*)  tolua_tousertype(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'operator=='",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->operator==(*rhs);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function '.eq'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Key of class  FULL_OBJ_ID */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_FULL_OBJ_ID_Key00
static int tolua_wwii_wwii_FULL_OBJ_ID_Key00(lua_State* tolua_S)
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
  const FULL_OBJ_ID* self = (const FULL_OBJ_ID*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Key'",NULL);
#endif
  {
   unsigned long tolua_ret = ( unsigned long)  self->Key();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Key'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* get function: nullObjID */
#ifndef TOLUA_DISABLE_tolua_get_wwii_nullObjID
static int tolua_get_wwii_nullObjID(lua_State* tolua_S)
{
  tolua_pushusertype(tolua_S,(void*)&nullObjID,"const FULL_OBJ_ID");
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* function: weapPrintFullObjIDC */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_weapPrintFullObjIDC00
static int tolua_wwii_wwii_weapPrintFullObjIDC00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"const FULL_OBJ_ID",0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  const FULL_OBJ_ID* id = ((const FULL_OBJ_ID*)  tolua_tousertype(tolua_S,1,0));
  char* buffer = ((char*)  tolua_tostring(tolua_S,2,0));
  {
   const char* tolua_ret = (const char*)  weapPrintFullObjIDC(id,buffer);
   tolua_pushstring(tolua_S,(const char*)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'weapPrintFullObjIDC'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

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

/* method: setHomeCPID of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_setHomeCPID00
static int tolua_wwii_wwii_HCUnit_setHomeCPID00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isusertype(tolua_S,2,"const FULL_OBJ_ID",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
  const FULL_OBJ_ID* newHomeCPID = ((const FULL_OBJ_ID*)  tolua_tousertype(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'setHomeCPID'",NULL);
#endif
  {
   self->setHomeCPID(*newHomeCPID);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'setHomeCPID'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: unitID of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_unitID00
static int tolua_wwii_wwii_HCUnit_unitID00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'unitID'",NULL);
#endif
  {
   unsigned int tolua_ret = ( unsigned int)  self->unitID();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'unitID'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: parentID of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_parentID00
static int tolua_wwii_wwii_HCUnit_parentID00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'parentID'",NULL);
#endif
  {
   unsigned int tolua_ret = ( unsigned int)  self->parentID();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'parentID'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: parent of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_parent00
static int tolua_wwii_wwii_HCUnit_parent00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'parent'",NULL);
#endif
  {
   HCUnit* tolua_ret = (HCUnit*)  self->parent();
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"HCUnit");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'parent'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: country of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_country00
static int tolua_wwii_wwii_HCUnit_country00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'country'",NULL);
#endif
  {
   WWII_COUNTRY tolua_ret = (WWII_COUNTRY)  self->country();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'country'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: side of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_side00
static int tolua_wwii_wwii_HCUnit_side00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'side'",NULL);
#endif
  {
   WWII_SIDE tolua_ret = (WWII_SIDE)  self->side();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'side'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: branch of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_branch00
static int tolua_wwii_wwii_HCUnit_branch00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'branch'",NULL);
#endif
  {
   BRANCH_ENUM tolua_ret = (BRANCH_ENUM)  self->branch();
   {
#ifdef __cplusplus
    void* tolua_obj = new BRANCH_ENUM(tolua_ret);
    tolua_pushusertype_and_takeownership(tolua_S,tolua_obj,"BRANCH_ENUM");
#else
    void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(BRANCH_ENUM));
    tolua_pushusertype_and_takeownership(tolua_S,tolua_obj,"BRANCH_ENUM");
#endif
   }
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'branch'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: homeCPID of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_homeCPID00
static int tolua_wwii_wwii_HCUnit_homeCPID00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'homeCPID'",NULL);
#endif
  {
   const FULL_OBJ_ID& tolua_ret = (const FULL_OBJ_ID&)  self->homeCPID();
   tolua_pushusertype(tolua_S,(void*)&tolua_ret,"const FULL_OBJ_ID");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'homeCPID'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: movementType of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_movementType00
static int tolua_wwii_wwii_HCUnit_movementType00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'movementType'",NULL);
#endif
  {
   BRANCH_ENUM tolua_ret = (BRANCH_ENUM)  self->movementType();
   {
#ifdef __cplusplus
    void* tolua_obj = new BRANCH_ENUM(tolua_ret);
    tolua_pushusertype_and_takeownership(tolua_S,tolua_obj,"BRANCH_ENUM");
#else
    void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(BRANCH_ENUM));
    tolua_pushusertype_and_takeownership(tolua_S,tolua_obj,"BRANCH_ENUM");
#endif
   }
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'movementType'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: name of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_name00
static int tolua_wwii_wwii_HCUnit_name00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'name'",NULL);
#endif
  {
   const char* tolua_ret = (const char*)  self->name();
   tolua_pushstring(tolua_S,(const char*)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'name'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: owner of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_owner00
static int tolua_wwii_wwii_HCUnit_owner00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'owner'",NULL);
#endif
  {
   const ChatHandle& tolua_ret = (const ChatHandle&)  self->owner();
   tolua_pushusertype(tolua_S,(void*)&tolua_ret,"const ChatHandle");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'owner'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ownerRank of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_ownerRank00
static int tolua_wwii_wwii_HCUnit_ownerRank00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ownerRank'",NULL);
#endif
  {
   RANK_NO tolua_ret = (RANK_NO)  self->ownerRank();
   {
#ifdef __cplusplus
    void* tolua_obj = new RANK_NO(tolua_ret);
    tolua_pushusertype_and_takeownership(tolua_S,tolua_obj,"RANK_NO");
#else
    void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(RANK_NO));
    tolua_pushusertype_and_takeownership(tolua_S,tolua_obj,"RANK_NO");
#endif
   }
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ownerRank'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: rank of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_rank00
static int tolua_wwii_wwii_HCUnit_rank00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'rank'",NULL);
#endif
  {
   unsigned char tolua_ret = ( unsigned char)  self->rank();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'rank'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: type of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_type00
static int tolua_wwii_wwii_HCUnit_type00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'type'",NULL);
#endif
  {
   HCUNIT_TYPE tolua_ret = (HCUNIT_TYPE)  self->type();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'type'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: unitClass of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_unitClass00
static int tolua_wwii_wwii_HCUnit_unitClass00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'unitClass'",NULL);
#endif
  {
   unsigned char tolua_ret = ( unsigned char)  self->unitClass();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'unitClass'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: handle of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_handle00
static int tolua_wwii_wwii_HCUnit_handle00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'handle'",NULL);
#endif
  {
   const ChatHandle& tolua_ret = (const ChatHandle&)  self->handle();
   tolua_pushusertype(tolua_S,(void*)&tolua_ret,"const ChatHandle");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'handle'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: callsign of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_callsign00
static int tolua_wwii_wwii_HCUnit_callsign00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'callsign'",NULL);
#endif
  {
   const char* tolua_ret = (const char*)  self->callsign();
   tolua_pushstring(tolua_S,(const char*)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'callsign'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: shortName of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_shortName00
static int tolua_wwii_wwii_HCUnit_shortName00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'shortName'",NULL);
#endif
  {
   const char* tolua_ret = (const char*)  self->shortName();
   tolua_pushstring(tolua_S,(const char*)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'shortName'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: isAbove of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_isAbove00
static int tolua_wwii_wwii_HCUnit_isAbove00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isusertype(tolua_S,2,"const HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
  const HCUnit* rhs = ((const HCUnit*)  tolua_tousertype(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'isAbove'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->isAbove(rhs);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'isAbove'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: isAbove of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_isAbove01
static int tolua_wwii_wwii_HCUnit_isAbove01(lua_State* tolua_S)
{
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
  unsigned short rhs = (( unsigned short)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'isAbove'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->isAbove(rhs);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
tolua_lerror:
 return tolua_wwii_wwii_HCUnit_isAbove00(tolua_S);
}
#endif //#ifndef TOLUA_DISABLE

/* method: isBelow of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_isBelow00
static int tolua_wwii_wwii_HCUnit_isBelow00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
  unsigned short rhs = (( unsigned short)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'isBelow'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->isBelow(rhs);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'isBelow'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: isBelow of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_isBelow01
static int tolua_wwii_wwii_HCUnit_isBelow01(lua_State* tolua_S)
{
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isusertype(tolua_S,2,"const HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
  const HCUnit* rhs = ((const HCUnit*)  tolua_tousertype(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'isBelow'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->isBelow(rhs);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
tolua_lerror:
 return tolua_wwii_wwii_HCUnit_isBelow00(tolua_S);
}
#endif //#ifndef TOLUA_DISABLE

/* method: isBelowOrEqual of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_isBelowOrEqual00
static int tolua_wwii_wwii_HCUnit_isBelowOrEqual00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
  unsigned short rhs = (( unsigned short)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'isBelowOrEqual'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->isBelowOrEqual(rhs);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'isBelowOrEqual'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: isBelowOrEqual of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_isBelowOrEqual01
static int tolua_wwii_wwii_HCUnit_isBelowOrEqual01(lua_State* tolua_S)
{
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isusertype(tolua_S,2,"const HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
  const HCUnit* rhs = ((const HCUnit*)  tolua_tousertype(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'isBelowOrEqual'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->isBelowOrEqual(rhs);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
tolua_lerror:
 return tolua_wwii_wwii_HCUnit_isBelowOrEqual00(tolua_S);
}
#endif //#ifndef TOLUA_DISABLE

/* method: isBrigade of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_isBrigade00
static int tolua_wwii_wwii_HCUnit_isBrigade00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'isBrigade'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->isBrigade();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'isBrigade'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: isDivision of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_isDivision00
static int tolua_wwii_wwii_HCUnit_isDivision00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'isDivision'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->isDivision();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'isDivision'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: isMarineUnit of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_isMarineUnit00
static int tolua_wwii_wwii_HCUnit_isMarineUnit00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'isMarineUnit'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->isMarineUnit();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'isMarineUnit'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: isParatroopUnit of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_isParatroopUnit00
static int tolua_wwii_wwii_HCUnit_isParatroopUnit00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'isParatroopUnit'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->isParatroopUnit();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'isParatroopUnit'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: isOwner of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_isOwner00
static int tolua_wwii_wwii_HCUnit_isOwner00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isusertype(tolua_S,2,"const ChatHandle",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  HCUnit* self = (HCUnit*)  tolua_tousertype(tolua_S,1,0);
  const ChatHandle* candidate = ((const ChatHandle*)  tolua_tousertype(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'isOwner'",NULL);
#endif
  {
   bool tolua_ret = (bool)  self->isOwner(*candidate);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'isOwner'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Lookup of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_Lookup00
static int tolua_wwii_wwii_HCUnit_Lookup00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertable(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  unsigned int unitID = (( unsigned int)  tolua_tonumber(tolua_S,2,0));
  {
   HCUnit* tolua_ret = (HCUnit*)  HCUnit::Lookup(unitID);
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"HCUnit");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Lookup'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Lookup of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_Lookup01
static int tolua_wwii_wwii_HCUnit_Lookup01(lua_State* tolua_S)
{
 tolua_Error tolua_err;
 if (
     !tolua_isusertable(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isusertype(tolua_S,2,"const ChatHandle",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
 {
  const ChatHandle* handle = ((const ChatHandle*)  tolua_tousertype(tolua_S,2,0));
  {
   HCUnit* tolua_ret = (HCUnit*)  HCUnit::Lookup(*handle);
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"HCUnit");
  }
 }
 return 1;
tolua_lerror:
 return tolua_wwii_wwii_HCUnit_Lookup00(tolua_S);
}
#endif //#ifndef TOLUA_DISABLE

/* method: Lookup of class  HCUnit */
#ifndef TOLUA_DISABLE_tolua_wwii_wwii_HCUnit_Lookup02
static int tolua_wwii_wwii_HCUnit_Lookup02(lua_State* tolua_S)
{
 tolua_Error tolua_err;
 if (
     !tolua_isusertable(tolua_S,1,"HCUnit",0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
 {
  const char* callsign = ((const char*)  tolua_tostring(tolua_S,2,0));
  {
   HCUnit* tolua_ret = (HCUnit*)  HCUnit::Lookup(callsign);
   tolua_pushusertype(tolua_S,(void*)tolua_ret,"HCUnit");
  }
 }
 return 1;
tolua_lerror:
 return tolua_wwii_wwii_HCUnit_Lookup01(tolua_S);
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_wwii_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_module(tolua_S,"wwii",1);
  tolua_beginmodule(tolua_S,"wwii");
   tolua_constant(tolua_S,"CLASS_CP",CLASS_CP);
   tolua_constant(tolua_S,"CLASS_FACILITY",CLASS_FACILITY);
   tolua_constant(tolua_S,"CLASS_BUILDING",CLASS_BUILDING);
   tolua_constant(tolua_S,"CLASS_LINK",CLASS_LINK);
   tolua_constant(tolua_S,"CLASS_WAYPOINT",CLASS_WAYPOINT);
   tolua_constant(tolua_S,"MAX_STRAT_CLASSES",MAX_STRAT_CLASSES);
   tolua_cclass(tolua_S,"OCTET_CELL","OCTET_CELL","",NULL);
   tolua_beginmodule(tolua_S,"OCTET_CELL");
    tolua_variable(tolua_S,"ox",tolua_get_OCTET_CELL_ox,tolua_set_OCTET_CELL_ox);
    tolua_variable(tolua_S,"oy",tolua_get_OCTET_CELL_oy,tolua_set_OCTET_CELL_oy);
   tolua_endmodule(tolua_S);
   tolua_cclass(tolua_S,"FULL_OBJ_ID","FULL_OBJ_ID","",NULL);
   tolua_beginmodule(tolua_S,"FULL_OBJ_ID");
    tolua_variable(tolua_S,"cell",tolua_get_FULL_OBJ_ID_cell,tolua_set_FULL_OBJ_ID_cell);
    tolua_variable(tolua_S,"id",tolua_get_FULL_OBJ_ID_unsigned_id,tolua_set_FULL_OBJ_ID_unsigned_id);
    tolua_variable(tolua_S,"objClass",tolua_get_FULL_OBJ_ID_objClass,tolua_set_FULL_OBJ_ID_objClass);
    tolua_variable(tolua_S,"objType",tolua_get_FULL_OBJ_ID_objType,tolua_set_FULL_OBJ_ID_objType);
    tolua_function(tolua_S,"sameCell",tolua_wwii_wwii_FULL_OBJ_ID_sameCell00);
    tolua_function(tolua_S,"sameCell",tolua_wwii_wwii_FULL_OBJ_ID_sameCell01);
    tolua_function(tolua_S,"sameCell",tolua_wwii_wwii_FULL_OBJ_ID_sameCell02);
    tolua_function(tolua_S,".eq",tolua_wwii_wwii_FULL_OBJ_ID__eq00);
    tolua_function(tolua_S,"Key",tolua_wwii_wwii_FULL_OBJ_ID_Key00);
   tolua_endmodule(tolua_S);
   tolua_variable(tolua_S,"nullObjID",tolua_get_wwii_nullObjID,NULL);
   tolua_function(tolua_S,"weapPrintFullObjIDC",tolua_wwii_wwii_weapPrintFullObjIDC00);
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
   tolua_constant(tolua_S,"HCU_GENERIC",HCU_GENERIC);
   tolua_constant(tolua_S,"HCU_BRIGADE",HCU_BRIGADE);
   tolua_constant(tolua_S,"HCU_DIVISION",HCU_DIVISION);
   tolua_constant(tolua_S,"MAX_HCUNIT_TYPES",MAX_HCUNIT_TYPES);
   tolua_cclass(tolua_S,"HCUnit","HCUnit","",NULL);
   tolua_beginmodule(tolua_S,"HCUnit");
    tolua_function(tolua_S,"setHomeCPID",tolua_wwii_wwii_HCUnit_setHomeCPID00);
    tolua_function(tolua_S,"unitID",tolua_wwii_wwii_HCUnit_unitID00);
    tolua_function(tolua_S,"parentID",tolua_wwii_wwii_HCUnit_parentID00);
    tolua_function(tolua_S,"parent",tolua_wwii_wwii_HCUnit_parent00);
    tolua_function(tolua_S,"country",tolua_wwii_wwii_HCUnit_country00);
    tolua_function(tolua_S,"side",tolua_wwii_wwii_HCUnit_side00);
    tolua_function(tolua_S,"branch",tolua_wwii_wwii_HCUnit_branch00);
    tolua_function(tolua_S,"homeCPID",tolua_wwii_wwii_HCUnit_homeCPID00);
    tolua_function(tolua_S,"movementType",tolua_wwii_wwii_HCUnit_movementType00);
    tolua_function(tolua_S,"name",tolua_wwii_wwii_HCUnit_name00);
    tolua_function(tolua_S,"owner",tolua_wwii_wwii_HCUnit_owner00);
    tolua_function(tolua_S,"ownerRank",tolua_wwii_wwii_HCUnit_ownerRank00);
    tolua_function(tolua_S,"rank",tolua_wwii_wwii_HCUnit_rank00);
    tolua_function(tolua_S,"type",tolua_wwii_wwii_HCUnit_type00);
    tolua_function(tolua_S,"unitClass",tolua_wwii_wwii_HCUnit_unitClass00);
    tolua_function(tolua_S,"handle",tolua_wwii_wwii_HCUnit_handle00);
    tolua_function(tolua_S,"callsign",tolua_wwii_wwii_HCUnit_callsign00);
    tolua_function(tolua_S,"shortName",tolua_wwii_wwii_HCUnit_shortName00);
    tolua_function(tolua_S,"isAbove",tolua_wwii_wwii_HCUnit_isAbove00);
    tolua_function(tolua_S,"isAbove",tolua_wwii_wwii_HCUnit_isAbove01);
    tolua_function(tolua_S,"isBelow",tolua_wwii_wwii_HCUnit_isBelow00);
    tolua_function(tolua_S,"isBelow",tolua_wwii_wwii_HCUnit_isBelow01);
    tolua_function(tolua_S,"isBelowOrEqual",tolua_wwii_wwii_HCUnit_isBelowOrEqual00);
    tolua_function(tolua_S,"isBelowOrEqual",tolua_wwii_wwii_HCUnit_isBelowOrEqual01);
    tolua_function(tolua_S,"isBrigade",tolua_wwii_wwii_HCUnit_isBrigade00);
    tolua_function(tolua_S,"isDivision",tolua_wwii_wwii_HCUnit_isDivision00);
    tolua_function(tolua_S,"isMarineUnit",tolua_wwii_wwii_HCUnit_isMarineUnit00);
    tolua_function(tolua_S,"isParatroopUnit",tolua_wwii_wwii_HCUnit_isParatroopUnit00);
    tolua_function(tolua_S,"isOwner",tolua_wwii_wwii_HCUnit_isOwner00);
    tolua_function(tolua_S,"Lookup",tolua_wwii_wwii_HCUnit_Lookup00);
    tolua_function(tolua_S,"Lookup",tolua_wwii_wwii_HCUnit_Lookup01);
    tolua_function(tolua_S,"Lookup",tolua_wwii_wwii_HCUnit_Lookup02);
   tolua_endmodule(tolua_S);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_wwii (lua_State* tolua_S) {
 return tolua_wwii_open(tolua_S);
};
#endif


#pragma once

#include "locl/loclApp.h"

////////////////////////////
// LOGICAL return functions.
////////////////////////////
extern LOGICAL loclLangInit(ENUM_LANGUAGES enumLang);
extern void loclLangKill();
extern LOGICAL loclGetString(ENUM_CATEGORIES e, UINT32 uID, const SCHAR** returnStr);

////////////////////////////
// UCHAR * return functions.
////////////////////////////
extern UCHAR* loclLangGetString(ENUM_CATEGORIES enumCategory, UINT32 uID);
extern UCHAR* loclGetStringCategory(ENUM_CATEGORIES e);

////////////////////////////
// LangTable functions
////////////////////////////
extern LANGTABLE* loclLangTableLoad(ENUM_LANGUAGES language, SINT32 category);
extern const SCHAR* loclGetLangTableString(LANGTABLE* lt, UINT32 ID);
extern void loclLangTableFree(LANGTABLE*);

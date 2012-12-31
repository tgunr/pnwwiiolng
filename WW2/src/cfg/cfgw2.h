#pragma once
// Copyright 2008 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled library
// under License or NDA

extern int cfgLoadW2(const char* ConfigName, void** pVariables);
extern void cfgSetListItemValue(void** ppVariables, int index, void* value);
extern void* cfgGetListItemValue(void** list, int index);
extern int cfgGetListItemType(void** list, int index);
extern int cfgLoadW2Defaults(void** ppVariables);

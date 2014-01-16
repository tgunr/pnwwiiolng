#pragma once
// Copyright 2008 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled library
// under License or NDA

int cfgLoadW2(const char* ConfigName, void** pVariables);
void cfgSetListItemValue(void** ppVariables, int index, void* value);
void* cfgGetListItemValue(void** list, int index);
int cfgGetListItemType(void** list, int index);
int cfgLoadW2Defaults(void** ppVariables);

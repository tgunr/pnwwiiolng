#if !defined(GRANNY_POINTER_HASH_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_pointer_hash.h $
// $DateTime: 2009/10/12 15:54:38 $
// $Change: 23529 $
// $Revision: #8 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(MemoryAllocatorGroup);

EXPTYPE struct pointer_hash;
struct pointer_hash_iterator;

EXPAPI pointer_hash *NewPointerHash(void);
EXPAPI void DeletePointerHash(pointer_hash *Hash);

EXPAPI void AddPointerToHash(pointer_hash &Hash, void const *Key, void *Data);
EXPAPI void RemovePointerFromHash(pointer_hash &Hash, void const *Key);

EXPAPI bool HashedPointerKeyExists(pointer_hash &Hash, void const *Key);

EXPAPI bool SetHashedPointerData(pointer_hash &Hash, void const *Key, void *Data);
EXPAPI bool GetHashedPointerData(pointer_hash &Hash, void const *Key, void *&Data);

pointer_hash_iterator* PointerHashBegin(pointer_hash &Hash);
void DeletePointerHashIterator(pointer_hash &Hash, pointer_hash_iterator* Iter);
bool PointerHashIteratorNext(pointer_hash &Hash, pointer_hash_iterator* Iter);

void const* PointerHashIteratorKey(pointer_hash_iterator* Iter);
void*       PointerHashIteratorData(pointer_hash_iterator* Iter);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_POINTER_HASH_H
#endif

// ========================================================================
// $File: //jeffr/granny/rt/granny_pointer_hash.cpp $
// $DateTime: 2009/10/12 15:54:38 $
// $Change: 23529 $
// $Revision: #16 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_pointer_hash.h"

#include "granny_log.h"
#include "granny_memory.h"
#include "granny_parameter_checking.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

#define SubsystemCode HashLogMessage

USING_GRANNY_NAMESPACE;
BEGIN_GRANNY_NAMESPACE;

struct hash_entry
{
    void const *Key;
    void *Data;

    hash_entry *Left;
    hash_entry *Right;
};

struct pointer_hash_iterator
{
    hash_entry* Entry;
};

inline int32x HashKeyDifference(const void* a, const void* b)
{
    intaddrx Diff = ((const uint8 *)a - (const uint8 *)b);
    if (Diff < 0)
        return -1;
    else if (Diff > 0)
        return 1;
    return 0;
}

#define CONTAINER_NAME pointer_hash
#define CONTAINER_ITEM_TYPE hash_entry
#define CONTAINER_ADD_FIELDS void const *Key, void *Data
#define CONTAINER_ADD_ASSIGN(Item) (Item)->Key = Key; (Item)->Data = Data;
#define CONTAINER_COMPARE_ITEMS(Item1, Item2) HashKeyDifference((Item1)->Key, (Item2)->Key)
#define CONTAINER_FIND_FIELDS void const *Key
#define CONTAINER_COMPARE_FIND_FIELDS(Item) HashKeyDifference(Key, (Item)->Key)
#define CONTAINER_SORTED 1
#define CONTAINER_KEEP_LINKED_LIST 0
#define CONTAINER_SUPPORT_DUPES 0
#include "granny_contain.inl"

END_GRANNY_NAMESPACE;

pointer_hash *GRANNY
NewPointerHash(void)
{
    GRANNY_ENTER();

    pointer_hash *Hash = Allocate(pointer_hash);
    if(Hash)
    {
        Initialize(Hash, 0);
    }

    return(Hash);
}

void GRANNY
DeletePointerHash(pointer_hash *Hash)
{
    GRANNY_ENTER();

    if(Hash)
    {
        FreeMemory(Hash);
        Deallocate(Hash);
    }
}

void GRANNY
AddPointerToHash(pointer_hash &Hash, void const *Key, void *Data)
{
    GRANNY_ENTER();

    Assert(Find(&Hash, Key) == 0);

    hash_entry *Entry = Add(&Hash, Key, Data);
    if(!Entry)
    {
        Log0(ErrorLogMessage, HashLogMessage, "Out of space in pointer hash table");
    }
}

void GRANNY
RemovePointerFromHash(pointer_hash &Hash, void const *Key)
{
    GRANNY_ENTER();

    hash_entry *Entry = Find(&Hash, Key);
    if (Entry)
        Remove(&Hash, Entry);
}


bool GRANNY
SetHashedPointerData(pointer_hash &Hash, void const *Key, void *Data)
{
    GRANNY_ENTER();

    bool Result = true;

    hash_entry *Entry = Find(&Hash, Key);
    if(Entry)
    {
        Entry->Data = Data;
    }
    else
    {
        Result = false;
        Entry = Add(&Hash, Key, Data);
        if(!Entry)
        {
            Log0(ErrorLogMessage, HashLogMessage, "Out of space in pointer hash table");
        }
    }

    return(Result);
}

bool GRANNY
GetHashedPointerData(pointer_hash &Hash, void const *Key, void *&Data)
{
    GRANNY_ENTER();

    hash_entry *Entry = Find(&Hash, Key);
    if(Entry)
    {
        Data = Entry->Data;
        return true;
    }

    Data = 0;

    return false;
}

bool GRANNY
HashedPointerKeyExists(pointer_hash &Hash, void const *Key)
{
    GRANNY_ENTER();

    return (Find(&Hash, Key) != NULL);
}


pointer_hash_iterator* GRANNY
PointerHashBegin(pointer_hash& Hash)
{
    pointer_hash_iterator* Iter = Allocate(pointer_hash_iterator);
    if (!Iter)
    {
        return 0;
    }

    Iter->Entry = First(&Hash);
    if (Iter->Entry == 0)
    {
        DeletePointerHashIterator(Hash, Iter);
        Iter = 0;
    }

    return Iter;
}

void GRANNY
DeletePointerHashIterator(pointer_hash& Hash, pointer_hash_iterator* Iter)
{
    Deallocate(Iter);
}

bool GRANNY
PointerHashIteratorNext(pointer_hash& Hash, pointer_hash_iterator* Iter)
{
    if (!Iter || !Iter->Entry)
        return false;

    Iter->Entry = Next(&Hash, Iter->Entry);
    return (Iter->Entry != 0);
}

void const* GRANNY
PointerHashIteratorKey(pointer_hash_iterator* Iter)
{
    CheckPointerNotNull(Iter,        return 0);
    CheckPointerNotNull(Iter->Entry, return 0);

    return Iter->Entry->Key;
}

void* GRANNY
PointerHashIteratorData(pointer_hash_iterator* Iter)
{
    CheckPointerNotNull(Iter,        return 0);
    CheckPointerNotNull(Iter->Entry, return 0);

    return Iter->Entry->Data;
}

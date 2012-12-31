#pragma once

#include "trn/trncd.h"
#include "ob/obdraw.h"
#include "math/Vec3d.h"

#include <queue>
#include <vector>

struct trncdDTLCollider;

// Attributes for each sprite.
struct Attributes
{
    bool kill;

    char key[64];
    FLOAT64VECTOR pos;
    float maxdist;

};

extern void radclutterKill();
extern void radclutterUpdate();
extern void radclutterInit();
extern void radclutterEnable(bool enable);
extern void radclutterFreeAttributeIfNecessary(size_t key);
extern void radclutterDirty();
extern void radclutterRemoveAll();

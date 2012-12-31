#pragma once

#include "pi/pi.h"
#include "fx/fxMgr.h"

extern LOGICAL fxMiscInit();

extern void fxMiscKill();
extern void fxMiscGrabScene();
extern void fxMiscDrawEffects();
extern void fxMiscStartExpConcussion(FLOAT32 rotate, SINT32 timelimit);
extern void fxMiscStartHitConcussion(FLOAT32 rotate, SINT32 timelimit);
extern void fxMiscUpdate();
extern void fxMiscBlackout();
extern void fxMiscResetAll();

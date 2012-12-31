#pragma once

class Texture;

extern LOGICAL persGunsightInit(void);
extern LOGICAL persGunsightKill(void);

extern LOGICAL persGunsightUpdate(void);

extern LOGICAL persPrepGunsightDraw(void);
extern LOGICAL persGunsightDraw(OBJECT * DOF,UINT32 IsInstrumentView);

extern UINT32 persGunsightRangeInc(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 persGunsightRangeDec(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 persGunsightTargetSpeedInc(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 persGunsightTargetSpeedDec(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 persGunsightTargetAngleInc(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 persGunsightTargetAngleDec(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 persGunsightTargetMarkBearing(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 persGunsightTorpedoDepthInc(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 persGunsightTorpedoDepthDec(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 persGunsightCalcTargetSolution(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 persGunsightConvergenceInc(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 persGunsightConvergenceDec(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 persGunsightTorpedoDepthDec(SINT32 Param,UINT32 Id,UINT32 Detach);

extern UINT32 persResetGunsightAdjust(FLOAT32);

extern LOGICAL persLoadGunsight(SINT8* FileName);
extern LOGICAL persLoadInstGunsight(SINT8* FileName);
extern LOGICAL persClearGunsights(void);

extern LOGICAL persGunSightSetBaseFOV(FLOAT32 FOV);
extern LOGICAL persAlignGunsight(OBJECT * AlignObj,OBJECT * ViewDOF,FLOAT32 AlignmentRange);
extern LOGICAL persAlignInstGunsight(OBJECT * AlignObj,OBJECT * ViewDOF,FLOAT32 AlignmentRange);
extern LOGICAL persUpdateGunsightView(void);
extern LOGICAL persUpdateInstGunsightView(void);
extern LOGICAL persGunsightClearData(void);
extern LOGICAL persGunsightGetInstFieldOfView(FLOAT32* FOV);
extern LOGICAL persGunsightGetGunSightFieldOfView(FLOAT32* FOV);
extern LOGICAL persGunsightCalcSightAngle(FLOAT32 * ReturnAngle,SINT8* AmmoName,FLOAT32 Range);

extern LOGICAL persGunsightAlignDOF(piMATRIX * RetMatrix, OBJECT * AlignedObject, OBJECT * AlignToObject, FLOAT32 AlignmentRange);

extern void persGunsightSetConvergenceRange(FLOAT32);
extern void persGunsightDrawFPSScope(int type);
extern void persGunsightDrawOverlay();

extern FLOAT32 persGunsightConvergenceRange();

extern FLOAT32 persGunsightGetRange(void);

#if USE_STATESET
Texture* persGunSightGetOverlayTexture();
#endif
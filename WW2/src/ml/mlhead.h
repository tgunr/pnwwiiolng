#pragma once

extern UINT32 mlheadGetHeadView(struct FLOAT64ATT* HeadAtt, struct FLOAT64ATT* EyeAtt);
extern UINT32 mlSetHeadViewCallback(SINT32 keyState, UINT32 functionNum, UINT32);
extern UINT32 mlGetHeadPos(struct FLOAT64VECTOR* Displacement, struct FLOAT64ATT* AttChange,
					struct mlVehicleData* VehicleData, struct FLOAT64VECTOR* NeckPos, union piMATRIX* NeckAttMat);
extern UINT32 mlToggleHeadLook(SINT32 keyState, UINT32 functionNum, UINT32);
extern UINT32 mlHeadLook(SINT32 param, UINT32 functionNum, UINT32);
extern UINT32 mlToggleMouseLook(SINT32 keyState, UINT32 functionNum, UINT32);
extern UINT32 mlMouseLook(SINT32 param, UINT32 functionNum, UINT32);
extern void mlCenterMouseLook();

extern void mlSetMouseLookAndTorsoKeymapFlag(LOGICAL);
extern LOGICAL mlMouseLookAndTorsoUseSameKeymap();

extern void mlSetMouseLookEnabled(LOGICAL);
extern LOGICAL mlMouseLookEnabled();


#ifndef MLHEAD_H
#define MLHEAD_H

#include "mltypes.h"

/************************************************************************************************
	mlHead
*************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

UINT32 mlheadGetHeadView(FLOAT64ATT* HeadAtt, FLOAT64ATT* EyeAtt);
UINT32 mlSetHeadViewCallback(SINT32 keyState, UINT32 functionNum, UINT32);
UINT32 mlGetHeadPos(FLOAT64VECTOR* Displacement, FLOAT64ATT* AttChange, 
					mlVehicleData* VehicleData, FLOAT64VECTOR* NeckPos,piMATRIX* NeckAttMat);
UINT32 mlToggleMouseLook(SINT32 keyState, UINT32 functionNum, UINT32);
UINT32 mlMouseLook(SINT32 param, UINT32 functionNum, UINT32);
void mlCenterMouseLook();

void mlSetMouseLookAndTorsoKeymapFlag(LOGICAL);
LOGICAL mlMouseLookAndTorsoUseSameKeymap();

void mlSetMouseLookEnabled(LOGICAL);
LOGICAL mlMouseLookEnabled();


#ifdef __cplusplus
}
#endif							

#endif // MLHEAD_H
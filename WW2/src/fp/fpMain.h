#pragma once
/*
	 First Person System Core
	 Copyright 2008 Playnet Inc, All rights reserved
	 Not to be distributed in API form except as part of a compiled library
	 under License or NDA

	Originating Programmer : Jonathan Miller

	Rev 1  c2001
*/

extern bool fpUpdatePerson(struct mlVehicleData* VehicleData, FLOAT64 DeltaTime);

extern LOGICAL fpInit(struct vehVehicle*vehicle);
extern LOGICAL fpKill(struct mlVehicleData* VehicleData);
extern LOGICAL fpProcessVehicleAnimationFlags(struct vehVehicle*vehicle);
extern LOGICAL fpHandleProneTrooperPosition(struct OBJECT* pObject, struct vehVehiclePositionData * PosData);
extern LOGICAL fpSetExternal(struct mlVehicleData * ModelData);
extern LOGICAL fpSetInternal(struct mlVehicleData * ModelData);
extern LOGICAL fpGetEyePoint(struct mlVehicleData* pVehicleData, union piMATRIX* pMatrix, LOGICAL allowGunSight);
//FLOAT32 fpGetZAdjust(int pose, int move, int point = 0, int type = 0); // fps = 0, tps = 1
extern FLOAT32 fpGetZAdjust(int pose, int move, int type = 0); // fps = 0, tps = 1

// walking support
extern UINT32 fpCollideRoundWithVehicleWalkObject(struct FASTCOLLIDE ** CollideListPtr,
										 UINT32* NumCollisionSlotsLeft,
										 struct piVECTOR const* OldPos,
										 struct piVECTOR const* NewPos);
extern void fpMoveToRelRidePos(struct vehVehicle* FPSRideVehicle, struct vehVehicle* Vehicle);
extern void fpSaveRelRidePos(struct vehVehicle* FPSRideVehicle, struct vehVehicle* Vehicle);

#define _FP_WALK_COLLIDE_FLAG 0x80000000

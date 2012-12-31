#ifndef MLUTILITY_H
#define MLUTILITY_H

#include "pi.h"
#include "vars.h"

#include "mlcompon.h"
#include "obmain.h"
#include "obcd.h"

/*
	Various utilities and conversion functions for the ml system
	Copyright 1999 Playnet Inc, All rights reserved

	Originating Programmer: Jonathan Hoof
	Maintaining Programmer: Jonathan Hoof
	Supporting Programmer: John Lundy
*/

// Converters from radians to degrees
// computed out because macros get ignored by the optimizer
#define mlDEG_TO_RAD(a) ((a*0.017453293f))
#define mlRAD_TO_DEG(a) ((a * 57.29577951f))

// model is the vehicle modelling system for coordinates
// render is the render system for coordinates
#define mlConvertModelPositionToRenderPosition(Dest,Src) \
					(Dest)->x = (Src)->x; \
					(Dest)->y = (Src)->y; \
					(Dest)->z = (Src)->z;
#define mlConvertRenderPositionToModelPosition(Dest,Src) \
					(Dest)->x = (Src)->x; \
					(Dest)->y = (Src)->y; \
					(Dest)->z = (Src)->z;

#define mlConvertModelVelocityToRenderVelocity(Dest,Src) \
					(Dest)->x = -(Src)->x; \
					(Dest)->y = -(Src)->y; \
					(Dest)->z = -(Src)->z;
#define mlConvertRenderVelocityToModelVelocity(Dest,Src) \
					(Dest)->x = -(Src)->x; \
					(Dest)->y = -(Src)->y; \
					(Dest)->z = -(Src)->z;

// Routine to take an input string of the format
//    varname = varcontents
// and extracts "varname" and "varcontents"
// used in the ml system parsing routines
// no checking for invalid pointers done
extern UINT32 mlUtilityGetVarnameAndContents(UCHAR ** Varname, // return varname string
											 UCHAR ** Varcontents, // return varcontents string
											 UCHAR * InputString); // input string


// routine to take a variable name and a variable name list
// and return the index of the variable name in that list
// if the varname doesn't exist, 0 is returned
// the index is returned in varnum
// no checking for invalid pointers done
extern UINT32 mlUtilityFindVarname(UINT32 *VarNum, // return index into list
									const UCHAR *VarName, // variable name to find in list
									const UCHAR *VehicleTypeVarNames, // list of variable names
									UINT32 VarNameSize, // required to index array correctly
									UINT32 MaxNames); // size of list

// routine that goes through a component list to find
// a component with name set to ComponentName
// returns result in ComponentPtr, NULL if not found
// no checking for invalid pointers done
extern UINT32 mlUtilityFindComponentByName(mlComponent **ComponentPtr, // return compontent ptr
											mlComponentList * ComponentList, // component list
											UCHAR *ComponentName, // component name to look for
											mlComponent * Component); // no longer used

// interpolate between two numbers
// Num1 and Num2 are the 2 numbers interpolated between
// Delta is the distance between Num1 and Num2, with 0.0 being Num1, and 1.0 being Num2
// no checking for invalid pointers done
extern UINT32 mlInterpolate(FLOAT64 *ReturnNum, // return number
							FLOAT64 Num1, // number at delta 0.0
							FLOAT64 Num2, // number at delta 1.0
							FLOAT64 Delta); // point between Num1 and Num2 to determine

// the rendering system uses a different attitude order and handedness than the
// modelling system
// this is because the system used in the rendering system doesn't make intuitive sense
// for the modelling system and vice versa
// so a translation routine is needed
// no checking for invalid pointers done
extern UINT32 mlConvertModelAttitudeToWorldAttitude(piVECTOR * WorldAtt, // return attitude
													piVECTOR * ModelAtt); // input attitude

// basic forward prediction algorithm
// Since the modelling routines run asyncronously to the frame rate, an "adjustment"
// is needed to extrapolate the exact position and attitude at the frame time
// otherwise an annoying chaotic jitter occurs
// no checking for invalid pointers done
// the routine determines the delta time from LastUpdateTime and frametime
extern UINT32 mlForwardPredict(FLOAT64VECTOR * NewPos, // adjusted position in frame time
							FLOAT64ATT * NewAtt, // adjusted attitude in frame time
							FLOAT64VECTOR * CurrentPos, // current position in model time
							FLOAT64ATT * CurrentAtt, // current attitude in model time
							FLOAT64VECTOR * CurrentVel, // current velocity in model time
							FLOAT64ATT * CurrentAttVel, // current attitude velocity in model time
							FLOAT64 LastUpdateTime); // frame time of last model update time

// Converts an object position/attitude matrix to the model system's attitude
extern UINT32 mlUtilityGetModelAttitudeFromObjectMatrix(FLOAT64ATT * Att,piMATRIX * Mat);

// Converts an object position/attitude matrix to the model system's position
extern UINT32 mlUtilityGetModelPositionFromObjectMatrix(FLOAT64VECTOR * Pos,piMATRIX * Mat);

// the object/modelling system inverts the "z" axis, thus if we want to use
// an object matrix/DOF matrix in the modelling system we must negate the z line
extern UINT32 mlUtilityConvertMatrixToModelReference(piMATRIX * DestMat,piMATRIX * SourceMat);

// converts a model-reference attitude to matrix format
extern UINT32 mlUtilityConvertAttToMatrix(piMATRIX * Mat,FLOAT64ATT * Att);

// sorts from smallest to largest
extern UINT32 mlUtilitySortCollideList(FASTCOLLIDE * CollideList,UINT32 NumElements);

extern UINT32 mlUtilityGetVarnameAndContents1(UCHAR ** Varname, UCHAR ** Varcontents,UCHAR * InputString);


SINT32 mlUtilityGetPosAndAttFromDOF(OBJECT* dofObj, piVECTOR const* relpos, piVECTOR* pos, piVECTOR* att);

UINT32 mlUtilityCheckLineSphereCollide(UINT32 * Collision, piVECTOR const* SphereCenter, 
									piVECTOR const* UseOldPos, piVECTOR const* UseNewPos, FLOAT32 Radius);


#endif
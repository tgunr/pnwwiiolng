#include "syscfg.h" // Path is project specific.
#include "pi/pi.h"
#include "include/vars.h"

#include <math.h>

#include "ogrp/ogrptype.h"

#include "ob/obmain.h"
#include "ob/obMgr.h"
#include "ob/obpool.h"
#include "ob/obplyr.h"
#include "ob/obdraw.h"

#include "math/math3d.h"
#include "math/matheul.h"
#include "math/mathvect.h"

#include "ml/mlutility.h"
#include "ml/mlgauges.h"
#include "physics/physicsMain.h"
#include "pers/persmain.h"

#define _COCKPIT_SCALEFACTOR 20.0;

UINT32 _SetObjectAndSubtreeToNotDraw(OBJECT * ObjectPtr);
UINT32 _SetObjectAndSubtreeToDraw(OBJECT * ObjectPtr);
UINT32 _ShowObjectAndSubtree(OBJECT * ObjectPtr);
UINT32 _HideObjectAndSubtree(OBJECT * ObjectPtr);
UINT32 _FindSubObjectByName(OBJECT ** ReturnObject,OBJECT * ObjectPtr, const char * Name);
UINT32 _FindSubObjectByDOFVar(OBJECT ** ReturnObject,OBJECT * ObjectPtr,UINT32 VarNum);
UINT32 _FindSubObjectByDOFVarAndSubVar(OBJECT ** ReturnObject,OBJECT * ObjectPtr,UINT32 VarNum,UINT32 SubVarNum);
static UINT32 _SetObjectAndTreeAsCockpit(OBJECT * Object);
static UINT32 _MoveAndRotateSubObject(OBJECT * ObjectPtr,
											 piMATRIX * CurrentMatrix,
											 piMATRIX * MatInt,
											 piMATRIX * OriginalMatrix,
											 bool IncludeSiblings);

UCHAR _Buffer[256];



// moves an object tree as a whole and
// rotates any DOF objects
//
// note: any object tagged as "don't draw" won't be affected
UINT32 ogrpMoveAndRotateObjectGroup(OBJECT * ObjectPtr,piVECTOR * Pos,piVECTOR * Att)
{
piMATRIX Matrix;

	  Matrix = IdentityMatrix;


//	  math3dMatrixRotate(&Matrix, Att);
	  mathEul_ToMatrix(&Matrix,EulOrdXYZr,Att);


	  Matrix._41 = Pos->x;
	  Matrix._42 = Pos->y;
	  Matrix._43 = Pos->z;

	  ObjectPtr->matint = IdentityMatrix;

	  ogrpMoveAndRotateSubObject(ObjectPtr,&Matrix,&Matrix);
//	obplyrUpdateThisObject(Object, &Pos, &Att);

	return 0;
}


// note: this routine doesn't catch siblings unless specified!
UINT32 ogrpMoveAndRotateSubObject(OBJECT * ObjectPtr,
									 piMATRIX * CurrentMatrix,
									 piMATRIX * OriginalMatrix)
{
	if (PN_UNLIKELY(ObjectPtr == nullptr))
		return 0;

	piMATRIX MatInt = ObjectPtr->matint;
	_MoveAndRotateSubObject(ObjectPtr, CurrentMatrix, &MatInt, OriginalMatrix, false /*IncludeSiblings*/);

	return 0;
}

// note: this routine doesn't catch siblings unless specified!
static UINT32 _MoveAndRotateSubObject(OBJECT * ObjectPtr,
										 piMATRIX * CurrentMatrix,
										 piMATRIX * MatInt,
										 piMATRIX * OriginalMatrix,
										 bool IncludeSiblings)
{
	if (PN_UNLIKELY(ObjectPtr == nullptr || CurrentMatrix == nullptr))
		return 0;

	piMATRIX NewMatrix;
	piMATRIX LocalMatInt;
	OBJECT * ObjAtt;
	piMATRIX Matrix,CumulativeMatrix;
	piVECTOR Rot;
	FLOAT32 TempNum;
	OBJECTDOF * Dof;
	OBJECTSTATICDOF * DofStatic;
	UINT32 NotDOF;
	UINT32	DofFlags;

	do
	{
		if (ObjectPtr->flags & OBJECT_DOF &&
			ObjectPtr->dof->dofvarnum != 0 &&
			ObjectPtr->dof->dofflags & (DOF_ONE_ROTATE_AXIS|DOF_ONE_TRANS_AXIS|DOF_USE_DOFPOSMATRIX))
		{
			Dof = ObjectPtr->dof;
			DofFlags = Dof->dofflags;
			DofStatic = ObjectPtr->dof->dofstatic;

			if (!(DofFlags & DOF_USE_DOFPOSMATRIX))
			{
				if (DofFlags & DOF_ONE_ROTATE_AXIS)
				{
					Rot.x = Rot.y = Rot.z = 0;

					if (DofFlags & DOF_IGNORECENTER)
					{
						if (DofFlags & DOF_ROTATES_X)
						{
							TempNum = DofStatic->dofmaxrotate.x - DofStatic->dofminrotate.x;
							TempNum *= Dof->dofvarvalue;
							TempNum += DofStatic->dofminrotate.x;
							Rot.x = -(FLOAT32)mlDEG_TO_RAD(TempNum);
						}

						if (DofFlags & DOF_ROTATES_Y)
						{
							TempNum = DofStatic->dofmaxrotate.y - DofStatic->dofminrotate.y;
							TempNum *= Dof->dofvarvalue;
							TempNum += DofStatic->dofminrotate.y;
							Rot.y = -(FLOAT32)mlDEG_TO_RAD(TempNum);
						}

						if (DofFlags & DOF_ROTATES_Z)
						{
							TempNum = DofStatic->dofmaxrotate.z - DofStatic->dofminrotate.z;
							TempNum *= Dof->dofvarvalue;
							TempNum += Dof->dofstatic->dofminrotate.z;
							Rot.z = -(FLOAT32)mlDEG_TO_RAD(TempNum);
						}
					}
					else
					{
						if (Dof->dofvarvalue > 0)
						{
							if (DofFlags & DOF_ROTATES_X)
							{
								TempNum = DofStatic->dofmaxrotate.x - DofStatic->dofcurrotate.x;
								TempNum *= Dof->dofvarvalue;
								TempNum += DofStatic->dofcurrotate.x;
								Rot.x = -(FLOAT32)mlDEG_TO_RAD(TempNum);
							}

							if (DofFlags & DOF_ROTATES_Y)
							{
								TempNum = DofStatic->dofmaxrotate.y - DofStatic->dofcurrotate.y;
								TempNum *= Dof->dofvarvalue;
								TempNum += DofStatic->dofcurrotate.y;
								Rot.y = -(FLOAT32)mlDEG_TO_RAD(TempNum);
							}

							if (DofFlags & DOF_ROTATES_Z)
							{
								TempNum = DofStatic->dofmaxrotate.z - DofStatic->dofcurrotate.z;
								TempNum *= Dof->dofvarvalue;
								TempNum += DofStatic->dofcurrotate.z;
								Rot.z = -(FLOAT32)mlDEG_TO_RAD(TempNum);
							}
						}
						else
						{
							if (DofFlags & DOF_ROTATES_X)
							{
								TempNum = DofStatic->dofminrotate.x - DofStatic->dofcurrotate.x;
								TempNum *= -Dof->dofvarvalue;
								TempNum += DofStatic->dofcurrotate.x;
								Rot.x = -(FLOAT32)mlDEG_TO_RAD(TempNum);
							}

							if (DofFlags & DOF_ROTATES_Y)
							{
								TempNum = DofStatic->dofminrotate.y - DofStatic->dofcurrotate.y;
								TempNum *= -Dof->dofvarvalue;
								TempNum += DofStatic->dofcurrotate.y;
								Rot.y = -(FLOAT32)mlDEG_TO_RAD(TempNum);
							}

							if (DofFlags & DOF_ROTATES_Z)
							{
								TempNum = DofStatic->dofminrotate.z - DofStatic->dofcurrotate.z;
								TempNum *= -Dof->dofvarvalue;
								TempNum += DofStatic->dofcurrotate.z;
								Rot.z = -(FLOAT32)mlDEG_TO_RAD(TempNum);
							}
						}
					}

					if (DofFlags & (DOF_ROTATES_X|DOF_ROTATES_Y|DOF_ROTATES_Y|DOF_ROTATES_Z|DOF_ROTATES_Z|DOF_ROTATES_X))
					{
						Matrix = IdentityMatrix;

                        if(DofFlags & DOF_ROTATES_X)
						{
							Matrix._22 = (FLOAT32)cos(Rot.x);
							Matrix._32 = (FLOAT32)sin(Rot.x);
							Matrix._23 = -Matrix._32;
							Matrix._33 = Matrix._22;
						}

                        if(DofFlags & DOF_ROTATES_Y)
						{
                            piMATRIX m = IdentityMatrix;

							m._33 = (FLOAT32)cos(Rot.y);
							m._13 = (FLOAT32)sin(Rot.y);
							m._31 = -m._13;
							m._11 =  m._33;

                            math3dMatrixMultiply(&Matrix, &m, &Matrix);
						}

						if(DofFlags & DOF_ROTATES_Z)
						{
                            piMATRIX m = IdentityMatrix;

							m._11 = (FLOAT32)cos(Rot.z);
							m._21 = (FLOAT32)sin(Rot.z);
							m._12 = -m._21;
							m._22 =  m._11;

                            math3dMatrixMultiply(&Matrix, &m, &Matrix);
						}
					}
					else
					{
						mathEul_ToMatrixFast(&Matrix,EulOrdXYZr,&Rot);
					}
				}
				else
				{
					Matrix = IdentityMatrix;
				}

				if (DofFlags & DOF_ONE_TRANS_AXIS)
				{
					if (DofFlags & DOF_IGNORECENTER)
					{
						if (DofFlags & DOF_TRANSLATES_X)
						{
							TempNum = DofStatic->dofmaxtrans.x - DofStatic->dofmintrans.x;
							TempNum *= Dof->dofvarvalue;
							TempNum += DofStatic->dofmintrans.x;
							Matrix._41 = TempNum*ScaleFactor;
						}

						if (DofFlags & DOF_TRANSLATES_Y)
						{
							TempNum = DofStatic->dofmaxtrans.y - DofStatic->dofmintrans.y;
							TempNum *= Dof->dofvarvalue;
							TempNum += DofStatic->dofmintrans.y;
							Matrix._42 = TempNum*ScaleFactor;
						}

						if (DofFlags & DOF_TRANSLATES_Z)
						{
							TempNum = DofStatic->dofmaxtrans.z - DofStatic->dofmintrans.z;
							TempNum *= Dof->dofvarvalue;
							TempNum += DofStatic->dofmintrans.z;
							Matrix._43 = TempNum*ScaleFactor;
						}
					}
					else
					{
						if (ObjectPtr->dof->dofvarvalue > 0)
						{
							if (DofFlags & DOF_TRANSLATES_X)
							{
								TempNum = DofStatic->dofmaxtrans.x - DofStatic->dofcurtrans.x;
								TempNum *= Dof->dofvarvalue;
								TempNum += DofStatic->dofcurtrans.x;
								Matrix._41 = TempNum*ScaleFactor;
							}

							if (DofFlags & DOF_TRANSLATES_Y)
							{
								TempNum = DofStatic->dofmaxtrans.y - DofStatic->dofcurtrans.y;
								TempNum *= Dof->dofvarvalue;
								TempNum += DofStatic->dofcurtrans.y;
								Matrix._42 = TempNum*ScaleFactor;
							}

							if (DofFlags & DOF_TRANSLATES_Z)
							{
								TempNum = DofStatic->dofmaxtrans.z - DofStatic->dofcurtrans.z;
								TempNum *= Dof->dofvarvalue;
								TempNum += DofStatic->dofcurtrans.z;
								Matrix._43 = TempNum*ScaleFactor;
							}
						}
						else
						{
							if (DofFlags & DOF_TRANSLATES_X)
							{
								TempNum = DofStatic->dofmintrans.x - DofStatic->dofcurtrans.x;
								TempNum *= -Dof->dofvarvalue;
								TempNum += DofStatic->dofcurtrans.x;
								Matrix._41 = TempNum*ScaleFactor;
							}

							if (DofFlags & DOF_TRANSLATES_Y)
							{
								TempNum = DofStatic->dofmintrans.y - DofStatic->dofcurtrans.y;
								TempNum *= -Dof->dofvarvalue;
								TempNum += DofStatic->dofcurtrans.y;
								Matrix._42 = TempNum*ScaleFactor;
							}

							if (DofFlags & DOF_TRANSLATES_Z)
							{
								TempNum = DofStatic->dofmintrans.z - DofStatic->dofcurtrans.z;
								TempNum *= -Dof->dofvarvalue;
								TempNum += DofStatic->dofcurtrans.z;
								Matrix._43 = TempNum*ScaleFactor;
							}
						}
					}
				}

				math3dMatrixMultiplyUnique(&CumulativeMatrix, &Dof->dofmatrix1, &Matrix);
			}
			else
			{
				CumulativeMatrix = Dof->dofmatrix1;
			}

			if (DofFlags & DOF_USE_DOFPOSMATRIX)
			{
				math3dMatrixMultiply(&CumulativeMatrix, &CumulativeMatrix, &Dof->dofposmatrix);
			}

			math3dMatrixMultiply(&CumulativeMatrix, &CumulativeMatrix, &Dof->dofmatrix2);

			// voila, we should be done
			math3dMatrixMultiplyUnique(&LocalMatInt, &CumulativeMatrix, MatInt);
			if (OriginalMatrix != nullptr)
			{
				math3dMatrixMultiplyUnique(&NewMatrix, &LocalMatInt, OriginalMatrix);
			}
			else
			{
				math3dMatrixMultiply(&NewMatrix, &CumulativeMatrix, &NewMatrix);
			}

			NotDOF = 0;
		}
		else
		{
			NotDOF = 1;
		}

		if (!(ObjectPtr->flags & OBJECT_NOTOBJECT))
		{
			for(ObjAtt = ObjectPtr; ObjAtt; ObjAtt = ObjAtt->attached)
			{
				ObjAtt->mat = NotDOF ? *CurrentMatrix : NewMatrix;

				// vehicles are the only things that move
				if(ObjAtt->collider && colliderIsType(ObjAtt->collider->getCategoryMask(), COLLIDER_VEHICLE_MASK))
					ObjAtt->collider->updateTransform(ObjAtt->mat);
			}
		}
		else
		{
			if (ObjectPtr->flags & OBJECT_DOF)
			{
				if (ObjectPtr->flag2 & OBJECT_DOF_USECHILDMATRIX &&
					NotDOF == 0)
				{
					ObjectPtr->mat = NewMatrix;
				}
				else
				{
					ObjectPtr->mat = *CurrentMatrix;
				}
			}
			else if (!(ObjectPtr->flags & OBJECT_LOD))
			{
				if (NotDOF == 1)
				{
					ObjectPtr->mat = *CurrentMatrix;
				}
				else
				{
					ObjectPtr->mat = NewMatrix;
				}
			}
		}

		if (OriginalMatrix != nullptr)
		{
			ObjAtt = ObjectPtr;
			do
			{
				ObjAtt->matint = *MatInt;

				ObjAtt = ObjAtt->attached;
			} while (ObjAtt != nullptr);
		}

				// check for LODs
		// if a LOD is not visible, and it's not a user-controlled vehicle
		// don't bother updating all the sub-matricies
		if (ObjectPtr->flags & OBJECT_LOD)
		{
		piVECTOR TempPos;

			TempPos.x = OriginalMatrix->_41;
			TempPos.y = OriginalMatrix->_42;
			TempPos.z = OriginalMatrix->_43;

			ObjectPtr->flags |= OBJECT_USEMATRIX;

			// if frame factor too low, force a relight on newly visible LODs
			if (FrameFactor > 0.99)
			{
				obdrawProcessLODCallback(ObjectPtr, 0, &TempPos, FALSE, 0.0f, nullptr);
			}
			else
			{
				obdrawProcessLODCallback(ObjectPtr, 0, &TempPos, TRUE, 0.0f, nullptr);
			}

//			ObjectPtr->mat = NewMatrix;
			ObjectPtr->mat = *CurrentMatrix;


			if (ObjectPtr->flags & OBJECT_SKIPDRAWING &&
				!(ObjectPtr->flag2 & OBJECT_USERVEHICLE) &&
				!(ObjectPtr->flag2 & OBJECT_SPECIAL_LOD))
			{
				if (IncludeSiblings)
				{
					ObjectPtr = ObjectPtr->sibling;
					continue;
				}
				else
				{
					break;
				}
			}
		}

		if (ObjectPtr->flag2 & OBJECT_DONT_UPDATE)
		{
			// hitmap objects are only updated when doing collision detection
			ObjectPtr = ObjectPtr->sibling;
			continue;
		}

		if (ObjectPtr->child != nullptr)
		{
			if (NotDOF == 1)
			{
				_MoveAndRotateSubObject(ObjectPtr->child, CurrentMatrix, MatInt, OriginalMatrix, true);
			}
			else
			{
				_MoveAndRotateSubObject(ObjectPtr->child, &NewMatrix, &LocalMatInt, OriginalMatrix, true);
			}
		}

		ObjectPtr = ObjectPtr->sibling;
	}
	while (ObjectPtr != nullptr && IncludeSiblings);

	return 0;
}

// frees an object tree
// obsolete, call obpoolFreeObjectCallback
UINT32 ogrpFreeObjectGroup(OBJECT * ObjectPtr)
{
	obpoolFreeObjectCallback(ObjectPtr);
	return 0;
}

// sets the DOF information for this particular object (not the subtree)
// CycleVarContents treats bounds differently when set to 1
// basically if a number goes beyond the bound, it "continues" at the other end of the bound
UINT32 ogrpSetDOFVariable(OBJECT * ObjectPtr, FLOAT32 VarContents, UINT32 CycleVarContents, UINT32 IgnoreCurrent)
{
	FLOAT32 ProcessedVarContents;

	if (PN_UNLIKELY(ObjectPtr == nullptr)) return 1;

	if((ObjectPtr->flags & OBJECT_DOF) && !(ObjectPtr->flag2 & OBJECT_MISSING))
	{
		if (IgnoreCurrent == 1)
		{
			ObjectPtr->dof->dofflags |= DOF_IGNORECENTER;

			if (ObjectPtr->dof->dofmaxval == ObjectPtr->dof->dofminval)
			{
				ProcessedVarContents = 0.0;
			}
			else
			{
				if (ObjectPtr->dof->dofmaxval < ObjectPtr->dof->dofminval)
				{
					ProcessedVarContents = (VarContents - ObjectPtr->dof->dofmaxval) /
								(ObjectPtr->dof->dofminval - ObjectPtr->dof->dofmaxval);
					ProcessedVarContents = (FLOAT32)(1.0 - ProcessedVarContents);
				}
				else
				{
					ProcessedVarContents = (VarContents - ObjectPtr->dof->dofminval) /
								(ObjectPtr->dof->dofmaxval - ObjectPtr->dof->dofminval);
				}
			}

			ObjectPtr->dof->dofvarvalue = ProcessedVarContents;
		}
		else
		{
			ObjectPtr->dof->dofflags &= ~DOF_IGNORECENTER;

			if (ObjectPtr->dof->dofmaxval < ObjectPtr->dof->dofminval)
			{
				ProcessedVarContents = (ObjectPtr->dof->dofminval - (FLOAT32)VarContents) +
									   ObjectPtr->dof->dofmaxval;

				if (!CycleVarContents)
				{
					if (ProcessedVarContents > ObjectPtr->dof->dofminval)
					{
						ProcessedVarContents = ObjectPtr->dof->dofminval;
					}

					if (ProcessedVarContents < ObjectPtr->dof->dofmaxval)
					{
						ProcessedVarContents = ObjectPtr->dof->dofmaxval;
					}
				}
				else
				{
					ProcessedVarContents -= ObjectPtr->dof->dofmaxval;
					ProcessedVarContents /= (ObjectPtr->dof->dofminval - ObjectPtr->dof->dofmaxval);
					ProcessedVarContents = (FLOAT32)(ProcessedVarContents - floor(ProcessedVarContents));
					ProcessedVarContents *= (ObjectPtr->dof->dofminval - ObjectPtr->dof->dofmaxval);
					ProcessedVarContents += ObjectPtr->dof->dofmaxval;
				}

				if (ProcessedVarContents > ObjectPtr->dof->dofneutralval)
				{
					if (ObjectPtr->dof->dofminval - ObjectPtr->dof->dofneutralval < 0.0001f &&
						ObjectPtr->dof->dofminval - ObjectPtr->dof->dofneutralval > -0.0001f)
					{
						ObjectPtr->dof->dofvarvalue = 1.0f;
					}
					else
					{
						ObjectPtr->dof->dofvarvalue =
								(ProcessedVarContents - ObjectPtr->dof->dofneutralval) /
								(ObjectPtr->dof->dofminval - ObjectPtr->dof->dofneutralval);
					}
				}
				else
				{
					if (ObjectPtr->dof->dofmaxval - ObjectPtr->dof->dofneutralval < 0.0001f &&
						ObjectPtr->dof->dofmaxval - ObjectPtr->dof->dofneutralval > -0.0001f)
					{
						ObjectPtr->dof->dofvarvalue = 0.0f;
					}
					else
					{
						ObjectPtr->dof->dofvarvalue =
								-(ProcessedVarContents - ObjectPtr->dof->dofneutralval) /
								(ObjectPtr->dof->dofmaxval - ObjectPtr->dof->dofneutralval);
					}
				}
			}
			else
			{

				ProcessedVarContents = (FLOAT32)VarContents;

				if (!CycleVarContents)
				{
					if (ProcessedVarContents > ObjectPtr->dof->dofmaxval)
					{
						ProcessedVarContents = ObjectPtr->dof->dofmaxval;
					}

					if (ProcessedVarContents < ObjectPtr->dof->dofminval)
					{
						ProcessedVarContents = ObjectPtr->dof->dofminval;
					}
				}
				else
				{
                    if(persIsPersonaVehicleType(VEH_TYPE_AIR))
                    {
					    ProcessedVarContents -= ObjectPtr->dof->dofminval;
					    ProcessedVarContents /= (FLOAT32)(ObjectPtr->dof->dofmaxval - ObjectPtr->dof->dofminval);
					    ProcessedVarContents = (FLOAT32)(ProcessedVarContents - floor(ProcessedVarContents));
					    ProcessedVarContents *= (FLOAT32)(ObjectPtr->dof->dofmaxval - ObjectPtr->dof->dofminval);
					    ProcessedVarContents += ObjectPtr->dof->dofminval;
                    }
                    else
                    {
                        float t = ProcessedVarContents / (ObjectPtr->dof->dofmaxval - ObjectPtr->dof->dofminval);
                        //t = t < 0 ? 0 : t > 1 ? 1 : t; // Ticket# 4913

                        ProcessedVarContents = ObjectPtr->dof->dofminval + t * (ObjectPtr->dof->dofmaxval - ObjectPtr->dof->dofminval);
                    }
			    }

				if (ProcessedVarContents > ObjectPtr->dof->dofneutralval)
				{
					if (ObjectPtr->dof->dofmaxval - ObjectPtr->dof->dofneutralval < 0.0001f &&
						ObjectPtr->dof->dofmaxval - ObjectPtr->dof->dofneutralval > -0.0001f)
					{
						ObjectPtr->dof->dofvarvalue = 1.0f;
					}
					else
					{
						ObjectPtr->dof->dofvarvalue =
								(ProcessedVarContents - ObjectPtr->dof->dofneutralval) /
								(ObjectPtr->dof->dofmaxval - ObjectPtr->dof->dofneutralval);
					}
				}
				else
				{
					if (ObjectPtr->dof->dofminval - ObjectPtr->dof->dofneutralval < 0.0001f &&
						ObjectPtr->dof->dofminval - ObjectPtr->dof->dofneutralval > -0.0001f)
					{
						ObjectPtr->dof->dofvarvalue = 0.0f;
					}
					else
					{
						ObjectPtr->dof->dofvarvalue =
								-(ProcessedVarContents - ObjectPtr->dof->dofneutralval) /
								(ObjectPtr->dof->dofminval - ObjectPtr->dof->dofneutralval);
					}
				}

			}
		}
	}

	return 0;
}

// sets up the DOF data for subsequent ogrpSetDOFVariable
//
// Varlist is the text-based list of variables in order, first being 0 and goes up from there
UINT32 ogrpParseDOFParameters(
							OBJECT * InObjectPtr,
							UCHAR * VarList,
							UINT32 NumVars,
							UINT32 MaxVarnameLength)
{
UCHAR * VarListPtr;
UINT32 VarNum,SubVarNum;
UINT32 i,j;
piMATRIX Matrix1,Matrix2;
piMATRIX CumulativeMatrix;
piVECTOR Vecx,Vecy,Vecz;
FLOAT32 Length;
UCHAR Buffer[32];
UCHAR * BufferPtr;
piVECTOR TempVec;
OBJECTSTATICDOF * DofStatic;
OBJECT * ObjectPtr;
LOGICAL dofValid;

	if (PN_UNLIKELY(InObjectPtr == nullptr))
	{
		return 1;
	}

	ObjectPtr = InObjectPtr;

	do
	{

		// set up DOF if this is a DOF bead
		if (ObjectPtr->flags & OBJECT_DOF)
		{
			ObjectPtr->dof->dofvarnum = 0;
			// parse comments string

			ObjectPtr->dof->dofmatrix1 = IdentityMatrix;
			ObjectPtr->dof->dofmatrix2 = IdentityMatrix;
			dofValid = 1;

			// first set up DOF matrices
			//

			Matrix1 = IdentityMatrix;

			// invert the origin movement
			Matrix1 = IdentityMatrix;
			Matrix1._41 = -ObjectPtr->dof->dofstatic->doforigin.x * ScaleFactor;
			Matrix1._42 = -ObjectPtr->dof->dofstatic->doforigin.y * ScaleFactor;
			Matrix1._43 = ObjectPtr->dof->dofstatic->doforigin.z * ScaleFactor;
			CumulativeMatrix = Matrix1;
	//		math3dMatrixMultiply(&NewMatrix,&Matrix1,&NewMatrix);

			// get x axis and normalize it
			Vecx.x = (ObjectPtr->dof->dofstatic->dofaxis.x - ObjectPtr->dof->dofstatic->doforigin.x);
			Vecx.y = (ObjectPtr->dof->dofstatic->dofaxis.y - ObjectPtr->dof->dofstatic->doforigin.y);
			Vecx.z = (ObjectPtr->dof->dofstatic->dofaxis.z - ObjectPtr->dof->dofstatic->doforigin.z);

			Length = (FLOAT32)sqrt(Vecx.x*Vecx.x + Vecx.y*Vecx.y + Vecx.z*Vecx.z);
			if(Length > 0.0001f)
			{
				Vecx.x /= Length;
				Vecx.y /= Length;
				Vecx.z /= Length;
			}else
				dofValid = 0;

			Vecy.x = ObjectPtr->dof->dofstatic->dofplane.x - ObjectPtr->dof->dofstatic->doforigin.x;
			Vecy.y = ObjectPtr->dof->dofstatic->dofplane.y - ObjectPtr->dof->dofstatic->doforigin.y;
			Vecy.z = ObjectPtr->dof->dofstatic->dofplane.z - ObjectPtr->dof->dofstatic->doforigin.z;

			// get perpendicular z axis vector
			Vecz.x = Vecx.y * Vecy.z - Vecy.y * Vecx.z;
			Vecz.y = -(Vecx.x * Vecy.z - Vecy.x * Vecx.z);
			Vecz.z = Vecx.x * Vecy.y - Vecy.x * Vecx.y;
			// normalize it
			Length = (FLOAT32)sqrt(Vecz.x * Vecz.x + Vecz.y * Vecz.y + Vecz.z * Vecz.z);
			if(Length > 0.0001f)
			{
				Vecz.x /= Length;
				Vecz.y /= Length;
				Vecz.z /= Length;
			}else
				dofValid = 0;

			// now get perpendicular y axis vector
			Vecy.x =  -(Vecx.y*Vecz.z - Vecz.y*Vecx.z);
			Vecy.y =  Vecx.x*Vecz.z - Vecz.x*Vecx.z;
			Vecy.z =  -(Vecx.x*Vecz.y - Vecz.x*Vecx.y);
			// normalize it
			Length = (FLOAT32)sqrt(Vecy.x*Vecy.x + Vecy.y*Vecy.y + Vecy.z*Vecy.z);
			if(Length > 0.0001f)
			{
				Vecy.x /= Length;
				Vecy.y /= Length;
				Vecy.z /= Length;
			}else
				dofValid = 0;

			if(dofValid)
			{
				// now make rotation matrix out of vectors
				Matrix1 = IdentityMatrix;
				Matrix1._11 = Vecx.x;
				Matrix1._12 = Vecx.y;
				Matrix1._13 = -Vecx.z;
				Matrix1._21 = Vecy.x;
				Matrix1._22 = Vecy.y;
				Matrix1._23 = -Vecy.z;
				Matrix1._31 = Vecz.x;
				Matrix1._32 = Vecz.y;
				Matrix1._33 = -Vecz.z;
				// we now have the matrix that rotates the IdentityMatrix matrix to the DOF's frame of reference

				math3dInvertMatrix(&Matrix2,&Matrix1);
				math3dMatrixMultiply(&CumulativeMatrix,&CumulativeMatrix,&Matrix2);

				ObjectPtr->dof->dofmatrix1 = CumulativeMatrix;

				CumulativeMatrix = Matrix1;

				Matrix1 = IdentityMatrix;
				Matrix1._41 = ObjectPtr->dof->dofstatic->doforigin.x * ScaleFactor;
				Matrix1._42 = ObjectPtr->dof->dofstatic->doforigin.y * ScaleFactor;
				Matrix1._43 = -ObjectPtr->dof->dofstatic->doforigin.z * ScaleFactor;

				math3dMatrixMultiply(&CumulativeMatrix,&CumulativeMatrix,&Matrix1);

				ObjectPtr->dof->dofmatrix2 = CumulativeMatrix;
			}else
			{
				WriteDebug("Invalid DOF data for object %s", ObjectPtr->name);
			}

			if (ObjectPtr->comment != nullptr)
			{
				// now parse dof parameters
				//

				sscanf(ObjectPtr->comment,"%s\n%f,%f,%f",
						_Buffer,
						&ObjectPtr->dof->dofminval,
						&ObjectPtr->dof->dofneutralval,
						&ObjectPtr->dof->dofmaxval);
				VarListPtr = VarList;
				VarNum = 0;

				for (i=0;i<NumVars;i++)
				{
					if (VarListPtr[0] == _Buffer[0])
					{
						if (strcmp((char*)VarListPtr,(char*)_Buffer) == 0)
						{
							VarNum = i;
							break;
						}
					}

					VarListPtr += MaxVarnameLength;
				}

				SubVarNum = 0;
				if (VarNum == 0)
				{
					// check for any names that have numbers attached to them
					// we do this separately in case the DOF varname has trailing numbers
					// anyway and we don't want to misparse it

					strcpy((char*)Buffer,(char*)_Buffer);
					BufferPtr = Buffer + strlen((char*)Buffer);
					if (BufferPtr != Buffer)
					{
						BufferPtr--;
						if (*BufferPtr >= '0' && *BufferPtr <= '9')
						{
							if (BufferPtr != Buffer)
							{
								BufferPtr--;
								if (*BufferPtr < '0' || *BufferPtr > '9')
								{
									BufferPtr++;
								}
							}

							sscanf((char*)BufferPtr,"%d",&SubVarNum);
							if (SubVarNum > 0)
							{
								SubVarNum--; // internally we're 0 based, in the flt file 1 based
							}

							*BufferPtr = '\0';

							VarListPtr = VarList;
							VarNum = 0;

							for (i=0;i<NumVars;i++)
							{
								if (VarListPtr[0] == Buffer[0])
								{
									if (strcmp((char*)VarListPtr,(char*)Buffer) == 0)
									{
										VarNum = i;
										break;
									}
								}

								VarListPtr += MaxVarnameLength;
							}
						}

					}
				}

				ObjectPtr->dof->dofvarnum = VarNum;
				ObjectPtr->dof->dofvarsubnum = SubVarNum;

			}
			// sanity check dof matrices
			for (i=0;i<4;i++)
			{
				for (j=0;j<4;j++)
				{
					if (_isnan(ObjectPtr->dof->dofmatrix1.m[i][j]) ||
						!_finite(ObjectPtr->dof->dofmatrix2.m[i][j]))
					{
						ObjectPtr->dof->dofmatrix1 = IdentityMatrix;
						ObjectPtr->dof->dofmatrix2 = IdentityMatrix;
						WriteDebug("Invalid DOF data for object %s, setting matrices to IdentityMatrix",
										ObjectPtr->name);
					}
				}
			}

			DofStatic = ObjectPtr->dof->dofstatic;
			mathSubVectors(&TempVec,&DofStatic->dofminrotate,&DofStatic->dofmaxrotate);
//			if (fabs(TempVec.x) > 0.001)
			if (TempVec.x > 0.001f || TempVec.x < -0.001f)
			{
				ObjectPtr->dof->dofflags |= DOF_ROTATES_X;
				ObjectPtr->dof->dofflags |= DOF_ONE_ROTATE_AXIS;
			}
//			if (fabs(TempVec.y) > 0.001)
			if (TempVec.y > 0.001f || TempVec.y < -0.001f)
			{
				ObjectPtr->dof->dofflags |= DOF_ROTATES_Y;
				ObjectPtr->dof->dofflags |= DOF_ONE_ROTATE_AXIS;
			}
//			if (fabs(TempVec.z) > 0.001)
			if (TempVec.z > 0.001f || TempVec.z < -0.001f)
			{
				ObjectPtr->dof->dofflags |= DOF_ROTATES_Z;
				ObjectPtr->dof->dofflags |= DOF_ONE_ROTATE_AXIS;
			}
			mathSubVectors(&TempVec,&DofStatic->dofminrotate,&DofStatic->dofcurrotate);
//			if (fabs(TempVec.x) > 0.001)
			if (TempVec.x > 0.001f || TempVec.x < -0.001f)
			{
				ObjectPtr->dof->dofflags |= DOF_ROTATES_X;
				ObjectPtr->dof->dofflags |= DOF_ONE_ROTATE_AXIS;
			}
//			if (fabs(TempVec.y) > 0.001)
			if (TempVec.y > 0.001f || TempVec.y < -0.001f)
			{
				ObjectPtr->dof->dofflags |= DOF_ROTATES_Y;
				ObjectPtr->dof->dofflags |= DOF_ONE_ROTATE_AXIS;
			}
//			if (fabs(TempVec.z) > 0.001)
			if (TempVec.z > 0.001f || TempVec.z < -0.001f)
			{
				ObjectPtr->dof->dofflags |= DOF_ROTATES_Z;
				ObjectPtr->dof->dofflags |= DOF_ONE_ROTATE_AXIS;
			}
			mathSubVectors(&TempVec,&DofStatic->dofmintrans,&DofStatic->dofmaxtrans);
//			if (fabs(TempVec.x) > 0.001)
			if (TempVec.x > 0.001f || TempVec.x < -0.001f)
			{
				ObjectPtr->dof->dofflags |= DOF_TRANSLATES_X;
				ObjectPtr->dof->dofflags |= DOF_ONE_TRANS_AXIS;
			}
//			if (fabs(TempVec.y) > 0.001)
			if (TempVec.y > 0.001f || TempVec.y < -0.001f)
			{
				ObjectPtr->dof->dofflags |= DOF_TRANSLATES_Y;
				ObjectPtr->dof->dofflags |= DOF_ONE_TRANS_AXIS;
			}
//			if (fabs(TempVec.z) > 0.001)
			if (TempVec.z > 0.001f || TempVec.z < -0.001f)
			{
				ObjectPtr->dof->dofflags |= DOF_TRANSLATES_Z;
				ObjectPtr->dof->dofflags |= DOF_ONE_TRANS_AXIS;
			}
			mathSubVectors(&TempVec,&DofStatic->dofmintrans,&DofStatic->dofcurtrans);
//			if (fabs(TempVec.x) > 0.001)
			if (TempVec.x > 0.001f || TempVec.x < -0.001f)
			{
				ObjectPtr->dof->dofflags |= DOF_ONE_TRANS_AXIS;
				ObjectPtr->dof->dofflags |= DOF_TRANSLATES_X;
			}
//			if (fabs(TempVec.y) > 0.001)
			if (TempVec.y > 0.001f || TempVec.y < -0.001f)
			{
				ObjectPtr->dof->dofflags |= DOF_ONE_TRANS_AXIS;
				ObjectPtr->dof->dofflags |= DOF_TRANSLATES_Y;
			}
//			if (fabs(TempVec.z) > 0.001)
			if (TempVec.z > 0.001f || TempVec.z < -0.001f)
			{
				ObjectPtr->dof->dofflags |= DOF_ONE_TRANS_AXIS;
				ObjectPtr->dof->dofflags |= DOF_TRANSLATES_Z;
			}

		}


		if (ObjectPtr->child != nullptr)
		{
			ogrpParseDOFParameters(ObjectPtr->child,VarList,NumVars,MaxVarnameLength);
		}

		ObjectPtr = ObjectPtr->sibling;
	}
	while (ObjectPtr != nullptr);


	return 0;
}


// takes the object and its subtree and tags them as "don't draw"
UINT32 ogrpSetObjectAndSubtreeToNotDraw(OBJECT * ObjectPtr)
{
OBJECT * Object;

	if (PN_UNLIKELY(ObjectPtr == nullptr))
	{
		return 0;
	}

	obmainSetFlagsInThisTree(ObjectPtr,OBJECT_SKIPDRAWING);
	obAdjustObjectTreeVisListStatus(ObjectPtr,0);

	return 0;

	ObjectPtr->flags |= OBJECT_SKIPDRAWING;

	if (ObjectPtr->child != nullptr)
	{
		_SetObjectAndSubtreeToNotDraw(ObjectPtr->child);
	}

	for (Object = ObjectPtr->attached;Object != nullptr;)
	{
		Object->flags |= OBJECT_SKIPDRAWING;

		Object = Object->attached;
	}

	return 0;

}

UINT32 _SetObjectAndSubtreeToNotDraw(OBJECT * ObjectPtr)
{
OBJECT * Object;

	if (PN_UNLIKELY(ObjectPtr == nullptr))
	{
		return 0;
	}

	ObjectPtr->flags |= OBJECT_SKIPDRAWING;

	if (ObjectPtr->child != nullptr)
	{
		_SetObjectAndSubtreeToNotDraw(ObjectPtr->child);
	}

	for (Object = ObjectPtr->attached;Object != nullptr;)
	{
		Object->flags |= OBJECT_SKIPDRAWING;

		Object = Object->attached;
	}

	if (ObjectPtr->sibling != nullptr)
	{
		_SetObjectAndSubtreeToNotDraw(ObjectPtr->sibling);
	}

	return 0;
}

// takes the object and its subtree and tags them as "do draw"
UINT32 ogrpSetObjectAndSubtreeToDraw(OBJECT * ObjectPtr)
{
OBJECT * Object;

	if (PN_UNLIKELY(ObjectPtr == nullptr))
	{
		return 0;
	}

	obmainClearFlagsInThisTree(ObjectPtr,OBJECT_SKIPDRAWING);
	obAdjustObjectTreeVisListStatus(ObjectPtr,0);

	return 0;

	ObjectPtr->flags &= ~OBJECT_SKIPDRAWING;

	if (ObjectPtr->child != nullptr)
	{
		_SetObjectAndSubtreeToDraw(ObjectPtr->child);
	}

	for (Object = ObjectPtr->attached;Object != nullptr;)
	{
		Object->flags &= ~OBJECT_SKIPDRAWING;

		Object = Object->attached;
	}

	return 0;


}

UINT32 _SetObjectAndSubtreeToDraw(OBJECT * ObjectPtr)
{
OBJECT * Object;

	if (PN_UNLIKELY(ObjectPtr == nullptr))
	{
		return 0;
	}

	ObjectPtr->flags &= ~OBJECT_SKIPDRAWING;

	if (ObjectPtr->child != nullptr)
	{
		_SetObjectAndSubtreeToDraw(ObjectPtr->child);
	}

	for (Object = ObjectPtr->attached;Object != nullptr;)
	{
		Object->flags &= ~OBJECT_SKIPDRAWING;

		Object = Object->attached;
	}

	if (ObjectPtr->sibling != nullptr)
	{
		_SetObjectAndSubtreeToDraw(ObjectPtr->sibling);
	}

	return 0;

}

// takes the object and its subtree and tags them as "don't draw"
UINT32 ogrpHideObjectAndSubtree(OBJECT * ObjectPtr)
{
OBJECT * Object;

	if (PN_UNLIKELY(ObjectPtr == nullptr))
	{
		return 0;
	}

	obmainClearFlagsInThisTree(ObjectPtr,OBJECT_VISIBLE);
	obAdjustObjectTreeVisListStatus(ObjectPtr,0);

	return 0;

	ObjectPtr->flags &= ~OBJECT_VISIBLE;

	if (ObjectPtr->child != nullptr)
	{
		_HideObjectAndSubtree(ObjectPtr->child);
	}

	for (Object = ObjectPtr->attached;Object != nullptr;)
	{
		Object->flags &= ~OBJECT_VISIBLE;

		Object = Object->attached;
	}

	return 0;

}

UINT32 _HideObjectAndSubtree(OBJECT * ObjectPtr)
{
OBJECT * Object;

	if (PN_UNLIKELY(ObjectPtr == nullptr))
	{
		return 0;
	}

	ObjectPtr->flags &= ~OBJECT_VISIBLE;

	if (ObjectPtr->child != nullptr)
	{
		_HideObjectAndSubtree(ObjectPtr->child);
	}

	for (Object = ObjectPtr->attached;Object != nullptr;)
	{
		Object->flags &= ~OBJECT_VISIBLE;

		Object = Object->attached;
	}

	if (ObjectPtr->sibling != nullptr)
	{
		_HideObjectAndSubtree(ObjectPtr->sibling);
	}

	return 0;
}

// takes the object and its subtree and tags them as "do draw"
UINT32 ogrpShowObjectAndSubtree(OBJECT * ObjectPtr)
{
OBJECT * Object;

	if (PN_UNLIKELY(ObjectPtr == nullptr))
	{
		return 0;
	}

	obmainSetFlagsInThisTree(ObjectPtr,OBJECT_VISIBLE);
	obAdjustObjectTreeVisListStatus(ObjectPtr,0);

	return 0;

	ObjectPtr->flags |= OBJECT_VISIBLE;

	if (ObjectPtr->child != nullptr)
	{
		_ShowObjectAndSubtree(ObjectPtr->child);
	}

	for (Object = ObjectPtr->attached;Object != nullptr;)
	{
		Object->flags |= OBJECT_VISIBLE;

		Object = Object->attached;
	}

	return 0;


}

UINT32 _ShowObjectAndSubtree(OBJECT * ObjectPtr)
{
OBJECT * Object;

	if (PN_UNLIKELY(ObjectPtr == nullptr))
	{
		return 0;
	}

	ObjectPtr->flags |= OBJECT_VISIBLE;

	if (ObjectPtr->child != nullptr)
	{
		_ShowObjectAndSubtree(ObjectPtr->child);
	}

	for (Object = ObjectPtr->attached;Object != nullptr;)
	{
		Object->flags |= OBJECT_VISIBLE;

		Object = Object->attached;
	}

	if (ObjectPtr->sibling != nullptr)
	{
		_ShowObjectAndSubtree(ObjectPtr->sibling);
	}

	return 0;

}

UINT32 ogrpFindSubObjectByName(OBJECT ** ReturnObject,OBJECT * ObjectPtr, const char * Name)
{
	if (PN_UNLIKELY(ObjectPtr == nullptr))
	{
		return 0;
	}

	if (PN_UNLIKELY(ReturnObject == nullptr))
	{
		return 0;
	}

	*ReturnObject = nullptr;

//	if (strncmp(Name,ObjectPtr->name,strlen(Name)) == 0 &&
//		strlen(Name) == strlen(ObjectPtr->name))
	if (ObjectPtr != nullptr)
	{
		if (Name[0] == ObjectPtr->name[0])
		{
			if (strcmp(Name,ObjectPtr->name) == 0)
			{
				*ReturnObject = ObjectPtr;

				return 0;
			}
		}
	}

	if (ObjectPtr->child != nullptr)
	{
		_FindSubObjectByName(ReturnObject,ObjectPtr->child,Name);
		if ((*ReturnObject) != nullptr)
		{
			return 0;
		}
	}

//	if (ObjectPtr->attached != nullptr)
//	{
//		_FindSubObjectByName(ReturnObject,ObjectPtr->child,Name);
//		if ((*ReturnObject) != nullptr)
//		{
//			return 0;
//		}
//	}

	return 0;
}

UINT32 _FindSubObjectByName(OBJECT ** ReturnObject,OBJECT * ObjectPtr, const char * Name)
{
OBJECT * ObjSib;

	if (PN_UNLIKELY(ObjectPtr == nullptr))
	{
		return 0;
	}

	if (PN_UNLIKELY(ReturnObject == nullptr))
	{
		return 0;
	}

	*ReturnObject = nullptr;

	ObjSib = ObjectPtr;

	do
	{
		if (ObjSib != nullptr)
		{
			if (Name[0] == ObjSib->name[0])
			{
				if (strcmp(Name,ObjSib->name) == 0)
				{
					*ReturnObject = ObjSib;

					return 0;
				}
			}
		}

		if (ObjSib->child != nullptr)
		{
			_FindSubObjectByName(ReturnObject,ObjSib->child,Name);
			if ((*ReturnObject) != nullptr)
			{
				return 0;
			}
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);


	return 0;
}

UINT32 ogrpFindSubObjectByDOFVarAndSubVar(OBJECT ** ReturnObject,OBJECT * ObjectPtr,UINT32 VarNum,UINT32 SubVarNum)
{
	if (PN_UNLIKELY(ObjectPtr == nullptr))
	{
		return 0;
	}

	if (PN_UNLIKELY(ReturnObject == nullptr))
	{
		return 0;
	}

	*ReturnObject = nullptr;

	if (ObjectPtr->flags & OBJECT_DOF)
	{
		if (ObjectPtr->dof->dofvarnum == VarNum && ObjectPtr->dof->dofvarsubnum == SubVarNum)
		{
			*ReturnObject = ObjectPtr;

			return 0;
		}
	}

	if (ObjectPtr->child != nullptr)
	{
		_FindSubObjectByDOFVarAndSubVar(ReturnObject,ObjectPtr->child,VarNum,SubVarNum);
		if ((*ReturnObject) != nullptr)
		{
			return 0;
		}
	}

	return 0;

}

UINT32 _FindSubObjectByDOFVarAndSubVar(OBJECT ** ReturnObject,OBJECT * ObjectPtr,UINT32 VarNum,UINT32 SubVarNum)
{
OBJECT* ObjSib;

	if (PN_UNLIKELY(ObjectPtr == nullptr))
	{
		return 0;
	}

	if (PN_UNLIKELY(ReturnObject == nullptr))
	{
		return 0;
	}

	ObjSib = ObjectPtr;

	do
	{
		*ReturnObject = nullptr;

		if (ObjSib->flags & OBJECT_DOF)
		{
			if (ObjSib->dof->dofvarnum == VarNum && ObjSib->dof->dofvarsubnum == SubVarNum)
			{
				*ReturnObject = ObjSib;

				return 0;
			}
		}

		if (ObjSib->child != nullptr)
		{
			_FindSubObjectByDOFVarAndSubVar(ReturnObject,ObjSib->child,VarNum,SubVarNum);
			if ((*ReturnObject) != nullptr)
			{
				return 0;
			}
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);


	return 0;
}


UINT32 ogrpFindSubObjectByDOFVar(OBJECT ** ReturnObject,OBJECT * ObjectPtr,UINT32 VarNum)
{
	if (PN_UNLIKELY(ObjectPtr == nullptr))
	{
		return 0;
	}

	if (PN_UNLIKELY(ReturnObject == nullptr))
	{
		return 0;
	}

	*ReturnObject = nullptr;

	if (ObjectPtr->flags & OBJECT_DOF)
	{
		if (ObjectPtr->dof->dofvarnum == VarNum)
		{
			*ReturnObject = ObjectPtr;

			return 0;
		}
	}

	if (ObjectPtr->child != nullptr)
	{
		_FindSubObjectByDOFVar(ReturnObject,ObjectPtr->child,VarNum);
		if ((*ReturnObject) != nullptr)
		{
			return 0;
		}
	}

//	if (ObjectPtr->attached != nullptr)
//	{
//		_FindSubObjectByDOFVar(ReturnObject,ObjectPtr->child,VarNum);
//		if ((*ReturnObject) != nullptr)
//		{
//			return 0;
//		}
//	}


	return 0;
}

UINT32 _FindSubObjectByDOFVar(OBJECT ** ReturnObject,OBJECT * ObjectPtr,UINT32 VarNum)
{
OBJECT * ObjSib;

	if (PN_UNLIKELY(ObjectPtr == nullptr))
	{
		return 0;
	}

	if (PN_UNLIKELY(ReturnObject == nullptr))
	{
		return 0;
	}

	ObjSib = ObjectPtr;

	do
	{

		*ReturnObject = nullptr;

		if (ObjSib->flags & OBJECT_DOF)
		{
			if (ObjSib->dof->dofvarnum == VarNum)
			{
				*ReturnObject = ObjSib;

				return 0;
			}
		}

		if (ObjSib->child != nullptr)
		{
			_FindSubObjectByDOFVar(ReturnObject,ObjSib->child,VarNum);
			if ((*ReturnObject) != nullptr)
			{
				return 0;
			}
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);


	return 0;
}

UINT32 ogrpFreeObjectTree(OBJECT * Object)
{
    obpoolFreeObjectCallback(Object);
	return 0;
}

UINT32 ogrpMakeObjectAChildOfAnotherObject(OBJECT * Parent,OBJECT * NewChild)
{
OBJECT * Object;

	if (Parent == nullptr || NewChild == nullptr)
	{
		return 0;
	}

	if (Parent->child == nullptr)
	{
		Parent->child = NewChild;
		return 0;
	}

	Object = Parent->child;

	while (Object->sibling != nullptr)
	{
		Object = Object->sibling;
	}

	Object->sibling = NewChild;


	return 0;
}

UINT32 ogrpSetObjectAndSubtreeAsCockpit(OBJECT * Object)
{
	Object->flag2 |= OBJECT_ISCOCKPIT;

	_SetObjectAndTreeAsCockpit(Object->child);

	return 0;
}

static UINT32 _SetObjectAndTreeAsCockpit(OBJECT * Object)
{
OBJECT * ObjAtt,* ObjSib;
//UINT32 i;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjSib = Object;

	do
	{
		ObjAtt = ObjSib;

		do
		{
			ObjAtt->flag2 |= OBJECT_ISCOCKPIT;
/*
			if (ObjAtt->flags & OBJECT_DOF)
			{
				ObjAtt->dof->dofmatrix1._41 *= (FLOAT32)_COCKPIT_SCALEFACTOR;
				ObjAtt->dof->dofmatrix1._42 *= (FLOAT32)_COCKPIT_SCALEFACTOR;
				ObjAtt->dof->dofmatrix1._43 *= (FLOAT32)_COCKPIT_SCALEFACTOR;
				ObjAtt->dof->dofmatrix2._41 *= (FLOAT32)_COCKPIT_SCALEFACTOR;
				ObjAtt->dof->dofmatrix2._42 *= (FLOAT32)_COCKPIT_SCALEFACTOR;
				ObjAtt->dof->dofmatrix2._43 *= (FLOAT32)_COCKPIT_SCALEFACTOR;
			}

			for (i=0;i<ObjAtt->numverts;i++)
			{
				ObjAtt->pdlverts[i].x = ObjAtt->pslverts[i].x * (FLOAT32)_COCKPIT_SCALEFACTOR;
				ObjAtt->pdlverts[i].y = ObjAtt->pslverts[i].y * (FLOAT32)_COCKPIT_SCALEFACTOR;
				ObjAtt->pdlverts[i].z = ObjAtt->pslverts[i].z * (FLOAT32)_COCKPIT_SCALEFACTOR;

			}
*/
			ObjAtt = ObjAtt->attached;
		}
		while (ObjAtt != nullptr);


		if (ObjSib->child != nullptr)
		{
			_SetObjectAndTreeAsCockpit(ObjSib->child);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);

	return 0;
}

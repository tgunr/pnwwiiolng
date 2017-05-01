#include <syscfg.h> // Path is project specific.

#include <math.h>

#include "weap/weapfx.h"
#include "fx/fxMgr.h"
#include "fx/fxHit.h"
#include "fx/fxExp.h"
#include "fx/fxBlt.h"
#include "fx/fxPcl.h"
#include "fx/fxSnd.h"
#include "fx/FXInterface.h"
#include "math/mathvect.h"
#include "math/mathnum.h"
#include "math/matheul.h"
#include "math/mathUnitConversion.h"
#include "math/math3d.h"
#include "ml/mlAtmo.h"
#include "trn/trnmain.h"
#include "veh/vehmain.h"
#include "include/vars.h"
#include "pers/persmain.h"
#include "gr2/gr2Anim.h"
#include "ob/obpool.h"
#include "ogrp/ogrptype.h"
#include "fx/fxUtl.h"
#include "pers/persmain.h"
#include "fp/fpFPS.h"
#include "fx/stoeffects.h"
#include "client/GameClient.h"
#include "spt/Random.h"
#include "ob/obMgr.h"
#include "fx/fxLgt.h"
#include "trn/trnwtr.h"
#include "trn/trnvars.h"
#include "ob/obmain.h"

#include <deque>

#define MAX_DEBRIS_HITS     128
#define MAX_MUZZLE_FLASH    128

static UINT32 AddBulletDecal(weapAmmo const* Ammo,
							 FASTCOLLIDE const* collide,
							 piVECTOR const* dir,
							 void* vehicle,
							 SINT32 penetration,
							 bool usehiteffect = true);
static UINT32 weapfxPlayExpSound(weapAmmo const* Ammo, FLOAT64VECTOR const* Pos);
static UINT32 weapfxAmmoHitMark(weapAmmo const* Ammo,
								piVECTOR const* pvecCollidePt,
								piVECTOR const* pvecNormal,
								OBJECT* lpCollideObj,
								OBJECT* lpTopLevelObj,
								void* pVehicle,
								SINT32 iPenetration);
static UINT32 weapfxExplosion(weapAmmo const* pAmmo,
					   FLOAT64VECTOR const* pPos,
					   piVECTOR const* pNormal,
					   bool bIsWater);
static UINT32 weapfxAmmoExpHitAttached(weapAmmo const* Ammo, SINT32 iPenetration);

static UINT32 _CreateMuzzleFlash(weapAmmo const* pAmmo, OBJECT* o, const FLOAT64VECTOR* pos);
static SINT32 _CreateBigGunFlash(const weapAmmo* pAmmo, OBJECT* o, const FLOAT64VECTOR* pPos);
static SINT32 _CreateTankGunFlash(const weapAmmo* pAmmo, OBJECT* o, const FLOAT64VECTOR* pPos);
static UINT32 _CreateDebris(weapAmmo const* pAmmo,
							FASTCOLLIDE const* pFast,
							piVECTOR const* pvecShot,
							void* pVehicle,
							SINT32 iPenetration);
static SINT32 _Attach3DBombSound(weapAmmo* pAmmo);
static bool _InitObjectParams(OBJECT* o, piVECTOR& dir, float scale, FLOAT64VECTOR& pos);

static OBJECT* _GetFreeFlash();
static OBJECT* _GetFreeDebris();

static void ReleaseFlash(OBJECT* o);

static bool _IsWater(const OBJECT* lpObj);
static bool _InitFlashObjs();

constexpr FLOAT32 PiOver180 = PI / 180.0f;

static OBJECT* _debris[MAX_DEBRIS_HITS];
static std::deque<OBJECT*> _freedebris;

static OBJECT* _muzzelflash[MAX_MUZZLE_FLASH];
static std::deque<OBJECT*> _freeflash;

void DoHitSound(FLOAT64VECTOR* Pos, FLOAT32 Energy,	const char* TextureName)
{
	const char* HitSoundName;
	FLOAT32 MinVolume;
	FLOAT32 MaxVolumeEnergy;
	FLOAT32 Volume;

	HitSoundName = nullptr;
	mlGetRandomHitSound(&HitSoundName,
						&MinVolume,
						&MaxVolumeEnergy,
						TextureName);

	if (HitSoundName == nullptr)
	{
		return;
	}

	if (Energy >= MaxVolumeEnergy)
	{
		Volume = 1.0f;
	}
	else
	{
		if (MaxVolumeEnergy == 0.0f)
		{
			Volume = 1.0f;
		}
		else
		{
			Volume = (1.0f - MinVolume) * Energy / MaxVolumeEnergy + MinVolume;
		}
	}

	if(Volume >= 0.01f)
	{
		SINT32 id = fxSndGetNewSound();

		if(id > 0)
		{
			fxSndSetSoundName(id, HitSoundName);
			fxSndSetPosition(id, Pos->x, Pos->y, Pos->z);
			fxSndSetPriority(id, IDE_TP_PRIORITY_LOW);
			fxSndSetSoundType(id, TYPE_FINITE_EXTERNAL_SOUND);
			fxSndSetVolume(id, Volume);
			fxSndSetPitch(id, 0.8f + 0.2f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f));
			fxSndSetPlayType(id, DEF_PLAYTYPE_WORLD);
			fxSndSetMaxDistance(id, 50.0f);
			fxSndSetMinDistance(id, 10.0f);

			fxSndInitSound(id);
		}
	}
}

/************************************************************************************************
_InitDebrisObjs
*************************************************************************************************/
static bool _InitDebrisObjs()
{
	SINT32 i;

	OBJECT* o;

	for (i = 0; i < MAX_DEBRIS_HITS; i++)
	{
		if (!obmgrCreateModelInstance("debris2"))
			return false;

		o = obmainFindNamedObject("debris2");

		if (!o)
		{
			return false;
		}

		// Draw double sided (if flat polygon)
		obmainSetFlagsInThisTree(o->child, OBJECT_NOBACKFACE);

		// Remove this object from the standard rendering system.
		obmainMoveToNotObjectTree(o->child);

		_debris[i] = o;

		_freedebris.push_back(o->child);
	}

	return true;
}

/************************************************************************************************
	weapfxInit
*************************************************************************************************/
LOGICAL weapfxInit()
{
 logTimerBeginMacro();

	if(!_InitDebrisObjs())
		return FALSE;

	if(!_InitFlashObjs())
		return FALSE;

 logTimerEndMacro();

	return TRUE;
}

#if USE_STATESET
struct AddBlendFunc
{
	OBJECT* o;
	BlendFunc* f;

	void Apply(OBJECT* o)
	{
		if(o)
		{
			if(!(o->flags & OBJECT_NOTOBJECT) && o->numverts > 0)
			{
				o->material->SetRenderState(f);
			}

			Apply(o->child);
			Apply(o->sibling);
			Apply(o->attached);
		}
	}

	void Add()
	{
		f = new BlendFunc;
		f->SetDstRGB(GL_ONE);

		Apply(o);
	}
};

bool _InitFlashObjs()
{
	static bool first = true;

	for(int i = 0; i < MAX_MUZZLE_FLASH; i++)
	{
		if(!obmgrCreateModelInstance("muzzleflash"))
			return false;

		OBJECT* o = obmainFindNamedObject("muzzleflash");

		if(first)
		{
			first = false;
			AddBlendFunc abf;
			abf.o = o;
			abf.Add();
		}

		_muzzelflash[i] = o;

		if(!_muzzelflash[i])
		{
			return false;
		}

		obmainSetFlagsInThisTree(o, OBJECT_ZWRITEOFF);
		ReleaseFlash(o->child);
	}

	return true;
}
#else
/************************************************************************************************
	_InitFlashObjs
*************************************************************************************************/
bool _InitFlashObjs()
{
	for(int i = 0; i < MAX_MUZZLE_FLASH; i++)
	{
		if(!obmgrCreateModelInstance("muzzleflash"))
			return false;

		OBJECT* o = obmainFindNamedObject("muzzleflash");

		_muzzelflash[i] = o;

		if(!_muzzelflash[i])
		{
			return FALSE;
		}

		obmainSetFlagsInThisTree(o, OBJECT_ZWRITEOFF);
		ReleaseFlash(o->child);
	}

	return true;
}
#endif

void _updateVehicleFlash(OBJECT* o, vehVehicle* veh)
{
	if(veh == nullptr) return;
	if(!veh->InUse) return;

	if(veh->VehicleObjectData == nullptr) return;
	if(veh->VehicleObjectData->PlaneObject == nullptr) return;

	piVECTOR dir;
	FLOAT64VECTOR gunpos;

	if(o->dof)
	{
		// Get current direction of flash so drawing does not appear a frame behind.
		fxUtlGetDOFDirection(o->dof, &o->shadow->mat, &dir);
		fxUtlGetDOFPos(o->dof, &o->shadow->mat, &gunpos);

		piMATRIX m = IdentityMatrix;

		// Create matrix to orient object to gun direction
		mathVectorToYAxis(&dir, &m);

		piMATRIX scale = IdentityMatrix;

		o->vel.set(dir.x, dir.y, dir.z);

		// Scale values saved in rot.x
		scale._11 = o->rot.x;
		scale._22 = o->rot.x;
		scale._33 = o->rot.x;

		// Scale orientation matrix
		math3dMatrixMultiply(&m, &m, &scale);

#if(1)
		// Random rotate around y
		piMATRIX roty = IdentityMatrix;

		roty._11 = cosf(o->rot.y);
		roty._13 = sinf(o->rot.y);
		roty._31 = -roty._13;
		roty._33 = roty._11;

		math3dMatrixMultiply(&m, &m, &roty);
#endif

		math3dInvertMatrix(&m, &m);

		// Translate to gun position.
		m._41 = float(gunpos.x - WorldOriginOffset.x);
		m._42 = float(gunpos.y - WorldOriginOffset.y);
		m._43 = float(-gunpos.z);

		// Apply matrix to object tree
		fxUtlSetMatrixInTree(o, &m);
	}
	else
	{
		piMATRIX m = IdentityMatrix;

		// Create matrix to orient object to gun direction
		mathVectorToYAxis(&o->vel, &m);

		piMATRIX scale = IdentityMatrix;

		// Scale values saved in rot.x
		scale._11 = o->rot.x;
		scale._22 = o->rot.x;
		scale._33 = o->rot.x;

		// Scale orientation matrix
		math3dMatrixMultiply(&m, &m, &scale);

#if(1)
		// Random rotate around y
		piMATRIX roty = IdentityMatrix;

		roty._11 = cosf(o->rot.y);
		roty._13 = sinf(o->rot.y);
		roty._31 = -roty._13;
		roty._33 = roty._11;

		math3dMatrixMultiply(&m, &m, &roty);
#endif

		math3dInvertMatrix(&m, &m);

		// Translate to gun position.
		m._41 = o->mat._41;
		m._42 = o->mat._42;
		m._43 = o->mat._43;

		// Apply matrix to object tree
		fxUtlSetMatrixInTree(o, &m);
	}
}

void _updateTroopFlash(OBJECT* o, vehVehicle* veh)
{
	piMATRIX firept = IdentityMatrix;

	if(veh == nullptr) return;
	if(!veh->InUse) return;

	if(veh->VehicleObjectData == nullptr) return;
	if(veh->VehicleObjectData->PlaneObject == nullptr) return;

	if(o->feature) // Using this variable to declare ghost rounds
	{
		gr2AnimGetFirePointMatrix(veh->VehicleObjectData->PlaneObject, &firept, true);
	}
	else
	{
		fpFPSGetFirePoint(veh->VehicleObjectData->PlaneObject, &firept, true);
	}

	FLOAT64VECTOR gunpos, player;

	// Get position at end of gun
	gunpos.x = firept._41;
	gunpos.y = firept._42;
	gunpos.z = firept._43;

	piMATRIX fireatt = IdentityMatrix;

	// Get player attitude and position
	if(veh->ModelData == nullptr)
	{
		fireatt = veh->PosData.AttMat;
		player = veh->PosData.Pos;
	}
	else
	{
		fireatt = veh->ModelData->VehicleAttMat;
		player = veh->ModelData->VehiclePos;
	}

	// Orient fire point to player attitude
	mathVectorMultiply64(&gunpos, &gunpos, &fireatt);
	// Transform to player pos
	mathAddVectors(&gunpos, &gunpos, &player);

	math3dMatrixMultiply(&firept, &fireatt, &firept);

	piVECTOR dir;
	// x and z reversed (whatever works)
	dir.set(-firept._12, firept._22, -firept._32);

	piMATRIX m = IdentityMatrix;

	// Create matrix to orient object to gun direction
	mathVectorToYAxis(&dir, &m);

	// Scale (values saved in rot.x)
	piMATRIX scale = IdentityMatrix;

	scale._11 = o->rot.x;
	scale._22 = o->rot.x;
	scale._33 = o->rot.x;

	// Scale orientation matrix
	math3dMatrixMultiply(&m, &m, &scale);

#if(1)
	// Random rotate around y
	piMATRIX roty = IdentityMatrix;

	roty._11 = cosf(o->rot.y);
	roty._13 = sinf(o->rot.y);
	roty._31 = -roty._13;
	roty._33 = roty._11;

	math3dMatrixMultiply(&m, &m, &roty);
#endif

	// Invert it so we can move object space flash to gun direction
	math3dInvertMatrix(&m, &m);

	// Translate to gun position.
	m._41 = float(gunpos.x - WorldOriginOffset.x);
	m._42 = float(gunpos.y - WorldOriginOffset.y);
	m._43 = float(-gunpos.z);

	// Apply matrix to object tree
	fxUtlSetMatrixInTree(o, &m);
}

#if(0)
static bool _updateMuzzleFlash(OBJECT* o)
{
	vehVehicle*veh = (vehVehicle*)o->lpspare;

	if(!veh)
		return false;

	if(!veh->InUse)
		return false;

	if(veh->VehicleType->Type == VEH_TYPE_PERSON)
	{
		_updateTroopFlash(o, veh);
	}
	else
	{
		_updateVehicleFlash(o, veh);
	}

	return true;
}
#endif

#include "ogl/oglMesh.h"
static bool _drawMuzzleFlash(OBJECT* o)
{
	if(!o->material)
		return true;

#if USE_STATESET
#else
	oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, 0);
	oglSetRenderState(oglRENDERSTATE_DESTBLEND, piBLEND_ONE);
#endif

	OBJECT* a = o;

	o->material->Apply();

	while(a)
	{
		oglSetWorldMatrix(&a->mat);

		if(a->meshContext && (a->flag2 & OBJECT_USEBUFFERS))
		{
			a->meshContext->Draw();
		}
		else if(a->flags & OBJECT_INDEXED)
		{
			piSTRIDEDDATA sd;
			a->StridedRenderData(&sd);

			piRenderIndexStridedScene(piPT_TRIANGLELIST,
									  piVT_VERTEX,
									  &sd,
									  a->numverts,
									  (UINT16*)a->ptris,
									  a->numtris * 3);
		}
		else
		{
			piSTRIDEDDATA sd;
			a->StridedRenderData(&sd);

			piRenderStridedScene(piPT_TRIANGLELIST,
								 piVT_VERTEX,
								 &sd,
								 a->numverts);
		}

		a = a->attached;
	}

#if USE_STATESET
#else
	oglSetRenderState(oglRENDERSTATE_DESTBLEND, DestBlend);
	oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, bZWrite);
#endif

	return true;
}

static bool _updateDebris(OBJECT* o)
{
	// Only display for 5 seconds
	if(frametime - o->createtime > 5000.0f)
	{
		o->createtime = 0;
		o->updatecallback = 0;
		o->updatecallback = nullptr; // Removes from obmainUpdate
		fxUtlSetMatrixInTree(o, &IdentityMatrix);
		obmainMoveToNotObjectTree(o);

		_freedebris.push_back(o);

		return true;
	}

	// Tree id used for time update
	int timediff = frametime - o->updatetime;
	float time = (float)timediff * 0.001f;
	static float drag = 0.0f;

	fxUtlApplyDrag(&o->vel, &drag, (float*)(&o->updatetime));

	o->updatetime = frametime;

	piMATRIX matRotX = IdentityMatrix;
	piMATRIX matRotY = IdentityMatrix;
	piMATRIX matRotZ = IdentityMatrix;
	piMATRIX matObj = IdentityMatrix;

	matObj = o->mat;

	// Translate to origin
	matObj._41 = 0.0f;
	matObj._42 = 0.0f;
	matObj._43 = 0.0f;

	// Rotate around X
	matRotX._22 = (FLOAT32)cos(o->rot.x);
	matRotX._23 = (FLOAT32)sin(o->rot.x);
	matRotX._32 = -matRotX._23;
	matRotX._33 =  matRotX._22;

	math3dMatrixMultiply(&matObj, &matObj, &matRotX);

	// Rotate around Y
	matRotY._11 = (FLOAT32)cos(o->rot.y);
	matRotY._13 = (FLOAT32)-sin(o->rot.y);
	matRotY._31 = -matRotY._13;
	matRotY._33 =  matRotY._11;

	math3dMatrixMultiply(&matObj, &matObj, &matRotY);

	// Rotate around Z
	matRotZ._11 = (FLOAT32)cos(o->rot.z);
	matRotZ._12 = (FLOAT32)sin(o->rot.z);
	matRotZ._21 = -matRotZ._12;
	matRotZ._22 =  matRotZ._11;

	math3dMatrixMultiply(&matObj, &matObj, &matRotZ);

	matObj._41 = o->mat._41 + time * o->vel.x;
	matObj._42 = o->mat._42 + time * o->vel.y;
	matObj._43 = o->mat._43 + time * (9.8f + o->vel.z);

	fxUtlSetMatrixInTree(o, &matObj);

	return true;
}

static bool _InitObjectParams(OBJECT* o, piVECTOR& dir, float scale, FLOAT64VECTOR& pos)
{
	piMATRIX matOrient = IdentityMatrix;

	float dot = math3dDotProduct(&dir, &dir);

	if(dot > -0.0001f && dot < 0.0001f)
	{
		return false;
	}

	if(_isnan(dot) || !_finite(dot))
	{
		return false;
	}

	piVECTOR vecDir = dir;

	vecDir.z = -vecDir.z;

	mathVectorToYAxis(&vecDir, &matOrient);
	math3dInvertMatrix(&matOrient, &matOrient);

	piMATRIX matScale = IdentityMatrix;

	// In case user has rotated object prior to this call
	piMATRIX matObj = IdentityMatrix;

	if(o)
	{
		matObj = o->mat;
	}

	matObj._41 = 0.0f;
	matObj._42 = 0.0f;
	matObj._43 = 0.0f;

	matScale._11 = scale;
	matScale._22 = scale;
	matScale._33 = scale;

	math3dMatrixMultiply(&matOrient, &matOrient, &matScale);
	math3dMatrixMultiply(&matObj, &matObj, &matOrient);

	matObj._41 = float(pos.x - WorldOriginOffset.x);
	matObj._42 = float(pos.y - WorldOriginOffset.y);
	matObj._43 = float(-pos.z);

	if(o)
	{
		fxUtlSetMatrixInTree(o, &matObj);
	}

	return true;
}

/************************************************************************************************
	weapfxOkForDebris
*************************************************************************************************/
LOGICAL weapfxOkForDebris(weapAmmo const* pAmmo, void* pVehicle)
{
	vehVehicle*pVeh = (vehVehicle*)pVehicle;

	// Currently only supported for aircraft hits.
	if(pVeh == nullptr) return FALSE;
	if(pVeh->VehicleType == nullptr) return FALSE;

	// Valid for sea and air for now
	switch(pVeh->VehicleType->Type)
	{
	case VEH_TYPE_AIR:
	case VEH_TYPE_SEA:
		{
			// Do nothing;
			break;
		}
	default:
		{
			return FALSE;
		}
	}

	// Bullets get a random hit
	if(pAmmo->AmmoType->SubType == WEAPAMMO_SUBTYPE_BALLISTIC)
	{
		if(abs(mathGetChaoticNumber() % 4) == 2)
			return TRUE;
	}

	// Explosives always throw debris
	if(pAmmo->AmmoType->SubType == WEAPAMMO_SUBTYPE_AIRBURST)
		return TRUE;

	return FALSE;
}

/************************************************************************************************
	_CreateDebris
*************************************************************************************************/
UINT32 _CreateDebris(weapAmmo const* pAmmo,
					 FASTCOLLIDE const* pFast,
					 piVECTOR const* pvecShot,
					 void* pVehicle,
					 SINT32 iPenetration)
{
	piVECTOR u = {0.0f, 1.0f, 0.0f};
	piVECTOR vecVel;
	FLOAT64VECTOR vecPos;

	// Only display debris is there has been damage.
	if(!iPenetration)
		return 2;

	// Only supported for airplane hits for now.
	if(!weapfxOkForDebris(pAmmo, pVehicle))
		return 1;

	OBJECT* o = _GetFreeDebris();

	if(!o)
	{
		return 1;
	}

	o->updatecallback = _updateDebris;
	obmainAddForUpdateCallback(o);

	// Add to normal render system for proper sorting
	obmainMoveToObjectTree(o);

	o->createtime = frametime; // Using this for create time
	o->updatetime = frametime; // Using this for update time

	weapfxGetObjectPosition(pFast, &vecPos);

	if(pFast->CollideObject && pFast->CollideObject->psnorms)
	{
		u = pFast->CollideObject->psnorms[pFast->PolyNum];

		math3dVectorMultiply(&u, &u, &pFast->CollideObject->mat);

		u.x -= pFast->CollideObject->mat.m[3][0];
		u.y -= pFast->CollideObject->mat.m[3][1];
		u.z -= pFast->CollideObject->mat.m[3][2];

		u.x += mathGetFloat32ChaoticNumber() * 2.0f - 1.0f;
		u.y += mathGetFloat32ChaoticNumber() * 2.0f - 1.0f;
		u.z += mathGetFloat32ChaoticNumber() * 2.0f - 1.0f;
	}

	// Eject from normal slightly
	weapfxGetVehicleVelocity(pVehicle, &vecVel);

	vecVel.x += u.x * 5.0f;
	vecVel.y += u.y * 5.0f;
	vecVel.z += u.z * 5.0f;

	float scale =  0.1f + mathGetFloat32ChaoticNumber() * 0.75f;

	// Need random rotation.
	o->rot.x = (float)(mathGetChaoticNumber() % 10) * PiOver180;
	o->rot.y = (float)(mathGetChaoticNumber() % 10) * PiOver180;
	o->rot.z = (float)(mathGetChaoticNumber() % 10) * PiOver180;

	o->vel = vecVel;

	_InitObjectParams(o, u, scale, vecPos);

	return TRUE;
}

/************************************************************************************************
	weapfxGetVehicleVelocity
*************************************************************************************************/
void weapfxGetVehicleVelocity(void* pVehicle, piVECTOR* lpVel)
{
	vehVehicle*pVeh = (vehVehicle*)pVehicle;

	if(pVeh->ModelData)
	{
		lpVel->x =  (FLOAT32)pVeh->ModelData->VehicleVel.x;
		lpVel->y = -(FLOAT32)pVeh->ModelData->VehicleVel.y;
		lpVel->z =  (FLOAT32)pVeh->ModelData->VehicleVel.z;
	}
	else
	{
		lpVel->x =  (FLOAT32)pVeh->PosData.Vel.x;
		lpVel->y = -(FLOAT32)pVeh->PosData.Vel.y;
		lpVel->z =  (FLOAT32)pVeh->PosData.Vel.z;
	}
}

/************************************************************************************************
	weapfxGetObjectPosition
*************************************************************************************************/
void weapfxGetObjectPosition(FASTCOLLIDE const* pFast, FLOAT64VECTOR* pPos)
{
	pPos->x = pFast->CollidePnt.x;
	pPos->y = pFast->CollidePnt.y;
	pPos->z = pFast->CollidePnt.z;

	if(pFast->CollideObject)
	{
		mathVectorMultiply64(pPos, pPos, &pFast->CollideObject->mat);
	}

	pPos->z = -pPos->z;

	pPos->x += WorldOriginOffset.x;
	pPos->y += WorldOriginOffset.y;
	pPos->z += WorldOriginOffset.z;
}

/************************************************************************************************
	weapfxFlashKillCallback
*************************************************************************************************/
void weapfxFlashKillCallback(void* data)
{
	OBJECT* o = (OBJECT*)data;

	if(o == nullptr)
	{
		return;
	}

	fxUtlSetMatrixInTree(o, &IdentityMatrix);
	ReleaseFlash(o);
}

/************************************************************************************************
	_GetFreeDebris
*************************************************************************************************/
OBJECT* _GetFreeDebris()
{
	if(_freedebris.empty())
	{
		return nullptr;
	}

	OBJECT* o = _freedebris.front();
	_freedebris.pop_front();

	return o;
}

static void ReleaseFlash(OBJECT* o)
{
	o->dof = nullptr;
	o->lpspare = nullptr;
	o->rot.x = 0;
	o->feature = 0;
	o->shadow = nullptr;
	o->killcallback = nullptr;
	o->updatecallback = nullptr;

	_freeflash.push_back(o);
	obmainMoveToNotObjectTree(o);
}

/************************************************************************************************
	_GetFreeFlash
*************************************************************************************************/
OBJECT* _GetFreeFlash()
{
	if(_freeflash.empty())
	{
		return nullptr;
	}

	OBJECT* o = _freeflash.front();
	_freeflash.pop_front();

	o->dof = nullptr;
	o->lpspare = nullptr;
	o->rot.x = 0;
	o->feature = 0;
	o->shadow = nullptr;

	return o;
}

/************************************************************************************************
	weapfxKill
*************************************************************************************************/
void weapfxKill()
{
	SINT32 i;

	for(i = 0; i < MAX_DEBRIS_HITS; i++)
	{
		if(obmainValidateObject(_debris[i]))
		{
			ogrpFreeObjectTree(_debris[i]);
		}
	}

	_freedebris.clear();

	for(i = 0; i < MAX_MUZZLE_FLASH; i++)
	{
		if(obmainValidateObject(_muzzelflash[i]))
		{
			ogrpFreeObjectTree(_muzzelflash[i]);
		}
	}

	_freeflash.clear();

	Write_Log(LOG_OLD, LOG_INFO, "[weapfxKill]");
}

/*
Oh god, I so didn't want to add this code, this is such a hack but at least this way I get around the mess that is ground.csv
*/

void _PlaySoundForKnife (FLOAT64VECTOR* Pos, SINT32 soundID)
{
	const SCHAR*	knifeSounds[2] = {"sounds\\genericknife.wav", "sounds\\stabknife.wav"};
	const SCHAR*	useSound;

	if (soundID < 0)
		useSound = knifeSounds[0];
	else if (soundID > 1)
		useSound = knifeSounds[1];
	else
		useSound = knifeSounds[soundID];

	SINT32 iID = fxSndGetNewSound();

	if(iID == -1)
		return;

	fxSndSetSoundName(iID, useSound);
	fxSndSetPosition(iID, Pos->x, Pos->y, Pos->z);
	fxSndSetPriority(iID, IDE_TP_PRIORITY_LOW);
	fxSndSetSoundType(iID, TYPE_FINITE_EXTERNAL_SOUND);
	fxSndSetVolume(iID, 1.0f);
	fxSndSetPitch(iID, 0.8f + 0.2f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f));
	fxSndSetPlayType(iID, DEF_PLAYTYPE_WORLD);
	fxSndSetMaxDistance(iID, 50.0f);
	fxSndSetMinDistance(iID, 10.0f);

	fxSndInitSound(iID);
}

void weapfxRicochetEffect(const weapAmmo& Ammo, const FASTCOLLIDE& collide, const char* effect)
{
	OBJECT* o = collide.CollideObject;

	if(o)
	{
		piVECTOR n;

		n.x = o->psnorms[collide.PolyNum].x;
		n.y = o->psnorms[collide.PolyNum].y;
		n.z = o->psnorms[collide.PolyNum].z;

		math3dMultiplyRotation(&n, &n, &o->mat);

		TransformEffect* te = TransformEffect::AddEffect(effect, Vec3f(n), 0, nullptr);

		if(te)
		{
			Vec3d p(Ammo.Position);
			p.z() = -p.z();

			te->enableemitters(true);;
			te->setmatrix(Matrixf::translate(p));
			te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
		}
	}
}

/************************************************************************************************
	weapfxAmmoHit
	Use this for hit marks (bullets/bombs) on objects. Called from external modules.
*************************************************************************************************/

UINT32 weapfxAmmoHit(weapAmmo const* Ammo,
					 FASTCOLLIDE const* lpFast,
					 piVECTOR const* OldPos,
					 piVECTOR const* NewPos,
					 void* pVehicle,
					 SINT32 iPenetration,
					 FLOAT64 KE)
{
	vehVehicle* Vehicle = (vehVehicle*)pVehicle;
	piVECTOR Norm;
	piVECTOR DeltaVec;
	vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();

	if(Ammo->Flags & WEAP_AMMOFLAG_NOFX) return 0;
	if(Ammo->AmmoType->IsSmokeGrenade == 1) return 0;

	mathSubVectors(&DeltaVec, NewPos, OldPos);
	mathReallyFastNormalize(DeltaVec);

	if(Vehicle)
	{
		FLOAT64VECTOR Pos;

		Pos.x = lpFast->CollidePnt.x;
		Pos.y = lpFast->CollidePnt.y;
		Pos.z = lpFast->CollidePnt.z;

		if(lpFast->CollideObject)
			mathVectorMultiply64(&Pos, &Pos, &lpFast->CollideObject->mat);

		mathAddVectors(&Pos, &Pos, &WorldOriginOffset); Pos.z = -Pos.z;

		if (Vehicle->VehicleType->Type != VEH_TYPE_PERSON)
		{
			if(lpFast->CollideObject)
				Norm = lpFast->CollideObject->psnorms[lpFast->PolyNum];
		}

		switch(Vehicle->VehicleType->Type)
		{
			case VEH_TYPE_PERSON:
				if(Vehicle != PlayerVehicle)
				{
					if (Ammo->AmmoType->SubType == WEAPAMMO_SUBTYPE_KNIFE)
					{
						_PlaySoundForKnife (&Pos, 1);
					}
					else
					{
						DoHitSound(&Pos,(FLOAT32)KE,"fps");
					}

					if(WantBloodEffects)
					{
						Vec3f n;

						// For debugging...
						static float scale = 0.25f;

						n.set((float)-Ammo->Velocity.x, (float)-Ammo->Velocity.y, (float)-Ammo->Velocity.z);
						n.fastnormalize();
						n *= scale;

						TransformEffect* te = TransformEffect::AddEffect("trooperhit", n, 0, nullptr);

						if(te)
						{
							Vec3d pos(Pos);
							pos.z() = -pos.z();

							te->enableemitters(true);;
							te->setmatrix(Matrixf::translate(pos));
							te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
						}
					}
				}
				//FXDoHitEffect(&Pos, KE, nullptr, lpFast->CollideObject, lpFast->PolyNum, "fps");

				// same side twich effects if I fired at the vehicle
				if(Ammo->FireVehicle &&
				   Vehicle &&
				   PlayerVehicle == (vehVehicle*)Ammo->FireVehicle &&
				   Vehicle->Side == PlayerVehicle->Side)
				{
					gr2AnimAddTwitch (Vehicle->VehicleObjectData->PlaneObject);
				}
			break;
			case VEH_TYPE_AIR:
				if(Vehicle != PlayerVehicle)
				{
					if (Ammo->AmmoType->SubType == WEAPAMMO_SUBTYPE_KNIFE)
					{
						_PlaySoundForKnife (&Pos, 0);
					}
					else
					{
						DoHitSound(&Pos,(FLOAT32)KE,"airvehicle");
					}
				}
				//FXDoHitEffect(&Pos, KE, &Norm, lpFast->CollideObject, lpFast->PolyNum, "airvehicle");
			break;
			case VEH_TYPE_GROUND:
				if(Vehicle != PlayerVehicle)
				{
					if (Ammo->AmmoType->SubType == WEAPAMMO_SUBTYPE_KNIFE)
					{
						_PlaySoundForKnife (&Pos, 0);
					}
					else
					{
						DoHitSound(&Pos,(FLOAT32)KE,"groundvehicle");
					}
				}

				if(Ammo->AmmoType->SubType != WEAPAMMO_SUBTYPE_KNIFE)
				{
				   // FXDoHitEffect(&Pos, KE, &Norm, lpFast->CollideObject, lpFast->PolyNum, "groundvehicle");
				}
			break;
			default:
				if(Vehicle != PlayerVehicle)
				{
					if (Ammo->AmmoType->SubType == WEAPAMMO_SUBTYPE_KNIFE)
					{
						_PlaySoundForKnife (&Pos, 0);
					}
					else
					{
						DoHitSound(&Pos,(FLOAT32)KE,"default");
					}
				}
				//FXDoHitEffect(&Pos, KE, &Norm, lpFast->CollideObject, lpFast->PolyNum, "default");
			break;
		}
	}

	if(Vehicle == nullptr || Vehicle->VehicleType->Type != VEH_TYPE_PERSON)
	{
		piVECTOR u;

		if(lpFast->Flags & ODE_TREE_COLLISION)
		{
			// Bullet
			if(Ammo->AmmoType->ExplosiveEnergy <= 0.0001f &&
			   Ammo->AmmoType->SubType == WEAPAMMO_SUBTYPE_BALLISTIC)
			{
				FLOAT64VECTOR vecPos;
				FLOAT64 fEnergy;
				FLOAT32 fVelSqr;

				weapfxGetObjectPosition(lpFast, &vecPos);

				fVelSqr = (FLOAT32)mathVectDotProduct(&Ammo->Velocity, &Ammo->Velocity);

				// 1J = 1/2 * mass * velocity^2
				fEnergy = fVelSqr * Ammo->AmmoType->ProjectileWeight * 0.5f;

				Vec3f n;
				n.set((float)-Ammo->Velocity.x, (float)-Ammo->Velocity.y, (float)-Ammo->Velocity.z);
				n.fastnormalize();

				TransformEffect* te = TransformEffect::AddEffect("genericbullethit", n, 0, nullptr);

				if(te)
				{
					Vec3d pos(vecPos);
					pos.z() = -pos.z();

					te->enableemitters(true);;
					te->setmatrix(Matrixf::translate(pos));
					te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
				}

				SINT32 id = fxSndGetNewSound();

				if(id == -1)
					return FALSE;

				const SCHAR* names[3] =
				{
					"sounds\\hitwood1.wav",
					"sounds\\hitwood2.wav",
					"sounds\\hitwood3.wav"
				};

				fxSndSetSoundName(id, names[GetRandom(0, 2)]);
				fxSndSetPosition(id, vecPos.x, vecPos.y, vecPos.z);
				fxSndSetPriority(id, IDE_TP_PRIORITY_LOW);
				fxSndSetSoundType(id, TYPE_FINITE_EXTERNAL_SOUND);
				fxSndSetVolume(id, 1.0f);
				fxSndSetPlayType(id, DEF_PLAYTYPE_WORLD);
				fxSndSetMaxDistance(id, 1069.0f);
				fxSndSetMinDistance(id, 10.0f);
				fxSndSetPitch(id, 1.0f);

				fxSndInitSound(id);

				return 0;
			}
		}

		if(lpFast->CollideObject && lpFast->CollideObject->psnorms == nullptr)
			return 0;

		if(lpFast->CollideObject)
		{
			u = lpFast->CollideObject->psnorms[lpFast->PolyNum];
			math3dVectorMultiply(&u, &u, &lpFast->CollideObject->mat);

			u.x -= lpFast->CollideObject->mat.m[3][0];
			u.y -= lpFast->CollideObject->mat.m[3][1];
			u.z -= lpFast->CollideObject->mat.m[3][2];
		}

		if(math3dDotProduct(&u, &DeltaVec) < 0.0f)
		{
			FLOAT64VECTOR Pos;

			Pos.x = lpFast->CollidePnt.x;
			Pos.y = lpFast->CollidePnt.y;
			Pos.z = lpFast->CollidePnt.z;

			if(lpFast->CollideObject)
				mathVectorMultiply64(&Pos, &Pos, &lpFast->CollideObject->mat);

			mathAddVectors(&Pos, &Pos, &WorldOriginOffset);
			Pos.z = -Pos.z;

			if(Ammo->AmmoType->ExplosiveEnergy <= 0.0001f)// bullet
			{
				if(lpFast->Flags & _AMMOCD_FLAG_ISAUTOGUN &&
				   Ammo->AmmoType->SubType != WEAPAMMO_SUBTYPE_KNIFE)
				{
					TransformEffect* te;
					te = TransformEffect::AddEffect("genericbullethit", Vec3f(u), 0, nullptr);

					if(te)
					{
						Vec3d p(Pos);
						p.z() = -p.z();

						te->enableemitters(true);;
						te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
						te->setmatrix(Matrixf::translate(p));
					}

					fxBltPlaySound(&Pos, lpFast->CollideObject);
				}
				else
				{
					AddBulletDecal(Ammo, lpFast, &DeltaVec, Vehicle, iPenetration);
					_CreateDebris(Ammo, lpFast, &DeltaVec, Vehicle, iPenetration);
				}
			}
			else
			{
				// explosion
				weapfxAmmoHitMark(Ammo,
								  &lpFast->CollidePnt,
								  &u,
								  lpFast->CollideObject,
								  lpFast->TopLevelObject,
								  Vehicle,
								  iPenetration);

				// Ticket# 5662: Bullet hit for rocket here.
				if(Ammo->AmmoType->SubType == WEAPAMMO_SUBTYPE_ROCKET)
				{
					AddBulletDecal(Ammo, lpFast, &DeltaVec, Vehicle, 1, false);
				}
				else
				{
					_CreateDebris(Ammo, lpFast, &DeltaVec, Vehicle, iPenetration);
				}
			}
		}
	}

	if(Vehicle == nullptr)
		return 0;

	if(Vehicle->VehicleType == nullptr)
		return 0;

#define MAX_DISTANCE2_TO_DISPLAY_BLOOD 4

	// Need for troop hits, i.e., smoke.
	if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
	{
		if(Ammo->AmmoType->ExplosiveEnergy <= 0.0001f)
		{
			// HACK: Needed a way to display a blood splat on objects next to troops
			if(lpFast[1].CollideObject &&
			  (lpFast[1].CollideObject->flag2 & OBJECT_TERRAIN_OBJECT ||
			   lpFast[1].CollideObject->flags & OBJECT_TERRAIN))
			{
				piVECTOR p1;
				p1.set(lpFast[0].CollidePnt.x, lpFast[0].CollidePnt.y, lpFast[0].CollidePnt.z);
				math3dVectorMultiply(&p1, &p1, &lpFast[0].CollideObject->mat);

				piVECTOR p2;
				p2.set(lpFast[1].CollidePnt.x, lpFast[1].CollidePnt.y, lpFast[1].CollidePnt.z);
				math3dVectorMultiply(&p2, &p2, &lpFast[1].CollideObject->mat);

				piVECTOR p3;
				mathSubVectors(&p3, &p2, &p1);
				float dist2 = math3dDotProduct(&p3, &p3);

				if(dist2 < MAX_DISTANCE2_TO_DISPLAY_BLOOD)
				{
					FXBULLETCREATE decal;
					memset(&decal, 0, sizeof(FXBULLETCREATE));

					weapfxGetObjectPosition(&lpFast[1], &decal.vecPos);

					decal.fRadius = 0.15f;
					decal.lpAttachedObj = lpFast[1].CollideObject;
					decal.lpTopLevelObj = lpFast[1].TopLevelObject;
					decal.iPolyNum = lpFast[1].PolyNum;
					decal.vecDelta = DeltaVec;
					decal.uAmmoSubType = Ammo->AmmoType->SubType;
					decal.flags = blt_decal_is_blood_splat;

					fxBltCreate(&decal);
				}
			}

			AddBulletDecal(Ammo,
						   lpFast,
						   &DeltaVec,
						   Vehicle,
						   iPenetration);
		}
	}

	return 0;
}

/************************************************************************************************
	weapfxAmmoExplosion
*************************************************************************************************/

UINT32 weapfxAmmoExplosion(weapAmmo const* Ammo, FLOAT64VECTOR const* Pos, FASTCOLLIDE const* lpFast,  SINT32 iPenetration)
{
	if(!Ammo->AmmoType->ExplosiveEnergy || (Ammo->Flags & WEAP_AMMOFLAG_NOFX))
		return 0;

	// Vehicle/Ground hit
	if(lpFast != nullptr)
	{
		piVECTOR n = {1.0f, 0.0f, 0.0f};

		if(lpFast->CollideObject && lpFast->CollideObject->psnorms != nullptr)
		{
			n = lpFast->CollideObject->psnorms[lpFast->PolyNum];

			math3dVectorMultiply(&n, &n, &lpFast->CollideObject->mat);

			n.x -= lpFast->CollideObject->mat.m[3][0];
			n.y -= lpFast->CollideObject->mat.m[3][1];
			n.z -= lpFast->CollideObject->mat.m[3][2];

			weapfxExplosion(Ammo, Pos, &n, _IsWater(lpFast->CollideObject));
		}
		else
		{
			weapfxExplosion(Ammo, Pos, nullptr, false);
		}
	}
	// Special case with satchels
	else if(Ammo->AmmoType->SubType == WEAPAMMO_SUBTYPE_PLACED)
	{
		weapfxAmmoExpHitAttached(Ammo, iPenetration);
	}
	// Air burst
	else
	{
		weapfxExplosion(Ammo, Pos, nullptr, false);
	}

	physicsAddWorldForce(*Pos, (FLOAT64)Ammo->AmmoType->ExplosiveEnergy);

	return 0;
}

/************************************************************************************************
	weapfxAmmoTracer
*************************************************************************************************/

UINT32 weapfxAmmoTracer(weapAmmo* Ammo, FLOAT64VECTOR const* LastPosition)
{
	FLOAT64VECTOR TempVec,TempVec1;
	FLOAT64 OneOverLen, Speed;
	piVECTOR Stretch;

	Speed = mathExactVectorLength(&Ammo->Velocity);

	mathSubVectors(&TempVec,&Ammo->Position,LastPosition);
	OneOverLen = 1.0f / Speed;
	TempVec.x *= OneOverLen;
	TempVec.y *= OneOverLen;
	TempVec.z *= OneOverLen;

	if (SuppressTracerSmoke == FALSE)
	{
		TempVec1 = *LastPosition;
		Stretch.x = (FLOAT32)(TempVec.x * Speed);
		Stretch.y = (FLOAT32)(TempVec.y * Speed);
		Stretch.z = (FLOAT32)(TempVec.z * Speed);

		TempVec1.x += TempVec.x * Speed * 0.5;
		TempVec1.y += TempVec.y * Speed * 0.5;
		TempVec1.z += TempVec.z * Speed * 0.5;
//		mathSubVectors(&TempVec1,&TempVec1,&Stretch); // reverted

#if(1)
#else
		FXCreateNewParticle(&TempVec1,
							&Stretch,
							0.0f,
							nullptr,
							0.2f,
							0.1f,
							0.05f,
							0.0f,
							Ammo->AmmoType->Diameter * 2.0f,
							Ammo->AmmoType->Diameter * 8.0f,
							Ammo->AmmoType->Diameter * 16.0f,
							Ammo->AmmoType->Diameter * 32.0f,
							frametime,
							frametime + 200,
							frametime + 400,
							frametime + 800,
							15,//12,
							nullptr,
							PARTICLE_FLAG_USE_VEL_FOR_STRETCH |
								PARTICLE_FLAG_AT_LEAST_1_PIXEL,
							nullptr);
#endif
	}


// use 3 particles, of different fades
	TempVec1 = *LastPosition;
	Stretch.x = (FLOAT32)(TempVec.x * Speed * 0.5);
	Stretch.y = (FLOAT32)(TempVec.y * Speed * 0.5);
	Stretch.z = (FLOAT32)(TempVec.z * Speed * 0.5);
//	mathSubVectors(&TempVec1,&TempVec1,&Stretch); // reverted

	if(Ammo->iParticleID == 0 && Ammo->AmmoType->TracerTime > 0.0f)
	{
		TransformEffect* te = nullptr;

		Vec3f stretch(Stretch);
		stretch = -stretch;

		ParticleEffect* pe1 = FxGet().cloneparticleeffect("tracerparticle");
		ParticleEffect* pe2 = FxGet().cloneparticleeffect("tracersmokeparticle");

		float scale = Ammo->AmmoType->TracerScale;

		if(pe1)
		{
			float s = Ammo->AmmoType->Diameter * scale;
			pe1->setsize(s, s);
			float len = (float)mathExactVectorLength(&Stretch);
			pe2->separation = len;
		}

		if(pe2)
		{
			float s1 = Ammo->AmmoType->Diameter * 2.0f;
			float s2 = Ammo->AmmoType->Diameter * 8.0f;
			float s3 = Ammo->AmmoType->Diameter * 16.0f;
			float s4 = Ammo->AmmoType->Diameter * 32.0f;

			pe2->setsize(s1, s2, s3, s4);
			pe2->settime(0, 200, 400, 800);
			pe2->setalpha(0.2f, 0.1f, 0.05f, 0.0f);

			float len = (float)mathExactVectorLength(&Stretch);
			pe2->separation = len;
		}

		te = TransformEffect::AddEffect("tracer", stretch, 0, nullptr);

		if(te)
		{
			// Effect dies when ammo is dead
			te->setweapammo(Ammo);
			te->enableemitters(true);

			if(te->getemitter(0) && pe1)
			{
				te->getemitter(0)->setparticleeffect(pe1);
				te->getemitter(0)->_lifetime = FloatToInt(Ammo->AmmoType->TracerTime * 1000.0f);
			}

			if(te->getemitter(1) && pe2)
			{
				te->getemitter(1)->setparticleeffect(pe2);
				te->getemitter(1)->_lifetime = FloatToInt(Ammo->AmmoType->TracerTime * 1000.0f * 0.15f);
		   }
		}

		Ammo->iParticleID = -1;
	}

	return 0;
}

/************************************************************************************************
	weapfxAmmoFlyby
*************************************************************************************************/

UINT32 weapfxAmmoFlyby(weapAmmo* Ammo)
{
	FLOAT64VECTOR Pos;
	piVECTOR Pos1,Pos2;
	piVECTOR Vec1,Vec2;
	FLOAT32 D1,D2;
	FLOAT32 TempNum;
	piVECTOR p;
	FLOAT64 SpeedOfSound;

	// Don't want this for bombs
	if(Ammo->AmmoType->SubType == WEAPAMMO_SUBTYPE_BOMB)
	{
		return 0;
	}

	if(Ammo->AmmoType->SubType == WEAPAMMO_SUBTYPE_TORPEDO)
	{
		return 0;
	}

	if(Ammo->AmmoType->SubType == WEAPAMMO_SUBTYPE_MORTAR)
	{
		return 0;
	}
	if(Ammo->AmmoType->Diameter <= 0.03f)
		return 0;

	p.x = -playpos.x;
	p.y = -playpos.y;
	p.z = -playpos.z;

	Pos1.x = (FLOAT32)(Ammo->LastCollidePos.x - WorldOriginOffset.x);
	Pos1.y = (FLOAT32)(Ammo->LastCollidePos.y - WorldOriginOffset.y);
	Pos1.z = (FLOAT32)(Ammo->LastCollidePos.z - WorldOriginOffset.z);

	Pos2.x = (FLOAT32)(Ammo->Position.x - WorldOriginOffset.x);
	Pos2.y = (FLOAT32)(Ammo->Position.y - WorldOriginOffset.y);
	Pos2.z = (FLOAT32)(Ammo->Position.z - WorldOriginOffset.z);

	// determine if we even hear the sound
	mathSubVectors(&Vec1,&Pos2,&Pos1);
	mathSubVectors(&Vec2,&p,&Pos1);
	D1 = mathVectDotProduct(&Vec1,&Vec2);
	if (D1 < 0.0f)
	{
		return 0;
	}
	mathSubVectors(&Vec1,&Pos1,&Pos2);
	mathSubVectors(&Vec2,&p,&Pos2);
	D2 = mathVectDotProduct(&Vec1,&Vec2);
	if (D2 < 0.0f)
	{
		return 0;
	}

	D1 = (FLOAT32)math3dFastSqrt(D1);
	D2 = (FLOAT32)math3dFastSqrt(D2);

	if (D2 + D1 < 0.001)
	{
		return 0;
	}
	// ok we're going to generate the sound
	// calculate the position in the world
	TempNum = D1 / (D1 + D2); // time, 0.0 = 1st pos, 1.0 = 2nd
	mathSubVectors(&Vec1,&Pos2,&Pos1);
	Pos2 = Pos1;
	Pos2.x += TempNum * Vec1.x;
	Pos2.y += TempNum * Vec1.y;
	Pos2.z += TempNum * Vec1.z;

	mathSubVectors(&Vec1,&p,&Pos2);
	if (mathVectDotProduct(&Vec1,&Vec1) < 40000.0)
	{
		Pos.x = Pos2.x + WorldOriginOffset.x;
		Pos.y = Pos2.y + WorldOriginOffset.y;
		Pos.z = Pos2.z + WorldOriginOffset.z;

		// voila, now play the sound
		mlGetAltSpeedOfSound(&SpeedOfSound,Ammo->Position.z);
		double sos2 = SpeedOfSound * SpeedOfSound;

		double speed2 = (FLOAT32)(mathExactVectorLength2(&Ammo->Velocity));

		if (speed2 < sos2)
		{
			SINT32 iID = fxSndGetNewSound();

			if(iID == -1)
				return 1;

			fxSndSetPitch(iID, 1.0f);
			fxSndSetVolume(iID, 1.0f);
			fxSndSetPriority(iID, IDE_TP_PRIORITY_LOW);
			fxSndSetSoundType(iID, TYPE_FINITE_EXTERNAL_SOUND);
			fxSndSetPosition(iID, Pos.x, Pos.y, Pos.z);
			fxSndSetMinDistance(iID, 100.0f);
			fxSndSetMaxDistance(iID, 200.0f);
			fxSndSetPlayType(iID, DEF_PLAYTYPE_WORLD);

			// subsonic round
			if (Ammo->AmmoType->Diameter < 0.02f)
			{
				fxSndSetSoundName(iID, "sounds\\loshot1.wav");
			}
			else if (Ammo->AmmoType->Diameter < 0.06f)
			{
				fxSndSetSoundName(iID, "sounds\\loshot2.wav");
			}
			else
			{
				fxSndSetSoundName(iID, "sounds\\loshot3.wav");
			}

			fxSndInitSound(iID);

			combattime = frametime;
		}
		else
		{
			SINT32 iID = fxSndGetNewSound();

			if(iID == -1)
				return 1;

			fxSndSetPitch(iID, 1.0f);
			fxSndSetVolume(iID, 1.0f);
			fxSndSetPriority(iID, IDE_TP_PRIORITY_LOW);
			fxSndSetSoundType(iID, TYPE_FINITE_EXTERNAL_SOUND);
			fxSndSetPosition(iID, Pos.x, Pos.y, Pos.z);
			fxSndSetPlayType(iID, DEF_PLAYTYPE_WORLD);
			fxSndSetMinDistance(iID, 100.0f);
			fxSndSetMaxDistance(iID, 200.0);

			// supersonic round
			if (Ammo->AmmoType->Diameter < 0.02f)
			{
				fxSndSetSoundName(iID, "sounds\\hishot1.wav");
			}
			else
			if (Ammo->AmmoType->Diameter < 0.06f)
			{
				fxSndSetSoundName(iID, "sounds\\hishot2.wav");
			}
			else
			{
				fxSndSetSoundName(iID, "sounds\\hishot3.wav");
			}

			fxSndInitSound(iID);

			combattime = frametime;
		}
	}

	return 0;
}

/************************************************************************************************
	weapfxAmmoMuzzleFlash
*************************************************************************************************/

UINT32 weapfxAmmoMuzzleFlash(weapAmmo const* Ammo,
							 FLOAT64VECTOR const* RelativePnt,
							 OBJECT* EmitterObject,
							 FLOAT64VECTOR const* Vel,
							 FLOAT64 LaunchTime,
							 const weapWeapon* weapon)
{
	FLOAT32 Intensity;
	vehVehicle*pVeh = (vehVehicle*)Ammo->FireVehicle;

	// Hack until all muzzle flashes are done.
	LOGICAL bNewMuzzleFlash = FALSE;

	if(pVeh != nullptr)
	{
		if(pVeh->VehicleType != nullptr)
		{
			switch(pVeh->VehicleType->Type)
			{
			case VEH_TYPE_PERSON:
			case VEH_TYPE_AIR:
				{
					bNewMuzzleFlash = TRUE;
				}
			}
		}
	}

	Intensity = Ammo->AmmoType->InitVelocity * Ammo->AmmoType->InitVelocity;
	Intensity *= 1.3f;
	Intensity *= Ammo->AmmoType->ProjectileWeight;

	// TODO: Below is fubar. All muzzleflash effects need to be moved to the ammo file
	// i.e., 'muzzelflash = muzzlesmoke' instead of 'muzzleflash = 1'.
	if(!(Ammo->Flags & WEAP_AMMOFLAG_NOMUZZLEFLASH))
	{
		if (EmitterObject != nullptr)
		{
			if(Ammo->AmmoType->MuzzleFlashType == WEAP_MUZZLEFLASH_GUN)
			{
				_CreateMuzzleFlash(Ammo, EmitterObject, RelativePnt);
			}
			else if(Ammo->AmmoType->MuzzleFlashType == WEAP_MUZZLEFLASH_BIGGUN)
			{
				_CreateBigGunFlash(Ammo, EmitterObject, RelativePnt);
			}
			else if(Ammo->AmmoType->MuzzleFlashType == WEAP_MUZZLEFLASH_TANK)
			{
				_CreateTankGunFlash(Ammo, EmitterObject, RelativePnt);
			}
			else if(Ammo->AmmoType->MuzzleFlashType == WEAP_MUZZLEFLASH_GENERIC)
			{
				_CreateMuzzleFlash(Ammo, EmitterObject, RelativePnt);
			}
			else if(Ammo->AmmoType->MuzzleFlashType == WEAP_MUZZLEFLASH_MORTAR)
			{
				Vec3f n;
				n.set(float(Ammo->Velocity.x), float(Ammo->Velocity.y), float(-Ammo->Velocity.z));
				n.fastnormalize();

				TransformEffect* te = TransformEffect::AddEffect("mortarflash", n, 0, nullptr);

				if(te)
				{
					Vec3d p(Ammo->Position);
					p.z() = -p.z();

					te->enableemitters(true);;
					te->setmatrix(Matrixf::translate(p));
					te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
				}
			}
			else if(Ammo->AmmoType->MuzzleFlashType == WEAP_MUZZLEFLASH_SMALLTANK)
			{
				Vec3f n;
				n.set(float(Ammo->Velocity.x), float(Ammo->Velocity.y), float(-Ammo->Velocity.z));
				n.fastnormalize();

				TransformEffect* te = TransformEffect::AddEffect("smalltankgunflash", n, 0, nullptr);

				if(te)
				{
					Vec3d p(Ammo->Position);
					p.z() = -p.z();

					te->enableemitters(true);;
					te->setmatrix(Matrixf::translate(p));
					te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
				}
			}
		}
		else
		{
			if(bNewMuzzleFlash)
			{
				switch(Ammo->AmmoType->MuzzleFlashType)
				{
				case WEAP_MUZZLEFLASH_MORTAR:
					{
						Vec3f n;
						n.set(float(Ammo->Velocity.x), float(Ammo->Velocity.y), float(-Ammo->Velocity.z));
						n.fastnormalize();

						TransformEffect* te = TransformEffect::AddEffect("mortarflash", n, 0, nullptr);

						if(te)
						{
							Vec3d p(Ammo->Position);

							if(RelativePnt)
							{
								p.x() += RelativePnt->x;
								p.y() += RelativePnt->y;
								p.z() += RelativePnt->z;
							}

							p.z() = -p.z();

							te->enableemitters(true);;
							te->setmatrix(Matrixf::translate(p));
							te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
						}

						break;
					}
				case WEAP_MUZZLEFLASH_SMALLTANK:
					{
						Vec3f n;
						n.set(float(Ammo->Velocity.x), float(Ammo->Velocity.y), float(-Ammo->Velocity.z));
						n.fastnormalize();

						TransformEffect* te = TransformEffect::AddEffect("smalltankgunflash", n, 0, nullptr);

						if(te)
						{
							Vec3d p(Ammo->Position);
							p.z() = -p.z();

							te->enableemitters(true);;
							te->setmatrix(Matrixf::translate(p));
							te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
						}

						break;
					}
				default:
					{
						_CreateMuzzleFlash(Ammo, nullptr, &Ammo->Position);
					}
				}
			}
			else
			{
				// AI guns
				if(weapon && weapon->WeaponType)
				{
					if(weapon->WeaponType->muzzleflash[0])
					{
						Vec3f n;
						n.set(float(Ammo->Velocity.x), float(Ammo->Velocity.y), float(-Ammo->Velocity.z));
						n.fastnormalize();

						TransformEffect* te = TransformEffect::AddEffect(weapon->WeaponType->muzzleflash, n, 0, nullptr);

						if(te)
						{
							Vec3d p(Ammo->Position);
							p.z() = -p.z();

							p += n * 0.15f;

							te->enableemitters(true);;
							te->setmatrix(Matrixf::translate(p));
							te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
						}

						return 0;
					}
				}
			}
		}
	}

	return 0;
}

SINT32 _CreateTankGunFlash(const weapAmmo* pAmmo, OBJECT* o, const FLOAT64VECTOR* pPos)
{
	FLOAT64VECTOR vPos = *pPos;

	if(o)
	{
		if(o->dof)
		{
			fxUtlGetDOFPos(o->dof, &o->mat, &vPos);
		}
		else
		{
			mathVectorMultiply64(&vPos, &vPos, &o->mat);
			mathAddVectors(&vPos, &vPos, &WorldOriginOffset);
			vPos.z = -vPos.z;
		}
	}

	piVECTOR vDir =
	{
		(FLOAT32)pAmmo->Velocity.x,
		(FLOAT32)pAmmo->Velocity.y,
		(FLOAT32)pAmmo->Velocity.z
	};

	mathReallyFastNormalize(vDir);

	TransformEffect* te = TransformEffect::AddEffect("tankgunflash", Vec3f(vDir), 0, nullptr);

	if(te)
	{
		Vec3d p = Vec3d(vPos);
		p.z() = -p.z();

		te->enableemitters(true);;
		te->setmatrix(Matrixf::translate(p));
		te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
	}

	if(!SuppressMuzzleLight)
	{
		fxLgtCreateMuzzleLight(&vPos, &vDir);
	}

	_CreateMuzzleFlash(pAmmo, o, pPos);

	return 0;
}

SINT32 _CreateBigGunFlash(const weapAmmo* pAmmo, OBJECT* o, const FLOAT64VECTOR* pPos)
{
	FLOAT64VECTOR vPos = *pPos;

	if(o)
	{
		if(o->dof)
		{
			fxUtlGetDOFPos(o->dof, &o->mat, &vPos);
		}
		else
		{
			mathVectorMultiply64(&vPos, &vPos, &o->mat);
			mathAddVectors(&vPos, &vPos, &WorldOriginOffset);
		}
	}

	piVECTOR vDir =
	{
		(FLOAT32)pAmmo->Velocity.x,
		(FLOAT32)pAmmo->Velocity.y,
		(FLOAT32)-pAmmo->Velocity.z
	};

	mathReallyFastNormalize(vDir);

	TransformEffect* te = TransformEffect::AddEffect("biggunflash", Vec3f(vDir), 0, nullptr);

	if(te)
	{
		vPos.z = -vPos.z;
		te->enableemitters(true);;
		te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
		te->setmatrix(Matrixf::translate(Vec3d(vPos)));
	}

	if(!SuppressMuzzleLight)
	{
		fxLgtCreateMuzzleLight(&vPos, &vDir);
	}

	return 0;
}

/************************************************************************************************
	_CreateMuzzleFlash
*************************************************************************************************/
UINT32 _CreateMuzzleFlash(weapAmmo const* pAmmo, OBJECT* o, const FLOAT64VECTOR* pos)
{
	if(pAmmo == nullptr)
		return 1;

	OBJECT* fx = _GetFreeFlash();

	if(!fx)
	{
		return 1;
	}

	int id = fxPclGetNewParticle();

	if(id == -1)
		return 1;

	FLOAT64VECTOR position = pAmmo->Position;

	if(o)
	{
		position = *pos;

		position.z = -position.z;

		mathVectorMultiply64(&position, &position, &o->mat);

		position.x += WorldOriginOffset.x;
		position.y += WorldOriginOffset.y;
		position.z += WorldOriginOffset.z;

		position.z = -position.z;
	}

	fxPclPushFlags(id, DEF_FXFLAG_OBJECT);
	fxPclPushFlags(id, DEF_FXFLAG_NOPHYSICS);
	fxPclSetUserKillCallback(id, weapfxFlashKillCallback);
	fxPclSetKillData(id, fx);
	fxPclSetType(id, IDE_PARTICLE_MUZZLEFLASH);
	fxPclSetPosition(id, position.x, position.y, position.z);
	fxPclSetAmmoFlags(id, pAmmo->Flags);

	if(o != nullptr)
	{
		fxPclSetEmitterObject(id, o);
		fxPclSetDOFObject(id, o->dof);
	}

	fxPclSetVehicle(id, (vehVehicle*)pAmmo->FireVehicle);

	piVECTOR v;

	v.x = (FLOAT32)pAmmo->Velocity.x;
	v.y = (FLOAT32)pAmmo->Velocity.y;
	v.z = (FLOAT32)pAmmo->Velocity.z;

	mathReallyFastNormalize(v);

	// For muzzle smoke and object orientation
	fxPclSetDirection(id, v.x, v.y, v.z);

	// Scale based on intensity
	// Intensity derivation copied from old method
	float intensity;

	intensity = pAmmo->AmmoType->InitVelocity * pAmmo->AmmoType->InitVelocity;
	intensity *= 1.3f;
	intensity *= pAmmo->AmmoType->ProjectileWeight;

	float scale = (FLOAT32)(pow(intensity, 0.333333f) * 2.0) * 0.01f;

	// Set radius for muzzle smoke
	fxPclSetRadius(id, pAmmo->AmmoType->Diameter * 50.0f);

	fxPclSetFXObject(id, fx);

	fxPclSetPt1Time(id, (FLOAT32)frametime);
	fxPclSetPt4Time(id, (FLOAT32)frametime + 100.0f);

	if(!SuppressMuzzleLight)
	{
		fxLgtCreateMuzzleLight(&position, &v);
	}

	// Scale based on intensity
	// Intensity derivation copied from old method
	intensity = pAmmo->AmmoType->InitVelocity * pAmmo->AmmoType->InitVelocity;
	intensity *= 1.3f;
	intensity *= pAmmo->AmmoType->ProjectileWeight;

	scale = (FLOAT32)(pow(intensity, 0.333333f) * 2.0) * 0.008f;

	fx->updatecallback = nullptr;

	if(pAmmo->FireVehicle)
	{
		vehVehicle* veh = (vehVehicle*)pAmmo->FireVehicle;

		if(veh->InUse)
		{
			if(veh->VehicleType->Type == VEH_TYPE_PERSON && veh->IsMe() && fpFPSIsAim())
				scale *= 0.5f;
			else
				scale *= 0.75f;
		}
	}

	fxPclSetScale(id, scale);

	fx->rot.x = scale;
	fx->drawcallback = _drawMuzzleFlash;
	fx->lpspare = (void*)pAmmo->FireVehicle;
	fx->feature = pAmmo->Flags & WEAP_AMMOFLAG_IS_GHOSTROUND ? 1 :
				  !Online && persIsCameraExternalMode() ? 1 :
				  0;
	fx->vel.set(v.x, v.y, -v.z);

	fx->createtime = frametime;

	// Increment by 90 degrees for a reason. Render object will display 'planes'
	static int rotate[] = {0, 90, 180, 270};
	fx->rot.y = DEG_TO_RAD(rotate[GetRandom(0, 3)]);

	obmainMoveToObjectTree(fx);

	if(!fxPclInitParticle(id))
	{
		fxPclDetachByID(id);
	}

	return 0;
}

/************************************************************************************************
	weapfxAmmoWaterSplash
*************************************************************************************************/

UINT32 weapfxAmmoWaterSplash(weapAmmo const* Ammo, FLOAT64VECTOR const* CollidePos, OBJECT* CollideObject, SINT32 PolyNum, FLOAT64 KE)
{
	piMATRIX mat;
	piVECTOR RotNorm;

	RotNorm = CollideObject->psnorms[PolyNum];
	mat = CollideObject->mat; mat._41 = mat._42 = mat._43 = 0;
	math3dVectorMultiply(&RotNorm, &RotNorm, &mat);

	if(Ammo->AmmoType->SubType == WEAPAMMO_SUBTYPE_TORPEDO && !Ammo->TorpedoIsRunning) // don't apply explosive energy to splash, just KE
	{
		Vec3f n(0, 0, -1);

		TransformEffect* te = TransformEffect::AddEffect("shellwaterhit", n, 0, nullptr);

		if(te)
		{
			Vec3d pos(*CollidePos);
			pos.z() = -pos.z();

			te->enableemitters(true);;
			te->setmatrix(Matrixf::translate(pos));
			te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
		}
	}
	else
	{
		switch(Ammo->AmmoType->SubType)
		{
		case WEAPAMMO_SUBTYPE_BOMB:
		case WEAPAMMO_SUBTYPE_TORPEDO:
		case WEAPAMMO_SUBTYPE_PLACED:
			{
				return 0;
			}
		}

		Vec3f n(0, 0, -1);

		TransformEffect* te = TransformEffect::AddEffect("watersplash", n, 0, nullptr);

		if(te)
		{
			Vec3d pos(*CollidePos);
			pos.z() = -pos.z();

			te->enableemitters(true);;
			te->setmatrix(Matrixf::translate(pos));
			te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
		}

		switch(Ammo->AmmoType->SubType)
		{
		case WEAPAMMO_SUBTYPE_GRENADE:
		case WEAPAMMO_SUBTYPE_MORTAR:
			{
				fxSndAddFiniteWorldSound("sounds/grenadesplash.wav",
										 5,
										 100,
										 IDE_TP_PRIORITY_LOW,
										 *CollidePos);
				break;
			}
		case WEAPAMMO_SUBTYPE_BALLISTIC:
			{
				fxSndAddFiniteWorldSound("sounds/hitwater1.wav",
										 5,
										 100,
										 IDE_TP_PRIORITY_LOW,
										 *CollidePos);
				break;
			}
		}
	}

	return 0;
}

/************************************************************************************************
	weapfxAmmoAttachSound
*************************************************************************************************/

UINT32 weapfxAmmoAttachSound(weapAmmo* Ammo)
{
	if(Ammo == nullptr) return 1;
	if(Ammo->AmmoType == nullptr) return 1;
	if(Ammo->AmmoType->Object == nullptr) return 1;

	// Bomb whistle
	if(Ammo->AmmoType->SubType == WEAPAMMO_SUBTYPE_BOMB)
	{
		return _Attach3DBombSound(Ammo);
	}

	return 1;
}

/************************************************************************************************
	_Attach3DBombSound
*************************************************************************************************/
SINT32 _Attach3DBombSound(weapAmmo* pAmmo)
{
	FLOAT32 fVolume = 1.0f;
	SINT32 iID = fxSndGetNewSound();

	if(iID == -1)
		return 1;

	// Bomb whistle reduced if inside aircraft.
	if(persIsPersonaVehicleType(VEH_TYPE_AIR))
		fVolume = 0.1f;

	fxSndSetSoundName(iID, "sounds\\3dbomb.wav");
	fxSndSetPriority(iID, IDE_TP_PRIORITY_LOW);
	fxSndSetSoundType(iID, TYPE_INFINITE_EXTERNAL_SOUND);
	fxSndSetPitch(iID, 1.0f);
	fxSndSetVolume(iID, fVolume);
	fxSndSetPlayType(iID, DEF_PLAYTYPE_AMMO);
	fxSndPushFlags(iID, DEF_SOUND_STOPS_AMBIENCE);
	fxSndSetMinDistance(iID, 20.0f);
	fxSndSetMaxDistance(iID, 8000.0f);
	fxSndSetAmmo(iID, pAmmo);

	if(!fxSndInitSound(iID))
	{
		fxSndStopSound(iID);
		return 2;
	}

	pAmmo->SoundID = iID;

	return 0;
}

/************************************************************************************************
	weapfxAmmoDetachSound
*************************************************************************************************/

UINT32 weapfxAmmoDetachAmmo(weapAmmo const* Ammo)
{
	if(bSoundEnable)
	{
		fxSndDetachAmmo(Ammo);
	}

	return 0;
}

/************************************************************************************************
	weapfxMountFireSound
*************************************************************************************************/

UINT32 weapfxMountFireSound(mlWeaponMount* Mount, UINT32 i, SINT32 RoundsLeft, FLOAT64VECTOR const* Pos, OBJECT* PlaneObject)
{
	UINT32 PlayType;

	PlayType = (Mount->Weapons[i].WeaponType->FireType == WEAP_FIRETYPE_SINGLESHOT) ? TYPE_FINITE_SOUND : TYPE_INFINITE_SOUND;

	if(((Mount->Weapons[i].WeaponType->FireType != WEAP_FIRETYPE_SINGLESHOT ||
		Mount->Weapons[i].NumRoundsAmmoLeft[Mount->Weapons[i].AmmoTypeInUse] != RoundsLeft) &&
		strlen(Mount->WeaponSounds[i]) > 0) &&
		RoundsLeft > 0)
	{
		if(Mount->WeaponSoundId[i] > 0 &&
		   Mount->Weapons[i].WeaponType->FireType == WEAP_FIRETYPE_SINGLESHOT)
		{
			weapfxMountStopFireSound(Mount, i, RoundsLeft);
		}

		if (Mount->WeaponSoundId[i] == 0)
		{
			SINT32 iID = fxSndGetNewSound();

			if(iID == -1)
				return 1;

			fxSndSetSoundName(iID, Mount->WeaponSounds[i]);
			fxSndSetPriority(iID, IDE_FP_PRIORITY_MANDATORY);
			fxSndSetSoundType(iID, PlayType);
			fxSndSetPitch(iID, 1.0f);
			fxSndSetVolume(iID, 1.0f);
			fxSndSetPosition(iID, Pos->x, Pos->y, Pos->z);
			fxSndSetEmitterObject(iID, PlaneObject);
			fxSndSetPlayType(iID, DEF_PLAYTYPE_HEAD);

			if(!fxSndInitSound(iID))
			{
				Mount->WeaponSoundId[i] = 0;
			}
			else
			{
				Mount->WeaponSoundId[i] = iID;
			}
		}
	}
	else
	{
		if (Mount->WeaponSoundId[i] > 0)
		{
			if (PlayType == TYPE_INFINITE_SOUND)
			{
				fxSndStopSoundDelayed(Mount->WeaponSoundId[i], 1);
				Mount->WeaponSoundId[i] = 0;
			}
		}
	}

	return 0;
}

/************************************************************************************************
	weapfxMountReloadSound
*************************************************************************************************/

UINT32 weapfxMountReloadSound(mlWeaponMount* Mount, UINT32 i, FLOAT64VECTOR const* Pos, OBJECT* PlaneObject)
{
	if(Mount->WeaponSoundsReload[i])
	{
		SINT32 iID = fxSndGetNewSound();

		if(iID == -1)
			return 1;

		fxSndSetSoundName(iID, Mount->WeaponSoundsReload[i]);
		fxSndSetPriority(iID, IDE_FP_PRIORITY_HIGH);
		fxSndSetSoundType(iID, TYPE_FINITE_SOUND);
		fxSndSetPitch(iID, 1.0f);
		fxSndSetVolume(iID, 1.0f);
		fxSndSetPosition(iID, Pos->x, Pos->y, Pos->z);
		fxSndSetEmitterObject(iID, PlaneObject);
		fxSndSetPlayType(iID, DEF_PLAYTYPE_HEAD);

		fxSndInitSound(iID);
	}

	return 0;
}

/************************************************************************************************
	weapfxMountStopFireSound
*************************************************************************************************/

UINT32 weapfxMountStopFireSound(mlWeaponMount* Mount, UINT32 i, SINT32 RoundsLeft)
{
	if (Mount->WeaponSoundId[i] > 0)
	{
		if(Mount->Weapons[i].WeaponType->FireType != WEAP_FIRETYPE_SINGLESHOT)
		{
			fxSndStopSound(Mount->WeaponSoundId[i]);
		}
		else
		{
			fxSndStopSoundDelayed(Mount->WeaponSoundId[i], 1);
		}

		Mount->WeaponSoundId[i] = 0;
	}

	return 0;
}

/************************************************************************************************
	AddBulletDecal
*************************************************************************************************/

UINT32 AddBulletDecal(weapAmmo const* lpAmmo,
					  FASTCOLLIDE const* lpFast,
					  piVECTOR const* pvecShot,
					  void* pVehicle,
					  SINT32 iPenetration,
					  bool usehiteffect)
{
	bool iswater = _IsWater(lpFast->CollideObject);

	if(iswater)
	{
		piVECTOR n = {0, 0, -1};
		FLOAT64VECTOR pos;
		weapfxGetObjectPosition(lpFast, &pos);

		float joules = (float)mathVectDotProduct(&lpAmmo->Velocity, &lpAmmo->Velocity) *
					   lpAmmo->AmmoType->ProjectileWeight * 0.5f;

		static float expo = 0.25f;

		float size = pow(joules, expo);

		TransformEffect* te = TransformEffect::AddEffect("waterhit", n, 0, nullptr);

		if(te)
		{
			Vec3d p(pos);
			p.z() = -p.z();

			te->enableemitters(true);;
			te->setmatrix(Matrixf::translate(p));
			te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);

			ParticleEffect* pe1 = FxGet().cloneparticleeffect("waterhitwakeparticle");

			size *= 0.25f;

			pe1->size[0] = size;
			pe1->size[1] = pe1->size[0] + size;
			pe1->size[2] = pe1->size[1] + size;
			pe1->size[3] = pe1->size[2] + size;

			Emitter* e = nullptr;

			for(int i = 0; te->getemitter(i); i++)
			{
				e = te->getemitter(i);

				if(e->getparticleeffect()->name == "waterhitwakeparticle")
				{
					e->setparticleeffect(pe1);
				}
			}
		}

		return 0;
	}

	piVECTOR n = {0, 0, -1};
	FLOAT64VECTOR pos;
	weapfxGetObjectPosition(lpFast, &pos);

	OBJECT* o = lpFast->CollideObject;

	if(o && o->psnorms)
	{
		n = o->psnorms[lpFast->PolyNum];
		math3dMultiplyRotation(&n, &n, &o->mat);
	}

	if(usehiteffect && lpAmmo->AmmoType->hiteffect[0])
	{
		TransformEffect* te = TransformEffect::AddEffect(lpAmmo->AmmoType->hiteffect, Vec3f(n), 0, nullptr);

		if(te)
		{
			Vec3d p(pos);
			p.z() = -p.z();

			te->enableemitters(true);;
			te->setmatrix(Matrixf::translate(p));
			te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
		}
	}

	FXBULLETCREATE bltCreate;
	FLOAT32 fVelSqr = 0.0f;

	memset(&bltCreate, 0, sizeof(FXBULLETCREATE));

	weapfxGetObjectPosition(lpFast, &bltCreate.vecPos);

	fVelSqr = (FLOAT32)mathVectDotProduct(&lpAmmo->Velocity, &lpAmmo->Velocity);

	///////////////////////////////
	// 1J = 1/2 * mass * velocity^2
	///////////////////////////////
	bltCreate.fEnergy = fVelSqr * lpAmmo->AmmoType->ProjectileWeight * 0.5f;

	bltCreate.fRadius = lpAmmo->AmmoType->Diameter * 0.5f;
	bltCreate.lpAttachedObj = lpFast->CollideObject;
	bltCreate.lpTopLevelObj = lpFast->TopLevelObject;
	bltCreate.iPolyNum = lpFast->PolyNum;
	bltCreate.vecDelta = *pvecShot;
	bltCreate.pVehicle = pVehicle;
	bltCreate.penetrate = iPenetration;
	bltCreate.vecVel = lpAmmo->Velocity;
	bltCreate.uIsTracer = lpAmmo->AmmoType->UseParticlesForTracer;
	bltCreate.uAmmoSubType = lpAmmo->AmmoType->SubType;

	return fxBltCreate(&bltCreate);
}

/************************************************************************************************
	weapfxPlayExpSound
*************************************************************************************************/
UINT32 weapfxPlayExpSound(weapAmmo const* Ammo, FLOAT64VECTOR const* Pos)
{
	FLOAT64VECTOR TempPos;
	FLOAT32 fDistance = 0.0f;
	SINT32 iID;

	if (Ammo->AmmoType->NearFarSoundDist < 0.001f)
	{
		iID = fxSndGetNewSound();

		if(iID == -1)
			return 1;

		fxSndSetPitch(iID, 1.0f);
		fxSndSetVolume(iID, 1.0f);
		fxSndSetPriority(iID, IDE_TP_PRIORITY_MANDATORY);
		fxSndSetSoundType(iID, TYPE_FINITE_EXTERNAL_SOUND);
		fxSndSetSoundName(iID, Ammo->AmmoType->ExplosionNameNear);
		fxSndSetPosition(iID, Pos->x, Pos->y, Pos->z);
		fxSndSetPlayType(iID, DEF_PLAYTYPE_WORLD);
		fxSndSetMinDistance(iID, 100.0f);
		fxSndSetMaxDistance(iID, 3218.0f);
		fxSndPushFlags(iID, DEF_SOUND_STOPS_AMBIENCE);
		fxSndSetAmmo(iID, Ammo);

		// Required after settings
		if(!fxSndInitSound(iID))
			return 1;

		return 0;
	}

	TempPos.x = -playpos.x + WorldOriginOffset.x;
	TempPos.y = -playpos.y + WorldOriginOffset.y;

	// WorldOriginOffset.z is always 0
	TempPos.z =  playpos.z;

	mathSubVectors(&TempPos,&TempPos,Pos);

	fDistance = math3dFastSqrt((FLOAT32)(TempPos.x * TempPos.x +
										 TempPos.y * TempPos.y +
										 TempPos.z * TempPos.z));

	if (fDistance < Ammo->AmmoType->NearFarSoundDist)
	{
		iID = fxSndGetNewSound();

		if(iID == -1)
			return 1;

		fxSndSetPitch(iID, 1.0f);
		fxSndSetVolume(iID, 1.0f);
		fxSndSetPriority(iID, IDE_TP_PRIORITY_MANDATORY);
		fxSndSetSoundType(iID, TYPE_FINITE_EXTERNAL_SOUND);
		fxSndSetSoundName(iID, Ammo->AmmoType->ExplosionNameNear);
		fxSndSetPosition(iID, Pos->x, Pos->y, Pos->z);
		fxSndSetPlayType(iID, DEF_PLAYTYPE_WORLD);
		fxSndSetMinDistance(iID, 100.0f);
		fxSndSetMaxDistance(iID, 3218.0f);
		fxSndPushFlags(iID, DEF_SOUND_STOPS_AMBIENCE);
		fxSndSetAmmo(iID, Ammo);

		// Required after settings
		if(!fxSndInitSound(iID))
			return 1;

		return 0;
	}
	else
	{
		iID = fxSndGetNewSound();

		if(iID == -1)
			return 1;

		fxSndSetPitch(iID, 1.0f);
		fxSndSetVolume(iID, 1.0f);
		fxSndSetPriority(iID, IDE_TP_PRIORITY_HIGH);
		fxSndSetSoundType(iID, TYPE_FINITE_EXTERNAL_SOUND);
		fxSndSetSoundName(iID, Ammo->AmmoType->ExplosionNameFar);
		fxSndSetPosition(iID, Pos->x, Pos->y, Pos->z);
		fxSndSetPlayType(iID, DEF_PLAYTYPE_WORLD);
		fxSndSetMinDistance(iID, 100.0f);
		fxSndSetMaxDistance(iID, 3218.0f);
		fxSndPushFlags(iID, DEF_SOUND_STOPS_AMBIENCE);

		// Required after settings
		if(!fxSndInitSound(iID))
		{
			fxSndStopSound(iID);
			return 1;
		}

		return 0;
	}
}

/************************************************************************************************
	weapfxAmmoHitMark
	Hit marks associated with an explosion. Internal to this module only.
*************************************************************************************************/
UINT32 weapfxAmmoHitMark(weapAmmo const* Ammo,
						 piVECTOR const* pvecCollidePt,
						 piVECTOR const* pvecNormal,
						 OBJECT* lpCollideObj,
						 OBJECT* lpTopLevelObj,
						 void* pVehicle,
						 SINT32 iPenetration)
{
	FLOAT64VECTOR vecPos;
	FLOAT32 fRadius = 0.0f;
	FLOAT32 fEnergy = 0.0f;
	SINT32 iID = 0;

	vehVehicle*pVeh = (vehVehicle*)pVehicle;

	vecPos.x = pvecCollidePt->x;
	vecPos.y = pvecCollidePt->y;
	vecPos.z = pvecCollidePt->z;

	if(lpCollideObj)
		mathVectorMultiply64(&vecPos, &vecPos, &lpCollideObj->mat);

	vecPos.z = -vecPos.z;

	vecPos.x += WorldOriginOffset.x;
	vecPos.y += WorldOriginOffset.y;
	vecPos.z += WorldOriginOffset.z;

	// Need better way to do this.
	if(_IsWater(lpCollideObj))
	{
		switch(Ammo->AmmoType->SubType)
		{
		case WEAPAMMO_SUBTYPE_GRENADE:
		case WEAPAMMO_SUBTYPE_MORTAR:
		case WEAPAMMO_SUBTYPE_TORPEDO:
			{
				break;
			}
		case WEAPAMMO_SUBTYPE_BOMB:
			{
				TransformEffect* te = TransformEffect::AddEffect("bigshellwaterhit", Vec3f(0, 0, -1), 0, nullptr);

				if(te)
				{
					te->enableemitters(true);;
					te->setmatrix(Matrixf::translate(vecPos));
					te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
				}

				break;
			}
		default:
			{
				if(Ammo->AmmoType->ExplosiveEnergy > 30000000.0f)
				{
					TransformEffect* te = TransformEffect::AddEffect("bigshellwaterhit", Vec3f(0, 0, -1), 0, nullptr);

					if(te)
					{
						te->enableemitters(true);;
						te->setmatrix(Matrixf::translate(vecPos));
						te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
					}
				}
				else
				{
					TransformEffect* te = TransformEffect::AddEffect("shellwaterhit", Vec3f(0, 0, -1), 0, nullptr);

					if(te)
					{
						te->enableemitters(true);;
						te->setmatrix(Matrixf::translate(vecPos));
						te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
					}
				}
			}
		}

		return 0;
	}

	if(pVeh != nullptr)
	{
		if(pVeh->VehicleType != nullptr)
		{
			// No hit marks on troops
			if(pVeh->VehicleType->Type == VEH_TYPE_PERSON)
				return 0;
		}
	}

	iID = fxHitGetNewHit();

	if(iID == -1)
		return 1;

	fEnergy = (FLOAT32)pow(Ammo->AmmoType->ExplosiveEnergy, 0.333f);

	if(fEnergy <= 0.0001f)
		return 1;

	fRadius = fEnergy * 0.004f;

	fxHitSetEnergy(iID, fEnergy);
	fxHitSetNormal(iID, pvecNormal->x, pvecNormal->y, pvecNormal->z);
	fxHitSetAttachedObject(iID, lpCollideObj);
	fxHitSetTopLevelObject(iID, lpTopLevelObj);
	fxHitSetPosition(iID, vecPos.x, vecPos.y, vecPos.z);
	if ( pVeh != nullptr )
	  fxHitSetVehicle(iID, pVeh);
	fxHitSetRadius(iID, fRadius);
	fxHitSetCreateTime(iID, (FLOAT32)frametime);

	fxHitInitHit(iID);

	return 0;
}

/************************************************************************************************
	weapfxExplosion
	Use this for all explosions. If normal is nullptr, implies air explosion.
*************************************************************************************************/
static UINT32 weapfxExplosion(weapAmmo const* pAmmo,
					   FLOAT64VECTOR const* pPos,
					   piVECTOR const* pNormal,
					   bool bIsWater)
{
	SINT32 iID = -1;

	if(!pPos)
	{
		Write_Log(LOG_FX, LOG_WARN, "[weapfxExplosion] pPos is nullptr");
		return 1;
	}

	if(_isnan(pPos->x) || !_finite(pPos->x))
	{
		Write_Log(LOG_FX, LOG_WARN, "[weapfxExplosion] pPos->x is INFINITE");
		return 1;
	}

	if(_isnan(pPos->y) || !_finite(pPos->y))
	{
		Write_Log(LOG_FX, LOG_WARN, "[weapfxExplosion] pPos->y is INFINITE");
		return 1;
	}

	if(_isnan(pPos->z) || !_finite(pPos->z))
	{
		Write_Log(LOG_FX, LOG_WARN, "[weapfxExplosion] pPos->z is INFINITE");
		return 1;
	}

	if(!pAmmo)
	{
		Write_Log(LOG_FX, LOG_WARN, "[weapfxExplosion] pAmmo is nullptr");
		return 1;
	}

	if(!pAmmo->AmmoType)
	{
		Write_Log(LOG_FX, LOG_WARN, "[weapfxExplosion] pAmmo->AmmoType is nullptr");
		return 1;
	}

	if(pAmmo->AmmoType->ExplosiveEnergy < 0.0001f)
	{
		Write_Log(LOG_FX, LOG_WARN, "[weapfxExplosion] ExplosiveEnergy <= 0.0");
		return 1;
	}

	combattime = frametime;

	if(!bIsWater && pAmmo->AmmoType->hiteffect[0])
	{
		Vec3f n(0, 0, -1);

		if(pNormal)
		{
			n.set(*pNormal);
		}

		TransformEffect* te = TransformEffect::AddEffect(pAmmo->AmmoType->hiteffect, n, 0, nullptr);

		if(te)
		{
			Vec3d pos(*pPos);
			pos.z() = -pos.z();

			te->enableemitters(true);;
			te->setmatrix(Matrixf::translate(pos));
			te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
		}

		weapfxPlayExpSound(pAmmo, pPos);

		return 0;
	}
	else
	{
		if(pAmmo->AmmoType->SubType == WEAPAMMO_SUBTYPE_MORTAR)
		{
			if(bIsWater)
			{
				Vec3f n(0, 0, -1);

				TransformEffect* te = TransformEffect::AddEffect("watersplash", n, 0, nullptr);

				if(te)
				{
					Vec3d pos(*pPos);
					pos.z() = -pos.z();

					te->enableemitters(true);;
					te->setmatrix(Matrixf::translate(pos));
					te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
				}

				fxSndAddFiniteWorldSound("sounds/grenadesplash.wav",
										 5,
										 100,
										 IDE_TP_PRIORITY_LOW,
										 *pPos);
				return 0;
			}
		}
	}

	iID = fxExpGetNewExplosion();

	if(iID == -1)
		return 1;

	if(pAmmo->AmmoType->SubType == WEAPAMMO_SUBTYPE_TORPEDO)
	{
		Write_Log(LOG_FX, LOG_INFO, "[weapfxExplosion] Weapon is ground torpedo burst");
		fxExpSetType(iID, IDE_EXPTYPE_AMMOGROUND);
	}
	else if(pNormal == nullptr)
	{
		Write_Log(LOG_FX, LOG_INFO, "[weapfxExplosion] Weapon is air burst");
		fxExpSetType(iID, IDE_EXPTYPE_AMMOAIR);
	}
	else
	{
		Write_Log(LOG_FX, LOG_INFO, "[weapfxExplosion] Weapon hit object");
		fxExpSetNormal(iID, pNormal->x, pNormal->y, pNormal->z);
		fxExpSetType(iID, IDE_EXPTYPE_AMMOGROUND);
	}

	fxExpSetEnergy(iID, pAmmo->AmmoType->ExplosiveEnergy);
	fxExpSetSubType(iID, pAmmo->AmmoType->SubType);
	fxExpSetAmmo(iID, pAmmo);

	fxExpSetPosition(iID, pPos->x, pPos->y, pPos->z);

	fxExpSetVelocity(iID,
					 (FLOAT32)pAmmo->LastVel.x,
					 (FLOAT32)pAmmo->LastVel.y,
					 (FLOAT32)pAmmo->LastVel.z);

	fxExpSetIsWater(iID, bIsWater);
	fxExpSetPt1Time(iID, (FLOAT32)frametime);

	fxExpInitExplosion(iID);

	weapfxPlayExpSound(pAmmo, pPos);

	return 0;
}

/************************************************************************************************
	weapfxAmmoExpHitAttached
	Explosions for satchels only.
*************************************************************************************************/
UINT32 weapfxAmmoExpHitAttached(weapAmmo const* Ammo, SINT32 iPenetration)
{
	FLOAT64VECTOR vecPos;
	piVECTOR vecNorm;
	SINT32 id = -1;

	vecPos.x = Ammo->AttachedPnt.x;
	vecPos.y = Ammo->AttachedPnt.y;
	vecPos.z = Ammo->AttachedPnt.z;

	mathVectorMultiply64(&vecPos, &vecPos, &Ammo->AttachedObject->mat);
	vecPos.z = -vecPos.z;

	vecPos.x += WorldOriginOffset.x;
	vecPos.y += WorldOriginOffset.y;
	vecPos.z += WorldOriginOffset.z;

	id = fxExpGetNewExplosion();

	if(id == -1)
		return 1;

	fxExpSetPosition(id, vecPos.x, vecPos.y, vecPos.z);
	fxExpSetEnergy(id, Ammo->AmmoType->ExplosiveEnergy);
	fxExpSetSubType(id, Ammo->AmmoType->SubType);
	fxExpSetType(id, IDE_EXPTYPE_SATCHEL);

	fxExpSetVelocity(id,
					 (FLOAT32)Ammo->Velocity.x,
					 (FLOAT32)Ammo->Velocity.y,
					 (FLOAT32)Ammo->Velocity.z);

	bool bWater = _IsWater(Ammo->AttachedObject);

	if(Ammo->AttachedObject != nullptr)
	{
		vecNorm = Ammo->AttachedNorm;

		math3dVectorMultiply(&vecNorm,
							 &vecNorm,
							 &Ammo->AttachedObject->mat);

		vecNorm.x -= Ammo->AttachedObject->mat.m[3][0];
		vecNorm.y -= Ammo->AttachedObject->mat.m[3][1];
		vecNorm.z -= Ammo->AttachedObject->mat.m[3][2];

		fxExpSetIsWater(id, bWater);
	}
	else
	{
		vecNorm.x = (FLOAT32)-Ammo->Velocity.x;
		vecNorm.y = (FLOAT32)-Ammo->Velocity.y;
		vecNorm.z = (FLOAT32) Ammo->Velocity.z;
	}

	fxExpSetNormal(id, vecNorm.x, vecNorm.y, vecNorm.z);

	fxExpSetPt1Time(id, (FLOAT32)frametime);

	fxExpInitExplosion(id);

	if(!bWater)
	{
		weapfxAmmoHitMark(Ammo,
						  &Ammo->AttachedPnt,
						  &vecNorm,
						  Ammo->AttachedObject,
						  nullptr,
						  Ammo->AttachedToVehicle,
						  iPenetration);
	}

	weapfxPlayExpSound(Ammo, &vecPos);

	return 0;
}

/************************************************************************************************
	_IsWater
*************************************************************************************************/
bool _IsWater(const OBJECT* lpObj)
{
	if(lpObj == nullptr)
		return false;

	if(trnwtrIsWaterTexture(lpObj->texid))
	{
		return true;
	}

	FLOAT32 fRollCoef = 0.0f;
	mlGetObjectBumpinessAndFriction(nullptr,
									nullptr,
									nullptr,
									nullptr,
									&fRollCoef,
									nullptr,
									lpObj);

	vehVehicle*pVehicle = persGetPersonaVehiclePtr();

	if(pVehicle != nullptr)
	{
		if(pVehicle->VehicleType != nullptr)
		{
			if(pVehicle->VehicleType->Type == VEH_TYPE_SEA)
			{
				if(fRollCoef > 0.999f && fRollCoef < 1.001f)
				{
					return true;
				}
			}
			else if(fRollCoef > 198.99f && fRollCoef < 199.001f)
			{
				return true;
			}
		}
	}
	else if(fRollCoef > 198.99f && fRollCoef < 199.001f)
	{
		return true;
	}

	return false;
}

/************************************************************************************************
	weapfxAttachSmokeGrenadeToAmmo
*************************************************************************************************/
UINT32 weapfxAttachSmokeGrenadeToAmmo(weapAmmo const* pAmmo)
{
	return 0;
}

/************************************************************************************************
	weapfxDetonateSmokeGrenade
	Start billowing smoke after tracer dies.
	return 1 to kill the ammo; 0 to keep it around for the old behavior
*************************************************************************************************/
UINT32 weapfxDetonateSmokeGrenade(FLOAT64VECTOR* pos, const weapAmmo* ammo)
{
	UINT16 mslifetime = 47500 + ((rand() & 63) * 100);	// 47.5-53.8 seconds

	///TODO: Set color flag the way you want it
	///TODO: Pass attitude vector
	UINT32 colorFlag = 0;

	//piMATRIX matrix = IdentityMatrix;
	piVECTOR att;

	if(ammo->AmmoType->SubType == WEAPAMMO_SUBTYPE_MORTAR)
	{
		att.y = 0;
		att.z = (float)atan2(ammo->Velocity.x, ammo->Velocity.y);
		att.x = (float)atan2(ammo->Velocity.z, sqrt(ammo->Velocity.x * ammo->Velocity.x + ammo->Velocity.y * ammo->Velocity.y));
	}
	else
	{
		att.x = (float)ammo->Att.Pitch;
		att.y = (float)ammo->Att.Roll;
		att.z = (float)ammo->Att.Yaw;
	}

	/*
	mathEul_ToMatrix(&matrix, EulOrdXYZs, &att);

	piVECTOR dir = {0, 0, 1};

	math3dVectorMultiply(&dir, &dir, &matrix);

	// Force smoke to rise a bit.
	//dir.z = -0.2f;

	mathReallyFastNormalize(dir);

	// assume direction out the Y
	AttitudeVector attitude(-dir.x, -dir.y, -dir.z);
	*/
	AttitudeVector attitude(att.x, att.y, att.z);

	if(ammo->AmmoType->SubType != WEAPAMMO_SUBTYPE_MORTAR)
	{
		pos->z -= 0.02f;

		if(strcmp((const char*)ammo->AmmoType->Name, "uk_smokegrenade") == 0)
			colorFlag = SmokeGrenadeSTO::BRITISH;
		else if(strcmp((const char*)ammo->AmmoType->Name, "fr_smokegrenade") == 0)
			colorFlag = SmokeGrenadeSTO::FRENCH;
		else if(strcmp((const char*)ammo->AmmoType->Name, "desmokegrenade") == 0)
			colorFlag = SmokeGrenadeSTO::GERMAN;
	}

	WorldPosition_m position(pos->x, pos->y, pos->z);

	SmokeGrenadeSTO* sg = new SmokeGrenadeSTO(position, attitude, mslifetime, colorFlag);

	if(Online)
	{
		sg->request(PlayerClient()->ChatConnection());
		delete sg;
	}
	else
	{
		ServerTrackedObject::addToIndex(sg);
	}

	combattime = frametime;

	/*
	const char* modelPath = nullptr;

	if(strcmp((const char*)ammo->AmmoType->Name, "uk_smokegrenade") == 0)
		modelPath = "TPS\\meshes\\uk_gren_smk\\uk_gren_smk";
	else if(strcmp((const char*)ammo->AmmoType->Name, "fr_smokegrenade") == 0)
		modelPath = "TPS\\meshes\\fr_gren_smk\\fr_gren_smk";
	else if(strcmp((const char*)ammo->AmmoType->Name, "desmokegrenade") == 0)
		modelPath = "TPS\\meshes\\de_gren_smk\\de_gren_smk";

	if(modelPath)
	{
		gr2AnimAddCorpse(modelPath, *pos, matrix, 120000); // 2 minutes
		return 1;
	}
	*/

	return (colorFlag > 0);
}

void _soundKill(void* data)
{
	SINT32* handle = (SINT32*)data;

	*handle = 0;
}

void weapfxTorpedo(weapAmmo* ammo)
{
	SINT32 ID = 0;

	if(ammo->iParticleID == 0)
	{
		TransformEffect* te = TransformEffect::AddEffect("torpedotrail", Vec3f(0, 0, 0), 0, nullptr);

		if(te)
		{
			te->enableemitters(true);;
			te->setweapammo(ammo);
			te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
		}

		ammo->iParticleID = -1;
	}

	if(ammo->SoundID == 0)
	{
		ID = fxSndGetNewSound();

		if(ID == -1)
			return;

		fxSndSetSoundName(ID, "sounds\\torp_loop.wav");
		fxSndSetPriority(ID, IDE_TP_PRIORITY_LOW);
		fxSndSetSoundType(ID, TYPE_INFINITE_EXTERNAL_SOUND);
		fxSndSetPitch(ID, 1.0f);
		fxSndSetVolume(ID, 1.0f);
		fxSndSetPlayType(ID, DEF_PLAYTYPE_AMMO);
		fxSndSetMinDistance(ID, 20.0f);
		fxSndSetMaxDistance(ID, 1000.0f);
		fxSndSetAmmo(ID, ammo);
		fxSndSetKillCallback(ID, _soundKill);
		fxSndSetKillData(ID, &ammo->SoundID);

		if(!fxSndInitSound(ID))
		{
			fxSndStopSound(ID);
			return;
		}

		ammo->SoundID = ID;
	}
}

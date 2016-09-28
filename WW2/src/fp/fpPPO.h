#pragma once

#include "physics/colliderQuery.h"
#include "wwii/wwiiPPOLoader.h"

typedef bool (*PPOCallback)();

namespace phui { class View; }

struct PlayerPlacedObject
{
	PlayerPlacedObject(PPOSpec* s);
	virtual ~PlayerPlacedObject();

	//bool GetTransform(piMATRIX& t);
	bool Update();
	bool Build();
	bool Execute();
	void Interrupt();
	bool Modified();
	void SetMaterial(struct OBJECT* o);
	SINT32 CoolDown();

	void UpdateColliders();
	bool CalculatePlacement(piVECTOR& pos, piVECTOR& att, bool checksOnly = false);

	void SetInvalid(const char* key, bool force = false);

	// callbacks
	void SetUpdateCallback(PPOCallback uc);
	void SetExecuteCallback(PPOCallback ec);
	void SetValidityCallback(PPOCallback vc);

	struct OBJECT* obj;
	bool valid;
	bool lastValid;
	bool failOnInvalid;
	bool interruptNextUpdate;
	bool pauseBuild;

	dGeomID abox; // avoidance box

	PPOSpec* spec;

	colliderQuery collider;
	colliderQuery::CollisionList hits;

	Vec3d finalPos;
	piVECTOR finalAtt;

	SINT32 buildAt;
	SINT32 lastUpdate;
	phui::View* buildUI;

	PPOCallback updateCallback;
	PPOCallback executeCallback;
	PPOCallback validityCallback;
};

extern void fpPPOInitiateBuildAction(UINT32 & state, UINT32 & control, SINT32 & stance);

extern PlayerPlacedObject* fpPPOCreate(int type, bool save = true);
extern void fpPPOSwitchPPO(bool adjust);
extern bool fpPPOReady(int type);
extern PPOSpec* fpPPOGetSpec(int type);
extern void fpPPOSetSpec(int type);
extern void fpPPOCreateMaterial();
extern void fpPPOResetMaterial();

extern void fpPPOSet(PlayerPlacedObject* to);
extern PlayerPlacedObject* fpPPOGet();
extern bool fpPPOUpdate();
extern void fpPPOKill(bool release = true);
extern void fpPPOCancel(bool release = true);
extern void fpPPOAlert(const char* key, ...);
extern void fpPPOAlert(const wchar_t* message = nullptr);

#pragma once
// Date: Tue Sep  6 18:49:28 CDT 2005
//
// Defines the gr2Weapon class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Troy Lowe

#include "ml/mltypes.h"
#include "gr2/gr2GameObject.h"
#include "gr2/gr2Data.h"

class gr2DamageMap;
class gr2DamageComponent;
class gr2DamageListener;
struct gr2DamageModelSpec;

typedef std::vector<gr2DamageComponent*> gr2ComponentsList;
typedef std::vector<gr2DamageMap*> gr2DamageMapList;
typedef std::vector<int> gr2MeshIndexList;
typedef std::vector<gr2DamageListener*> gr2DamageListenerList;

class gr2DamageListener
{
public:

	gr2DamageListener(){}
	virtual ~gr2DamageListener(){}

	// allows the listener to key off incoming damage
	// to the component; such as degrade the listener's
	// performance, etc...
	virtual void HandleDamage(gr2DamageComponent* component) = 0;

};

class gr2DamageMap
{
public:

	gr2DamageMap(gr2DamageComponent* dc, gr2DamageMapSpec* spec);
	virtual ~gr2DamageMap(){}

	// allows this map to absorb damage based on its armor specs
	void AbsorbDamage(FLOAT32* damage);
	void UpdateVisualDamage();

	// get the common mesh index
	// between the hitmap and
	// the base skeleton
	int GetBoneIndex() const;
	int GetMeshIndex() const;

protected:

	UINT32 _threshold;
	UINT32 _armor;
	FLOAT32 _quality;
	FLOAT32 _class;

	// the index of the bone this map is
	// connected too hit map is used
	SINT32 _boneIndex;
	SINT32 _meshIndex;

	// damage model this map belongs to
	gr2DamageComponent* _damageComponent;

	// spec used to create this map
	gr2DamageMapSpec* _spec;

	// bones we'll use during visual damage
	gr2MeshIndexList _meshes;
};

class gr2DamageComponent
{
public:

	gr2DamageComponent(gr2DamageModel* dm, gr2DamageComponentSpec* spec);
	gr2DamageComponent(gr2DamageComponent* dc, gr2DamageComponentSpec* spec);
	virtual ~gr2DamageComponent();

	// get the damage state associated with this component
	gr2State* GetState();

	// based on the current points, calculate damage level
	void CalculateDamageLevel();

	// used by TPS states to arbitrarily set the damage level
	void SetDamageLevel(UINT32 l);

	// get the current level for the named component; returns -1
	UINT32 GetDamageLevel();

	// this components parent damage model
	gr2DamageModel* GetDamageModel();

	// this component's name
	std::string& GetName();

	// this components parent damage model
	bool HideOnDestroy() const;

	// lets a child alert a parent it has been destroyed
	void OnChildDestroyed(gr2DamageComponent* child);

	// outgoing flags
	void UpdateDamageFlags(UINT32& index, UINT32& flags);

	// incoming flags
	void SetDamageFlags(UINT32& index, UINT32& flags);

	// update's the component's visual damage state
	void UpdateVisualDamage();

	// destroy this component
	void Destroy();

	// is this component destroyed?
	bool IsDestroyed();

	// if we have no maps; we're a logical component
	bool IsLogicalComponent() const;

	// apply damage to this model
	int ApplyDamage(UINT32 location, FLOAT32 damage);

	// lets other objects listen for incoming damage
	void AddListener(gr2DamageListener* object);

	// lookup a component by bone index
	bool LookupComponent(UINT32 index, gr2DamageComponent** component);
	bool LookupComponent(const std::string& name, gr2DamageComponent** component);

	// get our list of components
	gr2ComponentsList& GetComponents();

protected:

	// hitpoints for this component
	UINT32 _points;

	// current damage level
	UINT32 _level;

	// component's name
	std::string _name;

	// spec used to create this component
	gr2DamageComponentSpec* _spec;

	// damage model this map belongs to
	gr2DamageModel* _damageModel;

	// damage state this component activates
	gr2State* _state;

	// damage model this map belongs to
	gr2DamageComponent* _parentComponent;

	// child components
	gr2ComponentsList _components;

	// damage maps
	gr2DamageMapList _maps;

	// other objects that want to know when
	// this component has been damaged
	gr2DamageListenerList _listeners;
};

class gr2DamageModel
{
public:

	gr2DamageModel(gr2GameObject* owner, gr2DamageModelSpec* spec);
	virtual ~gr2DamageModel();

	gr2ModelRef GetHitmap();
	gr2GameObject* GetOwner();
	int GetLevels(); // no damage + visible damages levels
	int GetDamageLevels(); // only visible damage levels

	std::string& GetHitmapTag();
	std::string& GetDamageTag();
	UINT32 GetDamageFlags();

	// get the current level for the named component; returns -1
	// if the component can't be found
	int GetComponentLevel(std::string& name);

	gr2ComponentsList& GetComponents();

	// loops over all components and calculates the damage flags
	// based on each component's current damage state; these
	// flags are sent out as visbits
	void UpdateDamageFlags();

	// sets damage states from some external flags;
	// ie: 3rd person visbits
	void SetDamageFlags(UINT32 flags);

	// updates all damage components and mesh visual damage
	void UpdateVisualDamage();

	// destroy this object
	void Destroy();

	// update's the model's visual damage state
	// for the named component
	void UpdateVisualDamage(std::string& name);

	// destroy a specific component
	void Destroy(std::string& name);

	// apply damage to this model
	void ApplyDamage(UINT32 location, FLOAT32 damage);

	// apply damage to this model
	gr2DamageComponent* LookupComponent(UINT32 index);
	gr2DamageComponent* LookupComponent(const std::string& name);

protected:

	// overall health of this object
	UINT32 _points;

	UINT32 _damageFlags;
	UINT32 _lastDamageFlags;

	gr2DamageModelSpec* _spec;

	gr2GameObject* _owner;

	gr2ModelRef _hitMap;

	gr2ComponentsList _components;

};


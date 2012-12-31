#include <syscfg.h> // Path is project specific.
#include "gr2/gr2DamageModel.h"
#include "gr2/gr2GameObject.h"
#include "gr2/gr2Data.h"
#include "include/vars.h"
#include "random.h"

gr2DamageMap::gr2DamageMap(gr2DamageComponent* dc, gr2DamageMapSpec* spec) :
	_damageComponent(dc),
	_spec(spec),
	_boneIndex(-1),
	_meshIndex(-1),
	_threshold(spec->threshold),
	_armor(spec->armor),
	_quality(1.0f),
	_class(spec->armorClass)
{
	std::string hmesh = dc->GetDamageModel()->GetHitmapTag();

	if(hmesh.size())
		hmesh += "_";

	hmesh += _spec->mesh;

	// lookup the hitmap mesh index
	gr2ModelHasMesh(dc->GetDamageModel()->GetHitmap(), hmesh.c_str(), &_meshIndex, &_boneIndex);

	// initialize visual damage for this mesh set
	UpdateVisualDamage();
}

void gr2DamageMap::AbsorbDamage(FLOAT32* damage)
{
	// not enough damage to effect us
	if(_armor == 0 || *damage <= _threshold)
	{
		*damage = 0;
		return;
	}

	if(_quality)
	{
		// our current available armor
		FLOAT32 armor		= _quality * _armor;

		// determine how much we'll absorb this hit
		FLOAT32 absorbed	= (GetRandom(0.0f, _quality) * _armor);

		// we absorbed more than there was damage
		if(absorbed >= *damage)
		{
			*damage = 0;
			return;
		}

		// take what we absorbed from damage
		*damage -= absorbed;

		// determine our absorbtoin rating
		float absorption = absorbed/armor;

		// if the rating is higher than our class allows
		// we degrade our armor a little bit
		if(absorption > _class)
		{
			_quality -= (absorption - _class);

			// make sure we're above 0
			if(_quality < 0)
				_quality = 0;
		}
	}
}

void gr2DamageMap::UpdateVisualDamage()
{
	int damageLevel		= _damageComponent->GetDamageLevel();
	gr2DamageModel* dm	= _damageComponent->GetDamageModel();
	gr2ModelRef lod		= dm->GetOwner()->GetLOD();
	bool destroyed		= (damageLevel >= dm->GetDamageLevels());

	if(lod)
	{
		int lastFoundMesh		= -1;
		int damLevelMesh		= -1;
		int currentMesh			= -1;

		char meshName[128+1];

		for(int i = 0; i < dm->GetLevels(); i++)
		{
			bool currentLevel = (damageLevel == i);
			meshName[0] = 0;

			if(i > 0)
				snprintf(meshName, 128, "%s%d_%s", dm->GetDamageTag().c_str(), i, _spec->mesh.c_str());
			else
				snprintf(meshName, 128, "%s", _spec->mesh.c_str());

			if(gr2ModelHasMesh(lod, meshName, &currentMesh, NULL))
			{
				// if we haven't found the damage level mesh yet
				// save the last found mesh because we made need it
				if(damLevelMesh < 0)
					lastFoundMesh = currentMesh;

				// if this is the current damage level and we found
				// a mesh for it; lets save it
				if(currentLevel)
					damLevelMesh = currentMesh;

				// set the mesh active or not based on if this is
				// the current damage level
				gr2SetMeshActive(lod, currentMesh, currentLevel);
			}
		}

		// this makes sure we have a visual mesh if no specific
		// damage level mesh was found and we're still within
		// the visual damage range of meshes (0 to GetDamageLevels() - 1)
		if((!destroyed || !_damageComponent->HideOnDestroy()) && damLevelMesh < 0 && lastFoundMesh >= 0)
			gr2SetMeshActive(lod, lastFoundMesh, true);
	}

	// map is destroyed
	if(destroyed)
		gr2SetMeshActive(dm->GetHitmap(), _meshIndex, false);
}

int gr2DamageMap::GetBoneIndex() const
{
	return _boneIndex;
}

int gr2DamageMap::GetMeshIndex() const
{
	return _meshIndex;
}

gr2DamageComponent::gr2DamageComponent(gr2DamageModel* dm, gr2DamageComponentSpec* spec) :
	_damageModel(dm),
	_parentComponent(NULL),
	_spec(spec),
	_name(spec->name),
	_state(NULL),
	_points(spec->points),
	_level(0)
{
	for(int i = 0; i < (int)_spec->maps.size(); i++)
		_maps.push_back(new gr2DamageMap(this, &(_spec->maps.at(i))));

	for(int i = 0; i < (int)_spec->components.size(); i++)
		_components.push_back(new gr2DamageComponent(this, &(_spec->components.at(i))));

	_state = dm->GetOwner()->GetState(_spec->state.c_str());
}

gr2DamageComponent::gr2DamageComponent(gr2DamageComponent* dc, gr2DamageComponentSpec* spec) :
	_damageModel(dc->GetDamageModel()),
	_parentComponent(dc),
	_spec(spec),
	_name(spec->name),
	_state(NULL),
	_points(spec->points),
	_level(0)
{
	for(int i = 0; i < (int)_spec->maps.size(); i++)
		_maps.push_back(new gr2DamageMap(this, &(_spec->maps.at(i))));

	for(int i = 0; i < (int)_spec->components.size(); i++)
		_components.push_back(new gr2DamageComponent(this, &(_spec->components.at(i))));

	_state = _damageModel->GetOwner()->GetState(_spec->state.c_str());
}

gr2DamageComponent::~gr2DamageComponent()
{
	for(int i = 0; i < (int)_maps.size(); i++)
	{
		gr2DamageMap* dm = _maps[i];
		delete dm;
	}

	for(int i = 0; i < (int)_components.size(); i++)
	{
		gr2DamageComponent* component = _components[i];
		delete component;
	}

	_maps.clear();
	_components.clear();
}

gr2DamageModel* gr2DamageComponent::GetDamageModel()
{
	return _damageModel;
}

std::string& gr2DamageComponent::GetName()
{
	return _name;
}

void gr2DamageComponent::CalculateDamageLevel()
{
	UINT32 levels		= _damageModel->GetLevels();
	FLOAT32 range		= (FLOAT32)(1.0f/levels);
	FLOAT32 damage		= (_spec->points) ? 1.0f - ((FLOAT32)_points / _spec->points): 0;

	//_level = 0;

	for(int i = 1; i < (int)levels; i++)
	{
		if((damage >= (i * range)) && (damage < ((i * range) + range)) && (UINT32)i > _level)
		{
			SetDamageLevel(i);

			if(IsDestroyed())
			{
				gr2DamageComponent* component = NULL;

				for(int d = 0; d < (int)_spec->destroys.size(); d++)
				{
					component = _damageModel->LookupComponent(_spec->destroys[d]);
					if(component && !component->IsDestroyed()) component->Destroy();
				}

				if(_parentComponent && _parentComponent->IsLogicalComponent())
					_parentComponent->OnChildDestroyed(this);
			}

			// let any listeners know about the change in level
			for(int l = 0; l < (int)_listeners.size(); l++)
				_listeners[l]->HandleDamage(this);

			break;
		}
	}

	_damageModel->UpdateDamageFlags();
}

void gr2DamageComponent::SetDamageLevel(UINT32 l)
{
	_level = l;

	if(_level > (UINT32)_damageModel->GetDamageLevels())
		_level = (UINT32)_damageModel->GetDamageLevels();

	if(_level && _state)
		_damageModel->GetOwner()->SetState(_state);
}

UINT32 gr2DamageComponent::GetDamageLevel()
{
	return _level;
}

gr2State* gr2DamageComponent::GetState()
{
	return _state;
}

void gr2DamageComponent::UpdateVisualDamage()
{
	for(int i = 0; i < (int)_maps.size(); i++)
		_maps[i]->UpdateVisualDamage();
}

void gr2DamageComponent::UpdateDamageFlags(UINT32& index, UINT32& flags)
{
	// we're a logical component if we have no maps
	if(!IsLogicalComponent())
	{
		// do my flags first
		if(_level) flags |= (1<<((index * _damageModel->GetDamageLevels()) + (_level - 1)));

		// increment the component index
		++index;
	}

	for(int i = 0; i < (int)_components.size(); i++)
		_components[i]->UpdateDamageFlags(index, flags);
}

void gr2DamageComponent::SetDamageFlags(UINT32& index, UINT32& flags)
{
	// we're a logical component if we have no maps
	if(!IsLogicalComponent())
	{
		UINT32 base = index * _damageModel->GetDamageLevels();

		// do my flags first
		for(UINT32 l = 0; l < (UINT32)_damageModel->GetDamageLevels(); l++)
		{
			if(flags & (1<<(base+l)))
			{
				SetDamageLevel(l + 1);
				break;
			}
		}

		// increment the component index
		++index;
	}

	for(int i = 0; i < (int)_components.size(); i++)
		_components[i]->SetDamageFlags(index, flags);
}

void gr2DamageComponent::Destroy()
{
	_points = 0;
	CalculateDamageLevel();
	UpdateVisualDamage();
}

bool gr2DamageComponent::IsDestroyed()
{
	if(IsLogicalComponent())
	{
		int children, destroyed;
		children = destroyed = 0;

		for(int i = 0; i < (int)_components.size(); i++)
		{
			++children;

			if(_components[i]->IsDestroyed())
				++destroyed;
		}

		return (destroyed && destroyed == children);
	}
	else
	{
		return (_level >= (UINT32)_damageModel->GetDamageLevels());
	}
}

int gr2DamageComponent::ApplyDamage(UINT32 location, FLOAT32 damage)
{
	for(int i = 0; i < (int)_maps.size(); i++)
	{
		if(_maps[i]->GetMeshIndex() == location)
		{
			_maps[i]->AbsorbDamage(&damage);
			break;
		}
	}

	// do damage if we had some
	if(damage > 0)
	{
		if(damage >= _points)
			_points = 0;
		else
			_points -= (UINT32)damage;

		// we've made a change, so re-calculate
		CalculateDamageLevel();
	}

	// return the current level
	return GetDamageLevel();
}

void gr2DamageComponent::AddListener(gr2DamageListener* object)
{
	bool found = false;

	for(int l = 0; l < (int)_listeners.size(); l++)
	{
		if(_listeners.at(l) == object)
		{
			found = true;
			break;
		}
	}

	if(!found)
		_listeners.push_back(object);
}

bool gr2DamageComponent::LookupComponent(UINT32 index, gr2DamageComponent** component)
{
	// check to see if this is our map index
	for(int m = 0; m < (int)_maps.size(); m++)
	{
		if(_maps[m]->GetMeshIndex() == index)
		{
			*component = this;
			return true;
		}
	}

	// nope, lets check my children to see if they own the index
	for(int i = 0; i < (int)_components.size(); i++)
	{
		if(_components[i]->LookupComponent(index, component))
			return true;
	}

	// we're not it
	return false;
}

bool gr2DamageComponent::LookupComponent(const std::string& name, gr2DamageComponent** component)
{
	if(_name == name)
	{
		*component = this;
		return true;
	}

	// nope, lets check my children to see if they own the index
	for(int i = 0; i < (int)_components.size(); i++)
	{
		if(_components[i]->LookupComponent(name, component))
			return true;
	}

	// we're not it
	return false;
}

gr2ComponentsList& gr2DamageComponent::GetComponents()
{
	return _components;
}

bool gr2DamageComponent::HideOnDestroy() const
{
	return _spec->hideOnDestroy;
}

bool gr2DamageComponent::IsLogicalComponent() const
{
	return !(_maps.size());
}

// logical components are destroyed when all child components are destroyed
void gr2DamageComponent::OnChildDestroyed(gr2DamageComponent* child)
{
	if(!_components.size())
		return;

	for(int i = 0; i < (int)_components.size(); i++)
	{
		if(!_components[i]->IsDestroyed())
			return;
	}

	// if we made it here, all child components are destroyed
	gr2DamageComponent* component = NULL;

	for(int d = 0; d < (int)_spec->destroys.size(); d++)
	{
		component = _damageModel->LookupComponent(_spec->destroys[d]);
		if(component && !component->IsDestroyed()) component->Destroy();
	}
}

gr2DamageModel::gr2DamageModel(gr2GameObject* owner, gr2DamageModelSpec* spec) :
	_owner(owner), _spec(spec), _damageFlags(0), _lastDamageFlags(0), _points(0)
{
	if(_spec->hitmap.size())
	{
		_hitMap = gr2LoadModel(_spec->hitmap.c_str(), _owner->GetSkeleton(), NULL, 1);
		if(_hitMap) gr2SetGR2Object(_hitMap, _owner);
	}

	for(int i = 0; i < (int)_spec->components.size(); i++)
		_components.push_back(new gr2DamageComponent(this, &(_spec->components.at(i))));
}

gr2DamageModel::~gr2DamageModel()
{
	if(_hitMap)
		gr2FreeModel(_hitMap);

	for(int i = 0; i < (int)_components.size(); i++)
	{
		gr2DamageComponent* component = _components[i];
		delete component;
	}

	_hitMap = NULL;
	_components.clear();
}

UINT32 gr2DamageModel::GetDamageFlags()
{
	if(_damageFlags != _lastDamageFlags)
	{
		_lastDamageFlags = _damageFlags;
		return _damageFlags;
	}

	return 0;
}

int gr2DamageModel::GetComponentLevel(std::string& name)
{
	gr2DamageComponent* component = LookupComponent(name);
	return (component) ? component->GetDamageLevel(): -1;
}

void gr2DamageModel::UpdateVisualDamage()
{
	for(int i = 0; i < (int)_components.size(); i++)
		_components[i]->UpdateVisualDamage();
}

void gr2DamageModel::UpdateVisualDamage(std::string& name)
{
	gr2DamageComponent* component = LookupComponent(name);
	if(component) component->UpdateVisualDamage();
}

void gr2DamageModel::UpdateDamageFlags()
{
	UINT32 index = 0;
	_damageFlags = 0;

	for(int i = 0; i < (int)_components.size(); i++)
		_components[i]->UpdateDamageFlags(index, _damageFlags);
}

void gr2DamageModel::SetDamageFlags(UINT32 flags)
{
	UINT32 index = 0;
	_damageFlags = flags;

	for(int i = 0; i < (int)_components.size(); i++)
		_components[i]->SetDamageFlags(index, _damageFlags);
}

void gr2DamageModel::Destroy()
{
	for(int i = 0; i < (int)_components.size(); i++)
		_components[i]->Destroy();
}

void gr2DamageModel::Destroy(std::string& name)
{
	gr2DamageComponent* component = LookupComponent(name);
	if(component) component->Destroy();
}

void gr2DamageModel::ApplyDamage(UINT32 location, FLOAT32 damage)
{
	if(!_owner)	return;

	// lookup the affected component
	gr2DamageComponent* component = LookupComponent(location);

	// apply damage; return's the current damage level; 0 to GetLevels()
	// any damage level greater than zero and the state is activated
	if(component)
		component->ApplyDamage(location, damage);
}

gr2DamageComponent* gr2DamageModel::LookupComponent(UINT32 index)
{
	gr2DamageComponent* component = NULL;

	for(int i = 0; i < (int)_components.size(); i++)
	{
		if(_components[i]->LookupComponent(index, &component))
			break;
	}

	return component;
}

gr2DamageComponent* gr2DamageModel::LookupComponent(const std::string& name)
{
	gr2DamageComponent* component = NULL;

	for(int i = 0; i < (int)_components.size(); i++)
	{
		if(_components[i]->LookupComponent(name, &component))
			break;
	}

	return component;
}

gr2ComponentsList& gr2DamageModel::GetComponents()
{
	return _components;
}

gr2ModelRef gr2DamageModel::GetHitmap()
{
	return _hitMap;
}

gr2GameObject* gr2DamageModel::GetOwner()
{
	return _owner;
}

int gr2DamageModel::GetLevels()
{
	return _spec->levels;
}

int gr2DamageModel::GetDamageLevels()
{
	return _spec->levels - 1;
}

std::string& gr2DamageModel::GetDamageTag()
{
	return _spec->dtag;
}

std::string& gr2DamageModel::GetHitmapTag()
{
	return _spec->htag;
}
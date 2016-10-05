#pragma once

//! @file wwiiSyncVar.h
//! @brief Provides WWII::SyncVar and WWII::VarGroup templates/classes.
//!
//! WWII::SyncVar provides a means of encapsulating a variable which
//! requires synchronization with one or more remote processes/consumers.
//!
//! WWII::VarGroup provides a container for one-or-more SyncVars and a
//! mechanism for describing which sub-fields have changed between any
//! two updates.
//!
//! @see Teul::Packer

namespace WWII
{

//! @class WWII::VarGroup
//! @brief Encapsulates a group of synchronized variables.

class VarGroup
{
public:
	VarGroup(TeulTimeMS& parent) : m_lastModified(0ULL), m_parent(parent), m_members(0) {}
	virtual ~VarGroup() {}

	VarGroup& operator = (const VarGroup& rhs) = delete;	// DO NOT USE!

protected:
	TeulTimeMS	m_lastModified;
	TeulTimeMS&	m_parent;
	UINT32		m_members;

public:
	void setModified()
	{
		m_parent = m_lastModified = teulClock64;
	}

	void addMember()
	{
		++m_members;
	}

	TeulTimeMS lastModified() const { return m_lastModified; }

	//! Function to pack member SyncVars, implemented by derived classes.
	void groupPacker(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS lastPacked, UINT32& changes)
	{
		return (mode == TEUL_PACK) ? groupPack(a, lastPacked, changes) : groupUnpack(a, lastPacked, changes);
	}

	virtual void groupPack(ACTION_OBJECT* const a, const TeulTimeMS lastPacked, UINT32& changes) const = 0;
	virtual void groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS lastPacked, UINT32& changes) = 0;

	//! Function to pack self if there are any changes, otherwise returns false.
	bool packer(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS lastPacked) { return (mode == TEUL_PACK) ? pack(a, lastPacked) : unpack(a, lastPacked); }
	bool pack(ACTION_OBJECT* const a, const TeulTimeMS lastPacked) const;
	bool unpack(ACTION_OBJECT* const a, const TeulTimeMS lastPacked);
};

//! @class WWII::SyncVar
//! @brief Encapsulates a variable intended to be synchronized between two or more consumers.
//!
//! Access to the member value is firewalled through a set of operator overloads which
//! should ensure that the "last-altered" time is modified whenever the variable is.

template<typename _Type, SINT32 _BitSize=0>
class SyncVar
{
public:
	typedef _Type valuetype;
	typedef SyncVar<_Type, _BitSize> selftype;

private:
	Teul::Packer<valuetype> m_value;
	TeulTimeMS	m_lastModified;
	class VarGroup* const m_parent;

	const valuetype& value() const { return m_value.m_value; }
	valuetype& value() { return m_value.m_value; }

	SyncVar() {}
	SyncVar(const selftype& rhs) {}
	SyncVar(const valuetype& rhs) {}

public:
	template<typename Tp>
	SyncVar(class VarGroup* const parent, const Tp& _default)
		: m_value(_default), m_lastModified(teulClock64), m_parent(parent) { m_parent->addMember(); }
	SyncVar(class VarGroup* const parent)
		: m_value(), m_lastModified(teulClock64), m_parent(parent) { m_parent->addMember(); }

	valuetype operator = (const valuetype& rhs) { set(rhs); return value(); }
	selftype& operator = (const selftype& rhs) { m_value = rhs.m_value; m_lastModified = rhs.m_lastModified; return *this; }

	SINT32 bitSize() const { return _BitSize; }

	selftype& setModified()
	{
		m_parent->setModified();
		m_lastModified = teulClock64;
		return *this;
	}

	void set(const valuetype& rhs)
	{
		if ( rhs == value() )
			return;
		value() = rhs;
		setModified();
	}

	const valuetype& get() const { return value(); }
	const valuetype& operator()() const { return value(); }
	void operator()(const valuetype& rhs) { set(rhs); }

	selftype& operator+=(const valuetype& rhs) { value() += rhs; return setModified(); }
	selftype& operator-=(const valuetype& rhs) { value() -= rhs; return setModified(); }
	selftype& operator*=(const valuetype& rhs) { value() *= rhs; return setModified(); }
	selftype& operator/=(const valuetype& rhs) { value() /= rhs; return setModified(); }
	selftype& operator<<=(const int rhs) { value() <<= rhs; return setModified(); }
	selftype& operator>>=(const int rhs) { value() >>= rhs; return setModified(); }
	selftype& operator++() { ++value(); return setModified(); }
	selftype& operator--() { --value(); return setModified(); }
	selftype& operator++(int) { ++value(); return setModified(); }
	selftype& operator--(int) { --value(); return setModified(); }
	selftype& operator&=(const valuetype& rhs) { value() &= rhs; return setModified(); }
	selftype& operator|=(const valuetype& rhs) { value() |= rhs; return setModified(); }
	selftype& operator^=(const valuetype& rhs) { value() ^= rhs; return setModified(); }

	valuetype operator!() { return !get(); }

	//! Packers which only self-pack when data is modified.
	void pack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes, UINT32 fieldNo) const
	{
		const bool hasChanged = m_lastModified >= lastPack;
		if ( hasChanged )
		{
			changes |= (1 << fieldNo);
			m_value.pack(a, _BitSize);
		}
	}

	void unpack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes, UINT32 fieldNo)
	{
		const bool hasChanged = (changes & (1 << fieldNo)) != 0;
		if ( hasChanged )
			m_value.unpack(a, _BitSize);
	}
};

} // WWII namespace

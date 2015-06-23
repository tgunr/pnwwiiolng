#pragma once
// Copyright (C) PlayNet Inc, 2011 -- all rights reserved.

#pragma pack(push, 1)
//! @struct XYZTuple
//! @brief Vector class optimized for communicating values across the network.
template<typename Type_>
struct XYZTuple
{
	typedef Type_ value_type;
	typedef XYZTuple<value_type> selftype;

	//! Default zeroing constructor.
	XYZTuple<Type_>() : m_x(0), m_y(0), m_z(0) {}

	//! Memberwise constructor.
	XYZTuple<Type_>(const value_type& x_, const value_type& y_, const value_type& z_)
		: m_x(x_), m_y(y_), m_z(z_)
		{}

	//! Instantiate from any other vector type that implements x(), y() and z() members.
	template<typename _RhsType>
	XYZTuple(_RhsType rhs_)
		: m_x(rhs_.x())
		, m_y(rhs_.y())
		, m_z(rhs_.z())
		{}

	//! Add vectors.
	template<typename RhsType_>
	selftype& operator += (RhsType_& rhs_)
	{
		m_x += rhs_.x();
		m_y += rhs_.y();
		m_z += rhs_.z();
		return *this;
	}

	//! Subtract vectors.
	template<typename RhsType_>
	selftype& operator -= (RhsType_& rhs_)
	{
		m_x -= rhs_.x();
		m_y -= rhs_.y();
		m_z -= rhs_.z();
		return *this;
	}

	//! Add vectors and return new.
	template<typename RhsType_>
	selftype operator + (RhsType_& rhs_) const
	{
		return XYZTuple<value_type>(x() + rhs_.x(), y() + rhs_.y(), z() + rhs_.z());
	}

	//! Subtract vectors and return new.
	template<typename RhsType_>
	selftype operator - (RhsType_& rhs_) const
	{
		return XYZTuple<value_type>(x() - rhs_.x(), y() - rhs_.y(), z() - rhs_.z());
	}

	//! Copy members from another vector type that implements x(), y() and z()
	template<typename RhsType_>
	selftype& operator = (RhsType_& rhs_)
	{
		m_x = rhs_.x();
		m_y = rhs_.y();
		m_z = rhs_.z();
		return *this;
	}

	//! Equals operator with any other vector type that implements x(), y() and z()
	template<typename RhsType_>
	bool operator == (RhsType_& rhs_)
	{
		return ( (x() == rhs_.x()) & (y() == rhs_.y()) & (z() == rhs_.z()) );
	}

	//! Not-Equals operator with any other vector type that implements x(), y() and z()
	template<typename RhsType_>
	bool operator != (RhsType_& rhs_)
	{
		return ( (x() != rhs_.x()) | (y() != rhs_.y()) | (z() != rhs_.z()) );
	}

#pragma pack(push, 1)
	value_type	m_x, m_y, m_z;
	value_type	x() const { return m_x; }
	value_type	y() const { return m_y; }
	value_type	z() const { return m_z; }
#pragma pack(pop)
};
#pragma pack(pop)


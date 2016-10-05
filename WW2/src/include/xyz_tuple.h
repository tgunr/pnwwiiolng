#pragma once
// Copyright (C) PlayNet Inc, 2011 -- all rights reserved.

#pragma pack(push, 1)
//! @struct XYZTuple
//! @brief Vector class optimized for communicating values across the network.
template<typename Type_>
struct XYZTuple
{
	using value_type = Type_;
	using self_type = XYZTuple<value_type>;

	//! Default zeroing constructor.
	XYZTuple() noexcept : m_x(0), m_y(0), m_z(0) {}

	//! Member-wise constructor.
	XYZTuple(const value_type& x_, const value_type& y_, const value_type& z_) noexcept
		: m_x(x_), m_y(y_), m_z(z_)
		{}

	//! Instantiate from any other vector type that implements x(), y() and z() members.
	template<typename _RhsType>
	XYZTuple(_RhsType rhs_) noexcept
		: m_x(rhs_.x())
		, m_y(rhs_.y())
		, m_z(rhs_.z())
		{}

	//! Add vectors.
	template<typename RhsType_>
	self_type& operator += (const RhsType_& rhs_) noexcept
	{
		m_x += rhs_.x();
		m_y += rhs_.y();
		m_z += rhs_.z();
		return *this;
	}

	//! Subtract vectors.
	template<typename RhsType_>
	self_type& operator -= (const RhsType_& rhs_) noexcept
	{
		m_x -= rhs_.x();
		m_y -= rhs_.y();
		m_z -= rhs_.z();
		return *this;
	}

	//! Add vectors and return new.
	template<typename RhsType_>
	self_type operator + (const RhsType_& rhs_) const noexcept
	{
		self_type result = *this;
		result += rhs_;
		return result;
	}

	//! Subtract vectors and return new.
	template<typename RhsType_>
	self_type operator - (const RhsType_& rhs_) const noexcept
	{
		self_type result = *this;
		result -= rhs_;
		return result;
	}

	//! Copy members from another vector type that implements x(), y() and z()
	template<typename RhsType_>
	self_type& operator = (const RhsType_& rhs_) noexcept
	{
		m_x = rhs_.x();
		m_y = rhs_.y();
		m_z = rhs_.z();
		return *this;
	}

	//! Equals operator with any other vector type that implements x(), y() and z()
	template<typename RhsType_>
	bool operator == (const RhsType_& rhs_) const noexcept
	{
		return ( (x() == rhs_.x()) & (y() == rhs_.y()) & (z() == rhs_.z()) );
	}

	//! Not-Equals operator with any other vector type that implements x(), y() and z()
	template<typename RhsType_>
	bool operator != (const RhsType_& rhs_) const noexcept
	{
		return ( (x() != rhs_.x()) | (y() != rhs_.y()) | (z() != rhs_.z()) );
	}

#pragma pack(push, 1)
	value_type	m_x = 0., m_y = 0., m_z = 0.;
	value_type	x() const { return m_x; }
	value_type	y() const { return m_y; }
	value_type	z() const { return m_z; }
#pragma pack(pop)
};
#pragma pack(pop)


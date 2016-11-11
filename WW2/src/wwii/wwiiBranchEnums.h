#pragma once
//! \brief	Branch enumerations.

//tolua_begin
enum BRANCH_ENUM
{
	BRANCH_NOT_USED,			//!< Unused/Undefined branch.
	BRANCH_LAND,				//!< Infantry/ground forces.
	BRANCH_AIR,					//!< Air forces.
	BRANCH_SEA,					//!< Naval forces.
	MAX_BRANCHES				//!< 1-based "max" index.
};
//tolua_end


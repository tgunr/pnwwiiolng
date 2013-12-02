#pragma once

//! Player mission flags.
struct MISS_FLAGS
{
	MISS_FLAGS()
		: canEject(false), checksumsDirty(false)
		{}
	bool	canEject:1;				//!< Player can be ejected.
	bool	checksumsDirty:1;		//!< Player has invalid checksums.
};


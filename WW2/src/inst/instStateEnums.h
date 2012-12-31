#pragma once

//tolua_begin
namespace Instance
{
	enum State
	{
		Invalid					//!< Bad data.
	,	HostCtored				//!< C++ layer constructed, waiting for Lua layer initialization.
	,	LuaCtored				//!< Blessed by lua layer, pending population.
	,	MinPopMet				//!< Low-water threshold met, preparing to launch.
	,	CanSpawnIn				//!< Can spawn, can't play. Not implemented.
	,	Active					//!< Play started, spawning allowed, new people can still join.
	,	Finishing				//!< Playing, but no new joins of the instance. Not implemented.
	,	Closed					//!< Playing, but no new joins or spawns. Not implemented.
	,	Completed				//!< No-longer playing, scored, may have spawned a follow-on.
	} ;
}
//tolua_end

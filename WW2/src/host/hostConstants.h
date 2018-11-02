#pragma once
// Inter-host callbacks.

namespace Host
{
	enum { CallbackX = 222 };

	enum class Callbacks : unsigned char
	{
		MovePlayer_Y		=	0
	,	ForceDespawn_Y
	,	LuaPacket_Y // no longer used but kept so the enum stays in order
	,	CaptureScore_Y
	,	CaptureState_Y
	,	ResupplyIssue_Y
	,	ResupplyAccepted_Y
	};
}

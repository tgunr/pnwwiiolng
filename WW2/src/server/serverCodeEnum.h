#pragma once

namespace Server
{
	enum class Code : uint32_t
	{
		Added = 1,
		Exited,
		Replaced,
		Status,
		Deleted,
		TimedOut,
		InsertError
	};
}

#pragma once
// Copyright (C) PlayNet Inc 1999
#include <functional>

// This struct gets used for the callback allocation routines. A caveat
// of the original developers having a fetish for using structures rather
// than function parameters...

#include "teul/teulFuncTypes.h"

namespace Teul
{
	struct CallbackInit
	{
		CallbackInit()
			: sendAction(nullptr)
			, recvAction(nullptr)
			, error(nullptr)
			, callX(0), callY(0)
			, callXName(""), callYName("")
		{}

		ACTION_FN	sendAction;		//! Function for sending data or NULL.
		ACTION_FN	recvAction;		//! Function for receiving data or NULL.
		ERROR_FUNCTION error;	//! Error function.

		UCHAR		callX;			//! X position in the callback matrix.
		UCHAR		callY;			//! Y position in the callback matrix.
		const char  *callXName;
		const char  *callYName;
	};
} // namespace Teul

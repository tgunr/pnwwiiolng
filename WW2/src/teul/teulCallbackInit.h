#pragma once
// Copyright (C) PlayNet Inc 1999
#include <functional>

// This struct gets used for the callback allocation routines. A caveat
// of the original developers having a fetish for using structures rather
// than function parameters...

namespace Teul
{

	typedef std::function<SINT32(ACTION_OBJECT*)> ACTION_FN;

	struct CallbackInit
	{
		CallbackInit()
			: sendAction(nullptr)
			, recvAction(nullptr)
			, error(nullptr)
			, callX(0), callY(0)
		{}

		ACTION_FN	sendAction;		//! Function for sending data or NULL.
		ACTION_FN	recvAction;		//! Function for receiving data or NULL.
		ERROR_FUNCTION* error;	//! Error function.

		UCHAR		callX;			//! X position in the callback matrix.
		UCHAR		callY;			//! Y position in the callback matrix.
	};

} // namespace Teul

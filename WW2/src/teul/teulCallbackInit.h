#pragma once
// Copyright (C) PlayNet Inc 1999

// This struct gets used for the callback allocation routines. A caveat
// of the original developers having a fetish for using structures rather
// than function parameters...

namespace Teul
{

struct CallbackInit
{
	CallbackInit()
		: sendAction(NULL)
		, recvAction(NULL)
		, error(NULL)
		, callX(0), callY(0)
	{}

	ACTION_REF	sendAction;		//! Function for sending data or NULL.
	ACTION_REF	recvAction;		//! Function for receiving data or NULL.
	ERROR_FUNCTION* error;	//! Error function.

	UCHAR		callX;			//! X position in the callback matrix.
	UCHAR		callY;			//! Y position in the callback matrix.
} ;

} // namespace Teul

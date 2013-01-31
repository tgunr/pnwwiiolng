#pragma once
// Copyright (C) PlayNet Inc, 2011
// Various typedefs for function objects.

//! Teul callback functions "MUST" have the following form.
typedef SINT32	(ACTION)	( struct ACTION_OBJECT* action );
typedef ACTION*	ACTION_REF;

//! Callbacks that accept a new connection take the following form.
//	teulIndex is the opaque connection reference for the new connection
typedef void			(CONNECT_FUNCTION)(struct ENDPOINT& endpoint) ;

//! Teul error handlers must take this format.
typedef void (ERROR_FUNCTION) (SINT32 teulIndex, const UCHAR* const data, SINT32 size, const struct TEUL_CALLBACK* const object, SINT32 error);

#include <functional>
typedef std::function<SINT32(struct ACTION_OBJECT*)> ACTION_FN;


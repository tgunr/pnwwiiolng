#pragma once
// Copyright (C) PlayNet Inc, 2011
// Various typedefs for function objects.

//! Teul callback functions "MUST" have the following form.
using ACTION = SINT32(*)(struct ACTION_OBJECT* action);

//! Callbacks that accept a new connection take the following form.
//	teulIndex is the opaque connection reference for the new connection
using CONNECT_FUNCTION = void(*)(struct ENDPOINT& endpoint);

//! Teul error handlers must take this format.
using ERROR_FUNCTION = void(*)(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const struct TEUL_CALLBACK* const object, SINT32 error);

#include <functional>
using ACTION_FN = std::function<SINT32(struct ACTION_OBJECT*)>;


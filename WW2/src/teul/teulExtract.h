#pragma once
// Copyright (C) Playnet Inc 2011
// Created: Mon Oct  3 15:27:13 CDT 2011
// Originating Programmer: Oliver "kfsone" Smith

namespace Teul
{

//! Retrieve a specific type value from a stream and
//! advance the stream pointer past the value.
template<typename _Type>
const _Type& ExtractAndAdvance(const UCHAR*& from)
{
	const _Type& value = *(_Type*)from;
	from += sizeof(_Type);
	return value;
}

}

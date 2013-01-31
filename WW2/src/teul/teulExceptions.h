#pragma once
// Copyright (C) PlayNet Inc, 2011.
// Originating Programmer: Oliver "kfs1" Smith
// Created: Tue Aug 23 22:55:51 CDT 2011
//
// @file teul/teulExceptions.h
// @brief TeulKit Exception classes.

namespace Teul
{
	//! Generic teulKit error of an unspecified type.
	struct Error
	{
		Error(const char* const msg_, const SINT32 error_) : msg(msg_), error(error_) {}
		const char* msg ;
		SINT32 error ;
	} ;

	//! Error specific to a teulIndex.
	struct IndexError : public Error
	{
		IndexError(SINT32 teulIndex_, const char* const msg_, const SINT32 error_) : Error(msg_, error_), teulIndex(teulIndex_) {}
		IndexError(SINT32 teulIndex_, const SINT32 error_) : Error("Error", error_), teulIndex(teulIndex_) {}
		SINT32 teulIndex ;
	} ;

	//! Error that occurred at a specific iteration of a loop.
	struct IterationError : public Error
	{
		IterationError(size_t iter_, const char* const msg_, const SINT32 error_) : Error(msg_, error_), iter(iter_) {}
		size_t iter ;
	} ;

	//! Error associated with processing a range of values.
	//! @note The indexes should be numeric.
	struct RangeError : public Error
	{
		RangeError(size_t start_, size_t end_, const char* const msg_, const SINT32 error_) : Error(msg_, error_), start(start_), end(end_) {}
		size_t start, end ;
	} ;
}

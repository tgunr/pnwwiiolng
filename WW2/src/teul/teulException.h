#pragma once
// Copyright (C) PlayNet Inc, 2012 -- All rights reserved.
// Created:
// Originating Programmer: Oliver "kfsone" Smith
//! @brief Provides the Teul::Exception class.

namespace Teul
{
	class Exception
	{
	public:
		Exception(const SINT32 teulIndex_, const char* const reason_, const signed int code_)
			: m_teulIndex(teulIndex_), m_code(code_)
			{
				strncpy(m_reason, reason_, sizeof(m_reason) - 1);
				m_reason[sizeof(m_reason) - 1] = 0;
			}

	private:
		SINT32	m_teulIndex;
		char	m_reason[512];
		SINT32	m_code;

	public:
		SINT32 teulIndex() const { return m_teulIndex; }
		const char* const what() const { return m_reason; }
		SINT32 code() const { return m_code; }
	};
}


#pragma once

#include <memory.h>			// For malloc.

//! @class StrDupPtr
//! @brief Wrapper for strdup assignments that auto-frees them when they go out of scope.

class StrDupPtr
{
	char*	m_ptr = nullptr;
	size_t	m_length = 0;

public:
	StrDupPtr() noexcept = default;
	StrDupPtr(const char* const str_) noexcept { _initialize(str_, str_ ? strlen(str_) : 0);	}
	StrDupPtr(const char* const str_, size_t length_) noexcept { _initialize(str_, length_); }
	StrDupPtr(const StrDupPtr& rhs_) noexcept { copy(rhs_); }
	StrDupPtr(StrDupPtr&& rhs_) noexcept : m_ptr(rhs_.m_ptr), m_length(rhs_.m_length) { rhs_.m_ptr = nullptr; }
	StrDupPtr& operator = (const StrDupPtr& rhs_) noexcept { copy(rhs_); return *this; }
	StrDupPtr& operator = (const char* const str_) noexcept { copy(str_); return *this; }
	StrDupPtr& operator = (StrDupPtr&& rhs_) noexcept { std::swap(m_ptr, rhs_.m_ptr); std::swap(m_length, rhs_.m_length); return *this; }
	~StrDupPtr() noexcept { release(); }

	operator const char* () const noexcept { return m_ptr; }
	const char* str() const noexcept { return m_ptr; }
	char* str() noexcept { return m_ptr; }

	size_t size() const noexcept { return m_ptr ? m_length + 1 : 0; }
	size_t length() const noexcept { return m_ptr ? m_length : 0; }

public:
	void copy(const char* const str_) noexcept
	{
		release();
		if ( str_ != nullptr )
			_initialize(str_, strlen(str_));
	}
	void copy(const char* const str_, size_t length_) noexcept
	{
		release();
		_initialize(str_, length_);
	}
	void copy(const StrDupPtr& rhs_) noexcept
	{
		release();
		_initialize(rhs_.m_ptr, rhs_.m_length);
	}

	bool release() noexcept
	{
		if (m_ptr == nullptr)
			return false;
		free((void*)m_ptr);
		m_ptr = nullptr;
		m_length = 0;
		return true;
	}

private:
	void _initialize(const char* str_, size_t length_) noexcept
	{
		if ( str_ != nullptr && length_ > 0 )
		{
			char* newStr = static_cast<char*>(malloc(length_ + 1));
			ABORT_IF(newStr == nullptr);
			memcpy(newStr, str_, length_);
			newStr[length_] = 0;
			m_ptr = newStr;
			m_length = length_;
		}
	}
};


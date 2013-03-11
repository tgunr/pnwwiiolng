#pragma once

#include <memory.h>			// For malloc.

//! @class StrDupPtr
//! @brief Wrapper for strdup assignments that auto-frees them when they go out of scope.

class StrDupPtr
{
	char*	m_ptr;
	size_t	m_length;

public:
	StrDupPtr() : m_ptr(NULL), m_length(0) {}
	StrDupPtr(const char* const str_) : m_ptr(nullptr) { _initialize(str_, str_ ? strlen(str_) : 0);	}
	StrDupPtr(const char* const str_, size_t length_) : m_ptr(nullptr) { _initialize(str_, length_); }
	StrDupPtr(const StrDupPtr& rhs_) : m_ptr(nullptr) { copy(rhs_); }
	StrDupPtr(StrDupPtr&& rhs_) : m_ptr(nullptr) { m_ptr = rhs_.m_ptr; m_length = rhs_.m_length; if ( &rhs_ != this ) { rhs_.m_ptr = nullptr; }; }
	StrDupPtr& operator = (const StrDupPtr& rhs_) { copy(rhs_); return *this; }
	StrDupPtr& operator = (const char* const str_) { copy(str_); return *this; }
	StrDupPtr& operator = (StrDupPtr&& rhs_) { if ( &rhs_ != this ) { m_ptr = rhs_.m_ptr; m_length = rhs_.m_length; rhs_.m_ptr = nullptr; }; return *this; }
	~StrDupPtr() { release(); }

	operator const char* () const { return m_ptr; }
	const char* str() const { return m_ptr; }
	char* str() { return m_ptr; }

	size_t size() const { return m_ptr ? m_length + 1 : 0; }
	size_t length() const { return m_ptr ? m_length : 0; }

public:
	void copy(const char* const str_)
	{
		release();
		if ( str_ != NULL )
			_initialize(str_, strlen(str_));
	}
	void copy(const char* const str_, size_t length_)
	{
		release();
		_initialize(str_, length_);
	}
	void copy(const StrDupPtr& rhs_)
	{
		release();
		_initialize(rhs_.m_ptr, rhs_.m_length);
	}

	bool release()
	{
		if (m_ptr == NULL)
			return false;
		free((void*)m_ptr);
		m_ptr = NULL;
		m_length = 0;
		return true;
	}

private:
	void _initialize(const char* str_, size_t length_)
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


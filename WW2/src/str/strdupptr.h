#pragma once

//! @class StrDupPtr
//! @brief Wrapper for strdup assignments that auto-frees them when they go out of scope.

class StrDupPtr
{
	char* m_ptr;

public:
	StrDupPtr() : m_ptr(NULL) {}
	StrDupPtr(const char* const str_) : m_ptr(str_ ? strdup(str_) : NULL) {}
	StrDupPtr(const StrDupPtr& rhs_) : m_ptr(rhs_.m_ptr ? strdup(rhs_.m_ptr) : NULL) {}
	StrDupPtr& operator = (const StrDupPtr& rhs_) { copy(rhs_.m_ptr); return *this; }
	StrDupPtr& operator = (const char* const str_) { copy(str_); return *this; }
	~StrDupPtr() { release(); }

	operator const char* () const { return m_ptr; }
	const char* str() const { return m_ptr; }
	char* str() { return m_ptr; }

public:
	void copy(const char* const str_)
	{
		release();
		if ( str_ != NULL )
			m_ptr = strdup(str_);
	}

	bool release()
	{
		if (m_ptr == NULL)
			return false;
		free((void*)m_ptr);
		m_ptr = NULL;
		return true;
	}
};


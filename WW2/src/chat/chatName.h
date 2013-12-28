#pragma once

// Containers for holding chat names.

#include "chat/chatHandle.h"			// for chatHandle functions
#include "str/stringLib.h"				// for strcpysafe

namespace Chat
{
	template <class _CharType> struct name_traits;

	template <>
	struct name_traits<char>
	{
		enum { MaxHandleLen = MAX_HANDLE_LEN, MaxHandleSize = MAX_HANDLE_SIZE };
	};
	template <>
	struct name_traits<wchar_t>
	{
		enum { MaxHandleLen = MAX_HANDLE_WLEN, MaxHandleSize = MAX_HANDLE_WSIZE };
	};

	template <class _CharType, class _Traits = name_traits<_CharType>>
	struct BasicName : public _Traits
	{
		typedef _CharType value_type;
		typedef BasicName<_CharType> self_type;

		value_type m_name[_Traits::MaxHandleSize];

		//! Default ctor, initializes name to empty string.
		BasicName()
		{
			m_name[0] = 0;
		}

		//! String ctor.
		BasicName(const _CharType* name)
		{
			strsafecpy(m_name, name);
		}

		//! Ctor from handle value.
		BasicName(ChatHandle handle)
		{
			this->operator=(handle);
		}

		//! Copy constructor.
		BasicName(const self_type& rhs)
		{
			this->operator=(rhs);
		}

		//! Copy from string operator.
		BasicName& operator = (const _CharType* rhs)
		{
			if (m_name != rhs)
				strsafecpy(m_name, rhs);
			return *this;
		}

		//! Copy operator.
		BasicName& operator = (const self_type& rhs)
		{
			if (m_name != rhs.m_name)
				strsafecpy(m_name, rhs.m_name);
			return *this;
		}

		//! Assign from handle.
		BasicName& operator = (const ChatHandle handle)
		{
			chatGetCallsign(m_name, handle);
			return *this;
		}

		//! Return a pointer to the string representation.
		const _CharType* c_str() const { return m_name; }
		operator const _CharType* () const { return c_str(); }

		//! Convert to a handle (not-cheap).
		ChatHandle Handle() const { return chatGetHandle(m_name); }
		operator ChatHandle() const { return Handle(); }

		//! Return handle in lowercase, non-caching.
		ChatHandle LowercaseHandle() const { return chatGetLowercaseHandle(m_name); }

		//! Convert to upper-case
		void ToUpper() { strupr(m_name); }

		//! Set the name to empty.
		void Clear() { m_name[0] = 0; }

		//! Ensure all bytes are zero
		void Zero() { memset(m_name, 0, sizeof(m_name)); }

		//! Whether or not the name is populated with anything.
		bool Empty() const { return (m_name[0] == 0); }

		//! Test if the current string is legal as a name.
		bool IsValid() const { return chatIsValidCallsign(m_name); }

		//! Tests if the current name uses non-ascii characters - in a char-backed
		//! name, these would be utf8 characters.
		bool IsWideCallsign() const { return chatIsWideCallsign(m_name); }

		//! Tests equality between two names.
		bool operator == (const value_type* rhs) const;
		bool operator == (const self_type& rhs) const;
		bool operator != (const value_type* rhs) const { return !operator==(rhs); }
		bool operator != (const self_type& rhs) const { return !operator==(rhs); }

		void packRaw(unsigned int mode, ACTION_OBJECT* a) { a->packBytes(mode, m_name, sizeof(m_name)); }

		int format(const value_type* format, ...);
	};

	typedef BasicName<char> Name;
	typedef BasicName<wchar_t> NameW;

	template<> inline bool BasicName<char>::operator == (const char* rhs) const { return strcmp(m_name, rhs) == 0; }
	template<> inline bool BasicName<char>::operator == (const BasicName<char>& rhs) const { return strcmp(m_name, rhs.m_name) == 0; }
	template<> inline bool BasicName<wchar_t>::operator == (const wchar_t* rhs) const { return wcscmp(m_name, rhs) == 0; }
	template<> inline bool BasicName<wchar_t>::operator == (const BasicName<wchar_t>& rhs) const { return wcscmp(m_name, rhs.m_name) == 0; }

	template<> inline int BasicName<char>::format(const char* format, ...) { va_list ap; va_start(ap, format); int ret = vsnprintf(m_name, MaxHandleSize, format, ap); va_end(ap); return ret;  }
	template<> inline int BasicName<wchar_t>::format(const wchar_t* format, ...) { va_list ap; va_start(ap, format); int ret = vswprintf(m_name, MaxHandleSize, format, ap); va_end(ap); return ret; }
}

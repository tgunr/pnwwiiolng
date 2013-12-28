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

		//! Copy operator.
		BasicName& operator = (const self_type& rhs)
		{
			if (m_name != rhs.m_name)
				strsafecpy(m_name, rhs.m_name);
			return *this;
		}

		//! Copy from string operator.
		BasicName& operator = (const _CharType* rhs)
		{
			if (m_name != rhs)
				strsafecpy(m_name, rhs);
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

		//! Set the name to empty.
		void Clear() { m_name[0] = 0; }

		//! Whether or not the name is populated with anything.
		bool Empty() const { return (m_name[0] == 0); }

		//! Test if the current string is legal as a name.
		bool IsValid() const { return chatIsValidCallsign(m_name); }

		//! Tests if the current name uses non-ascii characters - in a char-backed
		//! name, these would be utf8 characters.
		bool IsWideCallsign() const { return chatIsWideCallsign(m_name); }

		//! Tests equality between two names.
		bool operator == (const self_type& rhs);
		bool operator == (const value_type* rhs);
		bool operator != (const self_type& rhs) { return !operator==(rhs); }
		bool operator != (const value_type* rhs) { return !operator==(rhs); }
	};

	typedef BasicName<char> Name;
	typedef BasicName<wchar_t> NameW;

	template<> inline bool BasicName<char>::operator == (const BasicName<char>& rhs) { return strcmp(m_name, rhs.m_name) == 0; }
	template<> inline bool BasicName<char>::operator == (const char* rhs) { return strcmp(m_name, rhs) == 0; }
	template<> inline bool BasicName<wchar_t>::operator == (const BasicName<wchar_t>& rhs) { return wcscmp(m_name, rhs.m_name) == 0; }
	template<> inline bool BasicName<wchar_t>::operator == (const wchar_t* rhs) { return wcscmp(m_name, rhs) == 0; }
}

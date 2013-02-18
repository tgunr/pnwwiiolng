#pragma once

#include "str/strdupptr.h"

enum ConfigVarType
{
	CVT_STRING,
	CVT_UNSIGNED_INT,
	CVT_SIGNED_INT
};

namespace Teul
{
	//! @class Teul::ConfigVar
	//! @brief Describes a configuration variable along with it's supposed type, default value and optional environment variable source.
	//! @detail Configuration variables describe a runtime manipulatable game setting which starts with
	//! a compile time default value that can be superceded by environment variables.
	//! The type is used to constrain how we access the variable.
	template<ConfigVarType VarType_>
	class ConfigVar
	{
	public:
		inline ConfigVar(const char* const name_, const char* const default_, const char* const envVarName_=nullptr);
		inline ~ConfigVar();

		enum { ValueType = VarType_ };
		typedef typename ConfigVar<VarType_> self_type;

	public:
		const char* Get() const { return m_value; }
		inline const char* GetString() const;
		inline const UINT64& GetUnsigned() const;
		inline const SINT64& GetSigned() const;

		inline void Set(const char* newValue_);
		inline void SetString(const char* newValue_);
		inline void SetUnsigned(const UINT64 newValue_);
		inline void SetSigned(const SINT64 newValue_);

		CONSTEXPR_FN bool IsNumber() const { return VarType_ == CVT_UNSIGNED_INT || VarType_ == CVT_SIGNED_INT; }

	public:
		// Assignment operator copies values, not names.
		self_type& operator = (const self_type& rhs_)
		{
			Set(rhs_.Get());
			return *this;
		}
		self_type& operator = (const char* const rhs_)
		{
			Set(rhs_);
			return *this;
		}

	private:
		enum { MaxIntStringSize = 22 };

		StrDupPtr		m_name;
		char*			m_value;

		union
		{
			const char*	um_ptr;
			UINT64		um_uint64;
			SINT64		um_sint64;
		} m_valueUnion;
	};

	//////////
	// Implementations.
	template<ConfigVarType Type_>
	ConfigVar<Type_>::ConfigVar(const char* const name_, const char* const default_, const char* const envVarName_)
		: m_name(name_)
		, m_value(nullptr)
	{
		ABORT_IF(name_ == nullptr);

		m_valueUnion.um_ptr = nullptr;

		if ( IsNumber() )
			m_value = static_cast<char*>(calloc(1, MaxIntStringSize));

		if ( default_ != nullptr )
			Set(default_);

		if ( envVarName_ )
		{
			const char* envValue = getenv(envVarName_);
			if ( envValue != nullptr )
				Set(envValue);
		}
	}

	template<ConfigVarType Type_>
	ConfigVar<Type_>::~ConfigVar()
	{
		if ( m_value != nullptr )
			free(m_value);
	}

	// The compiler will specialize this routine as needed.
	template<>
	void ConfigVar<CVT_STRING>::Set(const char* const newValue_)
	{
		SetString(newValue_);
	}

	template<>
	void ConfigVar<CVT_UNSIGNED_INT>::Set(const char* const newValue_)
	{
		if ( newValue_ != nullptr && *newValue_ )
			SetUnsigned(strtoull(newValue_, NULL, 10));
		else
		{
			m_valueUnion.um_uint64 = 0ULL;
			m_value[0] = '\0';
		}
	}

	template<>
	void ConfigVar<CVT_SIGNED_INT>::Set(const char* const newValue_)
	{
		if ( newValue_ != nullptr && *newValue_ )
			SetSigned(strtoll(newValue_, NULL, 10));
		else
		{
			m_valueUnion.um_sint64 = 0ULL;
			m_value[0] = '\0';
		}
	}

	template<> const char* ConfigVar<CVT_STRING>::GetString() const { return m_valueUnion.um_ptr;	}
	template<> const UINT64& ConfigVar<CVT_UNSIGNED_INT>::GetUnsigned() const { return m_valueUnion.um_uint64; }
	template<> const SINT64& ConfigVar<CVT_SIGNED_INT>::GetSigned() const { return m_valueUnion.um_sint64; }

	template<> void ConfigVar<CVT_STRING>::SetString(const char* const newValue_)
	{
		if ( newValue_ == m_value )
			return;
		if ( m_value )
			free(m_value);
		m_value = newValue_ ? strdup(newValue_) : nullptr;
		m_valueUnion.um_ptr = m_value;
	}

	template<> void ConfigVar<CVT_UNSIGNED_INT>::SetUnsigned(const UINT64 newValue_)
	{
		if ( newValue_ == GetUnsigned() )
			return;
		m_valueUnion.um_uint64 = newValue_;
		snprintf(m_value, MaxIntStringSize, "%llu", newValue_);
	}

	template<> void ConfigVar<CVT_SIGNED_INT>::SetSigned(const SINT64 newValue_)
	{
		if ( newValue_ == GetSigned() )
			return;
		m_valueUnion.um_sint64 = newValue_;
		snprintf(m_value, MaxIntStringSize, "%lld", newValue_);
	}

} // namespace Teul

#define CVAR_STRING(Name_, Default_, EnvName_)	\
	Teul::ConfigVar<CVT_STRING> cv_##Name_ ( #Name_, Default_, EnvName_ )

#define CVAR_UINT(Name_, Default_, EnvName_)	\
	Teul::ConfigVar<CVT_UNSIGNED_INT> cv_##Name_ ( #Name_, Default_, EnvName_ )

#define CVAR_SINT(Name_, Default_, EnvName_)	\
	Teul::ConfigVar<CVT_SIGNED_INT> cv_##Name_ ( #Name_, Default_, EnvName_ )

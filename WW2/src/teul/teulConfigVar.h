#pragma once

#include "dbug/dbug.h"			// For LogAbort
#include "str/strdupptr.h"

enum ConfigVarType
{
	CVT_STRING,
	CVT_UNSIGNED_INT,
	CVT_SIGNED_INT,
	CVT_BOOL
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
		ConfigVar(const char* const name_, const char* const default_, const char* const envVarName_, bool required);
		inline ~ConfigVar();

		enum { ValueType = VarType_ };
		typedef typename ConfigVar<VarType_> self_type;

	public:
		const char* Get() const { return m_value; }
		inline const char* GetString() const;
		inline const UINT32& GetUint32() const;
		inline const SINT32& GetSint32() const;
		inline bool GetBool() const;

		const char* GetName() const { return m_name; }
		const char* GetEnvVarName() const { return m_envVarName; }

		inline void Set(const char* newValue_);
		inline void SetString(const char* newValue_);
		inline void SetUint32(const UINT32 newValue_);
		inline void SetSint32(const SINT32 newValue_);
		inline void SetBool(bool newValue_);

		CONSTEXPR_FN bool IsNumber() const { return VarType_ == CVT_UNSIGNED_INT || VarType_ == CVT_SIGNED_INT; }

		// String's provide an operator const char*
		operator const char*() const { return GetString(); }
		operator const UINT32&() const { return GetUint32(); }
		operator const SINT32&() const { return GetSint32(); }
		operator bool() const { return GetBool(); }

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
		enum { MaxIntStringSize = 11 };

		StrDupPtr		m_name;
		const char*		m_envVarName;
		char*			m_value;
		bool			m_required;

		union
		{
			const char*	um_ptr;
			UINT32		um_uint32;
			SINT32		um_sint32;
			bool		um_bool;
		} m_union;
	};

	//////////
	// Implementations.
	template<ConfigVarType Type_>
	ConfigVar<Type_>::~ConfigVar()
	{
		if ( Type_ != CVT_BOOL && m_value != nullptr )
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
			SetUint32(strtoul(newValue_, NULL, 10));
		else
		{
			m_union.um_uint32 = 0ULL;
			m_value[0] = '\0';
		}
	}

	template<>
	void ConfigVar<CVT_SIGNED_INT>::Set(const char* const newValue_)
	{
		if ( newValue_ != nullptr && *newValue_ )
			SetSint32(strtol(newValue_, NULL, 10));
		else
		{
			m_union.um_sint32 = 0ULL;
			m_value[0] = '\0';
		}
	}

	template<> void ConfigVar<CVT_BOOL>::Set(const char* const newValue_);

	template<> const char* ConfigVar<CVT_STRING>::GetString() const { return m_union.um_ptr; }
	template<> const UINT32& ConfigVar<CVT_UNSIGNED_INT>::GetUint32() const { return m_union.um_uint32; }
	template<> const SINT32& ConfigVar<CVT_SIGNED_INT>::GetSint32() const { return m_union.um_sint32; }
	template<> bool ConfigVar<CVT_BOOL>::GetBool() const { return m_union.um_bool; }

	template<> void ConfigVar<CVT_STRING>::SetString(const char* const newValue_);
	template<> void ConfigVar<CVT_UNSIGNED_INT>::SetUint32(const UINT32 newValue_);
	template<> void ConfigVar<CVT_SIGNED_INT>::SetSint32(const SINT32 newValue_);
	template<> void ConfigVar<CVT_BOOL>::SetBool(bool newValue_);

} // namespace Teul

typedef Teul::ConfigVar<CVT_STRING> ConfigStr;
typedef Teul::ConfigVar<CVT_UNSIGNED_INT> ConfigUint;
typedef Teul::ConfigVar<CVT_SIGNED_INT> ConfigSint;
typedef Teul::ConfigVar<CVT_BOOL> ConfigBool;

#define CVAR_STRING(Name_, Default_, EnvName_, Required_, Description_)	\
	ConfigStr cv_##Name_ ( #Name_, Default_, EnvName_, Required_ )

#define CVAR_UINT(Name_, Default_, EnvName_, Description_)	\
	ConfigUint cv_##Name_ ( #Name_, Default_, EnvName_, false )

#define CVAR_SINT(Name_, Default_, EnvName_, Description_)	\
	ConfigSint cv_##Name_ ( #Name_, Default_, EnvName_, false )

#define CVAR_BOOL(Name_, Default_, EnvName_, Description_)	\
	ConfigBool cv_##Name_ ( #Name_, Default_, EnvName_, false )

#pragma once

#include "dbug/dbug.h"			// For LogAbort
#include "str/strdupptr.h"

enum ConfigVarType
{
	CVT_STRING,
	CVT_UNSIGNED_INT,
	CVT_SIGNED_INT,
	CVT_BOOL,
	CVT_IP_ADDRESS,
};

namespace Teul
{

	//! @class Teul::ConfigVar
	//! @brief Describes a configuration variable along with it's supposed type, default value and optional environment variable source.
	//! @detail Configuration variables describe a runtime manipulatable game setting which starts with
	//! a compile time default value that can be superceded by environment variables.
	//! The type is used to constrain how we access the variable.
	class ConfigVar
	{
	public:
		ConfigVar(ConfigVarType type_, const char* name_, const char* default_, const char* envVarName_, bool required_);
		~ConfigVar();

		// Assignment operator copies values, not names.
		ConfigVar& operator = (const ConfigVar& rhs_)
		{
			Set(rhs_.Get());
			return *this;
		}
		ConfigVar& operator = (const char* rhs_)
		{
			Set(rhs_);
			return *this;
		}

		const char* Get() const { return m_value; }
		const char* GetName() const { return m_name; }
		const char* GetEnvVarName() const { return m_envVarName; }
		ConfigVarType GetType() const { return m_type; }

		bool IsNumber() const { return (m_type == CVT_UNSIGNED_INT || m_type == CVT_SIGNED_INT); }
		void Set(const char* str_);

		const char* GetString() const { return m_value; }
		UINT32 GetUint32() const { return m_union.um_uint32; }
		SINT32 GetSint32() const { return m_union.um_sint32; }
		bool GetBool() { return m_union.um_bool; }

		void SetString(const char* newValue_);
		void SetUint32(const char* newValue_);
		void SetSint32(const char* newValue_);
		void SetBool(const char* newValue_);

		void SetUint32(UINT32 newValue_);
		void SetSint32(SINT32 newValue_);
		void SetBool(bool newValue_);

	protected:
		void SetStringValue(const char* str_);
		void SetUint32Value(const char* str_);
		void SetSint32Value(const char* str_);
		void SetBoolValue(const char* str_);
		void SetIPAddressValue(const char* str_);

		void SetUint32Value(UINT32 value_);
		void SetSint32Value(SINT32 value_);
		void SetBoolValue(bool value_);
		void SetIPAddressValue(UINT32 value_);

	protected:
		enum { MaxIntStringSize = 12, MaxIPAddressSize = 65 };

		StrDupPtr		m_name;
		ConfigVarType	m_type;
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

} // namespace Teul

typedef Teul::ConfigVar ConfigStr;
typedef Teul::ConfigVar ConfigUint;
typedef Teul::ConfigVar ConfigSint;
typedef Teul::ConfigVar ConfigBool;
typedef Teul::ConfigVar ConfigAddr;

#define CVAR_STRING(Name_, Default_, EnvName_, Required_, Description_)	\
	ConfigStr cv_##Name_ ( CVT_STRING, #Name_, Default_, EnvName_, Required_ )

#define CVAR_UINT(Name_, Default_, EnvName_, Description_)	\
	ConfigUint cv_##Name_ ( CVT_UNSIGNED_INT, #Name_, Default_, EnvName_, false )

#define CVAR_SINT(Name_, Default_, EnvName_, Description_)	\
	ConfigSint cv_##Name_ ( CVT_SIGNED_INT, #Name_, Default_, EnvName_, false )

#define CVAR_BOOL(Name_, Default_, EnvName_, Description_)	\
	ConfigBool cv_##Name_ ( CVT_BOOL, #Name_, Default_, EnvName_, false )

#define CVAR_ADDR(Name_, Default_, EnvName_, Required_, Description_)	\
	ConfigAddr cv_##Name_ ( CVT_IP_ADDRESS, #Name_, Default_, EnvName_, Required_ )


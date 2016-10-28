#pragma once

#include "str/strdupptr.h"

enum ConfigVarType
{
	CVT_STRING,
	CVT_UNSIGNED_INT,
	CVT_SIGNED_INT,
	CVT_BOOL,
	CVT_IP_ADDRESS,
	CVT_PATH,
	// Insert new entries above NUM_CONFIG_VAR_TYPES
	NUM_CONFIG_VAR_TYPES
};

namespace Teul
{

	//! @class Teul::ConfigVar
	//! @brief Describes a configuration variable along with it's supposed type, default value and optional environment variable source.
	//! @details Configuration variables describe a runtime manipulatable game setting which starts with
	//! a compile time default value that can be superseded by a config file which can be superseded by environment variables.
	//! The type is used to constrain how we access the variable.
	class ConfigVar
	{
	public:
		ConfigVar(ConfigVarType type_, const char* name_, const char* default_, const char* envVarName_, bool required_, const char* description_);
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

		const char* Get() const { ABORT_IF(!m_initialized); return m_value; }
		const char* GetName() const { return m_name.str(); }
		const char* GetEnvVarName() const { return m_envVarName; }
		ConfigVarType GetType() const { return m_type; }
		const char* GetDescription() const { return m_description; }
		const char* GetDefault() const { return m_default; }
		bool GetRequired() const { return m_required; }

		bool IsNumber() const { return (m_type == CVT_UNSIGNED_INT || m_type == CVT_SIGNED_INT); }
		void Set(const char* str_);

		const char* GetString() const { ABORT_IF(!m_initialized); return m_value; }
		UINT32 GetUint32() const { ABORT_IF(!m_initialized); return m_union.um_uint32; }
		SINT32 GetSint32() const { ABORT_IF(!m_initialized); return m_union.um_sint32; }
		bool GetBool() { ABORT_IF(!m_initialized); return m_union.um_bool; }

		void SetString(const char* newValue_);
		void SetUint32(const char* newValue_);
		void SetSint32(const char* newValue_);
		void SetBool(const char* newValue_);

		void SetUint32(UINT32 newValue_);
		void SetSint32(SINT32 newValue_);
		void SetBool(bool newValue_);

		//! Load in all configuration values.
		static void InitializeConfigVars();
		//! Will be true if the general purpose defaults file was loaded.
		static bool LoadedGeneralConfig();
		//! Will be true if the os-specific defaults file was loaded.
		static bool LoadedOSConfig();
		//! Will be true if we loaded either defaults file.
		static bool LoadedConfig() { return LoadedGeneralConfig() || LoadedOSConfig(); }

		//! Dump current config vars to stdout and exit.
		static void PrintCurrentVars();

		//! Generate a default config file to specified filename and exit.
		static void Generate(const char* filename_);

		//! Check that all required variables are set.
		static void CheckConfiguration();

	protected:
		void Initialize();

		void SetStringValue(const char* str_);
		void SetUint32Value(const char* str_);
		void SetSint32Value(const char* str_);
		void SetBoolValue(const char* str_);
		void SetIPAddressValue(const char* str_);
		void SetPathValue(const char* str_);

		void SetUint32Value(UINT32 value_);
		void SetSint32Value(SINT32 value_);
		void SetBoolValue(bool value_);
		void SetIPAddressValue(UINT32 value_);

	protected:
		enum { MaxIntStringSize = 12, MaxIPAddressSize = 65 };

		StrDupPtr			m_name;
		ConfigVarType		m_type;
		const char*			m_envVarName;
		const char* const	m_description;
		const char* const	m_default;
		char*				m_value;
		bool				m_required;
		bool				m_initialized;

		union
		{
			const char*	um_ptr;
			UINT32		um_uint32;
			SINT32		um_sint32;
			bool		um_bool;
		} m_union;
	};

} // namespace Teul

// The plan was to use templates to make the underlying types more efficient,
// with only the functionality present in the type being enabled and present
// at compile time - so you'd get a compile error, for instance, if you tried
// to get an integer from a string value.
// While that's not terribly complicated for any given compiler, it defeated
// reasonable levels of readability to get it to work for all three platforms.

using ConfigStr = Teul::ConfigVar;
using ConfigUint = Teul::ConfigVar;
using ConfigSint = Teul::ConfigVar;
using ConfigBool = Teul::ConfigVar;
using ConfigAddr = Teul::ConfigVar;
using ConfigPath = Teul::ConfigVar;

// $Id: pnRegion.h 102 2009-01-09 20:42:45Z osmith@CRS $
// Defines the Region structure and variables

#include <string>
#include <vector>
#include <stdexcept>

class PNRegion
{
public:
	static const unsigned int NUM_STRINGS = 3;

public:
	PNRegion() : m_name(L""), m_authSuffix(""), m_webSuffix("") {}
	PNRegion(const std::wstring& _name, const std::string& _authSuffix, const std::string& _webSuffix)
		: m_name(_name), m_authSuffix(_authSuffix), m_webSuffix(_webSuffix)
		{}
	~PNRegion() {}

	const std::wstring& name() const { return m_name; }
	const std::string& authSuffix() const { return m_authSuffix; }
	const std::string& webSuffix() const { return m_webSuffix; }

	std::wstring	m_name;
	std::string		m_authSuffix;
	std::string		m_webSuffix;
};

class region_error : public std::runtime_error
{
public:
    explicit region_error(const char* _name, const char* error)
        : runtime_error(error), m_name(_name) {}
	virtual ~region_error() _THROW0() {};
    const char* name() const { return m_name; }

protected:
    const char* m_name;
};

extern PNRegion g_region;

#include <syscfg.h>
#include <Windows.h>
#include <string>

#ifdef CRASH_HANDLER

//////////////////////////////////////////////////////////////////////////////
// Includes
//
// All the various includes that are not needed if the CRASH_HANDLER flag is disabled - that's
// technically all of them, but the compiler complains if we don't include Windows.h unconditionally.
#pragma region Includes
#include <comdef.h>
#include <Wbemidl.h>
#include <wincrypt.h>
#include <WinInet.h>
#include <stdarg.h>
#include <math.h>
#include <float.h>
#include <ShlObj.h>
#include <fcntl.h>
#include <VersionHelpers.h>
#include <iostream>
#include <fstream>
#include <locale>
#include <codecvt>
#include "versionNo.h"
#include "client_revision.h"
#include "sys/sysreg.h"
#include "unique_opaque_ptr.hpp"
#include "client/windows/handler/exception_handler.h"

#pragma endregion
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Configuration
//
// These are things we might tweak - note that because this is a really cheap implementation, we
// separate out all the URL components (like http:// vs https://) with individual vars here.
#pragma region Configuration

static const std::wstring HockeyApp_appIkey = L"11c7ac3fb886457eb6f56b266ff52ba3";
static const INTERNET_PORT HockeyApp_serverPort = INTERNET_DEFAULT_HTTPS_PORT;
static const bool HockeyApp_SSL = true;
static const std::wstring HockeyApp_serverName = L"sdk.hockeyapp.net";
static const std::wstring HockeyApp_apiPath = L"/api/2/apps/" + HockeyApp_appIkey + L"/crashes/upload";
static const std::wstring HockeyApp_userAgent = L"HockeySDK/0.1 (Windows custom)";

#pragma endregion
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// RAII utilities
//
// Contains wrappers for:
//	- Plain file descriptors (always _close() on exiting scope)
//	- COM (CoInitializeEx() on construction, CoUninitialize() on exiting scope if succeeded)
//	- WinInet (call InternetCloseHandle() on exiting scope)
#pragma region RAII Utilities

// Yet another thing which should be in fio
struct FD {
	int m_fd = -1;
	FD() {}
	FD(const FD &) = delete;
	FD(FD &&) = delete;
	FD(int fd) : m_fd(fd) {}
	FD(const wchar_t *filename, int openflag, int shareflag, int permissionflag) { _wsopen_s(filename, openflag, shareflag, permissionflag); }
	~FD() { if (m_fd != -1) { _close(m_fd); } }
	FD &_wsopen_s(const wchar_t *filename, int openflag, int shareflag, int permissionflag) {
		if (m_fd != -1) { _close(m_fd); }
		::_wsopen_s(&m_fd, filename, openflag, shareflag, permissionflag);
		return *this;
	}
	FD &operator=(const FD &) = delete;
	FD &operator=(FD &&) = delete;
	operator int() const { return m_fd; }
	explicit operator bool() const { return m_fd != -1; }
};

// COM RAII helper
struct CCom {
	HRESULT r;
	CCom() : CCom(nullptr, COINIT_APARTMENTTHREADED) {}
	CCom(LPVOID pvReserved, DWORD dwCoInit) { r = CoInitializeEx(pvReserved, dwCoInit); }
	~CCom() { if (SUCCEEDED(r) || r == S_FALSE) { CoUninitialize(); } }
	explicit operator bool() const { return SUCCEEDED(r) || r == S_FALSE; }
	CCom(const CCom &) = delete;
	CCom &operator=(const CCom &) = delete;
	CCom(CCom &&) = delete;
	CCom &operator=(CCom &&) = delete;
};

DEFINE_UNIQUE_OPAQUE_PTR_WITH_DELETER(HINTERNET, InternetCloseHandle);

#pragma endregion
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Global Context
//
// The structure defining the data we have available at crash time, including the crash handler
// object itself. This is actually passed as context to the crash callback below, but is also a
// global regardless so that the singleton context can be used to uninstall the crash handler
// if for some strange reason we wanted to do so.
#pragma region Global Context

struct CrashHandlerContext
{
	std::unique_ptr<google_breakpad::ExceptionHandler> crashHandler;
	std::wstring installationId;
	std::wstring machineModel; // All that WMI stuff is DEFINITELY not crash-safe, but std::wstring::c_str() is
	std::wstring machineManufacturer; // All that WMI stuff is DEFINITELY not crash-safe, but std::wstring::c_str() is
	std::wstring windowsVersion; // All that WMI stuff is DEFINITELY not crash-safe, but std::wstring::c_str() is
	std::wstring windowsBuild; // All that WMI stuff is DEFINITELY not crash-safe, but std::wstring::c_str() is
};
static std::unique_ptr<CrashHandlerContext> gCrashContext;

#pragma endregion
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Miscellaneous Utilities
//
// Bits and pieces of useful things.
#pragma region Miscellaneous Utilities

static bool ends_with(const std::wstring &s, const std::wstring &suffix)
{
	return s.compare(s.length() - suffix.length(), suffix.length(), suffix) == 0;
}

template <size_t characters>
static std::string GenerateRandomString()
{
	static_assert(characters % sizeof(unsigned int) == 0, "Must be an even number of unsigned ints");
	std::string result;

	// Officially a multipart boundary may consist of DIGIT / ALPHA / '()+_,-./:=?, but for simplicity we
	// just Base64-encode some random values. This maximizes effective use of the entropy in the values.
	std::unique_ptr<unsigned int[]> valueBuffer = std::make_unique<unsigned int[]>(characters / sizeof(unsigned int));
	// Just use rand_s(); CryptGenRandom() needs a CRYPTPROV handle, RtlGetRandom() requires LoadLibrary()/GetProcAddress().
	for (size_t i = 0; i < characters / sizeof(unsigned int); ++i) {
		rand_s(&valueBuffer[i]); // rand_s() only errors if the parameter is NULL
	}
	// Encode to Base64
	DWORD encodedBufferLen = 0; // Unfortunately CryptBinaryToString() won't truncate for us
	CryptBinaryToStringA(reinterpret_cast<BYTE *>(valueBuffer.get()), characters, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &encodedBufferLen);
	result.resize(encodedBufferLen);
	CryptBinaryToStringA(reinterpret_cast<BYTE *>(valueBuffer.get()), characters * 3 / 4, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &result[0], &encodedBufferLen);
	result.resize(characters); // Chop to only as many as requested, more efficient than substr()
	return result;
}

// Quickie util to encapsulate crash-safe path building, since std::wstring et al can allocate memory
template <size_t N>
static void crash_mkpath(wchar_t(&dest)[N], const wchar_t *base, ...) // __attribute__((sentinel))
{
	const wchar_t *piece = nullptr;
	va_list args;
	va_start(args, base);
	wcsncpy_s(dest, base, _TRUNCATE);
	wcsncat_s(dest, L"\\", _TRUNCATE);
	while ((piece = va_arg(args, const wchar_t *)) != nullptr) {
		wcsncat_s(dest, piece, _TRUNCATE);
	}
	va_end(args);
}

#pragma endregion
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Formatting Utilities
//
// At crash time it is not safe to use any of the usual standard library routines for dealing
// with strings easily, such as swprintf_s(). But using them is extremely convenient and makes
// the code much more readable. So these are simple, minimalistic implementations of
// printf()-like routines.
#pragma region Formatting Utilities

static void crash_write_raw(int fd, bool wide, const wchar_t *s, size_t n);
static void crash_write_str(int fd, bool wide, const wchar_t *s);
template <typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
static void crash_write_int(int fd, bool wide, T l, unsigned int base);
static void crash_wdprintfv(int fd, bool wide, const wchar_t * const format, va_list args);
static void crash_wdprintf(int fd, const wchar_t * const format, ...);
static void crash_dprintf(int fd, const wchar_t * const format, ...);

static void crash_write_raw(int fd, bool wide, const wchar_t *s, size_t n)
{
	if (wide) {
		// Wide output requested. Write the bytes out directly without interpretation.
		_write(fd, s, n * sizeof(wchar_t));
	} else {
		// Output to an ASCII descriptor requested.
		//
		// Since it's unsafe to rely on std::codecvt<> or wcstomb() at crash-time due to use of global
		// state and possible allocation), we do a "poor man's" version here, displaying ASCII characters
		// as-is and fudging anything else as raw bytes. (Anything else would be major overkill.)
		for (size_t i = 0; i < n; ++i) {
			if (s[i] < 0x007f) {
				_write(fd, &s[i], 1); // EXTREMELY INEFFICIENT, but simple and works.
			} else {
				// Actually, this is even less efficient than the other, it'll call right back into here.
				crash_wdprintf(fd, L"\\x{%x}", static_cast<unsigned int>(s[i]));
			}
		}
	}
}

static void crash_write_str(int fd, bool wide, const wchar_t *s)
{
	crash_write_raw(fd, wide, s, wcslen(s));
}

template <typename T, typename>
static void crash_write_int(int fd, bool wide, T l, unsigned int base)
{
	wchar_t buf[64] = { 0 };
	wchar_t *bptr = &buf[62];

	if (l == 0) {
		crash_write_raw(fd, wide, L"0", 1);
	} else if (std::is_signed<T>::value && l < 0) {
		crash_write_raw(fd, wide, L"-", 1);
		l = static_cast<T>(-static_cast<std::make_signed<T>::type>(l));
	}
	for (; l && bptr >= &buf[0]; l /= base, --bptr) {
		*bptr = L"0123456789abcdef"[l % base];
	}
	crash_write_str(fd, wide, bptr + 1);
}

// A quickie version of printf that can only do very basic formats.
static void crash_wdprintfv(int fd, bool wide, const wchar_t * const format, va_list args)
{
	const wchar_t *ptr = format;
	while (ptr && *ptr) {
		if (*ptr++ == L'%') {
			if (!*ptr) { crash_write_raw(fd, wide, L"%", 1);  break; }
			switch (*ptr++) {
				case L's': crash_write_str(fd, wide, va_arg(args, const wchar_t *)); break;
				case L'x': crash_write_int(fd, wide, va_arg(args, uint32_t), 16); break;
				case L'u': crash_write_int(fd, wide, va_arg(args, uint32_t), 10); break;
				case L'd': crash_write_int(fd, wide, va_arg(args, int32_t), 10); break;
				case L'X': crash_write_int(fd, wide, va_arg(args, uint64_t), 16); break; // Not a standard printf() format
				case L'U': crash_write_int(fd, wide, va_arg(args, uint64_t), 10); break; // Not a standard printf() format
				case L'D': crash_write_int(fd, wide, va_arg(args, int64_t), 10); break; // Not a standard printf() format
				case L'%': crash_write_raw(fd, wide, L"%", 1); break;
				default: crash_write_raw(fd, wide, ptr - 2, 2); break;
			}
		} else {
			crash_write_raw(fd, wide, ptr - 1, 1);
		}
	}
}

static void crash_wdprintf(int fd, const wchar_t * const format, ...)
{
	va_list args;
	va_start(args, format);
	crash_wdprintfv(fd, true, format, args);
	va_end(args);
}

static void crash_dprintf(int fd, const wchar_t * const format, ...)
{
	va_list args;
	va_start(args, format);
	crash_wdprintfv(fd, false, format, args);
	va_end(args);
}

#pragma endregion

//////////////////////////////////////////////////////////////////////////////
// Logging utilities
//
// These logging utilities really should be crash-time only, but unfortunately we can't rely on
// the dbug system etc. to be set up properly yet so instead these very limited utilities are used
// during crash processing as well.
#pragma region Logging Utilities

#if defined(DEBUG_WW2) || defined(GM_RELEASE)

// Can't rely on the normal logging subsystem at crash time. This minimal logger
// is guaranteed to work.
static void crash_log(const wchar_t * const format, ...)
{
	va_list args;
	va_start(args, format);
	crash_dprintf(_fileno(stderr), L"Crash handler: ");
	crash_wdprintfv(_fileno(stderr), false, format, args);
	va_end(args);
}

#else

// In pure-release mode we don't log.
#define crash_log(f, ...)

#endif

#pragma endregion
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// HTTP request utilities
//
// Utilities for adding information to the HTTP request used by the crash sender
#pragma region HTTP Request Utilities

static void AppendRequestParam(std::string &body, const std::string &boundary, const std::string &param, const std::string &value)
{
	body += "--" + boundary + "\r\n";
	body += "Content-Disposition: form-data; name=\"" + param + "\"\r\n";
	body += "\r\n";
	body += value + "\r\n";
}

enum class FileKind { ASCII, Wide, Binary };

static bool AppendRequestFile(std::string &body, const std::string &boundary, const std::string &param, const std::wstring &path, FileKind kind)
{
	std::ifstream f(path, std::ios::binary);

	if (f.fail()) {
		dbugLog(_FLW, "Failed to open path %S, can't send this crash log", path.c_str());
		return false;
	}

	f.seekg(0, std::ios::end);
	size_t len = static_cast<size_t>(f.tellg());
	f.seekg(0, std::ios::beg);
	std::unique_ptr<char[]> data = std::make_unique<char[]>(len);
	f.read(data.get(), len);

	if (f.fail()) {
		dbugLog(_FLW, "Failed to read from %S, can't send this crash log", path.c_str());
		return false;
	}

	std::wstring::size_type pos = path.find_last_of(L"\\");
	std::wstring wfilename;
	if (pos == std::wstring::npos) { wfilename = path; }
	else { wfilename = path.substr(pos + 1); }
	std::string filename = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(wfilename);

	body += "--" + boundary + "\r\n";
	body += "Content-Disposition: form-data; name=\"" + param + "\"; filename=\"" + filename + "\"\r\n";

	switch (kind) {
		case FileKind::Wide:
			// We could use ";charset=UTF-16LE" here and avoid the conversion, but it's not clear from
			// RFC7578 that that'd actually work (it states that many implementations assume a default encoding).
			body += "Content-Type: text/plain\r\n";
			body += "\r\n";
			body += std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(std::wstring(reinterpret_cast<const wchar_t *>(data.get()), len >> 1));
			break;
		case FileKind::ASCII:
			body += "Content-Type: text/plain\r\n";
			body += "\r\n";
			body += std::string(data.get(), len);
			break;
		case FileKind::Binary:
			body += "Content-Type: application/octet-stream\r\n";
			body += "\r\n";
			body.append(data.get(), len);
			break;
		default:
			assert(false);
			abort();
	}
	body += "\r\n";

	return true;
}

#pragma endregion
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Information Gathering Utilities
//
// Utility for grabbing system info before crash time, when we can still do weird
// things like mess with COM objects since that's the only way to get the data.
#pragma region Information Gathering Utilities

#define PullFailure(fmt, ...) do { dbugLog(_FLA, fmt, __VA_ARGS__); return; } while (0)
#define PullFailureIf(cond, fmt, ...) do { if ((cond)) { PullFailure(fmt, __VA_ARGS__); } } while (0)
#define PullFailureIfNotSucceed(fmt, ...) PullFailureIf(!SUCCEEDED(result.Error()), "Failed with %S in crash handler: " fmt, result.ErrorMessage(), __VA_ARGS__)

#pragma comment(lib, "wbemuuid.lib")
static void PullMotherboardData(CrashHandlerContext &context)
{
	_COM_SMARTPTR_TYPEDEF(IWbemLocator, __uuidof(IWbemLocator));
	_COM_SMARTPTR_TYPEDEF(IWbemServices, __uuidof(IWbemServices));
	_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject, __uuidof(IEnumWbemClassObject));
	_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));

	CCom com;
	IWbemLocatorPtr pLoc;
	IWbemServicesPtr pSvc;
	IEnumWbemClassObjectPtr pEnumerator1, pEnumerator2;
	IWbemClassObjectPtr pclsObj1, pclsObj2;
	ULONG uReturn1 = WBEM_S_FALSE, uReturn2 = WBEM_S_FALSE;
	_variant_t vtManuProp, vtModelProp, vtVersProp, vtBuildProp;
	_com_error result(ERROR_SUCCESS);

	PullFailureIf(!com, "Failed to initialize COM in crash handler");

	result = _com_error(CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr));
	PullFailureIfNotSucceed("Initializing COM security");

	result = _com_error(pLoc.CreateInstance(CLSID_WbemLocator));
	PullFailureIfNotSucceed("Creating CLSID_WbemLocator instance");

	result = _com_error(pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, 0, 0, 0, 0, &pSvc));
	PullFailureIfNotSucceed("Connecting Locator to ROOT\\CIMV2");

	result = _com_error(CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE));
	PullFailureIfNotSucceed("Setting COM proxy blanket");

	result = _com_error(pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_ComputerSystem"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator1));
	PullFailureIfNotSucceed("Executing SELECT * FROM Win32_ComputerSystem");

	result = _com_error(pEnumerator1->Next(WBEM_INFINITE, 1, &pclsObj1, &uReturn1));
	PullFailureIfNotSucceed("Stepping first enumerator");
	PullFailureIf(uReturn1 <= 0, "No results in first enumerator for crash handler");

	result = _com_error(pclsObj1->Get(L"Manufacturer", 0, &vtManuProp, nullptr, nullptr));
	PullFailureIfNotSucceed("Get Manufacturer variant value from first enumerator");

	result = _com_error(pclsObj1->Get(L"Model", 0, &vtModelProp, nullptr, nullptr));
	PullFailureIfNotSucceed("Get Model variant value from first enumerator");

	result = _com_error(pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_OperatingSystem"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator2));
	PullFailureIfNotSucceed("Executing SELECT * FROM Win32_OperatingSystem");

	result = _com_error(pEnumerator2->Next(WBEM_INFINITE, 1, &pclsObj2, &uReturn2));
	PullFailureIfNotSucceed("Stepping second enumerator");
	PullFailureIf(uReturn2 <= 0, "No results in second enumerator for crash handler");

	result = _com_error(pclsObj2->Get(L"Version", 0, &vtVersProp, nullptr, nullptr));
	PullFailureIfNotSucceed("Get Version variant value from second enumerator");

	result = _com_error(pclsObj2->Get(L"BuildNumber", 0, &vtBuildProp, nullptr, nullptr));
	PullFailureIfNotSucceed("Get BuildNumber variant value from second enumerator");

	context.machineManufacturer = vtManuProp.bstrVal;
	context.machineModel = vtModelProp.bstrVal;
	context.windowsVersion = vtVersProp.bstrVal;
	context.windowsBuild = vtBuildProp.bstrVal;
}

static void GenerateOrLoadInstallationId(CrashHandlerContext &context)
{
	context.installationId = L"00000000-0000-0000-0000-000000000000";
	// TODO: Improve sysregGetHKeyText() to restore distinguishing between "no such key" and other errors
	if (!sysregGetHKeyText(HKEY_CURRENT_USER, REG_KEY_MAIN, L"InstallationGuid", context.installationId)) {
		GUID newId;
		wchar_t buffer[48] = {0};

		dbugLog(_FLN, "No registry value for this installation. Creating new GUID.");
		CoCreateGuid(&newId);
		_snwprintf_s(buffer, _TRUNCATE, L"%08X-%04hX-%04hX-%04hX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
			newId.Data1, newId.Data2, newId.Data3, (newId.Data4[0] << 8) | newId.Data4[1],
			newId.Data4[2], newId.Data4[3], newId.Data4[4], newId.Data4[5], newId.Data4[6], newId.Data4[7]
		);
		if (!sysregSetHKeyText(HKEY_CURRENT_USER, REG_KEY_MAIN, L"InstallationGuid", buffer)) {
			dbugLog(_FLM, "Failed to save new installation GUID to registry, not using");
		}
		context.installationId = buffer;
		return;
	}
}

#pragma endregion
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Crash-time execution
//
// Contains the actual crash-time callback Breakpad will execute after a crash is saved.
#pragma region Crash-time execution

static bool CrashReportingCallback(
	const wchar_t *dumpPath, const wchar_t *minidumpID, void *context_, EXCEPTION_POINTERS *exinfo,
	MDRawAssertionInfo *assertion, bool succeeded
)
{
	// Really we could just use the global, but this is more elegant. The global's only there for if we want
	// to remove the handler at some later time.
	CrashHandlerContext *context = reinterpret_cast<CrashHandlerContext *>(context_);
	wchar_t pathBuffer[MAX_PATH] = { 0 };
	FD fd;

	crash_mkpath(pathBuffer, dumpPath, minidumpID, L".dmp.container", nullptr);
	if (fd._wsopen_s(pathBuffer, _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY, _SH_DENYRW, _S_IREAD | _S_IWRITE)) {
		// Write out the container format HockeyApp's custom upload API expects
		crash_wdprintf(fd, L"Package: com.playnet.wwiiol\n"); // TODO: Hardcoding the macOS client's bundle ID is probably not quite right
		crash_wdprintf(fd, L"Version Code: " GIT_COMMIT_STR "\n");
		crash_wdprintf(fd, L"Version Name: " STRPRODUCTVER "\n");
		crash_wdprintf(fd, L"Windows: %s (%s)\n", context->windowsVersion.c_str(), context->windowsBuild.c_str());
		crash_wdprintf(fd, L"Manufacturer: %s\n", context->machineManufacturer);
		crash_wdprintf(fd, L"Model: %s\n", context->machineModel);
		crash_wdprintf(fd, L"Date: %U\n", static_cast<uint64_t>(_time64(nullptr)));
		crash_wdprintf(fd, L"CrashReporter Key: %s\n", context->installationId.c_str());
		crash_wdprintf(fd, L"\n");
		crash_wdprintf(fd, L"MinidumpContainer");
	} else {
		crash_log(L"Warning: Failed to create container at %s\n", pathBuffer);
	}

	crash_mkpath(pathBuffer, dumpPath, minidumpID, L".dmp.metadata", nullptr);
	if (fd._wsopen_s(pathBuffer, _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY, _SH_DENYRW, _S_IREAD | _S_IWRITE)) {
		// TODO: Write out stuff like player ID, callsign, etc.
		crash_wdprintf(fd, L"userId=%s\n", L"");
		crash_wdprintf(fd, L"userName=%s\n", L"");
		crash_wdprintf(fd, L"contact=%s\n", L"");
		crash_wdprintf(fd, L"description=%s\n", L"");
	} else {
		crash_log(L"Warning: Failed to create metadata file at %s\n", pathBuffer);
	}

	return false; // Pass it on to the OS (otherwise user won't see anything at all)
}

#pragma endregion
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Report Sender
//
// The routine does all the hard work of actually sending the HTTP request up to the
// HockeyApp server to present a crash to the dashboard.
#pragma region Report Sender

static bool SendExistingReport(const unique_HINTERNET &base, const std::wstring &report)
{
	unique_HINTERNET conn = InternetConnect(base, HockeyApp_serverName.c_str(), HockeyApp_serverPort, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);

	if (!conn) {
		dbugLog(_FLW, "Failed in InternetConnect(), can't send the crash log %S at this time.", report.c_str());
		return false;
	}

	DWORD flags =
		INTERNET_FLAG_KEEP_CONNECTION |
		INTERNET_FLAG_NO_CACHE_WRITE |
		INTERNET_FLAG_NO_COOKIES |
		INTERNET_FLAG_NO_UI |
		INTERNET_FLAG_PRAGMA_NOCACHE |
		INTERNET_FLAG_RELOAD |
		(HockeyApp_SSL ? INTERNET_FLAG_SECURE : 0);
	const wchar_t *acceptTypes[] = { L"*/*", nullptr };
	unique_HINTERNET req = HttpOpenRequestW(conn, L"POST", HockeyApp_apiPath.c_str(), nullptr, nullptr, acceptTypes, flags, 0);

	if (!req) {
		dbugLog(_FLW, "Failed in HttpOpenRequest(), can't send the crash log %S at this time.", report.c_str());
		return false;
	}

	// Because we need to send binary data and Base64 is not possible in multipart/form-data
	// parts, we must encode the actual request body as a direct UTF-8 byte stream. We take
	// shameless advantage of std::string's ability to handle arbitrary bytes (including NUL).
	std::string multipartBoundary = "------------------" + GenerateRandomString<24>();
	std::string contentTypeHeader = "Content-Type: multipart/form-data; boundary=" + multipartBoundary + "\r\n";
	std::string requestBody;
	if (!AppendRequestFile(requestBody, multipartBoundary, "log", report + L".container", FileKind::Wide)) {
		return false;
	}
	if (!AppendRequestFile(requestBody, multipartBoundary, "attachment0", report, FileKind::Binary)) {
		return false;
	}
	AppendRequestParam(requestBody, multipartBoundary, "description", ""); // TODO fill these in from .metadata
	AppendRequestParam(requestBody, multipartBoundary, "userId", ""); // TODO fill these in from .metadata
	AppendRequestParam(requestBody, multipartBoundary, "contact", ""); // TODO fill these in from .metadata
	requestBody += "--" + multipartBoundary + "--\r\n";

	// Use ASCII variant so we don't have to do an extra conversion on the multipart boundary just for the header here.
	if (!HttpSendRequestA(req, contentTypeHeader.c_str(), -1, const_cast<char *>(requestBody.c_str()), requestBody.length())) {
		dbugLog(_FLW, "Failed in HttpSendRequest(), can't send the crash log %S at this time.", report.c_str());
		return false;
	}

	DWORD status_code = 0, status_code_len = sizeof(status_code);
	if (!HttpQueryInfo(req, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status_code, &status_code_len, 0)) {
		dbugLog(_FLW, "Failed in HttpQueryInfo(), can't send the crash log %S at this time.", report.c_str());
		return false;
	}

	if (status_code != 201) { // Server responds with 201 Created on success
		dbugLog(_FLW, "Failed because status code %u is not 201, can't send the crash log %S at this time.", status_code, report.c_str());
		return false;
	}

	dbugLog(_FLN, "Successfully sent crash log %S", report.c_str());
	return true;
}
#pragma endregion
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Report Sender Thread
//
// The callthroughs which look for pending crash logs and, if found, spawn a thread to
// call the report sender for each one.
#pragma region Report Sender Thread

static void SendExistingReports(const std::vector<std::pair<std::wstring, FILETIME>> reports)
{
	unique_HINTERNET base = InternetOpen(HockeyApp_userAgent.c_str(), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

	if (!base) {
		dbugLog(_FLW, "Failed in InternetOpen(), can't send crash logs at this time.");
		return;
	}

	// Note: This calculation is EXTREMELY EXTREMELY WRONG. THIS IS NOT SEVEN DAYS BY ANY
	// STRETCH OF ANYONE'S IMAGINATION, nor is it intended to be representative of anything
	// other than a quick back-of-the-envelope calculation for the convenience of this very
	// immature implementation. Anyone who uses this as an example of how to do anything is
	// extremely wrong and should be chastised appropriately.
	__time64_t oneWeekAgo = _time64(nullptr) - (86400 * 7); // Oh and did I mention, THIS IS EXTREMELY WRONG!!!!
	LONGLONG ll = (oneWeekAgo * 10000000ULL) + 116444736000000000ULL;
	FILETIME ftOneWeekAgo = { static_cast<DWORD>(ll), static_cast<DWORD>(ll >> 32) };

	for (const std::pair<std::wstring, FILETIME> &report : reports) {
		bool succeeded = SendExistingReport(base, report.first);

		// In a more mature implementation, one would store a retry count per-log and respond
		// to different errors (such as "no network available" etc.) intelligently. However,
		// this is not a mature implementation and we want to avoid filling the user's hard
		// drive, so we use the much simpler algorithm of "if it was sent, delete it instantly,
		// and if it wasn't, delete it if it's more than 7 days old".
		if (succeeded || CompareFileTime(&ftOneWeekAgo, &report.second) > 0) {
			_wunlink(report.first.c_str());
			_wunlink((report.first + L".container").c_str());
			_wunlink((report.first + L".metadata").c_str());
		}
	}
}

static void ProcessExistingReports(const std::wstring &dataPath)
{
	std::unordered_map<std::wstring, std::pair<std::wstring, FILETIME>> rawDumpPaths;
	std::unordered_map<std::wstring, std::wstring> containerPaths;
	std::unordered_map<std::wstring, std::wstring> metadataPaths;

	std::vector<std::pair<std::wstring, FILETIME>> possiblyValidReports;

	HANDLE hFile = nullptr;
	WIN32_FIND_DATA fileInfo;

	hFile = FindFirstFile((dataPath + L"\\*.*").c_str(), &fileInfo);
	if (hFile == INVALID_HANDLE_VALUE) {
		return;
	}
	do {
		if (fileInfo.cFileName[0] == L'.') {
			continue;
		}
		std::wstring filename = fileInfo.cFileName;

		if (ends_with(filename, L".dmp.container")) {
			containerPaths[filename.substr(0, filename.length() - wcslen(L".dmp.container"))] = filename;
		} else if (ends_with(filename, L".dmp.metadata")) {
			metadataPaths[filename.substr(0, filename.length() - wcslen(L".dmp.metadata"))] = filename;
		} else if (ends_with(filename, L".dmp")) {
			rawDumpPaths[filename.substr(0, filename.length() - wcslen(L".dmp"))] = { filename, fileInfo.ftCreationTime };
		}
	} while (FindNextFile(hFile, &fileInfo));

	FindClose(hFile);
	if (GetLastError() != ERROR_NO_MORE_FILES) {
		dbugLog(_FLW, "Failed enumerating crash files");
		return;
	}

	for (const std::pair<std::wstring, std::pair<std::wstring, FILETIME>> &kv : rawDumpPaths) {
		if (containerPaths.find(kv.first) == containerPaths.end() || metadataPaths.find(kv.first) == metadataPaths.end()) {
			// Invalid log (missing auxiliary data). Remove it, ignoring errors.
			dbugLog(_FLW, "Log missing its container or metadata: %S (%S)", kv.second.first.c_str(), kv.first.c_str());
			_wunlink((dataPath + L"\\" + kv.second.first).c_str());
			_wunlink((dataPath + L"\\" + kv.second.first + L".container").c_str());
			_wunlink((dataPath + L"\\" + kv.second.first + L".metadata").c_str());
		} else {
			dbugLog(_FLI, "Valid log (has its auxiliaries): %S (%S)", kv.second.first.c_str(), kv.first.c_str());
			possiblyValidReports.push_back({ dataPath + L"\\" + kv.second.first, kv.second.second });
			containerPaths.erase(kv.first);
			metadataPaths.erase(kv.first);
		}
	}

	// Any items left in rawDumpPaths or metadataPaths are orphans (not matching any container)
	dbugLog(_FLI, "Removing %u leftover metadata files", metadataPaths.size());
	for (const std::pair<std::wstring, std::wstring> &kv : metadataPaths) { _wunlink((dataPath + L"\\" + kv.second).c_str()); }
	dbugLog(_FLI, "Removing %u leftover container files", containerPaths.size());
	for (const std::pair<std::wstring, std::wstring> &kv : containerPaths) { _wunlink((dataPath + L"\\" + kv.second).c_str()); }

#if defined(BETA_RELEASE) || defined(ALPHA_RELEASE) || defined(DEBUG_WW2)
	std::thread(SendExistingReports, std::move(possiblyValidReports)).join();
#else
	std::thread(SendExistingReports, std::move(possiblyValidReports)).detach();
#endif
}

#pragma endregion
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// API
//
// The calls made public in the header for installing and removing the crash handler.
#pragma region API

void InstallCrashHandler()
{
	wchar_t *basePath = nullptr;
	HRESULT result = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_SIMPLE_IDLIST | KF_FLAG_CREATE, nullptr, &basePath);

	if (SUCCEEDED(result)) {
		std::wstring dataPath(basePath);
		CoTaskMemFree(basePath);

		// TODO: This path should really not be hardcoded, but at current there is no correct
		// support for a safe UserDataPath in fio. Replace with appropriate fioCfg*() call to
		// find path the same way the rest of the app does, once the rest of the app doesn't
		// do it completely wrong. Don't forget that mngopt and fio must be initialized before
		// using fioCfg*() calls, so this code may have to be moved to WinMainX().
		// This loop recursively ensures the existence of the directories along the chosen path, such as it is.
		for (const std::wstring &component : std::vector<std::wstring>{ L"Playnet", L"World War II Online", L"Crash Logs" }) {
			dataPath += L"\\" + component;
			CreateDirectory(dataPath.c_str(), nullptr); // we don't care if this fails
		}

		// Allocate the context for the crash handler.
		gCrashContext = std::make_unique<CrashHandlerContext>();

		// Grab the Windows version and motherboard data and installation ID now, since we won't be able to later.
		PullMotherboardData(*gCrashContext);
		GenerateOrLoadInstallationId(*gCrashContext);

		// Breakpad is installed here. The check for "did crashes happen in the last run?" is elsewhere.
		gCrashContext->crashHandler.reset(new google_breakpad::ExceptionHandler(
			dataPath.c_str(), // Crash log storage path
			nullptr, // Filter callback (none, we want all exception logs)
			CrashReportingCallback, // Crash callback
			gCrashContext.get(), // Callback context (pull the raw pointer)
			google_breakpad::ExceptionHandler::HANDLER_ALL // Handler types (we want all types)
		));
		dbugLog(_FLN, "Installed Breakpad exception handler, now trapping crashes. Log path is %S", dataPath.c_str());

		ProcessExistingReports(dataPath);
	} else {
		dbugLog(_FLM, "Failed to find %APPDATA%\\Roaming, crash handler not installed.");
	}
}

void RemoveCrashHandler()
{
	gCrashContext.reset();
}

#pragma endregion
//////////////////////////////////////////////////////////////////////////////

#else // No crash handler

void InstallCrashHandler()
{
}

void RemoveCrashHandler()
{
}

#endif

// $Id: LoginData.cpp 247 2009-05-11 16:11:42Z rampster@CRS $
// LoginData tracks the user's login preferences
// such as username and password and whether or not
// they wish to retain their password between sessions.

#include "LoginData.h"
#include "wwii/wwiiUTF.h"
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <assert.h>

// ------------------------------ Public Variables

LoginData loginData;

// ------------------------------ Local Variables

static const wchar_t*	HistoryFilename = L"lg.dat";
static		 wchar_t	HistoryFilePath[512];
static const UINT32		Guard1Value = 0xf1aca8c3;
static const UINT32		Guard2Value = 0xa1c349ab;

// ------------------------------ Member Functions

//////////////////////////////////////////////////////////////////////
// The default constructor attempts to load in data
// from the login history file.

LoginData::LoginData()
	: m_nullByte(0), m_version(255), m_revision(1)
	, m_guard1(Guard1Value)
	, m_passlen(-1), m_arenaID(1)
	, m_guard2(Guard2Value)
	, m_needsWrite(true)
{
	memset(m_username, 0, sizeof(m_username));
	memset(m_passMD5, 0, sizeof(m_passMD5));
	memset(m_spare, 0, sizeof(m_spare));
	memset(m_region, 0, sizeof(m_region));
}

//////////////////////////////////////////////////////////////////////
// Destructor -- if Mark For Write is set, commit.

LoginData::~LoginData()
{
	if ( m_needsWrite == true && HistoryFilePath[0] != 0 )
		Write();
}

//////////////////////////////////////////////////////////////////////
// Non-validating read of the history file.

bool
LoginData::Read()
{
	//  _stprintf_s(HistoryFilePath, _T("%s\\%s"), UserDataPath.c_str(), HistoryFilename);  //  MJH - Feburary 27, 2018.
	_stprintf_s(HistoryFilePath, _T("%s\\%s"), PG_userWorkingPath.c_str(), HistoryFilename);  //  \user -> My Documents -> bin.

	int file = -1;
	errno_t err = _tsopen_s(&file, HistoryFilePath, _O_RDONLY | _O_BINARY, _SH_DENYRW, 0);
	if ( err != 0 )
	{
		dbugLog(_FLI, "Unable to read history file: %d [%s]", err, _sys_errlist[err]);
		return false;
	}
	if ( file < 0 )
	{
		dbugLog(_FLI, "Failed to open history file");
		return false;
	}

	int bytesRead = _read(file, this, sizeof(*this));
	_close(file);

	m_needsWrite = false;

	return bytesRead >= 8;
}

//////////////////////////////////////////////////////////////////////
// Validate() checks that the data in the structure looks good and
// performs any revision modifications required.

bool
LoginData::Validate()
{
	if ( m_nullByte != 0 || m_version != 255 || m_revision == 0 )
		return false;

	////
	// Data Validation.
	switch ( m_revision )
	{
	case 1:
		if ( m_arenaID <= 0 || m_arenaID > 255 )
			return false;
		if ( m_guard1 != Guard1Value )
			return false;
		if ( m_guard2 != Guard2Value )
			return false;
	break;
	default:
		return false;
	break;
	}

	////
	// Do data conversion here

	return true;
}

//////////////////////////////////////////////////////////////////////
// Unpack the MD5 from binary values to a string

void
LoginData::GetPasswordMask(wchar_t* mask) const
{
	if ( m_passlen <= 0 )
		mask[0] = 0;
	else
	{
		int i;
		for ( i = 0; i < m_passlen; ++i )
		{
			mask[i] = L'*';
		}
		mask[i] = 0;
	}
}

//////////////////////////////////////////////////////////////////////
// Convert the MD5 from two 64 bit integers to a hex character string.

void
LoginData::GetPasswordMD5(UCHAR* md5, const size_t md5Size) const
{
	// Password is not stored.
	if ( m_passlen <= 0 )
		md5[0] = 0;
	else
	{
		sprintf((char*)md5, "%016llx%016llx", m_passMD5[0], m_passMD5[1]);
	}
}

//////////////////////////////////////////////////////////////////////
// Extract the username as a UTF8 character stream

bool
LoginData::GetUsernameAsUTF8(char* usernameDest, const size_t destSize) const
{
	return WWII::ConvertChars(m_username, usernameDest, destSize);
}

//////////////////////////////////////////////////////////////////////
// Populate the password fields

void
LoginData::SetPassword(bool /*savePassword*/, UINT32 passlen, const UCHAR* passMD5)
{
	m_savePassword = false; //savePassword;
	m_passlen = passlen;
	sscanf_s((const char*)passMD5, "%016llx%016llx", &m_passMD5[0], &m_passMD5[1]);
}

//////////////////////////////////////////////////////////////////////
// Save the history record to disk

bool
LoginData::Write()
{
	assert( m_guard1 == Guard1Value );
	assert( m_guard2 == Guard2Value );

	m_needsWrite = false;

	if ( !m_savePassword )
	{
		m_passlen = -1;
		m_passMD5[0] = m_passMD5[1] = 0ULL;
	}

	std::wstring filename = PG_exeDataPath + L"\\";
	filename += HistoryFilename;

	int file = -1;
	_wunlink(filename.c_str());
	int err = _wsopen_s(&file, HistoryFilePath, _O_CREAT | _O_TRUNC | _O_BINARY | _O_WRONLY, _SH_SECURE, _S_IREAD | _S_IWRITE);
	if ( err != 0 )
		return false;
	if ( file < 0 )
		return false;
	int bytesWritten = _write(file, this, sizeof(*this));
	_close(file);

	return (bytesWritten == sizeof(*this));
}

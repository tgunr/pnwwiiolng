#pragma once
// $Id: LoginData.h 105 2009-01-12 20:23:08Z osmith@CRS $
// Describes the data structure used to store information
// between Playgate sessions such as username, password,
// arena and region.

// How long can the password a user types actually be
static const size_t MAX_PASSWORD_LENGTH = 32;

class LoginData
{
public:
	LoginData();
	~LoginData();

	char	m_nullByte;			// Always zero
	UCHAR	m_version;			// Version ID byte
	UINT16	m_revision;			// Revision
	wchar_t	m_username[20];		// Was username
	UINT32	m_guard1;			// Guard value
	int		m_passlen;			// length of password
	UINT64	m_passMD5[2];		// Password MD5
	UINT32	m_arenaID;			// Arena ID they selected
	UINT32	m_guard2;			// Guard value
	wchar_t	m_region[128];		// Name of region
	bool	m_savePassword : 1;	// Whether or not to save passwords
	bool	m_needsWrite : 1;
	UCHAR	m_spare[256];
	UINT32  m_steamAccountId;	// steam account ID

	// Extract the password mask/MD5/username
	void GetPasswordMask(wchar_t* maskDest) const;
	void GetPasswordMD5(UCHAR* md5Dest, const size_t md5Size) const;
	bool GetUsernameAsUTF8(char* usernameDest, const size_t destSize) const;

	void SetPassword(bool savePassword, UINT32 passlen, const UCHAR* passMD5);

	// Load data from disk - does not perform any validation or conversion,
	// resulting data may not be in healthy state yet - see Validate()
	bool Read();

	// Check that what we read is valid and perform any revision conversions.
	bool Validate();

	void MarkForWrite() { m_needsWrite = true; }

private:
	// Save what we have - returns false if it is unable to save.
	bool Write();
};

extern LoginData loginData;
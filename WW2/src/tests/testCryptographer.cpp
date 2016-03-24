#include <syscfg.h> // Path is project specific.
#include "tests/testUnits.h"
// Test Module for chat's CommandWithArgs class.

#include "teul/teulEndpoint.h"
#include "teul/teulVersion.h"
#include "teul/teulEndpointImpl.h"

class CryptographerTest : public TestUnit
{
public: CryptographerTest() : TestUnit("Cryptographer") {}

public: virtual bool TestVersion(const UINT32 version, const UINT16 bufSize, const UINT64 startSeed)
		{
			UCHAR* buf1 = static_cast<UCHAR*>(OS_ALLOCA(bufSize + 4));
			UCHAR* buf2 = static_cast<UCHAR*>(OS_ALLOCA(bufSize + 4));
			UCHAR* buf3 = static_cast<UCHAR*>(OS_ALLOCA(bufSize + 4));

			ENDPOINT ep;
			EndpointImpl impl;
			ep.m_impl = &impl;
			ep.m_impl->m_version = ep.m_impl->m_cryptVersion = version;

			ep.m_impl->m_txCrypt.setSeed(startSeed);
			ep.m_impl->m_rxCrypt.setSeed(startSeed);

			CRITICAL( ExpectTrue( ep.m_impl->m_txCrypt.seed() == startSeed ) );
			CRITICAL( ExpectEqual( ep.m_impl->m_txCrypt.seed(), ep.m_impl->m_rxCrypt.seed() ) );

			for ( size_t i = 0; i <= bufSize; ++i )
			{
				buf1[i] = static_cast<UCHAR>((i + 7) & 0xff);
				buf2[i] = 0;
				buf3[i] = ~0;
			}

			UINT16 len1 = bufSize, len2 = 0;

			char versionStr[120];
			strprintf(versionStr, "Encrypt: %04x: %" PRSizeT " bytes w/%llx", version, bufSize, startSeed);

			BEGIN_SUB_STAGE( versionStr )

				memcpy(buf2, buf1, len1);
				ep.encrypt(buf2, len1);

				const bool buf1_matches_buf2 = (memcmp(buf1, buf2, len1) == 0);
				if ( version > Teul::BaseVersion )
				{
					// Encryption should have changed the contents.
					CRITICAL( ExpectFalse( buf1_matches_buf2 ) );
				}
				else
					CRITICAL( ExpectTrue( buf1_matches_buf2 ) );

				// Seed should have changed.
				if ( version >= Teul::TeulV2 )
				{
					CRITICAL( ExpectFalse( ep.m_impl->m_txCrypt.seed() == startSeed ) );
					// But the rx one shouldn't have.
					CRITICAL( ExpectEqual( ep.m_impl->m_rxCrypt.seed(), startSeed ) );
				}

			END_SUB_STAGE( versionStr )

			strprintf(versionStr, "Decrypt: %04x: %" PRSizeT " bytes w/%llu", version, bufSize, startSeed);

			BEGIN_SUB_STAGE( versionStr )

				// Now decrypt the encrypted data into a third buffer.
				len2 = len1;

				memcpy(buf3, buf2, len2);
				ep.decrypt(buf3, len2);

				// Old versions should have subtracted the size byte again,
				// new versions would never have changed it.
				CRITICAL( ExpectEqual( len2, bufSize ) );

				CRITICAL( ExpectEqual( len1, len2 ) );

				const bool buf1_matches_buf3 = (memcmp(buf1, buf3, len2) == 0);
				CRITICAL( ExpectTrue( buf1_matches_buf3 ) );

				if ( version >= Teul::TeulV2 )
				{
					CRITICAL( ExpectFalse( ep.m_impl->m_rxCrypt.seed() == startSeed ) );
					CRITICAL( ExpectEqual( ep.m_impl->m_rxCrypt.seed(), ep.m_impl->m_txCrypt.seed() ) );
				}

			END_SUB_STAGE( versionStr )

			return true;
		}

public: virtual bool Run() override
		{
			if ( !TestVersion(0, 1000, 0ULL) )
				return false;

			static const size_t numSeeds = 8;
			static const UINT64 seeds[numSeeds] = { 0x1ULL, ~0ULL, 0xdeadbeef, 0xbeefdead, 0x9e3779b9, 0xdeadbeefbeefdeadULL, 0xdeadbeeffeedbeefULL, 0xfeeddeadbeeffeedULL };
			for ( size_t i = 0; i < numSeeds; ++i )
			{
				for ( UINT32 version = Teul::BaseVersion; version <= (UINT32)Teul::VersionNumber; ++version )
				{
					for ( UINT16 sz = 4; sz < 64; ++sz )
					{
						if ( !TestVersion(version, sz, seeds[i]) )
							return false;
					}
					if ( !TestVersion(version, 1000, seeds[i]) )
						return false;
					if ( !TestVersion(version, 1207, seeds[i]) )
						return false;
				}
			}

			return true;
		}
};

static CryptographerTest test;


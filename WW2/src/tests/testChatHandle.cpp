#include <syscfg.h> // Path is project specific.
// Created: Tue Feb 16 23:02:55 PST 2010
//
// Tests for ChatHandle
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "tests/testUnits.h"
#include "wwii/wwiiPack.h"		// For wwiiPackHandle
#include "wwii/wwiiUTF.h"		// For UTF functions

// Test unit class.
class ChatHandleTestUnit : public TestUnit
{
public: ChatHandleTestUnit() : TestUnit("ChatHandle") {}
public: virtual ~ChatHandleTestUnit() {}

public: virtual bool Run() ;

	// Test chatIsValidCallsign and chatIsValidHandle
	// with a series of valid callsigns.
public:	// Templated to accept char* and wchar_t*
	template<typename _Type>
	bool
	TestGoodCallsigns(const _Type** goodSigns)
	{
		START_SUB_STAGE( "Good Callsigns" )
			for ( UINT32 i = 0 ; goodSigns[i] != NULL ; ++i )
			{
				TestFocus(goodSigns[i]) ;

				ExpectTrue( chatIsValidCallsign(goodSigns[i]) ) ;

				ExpectTrue( chatIsValidHandle(chatGetHandle(goodSigns[i])) ) ;
			}
		END_SUB_STAGE( "Good Callsigns" )
		return true ;
	}

	// Test chatIsValidCallsign with a series of invalid callsigns.
	// Note: We can't do the same tests on chatIsValidHandle, because
	// the handles may not pack in the first place.
public: // Templated to accept char* and wchar_t*
	template<typename _Type>
	bool
	TestBadCallsigns(const _Type** badSigns)
	{
		START_SUB_STAGE( "Bad Callsigns" )
			for ( UINT32 i = 0 ; badSigns[i] != NULL ; ++i )
			{
				TestFocus(badSigns[i]) ;
				// Test char* overload of chatIsValidCallsign
				ExpectFalse( chatIsValidCallsign(badSigns[i]) ) ;
			}
		END_SUB_STAGE( "Bad Callsigns" )
		return true ;
	}

	// Test that a known-good callsign packs the correct number
	// of bits and unpacks into the same valid callsign regardless
	// of offset position.
public:	// Templated so it can accept char* or wchar_t*
	template<typename _Type>
	bool
	TestPackHandle(const _Type* callsign)
	{
		char packBuffer[128] ;
		ChatHandle source = chatGetHandle(callsign) ;
		CRITICAL( ExpectFalse(source == 0ULL) ) ;

		bool result = true ;

		// Test wwiiPackHandle on a whole series of offsets.
		for ( UINT32 baseOffset = 0 ; baseOffset <= GAME_HANDLE_BITS * 2 ; ++baseOffset )
		{
			sprintf(packBuffer, "%u bits offset", baseOffset) ;
			TestFocus(packBuffer) ;

			ChatHandle dest = 0 ;

			SINT32 srcOffset = wwiiPackHandle(TEUL_PACK, (UCHAR*)packBuffer, baseOffset, &source, 0) ;
			SINT32 dstOffset = wwiiPackHandle(TEUL_UNPACK, (UCHAR*)packBuffer, baseOffset, &dest, 0) ;

			// We should have packed 'GAME_HANDLE_BITS' of data
			if (
				!ExpectEqual( srcOffset, GAME_HANDLE_BITS + baseOffset ) ||
				!ExpectEqual( dstOffset, GAME_HANDLE_BITS + baseOffset ) ||
				!ExpectEqual( source, dest ) )
			{
				result = false ;
			}
		}

		return result ;
	}
} ;

bool
ChatHandleTestUnit::Run()
{
	// My primary test character.
	#define Character "七"

	ChatHandle test ;
	char plainCallsign[MAX_HANDLE_SIZE] ;
	wchar_t wideCallsign[MAX_HANDLE_SIZE] ;
	const wchar_t* bogey = L"七" ;
	ChatHandle bogeyHandle = 9223372036854775874ULL ;

	//////////////////////////////////////////////////////////////////////
	// First of all, make sure that the UTF8 conversion functions work
	// since if they do not, some of the chatHandle functions will break.

	START_TEST_STAGE( "UTF8 Conversions" )

		#define FIRST_TEST "[" Character "]"

		START_SUB_STAGE( "char->wide->char" )

			strcpy(plainCallsign, FIRST_TEST) ;
			WWII::ConvertChars(plainCallsign, wideCallsign, sizeof(wideCallsign)) ;
			WWII::ConvertChars(wideCallsign, plainCallsign, sizeof(plainCallsign)) ;

			CRITICAL( ExpectEqual( strcmp(plainCallsign, FIRST_TEST), 0 ) ) ;

		END_SUB_STAGE( "char->wchar->char" )

		START_SUB_STAGE( "wchar->char" )

			WWII::ConvertChars(bogey, plainCallsign, sizeof(plainCallsign)) ;

			CRITICAL( ExpectEqual( strcmp(plainCallsign, Character), 0 ) ) ;

		END_SUB_STAGE( "wchar->char" )

	END_TEST_STAGE( "UTF8 Conversions" )

	//////////////////////////////////////////////////////////////////////
	// Now test that the fundamental "chatGetHandle" routine works by
	// testing some fixed callsigns against known values.
	// NOTE: Because we are using known values, there is no explicit
	// test for chatGetHandle("a") == chatGetHandle(L"A") etc.

	START_TEST_STAGE( "chatGetHandle" )

		START_SUB_STAGE( "chars" )

			CRITICAL(
				ExpectEqual( chatGetHandle("a"), 1ULL ) &&
				ExpectEqual( chatGetHandle("z"), 26ULL ) &&
				ExpectEqual( chatGetHandle("A"), 27ULL ) &&
				ExpectEqual( chatGetHandle("Z"), 52ULL ) &&
				ExpectEqual( chatGetHandle("-"), 63ULL ) &&
				ExpectEqual( chatGetHandle("_"), 63ULL ) &&
				ExpectEqual( chatGetHandle("aaaaaaaaaa"), 18300341342965825ULL ) &&
				ExpectEqual( chatGetHandle("zzzzzzzzzz"), 475808874917111450ULL ) &&
				ExpectEqual( chatGetHandle("AAAAAAAAAA"), 494109216260077275ULL ) &&
				ExpectEqual( chatGetHandle("ZZZZZZZZZZ"), 951617749834222900ULL ) &&
				ExpectEqual( chatGetHandle("AbCd123450"), 971343266556530843ULL )
			) ;

		END_SUB_STAGE( "chars" )

		START_SUB_STAGE( "wide chars" )

			CRITICAL(
				ExpectEqual( chatGetHandle(L"a"), 1ULL ) &&
				ExpectEqual( chatGetHandle(L"z"), 26ULL ) &&
				ExpectEqual( chatGetHandle(L"A"), 27ULL ) &&
				ExpectEqual( chatGetHandle(L"Z"), 52ULL ) &&
				ExpectEqual( chatGetHandle(L"-"), 63ULL ) &&
				ExpectEqual( chatGetHandle(L"_"), 63ULL ) &&
				ExpectEqual( chatGetHandle(L"aaaaaaaaaa"), 18300341342965825ULL ) &&
				ExpectEqual( chatGetHandle(L"zzzzzzzzzz"), 475808874917111450ULL ) &&
				ExpectEqual( chatGetHandle(L"AAAAAAAAAA"), 494109216260077275ULL ) &&
				ExpectEqual( chatGetHandle(L"ZZZZZZZZZZ"), 951617749834222900ULL ) &&
				ExpectEqual( chatGetHandle(L"AbCd123450"), 971343266556530843ULL )
			) ;

		END_SUB_STAGE( "wide chars" )

		START_SUB_STAGE( "Chinese wchars" )

			CRITICAL(
				ExpectEqual( chatGetHandle(L""), 0ULL ) &&
				ExpectEqual( chatGetHandle(L"七"), bogeyHandle ) &&
				ExpectEqual( chatGetHandle(L"七七七七"), 9223662325642330178ULL ) &&
				ExpectEqual( chatGetHandle(L"七abcd"), 9511615607682908226ULL )
			) ;

		END_SUB_STAGE( "Chinese wchars" )

	END_TEST_STAGE( "chatGetHandle" )

	//////////////////////////////////////////////////////////////////////
	// Now make sure chatGetCallsign works.

	START_TEST_STAGE( "chatGetCallsign" )

		test = chatGetHandle(Character) ;

		CRITICAL( ExpectEqual(test, bogeyHandle) ) ;

		CRITICAL(
			ExpectTrue(chatGetCallsign(plainCallsign, test)) &&
			ExpectEqual(strcmp(plainCallsign, Character), 0)
		) ;

		CRITICAL(
			ExpectTrue(chatGetCallsign(wideCallsign, test)) &&
			ExpectEqual(wcscmp(wideCallsign, bogey), 0)
		) ;

		CRITICAL( ExpectTrue( WWII::ConvertChars(wideCallsign, plainCallsign, sizeof(plainCallsign)) ) ) ;

	END_TEST_STAGE( "chatGetCallsign" )

	//////////////////////////////////////////////////////////////////////
	// Verify that the chatGetLowercaseHandle routine works.

	START_TEST_STAGE( "Case transformation" )

		// Operations with char* plain ascii callsigns
		START_SUB_STAGE( "ASCII char* callsigns" )

			ExpectFalse( chatGetHandle("kfs1") == chatGetHandle("KFS1") ) ;

			START_SUB_STAGE( "From char*" )
				ExpectEqual( chatGetHandle("kfs1"), chatGetLowercaseHandle("KFS1") ) ;
			END_SUB_STAGE( "From char*" )
			START_SUB_STAGE( "From ChatHandle" )
				ExpectEqual( chatGetHandle("kfs1"), chatGetLowercaseHandle(chatGetHandle("KFS1")) ) ;
			END_SUB_STAGE( "From ChatHandle" )

		END_SUB_STAGE( "ASCII char* callsigns" )

		// Operations with wchar_t* plain ascii callsigns
		START_SUB_STAGE( "ASCII wchar_t* callsigns" )

			// Verify case is being interpreted correctly.
			ExpectFalse( chatGetHandle(L"kfs1") == chatGetHandle(L"KFS1") ) ;

			START_SUB_STAGE( "Basic Equality With ASCII char* callsigns" )

				ExpectEqual( chatGetHandle("kfs1"), chatGetHandle(L"kfs1") ) ;
				ExpectEqual( chatGetLowercaseHandle("AbCdEfGhIj"), chatGetLowercaseHandle(L"AbCdEfGhIj") ) ;
				ExpectEqual( chatGetLowercaseHandle("aBcDeFgHiJ"), chatGetLowercaseHandle(L"aBcDeFgHiJ") ) ;

			END_SUB_STAGE( "Basic Equality With ASCII char* callsigns" )

			START_SUB_STAGE( "From wchar_t*" )
				ExpectEqual( chatGetHandle(L"kfs1"), chatGetLowercaseHandle(L"KFS1") ) ;
			END_SUB_STAGE( "From wchar_t*" )
			START_SUB_STAGE( "From ChatHandle" )
				ExpectEqual( chatGetHandle(L"kfs1"), chatGetLowercaseHandle(chatGetHandle(L"KFS1")) ) ;
			END_SUB_STAGE( "From ChatHandle" )

		END_SUB_STAGE( "ASCII wchar_t* callsigns" )

		// Operations with wchar_t* callsigns including chinese characters.
		START_SUB_STAGE( "Chinese wchar_t* callsigns" )

			ExpectEqual( chatGetHandle(L"七a丌c"), chatGetLowercaseHandle(L"七A丌C") ) ;
			ExpectEqual( chatGetHandle(L"七kfs1"), chatGetLowercaseHandle(L"七KFS1") ) ;

		END_SUB_STAGE( "Chinese wchar_t* callsigns" )

	END_TEST_STAGE( "Case transformation" )

	//////////////////////////////////////////////////////////////////////
	// Verify isValidCallsign and isValidHandle for positive and negative
	// functionality. GoodCallsigns tests that they return true for
	// known-valid callsigns and BadCallsigns tests they return false
	// for known bad callsigns.

	START_TEST_STAGE( "isValidCallsign/Handle" )

		// "前个例子1" contains an unusual (3-byte) character.
		const char* goodCharSigns[] =
		{
			"aa", "ab", "AA", "12", "1-", "12-", "ZZ", "A-A", "A123Z", "A-1-2-3-4", "A12345678b"
		,	"凸-", "凸凸凸凸-", "龠", "龠龠", "龠aa龠", "龠aa龠a", "龠---a", "gamemgr-07", "前个例子1"
		,	"aaa", "AAA", "aaaa", "AAAA", "aaaaa", "AAAAA", "aaaaaa", "AAAAAA", "aaaaaaa", "AAAAAAA", "aaaaaaaa", "AAAAAAAA", "aaaaaaaaaa", "AAAAAAAAAA"
		,	"AaAaAaAaAa", "aAaAaAaAaA"
		, NULL
		} ;
		const wchar_t* goodWcharSigns[] = { L"aa", L"ab", L"AA", L"12", L"1-", L"12-", L"ZZ", L"A-A", L"A123Z", L"A-1-2-3-4", L"A12345678b", L"凸-", L"凸凸凸凸-", L"龠", L"龠龠", L"龠aa龠", L"龠aa龠a", L"龠---a", L"gamemgr-07", L"前个例子1", NULL } ;
		const char* badCharSigns[] = { "-", "1", "a", "--", "abcdefghijklmnop", "凸凸凸凸凸", "冖冖冖冖冖冖冖冖", "a冖", NULL } ;
		const wchar_t* badWcharSigns[] = { L"-", L"1", L"a", L"--", L"abcdefghijklmnop", L"凸凸凸凸凸", L"冖冖冖冖冖冖冖冖", L"a冖", L"ысдфк", NULL } ;

		START_SUB_STAGE( "char* callsigns" )

			TestGoodCallsigns(goodCharSigns) ;
			TestBadCallsigns(badCharSigns) ;

		END_SUB_STAGE( "char* callsigns" )

		START_SUB_STAGE( "wchar_t* callsigns" )

			TestGoodCallsigns(goodWcharSigns) ;
			TestBadCallsigns(badWcharSigns) ;

		END_SUB_STAGE( "wchar_t* callsigns" )

		// Simple chatIsValidHandle tests.
		ExpectFalse( chatIsValidHandle(0ULL) ) ;
		ExpectFalse( chatIsValidHandle(1ULL) ) ;
		ExpectFalse( chatIsValidHandle(0ULL + WIDE_HANDLE_BIT) ) ;
		ExpectFalse( chatIsValidHandle(1ULL + WIDE_HANDLE_BIT) ) ;
		ExpectFalse( chatIsValidHandle(~0ULL) ) ;

	END_TEST_STAGE( "isValidCallsign" )

	//////////////////////////////////////////////////////////////////////
	//  Test wwiiPackHandle functionality

	START_TEST_STAGE( "wwiiPackHandle" )

		START_SUB_STAGE( "char* plain callsign" )

			// We validated this handle earlier.
			TestPackHandle("AbCd123450") ;

		END_SUB_STAGE( "char* plain callsign" )

		START_SUB_STAGE( "wchar_t* plain callsign" )

			// We validated this handle earlier.
			TestPackHandle(L"AbCd123450") ;

		END_SUB_STAGE( "char* plain callsign" )

		START_SUB_STAGE( "wchar_t* chinese callsign" )

			// We validated this handle earlier.
			TestPackHandle(L"七abcd") ;

		END_SUB_STAGE( "wchar_t* chinese callsign" )

	END_TEST_STAGE( "wwiiPackHandle" )

	return true ;
}

static ChatHandleTestUnit test ;

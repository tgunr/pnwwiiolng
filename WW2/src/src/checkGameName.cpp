#include <syscfg.h> // Path is project specific.
// Tool to check a game name for validity.

#include "chat/chatHandle.h"
#include "wwii/wwiiUTF.h"

//////////////////////////////////////////////////////////////////////
// Test if a callsign has a legal mix of utf/roman characters.

static void
_testCombination(const wchar_t* callsign)
{
	for ( int i = 1; callsign[i] > 0; ++i )
	{
		if ( (UINT32)callsign[i] > 120 )
			throw "Roman + UTF combination not allowed";
	}
}

//////////////////////////////////////////////////////////////////////
// Checks individual characters of the callsign

static void
_checkCharacters(const wchar_t* callsign)
{
	std::wstring badCharacters;
	UINT32 errors = 0;

	badCharacters = L"";

	for ( int i = 0; callsign[i] != 0; ++i )
	{
		UINT16 symbol = chatGetHandleCharacterSymbol(callsign[i]);
		if ( symbol != 0 )
			continue;

		if ( badCharacters.empty() == false )
			badCharacters += L", ";
		badCharacters += L"'";
		badCharacters += callsign[i];
		badCharacters += L"'";

		++errors;
	}

	if ( errors == 0 )
		return;

	if ( errors == 1 )
		badCharacters.insert(0, L"Invalid character: ");
	else if ( errors > 1 )
		badCharacters.insert(0, L"Invalid characters: ");

	static char message[512];
	WWII::ConvertChars(badCharacters.c_str(), message, sizeof(message));
	throw message;
}

//////////////////////////////////////////////////////////////////////
// Main function

int
main(int argc, char* argv[])
{
	if ( argc < 2 || argv[1][0] == '-' )
	{
		printf("Usage: %s gamename1 [... gamenameN]\n", argv[0]);
		printf("Tests one or more gamenames for validity\n");
		exit(0);
	}

	for ( int i = 1; i < argc; ++i )
	{
		printf("Checking [%s]: ", argv[i]);

		// Translate to wide characters.
		wchar_t callsign[513];
		WWII::ConvertChars(argv[i], callsign, sizeof(callsign));

		try
		{
			// Basic length check.
			if ( wcslen(callsign) > (UINT32)MAX_HANDLE_WLEN )
				throw "Too long";

			// If this says it's ok, it's ok.
			if ( chatIsValidCallsign(callsign) )
			{
				UINT64 handle = chatGetHandle(callsign);
				if ( handle == 0 )
					throw "Does not parse";

				const wchar_t* unpacked = chatGetWideCallsign(handle);
				if ( wcscmp(callsign, unpacked) != 0 )
					throw "Produces truncated callsign";

				printf("OK [%" PR64bitUT "].\n", handle);
				continue;
			}

			// If it starts with a roman character, it can't contain
			// UTF8 characters.
			if ( (callsign[0] >= 'a' && callsign[0] <= 'z')
				|| (callsign[0] >= 'A' && callsign[0] <= 'Z')
				|| (callsign[0] >= '0' && callsign[0] <= '9') )
			{
				// Check the combination protocol.
				_testCombination(callsign);
			}
			else
			{
				if ( wcslen(callsign) > 5 )
					throw "Too long";
			}

			_checkCharacters(callsign);

			printf("///TODO\n");

		}
		catch ( const char* error )
		{
			printf("FAIL: %s.\n", error);
		}
		catch ( ... )
		{
			printf("FAIL: *INTERNAL ERROR: Generated unhandled exception.\n");
		}
	}
}

#include "syscfg.h"
#include "sysAntihack.h"

#include <pa/md5.h>


namespace NetFrameSync {


//==============================================================================
// OS helpers
#if platOS == OS_WIN32
namespace {
	// Manual version of GetProcAddress, just to make things a *little* less easy to detect/hook.
	FARPROC GetProcedureAddress(HANDLE hModule, const char* pszProcName) {
		IMAGE_DOS_HEADER* pdhDosHeader = static_cast<IMAGE_DOS_HEADER*>(hModule);
		if ( pdhDosHeader->e_magic != IMAGE_DOS_SIGNATURE ) return NULL;
		IMAGE_NT_HEADERS* pndNTHeader = (IMAGE_NT_HEADERS*)(pdhDosHeader->e_lfanew + (long)hModule);
		if ( pndNTHeader->Signature !=IMAGE_NT_SIGNATURE ) return NULL;

		// Traverse the export table to see if we can find the export
		IMAGE_EXPORT_DIRECTORY* iedExports = (IMAGE_EXPORT_DIRECTORY*)(pndNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + (long)hModule);
		long* pNames = (long*)(iedExports->AddressOfNames + (long)hModule);
		short wOrdinalIndex = -1;
		for ( int i = 0; i < (int)iedExports->NumberOfFunctions; ++i ) {
			char* pszFunctionName = (char *)(pNames[i] + (long)hModule);
			if ( lstrcmpiA(pszFunctionName, pszProcName) == 0 ) {
				wOrdinalIndex = i;
				break;
			}
		}
		if ( wOrdinalIndex == -1 ) return NULL;

		// wIndex now holds the index of the function name in the names array, which is the index of the ordinal.
		// The ordinal also acts as the index of the address
		short* pOrdinals = (short*)(iedExports->AddressOfNameOrdinals + (long)hModule);
		unsigned long* pAddresses = (unsigned long*)(iedExports->AddressOfFunctions + (long)hModule);

		short wAddressIndex = pOrdinals[wOrdinalIndex] ;
		return (FARPROC)(pAddresses[wAddressIndex] + (long)hModule);
	}


	DWORD NameToOrdinal(HMODULE hModule, LPCSTR lpProcName) {
		HINSTANCE               pBase;
		PIMAGE_DOS_HEADER       pDOSHeader;
		PIMAGE_NT_HEADERS32     pNTHeader;
		PIMAGE_EXPORT_DIRECTORY pExportDir;
		DWORD                   lExportOffset, lExportSize;
		PDWORD                  pNamesArray;
		int                     lNumberOfNames;
		WORD                    nNameIndex;
		LPCSTR                  szName;
		PWORD                   pNameOrdinalsArray;
		int                     i;

		// Library base address
		pBase = hModule;

		// Verify DOS header
		pDOSHeader = (PIMAGE_DOS_HEADER)pBase;
		if ( pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE ) // "MZ"
			return -1;

		// NT Header   
		pNTHeader = (PIMAGE_NT_HEADERS)((LONG)pDOSHeader + pDOSHeader->e_lfanew);
		if ( pNTHeader->Signature != LOWORD(IMAGE_NT_SIGNATURE) ) // "PE"
			return -1;

		// RVA of export table   
		lExportOffset = pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
		lExportSize = pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

		// Pointer to export table   
		pExportDir = (PIMAGE_EXPORT_DIRECTORY)((DWORD)pBase + lExportOffset);

		// Pointer and length of AddressOfNames array   
		pNamesArray = (PDWORD)((DWORD)pBase + pExportDir->AddressOfNames);
		lNumberOfNames = (int)pExportDir->NumberOfNames;

		// Empty names array
		if ( lNumberOfNames == 0 )
			return -1;

		// Pointer to AddressOfNameOrdinals array   
		pNameOrdinalsArray = (PWORD)((DWORD)pBase + pExportDir->AddressOfNameOrdinals);

		// Search for name of function in the NamesArray   
		nNameIndex = -1;
		for ( i = 0; i < lNumberOfNames; i++ ) {
			szName = (char *)((DWORD)pBase + pNamesArray[i]);

			if ( !strcmp(szName, lpProcName) ) {
				nNameIndex = i;
				break;
			}
		}

		// Not found   
		if ( nNameIndex == (WORD)-1 )
			return -1;

		// Return ordinal #   
		return pNameOrdinalsArray[nNameIndex] + pExportDir->Base;
	}


#define IMAGE_FIRST_SECTION32( ntheader ) ((PIMAGE_SECTION_HEADER)        \
		((UINT64*)ntheader +                                               \
		 FIELD_OFFSET( IMAGE_NT_HEADERS32, OptionalHeader ) +                 \
		 ((PIMAGE_NT_HEADERS32)(ntheader))->FileHeader.SizeOfOptionalHeader   \
		))

	int GetProcLength(HMODULE hModule, DWORD Ordinal) {
		HINSTANCE               pBase;
		PIMAGE_DOS_HEADER       pDOSHeader;
		PIMAGE_NT_HEADERS32     pNTHeader;
		PIMAGE_EXPORT_DIRECTORY pExportDir;
		PIMAGE_SECTION_HEADER   pImageSectionArray;
		DWORD                   lExportOffset, lExportSize;
		DWORD                   SectionStart, SectionEnd;
		PDWORD                  pFunctionArray;
		FARPROC                 pFunctionAddr, pCurrentFunctionAddr;
		int                     nNumberOfSections;
		int                     i, Diff, Length;
		char                    szForwardedModule[MAX_PATH];
		char*                   szForwardedFunctionName;
		HINSTANCE               hForwardedMod;

		// Library base address
		pBase = hModule;

		// Verify DOS header
		pDOSHeader = (PIMAGE_DOS_HEADER)pBase;
		if ( pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE ) // "MZ"
			return 0;

		// NT Header   
		pNTHeader = (PIMAGE_NT_HEADERS)((LONG)pDOSHeader + pDOSHeader->e_lfanew);
		if ( pNTHeader->Signature != LOWORD(IMAGE_NT_SIGNATURE) ) // "PE"
			return 0;

		// RVA of export table   
		lExportOffset = pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
		lExportSize = pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

		// Pointer to export table and export functions array
		pExportDir = (PIMAGE_EXPORT_DIRECTORY)((DWORD)pBase + lExportOffset);
		pFunctionArray = (PDWORD)((DWORD)pBase + pExportDir->AddressOfFunctions);

		// Check if requested ordinal # is valid
		if ( (Ordinal < pExportDir->Base) ||
			((Ordinal - pExportDir->Base) > pExportDir->NumberOfFunctions) )
			return 0;

		// This works because the export table cannot have gaps
		// (if the ordinal is a gap then the corresponding export table entry contains zero)
		pFunctionAddr = (FARPROC)((DWORD)pBase + pFunctionArray[Ordinal - pExportDir->Base]);

		// Export forward ?
		if ( ((DWORD)pFunctionAddr >= (DWORD)pExportDir) && ((DWORD)pFunctionAddr < ((DWORD)pExportDir + lExportSize)) ) {
			szForwardedFunctionName = strchr((char *)pFunctionAddr, '.');
			if ( !szForwardedFunctionName )
				return 0;

			// Module (DLL) name and function name
			FillMemory(szForwardedModule, sizeof(szForwardedModule), 0);
			strncpy(szForwardedModule, (char *)pFunctionAddr, szForwardedFunctionName - (char *)pFunctionAddr);
			szForwardedFunctionName++;

			// Load library   
			hForwardedMod = LoadLibraryA(szForwardedModule);
			if ( !hForwardedMod )
				return 0;

			// Get length of function   
			Length = GetProcLength(hForwardedMod, NameToOrdinal(hForwardedMod, szForwardedFunctionName));
			FreeLibrary(hForwardedMod);
			return Length;
		}

		Length = INT_MAX;

		for ( i = 1; i <= (int)pExportDir->NumberOfFunctions; i++ ) {
			pCurrentFunctionAddr = (FARPROC)((DWORD)pBase + pFunctionArray[i - pExportDir->Base]);
			Diff = (PBYTE)pCurrentFunctionAddr - (PBYTE)pFunctionAddr;
			if ( Diff > 0 )
				Length = std::min(Length, Diff);
		}

		// This is the exported function with the higher address.   
		// Use the end of the image section to calculate the length of the function.   
		if ( Length == INT_MAX ) {
			pImageSectionArray = IMAGE_FIRST_SECTION32(pNTHeader);
			nNumberOfSections = pNTHeader->FileHeader.NumberOfSections;

			// Search all image sections   
			for ( i = 0; i < nNumberOfSections; i++ ) {
				// Code section   
				if ( pImageSectionArray[i].Characteristics & IMAGE_SCN_CNT_CODE ) {
					SectionStart = (DWORD)pBase + pImageSectionArray[i].VirtualAddress;
					SectionEnd = SectionStart + pImageSectionArray[i].Misc.VirtualSize;

					// Function lies within this section ?   
					if ( (DWORD)pFunctionAddr >= SectionStart && (DWORD)pFunctionAddr <= SectionEnd ) {
						Length = SectionEnd - (DWORD)pFunctionAddr;
						break;
					}
				}
			}
		}
		return Length;
	}
}
#endif // WIN32

//==============================================================================
// The actual anti-hack stuff. Currently just a quick-and-dirty checksum of code
// to see if it's been modified.

// The encrypt/decrypt and hash generation code.
namespace {
	using EncryptInfo = FrameHash::EncryptInfo;

	void Encrypt(char* out, const char* in, size_t len, EncryptInfo info) {
		// TODO
		memcpy(out, in, len);
	}

	void Decrypt(char* out, const char* in, size_t len, EncryptInfo info) {
		// TODO
		memcpy(out, in, len);
	}

	EncryptInfo GenerateNewEncryptInfo(void) {
		// TODO
		return frametime;
	}

	void Md5Hash(char md5[16], const char* c, size_t len) {
		MD5_CTX ctx;
		MD5Init(&ctx);
		MD5Update(&ctx, (unsigned char*)c, len);
		MD5Final((unsigned char*)md5, &ctx);
	}

	//--------------

	void Process(FrameHash& hash) {
#if platOS == OS_WIN32
		static const auto str_len = FrameHash::str_len;
		static const auto md5_len = FrameHash::md5_len;

		char libname[str_len];
		Decrypt(libname, hash.lib, str_len, hash.info);
		char funname[str_len];
		Decrypt(funname, hash.fun, str_len, hash.info);
		char md5[md5_len];

		if ( ! hash.processed ) {
			HMODULE lib = LoadLibraryA(libname);
			FARPROC addr = GetProcedureAddress(lib, funname);
			size_t func_len = GetProcLength(lib, NameToOrdinal(lib, funname));
			if ( addr == NULL || func_len == 0 ) {
				dbugLog(_FLW, "Failed to generate seed data for FrameHash function.");
			} else {
				Md5Hash(md5, (const char*)addr, func_len);
			}
			FreeLibrary(lib);
		} else {
			// Don't overwrite values until we've reported the old ones to the host.
			Decrypt(md5, hash.md5, 16, hash.info);
		}

		// Even if we haven't pulled new data, reencrypt with new info every update
		// so it always looks like we're doing something.
		hash.info = GenerateNewEncryptInfo();
		Encrypt(hash.lib, libname, str_len, hash.info);
		Encrypt(hash.fun, funname, str_len, hash.info);
		Encrypt(hash.md5, md5, md5_len, hash.info);
		hash.processed = true;
#else
		// This is Windows only for now.
		hash.processed = true;
#endif
	}
}

//------------------------------------------------------------------------------
// Message & update-related code

namespace {
	static const size_t num_hashes = 24;
	FrameHash hashes[num_hashes] = {};

	// Send/recv/update are separate to make our messages less predictable: Updates occur
	size_t update_idx = 0; // Increments every X frames
	size_t send_idx = 0;  // Increments every time we send, cannot pass recv
	size_t recv_idx = 0;  // Increments every time we receive, cannot enter send
}


void InitializeHashes(const char* data) {
	static const auto str_len = FrameHash::str_len;
	for ( size_t i = 0 ; i < num_hashes ; ++i ) {
		memcpy(hashes[i].lib, data, str_len); data += str_len;
		memcpy(hashes[i].fun, data, str_len); data += str_len;
	}

	for ( FrameHash& hash : hashes )
		Process(hash);
}


void UpdateNextHash(void) {
	Process(hashes[update_idx++]);
	if ( update_idx >= num_hashes ) update_idx = 0;
}


void DumpHashes(ACTION_OBJECT* a) {
	for ( FrameHash& hash : hashes ) {
		if ( ! hash.processed )
			Process(hash);
		hash.packer(TEUL_PACK, a);
	}
}


void SendHash(ACTION_OBJECT* a) {
	if ( send_idx == recv_idx ) // Can't send a reply to a message we haven't received
		return;
	if ( ! hashes[send_idx].processed )
		return;

	hashes[send_idx++].packer(TEUL_PACK, a);
	if ( send_idx >= num_hashes ) send_idx = 0;
}


void RecvHash(ACTION_OBJECT* a) {
	auto next_idx = recv_idx;
	if ( next_idx >= num_hashes ) next_idx = 0;
	if ( next_idx == send_idx ) {
		dbugLog(_FLW, "The host has requested too many frame hashes without giving us time to respond. This can lead to serious sync issues.");
		return;
	}

	hashes[recv_idx].packer(TEUL_UNPACK, a);
	recv_idx = next_idx;
}


}
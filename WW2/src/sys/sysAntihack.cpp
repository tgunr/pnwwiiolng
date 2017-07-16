#include "syscfg.h"
#include "sysAntihack.h"

#include <random>

#include <fio/fio.h>
#include <mem/mem.h>
#include <pa/md5.h>
#include <wwii/wwiiAntihack.h>


namespace NetFrameSync {


//==============================================================================
// OS helpers
#if platOS == OS_WIN32
namespace {
	struct ProcInfo {
		FARPROC addr;
		size_t len;
	};


	// Manual version of GetProcAddress, just to make things a *little* less easy to detect/hook.
	// TODO: OMG the type garbage here is awful. Just give up on playing nice with
	// the Windows API and do things properly.
	ProcInfo GetProcedureInfo(HANDLE lib, const char* name) {
		ProcInfo info = { NULL };

		if ( lib == NULL ) return info;
		IMAGE_DOS_HEADER* dos_header = static_cast<IMAGE_DOS_HEADER*>(lib);
		if ( dos_header->e_magic != IMAGE_DOS_SIGNATURE ) return info;
		IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)(dos_header->e_lfanew + (LONG)lib);
		if ( nt_header->Signature !=IMAGE_NT_SIGNATURE ) return info;

		// Traverse the export table to see if we can find the export
		IMAGE_EXPORT_DIRECTORY* exports = (IMAGE_EXPORT_DIRECTORY*)(nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + (LONG)lib);
		LONG* names = (LONG*)(exports->AddressOfNames + (LONG)lib);
		short func_idx = -1;
		for ( int i = 0; i < (int)exports->NumberOfFunctions; ++i ) {
			char* pszFunctionName = (char *)(names[i] + (LONG)lib);
			if ( lstrcmpiA(pszFunctionName, name) == 0 ) {
				func_idx = i;
				break;
			}
		}
		if ( func_idx == -1 ) {
			return info;
		}

		short* ordinals = (short*)(exports->AddressOfNameOrdinals + (LONG)lib);
		ULONG* addresses = (ULONG*)(exports->AddressOfFunctions + (LONG)lib);

		DWORD ordinal = (DWORD)ordinals[func_idx] ;
		info.addr = (FARPROC)(addresses[ordinal] + (LONG)lib);


		info.len = std::numeric_limits<decltype(info.len)>::max();
		for ( DWORD i = 0 ; i < exports->NumberOfFunctions ; ++i ) {
			FARPROC addr = (FARPROC)(addresses[i] + (LONG)lib);
			ptrdiff_t diff = (PBYTE)addr - (PBYTE)info.addr;
			if ( diff > 0 ) {
				info.len = std::min(info.len, (size_t)diff);
			}
		}

		if ( info.len == std::numeric_limits<decltype(info.len)>::max() ) {
			// This is the last exported function in the module, so we need to
			// use the end of the image section to figure out its length.
			IMAGE_SECTION_HEADER* section_array = (IMAGE_SECTION_HEADER*)(
				(UINT64*)nt_header +
				FIELD_OFFSET(IMAGE_NT_HEADERS32, OptionalHeader) +
				((IMAGE_NT_HEADERS32*)nt_header)->FileHeader.SizeOfOptionalHeader);

			for ( WORD i = 0 ; i < nt_header->FileHeader.NumberOfSections ; ++i ) {
				if ( section_array[i].Characteristics & IMAGE_SCN_CNT_CODE ) {
					DWORD start = (DWORD)lib + section_array[i].VirtualAddress;
					DWORD end = start + section_array[i].Misc.VirtualSize;
					if ( start <= (DWORD)info.addr && (DWORD)info.addr <= end ) {
						info.len = (size_t)(end - (DWORD)info.addr);
						break;
					}
				}
			}
		}

		if ( info.len == std::numeric_limits<decltype(info.len)>::max() )
			info.addr = NULL; // No length? No function

		return info;
	}
}
#endif // WIN32

//==============================================================================
// The actual anti-hack stuff. Currently just a quick-and-dirty checksum of code
// to see if it's been modified.

// The encrypt/decrypt and hash generation code.
namespace {
	enum HashState { HASH_RECV, HASH_PROC, HASH_SENT };
	struct StoredHashData {
		FrameHashData hash;
		HashState state;
	};

	//--------------

	void Md5Hash(char md5[16], const char* c, size_t len) {
		MD5_CTX ctx;
		MD5Init(&ctx);
		MD5Update(&ctx, (unsigned char*)c, len);
		MD5Final((unsigned char*)md5, &ctx);
	}

	//--------------

	void Process(StoredHashData& data) {
#if platOS == OS_WIN32
		// Copied to avoid ever having anything unobfuscated in global memory
		FrameHashData hash = data.hash;
		MemoryRequest& req = reinterpret_cast<MemoryRequest&>(hash);
		Decode(req);
		// Just to be safe in case the host sent us garbage
		req.lib[array_size(req.lib)-1] = '\0';
		req.fun[array_size(req.fun)-1] = '\0';
		
		if ( data.state == HASH_RECV ) {
			HMODULE lib = LoadLibraryA(req.lib);
			auto info = GetProcedureInfo(lib, req.fun);
			if ( info.addr == NULL ) {
				dbugLog(_FLW, "Failed to generate seed data for FrameHash function.");
			} else {
				Md5Hash(req.md5, (const char*)info.addr, info.len);
			}
			FreeLibrary(lib);

			data.state = HASH_PROC;
		}

		// Even if we haven't pulled new data, reobfuscate every update
		// so it always looks like we're doing something.
		req.key = (uint32_t)random();
		Encode(req);
		data.hash = hash;
#else
		// Currently nothing for non-Windows
		MemoryRequest& req = reinterpret_cast<MemoryRequest&>(hash);
		if ( data.state == HASH_RECV ) {
			memset(req.md5, (unsigned char)req.key, MemoryRequest::md5_len);
			data.state = HASH_PROC;
		}
#endif
	}
}

//------------------------------------------------------------------------------
// Message & update-related code

namespace {
	static const size_t num_hashes = 24;
	StoredHashData hashes[num_hashes] = {};

	// Send/recv/update are separate to make our messages less predictable
	size_t update_idx = 0; // Increments every X frames
	size_t send_idx = 0;  // Increments every time we send, cannot pass recv
	size_t recv_idx = 0;  // Increments every time we receive, cannot enter send
}


void InitializeHashes(void) {
	char* data = nullptr;
	unsigned int size = 0;
	if ( ! fioLoad("framehash_seed.bin", data, &size) ) {
		dbugLog(_FLA, "Failed to load seed data for frame hashes! This could lead to serious sync issues.");
		return;
	}

	// HACK: Dumped FrameHashData for a bunch of functions and saved it in a file.
	for ( size_t i = 0 ; i < array_size(hashes) && i < size / sizeof(FrameHashData) ; ++i ) {
		memcpy(&hashes[i].hash, data + (i * sizeof(FrameHashData)), sizeof(FrameHashData));
		hashes[i].state = HASH_RECV;
	}
	memFree(data);

	for ( auto& hash : hashes ) {
		Process(hash);
	}
}


void UpdateNextHash(void) {
	Process(hashes[update_idx++]);
	if ( update_idx >= num_hashes ) update_idx = 0;
}


void DumpHashes(ACTION_OBJECT* a) {
	for ( auto& data : hashes ) {
		if ( data.state == HASH_RECV ) {
			Process(data);
		}
		if ( data.state == HASH_PROC ) {
			data.hash.packer(TEUL_PACK, a);
		}
	}
}

FrameHashData* NextHash(void) {
	if ( hashes[send_idx].state != HASH_PROC ) {
		return nullptr;
	}

	FrameHashData* hash = &hashes[send_idx].hash;
	hashes[send_idx++].state = HASH_SENT;
	if ( send_idx >= num_hashes ) send_idx = 0;
	return hash;
}


void RecvHash(const FrameHashData& hash) {
	auto next_idx = recv_idx+1;
	if ( next_idx >= num_hashes ) next_idx = 0;
	if ( next_idx == send_idx && hashes[recv_idx].state != HASH_SENT ) {
		dbugLog(_FLW, "The host has requested too many frame hashes without giving us time to respond. This can lead to serious sync issues.");
		return;
	}

	hashes[recv_idx].hash = hash;
	hashes[recv_idx].state = HASH_RECV;
	recv_idx = next_idx;
}


}
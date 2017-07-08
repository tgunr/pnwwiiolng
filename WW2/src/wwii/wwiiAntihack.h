#pragma once


struct FrameHashData {
	SINT32 packer(PackMode mode, ACTION_OBJECT* a) {
		a->packer(mode, data, sizeof(data)*8);
		return a->offset;
	}

	// Hash of the in-code memory a function
	char data[30+30+16+4];
};


struct MemoryRequest {
	static const size_t str_len = 30;
	static const size_t md5_len = 16;

	char lib[str_len];
	char fun[str_len];
	char md5[md5_len];
	// Encrypts the above to avoid anything obvious in memory/packets
	uint32_t key;
};

static_assert ( sizeof(FrameHashData) == sizeof(MemoryRequest) , "Helper struct doesn't match hidden" );

inline void Encrypt(MemoryRequest&) {} // TODO
inline void Decrypt(MemoryRequest&) {} // TODO
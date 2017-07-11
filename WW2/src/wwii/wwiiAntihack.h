#pragma once


struct FrameHashData {
	FrameHashData(void) {}
	FrameHashData(ACTION_OBJECT* a) {
		a->offset = packer(TEUL_UNPACK, a);
	}

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

static_assert ( sizeof(FrameHashData) == sizeof(MemoryRequest) ,
                "Helper struct's size needs to match the obfuscating type's size" );

inline void Encode(MemoryRequest&) {} // TODO
inline void Decode(MemoryRequest&) {} // TODO

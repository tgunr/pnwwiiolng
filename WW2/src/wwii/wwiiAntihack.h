#pragma once


struct MemoryRequest {
	static const size_t str_len = 30;
	static const size_t md5_len = 16;

	char lib[str_len];
	char fun[str_len];
	char md5[md5_len];
	uint32_t key; // Masks the above to avoid anything obvious in memory/packets
};

struct FrameHashData {
	FrameHashData(void) {}
	FrameHashData(ACTION_OBJECT* a) {
		a->offset = packer(TEUL_UNPACK, a);
	}

	SINT32 packer(PackMode mode, ACTION_OBJECT* a) {
		a->offset = pack(mode, a->packed, a->offset);
		return a->offset;
	}

	// Because there are twenty different versions of this same code around
	SINT32 pack(PackMode mode, UCHAR* const buffer, SINT32 offset) {
		offset = teulPackVoid(mode, buffer, offset, a_, sizeof(a_));
		offset = teulPackVoid(mode, buffer, offset, b_, sizeof(b_));
		offset = teulPackVoid(mode, buffer, offset, c_, sizeof(c_));
		offset = teulPackUINT32(mode, buffer, offset, &d_, 0);
		return offset;
	}

	char a_[MemoryRequest::str_len];
	char b_[MemoryRequest::str_len];
	char c_[MemoryRequest::md5_len];
	uint32_t d_;
};

static_assert ( sizeof(FrameHashData) == sizeof(MemoryRequest) ,
                "Helper struct's size needs to match the obfuscating type's size" );

#define ENABLE_MASKING 1 // TESTING

// TODO: Implement something better when we have more than 10 minutes to spare
inline void Encode(MemoryRequest& req) {
#ifdef ENABLE_MASKING
	char* beg = reinterpret_cast<char*>(&req);
	const size_t req_size = sizeof(MemoryRequest) - sizeof(uint32_t); // Ignores key
	auto Get = [beg, req_size] (int i) -> char& {
		while ( i < 0 ) i += (int)req_size;
		while ( i > (int)req_size ) i -= (int)req_size;
		return *(beg + i);
	};

	for ( int i = 0 ; i < (int)req_size ; ++i ) {
		Get(i) += 0xff + i * req.key + i;
	}

	req.key += 0xff;
#endif
}

inline void Decode(MemoryRequest& req) {
#ifdef ENABLE_MASKING
	char* beg = reinterpret_cast<char*>(&req);
	const size_t req_size = sizeof(MemoryRequest) - sizeof(uint32_t); // Ignores key
	auto Get = [beg, req_size] (int i) -> char& {
		while ( i < 0 ) i += (int)req_size;
		while ( i > (int)req_size ) i -= (int)req_size;
		return *(beg + i);
	};

	req.key -= 0xff;

	for ( int i = 0 ; i < (int)req_size ; ++i ) {
		Get(i) -= 0xff + i * req.key + i;
	}
#endif
}

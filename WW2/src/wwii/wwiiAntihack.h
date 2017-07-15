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
		a->offset = teulPackVoid(mode, a->packed, a->offset, a, sizeof(a));
		a->offset = teulPackVoid(mode, a->packed, a->offset, b, sizeof(b));
		a->offset = teulPackVoid(mode, a->packed, a->offset, c, sizeof(c));
		a->offset = teulPackUINT32(mode, a->packed, a->offset, &d, 0);
		return a->offset;
	}

	char a[MemoryRequest::str_len];
	char b[MemoryRequest::str_len];
	char c[MemoryRequest::md5_len];
	uint32_t d;
};

static_assert ( sizeof(FrameHashData) == sizeof(MemoryRequest) ,
                "Helper struct's size needs to match the obfuscating type's size" );


// TODO: Implement something better when we have more than 10 minutes to spare
inline void Encode(MemoryRequest& req) {
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
}

inline void Decode(MemoryRequest& req) {
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
}

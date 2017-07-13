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


// TODO: Implement something better when we have more than 10 minutes to spare
inline void Encode(MemoryRequest& req) {
	char* beg = reinterpret_cast<char*>(&req);
	const size_t req_size = sizeof(MemoryRequest) - sizeof(uint32_t); // Ignores key
	auto Get = [beg, req_size] (int i) -> char& {
		while ( i < 0 ) i += (int)req_size;
		while ( i > (int)req_size ) i -= (int)req_size;
		return *(beg + i);
	};
	
	for ( int i = 0 ; i < req_size ; i += req.key ) {
		if ( req.key == 0 ) break;
		std::swap(Get(i), Get(i+req.key));
	}

	for ( int i = 0 ; i < req_size ; ++i ) {
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

	for ( int i = 0 ; i < req_size ; ++i ) {
		Get(i) -= 0xff + i * req.key + i;
	}

	for ( int i = 0 ; i < req_size ; i += req.key ) {
		if ( req.key == 0 ) break;
		std::swap(Get(i), Get(i+req.key));
	}
}

#pragma once


// NOTE: Is actually antihack stuff
namespace NetFrameSync {
	struct FrameHash {
		static const size_t str_len = 64;
		static const size_t md5_len = 16;

		char lib[str_len];
		char fun[str_len];
		char md5[md5_len];
		bool processed = false;
		// encryption stuff here
		typedef int EncryptInfo; // Whatever info is used to encrypt/decrypt the hash.
		int info;

		SINT32 packer(PackMode mode, ACTION_OBJECT* a) {
#if platBuild == BUILD_CLIENT
			if ( mode == TEUL_PACK && processed == false )
				dbugLog(_FLW, "Sending an unprocessed frame hash! This can lead to serious sync issues.");
#endif
			a->packer(mode, lib, str_len);
			a->packer(mode, fun, str_len);
			a->packer(mode, md5, md5_len);
			a->packer(mode, &info, 0); // TODO
			return a->offset;
		}
	};


	void InitializeHashes(const char* seed);
	void UpdateNextHash(void);

	void DumpHashes(ACTION_OBJECT*); // TODO: Dump stored hashes when we first connect
	// NOTE: This may not actually send anything; we only send after the hash has been processed.
	void SendHash(ACTION_OBJECT*); // TODO: 
	void RecvHash(ACTION_OBJECT*);

	// Update every X frames
	static const UINT64 tick_check_time = 500;


	// TEMP/TESTING: Exposing for dot command.
	void Process(FrameHash&);
}
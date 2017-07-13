#pragma once

struct FrameHashData;

// NOTE: Is actually antihack stuff
namespace NetFrameSync {
	void InitializeHashes(void);
	void UpdateNextHash(void);

	void DumpHashes(ACTION_OBJECT*); // TODO: Dump stored hashes when we first connect
	// NOTE: This may not actually send anything; we only send after the hash has been processed.
	void SendHash(ACTION_OBJECT*);

	void RecvHash(ACTION_OBJECT*);
	void RecvHash(const FrameHashData&);

	// Update every X frames
	static const UINT64 tick_check_time = 200;
}
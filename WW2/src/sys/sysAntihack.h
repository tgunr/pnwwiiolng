#pragma once

struct FrameHashData;

// NOTE: Is actually antihack stuff
namespace NetFrameSync {
	void InitializeHashes(void);
	void UpdateNextHash(void);

	void DumpHashes(ACTION_OBJECT*); // TODO: Dump stored hashes when we first connect
	// Retrieves the next hash to be sent, if any
	FrameHashData* NextHash(void);

	void RecvHash(const FrameHashData&);

	// Update every X frames
	static const UINT64 tick_check_time = 200;
}
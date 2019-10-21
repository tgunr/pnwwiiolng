#pragma once


namespace MissionJoinRequest {
	struct Request {
		Request(ChatHandle p) : player(p) {}
		Request(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }
		SINT32 packer(PackMode mode, ACTION_OBJECT* a) {
			a->offset = wwiiPackHandle(mode, a->packed, a->offset, &player, 0);
			return a->offset;
		}
		bool send(SINT32 chatConnection);

		ChatHandle player;
	};

	extern TEUL_CALLBACK* callback;
}
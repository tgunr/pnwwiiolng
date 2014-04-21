#include <syscfg.h> // Path is project specific.
#include "wwii/wwiiSyncVar.h"

namespace WWII
{

bool
VarGroup::pack(ACTION_OBJECT* const a, const TeulTimeMS& lastPacked) const
{
	if (m_lastModified < lastPacked)
		return false;

	// Ensure a->offset is nicely rounded.
	a->offset = (a->offset + 7) & ~7;

	const SINT32 changesOffset = a->offset;
	const SINT32 changeBits = (m_members + 7) & ~7;	// Round up to next whole byte.
	a->offset += changeBits;

	UINT32 changes = 0;
	groupPack(a, lastPacked, changes);

	teulPackUINT32(TEUL_PACK, a->packed, changesOffset, &changes, changeBits);

	return (a->offset >= 0);
}

bool
VarGroup::unpack(ACTION_OBJECT* const a, const TeulTimeMS& lastPacked)
{
	// Ensure a->offset is nicely rounded.
	a->offset = (a->offset + 7) & ~7;

	const SINT32 changeBits = (m_members + 7) & ~7;	// Round up to next whole byte.
	UINT32 changes = 0;
	a->packer(TEUL_UNPACK, changes, changeBits);

	if (changes == 0)
		return false;

	groupUnpack(a, lastPacked, changes);

	return (a->offset >= 0);
}

} // namespace WWII

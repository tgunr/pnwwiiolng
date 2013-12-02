#include <syscfg.h> // Path is project specific.
#include "teul/teulPackers.h"
#include "omg/omgMissionStatus.h"

namespace Chat
{

void
_doPack(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UCHAR& changes, WWII::VarGroup& varGroup, const UINT32 fieldNo)
{
	if ( mode == TEUL_PACK )
	{
		const bool changed = varGroup.packer(mode, a, lastPack);
		if ( changed )
			changes |= 1 << fieldNo;
	}
	else
	{
		const bool changed = (changes & (1 << fieldNo)) != 0;
		if ( changed )
			varGroup.packer(mode, a, lastPack);
	}
}

bool
MissionStatus::packer(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack)
{
	// Pack, or create space for, the list of changed groups.
	UCHAR changes = 0;
	SINT32 changesOffset = a->offset;

	a->packer(mode, changes);

	UINT32 fieldNo = 0;
	if ( mode == TEUL_PACK || (mode == TEUL_UNPACK && changes != 0) )
	{
		_doPack(mode, a, lastPack, changes, m_header, fieldNo++);
		_doPack(mode, a, lastPack, changes, m_status, fieldNo++);
		_doPack(mode, a, lastPack, changes, m_origin, fieldNo++);
		_doPack(mode, a, lastPack, changes, m_target, fieldNo++);
		_doPack(mode, a, lastPack, changes, m_lead, fieldNo++);
		_doPack(mode, a, lastPack, changes, m_fru, fieldNo++);
	}

	if ( mode == TEUL_PACK )
		teulPackUCHAR(mode, a->packed, changesOffset, &changes, 0);

	return ( changes != 0 );
}

} // namespace Chat

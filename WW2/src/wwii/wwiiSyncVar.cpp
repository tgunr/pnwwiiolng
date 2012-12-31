#include <syscfg.h> // Path is project specific.
#include "wwii/wwiiSyncVar.h"

namespace WWII
{

bool
VarGroup::packer(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPacked)
{
	if ( mode == TEUL_PACK && m_lastModified < lastPacked )
		return false ;

	// Ensure a->offset is nicely rounded.
	a->offset = (a->offset + 7) & ~7 ;

	const SINT32 changesOffset = a->offset ;
	const SINT32 changeBits = (m_members + 7) & ~7 ;	// Round up to next whole byte.
	UINT32 changes = 0 ;
	a->packer(mode, changes, changeBits) ;

	if ( mode == TEUL_UNPACK && changes == 0 )
		return false ;

	groupPacker(mode, a, lastPacked, changes) ;

	if ( mode == TEUL_PACK )
		teulPackUINT32(mode, a->packed, changesOffset, &changes, changeBits) ;

	return true ;
}

} // namespace WWII

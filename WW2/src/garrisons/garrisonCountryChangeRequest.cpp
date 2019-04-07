#include "syscfg.h" // Path is project specific.

#include "garrisons/garrisonCountryChangeRequest.h"
#include "wwii/wwiiPack.h"

//////////////////////////////////////////////////////////////////////
// Return false if the passed integer does not appear to be a valid
// teulIndex value

static inline bool _invalidTeulIndex(SINT32 teulIndex)
{
	return (teulIndex < 0);
}

SINT32
GarrisonCountryChangeReq::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_result, 8);
	a->offset = wwiiPackFullObjID(mode, a->packed, a->offset, &m_cp, 0); // 48 bits
	a->offset = wwiiPackCountry(mode, a->packed, a->offset, &m_country, 0);
	a->offset = teulPackSINT32(mode, a->packed, a->offset, &m_clientsTeulIndex, 0);

	return a->offset;
}

#if platBuild == BUILD_CLIENT
bool GarrisonCountryChangeReq::requestChangeCountry(SINT32 teulIndex, const FULL_OBJ_ID cp, WWII_COUNTRY const& country)
{
	if (PN_UNLIKELY(_invalidTeulIndex(teulIndex)))
		return false;

	// You must specify where you want to move it to and where you want to move it from
	if (PN_UNLIKELY(!cp.id))
		return false;

	return send(teulIndex, cp, country);
	
}
#endif // platBuild == BUILD_CLIENT
#pragma once

#include "weap/weapSpec.h"
#include "weap/weapDefs.h"
#include "teul/teulPackers.h"
#include "updt/updtEnums.h"
#include "host/hostRecordingSessionTypes.h"
#include "wwii/wwiiPack.h"
#include "wwii/wwiiPackBool.h"
#include "inst/instDescriptor.h"

struct PartialVehicle
{
	sortieid_t	index;			// Sortie ID / Vehicle Index
	UINT32		present;		// Present bits
	UINT16		otb;			// One time bits
	WEAPON_SPEC	spec;			// Weapon spec
	RANK_NO		rank;			// Rank
	bool		packSquadName;
	ChatHandle	squadHandle	;	// Squad handle
	ChatHandle	gameHandle;		// Gamename
	UCHAR		decal;			// Vehicle's decal.
	UINT32		brigadeID;
	UINT32		squadID;
	ChatHandle	squadHandle2;
	UINT32		missionID;
	UINT32		flags;
	UCHAR		mpFlags;
	UINT32		seats[MAX_WEAP_STATIONS];
	Instance::Descriptor instance;		// InstanceID.

	PartialVehicle(UCHAR* const data=NULL)
		: index(0), present(0), otb(0), spec(NULL_WEAP_SPEC), rank(0), squadHandle(0ULL), gameHandle(0ULL)
		, decal(0), brigadeID(0), squadID(0), squadHandle2(0), missionID(0), flags(0), mpFlags(0)
		, instance()
	{
		if ( data )
			packer(TEUL_UNPACK, data);
	}

	bool HasFirstTime() const { return (present & PRX_FIRST_TIME);  }

	SINT32 packer(LOGICAL mode, UCHAR* srcData, UINT32 /*version*/=~0)
	{
		SINT32 offset = 0;
		UCHAR* const realData = srcData + 2;
		switch ( *srcData )
		{
		case Host::evtRemoval:
			if ( mode == TEUL_UNPACK )
				index = *(UINT32*)(realData);
			else
				*(UINT32*)(realData) = index;
			break;
		break;
		case Host::evt1Time:
		case Host::evtUpdate:
			offset = teulPackUINT32(mode, realData, 0, &index, 0);
			offset = teulPackUINT32(mode, realData, offset, &present, PRX_NUM_BITS);
			if ( HasFirstTime() )
			{
				offset = wwiiPackWeaponSpec(mode, realData, offset, &spec, 0);
				offset = teulPackUINT16(mode, realData, offset, &otb, 8);
				offset = wwiiPackRankNo(mode, realData, offset, &rank, 7);
				offset = wwiiPackBool(mode, realData, offset, packSquadName);
				if ( packSquadName )
					offset = wwiiPackHandle(mode, realData, offset, &squadHandle, 0);
				offset = wwiiPackHandle(mode, realData, offset, &gameHandle, 0);
				const bool packDecal = (otb & OTB_USE_DECALS);
				if ( packDecal )
					offset = teulPackUCHAR(mode, realData, offset, &decal, 8);
				offset = teulPackUINT32(mode, realData, offset, &brigadeID, 16);
				offset = teulPackUINT32(mode, realData, offset, &squadID, 0);
				offset = wwiiPackHandle(mode, realData, offset, &squadHandle2, 0);
				offset = teulPackUINT32(mode, realData, offset, &missionID, 0);
				offset = teulPackUINT32(mode, realData, offset, &flags, 0);
				offset = teulPackUCHAR(mode, realData, offset, &mpFlags, 0);
				offset = teulPackVoid(mode, realData, offset, seats, sizeof(UINT32) * MAX_WEAP_STATIONS);

				ACTION_OBJECT a;
				a.packed = realData;
				a.offset = offset;

				offset = instance.packer(mode, &a);
			}
		break;
		default:
			index = 0;
		break;
		}

		return offset;
	}
};


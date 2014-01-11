#include <syscfg.h> // Path is project specific.
#include "pi/pi.h"
#include "include/vars.h"

#include "veh/vehmain.h"
#include "veh/vehcomms.h"
#include "veh/vehupdate.h"
#include "pers/persmain.h"
#include "comm/commClientMessages.h"
#include "rn/rnmain.h"
#include "weap/weapSpec.h"
#include "weap/weapWeapon.h"
#include "comm/commTypes.h"
#include "include/vars.h"
#include "client/GameClient.h"
#include "ui/uiDotCommands.h"


UINT32 vehcommSendVehicleRequest(SINT32 VehicleType, WEAPON_SERIAL const* pWeaponSerial)
{
	if(Online)
	{
		VEHICLE_REQUEST vRequest;

        if(pWeaponSerial == nullptr)
            return 0;

		memset(&vRequest,0,sizeof(vRequest));
		vRequest.timeStamp = frametime;
		vRequest.weapon = *pWeaponSerial;
		commSendVehicleJoin(PlayerClient()->CellConnection(), &vRequest);
	}else
	{
		VEHICLE_REPLY vReply;
		memset(&vReply,0,sizeof(vReply));

		vReply.timeStamp = frametime;

#if RELEASE_CLIENT
        switch(OverrideSpawner)
        {
        case BENCHANTWERP_OVERRIDE:
        case BENCHRAMAGEN_OVERRIDE:
            {
                // Force buzzard
                VehicleType = 0;
                break;
            }
        }
#endif

		vReply.spec = vehtypeWeaponSpec(VehicleType);
		vehcommProcessVehicleRequestResponse(&vReply);
	}

	return 0;
}

UINT32 vehcommProcessVehicleRequestResponse( VEHICLE_REPLY const* Reply)
{
    if(Reply == nullptr)
		piExitProgram(__FILE__, __LINE__,"[vehcommProcessVehicleRequestResponse] Reply == nullptr");

	if (Reply->replyCode < 0)
		piExitProgram(__FILE__, __LINE__,"[vehcommProcessVehicleRequestResponse] replyCode == %d");

	const WEAPON* const pWeapon = weapLookup(Reply->spec);

    if(pWeapon == nullptr)
        piExitProgram(__FILE__, __LINE__,"[vehcommProcessVehicleRequestResponse] pWeapon == nullptr");

	vehmainSpawnVehicle(Reply);

	return 0;
}

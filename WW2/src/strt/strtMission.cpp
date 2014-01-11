#include <syscfg.h> // Path is project specific.
// Created: Fri Aug 17 15:54:15 2001
//
// Handlers and stuff for mission related strat issues
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "strt/strtServerTypes.h"
#include "strt/strtJoinList.h"
#include "strt/strtPostedWeaponList.h"
#include "strt/strtData.h"
#include "strt/strtMission.h"
#include "strt/strtCPStruct.h"
#include "strt/strtFacilityStruct.h"

//Processes the mmHosts request for the "health" of a facility
SINT32
strtProcessCapacityRequest(const MM_CAPACITY_REQUEST& request, MM_CAPACITY_RESPONSE& response, FULL_OBJ_ID destFacilityID, bool vehicleDelivered)
{
	FacilityStruct* facility;
	CPStruct* cp;
	SINT32 capacity;
	UINT32 totalRPs;
	WeapJoinRequestNode* joinNode;

	dbugLog(_FLD, "c:%u m:%d f:%d.%d.%u processing capacity request tag %d"
                , request.customerID, request.missionID
				, request.facility.cell.ox, request.facility.cell.oy, request.facility.id
                , request.tag);

	//Copy the necessary information from one to the other
	response.tag = request.tag;
	response.missionID = request.missionID;
	response.customerID = request.customerID;
	response.facility = request.facility;
	response.totalRPs = 0;
	response.capacity = -1; //Used to indicate an error

	// Assume there was no return/reservation
	response.rtbFacility = nullObjID;
	response.rtbReservedWeapon = NULL_WEAP_SERIAL;
	response.vehicleDelivered = vehicleDelivered;

	// Determine if the player has a reservation here
	if ( request.tag == CAPACITY_AFTER_MISSION )
	{
		response.rtbFacility = destFacilityID;
		joinNode = WeapJoinRequestNode::Lookup(request.customerID);
		if ( joinNode != nullptr )
		{
			JOIN_LIST* list = joinNode->m_joinList;
			if ( list == nullptr )
			{
				dbugLog(_FLW, "joinNode with NULL list");
			}
			else if ( list->m_weapon == nullptr )
			{
				dbugLog(_FLW, "joinNode with NULL weapon");
			}
			else if ( list->m_weapon->m_status == POSTED_WEAPON::_RESERVED )
			{
				response.rtbFacility = list->m_weapon->m_facility->fullObjID;
				response.rtbReservedWeapon = list->m_weapon->m_weaponSerial;
			}
		}
	}

	FULL_OBJ_ID* dest = strtGetFullObjID(response.facility);

	if (dest != nullptr && dest->objClass == CLASS_CP)
	{
		capacity = 0;
		totalRPs = 0;
		//Average the capacity of the CP
		//Total the RP value
		cp = (CPStruct*)dest;
		if ( cp->ownableFacilities > 0 )
		{
			facility = cp->facilityList;

			while (facility!=nullptr)
			{
				if (facility->fullObjID.objType != FACILITY_FIREBASE)
				{
					totalRPs += facility->objectRPTotal;
					capacity += facility->capacity;
				}
				facility = facility->next;
			}

			capacity /= cp->ownableFacilities;

			if ( capacity < 0 )
				capacity = 0;
		}
		else
		{
			dbugLog(_FLA,"Unpopulated cp: %s [%d.%d.%u]", cp->name, dest->cell.ox, dest->cell.oy, dest->id);
		}

		response.capacity = capacity;
		response.totalRPs = totalRPs;
	}
	else if ( dest != nullptr )
	{
		facility = (FacilityStruct*)dest;
		if ( facility->capacity >= 0 )
				response.capacity = facility->capacity;
		else
		{
			dbugLog(_FLW, "%s has negative capacity", facility->name);
			response.capacity = 0;
		}
		response.totalRPs = facility->objectRPTotal;
	}
	else
	{
		dbugLog(_FLA,"Couldn't find strat object %d.%d.%u", request.facility.cell.ox, request.facility.cell.oy, request.facility.id);
	}

	return response.capacity;
}


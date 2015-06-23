#include <syscfg.h>	// Path is project specific.
// Member functions of scoreSortie.

#include "score/scoreSortie.h"
#include "teul/teulTime.h"
#include <unordered_map>
#include <queue>
#include <algorithm>
#include "map/mapPlayer.h"
#include "weap/weapWeapon.h"

typedef std::unordered_map<UINT32 /*sortieID*/, Scoring::Sortie*> SortieTable;
static SortieTable sorties;

typedef std::queue<Scoring::Sortie*> ExpiryList;
static ExpiryList expiringSorties;

//////////////////////////////////////////////////////////////////////
// Lookup the RDP value of a particular vehicle
// Caution: can return 0

static FLOAT32
_getVehicleValue(const WEAPON_SPEC& vehicleType)
{
	WEAPON* weapon = weapLookup(vehicleType);
	if ( weapon == nullptr )
		return 0.0f;

	if ( weapon->value <= 0 )
		return 0.0f;

	return (FLOAT32)weapon->value;
}

//////////////////////////////////////////////////////////////////////
// Constructor for a Sortie object.

Scoring::Sortie::Sortie(Player& pilot_)
	: m_sortieID(pilot_.sortieID())
	, m_spawnTime(teulTime())
	, m_despawnTime(0)
	, m_despawnedTo(nullObjID)
	, m_pilotID(pilot_.playerID())
	, m_crewID(0)
	, m_spec(pilot_.weapon().spec)
	, m_rtbCode(RTB_BAIL_FRIENDLY)
	, m_lastHitBySortieID(0)
	, m_chatHandle(pilot_.chatHandle())
	, m_killedBy(0ULL)
	, m_isPlaynet(pilot_.isPlaynet())
	, m_value(_getVehicleValue(pilot_.weapon().spec))
	, m_hitsTaken()
	, m_pilot(&pilot_)
	, m_crew(nullptr)
	, m_sortiesHit()
	, m_sortiesKilled()
	, m_typesHit()
	, m_typesKilled()
	, m_typesInteractedWith()
{
	sorties[sortieID()] = this;
}

//////////////////////////////////////////////////////////////////////
// Destructor for a sortie.

Scoring::Sortie::~Sortie()
{
	// Ensure they've actually left the sortie.
	assert( m_pilot == nullptr );
	assert( m_crew == nullptr );
}

//////////////////////////////////////////////////////////////////////
// Track the last hit to a sortie.

bool
Scoring::Sortie::addHitBy(const Scoring::Actor& actor_, const UINT64& joules_)
{
	m_lastHitBySortieID = actor_.sortieID();
	m_lastHitTime = teulClock64;

	m_hitsTaken.push_back(Scoring::Sortie::HitEntry(actor_, joules_));

	return true;
}

//////////////////////////////////////////////////////////////////////
// Track hits that we've made.

void
Scoring::Sortie::addHitTo(const Scoring::Actor& actor_, const UINT64& joules_)
{
	auto addition = m_sortiesHit.insert(actor_.sortieID());
	if ( addition.second == false )
	{
		return;
	}

	addInteraction(actor_.weapon());

	++ m_typesHit[actor_.weapon().asUINT32()];
}

//////////////////////////////////////////////////////////////////////
// Add another type to the list of unique types hit/killed.

void
Scoring::Sortie::addInteraction(const WEAPON_SPEC& spec_)
{
	m_typesInteractedWith.insert(spec_.asUINT32());
}

//////////////////////////////////////////////////////////////////////
// Add score for a kill.

void
Scoring::Sortie::addKill(const UINT32 sortieID_, const WEAPON_SPEC& spec_)
{
	assert( sortieID_ != 0 );
	assert( spec_.asUINT32() != 0 );

	auto addition = m_sortiesKilled.insert(sortieID_);
	if ( addition.second == false )
		return;

	addInteraction(spec_);

	++ m_typesKilled[spec_.asUINT32()];
}

//////////////////////////////////////////////////////////////////////
// Attach a crewman to a sortie.

void
Scoring::Sortie::attachCrew(Player& crew_)
{
	assert( m_crew == nullptr );
	m_crew = &crew_;
	m_crewID = crew_.playerID();
}

//////////////////////////////////////////////////////////////////////
// Set the rtb code/dest for this sortie.

void
Scoring::Sortie::setRTB(const CODE_RTB rtbCode_, const FULL_OBJ_ID& rtbDest_)
{
	dbugLog(_FLD, "v:%u setting rtb to %u", m_sortieID, rtbCode_);

	m_rtbCode = rtbCode_;
	if ( rtbCode_ == RTB_SUCCESS )
	{
		if ( rtbDest_.id != 0 )
			m_despawnedTo = rtbDest_;
	}
	else
		m_despawnedTo = nullObjID;

	if ( m_pilot )
	{
		m_pilot->missionPacket().vehicle().rtbState = rtbCode_;
		m_pilot->missionPacket().vehicle().rtbFacility = rtbDest_;
	}
	if ( m_crew )
	{
		m_crew->missionPacket().vehicle().rtbState = rtbCode_;
		m_crew->missionPacket().vehicle().rtbFacility = rtbDest_;
	}
}

//////////////////////////////////////////////////////////////////////
// When a sortie despawns, the players cease initiating new events.
// However we keep the sortie around for a while so that even if the
// players log out or immediately despawn, other sorties can still
// reference it for a while afterwards without having to access a
// database or anything.

void
Scoring::Sortie::despawned()
{
	if ( m_despawnTime != 0 )
	{
		dbugLog(_FLD, "duplicate attempt to despawn sortie.");	///TODO:134:REMOVE
		return;
	}

	m_despawnTime = teulTime();
	if ( m_pilot && m_pilot->sortie() == this )
		m_pilot->missionPacket().sortie().despawned_ms = teulClock64;
	if ( m_crew && m_crew->sortie() == this )
		m_crew->missionPacket().sortie().despawned_ms = teulClock64;

	dbugLog(_FLD, "v:%u despawned, pushing to expiration queue", sortieID());

	// Push it onto the queue of sorties waiting to time out so we can
	// actually purge it at a later time.
	expiringSorties.push(this);
}

//////////////////////////////////////////////////////////////////////
// Check the expiry queue for sorties that have had plenty of time to
// be referenced by other sorties and events, etc. E.g., make sure
// that a sortie that dropped bombs is around long enough for the
// bombs to hit the ground; for a satchel charge to go off etc.

void
mapExpireSorties()
{
	// Keep sorties in memory for 10 minutes
	static const UINT32 cutoff = teulTime() - (10 * 60);
	// If they despawned more than 10 minutes ago, we can expire them.
	while ( !expiringSorties.empty() && expiringSorties.front()->despawnTime() < cutoff )
	{
		Scoring::Sortie* const sortie = expiringSorties.front();
		expiringSorties.pop();
		dbugLog(_FLD, "v:%u expired", sortie->sortieID());
		sorties.erase(sortie->sortieID());
		delete sortie;
	}
}

//////////////////////////////////////////////////////////////////////
// Find a sortie by it's ID.

Scoring::Sortie*
Scoring::Sortie::Lookup(UINT32 sortieID_)
{
	auto it = sorties.find(sortieID_);
	return (it != sorties.end()) ? (it->second) : nullptr;
}

//////////////////////////////////////////////////////////////////////
// End a given player's sortie.

void
Player::closeCurrentSortie()
{
	if ( m_sortie != nullptr )
	{
		dbugLog(_FLD, "p:%u c:%s v:%u detaching from sortie %p", playerID(), callsign(), m_sortie->sortieID(), m_sortie);

		if ( m_sortie->despawnTime() == 0 )
		{
			const CODE_RTB code = (CODE_RTB)missionPacket().vehicle().rtbState();
			const FULL_OBJ_ID& dest = missionPacket().vehicle().rtbFacility();
			m_sortie->setRTB(code, dest);
			m_sortie->despawned();
		}

		evaluateMissionPacket();

		saveMissionPacket();

		m_sortie = nullptr;
		m_sortieID = 0;
	}
}

//////////////////////////////////////////////////////////////////////
// Assign a new sortie to a player as they spawn.

void
Player::newSortie(UINT32 newSortieID, bool isPilot)
{
	if ( m_sortie != nullptr && m_sortie->sortieID() == newSortieID )
	{
		dbugLog(_FLA, "tried to re-allocate same sortie!");
		return;
	}

	closeCurrentSortie();

	m_sortieID = newSortieID;

	Scoring::Sortie* const existingSortie = Scoring::Sortie::Lookup(sortieID());
	if ( isPilot )
	{
		if ( existingSortie != nullptr )
			LogAbort("p:%u v:%u duplicate sortie assigned to a pilot", playerID(), sortieID());
		m_sortie = new Scoring::Sortie(*this);
		dbugLog(_FLD, "p:%u c:%s v:%u pilot assigned new sortie %p", playerID(), callsign(), sortieID(), m_sortie);
	}
	else
	{
		if ( existingSortie == nullptr )
			LogAbort("p:%u v:%u absent sortie for crew", playerID(), sortieID());
		m_sortie = existingSortie;
		existingSortie->attachCrew(*this);
		dbugLog(_FLD, "p:%u c:%s v:%u crew attached to sortie %p", playerID(), callsign(), sortieID(), m_sortie);
	}

	missionPacket().sortie().spawned_ms.set(teulClock64);
	missionPacket().sortie().despawned_ms.set(0ULL);
}


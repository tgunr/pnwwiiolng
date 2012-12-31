#ifndef PN_WWII_PLAYERLUAEVENT_H
#define PN_WWII_PLAYERLUAEVENT_H 1

struct StratHCUnit ;
struct FacilityStruct ;

//tolua_begin
namespace Host
{
	struct PlayerEvent
	{
	//tolua_end
	private:
		UINT32			m_playerID ;		// PlayerID
		UINT32			m_missionID ;		// Mission they're on
		ChatHandle		m_handle ;			// Their chat handle
		char			m_gamename[MAX_HANDLE_SIZE] ;
		StratHCUnit*	m_unit ;			// HC Unit they are in
		FacilityStruct*	m_facility ;		// Facility they are at
		WEAPON_SPEC		m_weapon ;			// Weapon they're after/using

	public:
		PlayerEvent(UINT32 _playerID, const ChatHandle& _handle, UINT32 _missionID, StratHCUnit* _unit, FacilityStruct* _facility, const WEAPON_SPEC& _weapon)
			: m_playerID(_playerID)
			, m_missionID(_missionID)
			, m_handle(_handle)
			, m_unit(_unit)
			, m_facility(_facility)
			, m_weapon(_weapon)
			{
				chatGetCallsign(m_gamename, _handle) ;
			}

	//tolua_begin
		UINT32 playerID() const { return m_playerID ; }
		UINT32 id() const { return m_playerID ; }
		UINT32 missionID() const { return m_missionID ; }
		const char* gamename() const { return m_gamename ; }
		const ChatHandle& handle() const { return m_handle ; }
		StratHCUnit* unit() const { return (StratHCUnit*)m_unit ; }
		FacilityStruct* facility() const { return (FacilityStruct*)m_facility ; }
		WEAPON_SPEC weapon() const { return m_weapon ; }
		WWII_COUNTRY country() const { return (m_unit != NULL ? m_unit->country() : WWII_NO_COUNTRY) ; }
		WWII_SIDE side() const { return (m_unit != NULL ? m_unit->side() : WWII_NO_SIDE) ; }
	} ;
}
//tolua_end

#endif // PN_WWII_PLAYERLUAEVENT_H

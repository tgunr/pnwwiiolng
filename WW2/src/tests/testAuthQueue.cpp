#include <syscfg.h> // Path is project specific.
#include "auth/authQueue.cpp"		// So we can derive from it.
#include "auth/authClientData.h"
#include "tests/testUnits.h"

// We need to simulate sending updates to players.
struct QueueInfo
{
	Auth::ClientData* cdata;
	UINT32 position;
	UINT32 waitTime;
};
typedef std::vector<QueueInfo> SendQueue;
static SendQueue sendQueue;
size_t UpdatesQueued() { return sendQueue.empty() ? 0 : sendQueue.size(); }

void
authSendPlayerInQueue(Auth::ClientData* cdata, UINT32 position, UINT32 waitTime)
{
	size_t entryNo = sendQueue.size();
	sendQueue.resize(entryNo + 1);
	QueueInfo& entry = sendQueue[entryNo];
	entry.cdata = cdata;
	entry.position = position;
	entry.waitTime = waitTime;
}

class AuthQueueUnit : public TestUnit
{
public:
	// For testing we will create several persistent cdatas
	// define the maximum number of  arenas and players-per-arena
	// you want creating.
	static const size_t _MAX_ARENAS = 3;
	static const size_t _MAX_PLAYERS = 9;
private:
	std::vector<Auth::ClientData*> m_cdatas;
public:
	Auth::ClientData* GetCData(UINT32 arenaID, const playerid_t playerNo, bool isPremium)
	{
		assert( arenaID >= 1 && arenaID <= _MAX_ARENAS );
		assert( playerNo >= 1 && playerNo <= _MAX_PLAYERS );
		Auth::ClientData* cdata = m_cdatas[ (arenaID - 1) * _MAX_PLAYERS + (playerNo - 1) ];
		cdata->isPremium = isPremium;
		return cdata;
	}

public:
	AuthQueueUnit() : TestUnit("AuthQueue"), m_cdatas()
	{
		m_cdatas.reserve(_MAX_ARENAS * _MAX_PLAYERS);
		for ( unsigned int arenaID = 1; arenaID < _MAX_ARENAS + 1; ++arenaID )
		{
			for ( unsigned int playerNo = 1; playerNo < _MAX_PLAYERS + 1; ++ playerNo )
			{
				char username[16], password[16];
				sprintf(username, "user%u-%u", arenaID, playerNo);
				sprintf(password, "pass%u-%u", arenaID, playerNo);
				Auth::ClientData* cdata = new Auth::ClientData(arenaID, 0, 0, 0, username, password);
				cdata->arenaID = arenaID;
				m_cdatas.push_back(cdata);
			}
		}
	}
	~AuthQueueUnit()
	{
		for ( std::vector<Auth::ClientData*>::iterator it = m_cdatas.begin(); it != m_cdatas.end(); ++it )
		{
			delete* it;
		}
	}

protected:
	size_t ArenasIndexed() { return NumEntriesInContainer(arenas); }
	bool ArenasEmpty() { return IsContainerEmpty(arenas); }
	template<typename _Indexer> bool ArenaIndexed(const _Indexer& index) const { return DoesContainerContain(arenas, index); }

	template<typename _Container> bool PremiumQueueEmpty(const _Container& arena) const { return IsContainerEmpty(arena.m_premium.m_queue); }
	template<typename _Container> bool NonPremiumQueueEmpty(const _Container& arena) const { return IsContainerEmpty(arena.m_nonPremium.m_queue); }
	template<typename _Container> size_t PremiumsIndexed(const _Container& arena) const { return NumEntriesInContainer(arena.m_premium.m_queue); }
	template<typename _Container> size_t NonPremiumsIndexed(const _Container& arena) const { return NumEntriesInContainer(arena.m_nonPremium.m_queue); }

	template<typename _Container> size_t PlayersIndexed(const _Container& queue) const { return NumEntriesInContainer(queue.m_queue); }
	template<typename _Container> bool QueueEmpty(const _Container& queue) const { return IsContainerEmpty(queue.m_queue); }
	size_t NumEntriesForPlayer(QueueHeader& queue, Auth::ClientData* player) const
	{
		if ( queue.m_queue.empty() ) return 0;
		size_t instances = 0;
		PlayerQueue::iterator it;
		for ( it = queue.m_queue.begin(); it != queue.m_queue.	end(); ++it )
		{
			if ( *it == player )
				++instances;
		}
		return instances;
	}
	Equality PlayerInQueue(QueueHeader& queue, Auth::ClientData* player) const { return Equals(NumEntriesForPlayer(queue, player), 1); }

	virtual void BeginStage()
	{
		arenas.clear();
		sendQueue.clear();
	}

	virtual void EndStage()
	{
		arenas.clear();
		sendQueue.clear();
	}

private: bool AddPlayer(UINT32 arenaID, const playerid_t playerNo, bool isPremium)
	{
		START_SUB_STAGE( "Add Player" )
			assert( arenaID >= 1 && arenaID <= _MAX_ARENAS );
			assert( playerNo >= 1 && playerNo <= _MAX_PLAYERS );

			Auth::ClientData* cdata = GetCData(arenaID, playerNo, isPremium);

			// Make sure he isn't already indexed.
			if ( ArenaIndexed(arenaID) )
			{
				CRITICAL( Equals(NumEntriesForPlayer(arenas[arenaID].m_premium, cdata), 1) == IsLt );
				CRITICAL( Equals(NumEntriesForPlayer(arenas[arenaID].m_nonPremium, cdata), 1) == IsLt );
			}

			// Add him
			CRITICAL( ExpectTrue( Auth::Queue::AddPlayer(arenaID, GetCData(arenaID, playerNo, isPremium), isPremium) ) );
			CRITICAL( ArenaIndexed(arenaID) );

			// Make sure he IS indexed.
			if ( isPremium )
			{
				CRITICAL( Equals(NumEntriesForPlayer(arenas[arenaID].m_premium, cdata), 1) == IsEqual );
			}
			else
			{
				CRITICAL( Equals(NumEntriesForPlayer(arenas[arenaID].m_nonPremium, cdata), 1) == IsEqual );
			}

		END_SUB_STAGE( "Add Player" )

		return true;
	}

private: bool PopulateTestData(bool premiums[_MAX_PLAYERS+1])
	{
		START_SUB_STAGE( "Populate Test Data" )

			for ( UINT32 arenaID = 1; arenaID <= _MAX_ARENAS; ++arenaID )
			{
				for ( playerid_t playerNo = 1; playerNo <= _MAX_PLAYERS; ++playerNo )
				{
					AddPlayer(arenaID, playerNo, premiums[playerNo - 1]);
				}
			}

			CRITICAL( ExpectFalse( ArenasEmpty() ) );

			ExpectEqual( UpdatesQueued(), _MAX_ARENAS * _MAX_PLAYERS );

		END_SUB_STAGE( "Populate Test Data" )

		return true;
	}

private: bool TestQueuePositions(bool premiums[_MAX_PLAYERS+1], UINT32 expectedPositions[_MAX_PLAYERS+1])
	 {
		// Everyone should be queued with a position matching where they are
		// and a default wait time.
		UINT32 queuePosition = 0;
		for ( UINT32 arenaID = 1; arenaID <= _MAX_ARENAS; ++arenaID )
		{
			CRITICAL( ExpectTrue( ArenaIndexed(arenaID) ) );
			for ( playerid_t playerNo = 1; playerNo <= _MAX_PLAYERS; ++playerNo )
			{
				CRITICAL( queuePosition < sendQueue.size() );
				UINT32 expectedPosition = expectedPositions[playerNo - 1];
				if ( expectedPosition == 0 )
					continue;
				Auth::ClientData* cdata = GetCData(arenaID, playerNo, premiums[playerNo - 1]);
				QueueInfo& info = sendQueue[queuePosition];
				CRITICAL( ExpectTrue( info.cdata == cdata ) );
				CRITICAL( ExpectTrue( info.position == expectedPosition ) );
				UINT32 expectedWaitTime = (expectedPosition == 1 ? _DEFAULT_WAIT_TIME_S - 1 : _DEFAULT_WAIT_TIME_S * expectedPosition);
				CRITICAL( ExpectTrue( info.waitTime == expectedWaitTime ) );
				++queuePosition;
			}
		}
		return true;
	}

	private: bool ExpectUpdate(UINT32 updateNo, UINT32 arenaID, const playerid_t playerNo, bool premiums[_MAX_PLAYERS+1], UINT32 expectPosition, UINT32 expectWaitTime)
	{
		assert( arenaID >= 1 && arenaID <= _MAX_ARENAS );
		assert( playerNo >= 1 && playerNo <= _MAX_PLAYERS );
		CRITICAL( ExpectTrue(UpdatesQueued() > updateNo) );
		QueueInfo& info = sendQueue[updateNo];
		CRITICAL( info.cdata == GetCData(arenaID, playerNo, premiums[playerNo - 1]) );
		ExpectEqual( info.position, expectPosition );
		ExpectEqual( info.waitTime, expectWaitTime );
		return true;
	}

public: virtual bool Run()
	{
		ResetClock();

		START_TEST_STAGE( "Preliminary Checks" )

			// Make sure arena list starts empty.
			CRITICAL( ExpectTrue( ArenasEmpty() ) );
			CRITICAL( ExpectFalse( ArenaIndexed(1) || ArenaIndexed(2) || ArenaIndexed(3) ) );

		END_TEST_STAGE( "Preliminary Checks" )

		START_TEST_STAGE( "Test _getWaitTime" )
			PlayerEntry entry(GetCData(1, 1, true));
			QueueHeader header;

			CRITICAL( ExpectEqual( _getWaitTime(entry, 1, header), _DEFAULT_WAIT_TIME_S - 1 ) );
			CRITICAL( ExpectEqual( _getWaitTime(entry, 2, header), _DEFAULT_WAIT_TIME_S * 2 ) );

			AdvanceClock_s(10);
			CRITICAL( ExpectEqual( _getWaitTime(entry, 1, header), _DEFAULT_WAIT_TIME_S - 1 ) );
			CRITICAL( ExpectEqual( _getWaitTime(entry, 2, header), _DEFAULT_WAIT_TIME_S * 2 - 10 ) );

			AdvanceClock_s(_DEFAULT_WAIT_TIME_S - 10);
			CRITICAL( ExpectEqual( _getWaitTime(entry, 1, header), _DEFAULT_WAIT_TIME_S - 1 ) );
			CRITICAL( ExpectEqual( _getWaitTime(entry, 2, header), _DEFAULT_WAIT_TIME_S - 1 ) );

			AdvanceClock_s(_DEFAULT_WAIT_TIME_S);
			CRITICAL( ExpectEqual( _getWaitTime(entry, 1, header), _DEFAULT_WAIT_TIME_S - 1 ) );
			CRITICAL( ExpectEqual( _getWaitTime(entry, 2, header), _DEFAULT_WAIT_TIME_S - 1 ) );

			ResetClock();
			header.m_waitTimes.add(_DEFAULT_WAIT_TIME_S / 2);
			CRITICAL( ExpectEqual( _getWaitTime(entry, 1, header), _DEFAULT_WAIT_TIME_S - 1 ) );
			CRITICAL( ExpectEqual( _getWaitTime(entry, 2, header), _DEFAULT_WAIT_TIME_S - 1 ) );

			// Adding the same wait time should produce the same result.
			header.m_waitTimes.add(_DEFAULT_WAIT_TIME_S / 2);
			CRITICAL( ExpectEqual( _getWaitTime(entry, 1, header), _DEFAULT_WAIT_TIME_S - 1 ) );
			CRITICAL( ExpectEqual( _getWaitTime(entry, 2, header), _DEFAULT_WAIT_TIME_S - 1 ) );
			// But the 4th position in the queue should be expecting 2*_DEFAULT_WAIT_TIME
			CRITICAL( ExpectEqual( _getWaitTime(entry, 4, header), _DEFAULT_WAIT_TIME_S * 2 ) );

			// Reset the histogram and try a full wait time;
			// this should cause the second person to have a 2 *
			header.m_waitTimes.reset();
			header.m_waitTimes.add(_DEFAULT_WAIT_TIME_S);
			CRITICAL( ExpectEqual( _getWaitTime(entry, 1, header), _DEFAULT_WAIT_TIME_S - 1 ) );
			CRITICAL( ExpectEqual( _getWaitTime(entry, 2, header), _DEFAULT_WAIT_TIME_S * 2 ) );

			// Now add 2*WAIT_TIME so that the avearge is 2*WAIT_TIME
			header.m_waitTimes.add(_DEFAULT_WAIT_TIME_S * 3);
			CRITICAL( ExpectEqual( _getWaitTime(entry, 1, header), _DEFAULT_WAIT_TIME_S * 2) );
			CRITICAL( ExpectEqual( _getWaitTime(entry, 2, header), _DEFAULT_WAIT_TIME_S * 4) );

		END_TEST_STAGE( "Test _getWaitTime" )

		// Verify that we can add a player:
		START_TEST_STAGE( "First player/premium" )

			// Scoped cdata that gets deleted.
			const UINT32 arenaID = 1;
			Auth::ClientData* cdata = GetCData(1, 1, true);

			// Test that the Add function works.
			CRITICAL( ExpectTrue(Auth::Queue::AddPlayer(arenaID, cdata, true)) );

			// Test that the Arena table was populated as expected.
			CRITICAL( ExpectFalse( ArenasEmpty() ) );
			CRITICAL( ExpectTrue( ArenaIndexed(1) ) );
			ExpectFalse( ArenaIndexed(2) || ArenaIndexed(3) );

			// Test that the user went into the premium queue
			ArenaQueues& arenaQueue = arenas[arenaID];
			CRITICAL( ExpectFalse( PremiumQueueEmpty(arenaQueue) ) );

			// Sanity check that the non-premium queue didn't get anything.
			ExpectTrue( NonPremiumQueueEmpty(arenaQueue) );

			// Test that the premium user queue is not empty.
			CRITICAL( ExpectFalse( QueueEmpty(arenaQueue.m_premium) ) );

			// Check that there is one player in the queue.
			if ( ExpectEqual( PlayersIndexed(arenaQueue.m_premium), 1 ) )
			{
				// And sanity check that our player appears once.
				ExpectEqual( NumEntriesForPlayer(arenaQueue.m_premium, cdata), 1 );
			}

			// Check that it tried to send the player their information.
			ExpectEqual( UpdatesQueued(), 1 );

			// Validate that the update looks right.
			QueueInfo& info = sendQueue.front();
			ExpectTrue( info.cdata == cdata );
			ExpectTrue( info.position == 1 );
			ExpectTrue( info.waitTime == _DEFAULT_WAIT_TIME_S - 1 );

			sendQueue.clear();
			CRITICAL( ExpectEqual( UpdatesQueued(), 0 ) );

			// Verify that re-adding fails.
			CRITICAL( ExpectTrue( Auth::Queue::AddPlayer(arenaID, cdata, false) ) );
			ExpectEqual( UpdatesQueued(), 1 );
			sendQueue.clear();

			// Verify that we can remove him.
			ExpectTrue( Auth::Queue::RemovePlayer(arenaID, cdata, true, false) );

			// Verify that this did not clear the arenas tables, and the arena is still indexed.
			CRITICAL( ExpectFalse( ArenasEmpty() ) );
			CRITICAL( ExpectTrue( ArenaIndexed(arenaID) ) );

			// Verify that the premium queue is now empty.
			CRITICAL( ExpectTrue( PremiumQueueEmpty(arenas[arenaID]) ) );

			// Sanity check that this generated no updates.
			ExpectEqual( UpdatesQueued(), 0 );

			// Verify that if we try to remove them again we get false.
			ExpectFalse( Auth::Queue::RemovePlayer(arenaID, cdata, true, false) );

		END_TEST_STAGE( "First player/premium" )

		// Verify that if we add a premium player, we can't remove him
		// from the non-premium queue, but we can remove him from the premium queue.
		START_TEST_STAGE( "Add player/premium, remove player/non-premium, remove player/premium" );
			UINT32 arenaID = 2;
			Auth::ClientData* cdata = GetCData(arenaID, 1, true);

			ExpectTrue( Auth::Queue::AddPlayer(arenaID, cdata, true) );
			ExpectFalse( Auth::Queue::RemovePlayer(arenaID, cdata, false, false) );
			ExpectTrue( Auth::Queue::RemovePlayer(arenaID, cdata, true, false) );

			CRITICAL( ExpectFalse(ArenasEmpty()) );

		END_TEST_STAGE( "Add player/premium, remove player/non-premium, remove player/premium" );

		// Test adding players to multiple queues
		START_TEST_STAGE( "Add players to multiple queues." )

			bool premiums[_MAX_PLAYERS+1] = { false, true, true, false, false, true, false, true, false, true };
			if ( !PopulateTestData(premiums) )
				return false;

		END_TEST_STAGE( "Add players to multiple queues." )

		START_TEST_STAGE( "Non-sequential removal test." )

			// Populate the data.
			bool premiums[_MAX_PLAYERS+1] = { false, false, false, true, true, true, true, true, true, true };
			if ( !PopulateTestData(premiums) )
				return false;

			// Advance the clock and verify that no updates are generated.
			AdvanceClock_s(_UPDATE_INTERVAL * 5);
			sendQueue.clear();
			Auth::Queue::ProcessQueues();
			ExpectEqual( UpdatesQueued(), 0 );

			// Remove the middle player from the second arena's non-premium queue
			// and check that he does not generate any updates (he's not head
			// of the queue).
			Auth::ClientData* cdata = GetCData(2, 2, false);
			CRITICAL( ExpectEqual( NumEntriesForPlayer(arenas[2].m_nonPremium, cdata), 1 ) );
			CRITICAL( ExpectTrue( Auth::Queue::RemovePlayer(2, cdata, false, false) ) );
			ExpectFalse( arenas[2].m_nonPremium.m_modified );
			CRITICAL( ExpectEqual( NumEntriesForPlayer(arenas[2].m_nonPremium, cdata), 0 ) );

			AdvanceClock_s(_UPDATE_INTERVAL + 1);
			sendQueue.clear();
			Auth::Queue::ProcessQueues();
			ExpectEqual( UpdatesQueued(), 0 );

			// But if we remove the head of the queue...
			cdata = GetCData(2, 1, false);
			CRITICAL( ExpectEqual( NumEntriesForPlayer(arenas[2].m_nonPremium, cdata), 1 ) );
			CRITICAL( ExpectTrue( Auth::Queue::RemovePlayer(2, cdata, false, false) ) );
			CRITICAL( ExpectEqual( NumEntriesForPlayer(arenas[2].m_nonPremium, cdata), 0 ) );
			ExpectTrue( arenas[2].m_nonPremium.m_modified );
			AdvanceClock_s(_UPDATE_INTERVAL + 1);
			sendQueue.clear();
			Auth::Queue::ProcessQueues();

			// There should be an update for the 1 player remaining...
			ExpectEqual( UpdatesQueued(), 1 );
			sendQueue.clear();

			// If we remove the last guy, there should be no updates.
			ExpectTrue( Auth::Queue::RemovePlayer(2, GetCData(2, 3, false), false, false) );
			ExpectFalse( arenas[2].m_nonPremium.m_modified );
			AdvanceClock_s(_UPDATE_INTERVAL + 1);
			sendQueue.clear();
			Auth::Queue::ProcessQueues();
			ExpectEqual( UpdatesQueued(), 0 );

		END_TEST_STAGE( "Non-sequential removal test." )

		START_TEST_STAGE( "Validate position and wait times." )

			ResetClock();

			bool premiums[] = { false, false, false, true, true, true, true, true, true, true };

			if ( !PopulateTestData(premiums) )
				return false;

			{
				UINT32 expectedPositions[] = { 1, 2, 3, 1, 2, 3, 4, 5, 6, 7 };

				if ( !TestQueuePositions(premiums, expectedPositions) )
					return false;
			}

			sendQueue.clear();

			// Advance 2 minutes.
			AdvanceClock_s(2 * 60);
			Auth::Queue::ProcessQueues();
			CRITICAL( ExpectEqual(UpdatesQueued(), 0) );

			// "Disconnect" the second guy from the first arena
			UINT32 arenaID = 1;
			ExpectTrue( Auth::Queue::RemovePlayer(arenaID, GetCData(arenaID, 2, premiums[2 - 1]), premiums[2 - 1], false) );
			ExpectEqual( UpdatesQueued(), 0 );
			AdvanceClock_s(1 * 60);					// Add a minute to make sure updates would be wanted.
			Auth::Queue::ProcessQueues();
			ExpectEqual( UpdatesQueued(), 0 );

			bool isPremium = premiums[1 - 1];
			QueueHeader& header = (isPremium ? arenas[arenaID].m_premium : arenas[arenaID].m_nonPremium);

			// Now lets "launch" the first guy from the first arena,
			// which should establish a bonafide wait time for that queue.
			UINT32 waited = teulTime() - header.m_queue[0].m_entryTime;
			CRITICAL( ExpectTrue( Auth::Queue::RemovePlayer(arenaID, GetCData(arenaID, 1, isPremium), isPremium, true) ) );
			CRITICAL( ExpectEqual( header.m_waitTimes.population(), 1 ) );

			AdvanceClock_s(10);
			Auth::Queue::ProcessQueues();

			CRITICAL( ExpectEqual( UpdatesQueued(), 1 ) );

			// This guy has been waiting nearly as long as the first guy, so his
			// wait time should be _DEFAULT_WAIT_TIME_S - 1.
			if ( !ExpectUpdate(0, arenaID, 3, premiums, 1, _DEFAULT_WAIT_TIME_S - 1) )
				return false;

			// Calculate the average of their two waits.
			waited = (waited + teulTime() - header.m_queue[0].m_entryTime) / 2;

			CRITICAL( ExpectTrue( Auth::Queue::RemovePlayer(arenaID, GetCData(arenaID, 3, isPremium), isPremium, true) ) );
			sendQueue.clear();
			CRITICAL( ExpectTrue( Auth::Queue::AddPlayer(arenaID, GetCData(arenaID, 1, isPremium), isPremium) ) );
			CRITICAL( ExpectEqual( UpdatesQueued(), 1 ) );

			// There should be two histogram entries now.
			CRITICAL( ExpectEqual( header.m_waitTimes.population(), 2 ) );
			CRITICAL( ExpectEqual( header.m_waitTimes.average(), waited ) );

			if ( !ExpectUpdate(0, arenaID, 1, premiums, 1, waited) )
				return false;

			AdvanceClock_s(_DEFAULT_WAIT_TIME_S * 1);
			CRITICAL( ExpectTrue( Auth::Queue::AddPlayer(arenaID, GetCData(arenaID, 2, isPremium), isPremium) ) );
			AdvanceClock_s(_DEFAULT_WAIT_TIME_S * 1);
			CRITICAL( ExpectTrue( Auth::Queue::AddPlayer(arenaID, GetCData(arenaID, 3, isPremium), isPremium) ) );

			// The initial updates should indicate their long wait times,
			// Adding players doesn't modify the queue so these are all initial updates.
			CRITICAL( ExpectFalse( header.m_modified ) );
			ExpectUpdate(0, arenaID, 1, premiums, 1, waited);
			ExpectUpdate(1, arenaID, 2, premiums, 2, waited * 2);
			ExpectUpdate(2, arenaID, 3, premiums, 3, waited * 3);

			// Now pretend the queue has been modified.
			header.m_modified = true;
			sendQueue.clear();
			Auth::Queue::ProcessQueues();

			// Should be 3 updates on the queue.
			CRITICAL( ExpectEqual( UpdatesQueued(), 3 ) );
			ExpectUpdate(0, arenaID, 1, premiums, 1, waited - _DEFAULT_WAIT_TIME_S * 2);
			ExpectUpdate(1, arenaID, 2, premiums, 2, waited * 2 - _DEFAULT_WAIT_TIME_S);	// He's been waiting one wait
			ExpectUpdate(2, arenaID, 3, premiums, 3, waited * 3);							// He hasn't.

			AdvanceClock_s(_DEFAULT_WAIT_TIME_S);
			header.m_modified = true;
			sendQueue.clear();
			Auth::Queue::ProcessQueues();

			// Should be 3 updates on the queue.
			CRITICAL( ExpectEqual( UpdatesQueued(), 3 ) );
			ExpectUpdate(0, arenaID, 1, premiums, 1, _DEFAULT_WAIT_TIME_S - 1);
			ExpectUpdate(1, arenaID, 2, premiums, 2, waited * 2 - _DEFAULT_WAIT_TIME_S * 2);	// He's been waiting two waits
			ExpectUpdate(2, arenaID, 3, premiums, 3, waited * 3 - _DEFAULT_WAIT_TIME_S);		// He's been waiting one

		END_TEST_STAGE( "Validate position and wait times." )

		return true;
	}

};

static AuthQueueUnit test;


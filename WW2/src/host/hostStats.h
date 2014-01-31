#pragma once

namespace Host
{
	struct Stats
	{
		enum ID : size_t
		{
			AcceptConn
		,	PersXfer
		,	VehReq
		,	JoinRx
		,	JoinTx
		,	ActivateRx
		,	ActivateTx
		,	PlayerExitRx
		,	PlayerExitTx
		,	WorldUpdateRx
		,	WorldUpdateBad
		,	WorldUpdateTx
		,	HitRx
		,	HitTx
		,	Despawned
		,	Killed
		,	Death
		,	TreeFall
		,	NetError

		,	Max
		};

		size_t m_stat[ID::Max];
		char m_description[ID::Max * (21 + 12 + 1 + 2)];

		void add(const ID id_) { ++m_stat[id_]; }

		void describe(const char* const prefix);

		Stats& operator += ( Stats& rhs )
		{
			for ( size_t i = 0; i < ID::Max; ++i )
			{
				m_stat[i] += rhs.m_stat[i];
				rhs.m_stat[i] = 0;
			}
			return rhs;
		}
	};
	extern Stats totalStats, currentStats;
}

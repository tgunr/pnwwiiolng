#pragma once

#pragma pack(push, 1)
namespace Scoring
{
	class KillNotice
	{
	public:
		KillNotice() {}
		KillNotice(const class Sortie&, const class Sortie&, const SINT32 deadX_, const SINT32 deadY_);

		class Party
		{
		public:
			Party() {}
			Party(const class Sortie&);

		public:
			ChatHandle		m_handle;
			sortieid_t		m_sortie;
			WEAPON_SPEC		m_spec;
			UCHAR			m_team;
		};

		Party		m_killer;
		Party		m_victim;
		SINT32		m_x, m_y;
	};
}
#pragma pack(pop)

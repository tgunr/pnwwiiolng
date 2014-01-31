#pragma once
// Created: Tue Apr  5 11:35:50 CDT 2005
//
// Describes the LatLong class with to-text ability
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <math.h>

#define WORLD_ARC_WIDTH			180.0
#define WORLD_ARC_HEIGHT		70.1875

#define WORLD_WIDTH				8640000
#define WORLD_HEIGHT			4211200

class LatLong
{
public:
	LatLong(const double& x, const double& y)
	{
		_translate(x / WORLD_WIDTH * WORLD_ARC_WIDTH, m_east, m_longdeg, m_longmin, m_longsec);
		_translate(y / WORLD_HEIGHT * WORLD_ARC_HEIGHT, m_north, m_latdeg, m_latmin, m_latsec);

		snprintf(m_text, 40, "%c%d.%d.%d %c%d.%d.%d"
							, ((m_north) ? 'N':'S')
							, m_latdeg, m_latmin, m_latsec
							, ((m_east) ? 'E':'W')
							, m_longdeg, m_longmin, m_longsec
							);
	}

	operator const char* (void) { return m_text; }

public:
	int		m_longdeg, m_longmin, m_longsec;
	bool	m_east;
	int		m_latdeg, m_latmin, m_latsec;
	bool	m_north;
	char	m_text[40+1];

private:
	static inline void
	_translate(double coord, bool& sign, int& deg, int& min, int& sec)
	{
		if ( coord >= 0 )
			sign = true;
		else
		{
			coord = fabs(coord);
			sign = false;
		}
		coord /= 100;

		deg = (int)floor(coord);
		coord = modf(coord, &coord) * 60.0;
		min = (int)floor(coord);
		coord = modf(coord, &coord) * 60.0;
		sec = (int)floor(coord);
	}
};

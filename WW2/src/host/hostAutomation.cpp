#include <syscfg.h> // Path is project specific.
// Created: Thu Jun 22 12:40:21 CDT 2006
//
// Functions for running automated/fake vehicles
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatHandle.h"
#include "host/host.h"
#include "host/hostVehicle.h"
#include "weap/weapList.h"

class Fake
{
public:
	Fake() : hv(NULL), sx(0), sy(0), dist(0)
		   , cx(0), cy(0), cz(0), radius(0), speed(0), started(0)
		   , x(0), y(0), z(0), pitch(0), roll(0), yaw(0)
		   , vx(0), vy(0), vz(0), ax(0), ay(0), az(0)
		   , wx(0), wy(0), wz(0)
			{}

	void reset();
	void move();
	void moveAircraft();
	void moveOther();
	void update();

public:
	HostVehicle*	hv;
	FLOAT64			sx, sy;		// Deltas (speeds)
	FLOAT64			dist;			// Distance controller

	FLOAT64			cx, cy, cz;
	FLOAT64 radius;
	FLOAT64 speed;
	TeulTimeMS started;	// teulClock64 walltime in ms

	FLOAT64	x, y, z;
	FLOAT64	pitch, roll, yaw;
	FLOAT64	vx, vy, vz;
	FLOAT64 ax, ay, az;
	FLOAT64	wx, wy, wz;
};

typedef std::vector<Fake> FAKES;
static FAKES _fakes;

// Eersel
FLOAT64 g_fakeBaseX = 254900.94717361819;
FLOAT64 g_fakeBaseY = 3081051.2831075168;
FLOAT64 g_fakeBaseZ = 29.0;

UINT32 g_fakeRange = 500;
UINT32 g_fakeSpeed = 50;
FLOAT64 g_fakeDensity = 12.0;
FLOAT64 g_fakeAirZ = 600.0;
bool g_fakeMovement = true;
bool g_enableFunnyFarm = false;
static const UINT32 g_waves = 2;
UINT32 g_modify = 0;

void
Fake::reset()
{
	UINT32 unitNo = hv->vehicleData.squad;

	cx = g_fakeBaseX + (g_fakeDensity * (FLOAT64)(unitNo%16)) - 8 * g_fakeDensity;
	cy = g_fakeBaseY + (g_fakeDensity * (FLOAT64)(unitNo/16)) - 8 * g_fakeDensity;

	cz = g_fakeBaseZ;

	radius = 5 + (rand() % (g_fakeRange*100))/100;
	speed = 1 + (rand() % (g_fakeSpeed * 10)) / 10;

	// Bump aircraft altitude a little
	if ( hv->spec().wCategory == _AIRCRAFT )
	{
		cz += (g_fakeAirZ / 2) + (rand() % (UINT32)g_fakeAirZ*3);
		radius = g_fakeRange + (rand() % (g_fakeRange*1000))/100;
		speed = 75 + speed + (rand() % g_fakeSpeed);
	}
	else if ( hv->spec().wCategory == _INFANTRY )
		speed /= 4;
	else if ( hv->spec().wCategory == _SEA_VEHICLE )
		radius = 250 + 2*radius + (rand() % g_fakeRange);
	else
	{
		radius = (radius + 10) * 2;
		speed = speed + (rand() % g_fakeSpeed);
	}

	speed /= 1000;

	x = cx;
	y = cy;
	z = cz;

	dist = 0;

	sx = cx + radius * cos(0);
	sy = cy + radius * sin(0);

	hv->setReallyActive(true);
	hv->m_spawnTime = 0;

	started = (TeulTimeMS) rand();

	hv->updateGrid();
}

//////////////////////////////////////////////////////////////////////
// Reset the fake host vehicle positions

void
hostResetFakePositions()
{
	UINT32 i;
	FAKES::iterator it;
	for ( i = 0, it = _fakes.begin(); it != _fakes.end(); ++it, ++i )
	{
		it->reset();
	}
	dbugLog(_FLI, "reset %u vehicle positions to base %f.%f.%f", i, g_fakeBaseX, g_fakeBaseY, g_fakeBaseZ);
}

//////////////////////////////////////////////////////////////////////
// Create and initialize all of our fake weapons

static void
_initializeFakeWeapons()
{
	if ( g_enableFunnyFarm == false )
		return;

	static const UINT32 _WEAPONS = g_weaponList.size();
	UINT32 serial = 0x3fffffff;
	Fake entry;

	WEAPON_LIST::iterator it = g_weaponList.begin();
	for ( UINT32 wave = 0; wave < g_waves; ++wave )
	{
		for ( UINT32 i = 0; i < _WEAPONS; ++i )
		{
			if ( it == g_weaponList.end() )
				it = g_weaponList.begin();
			WEAPON* weap = it->second;
			++it;
			HostVehicle* fake = hostCreateRemoteVehicle(Instance::_CAMPAIGN, serial, weap->spec());
			assert(fake);
			SHARED_VEHICLE_INFO& vi = fake->vehicleData;
			sprintf(vi.callsign, "FAKE%04zu", _fakes.size());
			vi.packedCallsign = chatGetHandle(vi.callsign);
			vi.squad = i;
			vi.rank = (rand() % 22) + 1;
			hostActivateVehicle(fake, false);
			entry.hv = fake;
			entry.reset();
			_fakes.push_back(entry);

			--serial;
		}
	}

	dbugLog(_FLI, "Created %" PRSizeT " fake vehicles", _fakes.size());
}

//////////////////////////////////////////////////////////////////////
// Update all of the fake vehicles

void
hostRunFakeVehicles()
{
	static TeulTimeMS nextRun = teulClock64;
	if ( teulClock64 < nextRun )
		return;

	// Only do this once every 150ms
	if ( nextRun + 200 < teulClock64 )
		nextRun = teulClock64;
	nextRun = nextRun + 125 + (rand() & 63);

	if ( _fakes.empty() )
		_initializeFakeWeapons();

	FAKES::iterator it;
	for ( it = _fakes.begin(); it != _fakes.end(); ++it )
	{
		(*it).update();
	}
}

//////////////////////////////////////////////////////////////////////
// Update a vehicle's attributes based on the vehicle data we're
// maintaining for it

void
Fake::moveAircraft()
{
	TeulTimeMS t = teulClock64 - started;
	FLOAT64 w = speed / radius;

	yaw = (w * t);

	x = cx + radius * cos(yaw);
	y = cy + radius * sin(yaw);

	if ( cx == sx && cy == sy )
	{
		started = teulClock64;
		yaw = 0;
	}

	vx = -speed * 100 * sin(yaw);
	vy = speed * 100 * cos(yaw);

	SHARED_VEHICLE_INFO& vi = hv->vehicleData;

//	if ( vi.squad == 1 )
//	{
//		dbugLog(_FLI, "t=%llu, w=%g, yaw=%g, x=%g, y=%g, vx=%g, vy=%g", t, w, yaw, x, y, vx, vy);
//	}

	// Convert to radians
//	FLOAT64 yawR = yaw * PI / 180;

	vi.attitude.attZ = (SINT16)(cos(yaw) * 10000.0 + 0.5);

	vi.velocity.velX = (SINT16)(vx * 50.0 + 0.5);
	vi.velocity.velY = (SINT16)(vy * 50.0 + 0.5);
}

void
Fake::move()
{
	if ( !g_fakeMovement )
		return;

	moveAircraft();

//	switch ( hv->spec().wCategory )
//	{
//	case _AIRCRAFT:
//		moveAircraft();
//	break;
//	default:
//		moveOther();
//	}
}

void
Fake::update()
{
	move();

	SHARED_VEHICLE_INFO& vi = hv->vehicleData;
	hv->lastRX = teulClock64;
	vi.hostPosTimeStamp = teulClock64 + 1;
	vi.timeStamp = teulClock();
	vi.posTimeStamp = teulClock();
	hv->m_holder->timeStamp = teulClock();
	hv->m_holder->posTimeStamp = teulClock();

	vi.position.m_x = (int)(x * 100.0);
	vi.position.m_y = (int)(y * 100.0);
	vi.position.m_z = (int)((z + 2000.0) * 100.0);

/*
	vi.high.attitude.attX = (SINT16)(pitch * 10000.0 + 0.5);
	vi.high.attitude.attY = (SINT16)(roll * 10000.0 + 0.5);
	vi.high.attitude.attZ = (SINT16)(yaw * 10000.0 + 0.5);
	vi.low.attitude.attX = (SINT8)(pitch * 40.0 + 0.5);
	vi.low.attitude.attY = (SINT8)(roll * 40.0 + 0.5);
	vi.low.attitude.attZ = yaw;(SINT8)(yaw * 40.0 + 0.5);

	vi.high.velocity.velX = (SINT16)(vx * 50.0 + 0.5);
	vi.high.velocity.velY = (SINT16)(vy * 50.0 + 0.5);
	vi.high.velocity.velZ = (SINT16)(vz * 50.0 + 0.5);
	vi.low.velocity.velX = (SINT8)(log10(fabs(vx)*0.5 + 1.0)*50.0 + 0.5);
	vi.low.velocity.velY = (SINT8)(log10(fabs(vy)*0.5 + 1.0)*50.0 + 0.5);
	vi.low.velocity.velZ = (SINT8)(log10(fabs(vz)*0.5 + 1.0)*50.0 + 0.5);

	if (vx < 0.0)	vi.low.velocity.velX = -vi.low.velocity.velX;
	if (vy < 0.0)	vi.low.velocity.velY = -vi.low.velocity.velY;
	if (vz < 0.0)	vi.low.velocity.velZ = -vi.low.velocity.velZ;

	vi.high.acceleration.accelX = (SINT16)(ax * 100.0 + 0.5);
	vi.high.acceleration.accelY = (SINT16)(ay * 100.0 + 0.5);
	vi.high.acceleration.accelZ = (SINT16)(az * 100.0 + 0.5);
	vi.low.acceleration.accelX = (SINT8)(log10(fabs(ax) + 1.0)*50.0 + 0.5);
	vi.low.acceleration.accelY = (SINT8)(log10(fabs(ay) + 1.0)*50.0 + 0.5);
	vi.low.acceleration.accelZ = (SINT8)(log10(fabs(az) + 1.0)*50.0 + 0.5);

	if (ax < 0.0)	vi.low.acceleration.accelX = -vi.low.acceleration.accelX;
	if (ay < 0.0)	vi.low.acceleration.accelY = -vi.low.acceleration.accelY;
	if (az < 0.0)	vi.low.acceleration.accelZ = -vi.low.acceleration.accelZ;

	vi.high.rotational.velX = (SINT16)(wx * 1000.0 + 0.5);
	vi.high.rotational.velY = (SINT16)(wy * 1000.0 + 0.5);
	vi.high.rotational.velZ = (SINT16)(wz * 1000.0 + 0.5);
	vi.low.rotational.velX = (SINT8)(log10(fabs(wx) * 10.0 + 1.0)*50.0 + 0.5);
	vi.low.rotational.velY = (SINT8)(log10(fabs(wy) * 10.0 + 1.0)*50.0 + 0.5);
	vi.low.rotational.velZ = (SINT8)(log10(fabs(wz) * 10.0 + 1.0)*50.0 + 0.5);

	if (wx < 0.0)	vi.low.rotational.velX = -vi.low.rotational.velX;
	if (wy < 0.0)	vi.low.rotational.velY = -vi.low.rotational.velY;
	if (wz < 0.0)	vi.low.rotational.velZ = -vi.low.rotational.velZ;
*/

	hv->updateGrid();
}

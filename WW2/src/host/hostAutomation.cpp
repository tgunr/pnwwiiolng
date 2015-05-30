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
#include <future>

class Fake
{
public:
	Fake() : m_hv(nullptr), m_sx(0), m_sy(0), m_dist(0)
		   , m_cx(0), m_cy(0), m_cz(0), m_radius(0), m_speed(0), m_started(0)
		   , m_x(0), m_y(0), m_z(0), m_pitch(0), m_roll(0), m_yaw(0)
		   , m_vx(0), m_vy(0), m_vz(0), m_ax(0), m_ay(0), m_az(0)
		   , m_wx(0), m_wy(0), m_wz(0)
			{}

	void reset();
protected:
	void moveAircraft();
	void moveOther();
public:
	void update();

public:
	HostVehicle*	m_hv;
	FLOAT64			m_sx, m_sy;		// Deltas (speeds)
	FLOAT64			m_dist;			// Distance controller

	FLOAT64			m_cx, m_cy, m_cz;
	FLOAT64			m_radius;
	FLOAT64			m_speed;
	TeulTimeMS		m_started;	// teulClock64 walltime in ms

	FLOAT64			m_x, m_y, m_z;
	FLOAT64			m_pitch, m_roll, m_yaw;
	FLOAT64			m_vx, m_vy, m_vz;
	FLOAT64			m_ax, m_ay, m_az;
	FLOAT64			m_wx, m_wy, m_wz;
};

using FAKES = std::vector<Fake>;
static FAKES s_fakes;

// Eersel
FLOAT64 g_fakeBaseX = 254885;
FLOAT64 g_fakeBaseY = 3080999;
FLOAT64 g_fakeBaseZ = 29;

UINT32 g_fakeRange = 500;
UINT32 g_fakeSpeed = 50;
FLOAT64 g_fakeDensity = 12.0;
FLOAT64 g_fakeAirZ = 600.0;
bool g_fakeMovement = true;
#ifdef BETA_RELEASE
bool g_enableFunnyFarm = true;
#else
bool g_enableFunnyFarm = false;
#endif
static const UINT32 g_waves = 2;
UINT32 g_modify = 0;

void
Fake::reset()
{
	UINT32 unitNo = m_hv->vehicleData.squad;

	m_cx = g_fakeBaseX + (g_fakeDensity * (FLOAT64)(unitNo%16)) - 8 * g_fakeDensity;
	m_cy = g_fakeBaseY + (g_fakeDensity * (FLOAT64)(unitNo/16)) - 8 * g_fakeDensity;

	m_cz = g_fakeBaseZ;

	m_radius = 5 + (rand() % (g_fakeRange*100))/100;
	m_speed = 1 + (rand() % (g_fakeSpeed * 10)) / 10;

	// Bump aircraft altitude a little
	if ( m_hv->spec().wCategory == _AIRCRAFT )
	{
		m_cz += (g_fakeAirZ / 2) + (rand() % (UINT32)g_fakeAirZ*3);
		m_radius = g_fakeRange + (rand() % (g_fakeRange*1000))/100;
		m_speed = 75 + m_speed + (rand() % g_fakeSpeed);
	}
	else if ( m_hv->spec().wCategory == _INFANTRY )
		m_speed /= 4;
	else if ( m_hv->spec().wCategory == _SEA_VEHICLE )
		m_radius = 250 + 2*m_radius + (rand() % g_fakeRange);
	else
	{
		m_radius = (m_radius + 10) * 2;
		m_speed = m_speed + (rand() % g_fakeSpeed);
	}

	m_speed /= 1000;

	m_x = m_cx;
	m_y = m_cy;
	m_z = m_cz;

	m_dist = 0;

	m_sx = m_cx + m_radius * cos(0);
	m_sy = m_cy + m_radius * sin(0);

	m_hv->setReallyActive(true);
	m_hv->m_spawnTime = 0;

	m_started = (TeulTimeMS) rand();

	m_hv->updateGrid();
}

//////////////////////////////////////////////////////////////////////
// Reset the fake host vehicle positions

void
hostResetFakePositions()
{
	UINT32 i = 0;
	for ( auto it = s_fakes.begin(), end = s_fakes.end(); it != end; ++it, ++i )
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

	const size_t numWeapons = g_weaponList.size();
	const size_t numFakes = numWeapons * g_waves;
	s_fakes.reserve(numFakes);

	UINT32 serial = 0x3fffffff;
	Fake entry;

	WEAPON_LIST::const_iterator it = g_weaponList.cbegin();
	for (size_t fakeNo = 0; fakeNo < numFakes; ++fakeNo, ++it)
	{
		if ( it == g_weaponList.cend() )
			it = g_weaponList.cbegin();
		WEAPON* weap = it->second;
		HostVehicle* fake = hostCreateRemoteVehicle(Instance::g_campaign, serial, weap->spec());
		assert(fake);
		SHARED_VEHICLE_INFO& vi = fake->vehicleData;
		sprintf(vi.callsign, "FAKE%04zu", s_fakes.size());
		vi.packedCallsign = chatGetHandle(vi.callsign);
		vi.squad = fakeNo % numWeapons;
		vi.rank = (fakeNo % 22) + 1;
		hostActivateVehicle(fake, false);
		entry.m_hv = fake;
		entry.reset();
		s_fakes.push_back(entry);

		--serial;
	}

	dbugLog(_FLI, "Created %" PRSizeT " fake vehicles", s_fakes.size());
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

	if ( s_fakes.empty() )
		_initializeFakeWeapons();

	if (g_fakeMovement)
	{
		std::vector<std::future<Fake*>> updates;
		updates.reserve(s_fakes.size());
		for (auto& fake : s_fakes) {
			updates.emplace_back(std::async([](Fake& fake) -> Fake* {
				fake.update();
				return (fake.m_hv->inCorrectGrid()) ? nullptr : &fake;
			}, fake));
		}
		for (auto& result : updates) {
			Fake* fake = result.get();
			if (fake)
				fake->m_hv->updateGrid();
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Update a vehicle's attributes based on the vehicle data we're
// maintaining for it

void
Fake::moveAircraft()
{
	TeulTimeMS t = teulClock64 - m_started;
	FLOAT64 w = m_speed / m_radius;

	m_yaw = (w * t);

	m_x = m_cx + m_radius * cos(m_yaw);
	m_y = m_cy + m_radius * sin(m_yaw);

	if ( m_cx == m_sx && m_cy == m_sy )
	{
		m_started = teulClock64;
		m_yaw = 0;
	}

	m_vx = -m_speed * 100 * sin(m_yaw);
	m_vy = m_speed * 100 * cos(m_yaw);

	SHARED_VEHICLE_INFO& vi = m_hv->vehicleData;

//	if ( vi.squad == 1 )
//	{
//		dbugLog(_FLI, "t=%llu, w=%g, yaw=%g, x=%g, y=%g, vx=%g, vy=%g", t, w, yaw, x, y, vx, vy);
//	}

	// Convert to radians
//	FLOAT64 yawR = yaw * PI / 180;

	vi.attitude.attZ = (SINT16)(cos(m_yaw) * 10000.0 + 0.5);

	vi.velocity.velX = (SINT16)(m_vx * 50.0 + 0.5);
	vi.velocity.velY = (SINT16)(m_vy * 50.0 + 0.5);
}

void
Fake::moveOther()
{
	// For now: just duplicate moveAircraft
	TeulTimeMS t = teulClock64 - m_started;
	FLOAT64 w = m_speed / m_radius;

	m_yaw = (w * t);

	m_x = m_cx + m_radius * cos(m_yaw);
	m_y = m_cy + m_radius * sin(m_yaw);

	if (m_cx == m_sx && m_cy == m_sy)
	{
		m_started = teulClock64;
		m_yaw = 0;
	}

	m_vx = -m_speed * 100 * sin(m_yaw);
	m_vy = m_speed * 100 * cos(m_yaw);

	SHARED_VEHICLE_INFO& vi = m_hv->vehicleData;

	vi.attitude.attZ = (SINT16)(cos(m_yaw) * 10000.0 + 0.5);

	vi.velocity.velX = (SINT16)(m_vx * 50.0 + 0.5);
	vi.velocity.velY = (SINT16)(m_vy * 50.0 + 0.5);
}

void
Fake::update()
{
	switch (m_hv->spec().wCategory)
	{
	case _AIRCRAFT:
		moveAircraft();
		break;
	default:
		moveOther();
	}

	SHARED_VEHICLE_INFO& vi = m_hv->vehicleData;
	m_hv->lastRX = teulClock64;
	vi.hostPosTimeStamp = teulClock64 + 1;
	vi.timeStamp = teulClock();
	vi.posTimeStamp = teulClock();
	m_hv->m_holder->timeStamp = teulClock();
	m_hv->m_holder->posTimeStamp = teulClock();

	vi.position.m_x = (int)(m_x * 100.0);
	vi.position.m_y = (int)(m_y * 100.0);
	vi.position.m_z = (int)((m_z + 2000.0) * 100.0);
}

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

#include <syscfg.h> // Path is project specific.
#include "updt/updtTypes.h"
#include "persPlayer.h"

/************************************************************************************************
	persPlayer
*************************************************************************************************/

persPlayer::persPlayer() : _client(0), _lastupdate(0), _vehicle(0)
{
}

void persPlayer::Update(int t)
{
	if(_vehicle && _client && t - _lastupdate >= FE_UPDATE_INTERVAL)
	{
		_client->SendUpdate();
		_lastupdate = t;
	}
}

void persPlayer::SetVehicle(void*)
{
}

#include <syscfg.h> // Path is project specific.
#include <map>
#include <math.h>

#include "fio/fio.h"
#include "strt/strtTypes.h"
#include "mm/mmSpawnTypes.h"
#include "client/cmdLineClient.h"
#include "ww2client_help.h"
#include "client/clientNetError.h"
#include "veh/vehType.h"
#include "auth/authBypassOTP.h"

/************************************************************************************************
	cmdLineClient
*************************************************************************************************/

#define DEFAULT_PLAYERID_START 	84649
#define DEFAULT_HOST			"192.168.0.83"
#define DEG_TO_RAD(x) ((x) * M_PI / 180.0)

typedef int (*interp_cmd)(int argc, char* argv[]);
struct interp_cmd_lookup
{
	const char* name;
	interp_cmd cmd;
	const char* help;
} ;

static int spawn_cmd	(int argc, char* argv[]);
static int despawn_cmd	(int argc, char* argv[]);
static int move_cmd		(int argc, char* argv[]);
static int rotate_cmd	(int argc, char* argv[]);
static int velocity_cmd	(int argc, char* argv[]);
static int speed_cmd	(int argc, char* argv[]);
static int circle_cmd	(int argc, char* argv[]);
static int script_cmd	(int argc, char* argv[]);
static int formation_cmd(int argc, char* argv[]);
static int turret_cmd	(int argc, char* argv[]);
static int fire_cmd		(int argc, char* argv[]);
static int state_cmd	(int argc, char* argv[]);
static int visbit_cmd	(int argc, char* argv[]);
static int exit_cmd		(int argc, char* argv[]);
static int weaplist_cmd	(int argc, char* argv[]);
static int help_cmd		(int argc, char* argv[]);
static int fps_cmd		(int argc, char* argv[]);
static int visdam_cmd	(int argc, char* argv[]);
static int data_cmd		(int argc, char* argv[]);
static int ride_cmd		(int argc, char* argv[]);
static int resupply_cmd	(int argc, char* argv[]);
static int host_cmd		(int argc, char* argv[]);
static int playerid_cmd	(int argc, char* argv[]);

static interp_cmd_lookup INTERP_CMDS[] =
{
	{"spawn",		spawn_cmd,		SPAWN_HELP}, 		// spawn client#(s) vehname facname
	{"despawn", 	despawn_cmd,	DESPAWN_HELP},		// despawn client#(s)
	{"move",		move_cmd,		MOVE_HELP},			// move client#(s) [rel, abs] x y z
	{"rotate",		rotate_cmd,		ROTATE_HELP},		// rotate client#(s) [rel, abs] pitch roll yaw
	{"velocity",  	velocity_cmd,	VEL_HELP},			// velocity client#(s) [rel, abs] x y z
	{"speed",		speed_cmd,		SPEED_HELP},		// speed client#(s) speed
	{"circle",  	circle_cmd,		CIRCLE_HELP},		// circle client#(s) [rel, abs] x y z radius speed
	{"script",		script_cmd,		SCRIPT_HELP},		// script filename
	{"formation",	formation_cmd,	FORM_HELP},		// formation client#(s) [rank, file, wedge, circle, grid] (radius/spacing) (orientation) (cols)
	{"turret",		turret_cmd,		TURRET_HELP},		// turret client#(s) # elevation traverse
	{"fire",		fire_cmd,		FIRE_HELP},			// fire client#(s) mount
	{"state",		state_cmd,		STATE_HELP},		// state client#(s) statename on/off
	{"visbit",		visbit_cmd,		VISBIT_HELP},		// visbit client#(s) visname on/off
	{"exit", 		exit_cmd,		EXIT_HELP},			// exit
	{"weaplist", 	weaplist_cmd,	WEAPLIST_HELP},		// weaplist facname
	{"help", 		help_cmd,		HELP_HELP},			// help cmdname
	{"fps", 		fps_cmd,		FPS_HELP},			// fps client#(s) type state
	{"visdam", 		visdam_cmd,		VIS_HELP},			// fps client#(s) type state
	{"data", 		data_cmd,		""},				// data client#(s) (pos, vel, att)
	{"ride", 		ride_cmd,		""},				// ride client#(s) vehname seatnumber
	{"resupply",	resupply_cmd,   ""},				// resupply client#(s) vehname
	{"host",		host_cmd,		""},				// set host
	{"playerid",	playerid_cmd,   ""},				// set client start id
	{NULL, 	NULL}
};

static const char* STATE_NAMES[] =
{
	"all",
	"gear",
	"flaps",
	"prop",
	"canopy",
	"hatch",
	"exhaust",
	"oilleak",
	"fuelleak",
	"glycolleak",
	"enginefire",
	"fuelfire",
	"wingtipvortex",
	"enginesound",
	"bombbaydoor",
	"tankfire",
	"stukasiren",
	"divebrake",
	"crashexplosion",
	"crashejectriders",
	"treaddust",
	NULL
};

static char host[32] = DEFAULT_HOST;
static int playeridStart = DEFAULT_PLAYERID_START;
float _startx, _starty, _startz;

static int client_spawn(int clientIndex, WEAPON_SPEC const&);

static int  interp_parse_args(char* s, char* argv[], int maxargs);
static void interp_range(char* s, int* start, int* end);
static interp_cmd lookup_interp_cmd(const char* cmdName);

class CmdLineClient : public ww2::Client
{
public:
	CmdLineClient();

	void MoveTo(float,float,float);
	void MoveBy(float,float,float);
	void RotateTo(float, float, float);
	void RotateToRadians(float, float, float);
	void RotateBy(float, float, float);
	void SetSpeed(float);
	void AddSpeed(float);
	void SetVel(float,float,float);
	void AddVel(float,float,float);
	void GetDirection(float&,float&,float&);
	void GetPosition(float&,float&,float&);

	virtual void Update();

protected:
	float _x,_y,_z;
	float _vx, _vy, _vz;
	float _pitch, _roll, _yaw;
	unsigned int _lastupdate;
};

CmdLineClient* get_client(int i);

typedef std::map<int, CmdLineClient*> ClientMap;
static ClientMap clients;

/************************************************************************************************
	client_update
*************************************************************************************************/

void cmdLineClientUpdate()
{
	ww2::Client::NetUpdate();

	static unsigned int lasttime;
	unsigned int time = ww2::Client::NetTime();

	if(time - lasttime > 250)
	{
		lasttime = time;

		ClientMap::iterator it = clients.begin();
		for(; it != clients.end(); ++it)
		{
			it->second->Update();
			it->second->SendUpdate();
		}
	}
}

/************************************************************************************************
	client_spawn
*************************************************************************************************/

static int client_spawn(int clientIndex, WEAPON_SPEC const& spec)
{
	CmdLineClient* client = 0;

	ClientMap::iterator it = clients.find(clientIndex);
	if(it == clients.end())
	{
		client = new CmdLineClient;
		int err = -1;
		try
		{
			err = client->Login(host, playeridStart+clientIndex, AUTH_BYPASS_OTP);
		}catch(ClientNetError& err)
		{
			printf("%s\n", err.what());
		}

		if(err != 0)
		{
			printf("%d: login failed\n", clientIndex);
			delete client;
			return -1;
		}else
		{
			printf("%d: logged in\n", clientIndex);
		}

		clients[clientIndex] = client;
	}else
	{
		client = it->second;
	}

	if(!client) return -1;

	if(client->CellClientActive())
	{
		printf("%d: already spawned\n", clientIndex);
		return 0;
	}

	client->RequestDirectSpawn(spec);
	client->ClearWaits();
	client->SetWait(ww2::Client::SPAWN_NOTIFY_WAIT);
	client->SetWait(ww2::Client::CELL_CLIENT_WAIT);
	client->SetWait(ww2::Client::REDIRECT_WAIT);
	if(client->WaitForMessages(3) != 0) return -1;

	client->RequestVehicleJoin(client->SpawnNotify().weaponSerial,0);
	if(client->WaitForMessage(ww2::Client::VEHICLE_REPLY_WAIT, 3) != 0) return -1;

	client->MoveTo(_startx + clientIndex*10, _starty, _startz);
	printf("%d: spawned\n", clientIndex);

	return 0;
}


/************************************************************************************************
	spawn_cmd
*************************************************************************************************/

static int spawn_cmd(int argc, char* argv[])
{
	char* facname, *weapname;
	FULL_OBJ_ID fac;
	WEAPON_SPEC spec;
	int vehIdx, i;
	int start, end;

	if(argc != 3) return -1;

	interp_range(argv[1], &start, &end);

	weapname = argv[2];

	spec = weapFindWeaponByName(weapname);
	if(weapSpecIsNULL(spec))
	{
		printf("weapon %s not found\n", weapname);
		return -1;
	}

	for(i = start; i <= end; i++)
	{
		if(client_spawn(i, spec) != 0)
			printf("%d: spawn failed\n", i);

		ww2::Client::NetUpdate(100);
		usleep(50);
	}

	return 0;
}

/************************************************************************************************
	despawn_cmd
*************************************************************************************************/

static int despawn_cmd(int argc, char* argv[])
{
	int i, start, end;

	if(argc != 2) return -1;

	interp_range(argv[1], &start, &end);

	for(i = start; i <= end; i++)
	{
		CmdLineClient* client = get_client(i);
		if(!client) continue;

		if(client->CellClientActive() && client->Connection() > -1)
		{
			client->SendPlayerExit(PEXIT_RTB, nullObjID);
//			client->WaitForMessage(ww2::Client::MAP_CLIENT_WAIT,1);
			ww2::Client::NetUpdate(100);
			usleep(50);
		}
	}

	return 0;
}

/************************************************************************************************
	move_cmd
*************************************************************************************************/

static int move_cmd(int argc, char* argv[])
{
	int i, start, end, abs;

	if(argc != 6) return -1;

	interp_range(argv[1], &start, &end);
	abs = !strcmp(argv[2], "abs");

	for(i = start; i <= end; i++)
	{
		CmdLineClient* client = get_client(i);
		if(!client) continue;

		if(abs)
			client->MoveTo(atof(argv[3]), atof(argv[4]), atof(argv[5]));
		else
			client->MoveBy(atof(argv[3]), atof(argv[4]), atof(argv[5]));
	}
	return 0;
}

/************************************************************************************************
	rotate_cmd
*************************************************************************************************/

static int rotate_cmd(int argc, char* argv[])
{
	int i, start, end, abs;

	if(argc != 6) return -1;

	interp_range(argv[1], &start, &end);
	abs = !strcmp(argv[2], "abs");

	for(i = start; i <= end; i++)
	{
		CmdLineClient* client = get_client(i);
		if(!client) continue;

		if(abs)
			client->RotateTo(atof(argv[3]), atof(argv[4]), atof(argv[5]));
		else
			client->RotateBy(atof(argv[3]), atof(argv[4]), atof(argv[5]));
	}

	return 0;
}

/************************************************************************************************
	velocity_cmd
*************************************************************************************************/

static int velocity_cmd(int argc, char* argv[])
{
	int i, start, end, abs;

	if(argc != 6) return -1;

	interp_range(argv[1], &start, &end);
	abs = !strcmp(argv[2], "abs");

	for(i = start; i <= end; i++)
	{
		CmdLineClient* client = get_client(i);
		if(!client) continue;

		if(abs)
			client->SetVel(atof(argv[3]), atof(argv[4]), atof(argv[5]));
		else
			client->AddVel(atof(argv[3]), atof(argv[4]), atof(argv[5]));
	}

	return 0;
}

/************************************************************************************************
	speed_cmd
*************************************************************************************************/

static int speed_cmd(int argc, char* argv[])
{
	int i, start, end, abs;
	float speed;
	float x,y,z;
	float xp, yp, zp;

	if(argc != 4) return -1;

	interp_range(argv[1], &start, &end);
	abs = !strcmp(argv[2], "abs");
	speed = atof(argv[3]);

	for(i = start; i <= end; i++)
	{
		CmdLineClient* client = get_client(i);
		if(!client) continue;
		if(abs)
			client->SetSpeed(speed);
		else
			client->AddSpeed(speed);
	}

	return 0;
}

/************************************************************************************************
	circle_cmd
*************************************************************************************************/

static int circle_cmd(int argc, char* argv[])
{
	/*int i, start, end, abs;
	double* cookie;
	double centerX, centerY, centerZ, initRadius, initSpeed;
	double x, y, z, radius, startAngle;

	if(argc >= 2)
		interp_range(argv[1], &start, &end);

	if(argc == 2) // clear
	{
		for(i = start; i <= end; i++)
			client_add_controller(&clientModelData[i], NULL, NULL);
		return 0;
	}

	if(argc != 8) return -1;

	abs = !strcmp(argv[2], "abs");
	centerX = atof(argv[3]);
	centerY = atof(argv[4]);
	centerZ = atof(argv[5]);
	initRadius = atof(argv[6]);
	initSpeed = atof(argv[7]);

	if(!abs)
	{
		centerX += clientModelData[start].x;
		centerY += clientModelData[start].y;
		centerZ += clientModelData[start].z;
	}

	for(i = start; i <= end; i++)
	{
		x = centerX + initRadius + (_x - clientModelData[start].x);
		y = centerY + (_y - clientModelData[start].y);
		z = centerZ + (_z - clientModelData[start].z);
		radius = sqrt((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY) + (z - centerZ) * (z - centerZ));
		startAngle = -asin((y-centerY)/radius);

		cookie = malloc(8 * sizeof(double));
		cookie[0] = initSpeed / initRadius;
		cookie[1] = radius;
		cookie[2] = centerX;
		cookie[3] = centerY;
		cookie[4] = centerZ;
		cookie[5] = ww2clientNetClock() / 1000.0f;
		cookie[6] = startAngle;
		cookie[7] = radius * cookie[0];

		client_add_controller(&clientModelData[i], cookie, controller_circle);
	}*/

	return 0;
}

/************************************************************************************************
	script_cmd
*************************************************************************************************/

static int script_cmd(int argc, char* argv[])
{
	int fd;
	char line[256];

	if(argc != 2) return -1;

	fd = fioOpen(argv[1]);
	if(fd < 0) return -2;

	while(fioGetString(fd, line, 255) > 0)
	{
		int argc;
		char* argv[10];
		interp_cmd cmd;

		if(line[0] == '#') continue;

		printf("%s\n", line);
		argc = interp_parse_args(line, argv, 10);

		if(argc && argv[0])
		{
			cmd = lookup_interp_cmd(argv[0]);
			if(cmd)
				cmd(argc, argv);
		}
	}

	fioClose(fd);

	return 0;
}

/************************************************************************************************
	formation_cmd
*************************************************************************************************/

static int formation_cmd(int argc, char* argv[])
{
	int i, start, end;
	float angle, space;
	float dx, dy, startx, starty, startz;
	int cols;
	if(argc < 4) return -1;
	float _x, _y, _z, _pitch, _roll, _yaw;

	interp_range(argv[1], &start, &end);
	space = atof(argv[3]);

	if(argc > 4)
	{
		angle = atof(argv[4]) * M_PI / 180.0;
		dx = space * sin(angle);
		dy = space * cos(angle);
	}

	CmdLineClient* client = get_client(start);
	if(!client) return -1;

	client->GetPosition(startx, starty, startz);

	if(!strcmp(argv[2], "rank"))
	{
		if(argc != 5) return -1;

		for(i = start; i <= end; i++)
		{
			CmdLineClient* client = get_client(i);
			if(!client) continue;

			_x = startx + dx * (i - start);
			_y = starty + dy * (i - start);
			_z = startz;

			_pitch = 0;
			_roll = 0;
			_yaw = angle + M_PI/2.0;

			client->MoveTo(_x, _y, _z);
			client->RotateToRadians(_pitch, _roll, _yaw);
		}
	}else if(!strcmp(argv[2], "file"))
	{
		if(argc != 5) return -1;

		for(i = start; i <= end; i++)
		{
			CmdLineClient* client = get_client(i);
			if(!client) continue;

			_x = startx + dx * (i - start);
			_y = starty + dy * (i - start);
			_z = startz;

			_pitch = 0;
			_roll = 0;
			_yaw = angle;

			client->MoveTo(_x, _y, _z);
			client->RotateToRadians(_pitch, _roll, _yaw);
		}
	}else if(!strcmp(argv[2], "wedge"))
	{
		int half = (end + start) / 2;

		if(argc != 5) return -1;

		for(i = start; i <= end; i++)
		{
			CmdLineClient* client = get_client(i);
			if(!client) continue;

			dx = space * i;
			dy = (i <= half) ? space * i : space * (2 * half - i);

			_x = startx + dx * cos(angle) - dy * sin(angle);
			_y = starty + dy * cos(angle) + dx * sin(angle);
			_z = startz;

			_pitch = 0;
			_roll = 0;
			_yaw = angle;

			client->MoveTo(_x, _y, _z);
			client->RotateToRadians(_pitch, _roll, _yaw);
		}
	}else if(!strcmp(argv[2], "circle"))
	{
		double da = 2 * M_PI / (end - start + 1);
		if(argc != 4) return -1;

		for(i = start; i <= end; i++)
		{
			CmdLineClient* client = get_client(i);
			if(!client) continue;

			_x = startx + space * sin(da * (start - i));
			_y = starty + space * cos(da * (start - i));
			_z = startz;

			_pitch = 0;
			_roll = 0;
			_yaw = da * (start - i);

			client->MoveTo(_x, _y, _z);
			client->RotateToRadians(_pitch, _roll, _yaw);
		}
	}else if(!strcmp(argv[2], "grid"))
	{
		int col, row;
		if(argc != 6) return -1;

		cols = atoi(argv[5]);

		for(i = start; i <= end; i++)
		{
			CmdLineClient* client = get_client(i);
			if(!client) continue;

			row = (i - start) % cols;
			col = (i - start) / cols;

			dx = space * row;
			dy = space * col;

			_x = startx + dx * cos(angle) - dy * sin(angle);
			_y = starty + dy * cos(angle) + dx * sin(angle);
			_z = startz;

			_pitch = 0;
			_roll = 0;
			_yaw = angle;

			client->MoveTo(_x, _y, _z);
			client->RotateToRadians(_pitch, _roll, _yaw);
		}
	}else if(!strcmp(argv[2], "missingman"))
	{
		int half = (end + start) / 2;

		if(argc != 5) return -1;

		for(i = start; i <= end; i++)
		{
			CmdLineClient* client = get_client(i);
			if(!client) continue;

			if(i == 0)
			{
				dx = -space;
				dy = -space;
			}else
			{
				dx = space * i;
				dy = (i <= half) ? space * i : space * (2 * half - i);
			}

			_x = startx + dx * cos(angle) - dy * sin(angle);
			_y = starty + dy * cos(angle) + dx * sin(angle);
			_z = startz;

			_pitch = 0;
			_roll = 0;
			_yaw = angle;

			client->MoveTo(_x, _y, _z);
			client->RotateToRadians(_pitch, _roll, _yaw);
		}
	}

	return 0;
}

/************************************************************************************************
	turret_cmd
*************************************************************************************************/

static int turret_cmd(int argc, char* argv[])
{
	int i, start, end;
	int turretnum, traverse, elev;

	if(argc != 5) return -1;

	interp_range(argv[1], &start, &end);

	turretnum = atoi(argv[2]);
	traverse = (int)(atof(argv[3]) * 255);
	elev = int(atof(argv[4]) * 255);

	for(i = start; i <= end; i++)
	{
		CmdLineClient* client = get_client(i);
		if(!client) continue;

		client->SetTurretPosition(turretnum, traverse, elev);
	}

	return 0;
}

/************************************************************************************************
	fire_cmd
*************************************************************************************************/

static int fire_cmd(int argc, char* argv[])
{
/*	int i, j, start, end;
	int onetime;
	int mountstart, mountend;

	if(argc != 4) return -1;

	interp_range(argv[1], &start, &end);
	interp_range(argv[2], &mountstart, &mountend);
	onetime = !atoi(argv[3]);

	if(!strcmp(argv[2], "all"))
	{
		mountstart = 0;
		mountend = 16;
	}

	for(i = start; i <= end; i++)
	{
		for(j =  mountstart; j <= mountend; j++)
			ww2clientSetFireState(clients[i], j, 1, onetime);
	}*/

	return 0;
}

/************************************************************************************************
	state_cmd
*************************************************************************************************/

static int state_cmd(int argc, char* argv[])
{
	int i, j, k, val;
	int start, end;

	if(argc != 4) return -1;

	val = (!strcmp(argv[3], "on")) ? 1 : 0;

	interp_range(argv[1], &start, &end);

	for(i = start; i <= end; i++)
	{
		CmdLineClient* client = get_client(i);
		if(!client) continue;

		vehType const* vehtype = vehTypeGetBySpec(client->SpawnNotify().weaponSerial.spec);
		if(vehtype->Type == VEH_TYPE_PERSON) continue;

		for(j = 0; STATE_NAMES[j]; j++)
		{
			if(!strcmp(STATE_NAMES[j], argv[2]))
			{
				for(k = 0; k < VEH_MAX_STATE_BITS; k++)
				{
					if(j == 0 || vehtype->StateType[k].StateSubType == j)
					{
						client->SetStateFlagBit(k, val);
					}
				}
				break;
			}
		}
	}

	return 0;
}

/************************************************************************************************
	visbit_cmd
*************************************************************************************************/

static int visbit_cmd(int argc, char* argv[])
{
	return 0;
}

/************************************************************************************************
	exit_cmd
*************************************************************************************************/

static int exit_cmd(int argc, char* argv[])
{
	cmdLineClientKill();
	return 1;
}

void cmdLineClientKill()
{
	ClientMap::iterator it = clients.begin();
	for(; it != clients.end(); ++it)
	{
		CmdLineClient* client = it->second;
		if(!client || !client->CellClientActive() || client->Connection() < 0) continue;
		client->SendPlayerExit(PEXIT_RTB, nullObjID);
//		client->ClearWaits();
//		client->SetWait(ww2::Client::MAP_CLIENT_WAIT);
	}

	ww2::Client::NetUpdate(100);

	it = clients.begin();
	for(; it != clients.end(); ++it)
	{
		CmdLineClient* client = it->second;
//		client->WaitForMessages(1);
		client->Logout();

		ww2::Client::NetUpdate(100);
		usleep(50);

		delete client;
	}

	clients.clear();
	ww2::Client::NetUpdate();
}

/************************************************************************************************
	weaplist_cmd
*************************************************************************************************/

static int weaplist_cmd(int argc, char* argv[])
{
	/*FULL_OBJ_ID fac;
	char* facname;
	int id;

	if(argc != 3) return -1;

	id = atoi(argv[1]);
	if(id < 0 && id >= numClients) return 1;

	facname = argv[2];
	if(lookup_facility(facname, &fac) != 0)
	{
		printf("lookup on facility: %s failed\n", facname);
		return 2;
	}

	ww2clientRequestWeapList(clients[id], &fac);
	if(ww2clientWaitRequestCompletion(clients[id], WAIT_TIME) == 0)
	{
		FACILITY_WEAPON_LIST const* weapList;
		int i;

		weapList = ww2clientWeaponList(clients[id]);

		printf("%s weapon list\n", facname);

		for(i = 0; i < weapList->numWeapons; i++)
		{
			WEAPON* weapon = weapLookup(weapList->weapon[i]);
			if(weapon)
				printf("%s ", weapon->name);
		}
		printf("\n");
	}*/

	return 0;
}

/************************************************************************************************
	help_cmd
*************************************************************************************************/

static int help_cmd(int argc, char* argv[])
{
/*	if(argc == 1)
	{
		printf(HELP_HELP);

	}else
	{
		char* cmd = argv[1];
		int i;

		for(i = 0; INTERP_CMDS[i].name; i++)
		{
			if(!strcmp(INTERP_CMDS[i].name, cmd))
			{
				if(INTERP_CMDS[i].help)
				{
					printf(INTERP_CMDS[i].help);
				}

				break;
			}
		}
	}
	*/
	return 0;
}

/************************************************************************************************
	fps_cmd
*************************************************************************************************/

static int fps_cmd(int argc, char* argv[])
{
/*	int i, val, start, end;
	unsigned int flag;

	if(argc < 3) return -1;

	init_fps_bits();
	interp_range(argv[1], &start, &end);

	for(i = start; i <= end; i++)
	{
		flag = ww2clientStateFlag(clients[i]);

		if(!strcmp(argv[2], "weap"))
		{
			if(argc < 4 ) return -1;

			val = atoi(argv[3]);
			flag &= ~fpsWeapMask;
			flag |= (val << fpsWeapBitShift);
		}
		else if(!strcmp(argv[2], "sit"))
		{
			val = TPS_POSETYPE_SIT;
			flag &= ~fpsPoseMask;
			flag |= (val << fpsPoseBitShift);
		}
		else if(!strcmp(argv[2], "crouch"))
		{
			val = TPS_POSETYPE_CROUCH;
			flag &= ~fpsPoseMask;
			flag |= (val << fpsPoseBitShift);
		}
		else if(!strcmp(argv[2], "prone"))
		{
			val = TPS_POSETYPE_PRONE;
			flag &= ~fpsPoseMask;
			flag |= (val << fpsPoseBitShift);

		}else if(!strcmp(argv[2], "stand"))
		{
			val = TPS_POSETYPE_STAND;
			flag &= ~fpsPoseMask;
			flag |= (val << fpsPoseBitShift);
		}
		else if(!strcmp(argv[2], "sprint"))
		{
			val = TPS_MOVETYPE_SPRINT;
			flag &= ~fpsMoveMask;
			flag |= (val << fpsMoveBitShift);
		}
		else if(!strcmp(argv[2], "trot"))
		{
			val = TPS_MOVETYPE_FORWARD;
			flag &= ~fpsMoveMask;
			flag |= (val << fpsMoveBitShift);
		}else if(!strcmp(argv[2], "back"))
		{
			val = TPS_MOVETYPE_FORWARD;
			flag &= ~fpsMoveMask;
			flag |= (val << fpsMoveBitShift);
		}else if(!strcmp(argv[2], "deploy"))
		{
			val = TPS_MOVETYPE_DEPLOYED;
			flag &= ~fpsMoveMask;
			flag |= (val << fpsMoveBitShift);
		}
		else if(!strcmp(argv[2], "idle"))
		{
			val = TPS_MOVETYPE_IDLE;
			flag &= ~fpsMoveMask;
			flag |= (val << fpsMoveBitShift);
		}
		else if(!strcmp(argv[2], "relax"))
		{
			val = TPS_POINTTYPE_RELAXED;
			flag &= ~fpsPointMask;
			flag |= (val << fpsPointBitShift);
		}else if(!strcmp(argv[2], "down"))
		{
			val = TPS_POINTTYPE_DOWN;
			flag &= ~fpsPointMask;
			flag |= (val << fpsPointBitShift);
		}
		else if(!strcmp(argv[2], "right"))
		{
			val = 1;
			flag &= ~fpsHeadRightMask;
			flag |= (val << fpsHeadRightBitShift);

			val = 0;
			flag &= ~fpsHeadLeftMask;
			flag |= (val << fpsHeadLeftBitShift);
		}
		else if(!strcmp(argv[2], "left"))
		{
			val = 1;
			flag &= ~fpsHeadLeftMask;
			flag |= (val << fpsHeadLeftBitShift);

			val = 0;
			flag &= ~fpsHeadRightMask;
			flag |= (val << fpsHeadRightBitShift);

		}
		else if(!strcmp(argv[2], "ahead"))
		{
			val = 0;
			flag &= ~fpsHeadRightMask;
			flag |= (val << fpsHeadRightBitShift);

			val = 0;
			flag &= ~fpsHeadLeftMask;
			flag |= (val << fpsHeadLeftBitShift);
		}
		else if(!strcmp(argv[2], "alive"))
		{
			val = 0;
			flag &= ~fpsDeathMask;
			flag |= (val << fpsDeathBitShift);
		}
		else if(!strcmp(argv[2], "death"))
		{
			if(argc < 4 ) return -1;

			val = atoi(argv[3]);
			flag &= ~fpsDeathMask;
			flag |= (val << fpsDeathBitShift);
		}else if(!strcmp(argv[2], "emote"))
		{
			if(argc < 4 ) return -1;

			val = atoi(argv[3]);
			flag &= ~fpsEmoteMask;
			flag |= (val << fpsEmoteBitShift);
		}else if(!strcmp(argv[2], "alt"))
		{
			if(argc < 4 ) return -1;

			val = atoi(argv[3]);
			flag &= ~fpsAltMask;
			flag |= (val << fpsAltBitShift);
		}

		ww2clientSetStateFlag(clients[i], flag);
	}
	*/
	return 0;
}

/************************************************************************************************
	visdam_cmd
*************************************************************************************************/

static int visdam_cmd(int argc, char* argv[])
{
	/*int i, j, val;
	int start, end;

	if(argc != 4) return -1;

	val = (!strcmp(argv[3], "on")) ? 1 : 0;

	interp_range(argv[1], &start, &end);

	for(i = start; i <= end; i++)
	{
		int vehIdx = ww2clientVehicleIndex(clients[i]);
		if(vehIdx > -1 && vehIdx < _VehicleList.Count)
		{
			vehType* vehtype = &_VehicleList.List[vehIdx];
			if(vehtype->Type != VEH_TYPE_PERSON)
			{
				for(j = 0; j < VEH_MAX_STATE_BITS; j++)
				{
					if(vehtype->StateType[j].StateType == VEHANIM_STATE_TYPE_VISDAMAGE
						&& !strcmp(vehtype->StateType[j].Name, argv[2]) )
					{
						ww2clientSetStateFlagBit(clients[i], j, val);
						break;
					}
				}
			}
		}
	}
*/
	return 0;
}

/************************************************************************************************
	data_cmd
*************************************************************************************************/

static int data_cmd(int argc, char* argv[])
{
/*	int start, end;
	int i;

	if(argc != 3) return -1;
	interp_range(argv[1], &start, &end);

	for(i = start; i <= end; i++)
	{
		if(!strcmp(argv[2], "pos"))
		{
			printf("#%d pos: %f %f %f\n", i, _x, _y, _z);
		}else if(!strcmp(argv[2], "vel"))
		{
			printf("#%d vel: %f %f %f\n", i, _vx, _vy, _vz);
		}else if(!strcmp(argv[2], "att"))
		{
			printf("#%d att: %f %f %f\n", i, _pitch, _roll, _yaw);
		}
	}*/

	return 0;
}

/************************************************************************************************
	ride_cmd
*************************************************************************************************/

static int ride_cmd	(int argc, char* argv[])
{
/*	int i, start, end;
	const char* name;
	int seatnum;

	interp_range(argv[1], &start, &end);

	name = argv[2];
	seatnum = atoi(argv[3]);

	if(argc == 2)
	{
		for(i = start; i <= end; i++)
		{
			ww2clientSetRide(clients[i], 0, 0);
		}
	}else
	{
		for(i = start; i <= end; i++)
		{
			PLAYER_VEHICLE const* veh = ww2clientFindVehicleWithCallsign(clients[i], name);
			if(veh)
				ww2clientSetRide(clients[i], veh->vehicleData.index, seatnum);
		}
	}
		*/
	return 0;
}

/************************************************************************************************
	resupply_cmd
*************************************************************************************************/

static int resupply_cmd	(int argc, char* argv[])
{
/*	int i, start, end;
	const char* name;

	interp_range(argv[1], &start, &end);

	name = argv[2];

	if(argc == 2)
	{
		for(i = start; i <= end; i++)
		{
			ww2clientSetVisBits(clients[i], 0);
		}
	}else
	{
		for(i = start; i <= end; i++)
		{
			PLAYER_VEHICLE const* veh = ww2clientFindVehicleWithCallsign(clients[i], name);
			if(veh)
				ww2clientSetVisBits(clients[i], veh->vehicleData.index);
		}
	}
		*/
	return 0;
}

/************************************************************************************************
	interp_range
*************************************************************************************************/

static void interp_range(char* s, int* start, int* end)
{
	s = strtok(s, "-");
	*start = s ? atoi(s) : 0;

	s = strtok(NULL, "-");
	*end = s ? atoi(s) : *start;
}

/************************************************************************************************
	interp_parse_args
*************************************************************************************************/

static int interp_parse_args(char* line, char* argv[], int maxargs)
{
	int i;
	char* s;
	char sep = 0;

	s = line + strspn(line, " ");

	for(i = 0; *s && i < 10; i++)
	{
		argv[i] = s;

		if(sep == '\"')
		{
			s += strcspn(s, "\"");
			if(*s)
			{
				*s = '\0';
				s++;
				sep = *s;
			}
		}else
		{
			s += strcspn(s, " \"");
			if(*s)
			{
				sep = *s;
				*s = '\0';
				s++;
			}
		}

		if(sep == ' ')
			s += strspn(s, " ");

		sep = *s;

		if(sep == '\"' && *s)
			s++;
	}

	return i;
}


/************************************************************************************************
	lookup_interp_cmd
*************************************************************************************************/

static interp_cmd lookup_interp_cmd(const char* cmdName)
{
	int i;
	for(i = 0; INTERP_CMDS[i].name ; i++)
	{
		if (!strcmp(cmdName, INTERP_CMDS[i].name))
		{
			return INTERP_CMDS[i].cmd;
		}
	}

	return NULL;
}

int cmdLineClientExecuteCommand(char* line)
{
	int argc;
	char* argv[10];

	argc = interp_parse_args(line, argv, 10);

	return cmdLineClientExecuteCommand(argc, argv);
}

int cmdLineClientExecuteCommand(int argc, char* argv[])
{
	int err = -1;
	if(argc && argv[0])
	{
		interp_cmd cmd = lookup_interp_cmd(argv[0]);
		if(cmd)
		{
			err = cmd(argc, argv);
			if(err < 0)
				printf("invalid syntax\n");
		}
		else
		{
			printf("unknown command\n");
		}
	}
	return err;
}

void cmdLineClientSetPlayerIdStart(int id)
{
	playeridStart = id;
}

void cmdLineClientSetHost(const char* s)
{
	strncpy(host, s, 31);
}

static int host_cmd		(int argc, char* argv[])
{
	if(argc > 1)
		cmdLineClientSetHost(argv[1]);
}

static int playerid_cmd	(int argc, char* argv[])
{
	if(argc > 1)
		cmdLineClientSetPlayerIdStart(atoi(argv[1]));
}

void CmdLineClient::MoveTo(float x,float y,float z)
{
	_x = x; _y = y; _z = z;
}

void CmdLineClient::MoveBy(float x,float y,float z)
{
	_x += x; _y += y; _z += z;
}

void CmdLineClient::Update()
{
	unsigned t = ww2::Client::NetTime();
	float dt = (t - _lastupdate) / 1000.0;
	_lastupdate = t;

	_x += _vx * dt;
	_y += _vy * dt;
	_z += _vz * dt;

	SetVelocity(_vx,_vy,_vz);
	SetAttitude(_pitch, _roll, _yaw);
	SetPosition(_x,_y,_z);
	ww2::Client::Update();
}

CmdLineClient::CmdLineClient()
{
	_x = _y = _z = 0;
	_vx = _vy = _vz = 0;
	_pitch = _roll = _yaw = 0;
	_lastupdate = 0;
	SetAutoReconnect(false);
}

CmdLineClient* get_client(int i)
{
	ClientMap::iterator it = clients.find(i);
	if(it == clients.end()) return 0;

	return it->second;
}

void CmdLineClient::RotateTo(float pitch, float roll, float yaw)
{
	_pitch = -DEG_TO_RAD(pitch);
	_roll = DEG_TO_RAD(roll);
	_yaw = DEG_TO_RAD(yaw);
}

void CmdLineClient::RotateToRadians(float pitch, float roll, float yaw)
{
	_pitch = pitch;
	_roll = roll;
	_yaw = yaw;
}

void CmdLineClient::RotateBy(float pitch, float roll, float yaw)
{
	_pitch += -DEG_TO_RAD(pitch);
	_roll += DEG_TO_RAD(roll);
	_yaw += DEG_TO_RAD(yaw);
}

void CmdLineClient::SetSpeed(float speed)
{
	float x,y,z;
	GetDirection(x, y, z);
	SetVel(speed*x, speed*y, speed*z);
}

void CmdLineClient::AddSpeed(float speed)
{
	float x,y,z;
	GetDirection(x, y, z);
	AddVel(speed*x, speed*y, speed*z);
}

void CmdLineClient::SetVel(float vx, float vy, float vz)
{
	_vx = vx; _vy = vy; _vz = vz;
}

void CmdLineClient::AddVel(float vx, float vy, float vz)
{
	_vx += vx; _vy += vy; _vz += vz;
}

void CmdLineClient::GetDirection(float& x, float& y, float& z)
{
	float xp,yp,zp;

	x = 0; z = 0; y = 1;
	yp = y * cos(_pitch) + z * sin(_pitch);
	zp = z * cos(_pitch) - y * sin(_pitch);
	y = yp; z = zp; xp = x;

	xp = x * cos(_roll) + z * sin(_roll);
	zp = z * cos(_roll) - x * sin(_roll);
	x = xp; z = zp; yp = y;

	yp = y * cos(-_yaw) + x * sin(-_yaw);
	xp = x * cos(-_yaw) - y * sin(-_yaw);
	x = xp; y = yp; z = zp;
}

void cmdLineClientSetStartPosition(float x,float y,float z)
{
	_startx = x; _starty = y; _startz = z;
}


void CmdLineClient::GetPosition(float& x,float& y,float& z)
{
	x = _x; y = _y; z = _z;
}

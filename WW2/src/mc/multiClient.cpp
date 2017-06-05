#include <syscfg.h> // Path is project specific.
#ifdef MULTI_CLIENT

#include <map>
#include <math.h>
#include <stack>

#include "fio/fio.h"
#include "strt/strtTypes.h"
#include "mm/mmSpawnTypes.h"
#include "mc/multiClient.h"
#include "mc/multiClientHelp.h"
#include "client/clientNetError.h"
#include "client/GameClient.h"
#include "veh/vehType.h"
#include "ui/uiChat.h"
#include "mm/mmEnums.h"
#include "wwii/wwiiExitCodes.h"
#include "spt/Random.h"
#include "fio/fiod.h"

#define atoF(_x_) ((float)atof((_x_)))

/************************************************************************************************
	multiClient
*************************************************************************************************/

#define DEFAULT_PLAYERID_START 	84649
#define DEFAULT_HOST			"192.168.0.83"
#define DEG_TO_RAD(x) ((x) * PI / 180.0)

using interp_cmd = int (*)(int argc, char* argv[]);

struct interp_cmd_lookup
{
	const char* name;
	interp_cmd cmd;
	const char* help;
	bool queue;
};

using QUEUED_CMDS = std::deque<std::string>;
static QUEUED_CMDS _cmdQueue;

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
static int kill_cmd		(int argc, char* argv[]);
static int weapons_cmd	(int argc, char* argv[]);
static int help_cmd		(int argc, char* argv[]);
static int fps_cmd		(int argc, char* argv[]);
static int visdam_cmd	(int argc, char* argv[]);
static int data_cmd		(int argc, char* argv[]);
static int ride_cmd		(int argc, char* argv[]);
static int resupply_cmd	(int argc, char* argv[]);
static int host_cmd		(int argc, char* argv[]);
static int playerid_cmd	(int argc, char* argv[]);
static int wantchat_cmd	(int argc, char* argv[]);
static int chatpctg_cmd	(int argc, char* argv[]);
static int ranchat_cmd	(int argc, char* argv[]);
static int ticket_cmd	(int argc, char* argv[]);
static int pos_cmd		(int argc, char* argv[]);
static int login_cmd	(int argc, char* argv[]);
static int list_cmd		(int argc, char* argv[]);
static int clearq_cmd	(int argc, char* argv[]);

static interp_cmd_lookup INTERP_CMDS[] =
{
	{"help", 		help_cmd,		HELP_HELP, false},			// help cmdname
	{"script",		script_cmd,		SCRIPT_HELP, false},		// script filename
	{"host",		host_cmd,		HOST_HELP, false},			// set host
	{"playerid",	playerid_cmd,   PLAYERID_HELP, false},		// set client start id
	{"wantchat",	wantchat_cmd,   WANTCHAT_HELP, false},		// set whether you want chat or not
	{"chatpctg",	chatpctg_cmd,   CHATPCTG_HELP, false},		// how often to send a random chat message
	{"ticket", 		ticket_cmd,		TICKET_HELP, false},		// set the login info not including the playerid
	{"pos", 		pos_cmd,		POS_HELP, false},			// set the start spawn pos
	{"login", 		login_cmd,		LOGIN_HELP, true},			// login a number of clients
	{"list", 		list_cmd,		LIST_HELP, false},			// list info about a client or range of clients
	{"spawn",		spawn_cmd,		SPAWN_HELP, true}, 			// spawn client#(s) vehname facname
	{"despawn", 	despawn_cmd,	DESPAWN_HELP, true},		// despawn client#(s)
	{"kill", 		kill_cmd,		KILL_HELP, false},			// kills all clients, save yours if you are online
	{"data", 		data_cmd,		DATA_HELP, false},			// data client#(s) (pos, vel, att)
	{"move",		move_cmd,		MOVE_HELP, false},			// move client#(s) [rel, abs] x y z
	{"rotate",		rotate_cmd,		ROTATE_HELP, false},		// rotate client#(s) [rel, abs] pitch roll yaw
	{"velocity",  	velocity_cmd,	VEL_HELP, false},			// velocity client#(s) [rel, abs] x y z
	{"speed",		speed_cmd,		SPEED_HELP, false},			// speed client#(s) speed
	{"circle",  	circle_cmd,		CIRCLE_HELP, false},		// circle client#(s) [rel, abs] x y z radius speed
	{"formation",	formation_cmd,	FORM_HELP, false},			// formation client#(s) [rank, file, wedge, circle, grid] (radius/spacing) (orientation) (cols)
	{"weapons", 	weapons_cmd,	WEAPLIST_HELP, false},		// weaplist facname
	{"clearq", 		clearq_cmd,		CLEARQ_HELP, false},		// clear out the command queue
	//{"turret",		turret_cmd,		TURRET_HELP},	// turret client#(s) # elevation traverse
	//{"fire",		fire_cmd,		FIRE_HELP},			// fire client#(s) mount
	//{"state",		state_cmd,		STATE_HELP},		// state client#(s) statename on/off
	//{"visbit",		visbit_cmd,		VISBIT_HELP},	// visbit client#(s) visname on/off
	//{"fps", 		fps_cmd,		FPS_HELP},			// fps client#(s) type state
	//{"visdam", 		visdam_cmd,		VIS_HELP},		// fps client#(s) type state
	//{"ride", 		ride_cmd,		""},				// ride client#(s) vehname seatnumber
	//{"resupply",	resupply_cmd,   ""},				// resupply client#(s) vehname
	{nullptr, 	nullptr}
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
	nullptr
};

// ticket info
static UINT32 host = 0;
static UINT16 port = 0;
static UINT16 protocol = 0;
static UINT32 otp = 0;
static char data[25+1];

static bool wantChat = true;
static float chatPercentage = .5f;
static int playeridStart = DEFAULT_PLAYERID_START;
float _startx, _starty, _startz;

std::vector<int> players;

//static int client_spawn(int clientIndex, WEAPON_SPEC const&);

static int  interp_parse_args(char* s, char* argv[], int maxargs);
static void interp_range(char* s, int& start, int& end);
static interp_cmd lookup_interp_cmd(const char* cmdName);
static interp_cmd_lookup& lookup_interp_cmd_lookup(const char* cmdName);

class MultiClient : public ww2::Client
{
public:
	MultiClient();

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

MultiClient* get_client(int i);

using ClientMap = std::map<int, MultiClient*>;
static ClientMap clients;

/************************************************************************************************
	client_update
*************************************************************************************************/

void multiClientUpdate(int wait, bool force)
{
	if(!Online)
		ww2::Client::NetUpdate(wait);

	static std::vector<std::string> chats;

	if(chats.empty())
	{
		chats.push_back("Multiclient, reporting in!");
		chats.push_back("/time");
		chats.push_back("/wtf");
		chats.push_back("Spambot to the rescue!");
		chats.push_back("/all Suck it!");
	}

	static unsigned int lasttime = 0;
	static unsigned int nextCmd = 0;

	unsigned int time = ww2::Client::NetTime();

	if(time - lasttime > 250)
	{
		lasttime = time;

		ClientMap::iterator it = clients.begin();
		for(; it != clients.end(); ++it)
		{
			if (GetRandom(0.0f, 1.0f) < chatPercentage)
			{
				it->second->SendChatMessage(0, chats.at(GetRandom(0, chats.size() - 1)).c_str());
			}

			it->second->Update();
			it->second->SendUpdate();
		}
	}

	// run any queued commands
	if(((time - nextCmd) > 1000) && !_cmdQueue.empty())
	{
		// grab the next command to run
		std::string& cmdToRun = _cmdQueue.front();

		char line[255+1];
		strsafecpy(line, cmdToRun.c_str());

		// run the command
		multiClientExecuteCommand(line);

		// pop us off the queue
		_cmdQueue.pop_front();

        nextCmd = time;
        
	}
}

static bool queueCmd(int argc, char* argv[])
{
	int start = -1;
	int end = -1;

	interp_cmd_lookup& lookup = lookup_interp_cmd_lookup(argv[0]);
	bool queued = lookup.queue; // always queue for now
	std::string cmdToQueue;

	for(int i = 0; i < argc; i++)
	{
		if(!argv[i])
			continue;

		if(strcasecmp(argv[i], "q") == 0)
		{
			queued = true;
			continue;
		}

		if(i > 0)
			cmdToQueue += " ";

		if(i == 1 && strstr(argv[i], "-"))
		{
			interp_range(argv[i], start, end);
			cmdToQueue += "%d";
		}
		else
		{
			cmdToQueue += argv[i];
		}
	}

	// push our command on the stack to run later if we found the "q" flag at the end of the arguments
	if(queued)
	{
		if(start > -1)
		{
			// add a queued command for each vehicle index
			for(int i = start; i <= end; i++)
			{
				char line[255+1];
				snprintf(line, 255, cmdToQueue.c_str(), i);
				std::string cmdLine = line;
				_cmdQueue.push_back(cmdLine);
			}
		}
		else
		{
			_cmdQueue.push_back(cmdToQueue);
		}
	}

	return queued;
}

static void personaFromSpec(const WEAPON_SPEC spec, int& country, int& branch)
{
	country	= spec.wCountry;
	branch	= BRANCH_LAND;

	switch(spec.wCategory)
	{
		case 1: branch = BRANCH_AIR; break;
		case 3: branch = BRANCH_SEA; break;
	}
}

/************************************************************************************************
	client_login
*************************************************************************************************/
static int client_login(int clientIndex, int country, int branch)
{
	if(host == 0 || otp == 0)
		return -1;

	MultiClient* client = 0;

	ClientMap::iterator it = clients.find(clientIndex);
	if(it == clients.end())
	{
		client = new MultiClient;
		int err = -1;
		try
		{
			// set our player login identity
			if(playeridStart == -1 && clientIndex < (int)players.size())
				client->SetPlayerIdent(players[clientIndex], otp, 0);
			else
				client->SetPlayerIdent(playeridStart+clientIndex, otp, 0);

			err = client->Login(host, 0, wantChat);
		}
		catch(ClientNetError& e)
		{
			uiChatPrintLine("%d: connection exception thrown: %s", clientIndex, e.What());
		}

		if(err != 0)
		{
			uiChatPrintLine("%d: login failed", clientIndex);
			delete client;
			return -1;
		}
		/*
		else
		{
			uiChatPrintLine("%d: logged in", clientIndex);
		}
		*/

		clients[clientIndex] = client;

		// get our initial list of personas
		client->ClearWaits();
		client->SetWait(ww2::Client::PERSONAS_REQUEST_WAIT);
		client->RequestPersonaSummaries();

		if(client->WaitForMessages(5) != 0)
		{
			uiChatPrintLine("%d: Personas request timeout!", clientIndex);
			return 0; // go ahead and bail, but don't fail it
		}
	}
	else
	{
		client = it->second;
	}

	if(!client) return -1;

	const PersonaSummary* persona = client->GetPersona(country, branch);
	if(!persona)
	{
		uiChatPrintLine("%d: Persona not found!", clientIndex);
		return 0; // go ahead and bail, but's don't fail
	}

	// select our persona, non critical failure is it's not successful
	client->SelectPersona(*persona, true);

	uiChatPrintLine("%d: logged in!", clientIndex);

	// try logging in
	//Need to set side, country, areachat (boolean)
	client->SetAreaChat(true);


	return 0;
}

/************************************************************************************************
	client_spawn
*************************************************************************************************/
static int client_spawn(int clientIndex, const WEAPON_SPEC spec)
{
	int country = -1;
	int branch = -1;

	// set our persona
	personaFromSpec(spec, country, branch);

	MultiClient* client = 0;

	ClientMap::iterator it = clients.find(clientIndex);
	if(it == clients.end())
	{
		if(client_login(clientIndex, country, branch) != 0)
			return -1;

		it = clients.find(clientIndex);
	}

	if(it != clients.end())
		client = it->second;

	if(!client) return -1;

	if(client->CellClientActive())
	{
		uiChatPrintLine("%d: already spawned", clientIndex);
		return 0;
	}

	// make sure we have the correct persona selected
	const PersonaSummary* persona = client->GetPersona(country, branch);
	if(!persona)
	{
		uiChatPrintLine("%d: Persona not found!", clientIndex);
		return -1;
	}

	// it will only select it if it's different than the currently selected one
	if(!client->SelectPersona(*persona, true))
	{
		uiChatPrintLine("%d: Persona select failed!", clientIndex);
		return -1;
	}

	//write notification out to stdout
//	printf("%d: after selectpersona \n", clientIndex);


	// now let's spawn
	client->RequestDirectSpawn(spec);

	client->ClearWaits();
	client->SetWait(ww2::Client::SPAWN_NOTIFY_WAIT);
	client->SetWait(ww2::Client::CELL_CLIENT_WAIT);
//	client->SetWait(ww2::Client::REDIRECT_WAIT);

	if (client->WaitForMessages(10) != 0)
	{
		uiChatPrintLine("%d: spawn request timeout!", clientIndex);
		// pop us off the queue
		_cmdQueue.pop_front();
		return -1;
	}

	//write notification out to stdout
//	printf("%d: after WaitForMessages \n", clientIndex);


	//Testing - use the same method for vehicle as persona above
	client->ClearWaits();
	client->SetWait(ww2::Client::VEHICLE_REPLY_WAIT);

	client->RequestVehicleJoin(client->SpawnNotify().weaponSerial,0);


	//write notification out to stdout
//	printf("%d: after RequestVehicleJoin \n", clientIndex);


//	if(client->WaitForMessage(ww2::Client::VEHICLE_REPLY_WAIT, 5) != 0)
	if (client->WaitForMessages(5) != 0)
	{
		uiChatPrintLine("%d: vehicle join request timeout!", clientIndex);
		return -1;
	}

//	printf("%d: vehicle join request Completed\n", clientIndex);

	//Update the distance from the pos command by each new client
	//client->MoveTo(_startx + clientIndex*10, _starty, _startz);
	client->MoveTo(_startx + clientIndex * 1.50, _starty, _startz);

	uiChatPrintLine("%d: spawned", clientIndex);

	//write notification out to stdout
//	printf("client %d spawned \n", clientIndex);

	return 0;
}

/************************************************************************************************
	login_cmd
*************************************************************************************************/
static int login_cmd(int argc, char* argv[])
{
	int country, branch;
	int start, end;

	if(argc != 4) return -1;

	interp_range(argv[1], start, end);

	country = atoi(argv[2]);
	branch = atoi(argv[3]);

	for(int i = start; i <= end; i++)
	{
		if(client_login(i, country, branch) < 0)
			continue;

		//uiChatPrintLine("%d: login failed", i);

		// let all of the currently spawned in clients tell the host they are still alive
        
        // TBD: frisbone, temporarily commented out because it could be triggered from this call
		//multiClientUpdate(100, true);

		if(Online)
			PlayerClient()->SendUpdate();

		ww2::Client::NetUpdate(100);
		usleep(0);

		uiChatPrintLine("%d: logged in", i);
	}

	return 0;
}

/************************************************************************************************
	list_cmd
*************************************************************************************************/
static int list_cmd(int argc, char* argv[])
{
	int start, end;

	start = end = 0;

	if(argc < 1) return -1;

	if(argc > 1)
		interp_range(argv[1], start, end);

	MultiClient* client = nullptr;
	ClientMap::iterator it;

	for(int i = start; i <= end; i++)
	{
		it = clients.find(i);

		if(it != clients.end())
		{
			client = it->second;

			int id = i;
			const char* spawned = (client->CellClientActive() && client->CellConnection() > -1) ? "spawned": "not spawned";
			const PersonaSummary* ps = client->GetPersona();
			const char* weapon = "no weapon";
			const char* country = "unknown country";
			const char* branch = "unknown branch";

			if(ps)
			{

				switch(ps->m_countryID)
				{
					case 1: country = "English"; break;
					case 3: country = "French"; break;
					case 4: country = "German"; break;
				}

				switch(ps->m_branchID)
				{
					case 1: country = "Army"; break;
					case 2: country = "Air Force"; break;
					case 3: country = "Navy"; break;
				}

				if(client->CellClientActive() && client->CellConnection() > -1)
				{
					const vehType* vtype = vehTypeGetBySpec(client->SpawnNotify().weaponSerial.spec);

					if(vtype)
						weapon = (const char*)vtype->Id;
				}
			}

			if(strcasecmp(spawned, "spawned") == 0)
				uiChatPrintLine("%d: %s, %s %s,  %s", i, spawned, country, branch, weapon);
			else
				uiChatPrintLine("%d: logged in - but not spawned", i);
		}
		else
		{
			uiChatPrintLine("%d: *** not logged in ***", i);
		}
	}

	return 0;
}


/************************************************************************************************
	spawn_cmd
*************************************************************************************************/
static int spawn_cmd(int argc, char* argv[])
{
	char* weapname;
	WEAPON_SPEC spec;
	int i;
	int start, end;

	if(argc != 3) return -1;

	interp_range(argv[1], start, end);

	weapname = argv[2];

	spec = weapFindWeaponByName(weapname);
	if(spec.isNull())
	{
		uiChatPrintLine("weapon %s not found", weapname);
		return -1;
	}

	for(i = start; i <= end; i++)
	{
		if(client_spawn(i, spec) != 0)
			uiChatPrintLine("%d: spawn failed", i);

		// let all of the currently spawned in clients tell the host they are still alive
		//multiClientUpdate(100, true);

		if(Online)
			PlayerClient()->SendUpdate();

		ww2::Client::NetUpdate(100);
		usleep(0);
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

	interp_range(argv[1], start, end);

	for(i = start; i <= end; i++)
	{
		MultiClient* client = get_client(i);
		if(!client) continue;

		if(client->CellClientActive() && client->CellConnection() > -1)
		{
			client->SendPlayerExit(PEXIT_RTB, nullObjID);
//			client->WaitForMessage(ww2::Client::MAP_CLIENT_WAIT,1);
			ww2::Client::NetUpdate(100);
#if defined(WIN32)
			Sleep(0);
#else
			usleep(50);
#endif
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

	interp_range(argv[1], start, end);
	abs = !strcmp(argv[2], "abs");

	for(i = start; i <= end; i++)
	{
		MultiClient* client = get_client(i);
		if(!client) continue;

		if(abs)
			client->MoveTo(atoF(argv[3]), atoF(argv[4]), atoF(argv[5]));
		else
			client->MoveBy(atoF(argv[3]), atoF(argv[4]), atoF(argv[5]));
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

	interp_range(argv[1], start, end);
	abs = !strcmp(argv[2], "abs");

	for(i = start; i <= end; i++)
	{
		MultiClient* client = get_client(i);
		if(!client) continue;

		if(abs)
			client->RotateTo(atoF(argv[3]), atoF(argv[4]), atoF(argv[5]));
		else
			client->RotateBy(atoF(argv[3]), atoF(argv[4]), atoF(argv[5]));
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

	interp_range(argv[1], start, end);
	abs = !strcmp(argv[2], "abs");

	for(i = start; i <= end; i++)
	{
		MultiClient* client = get_client(i);
		if(!client) continue;

		if(abs)
			client->SetVel(atoF(argv[3]), atoF(argv[4]), atoF(argv[5]));
		else
			client->AddVel(atoF(argv[3]), atoF(argv[4]), atoF(argv[5]));
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

	if(argc != 4) return -1;

	interp_range(argv[1], start, end);
	abs = !strcmp(argv[2], "abs");
	speed = atoF(argv[3]);

	for(i = start; i <= end; i++)
	{
		MultiClient* client = get_client(i);
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
		interp_range(argv[1], start, end);

	if(argc == 2) // clear
	{
		for(i = start; i <= end; i++)
			client_add_controller(&clientModelData[i], nullptr, nullptr);
		return 0;
	}

	if(argc != 8) return -1;

	abs = !strcmp(argv[2], "abs");
	centerX = atoF(argv[3]);
	centerY = atoF(argv[4]);
	centerZ = atoF(argv[5]);
	initRadius = atoF(argv[6]);
	initSpeed = atoF(argv[7]);

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

//	std::string path = "../";
	std::string path = "";
	path += argv[1];

	fd = fiodOpen(path.c_str(), true);
	if(fd < 0) return -2;

	while(fioGetString(fd, line, 255) > 0)
	{
		int argc;
		char* argv[10];
		interp_cmd cmd;

		if(line[0] == '#') continue;

		uiChatPrintLine("%s", line);
		argc = interp_parse_args(line, argv, 10);

		if(argc && argv[0])
		{
			cmd = lookup_interp_cmd(argv[0]);

			// run it or queue it
			if(cmd && !queueCmd(argc, argv))
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

	interp_range(argv[1], start, end);
	space = atoF(argv[3]);

	if(argc > 4)
	{
		angle = atoF(argv[4]) * PI / 180.0f;
		dx = space * sin(angle);
		dy = space * cos(angle);
	}

	MultiClient* client = get_client(start);
	if(!client) return -1;

	client->GetPosition(startx, starty, startz);

	if(!strcmp(argv[2], "rank"))
	{
		if(argc != 5) return -1;

		for(i = start; i <= end; i++)
		{
			MultiClient* client = get_client(i);
			if(!client) continue;

			_x = startx + dx * (i - start);
			_y = starty + dy * (i - start);
			_z = startz;

			_pitch = 0;
			_roll = 0;
			_yaw = angle + PI/2.0f;

			client->MoveTo(_x, _y, _z);
			client->RotateToRadians(_pitch, _roll, _yaw);
		}
	}
	else if(!strcmp(argv[2], "file"))
	{
		if(argc != 5) return -1;

		for(i = start; i <= end; i++)
		{
			MultiClient* client = get_client(i);
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
	}
	else if(!strcmp(argv[2], "wedge"))
	{
		int half = (end + start) / 2;

		if(argc != 5) return -1;

		for(i = start; i <= end; i++)
		{
			MultiClient* client = get_client(i);
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
	}
	else if(!strcmp(argv[2], "circle"))
	{
		double da = 2 * PI / (end - start + 1);
		if(argc != 4) return -1;

		for(i = start; i <= end; i++)
		{
			MultiClient* client = get_client(i);
			if(!client) continue;

			_x = startx + space * (float)sin(da * (start - i));
			_y = starty + space * (float)cos(da * (start - i));
			_z = startz;

			_pitch = 0;
			_roll = 0;
			_yaw = (float)da * (start - i);

			client->MoveTo(_x, _y, _z);
			client->RotateToRadians(_pitch, _roll, _yaw);
		}
	}
	else if(!strcmp(argv[2], "grid"))
	{
		int col, row;
		if(argc != 6) return -1;

		cols = atoi(argv[5]);

		for(i = start; i <= end; i++)
		{
			MultiClient* client = get_client(i);
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
	}
	else if(!strcmp(argv[2], "missingman"))
	{
		int half = (end + start) / 2;

		if(argc != 5) return -1;

		for(i = start; i <= end; i++)
		{
			MultiClient* client = get_client(i);
			if(!client) continue;

			if(i == 0)
			{
				dx = -space;
				dy = -space;
			}
			else
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

	interp_range(argv[1], start, end);

	turretnum = atoi(argv[2]);
	traverse = (int)(atoF(argv[3]) * 255);
	elev = int(atoF(argv[4]) * 255);

	for(i = start; i <= end; i++)
	{
		MultiClient* client = get_client(i);
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

	interp_range(argv[1], start, end);
	interp_range(argv[2], mountstart, mountend);
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

	interp_range(argv[1], start, end);

	for(i = start; i <= end; i++)
	{
		MultiClient* client = get_client(i);
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
	kill_cmd
*************************************************************************************************/

static int kill_cmd(int argc, char* argv[])
{
	multiClientKill();
	return 1;
}

/************************************************************************************************
	clearq_cmd
*************************************************************************************************/

static int clearq_cmd(int argc, char* argv[])
{
	_cmdQueue.clear();
	uiChatPrintLine("Command queue cleared!");
	return 1;
}

void multiClientInit()
{
	if(!Online)
		ww2::Client::Init();

	char buffer[256+1];
	//strsafecpy(buffer, "script mc.script q");
	strsafecpy(buffer, "script mc.script");
	multiClientExecuteCommand(buffer, true);
}

void multiClientKill(bool exit)
{
	ClientMap::iterator it = clients.begin();
	for(; it != clients.end(); ++it)
	{
		MultiClient* client = it->second;
		if(!client || !client->CellClientActive() || client->CellConnection() < 0) continue;
		client->SendPlayerExit(PEXIT_RTB, nullObjID);
//		client->ClearWaits();
//		client->SetWait(ww2::Client::MAP_CLIENT_WAIT);
	}

	ww2::Client::NetUpdate(100);

	it = clients.begin();
	for(; it != clients.end(); ++it)
	{
		MultiClient* client = it->second;

//		client->WaitForMessages(1);
		client->Logout();
		//client->Kill();

		ww2::Client::NetUpdate(100);
#if defined(WIN32)
		Sleep(0);
#else
		usleep(50);
#endif

		delete client;
	}

	clients.clear();
	ww2::Client::NetUpdate();

	if(!Online && exit)
		ww2::Client::Kill();
}

/************************************************************************************************
	weapons_cmd
*************************************************************************************************/

static int weapons_cmd(int argc, char* argv[])
{
	int country = 0;
	int branch = 0;

	if(argc > 1) country = atoi(argv[1]);
	if(argc > 2) branch = atoi(argv[2]);

	uiChatPrintLine("Show vehicles for %d/%d", country, branch);

	for(UINT32 i = 0; i < vehtypeCount(); i++)
	{
		const vehType* type = vehtypeGetByIndex(i);
		if(!type) continue;

		bool countryMatch	= (country == 0 || type->WeaponSpec.country() == country);
		bool branchMatch	= (branch == 0);

		if(branch > 0)
		{
			if(branch == 1 && (type->WeaponSpec.wCategory == 2 || type->WeaponSpec.wCategory == 4))
				branchMatch = true;
			else if(branch == 2 && type->WeaponSpec.wCategory == 1)
				branchMatch = true;
			else if(branch == 3 && type->WeaponSpec.wCategory == 3)
				branchMatch = true;
		}

		if(countryMatch && branchMatch)
			uiChatPrintLine("%s", type->Id);
	}

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
	uiChatPrintLine("\n");

	if(argc == 1)
	{
		uiChatPrintLine(HELP_HELP);

	}
	else
	{
		char* cmd = argv[1];
		int i;

		for(i = 0; INTERP_CMDS[i].name; i++)
		{
			if(!strcmp(INTERP_CMDS[i].name, cmd))
			{
				if(INTERP_CMDS[i].help)
				{
					uiChatPrintLine(INTERP_CMDS[i].help);
				}

				break;
			}
		}
	}

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
	interp_range(argv[1], start, end);

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

	interp_range(argv[1], start, end);

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
					if(vehtype->StateType[j].StateType == VehAnim::STATE_TYPE_VISDAMAGE
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
	interp_range(argv[1], start, end);

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

	interp_range(argv[1], start, end);

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

	interp_range(argv[1], start, end);

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
	pos_cmd
*************************************************************************************************/

static int pos_cmd(int argc, char* argv[])
{
	FLOAT32 x = (FLOAT32)atoF(argv[1]);
	FLOAT32 y = (FLOAT32)atoF(argv[2]);
	FLOAT32 z = (FLOAT32)atoF(argv[3]);

	uiChatPrintLine("Setting spawn position to %.2f,%.2f,%.2f", x, y, z);

	multiClientSetStartPosition(x, y, z);

	return 0;
}

/************************************************************************************************
	ticket_cmd
*************************************************************************************************/

static int ticket_cmd(int argc, char* argv[])
{
	if(argc < 1)
		return 0;

	SINT8* ptr, tktApp[64], tktData[64], tktIP[64], tktPort[64], tktProtocol[64], tktOTP[64], tktBypass[64];

	FILE* stream = fopen(argv[1], "r");
	if (stream == nullptr)
		return -1;

	do
	{
		if (fgets(tktApp, 64, stream) == nullptr)
		{
			uiChatPrintLine("Error when reading apID");
			break;
		}

		if (fgets(tktData, 64, stream) == nullptr)
		{
			uiChatPrintLine("Error when reading data");
			break;
		}


		if (fgets(tktIP, 64, stream) == nullptr)
		{
			uiChatPrintLine("Error when reading serverIP");
			break;
		}

		// tktIP has a label on the front and a newline on the end - need to remove both
		std::string z7 = tktIP;
        size_t labelSep = z7.find(' ');
        // ensure a separator was found and there is enough space for a value
        if ( (labelSep != std::string::npos) && ((labelSep+2) < z7.length()) )
        {
            z7.erase(0, labelSep + 1);				//Remove the label
            z7.erase(z7.length() - 1, z7.length());		//delete the end of the string (newline)

            if (teulGetHost(z7.c_str(), &host) == 0)
            {
                uiChatPrintLine("Unable to translate host address");
                break;
            }
        }
        else
        {
            uiChatPrintLine("Error when parsing serverIP");
            break;
        }
        
		if (fgets(tktPort, 64, stream) == nullptr)
		{
			uiChatPrintLine("Error when reading port");
			break;
		}

		ptr = strchr(tktPort, ' ');
		if (ptr == nullptr)
		{
			uiChatPrintLine("Could not parse port");
			break;
		}

		ptr += 1;
		port = (UINT16)strtoul(ptr, nullptr, 10);

		if (fgets(tktProtocol, 64, stream) == nullptr)
		{
			uiChatPrintLine("Error when reading protocol");
			break;
		}

		ptr = strchr(tktProtocol, ' ');
		if (ptr == nullptr)
		{
			uiChatPrintLine("Could not parse port");
			break;
		}

		ptr += 1;
		protocol = (UINT16)strtoul(ptr, nullptr, 10);

		if (fgets(tktOTP, 64, stream) == nullptr)
		{
			uiChatPrintLine("Error when reading OTP");
			break;
		}

		ptr = strchr(tktOTP, ' ');
		if (ptr == nullptr)
		{
			uiChatPrintLine("Could not parse OTP");
			break;
		}

		ptr += 1;
		otp = strtoul(ptr, nullptr, 10);

		if (fgets(tktBypass, 64, stream) == nullptr)
		{
			uiChatPrintLine("Error when reading starting playerid");
			break;
		}

		ptr = strchr(tktBypass, ' ');
		if (ptr == nullptr)
		{
			uiChatPrintLine("Could not parse starting playerid");
			break;
		}

		ptr += 1;
		playeridStart = atoi(ptr);
	}
	while (FALSE);

	fclose(stream);

	if(host)
		uiChatPrintLine("Current Ticket Info: %s with OTP: %u Start PID: %d", tktIP, otp, playeridStart);

	return 0;
}

/************************************************************************************************
	interp_range
*************************************************************************************************/

static void interp_range(char* s, int& start, int& end)
{
	if(strstr(s, "-"))
	{
		sscanf(s, "%d-%d", &start, &end);
	}
	else
	{
		start = atoi(s);
		end = start;
	}

	/*
	s = strtok(s, "-");
	*start = s ? atoi(s) : 0;

	s = strtok(nullptr, "-");
	*end = s ? atoi(s) : *start;
	*/
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
		}
		else
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
	for(i = 0; INTERP_CMDS[i].name; i++)
	{
		if (!strcmp(cmdName, INTERP_CMDS[i].name))
		{
			return INTERP_CMDS[i].cmd;
		}
	}

	return nullptr;
}

/************************************************************************************************
	lookup_interp_cmd_lookup
*************************************************************************************************/

static interp_cmd_lookup& lookup_interp_cmd_lookup(const char* cmdName)
{
	for(int i = 0; INTERP_CMDS[i].name; i++)
	{
		if (!strcmp(cmdName, INTERP_CMDS[i].name))
			return INTERP_CMDS[i];
	}

	static interp_cmd_lookup dummy = { nullptr, nullptr, nullptr, false };

	return dummy;
}

int multiClientExecuteCommand(char* line, bool checkQ)
{
	int argc;
	char* argv[10];

	argc = interp_parse_args(line, argv, 10);

	return multiClientExecuteCommand(argc, argv, checkQ);
}

int multiClientExecuteCommand(int argc, char* argv[], bool checkQ)
{
	int err = -1;
	if(argc && argv[0])
	{
		interp_cmd cmd = lookup_interp_cmd(argv[0]);

		if(cmd)
		{
			if(!checkQ || !queueCmd(argc, argv))
				cmd(argc, argv);
		}
		else
		{
			uiChatPrintLine("unknown command");
		}
	}
	return err;
}

void multiClientSetPlayers(std::vector<int> p)
{
	playeridStart = -1; // means use the player list
	players.swap(p);
}

void multiClientSetPlayerIdStart(int id)
{
	playeridStart = id;
}

void multiClientSetHost(const char* s)
{
	teulGetHost(s, &host);
}

static int host_cmd(int argc, char* argv[])
{
	if(argc > 1)
		multiClientSetHost(argv[1]);

	return -1;
}

static int playerid_cmd(int argc, char* argv[])
{
	if(argc > 1)
		multiClientSetPlayerIdStart(atoi(argv[1]));

	return -1;
}

static int wantchat_cmd(int argc, char* argv[])
{
	if(argc > 1)
	{
		wantChat = (strcasecmp(argv[1], "true") == 0);
		uiChatPrintLine("Set wantChat to %s", (wantChat) ? "true" : "false");
	}

	return -1;
}

static int chatpctg_cmd(int argc, char* argv[])
{
	if(argc > 1)
	{
		chatPercentage = (float)atoF(argv[1]);

		if(chatPercentage < 0)
			chatPercentage = 0;
		else if(chatPercentage > 1.0f)
			chatPercentage = 1.0f;

		uiChatPrintLine("Set chatPercentage to %.2f", chatPercentage);
	}

	return -1;
}

void multiClientSetStartPosition(float x, float y, float z)
{
	_startx = x; _starty = y; _startz = z;
}





//The MultiClient Class
void MultiClient::MoveTo(float x,float y,float z)
{
	_x = x; _y = y; _z = z;
}

void MultiClient::MoveBy(float x,float y,float z)
{
	_x += x; _y += y; _z += z;
}

void MultiClient::Update()
{
	unsigned t = ww2::Client::NetTime();
	float dt = (float)(t - _lastupdate) / 1000.0f;
	_lastupdate = t;

	_x += _vx * dt;
	_y += _vy * dt;
	_z += _vz * dt;

	SetVelocity(_vx,_vy,_vz);
	SetAttitude(_pitch, _roll, _yaw);
	SetPosition(_x,_y,_z);
	ww2::Client::Update();
}

MultiClient::MultiClient()
{
	_x = _y = _z = 0;
	_vx = _vy = _vz = 0;
	_pitch = _roll = _yaw = 0;
	_lastupdate = 0;
	SetAutoReconnect(true);		//orig false
}

MultiClient* get_client(int i)
{
	ClientMap::iterator it = clients.find(i);
	if(it == clients.end()) return 0;

	return it->second;
}

void MultiClient::RotateTo(float pitch, float roll, float yaw)
{
	_pitch = (float)-DEG_TO_RAD(pitch);
	_roll = (float)DEG_TO_RAD(roll);
	_yaw = (float)DEG_TO_RAD(yaw);
}

void MultiClient::RotateToRadians(float pitch, float roll, float yaw)
{
	_pitch = pitch;
	_roll = roll;
	_yaw = yaw;
}

void MultiClient::RotateBy(float pitch, float roll, float yaw)
{
	_pitch += (float)-DEG_TO_RAD(pitch);
	_roll += (float)DEG_TO_RAD(roll);
	_yaw += (float)DEG_TO_RAD(yaw);
}

void MultiClient::SetSpeed(float speed)
{
	float x,y,z;
	GetDirection(x, y, z);
	SetVel(speed*x, speed*y, speed*z);
}

void MultiClient::AddSpeed(float speed)
{
	float x,y,z;
	GetDirection(x, y, z);
	AddVel(speed*x, speed*y, speed*z);
}

void MultiClient::SetVel(float vx, float vy, float vz)
{
	_vx = vx; _vy = vy; _vz = vz;
}

void MultiClient::AddVel(float vx, float vy, float vz)
{
	_vx += vx; _vy += vy; _vz += vz;
}

void MultiClient::GetDirection(float& x, float& y, float& z)
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

void MultiClient::GetPosition(float& x,float& y,float& z)
{
	x = _x; y = _y; z = _z;
}

#endif

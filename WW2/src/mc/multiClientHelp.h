#define SPAWN_HELP	\
"Usage: spawn <VEHIDX> <VEHICLETYPE>\n"\
"VEHIDX is 0-based index of vehicles. Use START-END for a range of clients.\n" \
"VEHICLETYPE is the name of the vehicle type. Use the weaplist command to get a possible vehicle types.\n" \
"Spawns vehicle(s) into the world at whatever position was set before this command was run.\n"

#define DESPAWN_HELP \
"Usage: despawn <VEHIDX>.\n" \
"VEHIDX is 0-based index of vehicles. Use START-END for a range of clients.\n"\
"Despawns vehicle(s) out of the world.\n"

#define MOVE_HELP \
"Usage: move <VEHIDX> <POSMODE> <X> <Y> <Z>\n" \
"VEHIDX is 0-based index of vehicles. Use START-END for a range of clients.\n" \
"POSMODE can be either 'rel' or 'abs'. 'rel' specifies position relative to current position. 'abs' is absolute world position.\n"\
"Move vehicle(s).\n"

#define ROTATE_HELP	\
"Usage: rotate <VEHIDX> <POSMODE> <PITCH> <ROLL> <YAW>\n" \
"VEHIDX is 0-based index of vehicles. Use START-END for a range of clients.\n" \
"POSMODE can be either 'rel' or 'abs'. 'rel' specifies rotation relative to current position. 'abs' is absolute world orientation.\n"\
"Rotates vehicles(s) about specific axis. Units are in degrees.\n"

#define VEL_HELP \
"Usage: rotate <VEHIDX> <POSMODE> <X> <Y> <Z>\n" \
"VEHIDX is 0-based index of vehicles. Use START-END for a range of clients.\n" \
"POSMODE can be either 'rel' or 'abs'. 'rel' specifies velocity relative to current velocity. 'abs' is an absolute velocity.\n"\
"Set vehicle(s) velocity vector. Units are in meters per second.\n"

#define HOST_HELP \
"Usage: host <HOST_IP>\n" \
"Set the host IP to connect to. Must be the IP of the mmhostd server."

#define WANTCHAT_HELP \
"Usage: wantchat [true|false]\n" \
"Set whether you want each client to connect to the chat host or not."

#define CHATPCTG_HELP \
"Usage: chatpctg <PERCENT>\n" \
"PERCENT is a number between 0.0f and 1.0f.\n"\
"Its the percentage chance each frame that any particular client will send out a chat. 0 means no chance while 1 means 100% chance."

#define PLAYERID_HELP \
"Usage: playerid <PLAYER_ID>\n" \
"Set the start player id of first client.\n"\
"You must have a player for each contiguous playerid after the start playerid up to the max number of clients you want to handle.\n" \
"For example, if your start playerid was 2500 and you wanted to connect 1000 clients, you would need a player (and playerid) for each id between 2500 and 3500.\n" \
"In the multiclient these clients would be referenced by a 0 based index. 0-999 in the case above."

#define SPEED_HELP	\
"Usage: speed <VEHIDX> <POSMODE> <SPEED>\n" \
"VEHIDX is 0-based index of vehicles. Use START-END for a range of clients.\n" \
"POSMODE can be either 'rel' or 'abs'. 'rel' specifies speed relative to current speed. 'abs' is an absolute speed.\n"\
"Set vehicle(s) velocity along direction it is pointing. Units are in meters per second.\n"

#define CIRCLE_HELP	\
"Usage: circle <VEHIDX> <POSMODE> <CENTERX> <CENTERY> <CENTERZ> <RADIUS> <SPEED>\n" \
"VEHIDX is 0-based index of vehicles. Use START-END for a range of clients.\n" \
"POSMODE can be either 'rel' or 'abs'. 'rel' specifies position relative to current position. 'abs' is absolute world position.\n" \
"RADIUS and SPEED are for the* first* vehicle. All other vehicles will maintain same relative position with respect to first vehicle.\n" \
"Type circle VEHIDX with no args to stop vehicles.\n"\
"Set vehicle(s) to move in a circle.\n"

#define SCRIPT_HELP	\
"Usage: script <FILENAME>" \
"FILENAME is the name of the script to run. File must be in your ww2mods root directory.\n" \
"Script file can contain any command you can type interactively. Also supports # for comments.\n"

#define FORM_HELP \
"Usage: formation <VEHIDX> <FORMATION> [variable args]" \
"VEHIDX is 0-based index of vehices. Use START-END for a range of clients.\n" \
"FORMATION specifies type of formation. Choices are rank, file, wedge, circle, grid, missingman.\n" \
"type help formation FORMATION for details about each formation. (help not implemented yet)\n" \
"Sets vehicles to the specified formation.\n"

#define TURRET_HELP	\
"Move vehicle turrets to the specified position.\n"  \
"Usage: turret VEHIDX TURRETNUM TRAVERSE ELEVATION" \
"VEHIDX is 0-based index of vehicles (see spawn help).\n" \
"TURRETNUM is 0-based index specifing which turret to move.\n" \
"TRAVERSE is the horizontal position of turret in degrees.\n" \
"ELEVATION is the vertical position of turret in degrees.\n"

#define FIRE_HELP \
"Fires a vehicle weapon.\n"  \
"Usage: fire VEHIDX MOUNTNUM CONTINOUS" \
"VEHIDX is 0-based index of vehicles (see spawn help).\n" \
"MOUNTNUM is 0-based index specifing which weapon mount to fire.\n" \
"CONTINUOUS is 1 or 0 specifying where you want to weapon to keep firing or just fire once.\n"

#define STATE_HELP \
"Sets the various states of a vehicle.\n"  \
"Usage: fire VEHIDX STATENAME ONOFF" \
"VEHIDX is 0-based index of vehicles (see spawn help).\n" \
"STATENAME is the name of state you want to change. you can use command completion to get a full list of possible states.\n" \
"ONOFF specifies the state, either on or off.\n"

#define VISBIT_HELP \
"Not implemented yet.\n"

#define KILL_HELP \
"Usage: kill\n" \
"Kills all currently connected clients."

#define CLEARQ_HELP \
"Usage: clearq\n" \
"Clears all queued commands."

#define WEAPLIST_HELP \
"Usage: weaplist <COUNTRY> <BRANCH>\n" \
"COUNTRY is the persona country id (1 = UK, 3 = FR, 4 = DE).\n" \
"BRANCH is the persona branch (1 = ARMY, 2 = AIRFORCE, 3 = NAVY\n"

#define HELP_HELP \
"List of commands. Type \"help <COMMAND>\" for help with particular command.\n" \
"\n" \
"spawn -- Spawns vehicle(s)\n" \
"despawn -- Despawns vehicle(s)\n"  \
"move -- Moves vehicle(s).\n" \
"rotates -- Rotates vehicle(s)\n" \
"velocity -- Sets vehicle(s) velocity vector.\n" \
"speed -- Set vehicle(s) velocity along direction it is pointing.\n"  \
"circle -- Sets vehicles(s) to move in a circle.\n"  \
"script -- Runs a simple playback script file.\n" \
"formation -- Sets vehicles to the specified formation.\n" \
"kill -- Exits ww2client.\n" \
"pos -- Set the start spawn position.\n" \
"ticket -- Set the auth bypass ticket to use.\n" \
"login -- Login 1 or more clients\n" \
"list -- List information about 1 or more clients.\n" \
"help -- This command.\n" \
"\n"

/*
"weaplist -- Gets weapon list for specified facility.\n" \
"turret -- Moves turret.\n" \
"fire -- Fires weapon mount.\n" \
"state -- Sets various state bits.\n" \
"visbit -- Set vehicle(s) visbits. (not implemented yet)\n" \
*/

#define WW2_CLIENT_HELP \
"This is the World War 2 Online commmand line client. Usage:\n" \
"\n" \
"    ww2client [options]\n" \
"\n" \
"Options:\n" \
"\n" \
"  -ip\t\tIP address (x.x.x.x) of game host to connect to. Default is " DEFAULT_HOST_IP ".\n"\
"  -otp\t\tOne Time Password. Default is %d.\n" \
"  -playerid\tStart player id. Default is %d. Can be overridden by PLAYERID environment variable.\n" \
"  -allies\tNumber of allied players to login. Default is one.\n" \
"  -axis\t\tNumber of axis players it login. Default is none.\n" \
"  -version\tGame version (x.x.x.x) of host. Default is " DEFAULT_APP_VERSION_STR ".\n"\
"  -port\t\tHost port to connect to. Default is %d.\n" \
"  -script [filename] Run playback script.\n" \
"  -data\t\tSet data directory. Default is data.\n" \
"  -help\t\tPrint this message.\n" \
"\n" \
"You should login with no more than 10 vehicles at a time, " \
"as I have setup everyone with a unique PLAYERID environment variable in blocks of 10.\n"

#define FPS_HELP	\
"Sets FPS animation bits\n" \
"Usage: fps VEHIDX [weap #/sit/crouch/prone/stand/sprint/trot/back/idle/relax/down/right/left/ahead/alive/death #]\n"

#define VIS_HELP	\
"visdamage\n"

#define TICKET_HELP "Usage: ticket <TICKET_FILE>\n" \
"TICKET_FILE is the name of a ticket file to use for each client's auth bypass."

#define POS_HELP "Usage: pos <X> <Y> <Z>\n" \
"Set the start spawn position in absolute x,y,z coordinates.\n" \
"Use the /whereami command to find world coordinates."

#define LOGIN_HELP \
"Usage: login <VEHIDX> <COUNTRY> <BRANCH>\n" \
"VEHIDX is 0-based index of vehicles. Use START-END for a range of clients.\n" \
"COUNTRY is the start persona country id (1 = UK, 3 = FR, 4 = DE).\n" \
"BRANCH is the start persona branch (1 = ARMY, 2 = AIRFORCE, 3 = NAVY\n"

#define LIST_HELP \
"Usage: list <VEHIDX>\n" \
"VEHIDX is 0-based index of vehicles. Use START-END for a range of clients.\n" \
"Lists the online status of each of the clients requested."

#define DATA_HELP \
"Usage: data <VEHIDX>\n" \
"VEHIDX is 0-based index of vehicles. Use START-END for a range of clients.\n " \
"Displays the position, velocity and attitude of the clients requested."
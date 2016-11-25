#! /usr/bin/python

from __future__ import print_function, absolute_import

from lib.helpers import *
from lib.mysqlcli import MySQLCli
import os
import shutil
import socket
from subprocess import check_call

DB_IMPORT_FILE = pathname("Database", "mysqldb.sql")
PLAYNET_PATH = "%splaynet" % (ROOT_PATH)
WWIIOL_PATH = pathname(PLAYNET_PATH, "wwiiol")
DATA_PATH = pathname(WWIIOL_PATH, "data")
STRAT_LOADER = "stratLoader%s" % (EXE_SFX)
CLIENT_PATH = pathname(".", "Client")

BASE_DEV_ID = 2000
BASE_USR_ID = 2100

def source_folder(purpose, default_path, filename):
    if os.path.exists(default_path):
        if os.path.exists(pathname(default_path, filename)):
            return default_path

    note("Path required for %s." % (purpose))
    info("Please provide the location of the %s. This directory should "
            "contain '%s'." % (purpose, filename))

    while True:
        new_path = input("%s path: " % purpose)
        if not new_path:
            die("Cannot continue without %s path - aborting." % (purpose))
        if not os.path.exists(new_path):
            warn("No such directory '%s'." % (new_path))
            continue
        if not os.path.exists(pathname(new_path, filename)):
            warn("Path does not contain '%s'." % (filename))
            continue
        # otherwise, we found it, we're good to go.
        return new_path


def get_ip_address():
    # Create a socket aimed at the outside world, and then
    # see what address is assigned to the local endpoint
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("8.8.8.8", 80))  # google's web server
    return str(s.getsockname()[0])


def generate_tkt_and_bat(game_name, player_id, base_ip):
    tkt_file = '%s.tkt' % (game_name)
    if os.path.exists(CLIENT_PATH):
        tkt_file = pathname(CLIENT_PATH, tkt_file)
    with open(tkt_file, "w") as tktfh:
        tktfh.write(
            "apID: 12\n"
            "data: Login Accepted\n"
            "serverIP: %s\n"
            "port: 27015\n"
            "protocol: 2\n"
            "OTP: 2342426\n"
            "bypass: %s\n" % (base_ip, player_id)
        )

    bat_file = '%s.bat' % (game_name)
    with open(bat_file, "w") as batfh:
        batfh.write(" ".join([
            "WW2.exe",
                "-ticket", tkt_file,
                "-window", "1",
                "-width", "800",
                "-height", "640",
                "-log", "0",
                "-data", "1",
                "-datadir", DATA_PATH + PATH_SEP
        ]))


def main():
    print(
        "----------------------------------------\n"
        "Playnet Cluster-in-a-box DB Bootstrapper\n"
        "----------------------------------------\n"
        "\n"
        "NOTE: This tool is only intended for populating a blank database.\n"
        "NOTE: Requires MySQL server installed and running on this machine.\n"
        "\n"
    )

    step("Checking for required files.")
    if not os.path.exists(DB_IMPORT_FILE):
        die("Missing '%s' file." % DB_IMPORT_FILE)
    if not os.path.exists(PLAYNET_PATH):
        die("Missing '%s' directory." % PLAYNET_PATH)
    try:
        # Cheezy hax: make sure the name is lower case.
        os.rename(pathname(WWIIOL_PATH, "DATA"), DATA_PATH)
    except:
        pass
    if not os.path.exists(DATA_PATH):
        die("Missing '%s' directory (populate with WW2Mods%sDATA)." % (
                DATA_PATH, PATH_SEP))
    if not os.path.exists(pathname(DATA_PATH, "blitzWor.csv")):
        die("No 'blitzWor.csv' in %s." % (DATA_PATH))
    if not os.path.exists(pathname(DATA_PATH, "terrain")):
        die("No 'terrain' directory in %s." % (DATA_PATH))
    bin_path = source_folder("Server Binaries", pathname(WWIIOL_PATH, "bin"),
            STRAT_LOADER)
    strat_loader = pathname(bin_path, STRAT_LOADER)
    lua_path = source_folder("Lua Scripts", "Lua", "rc.lua")

    step("Checking database access.")
    note("DATABASE ROOT PASSWORD:")
    info("Please provide the 'root' password you specified for the local db.")
    root_pw = input("Password: ")
    if not root_pw:
        die("No password - aborting.")
    print()
    cli = MySQLCli("root", root_pw, "127.0.0.1")

    step("Checking database is not populated.")
    result = cli.execute("SELECT COUNT(*) FROM information_schema.tables"
            " WHERE table_schema = 'wwiionline'")
    if int(result) > 0:
        die("Database already contains a populated `wwiionline` schema.")

    step("Populating database.")
    cli.execute_script(DB_IMPORT_FILE)

    step("Generating environment.cfg.")
    cfg_file = pathname(PLAYNET_PATH, "etc", "environment.cfg")
    base_ip = get_ip_address()
    with open(cfg_file, "w") as envfh:
        envfh.write(
            "WWII_CLUSTER=ciab\n"
            "APP_ID=12\n"
            "ARENA_ID=250\n"
            "STRAT_LANGUAGE=en\n"
            "LOG_PATH=%s\n"
            "DATA_DIR=%s\n"
            "LUA_BASE_PATH=%s\n"
            "TEUL_UWEB_PATH=%s\n"
            "BASE_ADDRESS=%s\n"
            "BASE_UPDT_ADDRESS=0.0.0.0\n"
            "WWII_DB_ADDRESS=127.0.0.1\n"
            "WWII_DB_SCHEMA=wwiionline\n"
            "WWII_DB_PASSWD=freebird\n"
            "DBSE_PRIMARY_ADDRESS=localhost\n"
            "DBSE_CONNECTION_TIMEOUT_MS=60000\n"
            "PEER_CONNECTION_TIMEOUT_MS=60000\n"
            "MANAGE_PROCESSES=dbd;strat;chat;map;cell\n"
                % (
                    pathname(PLAYNET_PATH, "tmp"),
                    DATA_PATH,
                    pathname(WWIIOL_PATH, "lua"),
                    pathname(WWIIOL_PATH, "web", "debug"),
                    base_ip,
                ))

    step("Importing terrain/strat data.")
    try:
        check_call([strat_loader, '-C', DATA_PATH])
    except:
        die("Unable to import terrain/strat data.")

    print()

    step("Copying Lua scripts.")
    lua_dest = pathname(WWIIOL_PATH, "lua")
    for script in os.listdir(lua_path):
        shutil.copy(pathname(lua_path, script), lua_dest)

    step("Generating .tkt and .bat files for test accounts.")
    for i in range(1, 6):
        for kind in ('rat', 'plyr'):
            name, pid = "%s%04dp" % (kind, i), BASE_DEV_ID + i
            generate_tkt_and_bat(name, pid, base_ip)

    step("Complete.")

if __name__ == '__main__':
    main()
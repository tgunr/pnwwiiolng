#! /usr/bin/python

from __future__ import print_function, absolute_import

from lib.helpers import *
from lib.mysqlcli import MySQLCli
import os
from subprocess import check_call

DB_IMPORT_FILE = "mysqldb.sql"
DEFAULT_CUSTOMER_ID = 1581
PLAYNET_PATH = "%splaynet" % (ROOT_PATH)
WWIIOL_PATH = pathname(PLAYNET_PATH, "wwiiol")
DATA_PATH = pathname(WWIIOL_PATH, "data")
STRAT_LOADER = "stratLoader%s" % (EXE_SFX)

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
    if not os.path.exists(DATA_PATH):
        die("Missing '%s' directory (populate with WW2Mods%sDATA." % (
                DATA_PATH, PATH_SEP))
    if not os.path.exists(pathname(DATA_PATH, "blitzWor.csv")):
        die("No 'blitzWor.csv' in %s." % (DATA_PATH))
    if not os.path.exists(pathname(DATA_PATH, "terrain")):
        die("No 'terrain' directory in %s." % (DATA_PATH))

    bin_path = pathname(WWIIOL_PATH, "bin")
    strat_loader = pathname(bin_path, STRAT_LOADER)
    if not os.path.exists(strat_loader):
        note("Missing '%s'" % (strat_loader))
        info("Please provide the location of the server binaries. This"
                " directory/folder should contain '%s'." % (STRAT_LOADER))
        while True:
            bin_path = input("Binary directory: ")
            if not bin_path:
                die("Cannot continue without access to binaries.")
            if not os.path.exists(bin_path):
                warn("No such directory.")
                continue
            strat_loader = pathname(bin_path, STRAT_LOADER)
            if not os.path.exists(strat_loader):
                warn("Could not find '%s', check directory and try again." % (
                        strat_loader))
                continue
            # found it.
            break

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
            "BASE_ADDRESS=127.0.0.1\n"
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
                    pathname(WWIIOL_PATH, "web", "debug")
                ))

    step("Importing terrain/strat data.")
    try:
        check_call([strat_loader, '-C', DATA_PATH])
    except:
        die("Unable to import terrain/strat data.")

    step("Querying account data.")
    account_name = cli.execute("SELECT `username` FROM `auth`.`auth_customer`"
            " WHERE `customerID` = %s" % (DEFAULT_CUSTOMER_ID))
    if not account_name:
        die("Account #%s not found in the database." % (DEFAULT_CUSTOMER_ID))

    print()

    note("DESIRED ACCOUNT NAME:")
    info("If the account does not exist, the existing account `%s` (#%s) "
            "will be replaced with this name." % (account_name,
            DEFAULT_CUSTOMER_ID))
    info("Leave blank to keep '%s'." % (account_name))
    new_name = input("Account name: ")
    if new_name:
        # Check if it exists.
        existing = cli.execute(
                "SELECT `customerID` FROM `auth`.`auth_customer` WHERE"
                    "`username` = '%s'" % (new_name))
        if existing:
            customer_id = existing
            info("Account exists (a:%s)" % (customer_id))
        else:
            cli.execute("UPDATE `auth`.`auth_customer` SET `username` = '%s', "
                    " `handle` = `%s` WHERE `customerID` = `%s`" % (
                        new_name, new_name, DEFAULT_CUSTOMER_ID, account_name))
            customer_id = DEFAULT_CUSTOMER_ID
        account_name = new_name

    # Look up the game id and game name
    results = cli.execute("SELECT `playerid`, `callsign`"
            " FROM `wwiionline`.`wwii_player` WHERE `customerid` = %s" % (
                customer_id))
    if not results:
        die("No player data found for account #%s." % (DEFAULT_CUSTOMER_ID))
    player_id, game_name = results.split()

    print()

    note("DESIRED GAME-NAME:")
    info("The existing game-name of this account is '%s' (#%s)." % (game_name,
            player_id))
    info("Leave blank to keep '%s'." % (game_name))
    new_name = input("Game-name: ")
    if new_name:
        cli.execute("UPDATE `wwiionline`.`wwii_player` SET `callsign` = '%s'"
                " WHERE `playerid` = %s" % (new_name, player_id))
        game_name = new_name

    info("Account name: %s" % account_name)
    info("Account ID  : %s" % customer_id)
    info("Game-name   : %s" % game_name)
    info("Player ID   : %s" % player_id)

    tkt_file = '%s.tkt' % (game_name)
    step("Generating ticket file.")

    with open(tkt_file, "w") as tktfh:
        tktfh.write(
            "apID: 12\n"
            "data: Login Accepted\n"
            "serverIP: 127.0.0.1\n"
            "port: 27015\n"
            "protocol: 2\n"
            "OTP: 2342426\n"
            "bypass: %s\n" % (player_id)
        )

    note("Ticket file is: %s%s%s" % (
            os.path.dirname(os.path.realpath(tkt_file)), PATH_SEP, tkt_file))

    step("Complete.")

if __name__ == '__main__':
    main()
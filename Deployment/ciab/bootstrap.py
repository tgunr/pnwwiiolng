#! /usr/bin/python

from __future__ import print_function, absolute_import

from lib.defaults import *
from lib.helpers import *
from lib.inifile import Config
from lib.mysqlcli import MySQLCli, MySQLCliError
import os
import shutil
import sys
from subprocess import check_call


def generate_tkt_and_bat(cfg, game_name, player_id):
    tkt_name = '%s.tkt' % (game_name)
    tkt_file = tkt_name
    if os.path.exists(CLIENT_PATH):
        tkt_file = pathname(CLIENT_PATH, tkt_file)
    with open(tkt_file, "w") as tktfh:
        tktfh.write(
            "apID: %s\n"
            "data: Login Accepted\n"
            "serverIP: %s\n"
            "port: 27015\n"
            "protocol: 2\n"
            "OTP: 2342426\n"
            "bypass: %s\n" % (cfg.app_id, cfg.base_address, player_id)
        )
	
    if IS_WIN:
        bat_file = '%s.bat' % (game_name)
        if os.path.exists(CLIENT_PATH):
            bat_file = pathname(CLIENT_PATH, bat_file)
        with open(bat_file, "w") as batfh:
            batfh.write(" ".join([
                "WW2.exe",
                    "-ticket", tkt_name,
                    "-window", "1",
                    "-width", "800",
                    "-height", "640",
                    "-log", "0",
                    "-data", "1",
                    "-datadir", cfg.data_path
            ]))
    else:
        sh_file = '%s.sh' % (game_name)
        if os.path.exists(CLIENT_PATH):
            sh_file = pathname(CLIENT_PATH, sh_file)
        with open(sh_file, "w") as batfh:
            batfh.write(" ".join([
                "open", "-a", "'World War II Online'", "--args",
                    "-ticket", tkt_name,
                    "-window", "1",
                    "-width", "800",
                    "-height", "640",
                    "-log", "0",
                    "-data", "1",
                    "-datadir", cfg.data_path
            ]))


class ConfigError(Exception):
    pass

class MissingPathError(ConfigError):
    pass


def verify_path(cfg, entry, contents, subdir=None):
    path = getattr(cfg, entry, None)
    if not path:
        raise MissingPathError("Missing setting for '%s'." % entry)
    if not os.path.exists(path):
        raise MissingPathError("%s: No such directory/access: '%s'." % (entry, 
                            path))
    if subdir:
        path = pathname(path, subdir)
        if not os.path.exists(path):
            raise MissingPathError("%s: Missing sub-directory/access: '%s'." % (
                                entry, path))
    for child in contents:
        if not os.path.exists(pathname(path, child)):
            raise MissingPathError("Missing '%s' in %s %s '%s'" % (child, entry,
                'directory' if child.endswith('/') else 'file', path))


def reject_same_path(cfg, purpose):
    src_path = os.path.abspath(getattr(cfg, purpose+'_origin_path'))
    dst_path = os.path.abspath(getattr(cfg, purpose+'_path'))
    if src_path == dst_path:
        raise ConfigError("'%s_origin_path' and '%s_path' point to the same"
                    " directory ('%s')." % (purpose, purpose, src_path))


def test_wwiiol_copy_target(cfg, subdir, contents):
    """
        Determines if we are using an existing install or whether we need
        to copy files to a wwiiol/ subdir and if the required source files
        are available to us.
        Returns True if we need to copy files, otherwise False.
    """

    s = Step('Checking %s directory(s).' % subdir)
    entry = '%s_origin_path' % subdir
    source_path = getattr(cfg, entry, None)
    if not source_path:
        note("'%s' is unset, assuming existing '%s' install." % (entry, subdir))
        verify_path(cfg, 'wwiiol_path', contents, subdir=subdir)
        info("'%s' appears to be present and populated." % (
                pathname(cfg.wwiiol_path, subdir)))
        return False

    # when a source path is set, we're expecting to have to install.
    # check the install directory looks good.
    verify_path(cfg, entry, contents)

    # but lets check if we'd be overwriting.
    try:
        # if this throws, the destination path isn't populated.
        verify_path(cfg, 'wwiiol_path', contents, subdir=subdir)
        q = "'%s' appears already populated but '%s' is set." % (
                pathname(cfg.wwiiol_path, subdir), entry)
        overwrite = question(q, "Overwrite?", [ 'Y', 'n' ])
        return overwrite.lower() == 'y'
    except MissingPathError:
        return True


def verify_data_directory(cfg):
    """ Checks the directory exists or populates it from data_origin_path. """
    cfg.xfer_data = test_wwiiol_copy_target(cfg, 'data', [ 'blitzWor.csv',
                               'terrain/', 'th_blitz/' ])
    if cfg.xfer_data:
        reject_same_path(cfg, 'data')
        if not cfg.data_transfer:
            raise ConfigError("No 'data_transfer' value set.")
        cfg.data_transfer = cfg.data_transfer.lower()
        if not cfg.data_transfer in CONFIG_XFER_OPTIONS:
            m = "'data_transfer' ('%s') must be one of: %s" % (
                    cfg.data_transfer, ', '.join(CONFIG_XFER_OPTIONS))
            raise ConfigError(m)
        info("Data will be '%s'd" % cfg.data_transfer)


def verify_bin_directories(cfg):
    """ Test for executables and whether we want to copy them. """
    cfg.bin_path = pathname(cfg.wwiiol_path, 'bin')
    cfg.xfer_exes = test_wwiiol_copy_target(cfg, 'bin', [
                            'controller' + EXE_SFX, STRAT_LOADER ])
    if cfg.xfer_exes:
        reject_same_path(cfg, 'bin')


def verify_lua_directory(cfg):
    """ Test for lua scripts and whether we want to copy them. """
    cfg.xfer_lua = test_wwiiol_copy_target(cfg, 'lua', [ 'rc.lua',
                            'stratlib.lua' ])
    if cfg.xfer_lua:
        reject_same_path(cfg, 'lua')


def verify_playnet_path(cfg):
    s = Step('Checking %s.' % cfg.playnet_path)
    verify_path(cfg, 'playnet_path', [ 'etc/' ])


def verify_wwiiol_path(cfg):
    s = Step('Checking %s.' % cfg.wwiiol_path)
    verify_path(cfg, 'wwiiol_path', [ 'bin/' ])
    verify_data_directory(cfg)
    verify_bin_directories(cfg)
    verify_lua_directory(cfg)


def verify_database_access(cfg):
    s = Step("Checking database access.")
    try:
        cli = MySQLCli(cfg.db_admin_user, cfg.db_admin_pass, cfg.db_host)
    except MySQLCliError as e:
        raise ConfigError(str(e))

    cfg.populate_db = True
    cfg.import_strat = True
    result = cli.execute("SELECT COUNT(*) FROM information_schema.tables"
            " WHERE table_schema = 'wwiionline' AND table_name = 'strat_cp'")
    if int(result) > 0:
        move = question("'%s' already appears to contain a populated"
                    " `wwiionline` schema. " % (cfg.db_host),
                    "(R)e-populate, (u)se existing data or (a)bort?",
                    [ 'R', 'u', 'a' ])
        if move == 'a':
            sys.exit(1)
        elif move == 'u':
            cfg.populate_db = False

    if not cfg.populate_db:
        # Check that the game user information they gave us works.
        ss = Step("Checking database access with game account.")
        try:
            usercli = MySQLCli(cfg.db_game_user, cfg.db_game_pass, cfg.db_host)
        except MySQLCliError as e:
            raise ConfigError(str(e))

        result = cli.execute("SELECT COUNT(*) FROM `wwiionline`.`strat_cp`")
        if int(result) > 0:
            move = question("Do you want to continue using the existing"
                        " strat data or re-build it from the SDM files?",
                        "Rebuild db strat data?", [ 'N', 'y' ])
            if move == 'n':
                cfg.import_strat = False


def verify_config(cfg):
    s = Step("Verifying config.")
    if not os.path.exists(cfg.db_import_file):
        raise MissingPathError("Missing '%s' file." % cfg.db_import_file)
    verify_playnet_path(cfg)
    verify_wwiiol_path(cfg)
    verify_database_access(cfg)


def create_user(cli, user, passwd):
    default_privs = [
        'ALTER', 'ALTER ROUTINE', 'CREATE', 'CREATE ROUTINE',
        'CREATE TEMPORARY TABLES', 'CREATE VIEW', 'DELETE', 'DROP', 'EVENT',
        'EXECUTE', 'INDEX', 'INSERT', 'LOCK TABLES', 'REFERENCES', 'SELECT',
        'SHOW DATABASES', 'SHOW VIEW', 'TRIGGER', 'UPDATE'
    ]
    def _generate_for_host(host, grant):
        cli.execute("DROP USER IF EXISTS '%s'@'%s';" % (user, host))
        cli.execute("CREATE USER '%s'@'%s' IDENTIFIED BY '%s'" % (user, host,
                passwd))
        privs = ','.join(default_privs)
        cli.execute("GRANT %s ON *.* TO '%s'@'%s' %s;" % (privs, user, host,
                grant))
    _generate_for_host('localhost', 'WITH GRANT OPTION')
    _generate_for_host('%', '')


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

    cfg = Config()

    while True:
        try:
            err = cfg.load()
            if err:
                raise ConfigError(err)
            verify_config(cfg)
            break
        except ConfigError as e:
            err = e
            pass
        retry = question(err, "Re-edit config and try again?", [ 'Y', 'n' ])
        if retry == 'n':
            sys.exit(1)

    step("Connecting to DB.")
    cli = MySQLCli(cfg.db_admin_user, cfg.db_admin_pass, cfg.db_host)

    if cfg.populate_db:
        step("Creating Game DB user.")
        create_user(cli, cfg.db_game_user, cfg.db_game_pass)

        step("Populating database.")
        cli.execute_script(DB_IMPORT_FILE)

    step("Generating environment.cfg.")
    cfg_file = pathname(PLAYNET_PATH, "etc", "environment.cfg")
    with open(cfg_file, "w") as envfh:
        envfh.write(
            "WWII_CLUSTER="+cfg.cluster_name+"\n"
            "APP_ID="+cfg.app_id+"\n"
            "ARENA_ID="+cfg.arena_id+"\n"
            "STRAT_LANGUAGE=en\n"
            "LOG_PATH="+cfg.log_path+"\n"
            "DATA_DIR="+cfg.data_path+"\n"
            "LUA_BASE_PATH="+cfg.lua_path+"\n"
            "TEUL_UWEB_PATH="+cfg.uweb_path+"\n"
            "BASE_ADDRESS="+cfg.base_address+"\n"
            "BASE_UPDT_ADDRESS=0.0.0.0\n"
            "WWII_DB_ADDRESS="+cfg.db_host+"\n"
            "WWII_DB_SCHEMA=wwiionline\n"
            "WWII_DB_USER="+cfg.db_game_user+"\n"
            "WWII_DB_PASSWD="+cfg.db_game_pass+"\n"
            "DBSE_PRIMARY_ADDRESS=localhost\n"
            "DBSE_CONNECTION_TIMEOUT_MS=60000\n"
            "PEER_CONNECTION_TIMEOUT_MS=60000\n"
            "MANAGE_PROCESSES=dbd;strat;chat;map;cell\n"
        )

    if cfg.xfer_exes:
        step("Copying executables.")
        shutil.rmtree(cfg.bin_path, ignore_errors=True)
        shutil.copytree(cfg.bin_origin_path, cfg.bin_path, symlinks=True)

    if cfg.xfer_lua:
        step("Copying Lua scripts.")
        shutil.rmtree(cfg.lua_path, ignore_errors=True)
        shutil.copytree(cfg.lua_origin_path, cfg.lua_path, symlinks=True)

    if cfg.xfer_data:
        step("'%s'ing data." % (cfg.data_transfer.title()))
        shutil.rmtree(cfg.data_path, ignore_errors=True)
        if cfg.data_transfer == 'copy':
            shutil.copytree(cfg.data_origin_path, cfg.data_path, symlinks=True)
        elif cfg.data_transfer == 'move':
            shutil.move(cfg.data_origin_path, cfg.data_path)
        elif cfg.data_transfer == 'link':
            os.symlink(cfg.data_origin_path, cfg.data_path)

    if cfg.import_strat:
        step("Importing terrain/strat data.")
        try:
            check_call([pathname(cfg.bin_path, STRAT_LOADER), '-C',
                    cfg.data_path])
        except:
            die("Unable to import terrain/strat data.")

    step("Generating .tkt and .bat files for test accounts.")
    for i in range(1, 6):
        for kind in ('rat', 'plyr'):
            name, pid = "%s%04dp" % (kind, i), BASE_DEV_ID + i
            generate_tkt_and_bat(cfg, name, pid)

    step("Complete.")

if __name__ == '__main__':
    try:
        main()
    except DieException as e:
        print(e.message)
        sys.exit(-1)

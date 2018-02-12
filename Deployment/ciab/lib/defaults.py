# Defaults for bootstrap.

from . helpers import *


DB_IMPORT_FILE = pathname("Database", "mysqldb.sql")
PLAYNET_PATH = "%splaynet" % (ROOT_PATH)
WWIIOL_PATH = pathname(PLAYNET_PATH, "wwiiol")
DATA_PATH = pathname(WWIIOL_PATH, "data")
STRAT_LOADER = "stratLoader%s" % (EXE_SFX)
CLIENT_PATH = pathname(".", "Client")

DB_ADMIN_USER = "root"
DB_ADMIN_PASS = ""

DB_GAME_USER = "wwiiol"
DB_GAME_PASS = "ciab4ever"

CONFIG_XFER_OPTIONS = [ 'copy', 'move' ]
if not IS_WIN:
	CONFIG_XFER_OPTIONS.append('link')

BASE_DEV_ID = 0
BASE_USR_ID = 0

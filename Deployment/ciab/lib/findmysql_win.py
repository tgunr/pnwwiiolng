# win32 implementation path for mysql.exe

import os.path
from winreg import *


MYSQL_EXE = "mysql.exe"


def _open_key_or_none(parent, key):
    try:
        return OpenKey(parent, key)
    except:
        return None

def _get_subkeys(key):
    for i in range(QueryInfoKey(key)[0]):
        yield EnumKey(key, i)

def _get_values(key):
    for i in range(QueryInfoKey(key)[1]):
        yield EnumValue(key, i)


def mysql_path_options():
    """ find locations for MYSQL_EXE on Windows platform """
    hklm = ConnectRegistry(None, HKEY_LOCAL_MACHINE)
    for swkey in (r"SOFTWARE", r"SOFTWARE\WOW6432Node"):
        mysqlKey = _open_key_or_none(hklm, r"%s\MySQL AB" % (swkey))
        if not mysqlKey:
            continue
        for appSubKey in _get_subkeys(mysqlKey):
            appKey = _open_key_or_none(mysqlKey, str(appSubKey))
            if appKey:
                for (name, value, typeId) in _get_values(appKey):
                    if name == 'Location' and typeId == 1:
                        path = os.path.expandvars(value)
                        yield os.path.expanduser(path)
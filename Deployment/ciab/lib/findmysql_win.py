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
    # Server and Client installs put their executable in a '\bin\'
    # subfolder, and we want to try and prefer this kind of install,
    # so we'll go out of our way to return those first.
    binPaths, nonbinPaths = [], []
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
                        nonbinPaths.append(path)
                        binPath = os.path.join(path, "bin")
                        if os.path.exists(binPath):
                            binPaths.append(binPath)
    if binPaths:
        yield from binPaths
    yield from nonbinPaths
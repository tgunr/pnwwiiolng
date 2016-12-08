#! env /usr/bin/python

from __future__ import print_function
from __future__ import absolute_import

import os

# Platform specific helpers
if os.name == 'nt':
    from . findmysql_win import mysql_path_options, MYSQL_EXE
else:
    from . findmysql_generic import mysql_path_options, MYSQL_EXE


def find_mysql_exe():
    """ Return an os-specific absolute path to a mysql executable. """
    for path in mysql_path_options():
        exe = os.path.join(path, MYSQL_EXE)
        if os.path.exists(exe):
            return exe
    return None


if __name__ == "__main__":
    """
        When launched directly, acts as a test for the presence of a
        suitable MySQL executable. If one is found, exit code is 0,
        otherwise exits with -1.
    """
    import sys
    mysql = find_mysql_exe()
    if not mysql:
        print("MySQL doesn't appear to be installed")
        sys.exit(-1)
    else:
        print("Found MySQL: %s" % (mysql))
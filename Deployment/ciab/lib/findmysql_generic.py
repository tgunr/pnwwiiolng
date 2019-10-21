# generic OS attempt to find mysql

import os.path

MYSQL_EXE = "mysql"

def mysql_path_options():
    """
        find locations for 'mysql' on an OS assumed to use a : separated
        environment variable called 'PATH'.
    """
    for path in os.getenv("PATH", '').split(':'):
        yield os.path.expanduser(path)

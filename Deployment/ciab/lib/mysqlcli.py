#! /usr/bin/python

# MySQL CLI manipulation functions

from __future__ import print_function, absolute_import

import os
import subprocess
from . helpers import die, DieException
from . findmysql import find_mysql_exe


DEFAULTS_FILE = 'mysql.defaults'


class _DefaultsFile(object):
    def __init__(self, user, pw, host):
        self._defaultsFile = None
        self._user, self._pw, self._host = user, pw, host

    def __enter__(self):
        defaultsFile = '%s.%s' % (DEFAULTS_FILE, os.getpid())
        with open(defaultsFile, "w") as deffh:
            deffh.write(
                "[client]\n"
                "user=%s\n"
                "password=%s\n"
                "host=%s\n"
                % (self._user, self._pw, self._host)
            )
            self._defaultsFile = defaultsFile
        return self._defaultsFile

    def __exit__(self, *args):
        if self._defaultsFile:
            os.remove(self._defaultsFile)
            self._defaultsFile = None

class MySQLCliError(Exception):
    pass

class MySQLCli(object):
    """
        Simple MySQL driver that uses per-action invocations of the
        command line utility, if one can be found, to execute SQL statements
        and return raw, undecorated output as text.
        Username, password and host are passed via a defaults file rather
        than on the command line.
    """
    def __init__(self, root_user, root_pw, host):
        """ Initialize the CLI driver. """
        self._mysql = find_mysql_exe()
        self._user, self._pass, self._host = root_user, root_pw, host
        if not self._mysql:
            die("Could not find a suitable mysql executable.")
        try:
            self.execute("SELECT NULL;")
        except Exception as e:
            raise MySQLCliError("Unable to access '%s' MySQL database as '%s'"
                " with the given password: %s." % (host, root_user, e)) from e

    def _run_cli_with_args(self, *args, **kwargs):
        """ Wrapper around the invocation of MySQL. """
        with _DefaultsFile(self._user, self._pass, self._host) as defaultsFile:
            cmd = [ self._mysql,
                    "--defaults-file=%s" % (defaultsFile),
                    "--batch", "--quick", "--unbuffered",
                    "-ANsr"  # no rehash, no header, no separators, raw output
            ]
            if args:
                cmd.extend(args)
            result = subprocess.check_output(cmd, **kwargs)
            return result.decode('ascii').strip()

    def execute_script(self, filename):
        """ effectively: mysql <filename """
        with open(filename, "r") as fh:
            return self._run_cli_with_args(stdin=fh)

    def execute(self, query):
        """ Returns the raw output from the specified SQL query. """
        return self._run_cli_with_args("-e", query)
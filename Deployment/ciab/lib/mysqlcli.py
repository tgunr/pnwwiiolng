#! /usr/bin/python

# MySQL CLI manipulation functions

from __future__ import print_function, absolute_import

import os
import subprocess
from . helpers import die


DEFAULTS_FILE = 'mysql.defaults'


class MySQLCli(object):
    def __init__(self, root_user, root_pw, host):
        """ Initialize the CLI driver. """
        with open(DEFAULTS_FILE, "w") as deffh:
            deffh.write(
                "[client]\n"
                "user=%s\n"
                "password=%s\n"
                "host=%s\n"
                % (root_user, root_pw, host)
            )
        self._open = True
        try:
            self.execute("SELECT NULL;")
        except:
            die("Unable to access MySQL database on '%s' as '%s' "
                "with the given password." % (host, root_user))

    def __del__(self):
        """ Cleanup - lets not leave the .defaults file around. """
        if self._open:
            os.remove(DEFAULTS_FILE)
            self._open = False

    def _run_cli_with_args(self, *args, **kwargs):
        """ Wrapper around the invocation of MySQL. """
        return subprocess.check_output([
            "mysql",
                "--defaults-file=%s" % (DEFAULTS_FILE),
                "--batch", "--quick", "--unbuffered",
                "-ANsr",  # no rehash, no header, no separators, raw output
                *args
        ], **kwargs).decode('ascii').strip()

    def execute_script(self, filename):
        """ effectively: mysql <filename """
        with open(filename, "r") as fh:
            return self._run_cli_with_args(stdin=fh)

    def execute(self, query):
        """ Returns the raw output from the specified SQL query. """
        return self._run_cli_with_args("-e", query)
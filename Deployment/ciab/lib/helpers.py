#! /usr/bin/python

from __future__ import print_function, absolute_import

import os
import sys

# Library functions

if os.name == 'nt':
    ROOT_PATH, PATH_SEP, EXE_SFX = "C:\\", "\\", ".exe"
else:
    ROOT_PATH, PATH_SEP, EXE_SFC = "/", "/", ""

def pathname(*args):
    return PATH_SEP.join(args)

def die(message):
    """ Exit, unsuccessfully, for a specified failure reason. """
    print("## ERROR: %s" % (message))
    sys.exit(1)

def warn(message):
    """ Present a non-terminal warning to the user. """
    print("#* WARNING: %s" % (message))

def note(message=""):
    """
        Present a minor but non-critical piece of information.
        May be surpressed at lower levels of verbosity reduction.
    """
    print("-- Note: %s" % (message))

def info(message=""):
    """
        Present an optional, minor piece of information to the user.
        Supressed at the first levels of verbosity reduction.
    """
    print("-- info: %s" % (message))

def step(message):
    """
        Minor annotation of progress.
    """
    print("- %s" % (message.upper()))

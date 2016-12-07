#! /usr/bin/python

from __future__ import print_function, absolute_import

import subprocess
import os


# Platform specifics

if os.name == 'nt':
    ROOT_PATH, PATH_SEP, EXE_SFX = "C:\\", "\\", ".exe"
    DEFAULT_EDITOR = "notepad"
    IS_WIN = True
else:
    ROOT_PATH, PATH_SEP, EXE_SFC = "/", "/", ""
    DEFAULT_EDITOR = "nano"
    IS_WIN = False


# Exception types

class DieException(Exception):
    """ Exit, unsuccessfully, for a specified failure reason. """
    def __init__(self, message):
        self.message = "## ERROR: %s" % (message)


_indent = ""


# Library functions

def pathname(*args):
    return PATH_SEP.join(args)

def die(message):
    """ Exit, unsuccessfully, for a specified failure reason. """
    raise DieException(message)

def warn(message):
    """ Present a non-terminal warning to the user. """
    print(_indent + "*** WARNING: %s" % (message))

def note(message=""):
    """
        Present a minor but non-critical piece of information.
        May be surpressed at lower levels of verbosity reduction.
    """
    print(_indent + "<.> Note: %s" % (message))

def info(message=""):
    """
        Present an optional, minor piece of information to the user.
        Supressed at the first levels of verbosity reduction.
    """
    print(_indent + "<i> info: %s" % (message))

def step(message):
    """
        Minor annotation of progress.
    """
    print(_indent + "=== %s ===" % (message.upper()))

class Step(object):
    def __init__(self, title):
        global _indent
        self._prev_indent = str(_indent)
        step(   title)
        _indent = _indent + '    '
    def __del__(self, *args):
        global _indent
        _indent = self._prev_indent

def question(message, prompt, accept):
    """ Asks a question while following indentation. """
    indent = _indent + "??? "
    print(indent + str(message))
    default = accept[0].lower() if accept[0][0].isupper() else None
    choices = ', '.join(accept)
    while True:
        answer = input(indent + prompt + " (" + choices + "): ")
        answer = answer.lower() or default
        if answer in accept or (default and answer == default):
            return answer
        print(_indent, "XXX Invalid entry, please choose " + choices)



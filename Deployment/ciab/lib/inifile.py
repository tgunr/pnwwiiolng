# Configuration for bootstrap, via an ini file.

from . defaults import *
from . helpers import *

from configparser import ConfigParser
import socket  # so we can look up the base ip
import subprocess

INI_FILE = "bootstrap.ini"

class IniText(object):
    def __init__(self, text):
        self.text = text
    def __str__(self):
        return self.text

class Comment(IniText):
    comment_nesting = 1
    def comment(self, level):
        return ';' * level * self.comment_nesting
    def __init__(self, text, level=1):
        super().__init__('%s %s' % (self.comment(level), text))

class Heading(Comment):
    comment_nesting = 3
    def __init__(self, level, text):
        cmt, returns = self.comment(level), '\n' * (level - 1)
        super().__init__('%s %s%s' % (text, cmt, returns), level=level)

class Field(IniText):
    def __init__(self, name, default, desc=None):
        self.name = name
        desc = "; %s\n" % (desc) if desc else ""
        super().__init__("%s%s=%s" % (desc, name, default))


def _get_ip_address():
    """
        Create a socket aimed at the outside world, and then
        see what address is assigned to the local endpoint.
    """
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("8.8.8.8", 80))  # google's web server
    return str(s.getsockname()[0])


_ini_spec = [
        '[wwiiol]',
        '',
        Heading(3, 'Bootstrap Configuration'),

        Heading(2, 'Database Config'),
        Field('db_host', '127.0.0.1', 'specify address of MySQL server'),
        '',
        Field('db_admin_user', DB_ADMIN_USER, 'specify administrative'
            ' account/password, used only during this setup'),
        Field('db_admin_pass', DB_ADMIN_PASS),
        '',
        Field('db_game_user', DB_GAME_USER, 'account game servers will'
            ' use to access the database. saved in environment.cfg'),
        Field('db_game_pass', DB_GAME_PASS),
        '',

        Heading(2, 'Paths/directories'),
        Field('playnet_path', PLAYNET_PATH, 'installation directory'),
        '',
        Field('data_origin_path', '', 'origin for data (if installing).'),
        Field('data_transfer', 'copy', 'specify whether to copy, move or'
            ' link the source data into the install directory.'),
        Field('bin_origin_path', os.path.abspath('Server'), 'where to'
            ' copy server binaries from (or empty).'),
        Field('lua_origin_path', os.path.abspath('Lua'), 'where to copy'
            ' lua scripts from (or empty).'),

        '',
        Heading(2, 'Less common parameters'),
        Heading(2, '(These are usually good defaults for a desktop server).'),
        Field('db_import_file', DB_IMPORT_FILE, 'database source SQL'),
        Field('cluster_name', 'ciab'),
        Field('app_id', '12'),
        Field('arena_id', '250'),
        Field('base_address', _get_ip_address()),
        '',
        Comment('other paths'),
        Field('log_path', pathname('%(playnet_path)s', 'tmp')),
        Field('wwiiol_path', pathname('%(playnet_path)s', 'wwiiol')),
        Field('data_path', pathname('%(wwiiol_path)s', 'data')),
        Field('uweb_path', pathname('%(wwiiol_path)s', 'uweb', 'debug')),
        Field('lua_path', pathname('%(wwiiol_path)s', 'lua')),
]


def _default_ini_text():
    return '\n'.join(str(field) for field in _ini_spec)


def _write_ini_file():
    note("Regenerating %s." % (INI_FILE))
    with open(INI_FILE, "w") as fh:
        fh.write(_default_ini_text())


class Config(object):
    def __init__(self):
        self.editor = os.getenv("EDITOR") or DEFAULT_EDITOR
        if not self.editor:
            raise DieError("No default EDITOR defined.")
        # create the ini file if it's missing.
        if self._need_ini_file():
            _write_ini_file()

    def _need_ini_file(self):
        if os.path.exists(INI_FILE):
            ans = question("'%s' already exists. We can use the existing entry"
                            " or re-generate it from defaults." % (INI_FILE),
                            "Re-generate .ini file?", [ 'Y', 'n' ])
            if ans != 'y':
                note('Using existing configuration.')
                return False
        return True

    def load(self):
        print("loading")
        cfgp = ConfigParser()

        # launch the editor so the user can make changes
        subprocess.check_call([ self.editor, os.path.abspath(INI_FILE) ])

        # now try and load/parse it.
        try:
            cfgp.read(INI_FILE)
        except Exception as e:
            return "Error in '%s': %s" % (INI_FILE, str(e))

        if not cfgp.has_section('wwiiol'):
            return "Could not find '[wwiiol]' section in '%s'." % (INI_FILE)

        # create attributes for all legal fields
        for fld in [fld for fld in _ini_spec if isinstance(fld, Field)]:
            setattr(self, fld.name, None)

        # now populate with values from the ini
        for key, value in cfgp.items('wwiiol'):
            setattr(self, key, value)

        return None
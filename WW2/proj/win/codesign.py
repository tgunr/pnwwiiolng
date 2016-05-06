#!/bin/env python

################################################################################
### On OS X, Xcode handles the whole mess of code signing issues on its own.
### However, on Windows, Visual Studio still doesn't have any innate support for
### this. There might be some addon or extension or something to do it, but the
### Internet is hilariously bereft of good info. Therefore, this script does
### as much of the dirty work as possible.
###
### This script is designed to wrap signtool.exe, as installed by Visual Studio
### 2015.

import sys, os

SIGNTOOL_PATH = "C:\\Program Files (x86)\\Microsoft SDKs\\Windows\\v7.1A\\Bin\\signtool.exe"
KEYFILE_PATH = "C:\\Users\\Playnet-BuildPC\\Documents\\codesigning.pfx"

def main():
    args = parse_args()
    if type(args.ksrc) != str:
        key = args.ksrc.read()
        args.ksrc.close()
    else:
        key = args.ksrc
    
    for operand in args.operands:
        if args.SIGN:
            if args.level == True:
                print("Signing %s" % operand)
            sign_file(operand, args.TOOL, args.KEYFILE, args.DESC, key, args.level)
            if args.level == True:
                print("Timestamping %s" % operand)
            timestamp_file(operand, args.TOOL, args.level)
        if not args.NOVERIFY:
            if args.level == True:
                print("Verifying %s" % operand)
            verify_file(operand, args.TOOL, args.level)
                
def sign_file(file, TOOL, KEYFILE, DESC, KEY, LEVEL):
    cmdargs = [
        TOOL, 'sign',
        '/f', KEYFILE, # Key file
        '/d', DESC, # Item description
        '/du', 'http://battlegroundeurope.net', # Item URL
        '/p', KEY, # Key password
        '/ph', # Generate page hashes
    ]
    if LEVEL == True:
        cmdargs += ['/v']
    elif LEVEL == False:
        cmdargs += ['/q']
    cmdargs += [file]
    if LEVEL == True:
        print(' '.join(cmdargs))
    (output, status) = run_command(cmdargs)
    if status != 0:
        print("Signing failed:")
        print(output)
        sys.exit(status)

def timestamp_file(file, TOOL, LEVEL):
    import random
    
    MAX_TRIES = 3
    servers = [
        'http://timestamp.comodoca.com/authenticode',
        'http://timestamp.verisign.com/scripts/timestamp.dll',
        'http://timestamp.globalsign.com/scripts/timestamp.dll',
        'http://tsa.starfieldtech.com',
    ]
    assert len(servers) >= MAX_TRIES
    
    tried = [-1]
    value = -1
    success = False
    
    for ntries in range(3):
        while value in tried:
            value = random.SystemRandom().randrange(0, len(servers))
        cmdargs = [
            TOOL, 'timestamp',
            '/tr', servers[value], # Use modern timestamp
            '/td', 'sha256', # Use SHA-256 hashing
        ]
        if LEVEL == True:
            cmdargs += ['/v']
        elif LEVEL == False:
            cmdargs += ['/q']
        cmdargs += [file]
        if LEVEL == True:
            print(' '.join(cmdargs))
        (output, status) = run_command(cmdargs)
        if status == 0:
            success = True
            break
        # Most likely timestamping timed out, keep going
    if success is False:
        print("Timestamping failed:")
        print(output)
        sys.exit(status)

def verify_file(file, TOOL, LEVEL):
    cmdargs = [
        TOOL, 'verify',
        '/pa', # Verify against plain Authenticode
        '/tw', # Warn if not timestamped
    ]
    if LEVEL == True:
        cmdargs += ['/v', '/d']
    elif LEVEL == False:
        cmdargs += ['/q']
    cmdargs += [file]
    if LEVEL == True:
        print(' '.join(cmdargs))
    (output, status) = run_command(cmdargs)
    if status != 0:
        print("Verification failed:")
        print(output)
        sys.exit(status)

def parse_args():
    import argparse
    
    # Main arg parser
    main_opt_parser = argparse.ArgumentParser(prefix_chars='/', add_help=False,
        description='Code signing wrapper for Playnet projects on Windows',
        epilog='Each item is signed and verified sequentially.'
    )
    main_opt_parser.set_defaults(level=None, TOOL=SIGNTOOL_PATH, KEYFILE=KEYFILE_PATH)
    
    # Common options
    main_opts = main_opt_parser.add_argument_group(title='General options')
    main_opts.add_argument('/HELP', action='help', help='Show this help')
    main_opts.add_argument('/VERSION', action='version', version='0.0.1')
    main_opts.add_argument('/TOOL', action='store', help='Alternate signtool.exe path')
    main_opts.add_argument('/QUIET', action='store_false', dest='level', help="No output")
    main_opts.add_argument('/VERBOSE', action='store_true', dest='level', help="Lots of output")
    main_opts.add_argument('operands', action='store', nargs='+', help='Files to process')
    
    # Commands and options
    sign_opts = main_opt_parser.add_argument_group(title='Signing options')
    sign_opts.add_argument('/SIGN', action='store_true', help='Sign files')
    sign_opts.add_argument('/NOVERIFY', action='store_true', help="Don't verify signatures")
    pass_opts = sign_opts.add_mutually_exclusive_group(required=True)
    pass_opts.add_argument('/PASS:HERE', action='store', dest='ksrc')
    pass_opts.add_argument('/PASS:FILE', type=argparse.FileType('r'), dest='ksrc')
    sign_opts.add_argument('/KEYFILE', action='store', help='PFX file')
    sign_opts.add_argument('/DESC', action='store', required=True, help='Item(s) description')
    
    # Parse away
    return main_opt_parser.parse_args()

def run_command(args):
    import shlex, subprocess
    
    with subprocess.Popen(args, stdout=subprocess.PIPE) as proc:
        rawdata = proc.communicate()[0]
        if sys.version_info[0] < 3:
            return (str(rawdata).strip('\n'), proc.returncode)
        else:
            return (str(rawdata, sys.stdout.encoding).strip('\n'), proc.returncode)

if __name__=="__main__":
   main()

"""
### Define some constants
version_info_file = "WW2/proj/versionNo.h"
revision_file_in  = "WW2/src/build_info.cpp.in"
revision_file_out = "build_info.cpp"

### General imports
import os, sys, tempfile, getpass, time

### Entry point
def main():
    if len(sys.argv) < 3:
        sys.exit("Usage: %s <build type> <output path>" % sys.argv[0])

    repo_path = os.path.realpath(os.path.dirname(sys.path[0]))
    git_command = "git -c color.ui=never -C %s" % repo_path
    build_type = sys.argv[1]
    output_directory = os.path.realpath(sys.argv[2])
    version_value = get_version_value(os.path.realpath(os.path.join(repo_path, version_info_file)))
    git_commit = get_command_output("%s rev-parse HEAD" % git_command)
    git_commitish = get_command_output("%s rev-parse --short HEAD" % git_command)
    git_state = "clean" if len(get_command_output("%s status --porcelain" % git_command)) == 0 else "dirty"
    host_platform = "Mac" if sys.platform == 'darwin' else ("PC" if sys.platform == 'win32' else "Linux")

    if build_type != 'client' and build_type != 'host':
        sys.exit("Build type must be 'client' or 'host'")

    replacements = {
        'DATETIME':          time.strftime('%a %b %d, %Y %H:%M:%S %Z'),
        'INPUT_FILE_NAME':   os.path.basename(revision_file_in),
        'OUTPUT_FILE_NAME':  revision_file_out,
        'PLATFORM':          host_platform,
        'BUILD_TYPE':        build_type,
        'CURRENT_USER':      getpass.getuser(),
        'BUILD_HOST':        get_command_output("hostname"),
        'GIT_ROOT':          repo_path,
        'PRODUCT_VERSION':   version_value,
        'GIT_COMMIT':        git_commit,
        'GIT_COMMITISH':     git_commitish,
        'GIT_STATE':         git_state,
    }

    configure_file(os.path.join(repo_path, revision_file_in), os.path.join(output_directory, revision_file_out), replacements)
    print("Wrote build information to " + os.path.join(output_directory, revision_file_out))
    if sys.platform == 'darwin' and os.environ.get('BUILT_PRODUCTS_DIR') != None and build_type == 'client':
        update_infoplist(replacements)
        print("Updated Mac Info.plist")

### Subroutine for running commands
def get_command_output(cmd):
    import shlex, subprocess
    
    rawdata = subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE).communicate()[0]
    if sys.version_info[0] < 3:
        return str(rawdata).strip('\n')
    else:
        return str(rawdata, sys.stdout.encoding).strip('\n')

### Subroutine for atomically replacing a file, with Windows being a giant pain
def replace_file_atomic(src, dst):
    # Because atomic writes are only a solved problem in Python 3.3+, shenanigans!
    if sys.version_info[0] > 3 or (sys.version_info[0] == 3 and sys.version_info[1] >= 3):
        # 3.3+, use the supported API for the purpose
        os.replace(src, dst)
    elif sys.platform == 'win32':
        # Windows < 3.3, haul in the pain-in-the-rear Win32 API
        import win32api, win32con
        win32api.MoveFileEx(src, dst, win32con.MOVEFILE_REPLACE_EXISTING)
    else:
        # Non-Windows < 3.3, use the perfectly good semantics POSIX guarantees but Windows is stupid about
        os.rename(src, dst)

### Perform replacement substitutions on a file atomically
def configure_file(src, dst, substitutions):
    with tempfile.NamedTemporaryFile(mode='w', delete=False) as ftmp:
        with open(src, 'r') as fin:
            for line in fin:
                for replKey, replVal in substitutions.iteritems():
                    line = line.replace(r'${%s}' % replKey, replVal)
                ftmp.write(line)
        ftmp.close()
        replace_file_atomic(ftmp.name, dst)

### Update the Mac Info.plist file
def update_infoplist(substitutions):
    infoplist_path = os.path.join(
        os.environ.get("BUILT_PRODUCTS_DIR"),
        os.environ.get("WRAPPER_NAME"),
        "Contents/Info.plist"
    )
    plist_subs = {
        'GitShaHash': substitutions['GIT_COMMITISH'], 'GitState': substitutions['GIT_STATE'],
        'CFBundleShortVersionString': substitutions['PRODUCT_VERSION'], 'CFBundleVersion': substitutions['PRODUCT_VERSION']
    }
    for pkey, pval in plist_subs.iteritems():
        get_command_output('plutil -replace "%s" -string "%s" "%s"' % (pkey, pval, infoplist_path))
    get_command_output('plutil -convert xml1 "%s"' % infoplist_path)

### Use the preprocessor to pull the version number from the version file
def get_version_value(version_info_file_path):
    # Once again Windows insists on making things difficult. Since there's no
    # way to redirect stdin into CL.EXE, we just use the completely pointless
    # and wasteful temp file on all platforms.
    version_expansions = "PN_WW2_PRODUCT_NO.PN_WW2_VERSION_NO.PN_WW2_PATCH_NO.PN_WW2_BUILD_NO"
    with tempfile.NamedTemporaryFile(mode='w', bufsize=0) as ftmp:
        ftmp.write(version_expansions + "\n") ; ftmp.flush()
        if sys.platform == 'win32':
            version_value = get_command_output('cl.exe /EP /FI "%s" /TP "%s"' % (version_info_file_path, ftmp.name))
        else:
            version_value = get_command_output('clang -E -P -include "%s" -x c++ "%s"' % (version_info_file_path, ftmp.name))
    return version_value.replace(' ', '')

### Let Python have "forward declarations" etc.
if __name__=="__main__":
   main()
"""

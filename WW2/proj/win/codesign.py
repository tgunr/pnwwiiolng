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
    remark(sys.argv, False)
    args = parse_args()
    remark(args, args.VS)
    if type(args.ksrc) != str:
        key = args.ksrc.read()
        args.ksrc.close()
    else:
        key = args.ksrc
    
    for operand in args.operands:
        if args.SIGN:
            if args.level == True:
                remark("Signing %s" % operand, args.VS)
            sign_file(operand, args.TOOL, args.KEYFILE, args.DESC, key, args.level, args.VS)
            if args.level == True:
                remark("Timestamping %s" % operand, args.VS)
            timestamp_file(operand, args.TOOL, args.level, args.VS)
        if not args.NOVERIFY:
            if args.level == True:
                remark("Verifying %s" % operand, args.VS)
            verify_file(operand, args.TOOL, args.level, args.VS)

def remark(msg, VS):
    if type(msg) != str:
        msg = str(msg)
    if VS == True:
        for l in msg.split('\n'):
            print(l.strip())
    else:
        print(msg)

def sign_file(file, TOOL, KEYFILE, DESC, KEY, LEVEL, VS):
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
        remark(' '.join(cmdargs), VS)
    (output, status) = run_command(cmdargs)
    if status != 0:
        remark("Signing failed:", VS)
        remark(output, VS)
        sys.exit(status)
    elif LEVEL == True:
        remark(output, VS)

def timestamp_file(file, TOOL, LEVEL, VS):
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
            remark(' '.join(cmdargs), VS)
        (output, status) = run_command(cmdargs)
        if status == 0:
            success = True
            break
        # Most likely timestamping timed out, keep going
    if success is False:
        remark("Timestamping failed:", VS)
        remark(output, VS)
        sys.exit(status)
    elif LEVEL == True:
        remark(output, VS)

def verify_file(file, TOOL, LEVEL, VS):
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
        remark(' '.join(cmdargs), VS)
    (output, status) = run_command(cmdargs)
    if status != 0:
        remark("Verification failed:", VS)
        remark(output, VS)
        sys.exit(status)
    elif LEVEL == True:
        remark(output, VS)

def parse_args():
    import argparse
    
    # Main arg parser
    main_opt_parser = argparse.ArgumentParser(prefix_chars='/', add_help=False,
        description='Code signing wrapper for Playnet projects on Windows',
        epilog='Each item is signed and verified sequentially.'
    )
    main_opt_parser.set_defaults(level=None, TOOL=SIGNTOOL_PATH, KEYFILE=KEYFILE_PATH, VS=False)
    
    # Common options
    main_opts = main_opt_parser.add_argument_group(title='General options')
    main_opts.add_argument('/HELP', action='help', help='Show this help')
    main_opts.add_argument('/VERSION', action='version', version='0.0.1')
    main_opts.add_argument('/TOOL', action='store', help='Alternate signtool.exe path')
    main_opts.add_argument('/QUIET', action='store_false', dest='level', help="No output")
    main_opts.add_argument('/VERBOSE', action='store_true', dest='level', help="Lots of output")
    main_opts.add_argument('/VS', action='store_true',help='Reformat output for Visual Studio')
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

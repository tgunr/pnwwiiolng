#!/bin/env python

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
    git_commit = get_command_output("%s rev-parse HEAD" % git_command)
    git_commitish = get_command_output("%s rev-parse --short HEAD" % git_command)
    git_state = "clean" if len(get_command_output("%s status --porcelain" % git_command)) == 0 else "dirty"
    version_value = get_version_value(os.path.realpath(os.path.join(repo_path, version_info_file))).strip()
    host_platform = "Mac" if sys.platform == 'darwin' else ("PC" if sys.platform == 'win32' else "Linux")

    if build_type != 'client' and build_type != 'host':
        sys.exit("Build type must be 'client' or 'host'")

    replacements = {
        'DATETIME':          time.strftime('%a %b %d, %Y %H:%M:%S %Z'),
        'INPUT_FILE_NAME':   os.path.basename(revision_file_in),
        'OUTPUT_FILE_NAME':  revision_file_out.replace("\\", "\\\\"),
        'PLATFORM':          host_platform,
        'BUILD_TYPE':        build_type,
        'CURRENT_USER':      getpass.getuser(),
        'BUILD_HOST':        get_command_output("hostname").strip(),
        'GIT_ROOT':          repo_path.replace("\\", "\\\\"),
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
    
    if sys.platform == 'win32':
        posix_parse = False
    else:
        posix_parse = True
    rawdata = subprocess.Popen(shlex.split(cmd, False, posix_parse), stdout=subprocess.PIPE).communicate()[0]
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
    with tempfile.NamedTemporaryFile(mode='wt', delete=False) as ftmp:
        with open(src, 'rt') as fin:
            for line in fin:
                for replKey, replVal in substitutions.iteritems():
                    line = line.replace(r'${%s}' % replKey, replVal.replace("\r\n", "\n"))
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
    if sys.platform == 'win32':
        version_expansions = "PN_WW2_PRODUCT_NO . PN_WW2_VERSION_NO . PN_WW2_PATCH_NO . PN_WW2_BUILD_NO"
        with tempfile.NamedTemporaryFile(mode='w', delete=False) as ftmp:
            ftmp.write(version_expansions)
            ftmp.close()
            raw_version_value = get_command_output('cl.exe /nologo /EP /FI %s /TP %s' % (version_info_file_path, ftmp.name)).strip()
            os.remove(ftmp.name)
            version_value = raw_version_value.replace('#pragma once', '')
    else:
        version_expansions = "PN_WW2_PRODUCT_NO.PN_WW2_VERSION_NO.PN_WW2_PATCH_NO.PN_WW2_BUILD_NO"
        with tempfile.NamedTemporaryFile(mode='w', bufsize=0) as ftmp:
            ftmp.write(version_expansions + "\n") ; ftmp.flush()
            version_value = get_command_output('clang -E -P -include "%s" -x c++ "%s"' % (version_info_file_path, ftmp.name))
    return version_value.replace(' ', '')

### Let Python have "forward declarations" etc.
if __name__=="__main__":
   main()

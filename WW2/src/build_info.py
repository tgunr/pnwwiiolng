#!/bin/env python

### Define some constants
version_info_file = "WW2/src/build_info.hpp"
revision_file_in  = "WW2/src/build_info.cpp.in"
revision_file_out = "build_info.cpp"

### This pattern is complex so define it out of line for clarity
version_info_pattern = r"""(?x)
    \s+PN_build_version_major\s=\s(?P<major>\d+),
    \s+PN_build_version_minor\s=\s(?P<minor>\d+),
    \s+PN_build_version_patch\s=\s(?P<patch>\d+),
    \s+PN_build_version_build\s=\s(?P<build>\d+)
"""

### General imports
import os, sys, tempfile, getpass, time, re, shutil, shlex, subprocess

### Entry point
def main():
    if len(sys.argv) < 3:
        raise Exception("Usage: %s <build type> <output path>" % sys.argv[0])
    
    repo_path = os.path.realpath(os.path.dirname(os.path.dirname(sys.path[0])))
    build_type = sys.argv[1]
    output_directory = os.path.realpath(sys.argv[2])
    host_platform = "Mac" if sys.platform == 'darwin' else ("PC" if sys.platform == 'win32' else "Linux")

    if build_type != 'client' and build_type != 'host':
        raise Exception("Build type must be 'client' or 'host'")
    
    git_command = "git -c color.ui=never -C %s" % repo_path
    git_commit = get_command_output("%s rev-parse HEAD" % git_command)
    git_commitish = get_command_output("%s rev-parse --short HEAD" % git_command)
    git_state = "clean" if len(get_command_output("%s status --porcelain" % git_command)) == 0 else "dirty"
    version_values = get_version_values(os.path.realpath(os.path.join(repo_path, version_info_file)))
    
    replacements = {
        'DATETIME':                     time.strftime('%a %b %d, %Y %H:%M:%S %Z'),
        'DATEYEAR':                     time.strftime('%Y'),
        'INPUT_FILE_NAME':              os.path.basename(revision_file_in),
        'OUTPUT_FILE_NAME':             revision_file_out.replace("\\", "\\\\"),
        'PLATFORM':                     host_platform,
        'BUILD_TYPE':                   build_type,
        'CURRENT_USER':                 getpass.getuser(),
        'BUILD_HOST':                   get_command_output("hostname").strip(),
        'GIT_ROOT':                     repo_path.replace("\\", "\\\\"),
        'PRODUCT_VERSION':              version_values['words'],
        'PRODUCT_VERSION_VALUE_MAJOR':  version_values['major'],
        'PRODUCT_VERSION_VALUE_MINOR':  version_values['minor'],
        'PRODUCT_VERSION_VALUE_PATCH':  version_values['patch'],
        'PRODUCT_VERSION_VALUE_BUILD':  version_values['build'],
        'PRODUCT_VERSION_VALUE':        version_values['value'],
        'GIT_COMMIT':                   git_commit,
        'GIT_COMMITISH':                git_commitish,
        'GIT_STATE':                    git_state,
    }
    
    configure_file(os.path.join(repo_path, revision_file_in), os.path.join(output_directory, revision_file_out), replacements)
    print("Wrote build information to " + os.path.join(output_directory, revision_file_out))
    if sys.platform == 'darwin' and os.environ.get('BUILT_PRODUCTS_DIR') != None and build_type == 'client':
        update_mac_infoplist(replacements)
        print("Updated Mac Info.plist")
    elif sys.platform == 'win32':
        update_win_rc(repo_path, replacements)
        print("Updated Windows .rc file")
    

### Subroutine for running commands
def get_command_output(cmd):
    if sys.platform == 'win32':
        posix_parse = False
    else:
        posix_parse = True
    rawdata = subprocess.Popen(shlex.split(cmd, False, posix_parse), stdout=subprocess.PIPE).communicate()[0]
    if sys.version_info[0] < 3:
        return str(rawdata).strip('\n')
    else:
        return str(rawdata, sys.stdout.encoding).strip('\n')

### Perform replacement substitutions on a file atomically
def configure_file(src, dst, substitutions):
    with tempfile.NamedTemporaryFile(mode='wt', delete=False) as ftmp:
        with open(src, 'rt') as fin:
            for line in fin:
                for replKey, replVal in substitutions.items():
                    line = line.replace(r'${%s}' % replKey, replVal.replace("\r\n", "\n"))
                ftmp.write(line)
        ftmp.close()
        shutil.move(ftmp.name, dst)

### Update the Mac Info.plist file
def update_mac_infoplist(substitutions):
    infoplist_path = os.path.join(
        os.environ.get("BUILT_PRODUCTS_DIR"),
        os.environ.get("WRAPPER_NAME"),
        "Contents",
        "Info.plist"
    )
    plist_subs = {
        'GitShaHash': substitutions['GIT_COMMITISH'], 'GitState': substitutions['GIT_STATE'],
        'CFBundleShortVersionString': substitutions['PRODUCT_VERSION'], 'CFBundleVersion': substitutions['PRODUCT_VERSION']
    }
    for pkey, pval in plist_subs.items():
        get_command_output('plutil -replace "%s" -string "%s" "%s"' % (pkey, pval, infoplist_path))
    get_command_output('plutil -convert xml1 "%s"' % infoplist_path)

### Update the Windows WW2.rc2 file
def update_win_rc(repo_path, substitutions):
    winrc_base_path = os.path.join(
        repo_path,
        "WW2",
        "proj",
        "win",
        "res"
    )
    winrc_in_path = os.path.join(winrc_base_path, "WW2.rc2.in")
    winrc_out_path = os.path.join(winrc_base_path, "WW2.rc2")
    # This is a copout - dropping the "output" back in the source directory
    # is bad form, but I don't know VS well enough to do better.
    configure_file(winrc_in_path, winrc_out_path, substitutions)

### Pull the version numbers from the version file
def get_version_values(version_info_file_path):
    with open(version_info_file_path, 'rt') as version_info_file:
        version_info_text = version_info_file.read()
    version_info_pieces = re.search(version_info_pattern, version_info_text)
    if not version_info_pieces:
        raise Exception("Failed to find version information in %s" % version_info_file_path)
    
    version_info = version_info_pieces.groupdict()
    version_info["value"] = "0x%s%s%s%s" % tuple(map(lambda s: s.zfill(2), version_info_pieces.groups()))
    version_info["words"] = "%s.%s.%s.%s" % tuple(version_info_pieces.groups())
    return version_info

### Let Python have "forward declarations" etc.
if __name__=="__main__":
   main()

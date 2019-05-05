#!/bin/bash
################################################################
# release.sh
#
# Supported platforms: Windows, Mac
# Supported shells: mobaxterm bash
#
# This script will automatically (if enabled) follow the 
# documented build procedures for building a PC installer
# which includes building the diffs for a PC patch.
#
# !!!IF THINGS LOOK OVERLY COMPLICATED!!!!
#
# Don't just assume that the author didn't know there was a way
# easier way to do something!  Bash shell implementation varies
# wildly and Mobaxterm's implementaion has a number of flaws 
# where it doesn't behave as expected.  One area in particular
# is how it translates user/group ownership and permissions.
# This bad behavior forces using sub-optimal approaches
# occsionally so DON'T change anything unless you throroughly 
# test it first under Mobaxterm
################################################################

function build_step {
    if [ "$pause_enabled" = true ] ; then
        out_msg='About to '$1$' '$3$' press any key to continue'
        read -p "$out_msg" -n 1 -r 
        echo    # (optional) move to a new line
    echo "Step "$buildstep". "$2" "$3
    else
    echo "Step "$buildstep". "$2" "$3
    fi

    if (( skipto > 0 )) ; then
        if (( skipto > buildstep )); then
            skip_step=true
    else
            skip_step=false
    fi
    else
        skip_step=false
    fi

    buildstep=$((buildstep+1))
}

function print_exit_msg {
IFS="|" read -r -a msgs <<< "$exit_msg"
echo    # (optional) move to a new line
for msg in "${msgs[@]}"
do
    echo "$msg"
done
}

function winpath2bash {
    bashpath=$(echo "$1" | sed 's/\\/\//g' | sed 's/://')
    bashpath="/drives/$bashpath"
}

trap '
echo    # (optional) move to a new line
echo "Exiting. . ."
print_exit_msg
sleep 5
' EXIT

################################################################

#    Setup

################################################################

WWII_ROOT=$(pwd)
WW2MODS="$WWII_ROOT/opt/ww2mods"
#WW2ARCHIVES="$WWII_ROOT/WW2Archives" - MJH
WW2ARCHIVES_ROOT=""
WW2ARCHIVES_RELEASE=""
INSTALLER_ROOT="$WWII_ROOT/Installer"
INSTALLER_RELEASE=""
LIBDIR="$WWII_ROOT/Installer/libs"
DEBUGDIR="$WWII_ROOT/build/client/Debug/Bin"
RELEASEDIR="$WWII_ROOT/build/client/Release/Bin"
#RELEASELIBDIR="$WWII_ROOT/Build/lib/Win32/Release"
PRODUCTS="WW2.exe Playgate.exe Settings.exe"
DATADIRS="DirectXRedist loc playgate"
DATAFILES="steam_appid.txt eula.txt license.rtf readme.rtf regions.txt tos.txt wwii1.ico"
ASSETFILES="eula.txt license.rtf readme.rtf wwii1.ico"
BUILDDIR="$APPDATA"
BUILDLOG="build_output.log"
#PREV_RELEASE="$PROGRAMFILES\\CRS\\Battleground Europe\\" - MJH
PREV_RELEASE="$PROGRAMFILES\\CRS\\WWIIOnline\\"

InstallerScriptName="WWIIOL_Installer_Template.iss"
build_release=""  # Valid Options: RC = Release Candidate, ALPHA = Alpha Release, BETA = Beta Release.
Client_release="rc"
Client_alpha="alpha"
Client_beta="beta"
Client_end="end"

echo
echo
echo "Installer Build Type."
echo "Available Options: -- "$Client_release" = Release Candidate"
echo "                   -- "$Client_alpha" = Alpha Release"
echo "                   -- "$Client_beta" = Beta Release"
echo "                   -- "$(echo $Client_end |tr [:lower:] [:upper:])" to Exit the script."
echo    # (optional) move to a new line

while read -p "Enter the Installer Build Type ( "$Client_release" | "$Client_alpha" | "$Client_beta" | "$(echo $Client_end |tr [:lower:] [:upper:])" ): " userInput; do
    if [[ ! -z "$userInput" ]]; then
        case $(echo "$userInput" |tr [:upper:] [:lower:]) in
            $Client_release)
                build_release=$Client_release
                echo
                echo "Release Candidate build."
                break
                ;;

            $Client_alpha)
                build_release=$Client_alpha
                echo
                echo "Alpha Client build."
                break
                ;;

            $Client_beta)
                build_release=$Client_beta
                echo
                echo "Beta Client build."
                break
                ;;

            $Client_end)
                exit_msg="When you are ready to proceed, restart the build process."
                
                exit 1
                ;;
        esac
    fi
done

skipto=$1
if (( skipto > 0 )) ; then
    echo "Will skip to step "$skipto
fi

buildstep=1
skip_step=false

#pause_enabled=true
pause_enabled=false

echo
echo

build_step "check" "Checking" "pre-requisites"

found_prereq_errors=false
exit_msg=""

myos=""
if [ "$OSTYPE" == "cygwin" ] ; then
    echo "Detected Windows under cygwin"

    myos="win"
    WW2ARCHIVES_ROOT="$WWII_ROOT/ww2assets/oswin/archivefiles"

elif [[ "${OSTYPE#*darwin}" != "$OSTYPE" ]] ; then
    echo "Detected OSX"
    echo "WARNING: If Xcode is running it will be terminated before the build starts.  Save any unsaved work!."
    echo "WARNING: DO NOT DO ANYTHING while build is running, especially when DropDMG is running!!!!"

    myos="osx"
    WW2ARCHIVES_ROOT="$WWII_ROOT/ww2assets/osmac/archivefiles"

else
    exit_msg="Unknown OS, make sure you are running this from a bash shell on the Mac, or Mobaxterm on Windows"
    exit 1
fi

echo

if [ "$myos" == "win" ] ; then
    isccpath=$(which iscc)
    echo "Inno Setup        : < "$isccpath" >"
    if [ ! -x "$isccpath" ] ;    then
        exit_msg=${exit_msg}'|Pre-requisites ERROR: Inno setup install directory not in windows path, please add it'
        found_prereq_errors=true
    fi

    devenvpath=$(which devenv)
    echo "Microsoft build   : < "$devenvpath" >"
    if [ ! -x "$devenvpath" ] ; then
        exit_msg=${exit_msg}'|Pre-requisites ERROR: Microsoft build (devenv) not in windows path, please add it'
        found_prereq_errors=true
    fi

    gitpath=$(which git)
    echo "Git bin directory : < "$gitpath" >"
    if [ ! -x "$gitpath" ] ; then
        exit_msg=${exit_msg}'|Pre-requisites ERROR: Git bin directory not in windows path, please add it'
        found_prereq_errors=true
    fi

    echo

else
    min_xcode_version_major=8
    min_xcode_version_minor=2
    
    #validate xcode version
    xcodepath=$(which xcodebuild)
    echo "Apple Xcode       : < "$gitpath" >"
    if [ ! -x "$xcodepath" ] ; then
        exit_msg=${exit_msg}'|Pre-requisites ERROR: XCode is not available, install version $min_xcode_version_major.$min_xcode_version_minor or greater'
        found_prereq_errors=true
    else
        xcode_version=$(xcodebuild -version)
        version_rsp=(${xcode_version// / })
        version_nums=(${version_rsp[1]//\./ })
        if (( version_nums[0] < min_xcode_version_major ))  || (( version_nums[1] < min_xcode_version_minor  && version_nums[0] == min_xcode_version_major )); then
            exit_msg=${exit_msg}'|Pre-requisites ERROR: Xcode version incorrect, update to version '$min_xcode_version_major'.'$min_xcode_version_minor' or greater' 
            found_prereq_errors=true
        fi
    fi
fi

gitpath=$(which git)
if [ ! -x "$gitpath" ] ; then
    exit_msg=${exit_msg}'|Pre-requisites ERROR: Git not found, please install and/or add to path'
    found_prereq_errors=true
fi

# make sure we are executing from the right directory
if [ ! "$(basename $(pwd))" == "wwiiolng" ] ; then
    exit_msg=${exit_msg}'|Pre-requisites ERROR: This command must be executed from the root dir: wwiiolng'
    found_prereq_errors=true
fi

# make sure the opt/ww2mods directory exists
if [ ! -d "opt/ww2mods" ] ; then
    exit_msg=$exit_msg'|Pre-requisites ERROR: ww2mods is not loaded (or linked) through opt directory, please correct'
    found_prereq_errors=true
fi

# set the installer root and archive root directory. $INSTALLER_ROOT $WW2ARCHIVES_ROOT (OSTYPE)is set earlier.
case $build_release in
    $Client_release)
        INSTALLER_RELEASE=$INSTALLER_ROOT"/release-candidate"
        WW2ARCHIVES_RELEASE=$WW2ARCHIVES_ROOT"/release-candidate"
        ;;
    $Client_alpha)
        INSTALLER_RELEASE=$INSTALLER_ROOT"/release-alpha"
        WW2ARCHIVES_RELEASE=$WW2ARCHIVES_ROOT"/release-alpha"
        ;;
    $Client_beta)
        INSTALLER_RELEASE=$INSTALLER_ROOT"/release-beta"
        WW2ARCHIVES_RELEASE=$WW2ARCHIVES_ROOT"/release-beta"
        ;;
esac

# make sure the installer root directory exists, set earlier.
if [ ! -d $INSTALLER_RELEASE ] ; then
    exit_msg=$exit_msg'|Pre-requisites ERROR: The directory: '$INSTALLER_RELEASE', does not exist. Please create'
    found_prereq_errors=true
fi

# make sure the archive root directory exists. $WW2ARCHIVES_ROOT is set by the OSTYPE located earlier.
if [ ! -d $WW2ARCHIVES_RELEASE ] ; then
    exit_msg=$exit_msg'|Pre-requisites ERROR: The directory: '$WW2ARCHIVES_RELEASE', does not exist. Please create'
    found_prereq_errors=true
fi

# Build Windows Patch Installers?
if [ "$myos" == "win" ] ; then
    while read -p "Build Windows Patch Installers (y|n) :" -n 1 -r ; do
        if [[ $REPLY =~ ^[Yy]$ ]] ; then
            build_patch=true
            echo
            echo
            echo "Building Windows Patch Installers."
            break

        else    
            build_patch=false
            echo
            echo
            echo "Windows Patch Installers WILL NOT be built."
            break
        fi
    done

    if [  ! -d $INSTALLER_RELEASE/previous  ] ; then
        exit_msg=$exit_msg'|Pre-requisites ERROR: The standard location for previous build "$INSTALLER_RELEASE"/previous, does not exist. Please Create'
        found_prereq_errors=true
    fi
fi

build_steam=false
steam_user=""
steam_pwd=""

if [ "$myos" == "win" ] ; then
    # make sure visual studio isn't running
    vsProc=`ps -W | grep devenv`
    if [ -n "$vsProc" ] ; then
        exit_msg=$exit_msg'|Pre-requisites ERROR: Visual studio cannot be running when starting the build, please close all instances'
        found_prereq_errors=true
    fi
else
    # check mac specific issues
    playgate_version=$(grep -r -m 1 -h 'PLAYGATE_VERSION' WW2/src)
    pn_elements=(${playgate_version// / })
    if [ ! "${pn_elements[2]}" == "0x01010109" ] ; then
        exit_msg=$exit_msg'|Pre-requisites ERROR: Playgate version is not correct ('${pn_elements[2]}') should be 0x01010109, please correct'
        found_prereq_errors=true
    fi
fi

if [ "$found_prereq_errors" = true ] ; then
    exit 1
fi

# Pre-requisites checking Completed.

echo
echo

noprompt=false

read -p "Build assuming all default responses? (y|n) :" -n 1 -r
echo    # (optional) move to a new line
if [[ $REPLY =~ ^[Yy]$ ]] ; then
    noprompt=true
fi

gitBranch=`git rev-parse --abbrev-ref HEAD`
arrayBranchElements=(${gitBranch//\// })
releasetype=${arrayBranchElements[0]}
releaseversion=${arrayBranchElements[1]}

echo
isrelease=false

if [ "$noprompt" = false ]; then
    read -p "Is this a formal release? (y|n) :" -n 1 -r
    echo    # (optional) move to a new line
else
    REPLY='y'
fi

echo

if [[ $REPLY =~ ^[Yy]$ ]] ; then
    if [ "$releasetype" != "release" ] && [ "$releasetype" != "hotfix" ]; then
        exit_msg="Formal releases must be on a release or hotfix branch, create a release or hotfix branch and restart build."
        exit 1
    fi

    echo "Release version is "$releaseversion
    isrelease=true

else
    if [ "$releasetype" == "feature" ] ; then
        if [ "$noprompt" = false ]; then
            echo "WARNING! You are building from a feature branch which is normally unwise. . ."
            read -p "Are you sure you want to continue? (y|n) :" -n 1 -r
            echo    # (optional) move to a new line
        else
            REPLY='n'    
        fi
        if [[ ! $REPLY =~ [Yy]$ ]] ; then
                exit_msg="Tend to your repository state and restart build"
                exit 1
        fi
    fi
fi

echo

build_step "test" "Testing" "repositories"

if [ "$myos" == "osx" ] ; then
    osascript -e 'tell app "Xcode" to quit'
fi

cleanedfiles=`git clean -d -x -n --exclude="Installer/release.sh" && git -C opt/ww2mods clean -d -x -n`
if [ -n "$cleanedfiles" ] ; then
    clean=false
    echo "Your repositories are not clean. . ."

    if [ "$isrelease" = true ] ; then
        if [ "$noprompt" = false ]; then
            read -p "You must clean them before proceeding, press C to continue :" -n 1 -r
            echo    # (optional) move to a new line
        else
                REPLY='c'
        fi

        if [[ ! $REPLY =~ ^[Cc]$ ]] ; then
            exit_msg="When you are ready to proceed, restart the build process"
            exit 1
        fi

        clean=true
    else
        if [ "$noprompt" = false ]; then
            read -p "Would you like to clean the repositories first? (y|n) :" -n 1 -r
            echo    # (optional) move to a new line
        else
            REPLY='y'
        fi

        if [[ $REPLY =~ ^[Yy]$ ]] ; then
            clean=true
        fi
    fi

    build_step "clean" "Cleaning" "repositories"
    if [ "$skip_step" = false ]; then
        if [ "$clean" = true ] ; then
            #Wickedness below because mac's built in bash sucks
            nl=$'\n'
            echo $cleanedfiles | sed "s/ Would/\\${nl}Would/g"
            if [ "$noprompt" = false ]; then
                read -p "The above files will be affected, please (V)erify to proceed :" -n 1 -r 
                echo    # (optional) move to a new line
                if [[ ! $REPLY =~ ^[Vv]$ ]] ; then
                    exit_msg="When you are ready to proceed, restart the build process"
                    exit 1
                fi
            fi
            echo "Cleaning wwiiolng"
            git clean -d -x -f --exclude="Installer/release.sh"
            echo "Resetting wwiiolng"
            git reset --hard HEAD
            echo "Cleaning ww2mods"
            git -C opt/ww2mods clean -d -x -f
            echo "Resetting ww2mods"
            git -C opt/ww2mods reset --hard HEAD
        fi
    fi
fi

read -p "Deploy to Steam when finished? (y|n) :" -n 1 -r
echo    # (optional) move to a new line
if [[ $REPLY =~ ^[Yy]$ ]] ; then
    build_steam=true
    echo -n "Enter Steam build user name: "
    read -r steam_user
    echo    # (optional) move to a new line

    if [ ! -z "$steam_user" ] ; then
        echo -n "Enter Steam build password: "
        read -s steam_pwd
        echo    # (optional) move to a new line

        echo "Preparing Steam build environment"
        echo "NOTE: If this is your first time on this computer you may be asked to enter a confirmation code emailed to you!!"

        if [ "$myos" == "win" ] ; then
            pushd "Libraries/SteamSDK/tools/ContentBuilder/builder"
            ./steamcmd.exe +login $steam_user $steam_pwd +quit
            popd

        else
            pushd "Libraries/SteamSDK/tools/ContentBuilder/builder_osx"
            chmod 744 steamcmd.sh
            chmod 744 osx32/steamcmd
            ./steamcmd.sh +login $steam_user $steam_pwd +quit
            popd
        fi
        echo "Preparing Steam build environment"

    else
        echo "Warning: No user specified, will not deploy to steam"
        build_steam=false
    fi
fi

#if [ "$build_patch" = true ] ; then
#    winpath2bash "$PREV_RELEASE"
#    build_step "copy" "copying" "over previous release for patch from $bashpath"
#    if [ "$skip_step" = false ]; then
#        rsync -av "$bashpath/" "$INSTALLER/previous/"
#    fi
#fi

echo "TODO: libxml prebuild"


################################################################

#    Build Debug Clients for .mdl creation.

################################################################

mac_product_base_name="World\ War\ II\ Online"
mac_product_name=$mac_product_base_name
if [ "$isrelease" = false ] && [ "$myos" == "osx" ] ; then
    echo "Updating CFBundleName and PRODUCT_NAME"

    case $build_release in
        $Client_alpha)
            mac_product_name=$mac_product_name"\ Alpha"
            ;;
        $Client_beta)
            mac_product_name=$mac_product_name"\ Beta"
            ;;
    esac

    strip_name=`echo "$mac_product_name" | sed 's/\\\//g'`
    plutil -replace CFBundleName -string "$strip_name"  WW2/proj/mac/Info.plist

    target_file='WW2/WW2OL.xcodeproj/project.pbxproj'
    grepresult=$(grep "$mac_product_name" $target_file)
    if [ ! -n "$grepresult" ] ; then
        sedargs='s/'$mac_product_base_name'/'$mac_product_name'/g'
        sed -i -e "$sedargs" "$target_file"
    fi
fi

if [ "$myos" == "osx" ] ; then
    touch $WW2ARCHIVES_RELEASE/playarc.ww2
fi

mac_app_name=$mac_product_name".app"

bld_config="Debug"
build_step "build" "Building" "debug version"
if [ "$skip_step" = false ]; then
    if [ "$myos" == "win" ] ; then
        # Building Debug client - Win (.mdl files)
        BUILDFILE="$BUILDDIR\\$bld_config$BUILDLOG"
        winpath2bash "$BUILDFILE"
        rm "$bashpath"
        echo "writing output to: "$BUILDFILE
        if [ "$isrelease" = true ] ; then
            echo "Cleaning "$bld_config" first"
            devenv WW2.sln /clean $bld_config
        fi
        devenv WW2.sln /build $bld_config /out "$BUILDFILE"

        errOutput=`grep ": error|: fatal" "$BUILDFILE"`
        if [ -n "$errOutput" ] ; then
            echo $errOutput
            exit_msg="Error: Build failed, see "$BUILDFILE" for errors"
            exit 1
        fi
    else
        # Building Debug client - Mac (.mdl files)
        if [ "$isrelease" = true ] ; then
            echo "Cleaning build"
            xcodebuild -project WW2/WW2OL.xcodeproj -scheme $bld_config' Client' clean PROVISIONING_PROFILE= CODE_SIGN_IDENTITY= WARNING_CFLAGS=-w
        fi
        xcodebuild -project WW2/WW2OL.xcodeproj -scheme $bld_config' Client' build PROVISIONING_PROFILE= CODE_SIGN_IDENTITY= WARNING_CFLAGS=-w
    fi
fi


################################################################

#  ReBuild the .mdl files in the opt/ww2mods directory.

################################################################

build_step "remove" "Removing" "old .mdl files"
if [ "$skip_step" = false ]; then
    if [ "$isrelease" = true ] ; then
        # Removing Old .mdl files from the opt/ww2mods directory
        find "$WW2MODS" -name "*.mdl" -delete
    fi
fi

if [ "$myos" == "win" ] ; then
    # Building New .mdl files - Win
    cp "$WW2MODS/Launch Files/ww2mods.bat" "$WW2MODS/."
    chmod 550 "$WW2MODS/ww2mods.bat"
    build_step "copy" "Copying" "Debug binaries into WW2MODS"
    if [ "$skip_step" = false ]; then
        for product in $PRODUCTS ; do
            cp "$DEBUGDIR/$product" "$WW2MODS/$product"
            chmod 550 "$WW2MODS/$product"
        done
    fi

    # Copy 3rd party libraries into WW2MODS
    build_step "copy" "Copying" "3rd party libs into WW2MODS"
    if [ "$skip_step" = false ]; then
        rsync -au "$LIBDIR/" "$WW2MODS/"
    fi

    build_step "build" "Building" "mdl files"
    if [ "$skip_step" = false ]; then
        pushd "$WW2MODS"
        ./ww2mods.bat
        popd
    fi
else
    # Building New .mdl files - Mac
    build_step "build" "Building" "mdl files"
    ww2path="WW2/proj/mac/build/$bld_config/$mac_app_name"
    ww2path=$(echo $ww2path | sed 's/\\//g')
    open -W "$ww2path" --args -BuildMDLOnly -data 1 -datadir "$(pwd)/opt/ww2mods/DATA/" -window 1
fi

mdlfiles=`find opt/ww2mods/DATA -name "*.mdl" -print`
if [ ! -n "$mdlfiles" ] ; then
    exit_msg="Error: Failed to build .mdl files, build aborted"
    exit 1
fi


################################################################

#    Build the archive files.

################################################################

build_step "build" "Building" "archive (.ww2) files"

if [ "$skip_step" = false ]; then
    if [ "$myos" == "win" ] ; then
        pushd "$INSTALLER"

        if [ -f "_PlayArchBuilder.bat" ] ; then
            ./_PlayArchBuilder.bat /$build_release

            ExitStatus=$?
            if [ "$ExitStatus" != "0" ] ; then
                exit_msg="Error: _PlayArchBuilder.bat returned: "$ExitStatus" Exit Stastus, build aborted"
                exit 1
            fi

        else
            exit_msg="Error: Failed to locate: "$INSTALLER"/_PlayArchBuilder.bat , build aborted"
            exit 1
        fi

        popd

        echo
        echo
    else
        xcodebuild -project archivetools/playbuilder.xcodeproj -scheme 'playbuilder Release' clean build
        archivetools/build/Release/playbuilder --verbose opt/ww2mods/DATA WW2Archives
    fi
fi


################################################################

# Building the Client files for the Installer (Win and Mac)

################################################################

# If not tagged as official release then we want beta
if [ "$isrelease" = true ] ; then
    bld_config="Release"
    bld_config_dir=$bld_config
else
    bld_config="BetaRelease"
    bld_config_dir=$bld_config
fi

build_step "build" "Building" "release version"
if [ "$skip_step" = false ]; then

    extradefines="$2"
    if [ "$myos" == "win" ] ; then
        BUILDFILE="$BUILDDIR\\$bld_config$BUILDLOG"
        winpath2bash "$BUILDFILE"
        rm "$bashpath"
        echo "writing output to: "$BUILDFILE
        if [ "$isrelease" = true ] ; then
            echo "Cleaning "$bld_config" first"
            devenv WW2.sln /clean $bld_config
        else
            set CL=/D"$extradefines"
        fi
        devenv WW2.sln /build $bld_config /out "$BUILDFILE"
    
        errOutput=`grep ": error|: fatal" "$BUILDFILE"`
        if [ -n "$errOutput" ] ; then
            echo $errOutput
            exit_msg="Error: Build failed, see "$BUILDFILE" for errors"
            exit 1
        fi
    else
        if [ "$isrelease" = true ] ; then
            echo "Cleaning build"
            xcodebuild -project WW2/WW2OL.xcodeproj -scheme $bld_config' Client' clean PROVISIONING_PROFILE= CODE_SIGN_IDENTITY= WARNING_CFLAGS=-w
        fi
        xcodebuild -project WW2/WW2OL.xcodeproj -scheme $bld_config' Client' build PROVISIONING_PROFILE= WARNING_CFLAGS=-w GCC_PREPROCESSOR_DEFINITIONS='$GCC_PREPROCESSOR_DEFINITIONS '$extradefines
    fi
fi

if [ "$myos" == "win" ] ; then
    build_step "copy" "Copying" "executable products - Win"
    if [ "$skip_step" = false ]; then
        pushd "$INSTALLER"

        if [ -f "_PrepareNewRelease.bat" ] ; then
            ./_PrepareNewRelease.bat /$build_release

            ExitStatus=$?
            if [ "$ExitStatus" != "0" ] ; then
                exit_msg="Error: _PrepareNewRelease.bat returned: "$ExitStatus" Exit Stastus, build aborted"
                exit 1
            fi
        
        else
            exit_msg="Error: Failed to locate: "$INSTALLER"/_PrepareNewRelease.bat , build aborted"
            exit 1
        fi

        popd

        case $build_release in
            $Client_release)
                chmod -R 550 $INSTALLER/release-candidate/.
                ;;
            $Client_alpha)
                chmod -R 550 $INSTALLER/release-alpha/.
                ;;
            $Client_beta)
                chmod -R 550 $INSTALLER/release-beta/.
                ;;
        esac
    fi
fi

if [ "$myos" == "win" ] ; then
    if [ "$build_patch" = true ] ; then
        build_step "create" "Creating" "diff files for patcher"
        if [ "$skip_step" = false ]; then
            pushd "$INSTALLER"
            if [ -f "_MakeDiffs.bat" ] ; then
                ./_MakeDiffs.bat /$build_release

            ExitStatus=$?
            if [ "$ExitStatus" != "0" ] ; then
                exit_msg="Error: _MakeDiffs.bat returned: "$ExitStatus" Exit Stastus, build aborted"
                exit 1
            fi

            else
                exit_msg="Error: Failed to locate - "$INSTALLER"/_MakeDiffs.bat, build aborted"
                exit 1
            fi
            popd
        fi

        build_step "build" "Building" "complete running patch install scripts: Patch"
        if [ "$skip_step" = false ]; then
            pushd "$INSTALLER"
            if [ -f "$InstallerScriptName" ] ; then
                iscc "$InstallerScriptName" /dm_ReleaseType=$build_release /dm_BuildType=patch
            else
                exit_msg="Error: Failed to locate - "$INSTALLER"/"$InstallerScriptName", Patch Install build aborted"
                exit 1
            fi
            popd
        fi


################################################################

# TODO
# Rollback Patcher is INCOMPLETE.

################################################################

#        build_step "create" "Creating" "diff files for patcher (rollback)"
#        if [ "$skip_step" = false ]; then
#            pushd "$INSTALLER"
#            if [ -f "_MakeDiffs.bat" ] ; then
#                ./_MakeDiffs.bat /$build_release
#            else
#                exit_msg="Error: Failed to locate - "$INSTALLER"/_MakeDiffs.bat, build aborted"
#                exit 1
#            fi
#            popd
#        fi

#        build_step "build" "Building" "complete running patch install scripts: Rollback"
#        if [ "$skip_step" = false ]; then
#            pushd "$INSTALLER"
#            if [ -f "$InstallerScriptName" ] ; then
#                iscc "$InstallerScriptName" /dm_ReleaseType=$build_release /dm_BuildType=rollback
#            else
#                exit_msg="Error: Failed to locate - "$INSTALLER"/"$InstallerScriptName", Rollback Install build aborted"
#                exit 1
#            fi
#            popd
#        fi
    fi

    build_step "build" "Building" "complete running full install scripts: Full Install"
    if [ "$skip_step" = false ]; then
        pushd "$INSTALLER"
        if [ -f "$InstallerScriptName" ] ; then
            iscc "$InstallerScriptName" /dm_ReleaseType=$build_release /dm_BuildType=full
        else
            exit_msg="Error: Failed to locate - "$INSTALLER"/"$InstallerScriptName", Full Install build aborted"
            exit 1
        fi
        popd
    fi
else
    build_step "create" "Creating" "dmg file"
    if [ "$skip_step" = false ]; then
        mkdir -p "$HOME/Library/Application Support/DropDMG/"{Configurations,Layouts}
        cp 'WW2/proj/mac/WW2 Release Items/DropDMG/Configurations/ww2.plist' "$HOME/Library/Application Support/DropDMG/Configurations"
        cp -r 'WW2/proj/mac/WW2 Release Items/DropDMG/Layouts/ww2' "$HOME/Library/Application Support/DropDMG/Layouts"
        plutil -replace 'content.options.destinationFolder' -string "$( pwd )/Installer/release/macrelease" "$HOME/Library/Application Support/DropDMG/Configurations/ww2.plist"
        app_path='WW2/proj/mac/build/'$bld_config_dir'/'$mac_app_name
        app_path=$(echo $app_path | sed 's/\\//g')
        /Applications/DropDMG.app/Contents/Frameworks/DropDMGFramework.framework/Versions/A/dropdmg --config-name ww2 "$app_path"
    fi
fi

if [ "$build_steam" = true ] ; then
    build_step "deploy" "Deploying" "to Steam"
    if [ "$skip_step" = false ]; then
        echo "When prompted to allow steam accesss to the network, enable it please"
        if [ "$myos" == "win" ] ; then
            pushd "Libraries/SteamSDK/tools/ContentBuilder/builder"
            ./steamcmd.exe +login $steam_user $steam_pwd +run_app_build "../../../../../Deployment/steam/scripts/app_pc_build_251950.vdf" +quit
            popd
        else
            pushd "Libraries/SteamSDK/tools/ContentBuilder/builder_osx"
            ./steamcmd.sh +login $steam_user $steam_pwd +run_app_build "../../../../../Deployment/steam/scripts/app_mac_build_"$bld_config"_251950.vdf" +quit
            popd
        fi
    fi
fi


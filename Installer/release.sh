#!/bin/bash
################################################################
# release.sh
#
# Supported platforms: Windows
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
print_exit_msg
sleep 5
' EXIT

WWII_ROOT=$(pwd)
WW2MODS="$WWII_ROOT/opt/ww2mods"
WW2ARCHIVES="$WWII_ROOT/WW2Archives"
INSTALLER="$WWII_ROOT/Installer"
DEBUGDIR="$WWII_ROOT/build/client/Debug/Bin"
LIBDIR="$WWII_ROOT/Installer/libs"
RELEASEDIR="$WWII_ROOT/build/client/Release/Bin"
#RELEASELIBDIR="$WWII_ROOT/Build/lib/Win32/Release"
PRODUCTS="WW2.exe Playgate.exe"
DATADIRS="DirectXRedist loc playgate screens"
DATAFILES="eula.txt license.rtf readme.rtf regions.txt tos.txt wwii1.ico"
BUILDDIR="$APPDATA"
BUILDLOG="build_output.log"
PREV_RELEASE="$PROGRAMFILES\\CRS\\Battleground Europe\\"

skipto=$1
if (( skipto > 0 )) ; then
    echo "Will skip to step "$skipto
fi

buildstep=1
skip_step=false

#pause_enabled=true
pause_enabled=false

build_step "check" "Checking" "pre-requisites"

found_prereq_errors=false
exit_msg=""

isccpath=$(which iscc)
if [ ! -x "$isccpath" ] ; then
    exit_msg=${exit_msg}'|Inno setup install directory not in windows path, please add it'
    found_prereq_errors=true
fi

devenvpath=$(which devenv)
if [ ! -x "$devenvpath" ] ; then
    exit_msg=${exit_msg}'|Microsoft build (devenv) not in windows path, please add it'
    found_prereq_errors=true
fi

gitpath=$(which git)
if [ ! -x "$gitpath" ] ; then
    exit_msg=${exit_msg}'|Git bin directory not in windows path, please add it'
    found_prereq_errors=true
fi

# make sure we are executing from the right directory
if [ ! "$(basename $(pwd))" == "wwiiolng" ] ; then
    exit_msg=${exit_msg}'|This command must be executed from the root dir: wwiiolng'
    found_prereq_errors=true
fi

# make sure the opt/ww2mods directory exists
if [ ! -d "opt/ww2mods" ] ; then
    exit_msg=$exit_msg'|ww2mods is not loaded (or linked) through opt directory, please correct'
    found_prereq_errors=true
fi

# make sure the archives directory exists
if [ ! -d "WW2Archives" ] ; then
    exit_msg=$exit_msg'|WW2Archives does not exist, please create'
    found_prereq_errors=true
fi

build_patch=true
while [ ! -d "$PREV_RELEASE" ] ; do
    echo "Standard location for previous build "$PREV_RELEASE" does not exist"
    echo -n "Please enter the directory containing the previous release (ENTER for none): "
    read -r HANDLE
    HANDLE="$HANDLE\\"
    if [ -z "$HANDLE" ] ; then
        echo "WARNING: No previous release specified, PC patch will not be built!"
        build_patch=false 
	break
    elif [ ! -r "$HANDLE" ] ; then
        echo "Invalid directory, '$HANDLE' try again"
    else
        PREV_RELEASE="$HANDLE"
    fi
done

build_steam=false
steam_user=""
steam_pwd=""

while [ ! -d "$PREV_RELEASE" ] ; do
    echo "Standard location for previous build "$PREV_RELEASE" does not exist"
    echo -n "Please enter the directory containing the previous release (ENTER for none): "
    read -r HANDLE
    HANDLE="$HANDLE\\"
    if [ -z "$HANDLE" ] ; then
        echo "WARNING: No previous release specified, PC patch will not be built!"
        build_patch=false 
	break
    elif [ ! -r "$HANDLE" ] ; then
        echo "Invalid directory, '$HANDLE' try again"
    else
        PREV_RELEASE="$HANDLE"
    fi
done

# make sure the previous release for the patch to apply exists
if [ "$build_patch" = true ] ; then
    if [ ! -f "$PREV_RELEASE/WW2.exe" ] ; then
        exit_msg=$exit_msg'|The previous release directory ($PREV_RELEASE) is not valid, restart when correct directory located'
        found_prereq_errors=true
    fi
fi

# make sure visual studio isn't running
vsProc=`ps -W | grep devenv`
if [ -n "$vsProc" ] ; then
    exit_msg=$exit_msg'Visual studio cannot be running when starting the build, please close all instances'
    found_prereq_errors=true
fi

if [ "$found_prereq_errors" = true ] ; then
    exit 1
fi

noprompt=false
read -p "Build assuming all default responses? (y|n) :" -n 1 -r
echo    # (optional) move to a new line
if [[ $REPLY =~ ^[Yy]$ ]] ; then
    noprompt=true
fi

gitBranch=`git rev-parse --abbrev-ref HEAD`
arrayBranchElements=(${gitBranch//\// })
releasetype=$arrayBranchElements[0]
releaseversion=$arrayBranchElements[1]

isrelease=false

if [ "$noprompt" = false ]; then
    read -p "Is this a formal release? (y|n) :" -n 1 -r
    echo    # (optional) move to a new line
else
    REPLY='y'
fi

if [[ $REPLY =~ ^[Yy]$ ]] ; then
    if [ "$releasetype" != "release" ] ; then
        exit_msg="Formal releases must be on a release branch, create a release branch and restart build"
	exit 1
    fi
    echo "Release version is "$releaseversion
    isrelease=true
else
    if [ "$releasetype" != "feature" ] ; then
        if [ "$noprompt" = false ]; then
            echo "WARNING! You are building from a feature branch which is normally unwise"
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

build_step "test" "Testing" "repositories"

cleanedfiles=`git clean -d -x -n --exclude="Installer/release.sh" && git -C opt/ww2mods clean -d -x -n`
if [ -n "$cleanedfiles" ] ; then
    clean=false
    echo "Your repositories are not clean"
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
            echo $cleanedfiles | sed 's/Would /\nWould /g'
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
        pushd "Libraries/SteamSDK/tools/ContentBuilder/builder"
	./steamcmd.exe +login $steam_user $steam_pwd +quit
        popd
	echo "Preparing Steam build environment"
    else
	echo "Warning: No user specified, will not deploy to steam"
	build_steam=false
    fi
fi

if [ "$build_patch" = true ] ; then
    winpath2bash "$PREV_RELEASE"
    build_step "copy" "copying" "over previous release for patch from $bashpath"
    if [ "$skip_step" = false ]; then
        rsync -auv "$bashpath/" "$INSTALLER/previous/"
    fi
fi

echo "TODO: libxml prebuild"

build_step "build" "Building" "debug version"
if [ "$skip_step" = false ]; then
    bld_config="Debug"
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
fi

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

build_step "remove" "Removing" "old .mdl files"
if [ "$skip_step" = false ]; then
    find "$WW2MODS" -name "*.mdl" -delete
    cp "$WW2MODS/Launch Files/ww2mods.bat" "$WW2MODS/."
    chmod 550 "$WW2MODS/ww2mods.bat"
fi

build_step "build" "Building" "mdl files"
if [ "$skip_step" = false ]; then
    pushd "$WW2MODS"
    ./ww2mods.bat
    popd

    mdlfiles=`find "$WW2MODS" -name "*.mdl" -print`
    if [ -n "$mdlfiles" ] ; then
        exit_msg="Error: Failed to build .mdl files, build aborted"
        exit 1
    fi
fi

build_step "build" "Building" "archive (.ww2) files"
if [ "$skip_step" = false ]; then

    "$WWII_ROOT/Tools/PlayBuilder/run.bat"

    # remove the empty archive file
    rm -f WW2Archives/playarc11.ww2
    echo
    echo
fi

build_step "copy" "Copying" "product files"
if [ "$skip_step" = false ]; then

    ### Copy build products to Installer
    find "$WW2ARCHIVES" -path "*/.sync" -prune -o -name "*.ww2" -print -exec cp {} "$INSTALLER/current/" \;
fi

build_step "build" "Building" "release version"
if [ "$skip_step" = false ]; then

    # If not tagged as official release then we want beta
    if [ "$isrelease" = true ] ; then
        bld_config="Release"
    else
        bld_config="BetaRelease"
    fi

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
fi

build_step "copy" "Copying" "executable products"
if [ "$skip_step" = false ]; then

    # Copy executables over
    for product in $PRODUCTS ; do
        srcfile="$WWII_ROOT/build/client/$bld_config/Bin/$product"
        dstfile="$INSTALLER/current/$product"
        echo "Copying $srcfile to $dstfile"
        cp $srcfile $dstfile
        echo "Setting perms on $dstfile"
        chmod 550 $dstfile
    done
    cp $WWII_ROOT/Installer/previous/Settings.exe $WWII_ROOT/Installer/current/.
    chmod 550 $WWII_ROOT/Installer/current/.
fi

build_step "copy" "Copying" " dlls and libraries"
if [ "$skip_step" = false ]; then
    rsync -auv "$INSTALLER/libs/" "$INSTALLER/current/"
fi

build_step "copy" "Copying" "WW2MODS datafiles datafiles"
if [ "$skip_step" = false ]; then
    for product in $DATAFILES ; do
        cp "$WW2MODS/$product" "$INSTALLER/current/$product"
        chmod 440 "$INSTALLER/current/$product"
    done
fi

build_step "copy" "Copying" "data directories"
if [ "$skip_step" = false ]; then
    for folder in $DATADIRS ; do
        mkdir -p "Installer/current/DATA/$folder"
        rsync -auv --exclude=".DS_Store" "$WW2MODS/DATA/$folder/" "$INSTALLER/current/DATA/$folder/"
    done
fi

build_step "prune" "Pruning" "unwanted files"
if [ "$skip_step" = false ]; then

    pushd "$INSTALLER/current"
    for file in WW2.{lib,ilk,exp} Playgate.{pbd,map,ilk} *.tkt *.bat ; do
        if [[ -e "$file" ]] ; then
            rm "$file"
	fi
    done
    popd
fi

prefix="Release"
if [ "$isrelease" = false ] ; then
    prefix="Beta"
fi

if [ "$build_patch" = true ] ; then
    build_step "create" "Creating" "diff files for patcher"
    if [ "$skip_step" = false ]; then
        pushd "$INSTALLER"
        ./MakeDiffs.bat
        popd
    fi

    fname="Installer/$prefix - Patch.iss"
    build_step "build" "Building" "complete running patch install scripts: "$fname
    if [ "$skip_step" = false ]; then
        iscc "$fname"
    fi
fi

fname="Installer/$prefix - FullInstall.iss"
build_step "build" "Building" "complete running full install scripts: "$fname
if [ "$skip_step" = false ]; then
    iscc "$fname"
fi


if [ "$build_steam" = true ] ; then
    build_step "deploy" "Deploying" "to Steam"
    if [ "$skip_step" = false ]; then
        echo "When prompted to allow steam accesss to the network, enable it please"
        pushd "Libraries/SteamSDK/tools/ContentBuilder/builder"
        ./steamcmd.exe +login $steam_user $steam_pwd +run_app_build "../../../../../Deployment/steam/scripts/app_pc_build_251950.vdf" +quit
        popd
    fi
fi


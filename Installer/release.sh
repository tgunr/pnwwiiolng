#!/bin/bash

function pause {
    if [ "$pause_enabled" = true ] ; then
        out_msg='About to '$1$' '$3$' press any key to continue'
        read -p "$out_msg" -n 1 -r 
        echo    # (optional) move to a new line
	echo $2" "$3
    else
	echo $2" "$3
    fi
}

function print_exit_msg {
IFS="|" read -r -a msgs <<< "$exit_msg"
echo    # (optional) move to a new line
for msg in "${msgs[@]}"
do
    echo "$msg"
done
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

pause_enabled=true

pause "check" "Checking" "pre-requisites"

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

# make sure the previous release for the patch to apply exists
if [ ! -f "$INSTALLER/previous/WW2.exe" ] ; then
    exit_msg=$exit_msg'|The previous release has not been copied into target dir "$INSTALLER"/previous yet, please do so now'
    found_prereq_errors=true
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

echo "TODO: libxml prebuild"

pause "build" "Building" "debug version"

bld_config="Debug"
BUILDFILE="$BUILDDIR\\$bld_config$BUILDLOG"
echo "writing output to: "$BUILDFILE
if [ "$isrelease" = true ] ; then
    echo "Cleaning "$bld_config" first"
    devenv WW2.sln /clean $bld_config
fi
devenv WW2.sln /build $bld_config /out "$BUILDFILE"

errOutput=`grep ": error" "$BUILDFILE"`
if [ -n "$errOutput" ] ; then
    echo $errOutput
    exit_msg="Error: Build failed, see "$BUILDFILE" for errors"
    exit 1
fi

pause "copy" "Copying" "Debug binaries into WW2MODS"

for product in $PRODUCTS ; do
	cp "$DEBUGDIR/$product" "$WW2MODS/$product"
	chmod 550 "$WW2MODS/$product"
done

# Copy 3rd party libraries into WW2MODS
rsync -au "$LIBDIR/" "$WW2MODS/"

pause "remove" "Removing" "old .mdl files"

find "$WW2MODS" -name "*.mdl" -delete
cp "$WW2MODS/Launch Files/ww2mods.bat" "$WW2MODS/."
chmod 550 "$WW2MODS/ww2mods.bat"

pause "build" "Building" "mdl files"

pushd "$WW2MODS"
./ww2mods.bat
popd

mdlfiles=`find "$WW2MODS" -name "*.mdl" -print`
if [ -n "$mdlfiles" ] ; then
    exit_msg="Error: Failed to build .mdl files, build aborted"
    exit 1
fi

pause "build" "Building" "archive (.ww2) files"

"$WWII_ROOT/Tools/PlayBuilder/run.bat"

# remove the empty archive file
rm -f WW2Archives/playarc11.ww2
echo
echo

pause "copy" "Copying" "product files"

### Copy build products to Installer
find "$WW2ARCHIVES" -path "*/.sync" -prune -o -name "*.ww2" -print -exec cp {} "$INSTALLER/current/" \;

pause "build" "Building" "release version"

# If not tagged as official release then we want beta
if [ "$isrelease" = true ] ; then
    bld_config="Release"
else
    bld_config="BetaRelease"
fi

BUILDFILE="$BUILDDIR\\$bld_config$BUILDLOG"
echo "writing output to: "$BUILDFILE
if [ "$isrelease" = true ] ; then
    echo "Cleaning "$bld_config" first"
    devenv WW2.sln /clean $bld_config
fi
devenv WW2.sln /build $bld_config /out "$BUILDFILE"

errOutput=`grep ": error" "$BUILDFILE"`
if [ -n "$errOutput" ] ; then
    echo $errOutput
    exit_msg="Error: Build failed, see "$BUILDFILE" for errors"
    exit 1
fi

pause "copy" "Copying" "executable products"

# Copy executables over
for product in $PRODUCTS ; do
    if [[ "$BUILDDIR$bld_config/Bin//$product" -nt "$INSTALLER/current/$product" ]] ; then
	   cp "$BUILDDIR$bld_config/Bin/$product" "$INSTALLER/current/$product"
	   chmod 550 "$INSTALLER/current/$product"
    fi
done
cp $WWII_ROOT/Installer/previous/Settings.exe $WWII_ROOT/Installer/current/.
chmod 550 $WWII_ROOT/Installer/current/.
   
pause "copy" "Copying" " dlls and libraries"

cp $WWII_ROOT/Installer/previous/libzmq.dll $WWII_ROOT/Installer/current/.
chmod 440 $WWII_ROOT/Installer/current/.

rsync -auv "$INSTALLER/libs/" "$INSTALLER/current/"

pause "copy" "Copying" "WW2MODS datafiles datafiles"

for product in $DATAFILES ; do
	cp "$WW2MODS/$product" "$INSTALLER/current/$product"
	chmod 440 "$INSTALLER/current/$product"
done

pause "copy" "Copying" "data directories"

for folder in $DATADIRS ; do
    mkdir -p "$INSTALLER/current/DATA/$folder"
    rsync -auv --exclude=".DS_Store" "$WW2MODS/DATA/$folder/" "$INSTALLER/current/DATA/$folder/"
done

pause "prune" "Pruning" "unwanted files"

pushd "$INSTALLER/current"
for file in WW2.{lib,ilk,exp} Playgate.{pbd,map,ilk} *.tkt *.bat ; do
	if [[ -e "$file" ]] ; then
		rm "$file"
	fi
done
popd

pause "create" "Creating" "diff files for patcher"

pushd "$INSTALLER"
./MakeDiffs.bat
popd

pause "build" "Building" "complete running patch install scripts"

iscc "$INSTALLER/release/winrelease/Patch Installer.iss"

pause "build" "Building" "complete running full install scripts"

iscc "$INSTALLER/release/winrelease/Full Installer.iss"


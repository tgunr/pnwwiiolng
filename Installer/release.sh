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
DATAFILES="steam_appid.txt eula.txt license.rtf readme.rtf regions.txt tos.txt wwii1.ico"
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

myos=""
if [ "$OSTYPE" == "cygwin" ] ; then
    echo "Detected Windows under cygwin"
    myos="win"
elif [[ "${OSTYPE#*darwin}" != "$OSTYPE" ]] ; then
    echo "Detected OSX"
    echo "WARNING: If Xcode is running it will be terminated before the build starts.  Save any unsaved work!."
    echo "WARNING: DO NOT DO ANYTHING while build is running, especially when DropDMG is running!!!!"
    myos="osx"
else
    exit_msg="Unknown OS, make sure you are running this from a bash shell on the Mac, or Mobaxterm on Windows"
    exit 1
fi

if [ "$myos" == "win" ] ; then
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
else
    min_xcode_version_major=8
    min_xcode_version_minor=2
    
    #validate xcode version
    xcodepath=$(which xcodebuild)
    if [ ! -x "$xcodepath" ] ; then
        exit_msg=${exit_msg}'|XCode is not available, install version $min_xcode_version_major.$min_xcode_version_minor or greater'
        found_prereq_errors=true
    else
        xcode_version=$(xcodebuild -version)
        version_rsp=(${xcode_version// / })
        version_nums=(${version_rsp[1]//\./ })
        if (( version_nums[0] < min_xcode_version_major ))  || (( version_nums[1] < min_xcode_version_minor )); then
            exit_msg=${exit_msg}'|Xcode version incorrect, update to version '$min_xcode_version_major'.'$min_xcode_version_minor' or greater' 
            found_prereq_errors=true
        fi
    fi
fi

gitpath=$(which git)
if [ ! -x "$gitpath" ] ; then
    exit_msg=${exit_msg}'|Git not found, please install and/or add to path'
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

if [ "$myos" == "win" ] ; then
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

else
    build_patch=false
fi

build_steam=false
steam_user=""
steam_pwd=""

# make sure the previous release for the patch to apply exists
if [ "$build_patch" = true ] ; then
    if [ ! -f "$PREV_RELEASE/WW2.exe" ] ; then
        exit_msg=$exit_msg'|The previous release directory ($PREV_RELEASE) is not valid, restart when correct directory located'
        found_prereq_errors=true
    fi
fi

if [ "$myos" == "win" ] ; then
    # make sure visual studio isn't running
    vsProc=`ps -W | grep devenv`
    if [ -n "$vsProc" ] ; then
        exit_msg=$exit_msg'|Visual studio cannot be running when starting the build, please close all instances'
        found_prereq_errors=true
    fi
else
    # check mac specific issues
    playgate_version=$(grep -r -m 1 -h 'PLAYGATE_VERSION' WW2/src)
    pn_elements=(${playgate_version// / })
    if [ ! "${pn_elements[2]}" == "0x01010109" ] ; then
        exit_msg=$exit_msg'|Playgate version is not correct ('${pn_elements[2]}') should be 0x01010109, please correct'
        found_prereq_errors=true
    fi
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

if [ "$myos" == "osx" ] ; then
    osascript -e 'tell app "Xcode" to quit'
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

if [ "$build_patch" = true ] ; then
    winpath2bash "$PREV_RELEASE"
    build_step "copy" "copying" "over previous release for patch from $bashpath"
    if [ "$skip_step" = false ]; then
        rsync -auv "$bashpath/" "$INSTALLER/previous/"
    fi
fi

echo "TODO: libxml prebuild"

mac_product_base_name="World\ War\ II\ Online"
mac_product_name=$mac_product_base_name
if [ "$isrelease" = false ] && [ "$myos" == "osx" ] ; then
    echo "Updating CFBundleName and PRODUCT_NAME"
    plutil -replace CFBundleName -string "$mac_product_name"  WW2/proj/mac/Info.plist

    mac_product_name=$mac_product_name"\ Beta"
    target_file='WW2/WW2OL.xcodeproj/project.pbxproj'
    grepresult=$(grep "$mac_product_name" $target_file)
    if [ ! -n "$grepresult" ] ; then
        sedargs='s/'$mac_product_base_name'/'$mac_product_name'/g'
        sed -i -e "$sedargs" "$target_file"
    fi
    touch WW2Archives/playarc.ww2
fi
mac_app_name=$mac_product_name".app"

build_step "build" "Building" "debug version"
if [ "$skip_step" = false ]; then
    bld_config="Debug"
    if [ "$myos" == "win" ] ; then
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
        if [ "$isrelease" = true ] ; then
            echo "Cleaning build"
            xcodebuild -project WW2/WW2OL.xcodeproj -scheme $bld_config' Client' clean PROVISIONING_PROFILE= CODE_SIGN_IDENTITY= WARNING_CFLAGS=-w
        fi
        xcodebuild -project WW2/WW2OL.xcodeproj -scheme $bld_config' Client' build PROVISIONING_PROFILE= CODE_SIGN_IDENTITY= WARNING_CFLAGS=-w
    fi
fi

build_step "remove" "Removing" "old .mdl files"
if [ "$skip_step" = false ]; then
    if [ "$isrelease" = true ] ; then
        find "$WW2MODS" -name "*.mdl" -delete
    fi
fi

if [ "$myos" == "win" ] ; then
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
    build_step "build" "Building" "mdl files"
    ww2path="WW2/proj/mac/build/$bld_config/$mac_app_name"
    ww2path=$(echo $ww2path | sed 's/\\//g')
    open -W "$ww2path" --args -BuildMDLOnly -data 1 -datadir "$(pwd)/opt/ww2mods/DATA/" -window 1
fi

mdlfiles=`find opt/ww2mods -name "*.mdl" -print`
if [ ! -n "$mdlfiles" ] ; then
    exit_msg="Error: Failed to build .mdl files, build aborted"
    exit 1
fi

build_step "build" "Building" "archive (.ww2) files"
if [ "$skip_step" = false ]; then

    if [ "$myos" == "win" ] ; then
        "$WWII_ROOT/Tools/PlayBuilder/run.bat"

        # remove the empty archive file
        rm -f WW2Archives/playarc11.ww2
        echo
        echo
    else
        xcodebuild -project archivetools/playbuilder.xcodeproj -scheme 'playbuilder Release' clean build
        archivetools/build/Release/playbuilder --verbose opt/ww2mods/DATA WW2Archives
    fi
fi

if [ "$myos" == "win" ] ; then
    build_step "copy" "Copying" "production files to installer directory"
    if [ "$skip_step" = false ]; then

        ### Copy build products to Installer
        find "$WW2ARCHIVES" -path "*/.sync" -prune -o -name "*.ww2" -print -exec cp {} "$INSTALLER/current/" \;
    fi
fi

build_step "build" "Building" "release version"
if [ "$skip_step" = false ]; then

    # If not tagged as official release then we want beta
    if [ "$isrelease" = true ] ; then
        bld_config="Release"
    else
        if [ "$myos" == "win" ] ; then
            bld_config="BetaRelease"
        else
            # NOTE: TODO - we should have a BetaRelease scheme on Mac side
            bld_config="Release"
        fi
    fi

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
            SET CL=/D"$extradefines"
        fi
        devenv WW2.sln /build $bld_config /out "$BUILDFILE"
    
        errOutput=`grep ": error|: fatal" "$BUILDFILE"`
        if [ -n "$errOutput" ] ; then
            echo $errOutput
            exit_msg="Error: Build failed, see "$BUILDFILE" for errors"
            exit 1
        fi
    else
        codesign=''
        if [ "$isrelease" = true ] ; then
            echo "Cleaning build"
            xcodebuild -project WW2/WW2OL.xcodeproj -scheme $bld_config' Client' clean PROVISIONING_PROFILE= CODE_SIGN_IDENTITY= WARNING_CFLAGS=-w
            codesign='Developer ID Application: Playnet Inc (J63A54QWD2)'
        fi
        xcodebuild -project WW2/WW2OL.xcodeproj -scheme $bld_config' Client' build PROVISIONING_PROFILE= CODE_SIGN_IDENTITY=$codesign WARNING_CFLAGS=-w GCC_PREPROCESSOR_DEFINITIONS='$GCC_PREPROCESSOR_DEFINITIONS $extradefines'
    fi
fi

if [ "$myos" == "win" ] ; then
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
fi

prefix="Release"
if [ "$isrelease" = false ] ; then
    prefix="Beta"
fi

if [ "$myos" == "win" ] ; then
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
else
    build_step "create" "Creating" "dmg file"
    pushd "WW2/proj/mac/WW2 Release Items/DropDMG"
    unzip -o DropDMG.zip
    popd
    mkdir -p "$HOME/Library/Application Support/DropDMG/"{Configurations,Layouts}
    cp 'WW2/proj/mac/WW2 Release Items/DropDMG/Configurations/ww2.plist' "$HOME/Library/Application Support/DropDMG/Configurations"
    cp -r 'WW2/proj/mac/WW2 Release Items/DropDMG/Layouts/ww2' "$HOME/Library/Application Support/DropDMG/Layouts"
    plutil -replace 'content.options.destinationFolder' -string "$( pwd )/Installer/release/macrelease" "$HOME/Library/Application Support/DropDMG/Configurations/ww2.plist"
    app_path='WW2/proj/mac/build/'$bld_config'/'$mac_app_name
    app_path=$(echo $app_path | sed 's/\\//g')
    WW2/proj/mac/WW2\ Release\ Items/DropDMG/DropDMG.app/Contents/Frameworks/DropDMGFramework.framework/Versions/A/dropdmg --config-name ww2 "$app_path"
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
            ./steamcmd.sh +login $steam_user $steam_pwd +run_app_build "../../../../../Deployment/steam/scripts/app_mac_build_251950.vdf" +quit
            popd
        fi
    fi
fi


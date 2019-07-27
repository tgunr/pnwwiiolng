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

# Notes for the Mac build:
# The Mac build rewrites the Xcode project to set the bundle to
# the desired name. It does so by appending the build to the end
# of the pre-existing name. This can result in the name having
# multiple "alpha", "beta" etc tags at its end, resulting in a
# bundle named such that this script cannot locate to use. To
# solve this the script saves the current files then restores
# them later.
#
# The backup files are:
#     WW2OL.xcodeproj -> WW2OL_BUILD_BAK.xcodeproj
#     Info.plist      -> Info_BUILD_BAK.plist
#
# If the script aborts before the files are restored the
# contaminated files WILL cause issues and should be deleted
# and the saved files renamed to take their place.
#
# 25-June-2019
################################################################


function build_step {
    if [ "$pause_enabled" = true ] ; then
        out_msg='About to '$1$' '$3$' press any key to continue'
        read -p "$out_msg" -n 1 -r 
        echo    # (optional) move to a new line
        echo "   >>>>>>>>>>    ==============================       Step "$buildstep". "$2" "$3
    else
        echo "   >>>>>>>>>>    ==============================       Step "$buildstep". "$2" "$3
    fi

    if (( skipto > 0 )) ; then
        if (( skipto > buildstep )); then
            skip_step=true
        else
            skip_step=false
        fi
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

function bashpath2win {
    workingBashPath=$1
    # echo "01 -- Input Path                  : >"$workingBashPath"<"

    workingBashPath=${workingBashPath#/drives/}
    # echo "02 -- /drives/ removed            : >"$workingBashPath"<"

    driveLetter=${workingBashPath::1}
    # echo "03 -- Drive Letter                : >"$driveLetter"<"
    
    driveLetter=${driveLetter^^}
    # echo "04 -- Drive Letter to Upper Case  : >"$driveLetter"<"

    workingBashPath=${workingBashPath:1}
    # echo "05 -- Drive Letter Removed        : >"$workingBashPath"<"

    workingBashPath=$driveLetter":"$workingBashPath
    # echo "06 -- Corrected Windows Path      : >"$workingBashPath"<"
    
    workingBashPath=$(echo "$workingBashPath" | sed 's.\/.\\.g')
    # echo "07 -- Corrected Windows Separator : >"$workingBashPath"<"
    
    winpath="$workingBashPath"
}

seconds2time ()
{
    T=$1
    D=$((T/60/60/24))
    H=$((T/60/60%24))
    M=$((T/60%60))
    S=$((T%60))

    if [[ ${D} != 0 ]] ; then
        displayTime=$(printf '%d days %02d:%02d:%02d' $D $H $M $S)
    else
        displayTime=$(printf '%02d:%02d:%02d' $H $M $S)
    fi
}

trap '
echo    # (optional) move to a new line
echo $(date +%F_%T)"  >>>>  Exiting . . ."
print_exit_msg
echo
echo "-----------------------------------------------------------------------------------------------------------------------"
sleep 5
' EXIT

################################################################

#    Configure

################################################################

WWII_ROOT=$(pwd)
WW2MODS="$WWII_ROOT/opt/ww2mods"
WW2ARCHIVES_ROOT=""
WW2ARCHIVES_RELEASE=""
INSTALLER_ROOT=""
INSTALLER_RELEASE=""
DEBUGDIR="$WWII_ROOT/build/client/Debug/Bin"
RELEASEDIR="$WWII_ROOT/build/client/Release/Bin"
PRODUCTS="WW2.exe Playgate.exe Settings.exe"
DATADIRS="DirectXRedist loc playgate"
DATAFILES="steam_appid.txt eula.txt license.rtf readme.rtf regions.txt tos.txt wwii1.ico"
ASSETFILES="eula.txt license.rtf readme.rtf wwii1.ico"
BUILDDIR="$APPDATA"
BUILDLOG="build_output.log"
PREV_RELEASE="$PROGRAMFILES\\CRS\\WWIIOnline\\"
MDL_COMMANDLINE_ARGS=""

InstallerScriptName="WWIIOL_Installer_Template.iss"
build_release=""
Client_release="rc"
Client_alpha="alpha"
Client_beta="beta"
Client_end="end"

buildstep=1
skip_step=false

#pause_enabled=true
pause_enabled=false

skipto=$1
if (( skipto > 0 )) ; then
    echo "Will skip to step "$skipto
fi


echo
echo
echo "-----------------------------------------------------------------------------------------------------------------------"
echo $(date +%F_%T)"  >>>>  Release Script Starting."
echo
echo "NOTE: Press Ctrl + C to Exit the script."
echo
echo
build_step "configure" "Configing" "script options"
echo
echo
echo "Installer Build Type."
echo "Available Options: -- r = Release Candidate  -  Release EXE Build"
echo "                   -- a = Alpha Release      -  BetaRelease EXE Build"
echo "                   -- b = Beta Release       -  BetaRelease EXE Build"
echo "                   -- e to Exit the script."
echo    # (optional) move to a new line

while read -p "Enter the Installer Build Type ( r|a|b|e ) : "  -n 1 -r ; do
    case $(echo "$REPLY" |tr [:upper:] [:lower:]) in
        "r")
            build_release=$Client_release
            echo
            echo "Note: Release Candidate build."
            break
            ;;

        "a")
            build_release=$Client_alpha
            echo
            echo "Note: Alpha Release build."
            break
            ;;

        "b")
            build_release=$Client_beta
            echo
            echo "Note: Beta Release build."
            break
            ;;

        "e")
            exit_msg="When you are ready to proceed, restart the build process."
            
            exit 1
            ;;

        *)
            echo
            ;;
    esac
done

echo
echo
build_step "check" "Checking" "pre-requisites"

found_prereq_errors=false
exit_msg=""

myos=""

INSTALLER_ROOT="$WWII_ROOT/Installer"

if [ "$OSTYPE" == "cygwin" ] ; then
    echo "Detected Windows under cygwin"

    myos="win"
    WW2ARCHIVES_ROOT="$WWII_ROOT/Installer/ww2assets/oswin/archivefiles"
    
    bashpath2win "$WW2MODS"
    MDL_COMMANDLINE_ARGS="-BuildMDLOnly -window 1 -width 1024 -height 768 -data 1 -datadir $winpath\DATA"

elif [[ "${OSTYPE#*darwin}" != "$OSTYPE" ]] ; then
    echo "Detected OSX"
    echo "WARNING: If Xcode is running it will be terminated before the build starts.  Save any unsaved work!."
    echo "WARNING: DO NOT DO ANYTHING while build is running, especially when DropDMG is running!!!!"

    myos="osx"
    WW2ARCHIVES_ROOT="$WWII_ROOT/Installer/ww2assets/osmac/archivefiles"
    MDL_COMMANDLINE_ARGS="-BuildMDLOnly -window 1 -width 1024 -height 768 -data 1 -datadir $WW2MODS/DATA/"

else
    exit_msg="Pre-requisites ERROR: Unknown OS, make sure you are running this from a bash shell on the Mac, or Mobaxterm on Windows."

    exit 1
fi

gitpath=$(which git)
echo
echo "Directories:"
echo "Git bin directory  ============  :  < "$gitpath" >"
if [ ! -x "$gitpath" ] ; then
    exit_msg=${exit_msg}'|Pre-requisites ERROR: Git bin directory not in windows path, please add it.'
    found_prereq_errors=true
fi

if [ "$myos" == "win" ] ; then
    isccpath=$(which iscc)
    echo "Inno Setup  ===================  :  < "$isccpath" >"
    if [ ! -x "$isccpath" ] ;    then
        exit_msg=${exit_msg}'|Pre-requisites ERROR: Inno setup install directory not in windows path, please add it.'
        found_prereq_errors=true
    fi

    devenvpath=$(which devenv)
    echo "Microsoft build  ==============  :  < "$devenvpath" >"
    if [ ! -x "$devenvpath" ] ; then
        exit_msg=${exit_msg}'|Pre-requisites ERROR: Microsoft build (devenv) not in windows path, please add it.'
        found_prereq_errors=true
    fi
else
    min_xcode_version_major=8
    min_xcode_version_minor=2

    #validate xcode version
    xcodepath=$(which xcodebuild)
    echo "Apple Xcode Path  =============  :  < "$xcodepath" >"
    if [ ! -x "$xcodepath" ] ; then
        exit_msg=${exit_msg}'|Pre-requisites ERROR: XCode is not available. Please install version $min_xcode_version_major.$min_xcode_version_minor or greater.'
        found_prereq_errors=true

    else
        xcode_version=$(xcodebuild -version)
        version_rsp=(${xcode_version// / })
        version_nums=(${version_rsp[1]//\./ })
        echo " * Apple Xcode Version  =======  :  < "$xcode_version" >"
        # echo " * Apple Xcode Version_rsp  ===  :  < "$version_rsp" >"
        # echo " * Apple Xcode Version_nums  ==  :  < "${version_nums[0]}"."${version_nums[1]}"."${version_nums[2]}" >"

        if (( version_nums[0] < min_xcode_version_major )) || (( version_nums[1] < min_xcode_version_minor  && version_nums[0] == min_xcode_version_major )); then
            exit_msg=${exit_msg}'|Pre-requisites ERROR: Xcode version incorrect, update to version '$min_xcode_version_major'.'$min_xcode_version_minor' or greater.' 
            found_prereq_errors=true
        fi

        if (( version_nums[0] > 9 )); then
            exit_msg=${exit_msg}'|Pre-requisites ERROR: Xcode versions later than 9.4.1 are Currently (May 2019) Incompatible with World War II Online builds.'
            found_prereq_errors=true
        fi
    fi
fi

# make sure we are executing from the right directory
if [ ! "$(basename $(pwd))" == "wwiiolng" ] ; then
    exit_msg=${exit_msg}'|Pre-requisites ERROR: This command must be executed from the root dir: wwiiolng.'
    found_prereq_errors=true
fi

# make sure the opt/ww2mods directory exists
if [ ! -d "opt/ww2mods" ] ; then
    exit_msg=$exit_msg'|Pre-requisites ERROR: ww2mods is not loaded (or linked) through opt directory. Please correct it.'
    found_prereq_errors=true
fi

# set the installer root and archive root directory. $INSTALLER_ROOT $WW2ARCHIVES_ROOT (OSTYPE) is set earlier.
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

if [ "$myos" == "osx" ] ; then
    WW2ARCHIVES_RELEASE=$WW2ARCHIVES_ROOT
fi

# make sure the installer release directory exists, set earlier.
if [ ! -d "$INSTALLER_RELEASE" ] ; then
    exit_msg=$exit_msg'|Pre-requisites ERROR: The directory: '$INSTALLER_RELEASE', does not exist. Please create it.'
    found_prereq_errors=true
fi

# make sure the archive root directory exists. $WW2ARCHIVES_ROOT is set by the OSTYPE located earlier.
if [ ! -d $WW2ARCHIVES_RELEASE ] ; then
    exit_msg=$exit_msg'|Pre-requisites ERROR: The directory: '$WW2ARCHIVES_RELEASE', does not exist. Please create it.'
    found_prereq_errors=true
fi

echo "ww2Mods Directory  ============  :  < "$WW2MODS" >"
echo "Playarc Release Directory  ====  :  < "$WW2ARCHIVES_RELEASE" >"
echo "Installer Release Directory  ==  :  < "$INSTALLER_RELEASE" >"

# Build Windows Patch Installers?
if [ "$myos" == "win" ] ; then
    echo
    echo
    while read -p "Build Windows Patch Installers ( y|n ) : " -n 1 -r ; do
        case $(echo "$REPLY" |tr [:upper:] [:lower:]) in
            "y")
                build_patch=true
                break
                ;;

            "n")
                build_patch=false
                break
                ;;

            *)
                echo
                ;;
        esac
    done

    if [  ! -d $INSTALLER_RELEASE/previous  ] ; then
        exit_msg=$exit_msg'|Pre-requisites ERROR: The standard location for previous build "$INSTALLER_RELEASE"/previous, does not exist. Please Create it.'
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
        exit_msg=$exit_msg'|Pre-requisites ERROR: Visual studio cannot be running when starting the build, please close all instances.'
        found_prereq_errors=true
    fi

else
    # check mac specific issues
    playgate_version=$(grep -r -m 1 -h 'PLAYGATE_VERSION' WW2/src)
    pn_elements=(${playgate_version// / })

    if [ ! "${pn_elements[2]}" == "0x01010109" ] ; then
        exit_msg=$exit_msg'|Pre-requisites ERROR: Playgate version is not correct ('${pn_elements[2]}') should be 0x01010109, please correct.'
        found_prereq_errors=true
    fi
fi

if [ "$found_prereq_errors" = true ] ; then

    exit 1
fi

#
# Pre-requisites checking Completed.
#

noprompt=false

echo
echo
while read -p "Build using all the default responses? ( y|n ) : " -n 1 -r ; do
        case $(echo "$REPLY" |tr [:upper:] [:lower:]) in
            "y")
                noprompt=true
                break
                ;;

            "n")
                noprompt=false
                break
                ;;

            *)
                echo
                ;;
        esac
done

gitBranch=`git rev-parse --abbrev-ref HEAD`
arrayBranchElements=(${gitBranch//\// })
releasetype=${arrayBranchElements[0]}
releaseversion=${arrayBranchElements[1]}

isrelease=false
rebuildMDLfiles=false

if [ "$noprompt" = false ]; then
    echo
    echo
    while read -p "Is this a formal release? ( y|n ) : " -n 1 -r ; do
        case $(echo "$REPLY" |tr [:upper:] [:lower:]) in
            "y")
                break
                ;;

            "n")
                break
                ;;

            *)
                echo
                ;;
        esac
    done

else
    REPLY='y'
fi

if [[ $REPLY =~ ^[Yy]$ ]] ; then
    if [ "$releasetype" != "release" ] && [ "$releasetype" != "hotfix" ]; then
        exit_msg="Formal releases must be on a release or hotfix branch, create a release or hotfix branch and restart the build."

        exit 1
    fi

    echo "Release version is "$releaseversion

    isrelease=true
    # rebuildMDLfiles=true

else
    if [ "$releasetype" == "feature" ] ; then
        if [ "$noprompt" = false ]; then
            echo
            echo "WARNING! You are building from a Feature branch which is normally unwise . . ."
            echo
            while read -p "Are you sure you want to continue? ( y|n ) : " -n 1 -r ; do
                case $(echo "$REPLY" |tr [:upper:] [:lower:]) in
                    "y")
                        break
                        ;;

                    "n")
                        break
                        ;;

                    *)
                        echo
                        ;;
                esac
            done

        else
            REPLY='n'    
        fi

        if [[ ! $REPLY =~ [Yy]$ ]] ; then
                exit_msg="Tend to your repository state and restart build."

                exit 1
        fi
    fi
fi

echo
echo
echo "NOTE:  Re-generating ALL the .mdl files."
rebuildMDLfiles=true

echo
echo
build_step "test" "Testing" "repositories"
# echo "Testing repositories."

if [ "$myos" == "osx" ] ; then
    osascript -e 'tell app "Xcode" to quit'
fi

cleanedfiles=`git clean -d -x -n --exclude="Installer/release.sh" && git -C opt/ww2mods clean -d -x -n`

if [ -n "$cleanedfiles" ] ; then
    clean=false
    echo "Your repositories are not clean . . ."

    if [ "$isrelease" = true ] ; then
        if [ "$noprompt" = false ]; then
            read -p "You must clean them before proceeding, press C to continue :" -n 1 -r
            echo    # (optional) move to a new line
        else
            REPLY='c'
        fi

        if [[ ! $REPLY =~ ^[Cc]$ ]] ; then
            exit_msg="When you are ready to proceed, restart the build process."

            exit 1
        fi

        clean_repo=true
    else
        if [ "$noprompt" = false ]; then
            echo
            while read -p "Would you like to clean the repositories first? ( y|n ) : " -n 1 -r ; do
                case $(echo "$REPLY" |tr [:upper:] [:lower:]) in
                    "y")
                        clean_repo=true
                        break
                        ;;

                    "n")
                        clean_repo=false
                        break
                        ;;

                    *)
                        echo
                        ;;
                esac
            done
        else
            clean_repo=false
        fi
    fi
fi

echo
echo
while read -p "Deploy to Steam when finished? ( y|n ) : " -n 1 -r ; do
    case $(echo "$REPLY" |tr [:upper:] [:lower:]) in
        "y")
            build_steam=true
            break
            ;;

        "n")
            build_steam=false
            break
            ;;

        *)
            echo
            ;;
    esac
done

echo
echo
echo "Options Selected:"
echo "  Installer Build type          : "$build_release
echo "  Detected Operating System     : "$([[ $myos == "win" ]] && echo "Win" || echo "Mac")

if [ "$myos" == "win" ] ; then
    echo "  Build Windows Patch Files     : "$([[ $build_patch == true ]] && echo "Yes" || echo "No")
fi

echo "  Use Default Responses         : "$([[ $noprompt == true ]] && echo "Yes" || echo "No")
echo "  Formal Release                : "$([[ $isrelease == true ]] && echo "Yes" || echo "No")
echo "  Rebuild ALL .mdl Files        : "$([[ $rebuildMDLfiles == true ]] && echo "Yes" || echo "No")
echo "  Clean the Repositories        : "$([[ $clean_repo == true ]] && echo "Yes" || echo "No")
echo "  Deploy to Steam when finished : "$([[ $build_steam == true ]] && echo "Yes" || echo "No")
echo

echo "To continue with the Options Shown above,"
echo "please (V)erify to proceed OR (A)bort to exit."
while read -p "(V)erify  OR  (A)bort ( v|a ) : " -n 1 -r ; do
    case $(echo "$REPLY" |tr [:upper:] [:lower:]) in
        "v")
            break
            ;;

        "a")
            exit_msg=$exit_msg'|When you are ready to proceed, restart the build process.'

            exit 1
            break
            ;;

        *)
            echo
            ;;
    esac
done


################################################################

#    Run

################################################################
startTime=$(date +%s)

# ==========================================================================
# skip_step=true
# ==========================================================================


if [ "$skip_step" = false ]; then
    if [ "$clean_repo" = true ] ; then
        echo
        echo
        build_step "clean" "Cleaning" "repositories"
        
        #Wickedness below because mac's built in bash sucks
        nl=$'\n'
        
        echo $cleanedfiles | sed "s/ Would/\\${nl}Would/g"
        
        if [ "$noprompt" = false ]; then
            echo "The above files will be affected, please (V)erify to proceed OR (A)bort to exit."
            while read -p "(V)erify  OR  (A)bort ( v|a ): " -n 1 -r ; do
                case $(echo "$REPLY" |tr [:upper:] [:lower:]) in
                    "v")
                        break
                        ;;

                    "a")
                        exit_msg="When you are ready to proceed, restart the build process."

                        exit 1
                        break
                        ;;

                    *)
                        echo
                        ;;
                esac
            done
        fi

        echo
        echo "Cleaning wwiiolng"
        git clean -d -x -f --exclude="Installer/release.sh"

        echo "Resetting wwiiolng"
        git reset --hard HEAD

        echo "Cleaning ww2mods"
        git -C opt/ww2mods clean -d -x -f

        echo "Resetting ww2mods"
        git -C opt/ww2mods reset --hard HEAD

        echo
        echo "NOTE: The repositories HAVE BEEN Cleaned."

    else
        echo
        echo
        echo "NOTE: The repositories were NOT Cleaned."

    fi
fi

if [[ $build_steam == true ]] ; then
    echo
    echo "Preparing Steam build environment."
    echo "NOTE: If this is your first time on this computer you may be asked to enter a confirmation code emailed to you!!"
    echo
    echo -n "Enter Steam build user name: "
    read -r steam_user
    echo    # (optional) move to a new line

    if [ ! -z "$steam_user" ] ; then
        echo -n "Enter Steam build password: "
        read -s steam_pwd
        echo    # (optional) move to a new line
        echo

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

    else
        echo
        echo "Warning: No user specified, will NOT Deploy to Steam."
        build_steam=false
    fi
fi

echo
echo "TODO: libxml prebuild"


################################################################

#    Build Debug Client (Debug) for .mdl creation.

################################################################

mac_product_base_name="World\ War\ II\ Online"
mac_product_name=$mac_product_base_name
target_file='WW2/WW2OL.xcodeproj/project.pbxproj'
buildBackupTag="_BUILD_BAK"

if [ "$myos" == "osx" ] ; then
    touch $WW2ARCHIVES_RELEASE/playarc.ww2

    # Mac.app Project. Backup the Project Files.
    echo
    echo "Backup the Mac Project Files."
    echo

    cp -rvp "WW2/WW2OL.xcodeproj" "WW2/WW2OL$buildBackupTag.xcodeproj"
    cp -rvp "WW2/proj/mac/Info.plist" "WW2/proj/mac/Info$buildBackupTag.plist"

    echo
    echo "Mac Project Files Backup Completed."
    echo
fi

if [ "$isrelease" = false ] && [ "$myos" == "osx" ] ; then

    echo
    echo "Updating CFBundleName and PRODUCT_NAME"

    # Set Build Client Title on the Original Files.
    mac_product_name=$mac_product_base_name

    case $build_release in
        $Client_alpha)
            mac_product_name="$mac_product_base_name\ Alpha"
            ;;

        $Client_beta)
            mac_product_name="$mac_product_base_name\ Beta"
            ;;
    esac

    strip_name=`echo "$mac_product_name" | sed 's/\\\//g'`
    plutil -replace CFBundleName -string "$strip_name"  WW2/proj/mac/Info.plist

    grepresult=$(grep "$mac_product_name" $target_file)

    if [ ! -n "$grepresult" ] ; then
        sedargs='s/'$mac_product_base_name'/'$mac_product_name'/g'
        sed -i -e "$sedargs" "$target_file"
    fi
fi

mac_app_name=$mac_product_name".app"

if [ "$myos" == "osx" ] ; then
    echo
    echo "Mac Build Product Name: "$mac_product_name
    echo "Mac app Name: "$mac_app_name
fi

bld_config="Debug"

if [ "$skip_step" = false ] ; then
    echo
    echo
    build_step "build" "Building" "debug client for .mdl file creation"

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
            exit_msg="Error: Build failed, see "$BUILDFILE" for errors."
            
            exit 1
        fi

    else
        # Building Debug client - Mac (.mdl files)
#        if [ "$isrelease" = true ] ; then
#            echo "Cleaning build"
#            xcodebuild -project WW2/WW2OL.xcodeproj -scheme $bld_config' Client' clean PROVISIONING_PROFILE= CODE_SIGN_IDENTITY= WARNING_CFLAGS=-w
#        fi
#        xcodebuild -project WW2/WW2OL.xcodeproj -scheme $bld_config' Client' build PROVISIONING_PROFILE= CODE_SIGN_IDENTITY= WARNING_CFLAGS=-w

        xcodebuild -project WW2/WW2OL.xcodeproj -scheme $bld_config' Client' clean build PROVISIONING_PROFILE= CODE_SIGN_IDENTITY= WARNING_CFLAGS=-w
    fi
fi


################################################################

#  ReBuild the .mdl files.

################################################################

# Removing Old .mdl files from the opt/ww2mods directory
if [ "$skip_step" = false ] ; then
    if [ "$isrelease" = true ] || [ "$rebuildMDLfiles" = true ] ; then
        echo
        build_step "remove" "Removing" "old .mdl files"
        find "$WW2MODS" -name "*.mdl" -delete
    fi

    if [ "$myos" == "win" ] ; then
        # Building New .mdl files - Win
        cp "$INSTALLER_ROOT/_buildMDLfiles.bat" "$WW2MODS/."
        chmod 550 "$WW2MODS/_buildMDLfiles.bat"

        echo
        if [ "$skip_step" = false ]; then
            build_step "copy" "Copying" "Debug binaries into WW2MODS"
            for product in $PRODUCTS ; do
                cp "$DEBUGDIR/$product" "$WW2MODS/$product"
                chmod 550 "$WW2MODS/$product"
            done
        fi

        # Copy 3rd party libraries into WW2MODS

        echo
        if [ "$skip_step" = false ] ; then
            build_step "copy" "Copying" "3rd party libs into WW2MODS (Stram Files)"
            # rsync -au "$WWII_ROOT/Installer/ww2assets/steam/" "$WW2MODS/"
            rsync -au "$INSTALLER_ROOT/ww2assets/steam/" "$WW2MODS/"
        fi

        echo
        if [ "$skip_step" = false ] ; then
            build_step "build" "Building" "mdl files - Win"
            echo

            pushd "$WW2MODS"
            if [ -f "_buildMDLfiles.bat" ] ; then
                ./_buildMDLfiles.bat /args "$MDL_COMMANDLINE_ARGS"

                ExitStatus=$?
                if [ "$ExitStatus" != "0" ] ; then
                    exit_msg="Error: _buildMDLfiles.bat returned: "$ExitStatus" Exit Status, build aborted."
                    exit 1
                fi

            else
                exit_msg="Error: Failed to locate: "$WW2MODS"/_buildMDLfiles.bat, build aborted."

                exit 1
            fi
            popd
            echo
        fi

    else
        # Building New .mdl files - Mac

        echo
        if [ "$skip_step" = false ] ; then
            build_step "build" "Building" "mdl files - Mac"
            echo
            ww2path="WW2/proj/mac/build/$bld_config/$mac_app_name"
            ww2path=$(echo $ww2path | sed 's/\\//g')

            open -W "$ww2path" --args $MDL_COMMANDLINE_ARGS
        fi
    fi

    mdlfiles=`find opt/ww2mods/DATA -name "*.mdl" -print`

    if [ ! -n "$mdlfiles" ] ; then
        exit_msg="Error: Failed to build .mdl files, build aborted."

        exit 1

    else
        echo
        echo "NOTE: .mdl files build completed."
    fi
fi

################################################################

#    Build the game archive files.

################################################################

echo
if [ "$skip_step" = false ] ; then
    build_step "build" "Building" "archive (.ww2) files"
    
    if [ "$myos" == "win" ] ; then
        echo
        pushd "$INSTALLER_ROOT"

        if [ -f "_PlayArchBuilder.bat" ] ; then
            ./_PlayArchBuilder.bat /$build_release

            ExitStatus=$?
            if [ "$ExitStatus" != "0" ] ; then
                exit_msg="Error: _PlayArchBuilder.bat returned: "$ExitStatus" Exit Status, build aborted."
                exit 1
            fi

        else
            exit_msg="Error: Failed to locate: "$INSTALLER_ROOT"/_PlayArchBuilder.bat, build aborted."

            exit 1
        fi
        popd

    else
        # Build the playbuilder
        xcodebuild -project archivetools/playbuilder.xcodeproj -scheme 'playbuilder Release' clean build

        # Build the Playarc file.
        archivetools/build/Release/playbuilder --verbose $WW2MODS/DATA $WW2ARCHIVES_RELEASE

    fi
fi


################################################################

# Building the Game Client files for the Installer (Win and Mac)

################################################################

# If not tagged as official release then we want beta
if [ "$isrelease" = true ] ; then
    bld_config="Release"
    bld_config_dir=$bld_config

else
    bld_config="BetaRelease"
    bld_config_dir=$bld_config
fi

echo
if [ "$skip_step" = false ] ; then
    build_step "build" "Building" "installer release files"

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
            exit_msg="Error: Build failed, see "$BUILDFILE" for errors."

            exit 1
        fi

    else
#        if [ "$isrelease" = true ] ; then
#            echo "Cleaning build"
#            xcodebuild -project WW2/WW2OL.xcodeproj -scheme $bld_config' Client' clean PROVISIONING_PROFILE= CODE_SIGN_IDENTITY= WARNING_CFLAGS=-w
#        fi
#        xcodebuild -project WW2/WW2OL.xcodeproj -scheme $bld_config' Client' build PROVISIONING_PROFILE= WARNING_CFLAGS=-w GCC_PREPROCESSOR_DEFINITIONS='$GCC_PREPROCESSOR_DEFINITIONS '$extradefines

        xcodebuild -project WW2/WW2OL.xcodeproj -scheme $bld_config' Client' clean build PROVISIONING_PROFILE= WARNING_CFLAGS=-w GCC_PREPROCESSOR_DEFINITIONS='$GCC_PREPROCESSOR_DEFINITIONS '$extradefines
    fi
fi

if [ "$myos" == "osx" ] ; then

    # Restore the Original Mac Project Files.
    echo
    echo "Restore the Original Mac Project Files."
    echo

    # Delete the Altered Files.
    rm -rf "WW2/WW2OL.xcodeproj"
    rm -rf "WW2/proj/mac/Info.plist"

    # Restore the Original Files.
    mv -fnv "WW2/WW2OL"$buildBackupTag".xcodeproj" "WW2/WW2OL.xcodeproj"
    mv -fnv "WW2/proj/mac/Info"$buildBackupTag".plist" "WW2/proj/mac/Info.plist"

    echo
    echo "Mac Project Files Restore Completed."
    echo
fi


# ==========================================================================
# skip_step=false
# ==========================================================================


if [ "$myos" == "win" ] ; then
    echo
    echo
    if [ "$skip_step" = false ] ; then
        build_step "copy" "Copying" "preparing the installer source directories - Win"
        echo
        pushd "$INSTALLER_ROOT"

        if [ -f "_PrepareNewRelease.bat" ] ; then
            ./_PrepareNewRelease.bat /$build_release

            ExitStatus=$?
            if [ "$ExitStatus" != "0" ] ; then
                exit_msg="Error: _PrepareNewRelease.bat returned: "$ExitStatus" Exit Status, build aborted."

                exit 1
            fi

        else
            exit_msg="Error: Failed to locate: "$INSTALLER_ROOT"/_PrepareNewRelease.bat , build aborted."

            exit 1
        fi
        popd

        # chmod -R 550 $INSTALLER_RELEASE/.

        # make sure the previous release for the patch to apply exists
        if [ "$build_patch" = true ] ; then
            if [ ! -f "$INSTALLER_RELEASE/WW2.exe" ] ; then
                echo
                echo "WARNING: The previous release directory ('$INSTALLER_RELEASE'),"
                echo "WARNING: does not contain a valid release."
                echo "WARNING:"
                echo "WARNING: The Windows Patch Installs WILL NOT be built."
                build_patch=false
            fi
        fi

        if [ "$build_patch" = true ] ; then
            echo
            echo

            if [ "$skip_step" = false ] ; then
                build_step "create" "Creating" "diff files for patch"

                pushd "$INSTALLER_ROOT"

                if [ -f "_MakeDiffs.bat" ] ; then
                    ./_MakeDiffs.bat /$build_release

                    ExitStatus=$?
                    if [ "$ExitStatus" != "0" ] ; then
                        exit_msg="Error: _MakeDiffs.bat returned: "$ExitStatus" Exit Status, build aborted."

                        exit 1
                    fi
                else
                    exit_msg="Error: Failed to locate - "$INSTALLER_ROOT"/_MakeDiffs.bat, build aborted."

                    exit 1
                fi
                popd

                echo
                echo
                if [ "$skip_step" = false ] ; then
                    build_step "build" "Building" "complete running patch install scripts: Patch"

                    pushd "$INSTALLER_ROOT"

                    if [ -f "$InstallerScriptName" ] ; then
                        iscc "$InstallerScriptName" /dm_ReleaseType=$build_release /dm_BuildType=patch

                        ExitStatus=$?
                        if [ "$ExitStatus" != "0" ] ; then
                            exit_msg="Error: The Patch Inno Installer Script returned: "$ExitStatus" Exit Status, build aborted."

                            exit 1
                        fi
                    else
                        exit_msg="Error: Failed to locate - "$INSTALLER_ROOT"/"$InstallerScriptName", Patch Install build aborted."

                        exit 1
                    fi
                    popd
                fi
            fi


################################################################

echo "TODO  Rollback Patcher is INCOMPLETE."
echo

################################################################


#            echo
#            echo
#            if [ "$skip_step" = false ] ; then
#                build_step "create" "Creating" "diff files for patcher (rollback)"
#                pushd "$INSTALLER_ROOT"
#
#                if [ -f "_MakeDiffs.bat" ] ; then
#                    ./_MakeDiffs.bat /$build_release
#
#                    ExitStatus=$?
#                    if [ "$ExitStatus" != "0" ] ; then
#                        exit_msg="Error: _MakeDiffs.bat returned: "$ExitStatus" Exit Status, build aborted."
#
#                        exit 1
#                    fi
#                else
#                    exit_msg="Error: Failed to locate - "$INSTALLER_ROOT"/_MakeDiffs.bat, build aborted."
#
#                    exit 1
#                fi
#                popd
#
#                echo
#                echo
#                if [ "$skip_step" = false ] ; then
#                    build_step "build" "Building" "complete running patch install scripts: Rollback"
#                    pushd "$INSTALLER_ROOT"
#
#                    if [ -f "$InstallerScriptName" ] ; then
#                        iscc "$InstallerScriptName" /dm_ReleaseType=$build_release /dm_BuildType=rollback
#
#                        if [ "$ExitStatus" != "0" ] ; then
#                            exit_msg="Error: The Rollback Inno Installer Script returned: "$ExitStatus" Exit Status, build aborted."
#
#                            exit 1
#                        fi
#                    else
#                        exit_msg="Error: Failed to locate - "$INSTALLER_ROOT"/"$InstallerScriptName", Rollback Install build aborted."
#
#                        exit 1
#                    fi
#                    popd
#                fi
#            fi

            # Build Patch End.
        fi

        echo
        echo
        
        if [ "$skip_step" = false ] ; then
            build_step "build" "Building" "complete running full install scripts: Full Install"
            pushd "$INSTALLER_ROOT"

            if [ -f "$InstallerScriptName" ] ; then
                iscc "$InstallerScriptName" /dm_ReleaseType=$build_release /dm_BuildType=full

                ExitStatus=$?
                if [ "$ExitStatus" != "0" ] ; then
                    exit_msg="Error: The Full Inno Installer Script returned: "$ExitStatus" Exit Status, build aborted."

                    exit 1
                fi
            else
                exit_msg="Error: Failed to locate - "$INSTALLER_ROOT"/"$InstallerScriptName", Full Install build aborted."

                exit 1
            fi
            popd
        fi
    fi

else
    echo
    if [ "$skip_step" = false ] ; then
        build_step "create" "Creating" "Mac DMG file"

        # Cleaned Names - Remove the \ from the names.
        # mac_base_name="World\ War\ II\ Online", mac_product_name=$mac_product_base_name"\ Alpha"
        DMG_Image_Dir_Name=`echo "$mac_product_name" | sed 's/\\\//g'`

        # mac_app_name=$mac_product_name".app"
        DMG_WW2_App_Name=`echo "$mac_app_name" | sed 's/\\\//g'`

        # Root Directories.
        DMG_Repo_Root="$INSTALLER_ROOT/ww2assets/osmac/WW2_Release_Items/DropDMG"
        DMG_User_Root="$HOME/Library/Application Support/DropDMG"
        DMG_WW2_App_Source="WW2/proj/mac/build/"$bld_config_dir"/"$DMG_WW2_App_Name

        # DropDMG Config Files.
        DMG_Config_Source=$DMG_Repo_Root"/ConfigFiles"
        DMG_Config_Destination=$DMG_User_Root

        # ww2assets Directory.
        DMG_Assets_Root="$INSTALLER_ROOT/ww2assets"

        echo
        echo "Copy the repository DropDMG config files to the user directory."
        echo
        mkdir -p "$DMG_User_Root"{Configurations,Layouts,Licenses}
        cp -rv "$DMG_Repo_Root/ConfigFiles/" "$DMG_User_Root/"
        echo
        echo

        if [ -d "$INSTALLER_RELEASE/$DMG_Image_Dir_Name" ] ; then
            echo "Deleting an Old DMG Image Directory: $INSTALLER_RELEASE/$DMG_Image_Dir_Name."
            echo
            echo
            rm -rf "$INSTALLER_RELEASE/$DMG_Image_Dir_Name"
        fi

        dmgfiles=`find $INSTALLER_RELEASE -name "*.dmg" -print`

        if [ ! -n "$dmg" ] ; then
            echo "Deleting Old DMG Disk Image (.dmg) Files in the Directory: $INSTALLER_RELEASE"
            echo
            echo

            find "$INSTALLER_RELEASE" -name "*.dmg" -delete
        fi

        echo "Create a New DMG Image Directory."
        echo
        cp -rv "$DMG_Repo_Root/World War II Online DropDMG Template" "$INSTALLER_RELEASE/$DMG_Image_Dir_Name"

#        echo
#        echo
#        echo "Create a Symbolic Link to the Library/Application Support directory within the New DMG image directory."
#        echo
#        ln -s "/Library/Application Support" "$INSTALLER_RELEASE/$DMG_Image_Dir_Name"
#        # sudo ln -s "/Library/Application Support" "$INSTALLER_RELEASE/$DMG_Image_Dir_Name"

        echo
        echo
        echo "Update the readme.rtf file -> DMG Image Directory."
        echo
        cp -v "$DMG_Assets_Root/readme.rtf" "$INSTALLER_RELEASE/$DMG_Image_Dir_Name"

        echo
        echo
        echo "Update the license.rtf file -> Repo ConfigFiles / Licenses / WW2_Lic."
        echo
        cp -v "$DMG_Assets_Root/docs/license.rtf" "$DMG_Repo_Root/ConfigFiles/Licenses/WW2_Lic/en.rtf"

        echo
        echo
        echo "Copy the WWIIOL app to the DMG Image Directory."
        echo
        cp -vr "$DMG_WW2_App_Source" "$INSTALLER_RELEASE/$DMG_Image_Dir_Name/"

        if [ "$skip_step" = false ]; then
            echo
            echo
            echo "Create the DMG package for the Mac."
            echo
            # plutil -replace 'content.options.destinationFolder' -string "$( pwd )/Installer/release/macrelease" "$HOME/Library/Application Support/DropDMG/Configurations/WW2_Config.plist"

            /Applications/DropDMG.app/Contents/Frameworks/DropDMGFramework.framework/Versions/A/dropdmg --config-name WW2_Config "$INSTALLER_RELEASE/$DMG_Image_Dir_Name"
        fi
    fi
fi

echo
echo
if [ "$build_steam" = true ] ; then
    if [ "$skip_step" = false ] ; then
        build_step "deploy" "Deploying" "to Steam"
        
        SteamScriptRootPath="../../../../../Installer/steambuild/scripts"
        SteamBuildScript=""
        
        echo "When prompted to allow steam accesss to the network, enable it please."

        if [ "$myos" == "win" ] ; then

            # Preparing the Steam Source Directories.
            pushd "$INSTALLER_ROOT"

            if [ -f "_PrepareNewSteamRelease.bat" ] ; then
                ./_PrepareNewSteamRelease.bat /$build_release

                ExitStatus=$?
                if [ "$ExitStatus" != "0" ] ; then
                    exit_msg="Error: _PrepareNewSteamRelease.bat returned: "$ExitStatus" Exit Status, build aborted."

                    exit 1
                fi
            else
                exit_msg="Error: Failed to locate - "$INSTALLER_ROOT"/_PrepareNewSteamRelease.bat, build aborted."

                exit 1
            fi
            popd

            # Start the Steam Build  --  Win.

            pushd "Libraries/SteamSDK/tools/ContentBuilder/builder"
            case $build_release in
                $Client_release)
                    SteamBuildScript=$SteamScriptRootPath"/app_win_build_RC_251950.vdf"
                    ;;

                $Client_alpha)
                    SteamBuildScript=$SteamScriptRootPath"/app_win_build_Alpha_251950.vdf"
                    ;;

                $Client_beta)
                    SteamBuildScript=$SteamScriptRootPath"/app_win_build_Beta_251950.vdf"
                    ;;
            esac

            ./steamcmd.exe +login $steam_user $steam_pwd +run_app_build $SteamBuildScript +quit
            popd

        else

            echo
            echo
            echo "==================================================================================================="
            echo
            echo "       Preparing the Source Directories for a NEW Steam release. Please wait. . ."
            echo
            echo "==================================================================================================="

            echo
            echo
            echo "Copy the WWIIOL app to the Steam Install Directory."
            echo

            case $build_release in
                $Client_release)
                    rm -r "$INSTALLER_RELEASE"/steam/WWIIOnLine/*
                    cp -vr "$DMG_WW2_App_Source" "$INSTALLER_RELEASE/steam/WWIIOnLine/"
                    ;;

                $Client_alpha)
                    rm -r "$INSTALLER_RELEASE"/steam/WWIIOnLine_Alpha/*
                    cp -vr "$DMG_WW2_App_Source" "$INSTALLER_RELEASE/steam/WWIIOnLine_Alpha/"
                    ;;

                $Client_beta)
                    rm -r "$INSTALLER_RELEASE"/steam/WWIIOnLine_Beta/*
                    cp -vr "$DMG_WW2_App_Source" "$INSTALLER_RELEASE/steam/WWIIOnLine_Beta/"
                    ;;
            esac

            # Start the Steam Build  --  Mac.
            pushd "Libraries/SteamSDK/tools/ContentBuilder/builder_osx"
            echo
            echo

            case $build_release in
                $Client_release)
                    SteamBuildScript=$SteamScriptRootPath"/app_mac_build_RC_251950.vdf"
                    ;;

                $Client_alpha)
                    SteamBuildScript=$SteamScriptRootPath"/app_mac_build_Alpha_251950.vdf"
                    ;;

                $Client_beta)
                    SteamBuildScript=$SteamScriptRootPath"/app_mac_build_Beta_251950.vdf"
                    ;;
            esac

            ./steamcmd.sh +login $steam_user $steam_pwd +run_app_build  $SteamBuildScript +quit
            popd
        fi
    fi
fi

endTime=$(date +%s)
durationTime=`expr ${endTime} - ${startTime}`

if [ "$myos" == "win" ] ; then
    displayTime=`date +%H:%M:%S -ud @${durationTime}`
else
    seconds2time "$durationTime"
fi

echo
echo
echo ">>>>  Release Script completing. ( $displayTime )"
echo
echo


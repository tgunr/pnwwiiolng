#!/bin/bash

WW2ROOT=$(dirname $0)/..
WW2MODS="$WW2ROOT/opt/ww2mods"
WW2ARCHIVES="$WW2ROOT/WW2Archives"
INSTALLER="$WW2ROOT/Installer"
DEBUGDIR="$WW2ROOT/build/client/Debug/Bin"
LIBDIR="$WW2ROOT/Installer/libs"
RELEASEDIR="$WW2ROOT/build/client/Release/Bin"
#RELEASELIBDIR="$WW2ROOT/Build/lib/Win32/Release"
PRODUCTS="WW2.exe Playgate.exe Settings.exe"
DATADIRS="DirectXRedist loc playgate screens"
DATAFILES="eula.txt license.rtf readme.rtf regions.txt tos.txt wwii1.ico"

if [[ ! "$(basename $(pwd))" == "wwiiolng" ]] ; then
    echo "This command must be executed from the root dir: wwiiolng"
    exit 1
fi

echo "Executing release build steps"
read -p "Ensure version number is set and Git repo is clean? (y|n) :" -n 1 -r
echo    # (optional) move to a new line
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
    exit 1
fi

echo "Build DEBUG release"
echo "DEBUG version needs to be built to create mdl files"
echo "You may skip if already constructed"
read -p "DEBUG version of solution, ready? (y|n|s) :" -n 1 -r
echo    # (optional) move to a new line
if [[ $REPLY =~ [Nn]$ ]]
then
    exit 1
fi

if [[ ! $REPLY =~ [Ss]$ ]]
then

	echo "Copy Debug binaries into WW2MODS"
	for product in $PRODUCTS ; do
		cp "$DEBUGDIR/$product" "$WW2MODS/$product"
		touch "$WW2MODS/$product"
	done

	# Copy 3rd party libraries into WW2MODS
	rsync -au "$LIBDIR/" "$WW2MODS/"

	echo "Removing old .mdl files"
	find "$WW2MODS" -name "*.mdl" -delete
	cp "$WW2MODS/Launch Files/ww2mods.bat" "$WW2MODS/."
	echo "Executing DEBUG version to build new mdl files"
	pushd "$WW2MODS"
	./ww2mods.bat
	popd

	echo "Building new .mdl files"
	"$WW2ROOT/Tools/PlayBuilder/run.bat"
	echo
	echo
	echo "WW2Archives should be ready now"

fi

### Copy build products to Installer
echo "Ensure that $INSTALLER/current contains the install files from the previous version's installer,"
echo "About to Copy $WW2ARCHIVES to $INSTALLER/current,"
read -p "are previous and source for current set correctly? (y|n):" -n 1 -r
echo    # (optional) move to a new line
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
    exit 1
fi

find "$WW2ARCHIVES" -path "*/.sync" -prune -o -name "*.ww2" -print -exec cp {} "$INSTALLER/current/" \;

echo "Build RELEASE version"
echo "RELEASE version needs to be built"
read -p "RELEASE version of solution, ready? (y|n) :" -n 1 -r
echo    # (optional) move to a new line
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
    exit 1
fi
   
echo "Copying products to ww2mods"
for product in $PRODUCTS ; do
    if [[ "$RELEASEDIR/$product" -nt "$INSTALLER/current/$product" ]] ; then
	   cp "$RELEASEDIR/$product" "$INSTALLER/current/$product"
	   touch "$INSTALLER/current/$product"
    fi
done
   
#echo "Copy libzmq.dll to Installer libs"
#cp "$RELEASELIBDIR/libzmq.dll" "$INSTALLER/libs/"

echo "Copying Installer libs to current"
rsync -auv "$INSTALLER/libs/" "$INSTALLER/current/"

echo "Copying WW2MODS products to current"
for product in $DATAFILES ; do
	cp "$WW2MODS/$product" "$INSTALLER/current/$product"
	cp "$WW2MODS/$product" "$INSTALLER/assets/$product"	
	touch "$INSTALLER/current/$product"
done

echo "Copying files to $INSTALLER/current/DATA"
for folder in $DATADIRS ; do
    mkdir -p "$INSTALLER/current/DATA/$folder"
    rsync -auv --exclude=".DS_Store" "$WW2MODS/DATA/$folder/" "$INSTALLER/current/DATA/$folder/"
done

echo "Pruning unwanted files"
pushd "$INSTALLER/current"
for file in WW2.{lib,ilk,exp} Playgate.{pbd,map,ilk} *.tkt *.bat ; do
	if [[ -e "$file" ]] ; then
		rm "$file"
	fi
done
popd

echo "Creating diff files for patcher"
pushd "$INSTALLER"
./MakeDiffs.bat
popd

echo "Run complete, ready to execute INNO Installer scripts"

#! /bin/bash
####################################################
# Script to generate the host makefiles from CMake
# cmake must be in your path.

baseDir=`dirname $0`
projParent="${baseDir}"
projDir="build"
relativeSrcDir="../WW2/src"

hash cmake || {
  echo "'Cmake' is required to build the host."
  exit 1
}

cd $baseDir || exit 1
if [ ! \( -e "${projParent}" -a -x "${projParent}" \) ]; then
  echo "${projParent} folder doesn't exist. Check source tree."
  exit 0
fi
cd "${projParent}" || exit 1
if [ ! -e "${projDir}" ] ; then
  mkdir "${projDir}" || exit 1
fi
if [ ! \( -d "${projDir}" -o -L "${projDir}" \) ] ; then
  echo "'${projDir} in '${projParent}' is not a directory or symlink."
  exit 1
fi
cd "${projDir}" || exit 1

cmake -G "Unix Makefiles" "${relativeSrcDir}" $*


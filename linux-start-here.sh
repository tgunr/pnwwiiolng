#!/usr/bin/env bash

# from http://superuser.com/questions/427318/test-if-a-package-is-installed-in-apt
is_installed()
{
  dpkg-query -Wf'${db:Status-abbrev}' "${1}" 2>/dev/null | grep -q '^i'
}

if [ "${USER}" != "vagrant" ]; then
	echo "=========================================="
	echo "USAGE NOTE:"
	echo "=========================================="
	sleep 1
	echo ""
	echo "The preferred way to use this script is to run it from inside of a Vagrant virtualbox machine to ensure a consistent build environment."
	echo ""
	echo "You can obtain vagrant from http://www.vagrantup.com/ and virtualbox by typing"
	echo " $ sudo apt-get install -y virtualbox"
	echo ""
	echo "See the vagrant/linux directory for additional details."
	echo "These scripts were developed to be compatible with 64-bit Linux 14.04 LTS and Linux Mint 17."
	echo ""
	sleep 3
	echo -n "CONTINUE RUNNING IN THE CURRENT ENVIRONMENT? [y/N] "
	read response
	echo
	if [ "${response}" != "y" -a "${response}" != "Y" ] ; then
		_want_packages="vagrant virtualbox virtualbox-dkms"
		for pkg in ${_want_packages}; do
			if ! is_installed $pkg ; then _missing_packages+="${pkg} " ; fi
		done
		if [ -z "${_missing_packages}" ] ; then
			echo "NOTE: Required packages (${_want_packages}) are already installed."
		else
			sudo aptitude install ${_missing_packages}
		fi
		exit 0
	fi
fi

echo "=========================================="
echo "Step 1: Install required packages."
echo "=========================================="
echo ""
# Install essential build packages
sudo aptitude install -y \
	vim ctags cmake git \
	gcc-4.8 gcc g++-4.8 g++ gcc-4.8-doc \
	libmysqlclient18 libmysqlclient-dev \
	lua5.1 libtolua++5.1-dev \
	ncurses-dev \
	libevent-2.0-5 libevent-extra-2.0-5 libevent-pthreads-2.0-5 libevent-dev \
 || {
  echo "Package install failed - aborting."
  exit 1
}

echo ""
echo "=========================================="
echo "Step 2: Install optional packages."
echo "=========================================="
echo "(Answer 'n' if you do not wish to install them)"
echo ""
sudo aptitude install \
	vim-doc vim-scripts \
	gdb valgrind ddd \
	gitk git-gui \
	systemtap-sdt-dev \
	subl

echo ""
echo "=========================================="
echo "Step 3: Generate make files with cmake"
echo "=========================================="
echo ""

# Make sure there's a "build64" directory.
rm -rf "build64"
if [ ! -d "build64" ]; then mkdir build64; fi

# Generate the makefiles from cmake
basepath=`pwd`
cd build64
	cmake -G "Sublime Text 2 - Unix Makefiles" ${basepath}/WW2/src -DCMAKE_BUILD_TYPE=Debug -DPN_DISABLE_AUTH=ON -DPN_BUILD_32BIT=OFF

echo ""
echo "Done."
echo "To build,"
echo " $ cd build64"
echo " $ make -j9"
echo ""


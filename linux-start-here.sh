#!/usr/bin/env bash

# from http://superuser.com/questions/427318/test-if-a-package-is-installed-in-apt
is_installed()
{
  dpkg-query -Wf'${db:Status-abbrev}' "${1}" 2>/dev/null | grep -q '^i'
}

if [ "${USER}" != "vagrant" ]; then
	echo "=========================================="
	echo "INFO: Workstation environment detected."
	echo "=========================================="
	sleep 1
	echo ""
	echo "This script is designed with the Vagrant/VirtualBox virtual development environment (vmde) in mind. See the vagrant/linux directory for additional details."
	echo ""
	echo "It can either install, configure and deploy a vmde for you /now/ or it can execute in the local environment."
	echo ""
	sleep 5
	if [ ! -d "devenv/linux/.vagrant" ] ; then
		response=""
		while [ "x${response}" != "xy" -a "x${response}" != "xn" ]; do
			echo -n "Bring up a virtual development machine? [y/n] "
			read response
		done
		if [ "${response}" = "y" ]; then
			cd devenv/linux || die "Unable to access devenv/linux"
			less README.txt
			./BOOTSTRAP.sh
			exit 0
		fi

		echo ""
		echo "If you need to manually install or configure vagrant, e.g. to work with a different VM provider such as VMware, you can obtain vagrant from http://www.vagrantup.com/."
	else
		echo "(devenv/linux/.vagrant directory found, assuming vagrant is installed/configured)"
		echo "(if not, run devenv/linux/BOOTSTRAP.sh)"
	fi

	echo ""
	echo -n "CONTINUE RUNNING THIS SCRIPT IN YOUR WORKSTATION ENVIRONMENT? [y/N] "
	read response
	if [ "${response}" != "y" ]; then
		exit 0
	fi
	echo ""

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
	subl sublime-text
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


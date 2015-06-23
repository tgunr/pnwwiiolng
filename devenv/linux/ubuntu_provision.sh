#!/usr/bin/env bash
# Prep the local machine for building WWIIOL
# This script is also invoked by the vagrant tear-up script

echo "INFO: Updating APT cache"
aptitude update -y
echo "INFO: Installing required modules"
aptitude install -y \
	vim ctags cmake git \
	gcc-4.9 g++-4.9 gcc-4.9-doc gcc g++ \
	libmysqlclient18 libmysqlclient-dev \
	build-essential flex bison \
	lua5.1 libtolua++5.1-dev \
	ncurses-dev \
	libevent-2.0-5 libevent-extra-2.0-5 libevent-pthreads-2.0-5 libevent-dev

#clang-3.6 clang-3.6-doc clang-3.6-examples python-clang-3.6

echo "INFO: Creating directories"
if [ ! -d /playnet ] ; then mkdir /playnet ; fi
if [ ! -d /playnet/wwiiolng ] ; then mkdir /playnet/wwiiolng ; fi

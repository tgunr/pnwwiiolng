#!/usr/bin/env bash

echo "INFO: Updating APT cache"
aptitude update -y
echo "INFO: Installing required modules"
aptitude install -y \
	vim ctags cmake git \
	gcc-4.8 g++-4.8 gcc-4.8-doc \
	libmysqlclient18 libmysqlclient-dev \
	lua5.1 libtolua++5.1-dev \
	ncurses-dev \
	libevent-2.0-5 libevent-extra-2.0-5 libevent-pthreads-2.0-5 libevent-dev

#clang-3.5 clang-3.5-doc clang-3.5-examples python-clang-3.5

echo "INFO: Creating directories"
if [ ! -d /playnet ] ; then mkdir /playnet ; fi
if [ ! -d /playnet/wwiiolng ] ; then mkdir /playnet/wwiiolng ; fi

chown vagrant.vagrant /playnet /playnet/wwiiolng

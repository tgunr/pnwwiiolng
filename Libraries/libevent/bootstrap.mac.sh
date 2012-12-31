#!/bin/sh
# MacOS bootstrap.

CFLAGS="-arch i386 -arch x86_64" CXXFLAGS="${CFLAGS}" ./configure --prefix=/usr/local --enable-static=yes --enable-shared=no --disable-dependency-tracking

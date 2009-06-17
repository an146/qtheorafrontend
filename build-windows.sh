#!/bin/sh
rm -Rf build Makefile
qmake -spec win32-g++
make
makensis -NOCD src/installer.nsi

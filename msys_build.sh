#!/bin/sh

export PATH=.:/mingw64/bin:/usr/local/bin:/mingw/bin:/bin

make EXE=sunsetter-windows-amd64.exe CC=gcc CXX=g++

ls -lh

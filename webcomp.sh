#!/bin/bash

rm main/webfile.c
cd web
./webcomp filelist.txt ../main/webfile.c
cd -

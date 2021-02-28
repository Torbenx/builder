#!/bin/sh
DIRECTORY=`dirname $0`
gnome-terminal -- python $DIRECTORY/android_log.py $1 $2

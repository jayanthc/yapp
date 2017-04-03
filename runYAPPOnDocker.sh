#!/bin/bash

# replace this with the location of your data files
datadir=~/Astronomy/data

# set up a relay between XQuartz and a TCP port
socat TCP-LISTEN:6000,reuseaddr,fork UNIX-CLIENT:\"$DISPLAY\" & SOCAT_PID=$!

# run a container mounting the data directory to /data, and setting up the
# $DISPLAY environment variable, and run the given YAPP command ($1)
docker run -i -t -v $datadir:/data -e DISPLAY=$(ifconfig en0 | grep "inet " | cut -d " " -f 2):0 yapp

# kill socat once we're done
kill $SOCAT_PID


#!/usr/bin/python

# yapp_addprof.py
# Add [calibrated] profiles from two polarisations.

import sys
import os
import getopt
import math
import numpy
import matplotlib.pyplot as plotter

# function definitions
def PrintUsage(ProgName):
    "Prints usage information."
    print "Usage: " + ProgName + " [options] <data-files>"
    print "    -h  --help                           ",                        \
          "Display this usage information"
    print "    -g  --graphics                       ",                        \
          "Turn on graphics"
    return

# constant
HeaderLines = 4

# default
showPlot = False        # plot flag

# get the command line arguments
ProgName = sys.argv[0]
OptsShort = "hg"
OptsLong = ["help", "graphics"]

# get the arguments using the getopt module
try:
    (Opts, Args) = getopt.getopt(sys.argv[1:], OptsShort, OptsLong)
except getopt.GetoptError, ErrMsg:
    # print usage information and exit
    sys.stderr.write("ERROR: " + str(ErrMsg) + "!\n")
    PrintUsage(ProgName)
    sys.exit(1)

optind = 1
# parse the arguments
for o, a in Opts:
    if o in ("-h", "--help"):
        PrintUsage(ProgName)
        sys.exit()
    elif o in ("-g", "--graphics"):
        showPlot = True
        optind = optind + 1
    else:
        PrintUsage(ProgName)
        sys.exit(1)

# user input validation
if (1 == len(sys.argv)):
    ErrMsg = "No input given"
    sys.stderr.write("ERROR: " + str(ErrMsg) + "!\n")
    # print usage information and exit
    PrintUsage(ProgName)
    sys.exit(1)

NBins = len(open(sys.argv[optind]).readlines()) - HeaderLines
x = numpy.array([float(i) / NBins for i in range(NBins)])

# read profiles
prof0 = numpy.loadtxt(sys.argv[optind], dtype=numpy.float32,                  \
                      comments="#", delimiter="\n")
prof1 = numpy.loadtxt(sys.argv[optind+1], dtype=numpy.float32,                \
                      comments="#", delimiter="\n")

profSum = prof0 + prof1

# write the summed profiles to disk
# build filename
fileSumProf = os.path.splitext(sys.argv[optind])[0] + ".sum.ypr"
fsrc = open(sys.argv[optind], "r")
fdest = open(fileSumProf, "w")
# copy the header lines from the original file
for  j in range(HeaderLines):
    fdest.write(fsrc.readline())
fsrc.close()
# write the calibrated profile
profSum.tofile(fdest, "\n", "%.10f")
fdest.close()

# make plots
if (showPlot):
    plotter.plot(x, prof0, label="pol0")
    plotter.plot(x, prof1, label="pol1")
    plotter.plot(x, profSum, label="pol0 + pol1")
    ticks, labels = plotter.yticks()
    plotter.yticks(ticks, map(lambda val: "%.1f" % val, ticks * 1e3))
    plotter.xlabel("Phase")
    plotter.ylabel("Flux Density (mJy)")
    plotter.legend(loc="best")
    plotter.show()


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

# default
showPlot = False        # plot flag
writeStd = False        # write standard deviation in output file

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

# TODO: make the header reading proper
# count the number of header lines in the first file (assume to be the same for
#    all files)
HeaderLines = 0
hdr = open(sys.argv[optind])
for line in hdr:
    if ("#" == line[0]):
        HeaderLines = HeaderLines + 1
hdr.close()

# get the standard deviations from the two polarizations and compute that of the
#   sum
if (5 == HeaderLines):
    # open pol0
    hdr = open(sys.argv[optind])
    for i, line in enumerate(hdr):
        if (4 == i):
            # read 1-sigma error in S and convert to Jy
            DeltaS0 = float(line[37:-5]) * 1e-6
    hdr.close()
    # open pol1
    hdr = open(sys.argv[optind+1])
    for i, line in enumerate(hdr):
        if (4 == i):
            # read 1-sigma error in S and convert to Jy
            DeltaS1 = float(line[37:-5]) * 1e-6
    hdr.close()
    # compute standard deviation of the sum (assume the two polarizations are
    #   independent random variables, so that the covariance is 0)
    DeltaS = math.sqrt((DeltaS0 * DeltaS0) + (DeltaS1 * DeltaS1))
    writeStd = True

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
# TODO: make proper
# copy the header lines (- standard deviation) from the original file, if
#   adding calibrated data, otherwise copy all header lines
if (writeStd):
    for  j in range(HeaderLines-1):
        fdest.write(fsrc.readline())
    # write the new standard deviation
    fdest.write("# Standard deviation of S           : "
                + str("%.3f" % (DeltaS * 1e6)) + " uJy\n")
else:
    for  j in range(HeaderLines):
        fdest.write(fsrc.readline())
# write the calibrated profile
profSum.tofile(fdest, "\n", "%.10f")
fdest.close()
fsrc.close()

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


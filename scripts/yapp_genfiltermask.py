#!/usr/bin/python

# yapp_genfiltermask.py
# Generate filter mask for yapp_filter. This is to be multiplied with the
#   frequency domain signal.
#
# Based on vegas_genfilter.py
# Created by Jayanth Chennamangalam on 2013.02.18

import sys
import getopt
import math
import numpy
import matplotlib.pyplot as plotter

# function definitions
def PrintUsage(ProgName):
    "Prints usage information."
    print "Usage: " + ProgName + " [options]"
    print "    -h  --help                 Display this usage information"
    print "    -n  --nfft <nfft>          Length of FFT used in yapp_filter"
    print "                               (default is 4096)"
    print "    -t  --tsamp <tsamp>        Sampling time in seconds"
    print "    -l  --lower <freq>         Lower cut-off frequency in Hz"
    print "                               (default is 0.1 Hz)"
    print "    -u  --upper <freq>         Upper cut-off frequency in Hz"
    print "                               (default is 1000 Hz)"
    print "    -g  --graphics             Turn on graphics"
    return

# default values
NFFT = 4096                 # number of points in FFT used in yapp_filter
TSamp = 0.0                 # sampling time
FLow = 0.1                  # Hz, (1 / 10 s)
FHigh = 1000                # Hz, (1 / 1 ms)
Plot = False                # plot flag

# get the command line arguments
ProgName = sys.argv[0]
OptsShort = "hn:t:l:u:g"
OptsLong = ["help", "nfft=", "tsamp=", "lower=", "upper=", "graphics"]

# get the arguments using the getopt module
try:
    (Opts, Args) = getopt.getopt(sys.argv[1:], OptsShort, OptsLong)
except getopt.GetoptError, ErrMsg:
    # print usage information and exit
    sys.stderr.write("ERROR: " + str(ErrMsg) + "!\n")
    PrintUsage(ProgName)
    sys.exit(1)

# parse the arguments
for o, a in Opts:
    if o in ("-h", "--help"):
        PrintUsage(ProgName)
        sys.exit()
    elif o in ("-n", "--nfft"):
        NFFT = int(a)
    elif o in ("-t", "--tsamp"):
        TSamp = float(a)
    elif o in ("-l", "--lower"):
        FLow = float(a)
    elif o in ("-u", "--upper"):
        FHigh = float(a)
    elif o in ("-g", "--graphics"):
        Plot = True
    else:
        PrintUsage(ProgName)
        sys.exit(1)

if (0.0 == TSamp):
    ErrMsg = "Sampling time not specified"
    sys.stderr.write("ERROR: " + str(ErrMsg) + "!\n")
    # print usage information and exit
    PrintUsage(ProgName)
    sys.exit(1)

# compute number of usable points in FFT output
NUsable = (NFFT / 2) + 1

# compute the usable bandwidth
Bandwidth = (1.0 / TSamp) / 2

# compute frequency resolution
Res = Bandwidth / NUsable

# compute the (conservative) indices of FLow and FHigh
IdxLow = math.floor(FLow / Res)
IdxHigh = math.ceil(FHigh / Res)
if (0.0 == IdxLow):
    print "WARNING: Frequency resolution may not be small enough. "           \
          + "Consider using a longer transform."
if (IdxHigh + 1 > NUsable):
    ErrMsg = "Upper cut-off frequency greater than usable bandwidth"
    sys.stderr.write("ERROR: " + str(ErrMsg) + "!\n")
    sys.exit(1)

# generate filter mask
Mask = numpy.zeros(NUsable)
Mask[IdxLow:IdxHigh+1] = 1.0
# make sure the DC bin is zero
Mask[0] = 0.0

# write the coefficients to disk and also plot it
MaskFilename = "yapp_mask_"                                                   \
               + str(NFFT) + "_"                                              \
               + str(TSamp) + "_"                                             \
               + str(FLow) + "_"                                              \
               + str(FHigh) + ".dat" 
Mask.astype('uint8').tofile(MaskFilename)
# plot the mask
if (Plot):
    plotter.plot(Mask)

if (Plot):
    plotter.show()


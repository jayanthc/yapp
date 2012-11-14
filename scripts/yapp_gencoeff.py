#!/usr/bin/python

# yapp_gencoeff.py
# Generate PFB filter coefficients for yapp_ft. The filter coefficients array
# may contain duplicates for optimised reading from the GPU.
#
# Based on vegas_gencoeff.py
# Created by Jayanth Chennamangalam based on code by Sean McHugh, UCSB

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
    print "    -n  --nfft <nfft>          Length of the Fourier transform"
    print "                               (default is 4096)"
    print "    -t  --ntaps <ntaps>        Number of taps in PFB"
    print "                               (default is 8)"
#    print "    -b  --nsubbands <value>    Number of sub-bands in data"
#    print "                               (default is 1)"
    print "    -g  --graphics              Turn on graphics"
    return

# default values
NFFT = 4096                 # number of points in FFT
NTaps = 8                   # number of taps in PFB
NSubBands = 1               # number of sub-bands in data
Plot = False                # plot flag

# get the command line arguments
ProgName = sys.argv[0]
#OptsShort = "hn:t:b:g"
#OptsLong = ["help", "nfft=", "ntaps=", "nsubbands=", "graphics"]
OptsShort = "hn:t:g"
OptsLong = ["help", "nfft=", "ntaps=", "graphics"]

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
    elif o in ("-t", "--ntaps"):
        NTaps = int(a)
    #elif o in ("-b", "--sub-bands"):
    #    NSubBands = int(a)
    elif o in ("-g", "--graphics"):
        Plot = True
    else:
        PrintUsage(ProgName)
        sys.exit(1)

M = NTaps * NFFT

# the filter-coefficient-generation section -->
X = numpy.array([(float(i) / NFFT) - (float(NTaps) / 2) for i in range(M)])
PFBCoeff = numpy.sinc(X) * numpy.hanning(M)
# <-- the filter-coefficient-generation section

# 32-bit (float) coefficients
PFBCoeffFloat32 = numpy.zeros(M * NSubBands, numpy.float32)
k = 0
for i in range(len(PFBCoeff)):
    Coeff = float(PFBCoeff[i])
    for m in range(NSubBands):
        PFBCoeffFloat32[k+m] = Coeff
    k = k + NSubBands

# write the coefficients to disk and also plot it
FileCoeff = open("coeff_"                                                     \
                  + str(NTaps) + "_"                                          \
                  + str(NFFT) + "_"                                           \
                  + str(NSubBands) + ".dat",                                  \
                 "wb")
FileCoeff.write(PFBCoeffFloat32)
# plot the coefficients
if (Plot):
    plotter.plot(PFBCoeffFloat32)

FileCoeff.close()

if (Plot):
    plotter.show()


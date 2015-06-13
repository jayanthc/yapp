#!/usr/bin/python

# yapp_genpfbcoeff.py
# Generate PFB filter coefficients for yapp_ft. The filter coefficients array
# may contain duplicates for optimised reading from the GPU.
#
# Based on vegas_gencoeff.py
# Created by Jayanth Chennamangalam based on code by Sean McHugh, UCSB

"""Usage: yapp_genpfbcoeff.py [options]

Options:
    -h --help                         Display this usage information
    -n --nfft <nfft>                  Length of the Fourier transform
                                      [default: 4096]
    -t --ntaps <ntaps>                Number of taps in PFB
                                      [default: 8]
    -g --graphics                     Turn on graphics

"""
import sys
from docopt import docopt
import math
import numpy
import matplotlib.pyplot as plotter

args = docopt(__doc__, version="1.0.0")

# default values
NFFT = int(args["--nfft"])      # number of points in FFT
NTaps = int(args["--ntaps"])    # number of taps in PFB
NSubBands = 1                   # number of sub-bands in data
Plot = args["--graphics"]       # plot flag

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
        PFBCoeffFloat32[k + m] = Coeff
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

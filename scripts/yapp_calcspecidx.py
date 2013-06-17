#!/usr/bin/python

# yapp_calcspecidx.py
# Calculate spectral index from a series of folded profiles.

import sys
import os
import getopt
import math
import numpy
import matplotlib.pyplot as plotter
import yapp_common as yapp

# function definitions
def PrintUsage(ProgName):
    "Prints usage information."
    print "Usage: " + ProgName + " [options] <data-files>"
    print "    -h  --help                           ",                        \
          "Display this usage information"
    print "    -T  --tsys <tsys>                    ",                        \
          "System temperature in K"
    print "    -G  --gain <gain>                    ",                        \
          "Gain in K/Jy"
    print "    -p  --npol <npol>                    ",                        \
          "Number of polarisations"
    print "    -n  --onstart <phase>                ",                        \
          "Start phase of pulse"
    print "    -f  --onstop <phase>                 ",                        \
          "End phase of pulse"
    print "    -b  --basefit <order>                ",                        \
          "Do polynomial-fit baseline subtraction\n",                         \
          "                                         ",                        \
          "with given order"
    print "    -l  --show-legend                    ",                        \
          "Show legend"
    return

# defaults
doCal = True
Tsys = 0.0
showLegend = False

# get the command line arguments
ProgName = sys.argv[0]
OptsShort = "hT:G:p:n:f:b:l"
OptsLong = ["help", "tsys=", "gain=", "npol=", "onstart=", "onstop=",         \
            "basefit=", "show-legend"]

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
    elif o in ("-T", "--tsys"):
        Tsys = float(a)
        optind = optind + 2
    elif o in ("-G", "--gain"):
        G = float(a)
        optind = optind + 2
    elif o in ("-p", "--npol"):
        NPol = int(a)
        optind = optind + 2
    elif o in ("-n", "--onstart"):
        on = float(a)
        optind = optind + 2
    elif o in ("-f", "--onstop"):
        off = float(a)
        optind = optind + 2
    elif o in ("-b", "--basefit"):
        polyOrder = int(a)
        optind = optind + 2
    elif o in ("-l", "--show-legend"):
        showLegend = True
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

if (0.0 == Tsys):
    doCal = False
    print "WARNING: No Tsys given. No calibration will be performed!"

NBands = len(sys.argv) - optind
SMean = numpy.zeros(NBands)

# read the centre frequencies
Bands = []
for fileProf in sys.argv[optind:]:
    Bands.append([float((open(fileProf).readline())[37:-5]), fileProf])
Bands.sort()

f = numpy.zeros(NBands)
for i in range(NBands):
    f[i] = Bands[i][0]

# read the bandwidth and duration of observation from the first file
hdr = open(sys.argv[optind])
# skip centre frequency
hdr.readline()
# read the original channel bandwidth in MHz
ChanBW = float((hdr.readline())[37:-5])
# read the bandwidth in MHz and convert to Hz
BW = float((hdr.readline())[37:-5]) * 1e6
# read duration in seconds
tObs = float((hdr.readline())[37:-3])
hdr.close()

# TODO: make the header reading proper
# count the number of header lines in the first file (assume to be the same for
#    all files)
HeaderLines = 0
hdr = open(sys.argv[optind])
for line in hdr:
    if ("#" == line[0]):
        HeaderLines = HeaderLines + 1
hdr.close()

NBins = len(open(sys.argv[optind]).readlines()) - HeaderLines
x = numpy.array([float(i) / NBins for i in range(NBins)])

onBin = int(on * NBins)
offBin = int(off * NBins)

# create calibrated folded profile subplot
plotter.subplot(121)

# create array to hold 1-sigma error values
DeltaS = numpy.zeros(NBands)
DeltaSMean = numpy.zeros(NBands)

for i in range(NBands):
    # read raw profile
    prof = numpy.loadtxt(Bands[i][1], dtype=numpy.float32,                    \
                         comments="#", delimiter="\n")

    if (doCal):
        (prof, DeltaS[i]) = yapp.DoCal(prof, onBin, offBin,                   \
                                       Tsys, G, NPol, tObs, NBins, BW,        \
                                       polyOrder)

        # write the calibrated profiles to disk
        # build filename
        fileCalProf = os.path.splitext(Bands[i][1])[0] + ".cal.ypr"
        fsrc = open(Bands[i][1], "r")
        fdest = open(fileCalProf, "w")
        # copy the header lines from the original file
        for  j in range(HeaderLines):
            fdest.write(fsrc.readline())
        # add the 1-sigma error in S
        fdest.write("# Standard deviation of S           : "
                    + str("%.3f" % (DeltaS[i] * 1e6)) + " uJy\n")
        fsrc.close()
        # write the calibrated profile
        prof.tofile(fdest, "\n", "%.10f")
        fdest.close()
    else:
        # TODO: make this proper
        hdr = open(Bands[i][1])
        for h, line in enumerate(hdr):
            if (4 == h):
                # read 1-sigma error in S and convert to Jy
                DeltaS[i] = float(line[37:-5]) * 1e-6
        hdr.close()

    # calculate peak and mean flux density
    SPeak = numpy.max(prof)
    SMean[i] = numpy.sum(prof[onBin:offBin]) / NBins
    lenPulse = len(prof[onBin:offBin])
    # derived using propagation of errors
    DeltaSMean[i] = (DeltaS[i] * numpy.sqrt(lenPulse + 2)) / NBins
    print str("%.3f" % f[i]) + " MHz: "                                       \
          + "DeltaS = " + str("%.3f" % (DeltaS[i] * 1e6)) + " uJy; "          \
          + "SMean = " + str("%.3f" % (SMean[i] * 1e6))                       \
          + "+/-" + str("%.3f" % (DeltaSMean[i] * 1e6)) + " uJy"

    plotLabel = str(f[i]) + " MHz"
    plotter.plot(x, prof, label=plotLabel)
    ticks, labels = plotter.yticks()
    plotter.yticks(ticks, map(lambda val: "%.1f" % val, ticks * 1e3))
    plotter.xlabel("Phase")
    plotter.ylabel("Flux Density (mJy)")
    i = i + 1

if showLegend:
    plotter.legend(loc="best")

# create flux density versus frequency subplot
plotter.subplot(122)

# make sure the values within log10 are > 0 by converting to microJy
SMean = SMean * 1e6
DeltaSMean = DeltaSMean * 1e6

# do a linear fit to the log10 values to calculate the spectral index
specIdxFit = numpy.polyfit(numpy.log10(f), numpy.log10(SMean), 1)
specIdxLine = specIdxFit[0] * numpy.log10(f) + specIdxFit[1]
# compute the spectral index
specIdx = (specIdxLine[0] - specIdxLine[-1])          \
          / (math.log10(f[0]) - math.log10(f[-1]))
print "Spectral index = ", specIdx

plotter.errorbar(f, SMean, yerr=DeltaSMean, fmt="bo")
#plotter.plot(f, specIdxLine, "r")
# get the y-axis tick labels in non-log10
ticks, labels = plotter.yticks()
labels = [str("%.1f" % i) for i in ticks]
plotter.yticks(ticks, labels)
plotter.ylabel("Mean Flux Density ($\mu$Jy)")
# get the x-axis tick labels in non-log10
ticks, labels = plotter.xticks()
# convert units as appropriate
if min(f) > 1e3:    # if f is in 1000 MHz (1 GHz)
    labels = [str("%.1f" % (i * 1e-3)) for i in ticks]
    plotter.xlabel("Frequency (GHz)")
else:
    labels = [str("%.1f" % i) for i in ticks]
    plotter.xlabel("Frequency (MHz)")
plotter.xticks(ticks, labels)

plotter.show()


#!/usr/bin/python

# yapp_calcspecidx.py
# Calculate spectral index from a series of folded profiles.

import sys
import getopt
import math
import numpy
import matplotlib.pyplot as plotter

# function definitions
def PrintUsage(ProgName):
    "Prints usage information."
    print "Usage: " + ProgName + " [options] <data-files>"
    print "    -h  --help                 Display this usage information"
    print "    -T  --tsys <tsys>          System temperature in K"
    print "    -G  --gain <gain>          Gain in K/Jy"
    print "    -p  --npol <npol>          Number of polarisations"
    print "    -n  --onstart <phase>      Start phase of pulse"
    print "    -f  --onstop <phase>       End phase of pulse"
    print "    -b  --basefit              Do polynomial fit baseline subtraction"
    return

# defaults
polyfit = False

# get the command line arguments
ProgName = sys.argv[0]
OptsShort = "hT:G:p:n:f:b"
OptsLong = ["help", "tsys=", "gain=", "npol=", "onstart=", "onstop=", "basefit"]

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
        polyfit = True
        optind = optind + 1
    else:
        PrintUsage(ProgName)
        sys.exit(1)

NBins = len(open(sys.argv[optind]).readlines()) - 4
x = numpy.array([float(i) / NBins for i in range(NBins)])

onBin = int(on * NBins)
offBin = int(off * NBins)

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

# create calibrated folded profile subplot
plotter.subplot(121)

for i in range(NBands):
    # read raw profile
    prof = numpy.loadtxt(Bands[i][1], dtype=numpy.float32,                    \
                         comments="#", delimiter="\n")

    # extract the off-pulse regions
    baseline = prof.copy()
    baseline[onBin:offBin] = numpy.median(prof)

    if polyfit:
        # flatten the baseline using a 4th-degree polynomial fit 
        fit = numpy.polyfit(x, baseline, 4)
        y = fit[0] * x**4 + fit[1] * x**3 + fit[2] * x**2 + fit[3] * x + fit[4]
    else:
        y = numpy.median(baseline)

    # calculate the mean and RMS of the off-pulse region
    baseline = baseline - y
    offMean = numpy.mean(baseline)
    offRMS = numpy.std(baseline)

    # compute the calibration factor using eq. (7.12), Lorimer & Kramer
    C = Tsys / (offRMS * G * math.sqrt(NPol * (tObs / NBins) * BW))

    # calibrate the profile
    prof = (prof - y) * C

    # calculate peak and mean flux density
    SPeak = numpy.max(prof)
    SMean[i] = numpy.sum(prof[onBin:offBin]) / NBins

    plotLabel = str(f[i]) + " MHz"
    plotter.plot(x, prof, label=plotLabel)
    ticks, labels = plotter.yticks()
    plotter.yticks(ticks, map(lambda val: "%.1f" % val, ticks * 1e3))
    plotter.xlabel("Phase")
    plotter.ylabel("Flux Density (mJy)")
    i = i + 1

plotter.legend(loc="best")

# create flux density versus frequency subplot
plotter.subplot(122)

#blah = f.copy()
#bleh = SMean.copy()

f = numpy.log10(f)
# make sure the values within log10 are > 0
SMean = SMean + abs(min(SMean)) + 1
SMean = numpy.log10(SMean)

# do a linear fit
specIdxFit = numpy.polyfit(f, SMean, 1)
specIdxLine = specIdxFit[0] * f + specIdxFit[1]

plotter.scatter(f, SMean)
plotter.plot(f, specIdxLine, "r")
#ticks, labels = plotter.yticks()
#plotter.yticks(ticks, map(lambda val: "%.3f" % val, ticks * 1e3))
plotter.xlabel("log10(Frequency (MHz))")
plotter.ylabel("log10(Mean Flux Density (Jy) + X)")

# make sure the values within log10 are > 0
specIdxLine = specIdxLine + abs(min(specIdxLine)) + 1
specIdx = (math.log10(specIdxLine[0]) - math.log10(specIdxLine[-1]))          \
          / (math.log10(f[0]) - math.log10(f[-1]))

print "Spectral index = ", specIdx

plotter.show()


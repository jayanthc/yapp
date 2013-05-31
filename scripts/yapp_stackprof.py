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
    print "    -t  --tobs <tobs>          Length of observation in s"
    print "    -B  --bw <bw>              Bandwidth in Hz"
    print "    -n  --onstart <phase>      Start phase of pulse"
    print "    -f  --onstop <phase>       End phase of pulse"
    print "    -b  --basefit              Do polynomial fit baseline subtraction"
    return

# defaults
polyfit = False

# get the command line arguments
ProgName = sys.argv[0]
OptsShort = "hT:G:p:t:B:n:f:b"
OptsLong = ["help", "tsys=", "gain=", "npol=", "tobs=", "bw=", "onstart=",
            "onstop=", "basefit"]

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
    elif o in ("-t", "--tobs"):
        tObs = float(a)
        optind = optind + 2
    elif o in ("-B", "--bw"):
        BW = float(a)
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

NBins = len(open(sys.argv[optind]).readlines())
x = numpy.array([float(i) / NBins for i in range(NBins)])

onBin = int(on * NBins)
offBin = int(off * NBins)

NBands = len(sys.argv) - optind

f = numpy.array([11.2, 12.5, 13.8, 15.0, 16.3, 17.6])

profImg = numpy.zeros(NBands * NBins)
profImg.shape = (NBands, NBins)

i = 0
for fileProf in sys.argv[optind:]:
    # read raw profile
    profImg[i] = numpy.fromfile(fileProf, dtype=numpy.float32, sep="\n")

    # extract the off-pulse regions
    baseline = profImg[i].copy()
    baseline[onBin:offBin] = numpy.median(profImg[i])

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
    profImg[i] = (profImg[i] - y) * C

    # increment the index
    i = i + 1

img = plotter.pcolormesh(x, f, profImg, cmap="hot")
plotter.colorbar(img, orientation="vertical").set_label("Flux Density (Jy)")
plotter.xlabel("Phase")
plotter.ylabel("Frequency (GHz)")
plotter.show()

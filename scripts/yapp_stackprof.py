#!/usr/bin/python

# yapp_stackprof.py
# Calibrate folded sub-band profiles and create a 2D plot of
#   frequency versus phase.

import sys
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
    print "    -l  --line                           ",                        \
          "1D stacked plots instead of 2D image"
    return

# defaults
doCal = True
Tsys = 0.0
showLinePlot = False

# get the command line arguments
ProgName = sys.argv[0]
OptsShort = "hT:G:p:n:f:b:l"
OptsLong = ["help", "tsys=", "gain=", "npol=", "onstart=", "onstop=",         \
            "basefit=", "line"]

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
    elif o in ("-l", "--line"):
        showLinePlot = True
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

if (doCal):
    onBin = int(on * NBins)
    offBin = int(off * NBins)

profImg = numpy.zeros(NBands * NBins)
profImg.shape = (NBands, NBins)

for i in range(NBands):
    # read raw profile
    profImg[i] = numpy.loadtxt(Bands[i][1], dtype=numpy.float32,              \
                               comments="#", delimiter="\n")
    if (doCal):
        # get the calibrated profile (and ignore the 1-sigma error)
        (profImg[i], _) = yapp.DoCal(profImg[i], onBin, offBin,               \
                                     Tsys, G, NPol, tObs, NBins, BW,          \
                                     polyOrder)

# matplotlib.pyplot.imshow() does not align the rows correctly with respect to
# the centre frequency of the channels, so compute the lowest frequency per
# channel, to pass to imshow()
for i in range(NBands):
    f[i] = f[i] - ((BW * 1e-6) / 2) + (ChanBW / 2)      # BW is in MHz

if showLinePlot:
    offset = 2 * numpy.std(profImg)
    if NBands < 10:
        numTicks = NBands
    else:
        numTicks = 10   # don't want more than 10 ticks on the y-axis
    step = int(math.ceil(float(NBands) / numTicks))
    yticks = numpy.array([i * offset for i in range(0, NBands, step)])
    ylabels = f[::step]
    for i in range(NBands):
        profImg[i] = profImg[i] + (i * offset)
        plotter.plot(x, profImg[i])
        plotter.yticks(yticks, map(lambda val: "%.1f" % val, f))
else:
    img = plotter.imshow(profImg, origin="lower", aspect="auto", \
                     interpolation="nearest", cmap="jet")
    img.set_extent([min(x), max(x), min(f), max(f) + (BW * 1e-6)])
    cbar = plotter.colorbar(img, orientation="vertical")
    cbar.set_label("Flux Density (Jy)")

plotter.xlabel("Phase")
plotter.ylabel("Frequency (MHz)")
plotter.show()


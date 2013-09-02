#!/usr/bin/python

#
# yapp_common.py
# Common functions
#

import numpy

#
# perform calibration
#
def DoCal(prof, onBin, offBin, Tsys, G, NPol, tObs, NBins, BW, polyOrder, Gamma):
    # extract the off-pulse regions
    baseline = prof.copy()
    baseline[onBin:offBin] = numpy.median(prof)

    x = numpy.array([float(i) / NBins for i in range(NBins)])
    y = numpy.zeros(NBins)
    if (polyOrder != 0.0):  # do polynomial fitting
        fit = numpy.polyfit(x, baseline, polyOrder)
        for i in range(polyOrder + 1):
            y = y + fit[i] * x**(polyOrder - i)
    else:                   # just calculate the median
        y = numpy.median(baseline)

    # calculate the RMS of the off-pulse region
    baseline = baseline - y
    offRMS = numpy.std(baseline)

    # compute the calibration factor using eq. (7.12), Lorimer & Kramer
    C = Tsys / (offRMS * G * numpy.sqrt(NPol * (tObs / NBins) * BW))
    C = C / numpy.sqrt(Gamma)
    DeltaS = C * offRMS

    # calibrate the profile
    # NOTE: instead of subtracting the mean, we subtract either a baseline or
    #       the median
    prof = (prof - y) * C

    return (prof, DeltaS)



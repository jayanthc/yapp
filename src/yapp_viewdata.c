/*
 * @file yapp_viewdata.c
 * Program to plot raw dynamic spectrum data.
 *
 * @verbatim
 * Usage: yapp_viewdata [options] <dynamic-spectrum-data-file>
 *     -h  --help                           Display this usage information
 *     -s  --skip-percent <percentage>      The percentage of data to be skipped
 *     -S  --skip-time <time>               The length of data in seconds, to be
 *                                          skipped
 *     -p  --proc-percent <percentage>      The percentage of data to be
 *                                          processed
 *                                          (default is 100)
 *     -P  --proc-time <time>               The length of data in seconds, to be
 *                                          processed
 *                                          (default is all)
 *     -b  --block-size <samples>           Number of samples read in one block
 *                                          (default is 4096 samples)
 *     -c  --clip-level <level>             Number of sigmas above threshold;
 *                                          will clip anything above this level
 *     -m  --colour-map <name>              MATLAB colour map for plotting
 *                                          (default is 'jet')
 *     -i  --invert                         Invert the background and foreground
 *                                          colours in plots
 *     -n  --non-interactive                Run in non-interactive mode
 *     -v  --version                        Display the version @endverbatim
 *
 * @author Jayanth Chennamangalam
 * @date 2008.11.14
 */

/* TODO: 1. ORT & MST radar data reads nan or inf for the last few samples of
            data
         2. No need for DEF_PROC_TIME
         3. Read and plot dedispersed data */

#include "yapp.h"
#include "yapp_sigproc.h"   /* for SIGPROC filterbank file format support */
#include "colourmap.h"

/* TODO: Handle the headerless/header-separated filterbank format file */

/**
 * The build version string, maintained in the file version.c, which is
 * generated by makever.c.
 */
extern const char *g_pcVersion;

/* PGPLOT device ID */
extern int g_iPGDev;

/* data file */
extern FILE *g_pFSpec;

/* the following are global only to enable cleaning up in case of abnormal
   termination, such as those triggered by SIGINT or SIGTERM */
char *g_pcIsTimeGood = NULL;
float *g_pfBuf = NULL;
float *g_pfPlotBuf = NULL;
float *g_pfXAxis = NULL;
float *g_pfYAxis = NULL;

int main(int argc, char *argv[])
{
    char *pcFileSpec = NULL;
    int iFormat = DEF_FORMAT;
    int iDataSkipPercent = DEF_SKIP_PERCENT;
    int iDataSkipTime = DEF_SKIP_TIME;
    int iDataProcPercent = DEF_PROC_PERCENT;
    int iDataProcTime = DEF_PROC_TIME;
    int iProcSpec = PROC_SPEC_NOTSEL;   /* by default, the processing
                                           specification is not selected */
    YUM_t stYUM = {{0}};
    double dTNextBF = 0.0;
    float *pfTimeSectGain = NULL;
    int iTotSampsPerBlock = 0;  /* iNumChans * iBlockSize */
    int iDataSizePerBlock = 0;  /* fSampSize * iNumChans * iBlockSize */
    float fStatBW = 0.0;
    float fNoiseRMS = 0.0;
    float fThreshold = 0.0;
    float fSNRMin = 0.0;
    float fClipLevel = 0.0;
    double dNumSigmas = 0.0;
    double dTSampInSec = 0.0;   /* holds sampling time in s */
    int iNumTicksY = PG_TICK_STEPS_Y;
    double dTNow = 0.0;
    int iTimeSect = 0;
    int iBadTimeSect = 0;
    char cIsInBadTimeRange = YAPP_FALSE;
    float *pfSpectrum = NULL;
    long lBytesToSkip = 0;
    long lBytesToProc = 0;
    int iTimeSampsSkip = 0;
    int iTimeSampsToProc = 0;
    int iBlockSize = DEF_SIZE_BLOCK;
    int iNumReads = 0;
    int iTotNumReads = 0;
    int iReadBlockCount = 0;
    char cIsLastBlock = YAPP_FALSE;
    int iRet = YAPP_RET_SUCCESS;
    float fDataMin = 0.0;
    float fDataMax = 0.0;
    int iReadItems = 0;
    float fColMin = 0.0;
    float fColMax = 0.0;
    float fXStep = 0.0;
    float fYStep = 0.0;
    float fButX = 0.0;
    float fButY = 0.0;
    char cCurChar = 0;
    int iNumSamps = 0;
    int iDiff = 0;
    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;
    int m = 0;
    int iColourMap = DEF_CMAP;
    int iInvCols = YAPP_FALSE;
    char cIsNonInteractive = YAPP_FALSE;
    const char *pcProgName = NULL;
    int iNextOpt = 0;
    /* valid short options */
    const char* const pcOptsShort = "hs:S:p:P:b:c:m:inv";
    /* valid long options */
    const struct option stOptsLong[] = {
        { "help",                   0, NULL, 'h' },
        { "skip-percent",           1, NULL, 's' },
        { "skip-time",              1, NULL, 'S' },
        { "proc-percent",           1, NULL, 'p' },
        { "proc-time",              1, NULL, 'P' },
        { "block-size",             1, NULL, 'b' },
        { "clip-level",             1, NULL, 'c' },
        { "colour-map",             1, NULL, 'm' },
        { "invert",                 0, NULL, 'i' },
        { "non-interactive",        0, NULL, 'n' },
        { "version",                0, NULL, 'v' },
        { NULL,                     0, NULL, 0   }
    };

    /* get the filename of the program from the argument list */
    pcProgName = argv[0];

    /* parse the input */
    do
    {
        iNextOpt = getopt_long(argc, argv, pcOptsShort, stOptsLong, NULL);
        switch (iNextOpt)
        {
            case 'h':   /* -h or --help */
                /* print usage info and terminate */
                PrintUsage(pcProgName);
                return YAPP_RET_SUCCESS;

            case 's':   /* -s or --skip-percent */
                /* set option */
                if ((PROC_SPEC_NOTSEL == iProcSpec)
                    || (PROC_SPEC_PERCENT == iProcSpec))
                {
                    iDataSkipPercent = atoi(optarg);
                    if (iDataSkipPercent > 100)
                    {
                        (void) fprintf(stderr,
                                       "ERROR: Data skip percentage should be "
                                       "less than 100!\n");
                        PrintUsage(pcProgName);
                        return YAPP_RET_ERROR;
                    }

                    iProcSpec = PROC_SPEC_PERCENT;
                }
                else    /* if the specification mode is time, not percentage */
                {
                    (void) fprintf(stderr,
                                   "ERROR: Data processing specification mode "
                                   "should be either exclusively percentage or "
                                   "exclusively time!\n");
                    PrintUsage(pcProgName);
                    return YAPP_RET_ERROR;
                }
                break;

            case 'S':   /* -S or --skip-time */
                /* set option */
                if ((PROC_SPEC_NOTSEL == iProcSpec)
                    || (PROC_SPEC_TIME == iProcSpec))
                {
                    iDataSkipTime = atoi(optarg);
                    iProcSpec = PROC_SPEC_TIME;
                }
                else    /* if the specification mode is percentage, not time */
                {
                    (void) fprintf(stderr,
                                   "ERROR: Data processing specification mode "
                                   "should be either exclusively percentage or "
                                   "exclusively time!\n");
                    PrintUsage(pcProgName);
                    return YAPP_RET_ERROR;
                }
                break;

            case 'p':   /* -p or --proc-percent */
                /* set option */
                if ((PROC_SPEC_NOTSEL == iProcSpec)
                    || (PROC_SPEC_PERCENT == iProcSpec))
                {
                    iDataProcPercent = atoi(optarg);
                    if (iDataProcPercent > 100)
                    {
                        (void) fprintf(stderr,
                                       "ERROR: Data processing percentage "
                                       "should be less than 100!\n");
                        PrintUsage(pcProgName);
                        return YAPP_RET_ERROR;
                    }

                    iProcSpec = PROC_SPEC_PERCENT;
                }
                else    /* if the specification mode is time, not percentage */
                {
                    (void) fprintf(stderr,
                                   "ERROR: Data processing specification mode "
                                   "should be either exclusively percentage or "
                                   "exclusively time!\n");
                    PrintUsage(pcProgName);
                    return YAPP_RET_ERROR;
                }
                break;

            case 'P':   /* -P or --proc-time */
                /* set option */
                if ((PROC_SPEC_NOTSEL == iProcSpec)
                    || (PROC_SPEC_TIME == iProcSpec))
                {
                    iDataProcTime = atoi(optarg);
                    iProcSpec = PROC_SPEC_TIME;
                }
                else    /* if the specification mode is percentage, not time */
                {
                    (void) fprintf(stderr,
                                   "ERROR: Data processing specification mode "
                                   "should be either exclusively percentage or "
                                   "exclusively time!\n");
                    PrintUsage(pcProgName);
                    return YAPP_RET_ERROR;
                }
                break;

            case 'b':   /* -b or --block-size */
                /* set option */
                iBlockSize = atoi(optarg);
                break;

            case 'c':   /* -c or --clip-level */
                /* set option */
                fClipLevel = (float) atof(optarg);
                break;

            case 'm':   /* -m or --colour-map */
                /* set option */
                iColourMap = GetColourMapFromName(optarg);
                break;

            case 'i':  /* -i or --invert */
                /* set option */
                iInvCols = YAPP_TRUE;
                break;

            case 'n':  /* -n or --non-interactive */
                /* set option */
                cIsNonInteractive = YAPP_TRUE;
                break;

            case 'v':   /* -v or --version */
                /* display the version */
                (void) printf("%s\n", g_pcVersion);
                return YAPP_RET_SUCCESS;

            case '?':   /* user specified an invalid option */
                /* print usage info and terminate with error */
                (void) fprintf(stderr, "ERROR: Invalid option!\n");
                PrintUsage(pcProgName);
                return YAPP_RET_ERROR;

            case -1:    /* done with options */
                break;

            default:    /* unexpected */
                assert(0);
        }
    } while (iNextOpt != -1);

    /* no arguments */
    if (argc <= optind)
    {
        (void) fprintf(stderr, "ERROR: Input file not specified!\n");
        PrintUsage(pcProgName);
        return YAPP_RET_ERROR;
    }

    /* register the signal-handling function */
    iRet = YAPP_RegisterSignalHandlers();
    if (iRet != YAPP_RET_SUCCESS)
    {
        (void) fprintf(stderr,
                       "ERROR: Handler registration failed!\n");
        return YAPP_RET_ERROR;
    }

    /* get the input filename */
    pcFileSpec = argv[optind];

    /* determine the file type */
    iFormat = YAPP_GetFileType(pcFileSpec);
    if (YAPP_RET_ERROR == iFormat)
    {
        (void) fprintf(stderr,
                       "ERROR: File type determination failed!\n");
        return YAPP_RET_ERROR;
    }

    /* read metadata */
    iRet = YAPP_ReadMetadata(pcFileSpec, iFormat, &stYUM);
    if (iRet != YAPP_RET_SUCCESS)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading metadata failed for file %s!\n",
                       pcFileSpec);
        return YAPP_RET_ERROR;
    }

    /* convert sampling interval to seconds */
    dTSampInSec = stYUM.dTSamp / 1e3;

    /* copy next beam-flip time */
    dTNextBF = stYUM.dTNextBF;

    /* check which of the data processing specification modes - percentage or
       time - has been selected by the user, and calculate bytes to skip and
       read */
    if (PROC_SPEC_TIME == iProcSpec)
    {
        if (0 == iDataProcTime)
        {
            iDataProcTime = stYUM.iTimeSamps * dTSampInSec;
        }

        /* check if the input time duration is less than the length of the
           data */
        if (((double) iDataProcTime) > (stYUM.iTimeSamps * dTSampInSec))
        {
            (void) fprintf(stderr,
                           "ERROR: Input time is longer than length of "
                           "data!\n");
            YAPP_CleanUp();
            return YAPP_RET_ERROR;
        }

        lBytesToSkip = (long) ((iDataSkipTime * 1000.0 / stYUM.dTSamp)
                                                        /* number of samples */
                               * stYUM.iNumChans
                               * stYUM.fSampSize);
        lBytesToProc = (long) ((iDataProcTime * 1000.0 / stYUM.dTSamp)
                                                        /* number of samples */
                               * stYUM.iNumChans
                               * stYUM.fSampSize);
    }
    else    /* if it is not selected, or percentage is selected, use percentage
               mode */
    {
        lBytesToSkip = (long) (floorf(stYUM.iTimeSamps
                                     * (((float) iDataSkipPercent) / 100))
                                                        /* number of samples */
                               * stYUM.iNumChans
                               * stYUM.fSampSize);
        lBytesToProc = (long) (ceilf(stYUM.iTimeSamps
                                    * (((float) iDataProcPercent) / 100))
                                                        /* number of samples */
                               * stYUM.iNumChans
                               * stYUM.fSampSize);
    }

    /* if lBytesToSkip is not a multiple of the block size, make it one */
    if (((float) lBytesToSkip / iBlockSize) - (lBytesToSkip / iBlockSize) != 0)
    {
        (void) printf("WARNING: Bytes to skip not a multiple of block size! ");
        lBytesToSkip -= (((float) lBytesToSkip / iBlockSize)
                         - (lBytesToSkip / iBlockSize)) * iBlockSize;
        (void) printf("Newly calculated size of data to be skipped: %ld "
                      "bytes\n",
                      lBytesToSkip);
    }

    if (lBytesToSkip >= stYUM.lDataSizeTotal)
    {
        (void) printf("WARNING: Data to be skipped is greater than or equal to "
                      "the size of the file! Terminating.\n");
        YAPP_CleanUp();
        return YAPP_RET_SUCCESS;
    }

    if ((lBytesToSkip + lBytesToProc) > stYUM.lDataSizeTotal)
    {
        (void) printf("WARNING: Total data to be read (skipped and processed) "
                      "is more than the size of the file! ");
        lBytesToProc = stYUM.lDataSizeTotal - lBytesToSkip;
        (void) printf("Newly calculated size of data to be processed: %ld "
                      "bytes\n",
                      lBytesToProc);
    }

    iTimeSampsSkip = (int) (lBytesToSkip / (stYUM.iNumChans * stYUM.fSampSize));
    (void) printf("Skipping\n"
                  "    %ld of %ld bytes\n"
                  "    %d of %d time samples\n"
                  "    %.10g of %.10g seconds\n",
                  lBytesToSkip,
                  stYUM.lDataSizeTotal,
                  iTimeSampsSkip,
                  stYUM.iTimeSamps,
                  (iTimeSampsSkip * dTSampInSec),
                  (stYUM.iTimeSamps * dTSampInSec));

    iTimeSampsToProc = (int) (lBytesToProc / (stYUM.iNumChans * stYUM.fSampSize));
    iNumReads = (int) ceilf(((float) iTimeSampsToProc) / iBlockSize);
    iTotNumReads = iNumReads;

    /* optimisation - store some commonly used values in variables */
    iTotSampsPerBlock = stYUM.iNumChans * iBlockSize;
    iDataSizePerBlock = (int) (stYUM.fSampSize * iTotSampsPerBlock);

    (void) printf("Processing\n"
                  "    %ld of %ld bytes\n"
                  "    %d of %d time samples\n"
                  "    %.10g of %.10g seconds\n"
                  "in %d reads with block size %d time samples...\n",
                  lBytesToProc,
                  stYUM.lDataSizeTotal,
                  iTimeSampsToProc,
                  stYUM.iTimeSamps,
                  (iTimeSampsToProc * dTSampInSec),
                  (stYUM.iTimeSamps * dTSampInSec),
                  iNumReads,
                  iBlockSize);

    /* calculate the threshold */
    dNumSigmas = YAPP_CalcThresholdInSigmas(iTimeSampsToProc);
    if ((double) YAPP_RET_ERROR == dNumSigmas)
    {
        (void) fprintf(stderr, "ERROR: Threshold calculation failed!\n");
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }
    //if (iFormat != YAPP_FORMAT_DTS_TIM)
    {
        fStatBW = stYUM.iNumGoodChans * stYUM.fChanBW;  /* in MHz */
        (void) printf("Usable bandwidth                  : %g MHz\n", fStatBW);
        fNoiseRMS = 1.0 / sqrt(fStatBW * stYUM.dTSamp * 1e3);
        (void) printf("Expected noise RMS                : %g\n", fNoiseRMS);
        fThreshold = (float) (dNumSigmas * fNoiseRMS);
        (void) printf("Threshold                         : %g\n", fThreshold);
        /* calculate the minimum SNR */
        fSNRMin = fThreshold / fNoiseRMS;
    }

    /* allocate memory for the time sample goodness flag array */
    g_pcIsTimeGood = (char *) YAPP_Malloc(iTimeSampsToProc,
                                          sizeof(char),
                                          YAPP_FALSE);
    if (NULL == g_pcIsTimeGood)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation for time sample goodness "
                       "flag array failed! %s!\n",
                       strerror(errno));
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }
    /* set all elements to 'YAPP_TRUE' */
    (void) memset(g_pcIsTimeGood, YAPP_TRUE, iTimeSampsToProc);

    /* open the dynamic spectrum data file for reading */
    g_pFSpec = fopen(pcFileSpec, "r");
    if (NULL == g_pFSpec)
    {
        (void) fprintf(stderr,
                       "ERROR: Opening file %s failed! %s.\n",
                       pcFileSpec,
                       strerror(errno));
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }

    /* allocate memory for the buffer, based on the number of channels and time
       samples */
    g_pfBuf = (float *) YAPP_Malloc((size_t) stYUM.iNumChans * iBlockSize * stYUM.fSampSize,
                                    sizeof(float),
                                    YAPP_FALSE);
    if (NULL == g_pfBuf)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation for buffer failed! %s!\n",
                       strerror(errno));
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }

    if (1 == iNumReads)
    {
        cIsLastBlock = YAPP_TRUE;
    }

    if (YAPP_FORMAT_FIL == iFormat)
    {
        /* TODO: Need to do this only if the file contains the header */
        /* skip the header */
        (void) fseek(g_pFSpec, (long) stYUM.iHeaderLen, SEEK_SET);
        /* skip data, if any are to be skipped */
        (void) fseek(g_pFSpec, lBytesToSkip, SEEK_CUR);
    }
    else
    {
        /* skip data, if any are to be skipped */
        (void) fseek(g_pFSpec, lBytesToSkip, SEEK_SET);
    }

    /* open the PGPLOT graphics device */
    g_iPGDev = cpgopen(PG_DEV);
    if (g_iPGDev <= 0)
    {
        (void) fprintf(stderr,
                       "ERROR: Opening graphics device %s failed!\n",
                       PG_DEV);
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }

    /* set the background colour to white and the foreground colour to
       black, if user requires so */
    if (YAPP_TRUE == iInvCols)
    {
        cpgscr(0, 1.0, 1.0, 1.0);
        cpgscr(1, 0.0, 0.0, 0.0);
    }

    /* set up the plot's X-axis */
    g_pfXAxis = (float *) YAPP_Malloc(iBlockSize,
                                      sizeof(float),
                                      YAPP_FALSE);
    if (NULL == g_pfXAxis)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation for X-axis failed! %s!\n",
                       strerror(errno));
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }

    if (iFormat != YAPP_FORMAT_DTS_TIM)
    {
        /* set up the image plot's Y-axis (frequency) */
        g_pfYAxis = (float *) YAPP_Malloc(stYUM.iNumChans,
                                         sizeof(float),
                                         YAPP_FALSE);
        if (NULL == g_pfYAxis)
        {
            (void) fprintf(stderr,
                           "ERROR: Memory allocation for Y-axis failed! %s!\n",
                           strerror(errno));
            YAPP_CleanUp();
            return YAPP_RET_ERROR;
        }
        for (i = 0; i < stYUM.iNumChans; ++i)
        {
            g_pfYAxis[i] = stYUM.fFMin + i * stYUM.fChanBW;
        }
    }

    /* calculate the tick step sizes */
    fXStep = (int) ((((iBlockSize - 1) * dTSampInSec) - 0)
                    / PG_TICK_STEPS_X);
    if (iFormat != YAPP_FORMAT_DTS_TIM)
    {
        if (YAPP_TRUE == stYUM.iFlagSplicedData)
        {
            iNumTicksY = stYUM.iNumBands + 1;
        }
        fYStep = (int) ((stYUM.fFMax - stYUM.fFMin) / iNumTicksY);
    }

    /* allocate memory for the cpgimag() plotting buffer */
    g_pfPlotBuf = (float *) YAPP_Malloc((stYUM.iNumChans * iBlockSize),
                                        sizeof(float),
                                        YAPP_FALSE);
    if (NULL == g_pfPlotBuf)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation for plot buffer failed! "
                       "%s!\n",
                       strerror(errno));
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }

    while (iNumReads > 0)
    {
        /* read data */
        (void) printf("\rReading data block %d.", iReadBlockCount);
        (void) fflush(stdout);
        iReadItems = YAPP_ReadData(g_pfBuf,
                                   stYUM.fSampSize,
                                   iTotSampsPerBlock);
        if (YAPP_RET_ERROR == iReadItems)
        {
            (void) fprintf(stderr, "ERROR: Reading data failed!\n");
            YAPP_CleanUp();
            return YAPP_RET_ERROR;
        }
        pfSpectrum = g_pfBuf;
        --iNumReads;
        ++iReadBlockCount;

        if (iReadItems < iTotSampsPerBlock)
        {
            iDiff = (stYUM.iNumChans * iBlockSize) - iReadItems;

            /* reset remaining elements to '\0' */
            (void) memset((g_pfBuf + iReadItems),
                          '\0',
                          (sizeof(float) * iDiff));
        }

        /* calculate the number of time samples in the block - this may not
           be iBlockSize for the last block, and should be iBlockSize for
           all other blocks */
        iNumSamps = iReadItems / stYUM.iNumChans;

        if (YAPP_FORMAT_SPEC == iFormat)
        {
            /* flag bad time sections, and if required, normalise within the
               beam flip time section and perform gain correction */
            for (i = 0; i < iNumSamps; ++i)
            {
                dTNow += dTSampInSec;   /* in s */

                if ((dTNow >= (*stYUM.padBadTimes)[iBadTimeSect][BADTIME_BEG])
                    && (dTNow <= (*stYUM.padBadTimes)[iBadTimeSect][BADTIME_END]))
                {
                    cIsInBadTimeRange = YAPP_TRUE;
                    g_pcIsTimeGood[(iReadBlockCount-1)*iBlockSize+i]
                        = YAPP_FALSE;
                }

                if ((YAPP_TRUE == cIsInBadTimeRange)
                    && (dTNow > (*stYUM.padBadTimes)[iBadTimeSect][BADTIME_END]))
                {
                    cIsInBadTimeRange = YAPP_FALSE;
                    ++iBadTimeSect;
                }

                /* get the beam flip time section corresponding to this
                   sample */
                if (dTNow > dTNextBF)
                {
                    dTNextBF += stYUM.dTBFInt;

                    if (iTimeSect >= stYUM.iBFTimeSects)
                    {
                        (void) fprintf(stderr,
                                       "ERROR: Beam flip time section anomaly "
                                       "detected!\n");
                        YAPP_CleanUp();
                        return YAPP_RET_ERROR;
                    }
                    ++iTimeSect;
                }

                pfTimeSectGain = stYUM.pfBFGain + iTimeSect * stYUM.iNumChans;
                pfSpectrum = g_pfBuf + i * stYUM.iNumChans;
                for (j = 0; j < stYUM.iNumChans; ++j)
                {
                    if (stYUM.pcIsChanGood[j])
                    {
                        if (fClipLevel != 0.0)
                        {
                            if (pfSpectrum[j] > ((dNumSigmas + fClipLevel)
                                                 * fNoiseRMS))
                            {
                                pfSpectrum[j] = (dNumSigmas + fClipLevel)
                                                * fNoiseRMS;
                            }
                            else if (pfSpectrum[j] < -((dNumSigmas + fClipLevel)
                                                       * fNoiseRMS))
                            {
                                pfSpectrum[j] = -((dNumSigmas + fClipLevel)
                                                  * fNoiseRMS);
                            }
                        }

                        pfSpectrum[j] = (pfSpectrum[j]
                                         / stYUM.pfBFTimeSectMean[iTimeSect])
                                        - pfTimeSectGain[j];
                    }
                    else    /* remove bad channels */
                    {
                        pfSpectrum[j] = 0.0;
                    }
                }
            }
        }
        else if (YAPP_FORMAT_FIL == iFormat)
        {
            /* perform clipping, if required */
            for (i = 0; i < iNumSamps; ++i)
            {
                pfSpectrum = g_pfBuf + i * stYUM.iNumChans;
                for (j = 0; j < stYUM.iNumChans; ++j)
                {
                    if (fClipLevel != 0.0)
                    {
                        if (pfSpectrum[j] > ((dNumSigmas + fClipLevel)
                                             * fNoiseRMS))
                        {
                            pfSpectrum[j] = (dNumSigmas + fClipLevel)
                                            * fNoiseRMS;
                        }
                        else if (pfSpectrum[j] < -((dNumSigmas + fClipLevel)
                                                   * fNoiseRMS))
                        {
                            pfSpectrum[j] = -((dNumSigmas + fClipLevel)
                                              * fNoiseRMS);
                        }
                    }
                }
            }
        }

        fDataMin = pfSpectrum[0];
        fDataMax = pfSpectrum[0];
        for (j = 0; j < iBlockSize; ++j)
        {
            pfSpectrum = g_pfBuf + j * stYUM.iNumChans;
            for (k = 0; k < stYUM.iNumChans; ++k)
            {
                if (pfSpectrum[k] < fDataMin)
                {
                    fDataMin = pfSpectrum[k];
                }
                if (pfSpectrum[k] > fDataMax)
                {
                    fDataMax = pfSpectrum[k];
                }
            }
        }

        #ifdef DEBUG
        (void) printf("Minimum value of data             : %g\n",
                      fDataMin);
        (void) printf("Maximum value of data             : %g\n",
                      fDataMax);
        #endif

        if (-fThreshold > fDataMin)
        {
            fColMin = -fThreshold;
        }
        else
        {
            fColMin = fDataMin;
        }
        if (fThreshold < fDataMax)
        {
            fColMax = fThreshold;
        }
        else
        {
            fColMax = fDataMax;
        }

        if (iFormat != YAPP_FORMAT_DTS_TIM)
        {
            /* get the transpose of the two-dimensional array */
            i = 0;
            j = 0;
            k = 0;
            for (l = 0; l < iBlockSize; ++l)
            {
                pfSpectrum = g_pfBuf + l * stYUM.iNumChans;
                for (m = 0; m < stYUM.iNumChans; ++m)
                {
                    g_pfPlotBuf[j] = pfSpectrum[m];
                    j = k + i * iBlockSize;
                    ++i;
                }
                i = 0;
                j = ++k;
            }
        }

        /* erase just before plotting, to reduce flicker */
        cpgeras();
        for (i = 0; i < iBlockSize; ++i)
        {
            g_pfXAxis[i] = (float) (((iReadBlockCount - 1)
                                     * iBlockSize
                                     * dTSampInSec)
                                    + (i * dTSampInSec));
        }
        if (iFormat != YAPP_FORMAT_DTS_TIM)
        {
            pgwrapPlot2D(g_pfPlotBuf, fDataMin, fDataMax,
                         g_pfXAxis, iBlockSize, dTSampInSec,
                         g_pfYAxis, stYUM.iNumChans, stYUM.fChanBW,
                         "Time (s)", "Frequency (MHz)", "Before Dedispersion",
                         iColourMap);
        }
        else
        {
            cpgsvp(PG_VP_ML, PG_VP_MR, PG_VP_MB, PG_VP_MT);
            cpgswin(g_pfXAxis[0],
                    g_pfXAxis[iBlockSize-1],
                    fColMin,
                    fColMax);
            cpglab("Time (s)", "Total Power", "Time Series");
            cpgbox("BCNST", 0.0, 0, "BCNST", 0.0, 0);
            cpgsci(PG_CI_PLOT);
            cpgline(iBlockSize, g_pfXAxis, g_pfBuf);
            cpgsci(PG_CI_DEF);
        }

        if (!(cIsLastBlock))
        {
            if (!(cIsNonInteractive))
            {
                /* draw the 'next' and 'exit' buttons */
                cpgsvp(PG_VP_BUT_ML, PG_VP_BUT_MR, PG_VP_BUT_MB, PG_VP_BUT_MT);
                cpgswin(PG_BUT_L, PG_BUT_R, PG_BUT_B, PG_BUT_T);
                cpgsci(PG_BUT_FILLCOL); /* set the fill colour */
                cpgrect(PG_BUTNEXT_L, PG_BUTNEXT_R, PG_BUTNEXT_B, PG_BUTNEXT_T);
                cpgrect(PG_BUTEXIT_L, PG_BUTEXIT_R, PG_BUTEXIT_B, PG_BUTEXIT_T);
                cpgsci(0);  /* set colour index to white */
                cpgtext(PG_BUTNEXT_TEXT_L, PG_BUTNEXT_TEXT_B, "Next");
                cpgtext(PG_BUTEXIT_TEXT_L, PG_BUTEXIT_TEXT_B, "Exit");

                fButX = (PG_BUTNEXT_R - PG_BUTNEXT_L) / 2;
                fButY = (PG_BUTNEXT_T - PG_BUTNEXT_B) / 2;

                while (YAPP_TRUE)
                {
                    iRet = cpgcurs(&fButX, &fButY, &cCurChar);
                    if (0 == iRet)
                    {
                        (void) fprintf(stderr,
                                       "WARNING: "
                                       "Reading cursor parameters failed!\n");
                        break;
                    }

                    if (((fButX >= PG_BUTNEXT_L) && (fButX <= PG_BUTNEXT_R))
                        && ((fButY >= PG_BUTNEXT_B) && (fButY <= PG_BUTNEXT_T)))
                    {
                        /* animate button click */
                        cpgsci(PG_BUT_FILLCOL);
                        cpgtext(PG_BUTNEXT_TEXT_L, PG_BUTNEXT_TEXT_B, "Next");
                        cpgsci(0);  /* set colour index to white */
                        cpgtext(PG_BUTNEXT_CL_TEXT_L, PG_BUTNEXT_CL_TEXT_B, "Next");
                        (void) usleep(PG_BUT_CL_SLEEP);
                        cpgsci(PG_BUT_FILLCOL); /* set colour index to fill
                                                   colour */
                        cpgtext(PG_BUTNEXT_CL_TEXT_L, PG_BUTNEXT_CL_TEXT_B, "Next");
                        cpgsci(0);  /* set colour index to white */
                        cpgtext(PG_BUTNEXT_TEXT_L, PG_BUTNEXT_TEXT_B, "Next");
                        cpgsci(1);  /* reset colour index to black */
                        (void) usleep(PG_BUT_CL_SLEEP);

                        break;
                    }
                    else if (((fButX >= PG_BUTEXIT_L) && (fButX <= PG_BUTEXIT_R))
                        && ((fButY >= PG_BUTEXIT_B) && (fButY <= PG_BUTEXIT_T)))
                    {
                        /* animate button click */
                        cpgsci(PG_BUT_FILLCOL);
                        cpgtext(PG_BUTEXIT_TEXT_L, PG_BUTEXIT_TEXT_B, "Exit");
                        cpgsci(0);  /* set colour index to white */
                        cpgtext(PG_BUTEXIT_CL_TEXT_L, PG_BUTEXIT_CL_TEXT_B, "Exit");
                        (void) usleep(PG_BUT_CL_SLEEP);
                        cpgsci(PG_BUT_FILLCOL); /* set colour index to fill
                                                   colour */
                        cpgtext(PG_BUTEXIT_CL_TEXT_L, PG_BUTEXIT_CL_TEXT_B, "Exit");
                        cpgsci(0);  /* set colour index to white */
                        cpgtext(PG_BUTEXIT_TEXT_L, PG_BUTEXIT_TEXT_B, "Exit");
                        cpgsci(1);  /* reset colour index to black */
                        (void) usleep(PG_BUT_CL_SLEEP);

                        YAPP_CleanUp();
                        return YAPP_RET_SUCCESS;
                    }
                }
            }
            else
            {
                /* pause before erasing */
                (void) usleep(PG_PLOT_SLEEP);
            }
        }

        if (1 == iNumReads)
        {
            cIsLastBlock = YAPP_TRUE;
        }
    }

    (void) printf("DONE!\n");

    YAPP_CleanUp();

    return YAPP_RET_SUCCESS;
}

/*
 * Prints usage information
 */
void PrintUsage(const char *pcProgName)
{
    (void) printf("Usage: %s [options] <dynamic-spectrum-data-file>\n",
                  pcProgName);
    (void) printf("    -h  --help                           ");
    (void) printf("Display this usage information\n");
    (void) printf("    -s  --skip-percent <percentage>      ");
    (void) printf("The percentage of data to be skipped\n");
    (void) printf("    -S  --skip-time <time>               ");
    (void) printf("The length of data in seconds, to be\n");
    (void) printf("                                         ");
    (void) printf("skipped\n");
    (void) printf("    -p  --proc-percent <percentage>      ");
    (void) printf("The percentage of data to be processed\n");
    (void) printf("                                         ");
    (void) printf("(default is 100)\n");
    (void) printf("    -P  --proc-time <time>               ");
    (void) printf("The length of data in seconds, to be\n");
    (void) printf("                                         ");
    (void) printf("processed\n");
    (void) printf("                                         ");
    (void) printf("(default is all)\n");
    (void) printf("    -b  --block-size <samples>           ");
    (void) printf("Number of samples read in one block\n");
    (void) printf("                                         ");
    (void) printf("(default is 4096 samples)\n");
    (void) printf("    -c  --clip-level <level>             ");
    (void) printf("Number of sigmas above threshold; will\n");
    (void) printf("                                         ");
    (void) printf("clip anything above this level\n");
    (void) printf("    -m  --colour-map <name>              ");
    (void) printf("MATLAB colour map for plotting\n");
    (void) printf("                                         ");
    (void) printf("(default is 'jet')\n");
    (void) printf("    -i  --invert                         ");
    (void) printf("Invert background and foreground\n");
    (void) printf("                                         ");
    (void) printf("colours in plots\n");
    (void) printf("    -n  --non-interactive                ");
    (void) printf("Run in non-interactive mode\n");
    (void) printf("    -v  --version                        ");
    (void) printf("Display the version\n");

    return;
}


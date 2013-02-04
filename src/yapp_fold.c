/*
 * @file yapp_fold.c
 * Program to fold filterbank or dedispersed time series data at a specified
 *  period.
 *
 * @verbatim
 * Usage: yapp_fold [options] <data-file>
 *     -h  --help                           Display this usage information
 *     -s  --skip <time>                    The length of data in seconds, to be
 *                                          skipped
 *                                          (default is 0 s)
 *     -p  --proc <time>                    The length of data in seconds, to be
 *                                          processed
 *                                          (default is all)
 *     -t  --period <period>                Folding period in milliseconds
 *     -i  --invert                         Invert the background and foreground
 *                                          colours in plots
 *     -e  --non-interactive                Run in non-interactive mode
 *     -v  --version                        Display the version @endverbatim
 *
 * @author Jayanth Chennamangalam
 * @date 2012.10.23
 */

#include "yapp.h"
#include "yapp_sigproc.h"   /* for SIGPROC filterbank file format support */
#include "colourmap.h"

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
float *g_pfBuf = NULL;
float *g_pfProfBuf = NULL;
float *g_pfPlotBuf = NULL;
double *g_pdPhase = NULL;
float *g_pfPhase = NULL;
float *g_pfXAxis = NULL;
float *g_pfYAxis = NULL;

int main(int argc, char *argv[])
{
    char *pcFileData = NULL;
    int iFormat = DEF_FORMAT;
    double dDataSkipTime = 0.0;
    double dDataProcTime = 0.0;
    YUM_t stYUM = {{0}};
    int iTotSampsPerBlock = 0;  /* iNumChans * iBlockSize */
    int iDataSizePerBlock = 0;  /* fSampSize * iNumChans * iBlockSize */
    float fStatBW = 0.0;
    float fNoiseRMS = 0.0;
    float fThreshold = 0.0;
    float fSNRMin = 0.0;
    double dNumSigmas = 0.0;
    double dTSampInSec = 0.0;   /* holds sampling time in s */
    double dTNow = 0.0;
    int iTimeSect = 0;
    long lBytesToSkip = 0;
    long lBytesToProc = 0;
    int iTimeSampsSkip = 0;
    int iTimeSampsToProc = 0;
    int iBlockSize = 0;
    int iNumReads = 0;
    int iTotNumReads = 0;
    int iReadBlockCount = 0;
    char cIsLastBlock = YAPP_FALSE;
    int iRet = YAPP_RET_SUCCESS;
    float fDataMin = 0.0;
    float fDataMax = 0.0;
    int iReadItems = 0;
    float fXStep = 0.0;
    float fButX = 0.0;
    float fButY = 0.0;
    char cCurChar = 0;
    int iNumSamps = 0;
    double dPeriod = 0.0;
    double dPhase = 0.0;
    double dPhaseStep = 0.0;
    int iSampsPerPeriod = 0;
    long int lSampCount = 0;
    float fMeanNoise = 0.0;
    float fRMSNoise = 0.0;
    int iDiff = 0;
    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;
    int m = 0;
    float *pfSpectrum = NULL;
    float *pfProfSpec = NULL;
    int iColourMap = DEF_CMAP;
    int iInvCols = YAPP_FALSE;
    char cIsNonInteractive = YAPP_FALSE;
    const char *pcProgName = NULL;
    int iNextOpt = 0;
    /* valid short options */
    const char* const pcOptsShort = "hs:p:t:iev";
    /* valid long options */
    const struct option stOptsLong[] = {
        { "help",                   0, NULL, 'h' },
        { "skip",                   1, NULL, 's' },
        { "proc",                   1, NULL, 'p' },
        { "period",                 1, NULL, 't' },
        { "colour-map",             1, NULL, 'm' },
        { "invert",                 0, NULL, 'i' },
        { "non-interactive",        0, NULL, 'e' },
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

            case 's':   /* -s or --skip */
                /* set option */
                dDataSkipTime = atof(optarg);
                break;

            case 'p':   /* -p or --proc */
                /* set option */
                dDataProcTime = atof(optarg);
                break;

            case 't':   /* -t or --period */
                /* set option */
                dPeriod = atof(optarg);
                break;

            case 'm':   /* -m or --colour-map */
                /* set option */
                iColourMap = GetColourMapFromName(optarg);
                break;

            case 'i':  /* -i or --invert */
                /* set option */
                iInvCols = YAPP_TRUE;
                break;

            case 'e':  /* -e or --non-interactive */
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

    /* user input validation */
    if (0.0 == dPeriod)
    {
        (void) fprintf(stderr, "ERROR: Folding period not specified!\n");
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
    pcFileData = argv[optind];

    /* determine the file type */
    iFormat = YAPP_GetFileType(pcFileData);
    if (YAPP_RET_ERROR == iFormat)
    {
        (void) fprintf(stderr,
                       "ERROR: File type determination failed!\n");
        return YAPP_RET_ERROR;
    }
    if (!((iFormat == YAPP_FORMAT_DTS_TIM)
          || (YAPP_FORMAT_FIL == iFormat)
          || (YAPP_FORMAT_SPEC == iFormat)))
    {
        (void) fprintf(stderr,
                       "ERROR: Invalid file type!\n");
        return YAPP_RET_ERROR;
    }

    /* read metadata */
    iRet = YAPP_ReadMetadata(pcFileData, iFormat, &stYUM);
    if (iRet != YAPP_RET_SUCCESS)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading metadata failed for file %s!\n",
                       pcFileData);
        return YAPP_RET_ERROR;
    }

    /* convert sampling interval to seconds */
    dTSampInSec = stYUM.dTSamp / 1e3;

    if (0.0 == dDataProcTime)
    {
        dDataProcTime = stYUM.iTimeSamps * dTSampInSec;
    }
    /* check if the input time duration is less than the length of the
       data */
    else if (dDataProcTime > (stYUM.iTimeSamps * dTSampInSec))
    {
        (void) fprintf(stderr,
                       "ERROR: Input time is longer than length of data!\n");
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }

    lBytesToSkip = (long) floor((dDataSkipTime / dTSampInSec)
                                                    /* number of samples */
                           * stYUM.iNumChans
                           * stYUM.fSampSize);
    lBytesToProc = (long) floor((dDataProcTime / dTSampInSec)
                                                    /* number of samples */
                           * stYUM.iNumChans
                           * stYUM.fSampSize);

    /* calculate the number of bins in one profile */
    iSampsPerPeriod = (int) round(dPeriod / stYUM.dTSamp);

    /* compute the block size - a large multiple of iSampsPerPeriod */
    iBlockSize = DEF_FOLD_PULSES * iSampsPerPeriod;
    if (iBlockSize > MAX_SIZE_BLOCK)
    {
        int iNumFold = MAX_SIZE_BLOCK / iSampsPerPeriod;
        iBlockSize = iNumFold * iSampsPerPeriod;
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
    if (iTimeSampsToProc <= iBlockSize)
    {
        iNumReads = 1;
    }
    else
    {
        iNumReads = (int) floorf(((float) iTimeSampsToProc) / iBlockSize);
    }
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

    /* open the time series data file for reading */
    g_pFSpec = fopen(pcFileData, "r");
    if (NULL == g_pFSpec)
    {
        (void) fprintf(stderr,
                       "ERROR: Opening file %s failed! %s.\n",
                       pcFileData,
                       strerror(errno));
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }

    /* allocate memory for the buffer, based on the number of channels and time
       samples */
    g_pfBuf = (float *) YAPP_Malloc((size_t) stYUM.iNumChans * iBlockSize,
                                    sizeof(float),
                                    YAPP_FALSE);
    if (NULL == g_pfBuf)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation failed! %s!\n",
                       strerror(errno));
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }

    if (1 == iNumReads)
    {
        cIsLastBlock = YAPP_TRUE;
    }

    if ((YAPP_FORMAT_DTS_TIM == iFormat) || (YAPP_FORMAT_FIL == iFormat))
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

    /* the phase array */
    g_pdPhase = (double *) YAPP_Malloc(iSampsPerPeriod,
                                      sizeof(double),
                                      YAPP_FALSE);
    if (NULL == g_pdPhase)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation for phase array failed! "
                       "%s!\n",
                       strerror(errno));
        cpgclos();
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }
    /* set up the plot's X-axis */
    g_pfPhase = (float *) YAPP_Malloc(iSampsPerPeriod,
                                      sizeof(float),
                                      YAPP_FALSE);
    if (NULL == g_pfPhase)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation for phase plot array failed! "
                       "%s!\n",
                       strerror(errno));
        cpgclos();
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }
    for (i = 0; i < iSampsPerPeriod; ++i)
    {
        g_pdPhase[i] = (double) i * (stYUM.dTSamp / dPeriod);
        g_pfPhase[i] = (float) g_pdPhase[i];
    }
    dPhaseStep = g_pdPhase[1];

    g_pfXAxis = (float *) YAPP_Malloc(iSampsPerPeriod * DEF_FOLD_PULSES,
                                      sizeof(float),
                                      YAPP_FALSE);
    if (NULL == g_pfXAxis)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation for phase plot array failed! "
                       "%s!\n",
                       strerror(errno));
        cpgclos();
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }
    for (i = 0; i < DEF_FOLD_PULSES * iSampsPerPeriod; ++i)
    {
        g_pfXAxis[i] = i;
    }
    if (YAPP_FORMAT_DTS_TIM == iFormat)
    {
        /* allocate memory for the accumulation buffer */
        g_pfProfBuf = (float *) YAPP_Malloc(iSampsPerPeriod,
                                           sizeof(float),
                                           YAPP_TRUE);
        if (NULL == g_pfProfBuf)
        {
            (void) fprintf(stderr,
                           "ERROR: Memory allocation for plot buffer failed! "
                           "%s!\n",
                           strerror(errno));
            YAPP_CleanUp();
            return YAPP_RET_ERROR;
        }
    }
    else
    {
        g_pfProfBuf = (float *) YAPP_Malloc((size_t) stYUM.iNumChans * iSampsPerPeriod,
                                            sizeof(float),
                                            YAPP_TRUE);
        if (NULL == g_pfProfBuf)
        {
            (void) fprintf(stderr,
                           "ERROR: Memory allocation for plot buffer failed! "
                           "%s!\n",
                           strerror(errno));
            YAPP_CleanUp();
            return YAPP_RET_ERROR;
        }
        g_pfPlotBuf = (float *) YAPP_Malloc((size_t) stYUM.iNumChans * iSampsPerPeriod,
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
        if (stYUM.cIsBandFlipped)
        {
            for (i = 0; i < stYUM.iNumChans; ++i)
            {
                g_pfYAxis[i] = stYUM.fFMax - i * stYUM.fChanBW;
            }
        }
        else
        {
            for (i = 0; i < stYUM.iNumChans; ++i)
            {
                g_pfYAxis[i] = stYUM.fFMin + i * stYUM.fChanBW;
            }
        }
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
        --iNumReads;
        ++iReadBlockCount;

        if (iReadItems < iTotSampsPerBlock)
        {
            iDiff = iBlockSize - iReadItems;

            /* reset remaining elements to '\0' */
            (void) memset((g_pfBuf + iReadItems),
                          '\0',
                          (sizeof(float) * iDiff));
        }

        /* calculate the number of time samples in the block - this may not
           be iBlockSize for the last block, and should be iBlockSize for
           all other blocks */
        iNumSamps = iReadItems / stYUM.iNumChans;

        if (YAPP_FORMAT_DTS_TIM == iFormat)
        {
            fMeanNoise = YAPP_CalcMean(g_pfBuf, iNumSamps);
            fRMSNoise = YAPP_CalcRMS(g_pfBuf, iNumSamps, fMeanNoise);

            /* fold data */
            for (i = 0; i < iNumSamps; ++i)
            {
                /* compute the phase */
                dPhase = (double) lSampCount * (stYUM.dTSamp / dPeriod);
                dPhase = dPhase - floor(dPhase);
                /* compute the index into the profile array */
                j = dPhase * iSampsPerPeriod;
                g_pfProfBuf[j] += (((g_pfBuf[i] - fMeanNoise) / fRMSNoise)
                                   / DEF_FOLD_PULSES);
                ++lSampCount;
            }
        }
        else
        {
            fMeanNoise = YAPP_CalcMean(g_pfBuf, iNumSamps * stYUM.iNumChans);
            fRMSNoise = YAPP_CalcRMS(g_pfBuf,
                                     iNumSamps * stYUM.iNumChans,
                                     fMeanNoise);

            /* fold data */
            for (i = 0; i < iNumSamps; ++i)
            {
                /* compute the phase */
                dPhase = (double) lSampCount * (stYUM.dTSamp / dPeriod);
                dPhase = dPhase - floor(dPhase);
                /* compute the index into the profile array */
                j = dPhase * iSampsPerPeriod;
                pfSpectrum = g_pfBuf + i * stYUM.iNumChans;
                pfProfSpec = g_pfProfBuf + j * stYUM.iNumChans;
                for (k = 0; k < stYUM.iNumChans; ++k)
                {
                    pfProfSpec[k] += (((pfSpectrum[k] - fMeanNoise) / fRMSNoise)
                                      / DEF_FOLD_PULSES);
                }
                ++lSampCount;
            }
        }

        #ifdef DEBUG
        (void) printf("Minimum value of data             : %g\n",
                      fDataMin);
        (void) printf("Maximum value of data             : %g\n",
                      fDataMax);
        #endif

        /* erase just before plotting, to reduce flicker */
        cpgeras();
        if (YAPP_FORMAT_DTS_TIM == iFormat)
        {
            fDataMin = g_pfProfBuf[0];
            fDataMax = g_pfProfBuf[0];
            for (i = 0; i < (iSampsPerPeriod * stYUM.iNumChans); ++i)
            {
                if (g_pfProfBuf[i] < fDataMin)
                {
                    fDataMin = g_pfProfBuf[i];
                }
                if (g_pfProfBuf[i] > fDataMax)
                {
                    fDataMax = g_pfProfBuf[i];
                }
            }

            cpgsvp(PG_VP_ML, PG_VP_MR, PG_VP_MB, PG_VP_MT);
            cpgswin(g_pfPhase[0],
                    g_pfPhase[iSampsPerPeriod-1],
                    fDataMin,
                    fDataMax);
            cpglab("Phase", "Power (arbitrary units)", "Folded Profile");
            cpgbox("BCNST", 0.0, 0, "BCNST", 0.0, 0);
            cpgsci(PG_CI_PLOT);
            cpgline(iSampsPerPeriod, g_pfPhase, g_pfProfBuf);
            cpgsci(PG_CI_DEF);
        }
        else
        {
            fDataMin = g_pfProfBuf[0];
            fDataMax = g_pfProfBuf[0];
            for (i = 0; i < iSampsPerPeriod; ++i)
            {
                pfProfSpec = g_pfProfBuf + i * stYUM.iNumChans;
                for (j = 0; j < stYUM.iNumChans; ++j)
                {
                    if (pfProfSpec[j] < fDataMin)
                    {
                        fDataMin = pfProfSpec[j];
                    }
                    if (pfProfSpec[j] > fDataMax)
                    {
                        fDataMax = pfProfSpec[j];
                    }
                }
            }

            /* get the transpose of the two-dimensional array */
            k = 0;
            l = 0;
            m = 0;
            for (i = 0; i < iSampsPerPeriod; ++i)
            {
                pfProfSpec = g_pfProfBuf + i * stYUM.iNumChans;
                for (j = 0; j < stYUM.iNumChans; ++j)
                {
                    g_pfPlotBuf[l] = pfProfSpec[j];
                    l = m + k * iSampsPerPeriod;
                    ++k;
                }
                k = 0;
                l = ++m;
            }

            Plot2D(g_pfPlotBuf, fDataMin, fDataMax,
                   g_pfPhase, iSampsPerPeriod, dPhaseStep,
                   g_pfYAxis, stYUM.iNumChans, stYUM.fChanBW,
                   "Phase", "Frequency (MHz)", "Folded Dynamic Spectrum",
                   iColourMap);
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

                        cpgclos();
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

    cpgclos();
    YAPP_CleanUp();

    return YAPP_RET_SUCCESS;
}

/*
 * Prints usage information
 */
void PrintUsage(const char *pcProgName)
{
    (void) printf("Usage: %s [options] <data-file>\n",
                  pcProgName);
    (void) printf("    -h  --help                           ");
    (void) printf("Display this usage information\n");
    (void) printf("    -s  --skip <time>                    ");
    (void) printf("The length of data in seconds, to be\n");
    (void) printf("                                         ");
    (void) printf("skipped\n");
    (void) printf("                                         ");
    (void) printf("(default is 0 s)\n");
    (void) printf("    -p  --proc <time>                    ");
    (void) printf("The length of data in seconds, to be\n");
    (void) printf("                                         ");
    (void) printf("processed\n");
    (void) printf("                                         ");
    (void) printf("(default is all)\n");
    (void) printf("    -t  --period <period>                ");
    (void) printf("Folding period in milliseconds\n");
    (void) printf("    -i  --invert                         ");
    (void) printf("Invert background and foreground\n");
    (void) printf("                                         ");
    (void) printf("colours in plots\n");
    (void) printf("    -e  --non-interactive                ");
    (void) printf("Run in non-interactive mode\n");
    (void) printf("    -v  --version                        ");
    (void) printf("Display the version\n");

    return;
}


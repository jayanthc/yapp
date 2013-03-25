/*
 * @file yapp_smooth.c
 * Program to smooth (low-pass filter) dedispersed time series data.
 *
 * @verbatim
 * Usage: yapp_smooth [options] <data-file>
 *     -h  --help                           Display this usage information
 *     -s  --skip <time>                    The length of data in seconds, to be
 *                                          skipped
 *                                          (default is 0 s)
 *     -p  --proc <time>                    The length of data in seconds, to be
 *                                          processed
 *                                          (default is all)
 *     -w  --width <width>                  Width of boxcar window in milliseconds
 *                                          (default is 1 ms)
 *     -g  --graphics                       Turn on plotting
 *     -i  --invert                         Invert the background and foreground
 *                                          colours in plots
 *     -e  --non-interactive                Run in non-interactive mode
 *     -v  --version                        Display the version @endverbatim
 *
 * @author Jayanth Chennamangalam
 * @date 2013.01.08
 */

#include "yapp.h"
#include "yapp_sigproc.h"   /* for SIGPROC filterbank file format support */

/**
 * The build version string, maintained in the file version.c, which is
 * generated by makever.c.
 */
extern const char *g_pcVersion;

/* PGPLOT device ID */
extern int g_iPGDev;

/* data file */
extern FILE *g_pFData;

/* the following are global only to enable cleaning up in case of abnormal
   termination, such as those triggered by SIGINT or SIGTERM */
float *g_pfBuf = NULL;
float *g_pfOutBuf = NULL;
float *g_pfXAxis = NULL;

int main(int argc, char *argv[])
{
    FILE *pFOut = NULL;
    char *pcFileData = NULL;
    char *pcFileOut = NULL;
    char acFileOut[LEN_GENSTRING] = {0};
    int iFormat = DEF_FORMAT;
    double dDataSkipTime = 0.0;
    double dDataProcTime = 0.0;
    YUM_t stYUM = {{0}};
    int iTotSampsPerBlock = 0;  /* iBlockSize */
    int iDataSizePerBlock = 0;  /* fSampSize * iBlockSize */
    double dTSampInSec = 0.0;   /* holds sampling time in s */
    long lBytesToSkip = 0;
    long lBytesToProc = 0;
    int iTimeSampsToSkip = 0;
    int iTimeSampsToProc = 0;
    int iBlockSize = 0;
    int iOutBlockSize = 0;
    int iNumReads = 0;
    int iTotNumReads = 0;
    int iReadBlockCount = 0;
    float fWidth = 1.0; /* in ms */
    char cIsLastBlock = YAPP_FALSE;
    int iRet = YAPP_RET_SUCCESS;
    float fDataMin = 0.0;
    float fDataMax = 0.0;
    int iReadItems = 0;
    float fButX = 0.0;
    float fButY = 0.0;
    char cCurChar = 0;
    int iNumSamps = 0;
    int iSampsPerWin = 0;
    int iDiff = 0;
    int i = 0;
    char cIsFirst = YAPP_TRUE;
    float fMeanOrig = 0.0;
    float fRMSOrig = 0.0;
    float fMeanOrigAll = 0.0;
    float fRMSOrigAll = 0.0;
    float fMeanSmoothed = 0.0;
    float fRMSSmoothed = 0.0;
    float fMeanSmoothedAll = 0.0;
    float fRMSSmoothedAll = 0.0;
    char cHasGraphics = YAPP_FALSE;
    int iInvCols = YAPP_FALSE;
    char cIsNonInteractive = YAPP_FALSE;
    const char *pcProgName = NULL;
    int iNextOpt = 0;
    /* valid short options */
    const char* const pcOptsShort = "hs:p:w:giev";
    /* valid long options */
    const struct option stOptsLong[] = {
        { "help",                   0, NULL, 'h' },
        { "skip",                   1, NULL, 's' },
        { "proc",                   1, NULL, 'p' },
        { "width",                  1, NULL, 'w' },
        { "graphics",               0, NULL, 'g' },
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

            case 'w':   /* -w or --width */
                /* set option */
                fWidth = atof(optarg);
                if (fWidth > 1) /* 1 ms */
                {
                    fprintf(stderr,
                            "WARNING: The chosen boxcar width may suppress "
                            "pulsars with periods less than %g ms in the "
                            "smoothed data!\n",
                            fWidth);
                }
                break;

            case 'g':   /* -g or --graphics */
                /* set option */
                cHasGraphics = YAPP_TRUE;
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
    if (iFormat != YAPP_FORMAT_DTS_TIM)
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

    /* calculate bytes to skip and read */
    if (0.0 == dDataProcTime)
    {
        dDataProcTime = (stYUM.iTimeSamps * dTSampInSec) - dDataSkipTime;
    }
    /* check if the input time duration is less than the length of the
       data */
    else if (dDataProcTime > (stYUM.iTimeSamps * dTSampInSec))
    {
        (void) fprintf(stderr,
                       "WARNING: Input time is longer than length of "
                       "data!\n");
    }

    lBytesToSkip = (long) floor((dDataSkipTime / dTSampInSec)
                                                    /* number of samples */
                           * stYUM.fSampSize);
    lBytesToProc = (long) floor((dDataProcTime / dTSampInSec)
                                                    /* number of samples */
                           * stYUM.fSampSize);

    /* calculate the number of samples in one boxcar window */
    iSampsPerWin = (int) round(fWidth / stYUM.dTSamp);
    /* make the number of samples odd */
    if (0 == (iSampsPerWin % 2))
    {
        iSampsPerWin += 1;
        (void) fprintf(stderr,
                       "WARNING: Number of samples per window modified to "
                       "be odd, new boxcar window width is %g ms.\n",
                       stYUM.dTSamp * iSampsPerWin);
    }

    /* compute the block size - a large multiple of iSampsPerWin */
    iBlockSize = DEF_WINDOWS * iSampsPerWin;
    if (iBlockSize > MAX_SIZE_BLOCK)
    {
        iBlockSize = MAX_SIZE_BLOCK;
    }

    if (lBytesToSkip >= stYUM.lDataSizeTotal)
    {
        (void) fprintf(stderr,
                       "ERROR: Data to be skipped is greater than or equal to "
                       "the size of the file!\n");
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
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

    /* change block size according to the number of samples to be processed */
    if (iTimeSampsToProc < iBlockSize)
    {
        iBlockSize = (int) ceil(dDataProcTime / dTSampInSec);
    }

    iTimeSampsToSkip = (int) (lBytesToSkip / (stYUM.fSampSize));
    (void) printf("Skipping\n"
                  "    %ld of %ld bytes\n"
                  "    %d of %d time samples\n"
                  "    %.10g of %.10g seconds\n",
                  lBytesToSkip,
                  stYUM.lDataSizeTotal,
                  iTimeSampsToSkip,
                  stYUM.iTimeSamps,
                  (iTimeSampsToSkip * dTSampInSec),
                  (stYUM.iTimeSamps * dTSampInSec));

    iTimeSampsToProc = (int) (lBytesToProc / (stYUM.fSampSize));
    /* calculate the actual number of samples that will be processed in one
       iteration */
    iOutBlockSize = iBlockSize - (iSampsPerWin - 1);
    /* based on actual processed blocks, rather than read blocks */
    iNumReads = (int) ceilf(((float) iTimeSampsToProc) / iOutBlockSize);
    iTotNumReads = iNumReads;

    /* optimisation - store some commonly used values in variables */
    iTotSampsPerBlock = iBlockSize;
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
                  iOutBlockSize);

    (void) printf("Boxcar window width is %d time samples.\n", iSampsPerWin);

    /* open the time series data file for reading */
    g_pFData = fopen(pcFileData, "r");
    if (NULL == g_pFData)
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
    g_pfBuf = (float *) YAPP_Malloc((size_t) iBlockSize,
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

    /* open the time series data file for writing */
    pcFileOut = YAPP_GetFilenameFromPath(pcFileData, EXT_TIM);
    (void) sprintf(acFileOut,
                   "%s_%s%gms%s",
                   pcFileOut,
                   INFIX_SMOOTHED,
                   fWidth,
                   EXT_TIM);
    pFOut = fopen(acFileOut, "w");
    if (NULL == pFOut)
    {
        (void) fprintf(stderr,
                       "ERROR: Opening file %s failed! %s.\n",
                       acFileOut,
                       strerror(errno));
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }

    /* skip the header by copying it to the output file */
    char acBuf[stYUM.iHeaderLen];
    (void) fread(acBuf, sizeof(char), (long) stYUM.iHeaderLen, g_pFData);
    (void) fwrite(acBuf, sizeof(char), (long) stYUM.iHeaderLen, pFOut);
    /* skip data, if any are to be skipped */
    (void) fseek(g_pFData, lBytesToSkip, SEEK_CUR);

    /* open the PGPLOT graphics device */
    if (cHasGraphics)
    {
        g_iPGDev = cpgopen(PG_DEV);
        if (g_iPGDev <= 0)
        {
            (void) fprintf(stderr,
                           "ERROR: Opening graphics device %s failed!\n",
                           PG_DEV);
            (void) fclose(pFOut);
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

        cpgsubp(1, 2);
        cpgsch(PG_CH_2P);

        /* set up the plot's X-axis */
        g_pfXAxis = (float *) YAPP_Malloc(iBlockSize,
                                          sizeof(float),
                                          YAPP_FALSE);
        if (NULL == g_pfXAxis)
        {
            (void) fprintf(stderr,
                           "ERROR: Memory allocation for X-axis failed! %s!\n",
                           strerror(errno));
            (void) fclose(pFOut);
            YAPP_CleanUp();
            return YAPP_RET_ERROR;
        }
    }

    /* allocate memory for the accumulation buffer */
    g_pfOutBuf = (float *) YAPP_Malloc((size_t) iOutBlockSize,
                                       sizeof(float),
                                       YAPP_TRUE);
    if (NULL == g_pfOutBuf)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation for plot buffer failed! "
                       "%s!\n",
                       strerror(errno));
        (void) fclose(pFOut);
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
            (void) fclose(pFOut);
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
        iNumSamps = iReadItems;

        if (cIsFirst)
        {
            /* copy first (iSampsPerWin / 2) time samples to the output */
            (void) memcpy(g_pfOutBuf, g_pfBuf, (iSampsPerWin / 2) * sizeof(float));
            (void) fwrite(g_pfOutBuf,
                          sizeof(float),
                          (long) (iSampsPerWin / 2),
                          pFOut);
            cIsFirst = YAPP_FALSE;
        }

        /* smooth data */
        (void) memset(g_pfOutBuf, '\0', (sizeof(float) * iOutBlockSize));
        (void) YAPP_Smooth(g_pfBuf, iNumSamps, iSampsPerWin, g_pfOutBuf);
        /* write smoothed data to file */
        (void) fwrite(g_pfOutBuf,
                      sizeof(float),
                      (long) (iNumSamps - (iSampsPerWin - 1)),
                      pFOut);

        /* calculate statistics */
        /* original signal */
        fMeanOrig = YAPP_CalcMean(g_pfBuf, iNumSamps - (iSampsPerWin - 1));
        fMeanOrigAll += fMeanOrig;
        fRMSOrig = YAPP_CalcRMS(g_pfBuf,
                                iNumSamps - (iSampsPerWin - 1),
                                fMeanOrig);
        fRMSOrig *= fRMSOrig;
        fRMSOrig *= (iNumSamps - (iSampsPerWin - 1) - 1);
        fRMSOrigAll += fRMSOrig;

        /* smoothed signal */
        fMeanSmoothed = YAPP_CalcMean(g_pfOutBuf, iNumSamps - (iSampsPerWin - 1));
        fMeanSmoothedAll += fMeanSmoothed;
        fRMSSmoothed = YAPP_CalcRMS(g_pfOutBuf,
                                    iNumSamps - (iSampsPerWin - 1),
                                    fMeanSmoothed);
        fRMSSmoothed *= fRMSSmoothed;
        fRMSSmoothed *= (iNumSamps - (iSampsPerWin - 1) - 1);
        fRMSSmoothedAll += fRMSSmoothed;

        /* set the file position to rewind by (iSampsPerWin - 1) samples */
        (void) fseek(g_pFData, -((iSampsPerWin - 1) * sizeof(float)), SEEK_CUR);

        if (cHasGraphics)
        {
            /* use a separate plotting buffer so that the x-axes for both
               before and after plots are equivalent */
            float* pfPlotBuf = g_pfBuf + (iSampsPerWin / 2);
            fDataMin = pfPlotBuf[0];
            fDataMax = pfPlotBuf[0];
            for (i = 0; i < iOutBlockSize; ++i)
            {
                if (pfPlotBuf[i] < fDataMin)
                {
                    fDataMin = pfPlotBuf[i];
                }
                if (pfPlotBuf[i] > fDataMax)
                {
                    fDataMax = pfPlotBuf[i];
                }
            }

            #ifdef DEBUG
            (void) printf("Minimum value of data             : %g\n",
                          fDataMin);
            (void) printf("Maximum value of data             : %g\n",
                          fDataMax);
            #endif

            cpgpanl(1, 1);
            /* erase just before plotting, to reduce flicker */
            cpgeras();
            for (i = 0; i < iOutBlockSize; ++i)
            {
                g_pfXAxis[i] = (float) (dDataSkipTime
                                        + (((iReadBlockCount - 1)
                                            * iOutBlockSize
                                            * dTSampInSec)
                                           + (i * dTSampInSec)));
            }

            cpgsvp(PG_VP_ML, PG_VP_MR, PG_VP_MB, PG_VP_MT);
            cpgswin(g_pfXAxis[0],
                    g_pfXAxis[iOutBlockSize-1],
                    fDataMin,
                    fDataMax);
            cpglab("Time (s)", "", "Before Smoothing");
            cpgbox("BCNST", 0.0, 0, "BCNST", 0.0, 0);
            cpgsci(PG_CI_PLOT);
            cpgline(iOutBlockSize, g_pfXAxis, pfPlotBuf);
            cpgsci(PG_CI_DEF);

            fDataMin = g_pfOutBuf[0];
            fDataMax = g_pfOutBuf[0];
            for (i = 0; i < (iNumSamps - (iSampsPerWin - 1)); ++i)
            {
                if (g_pfOutBuf[i] < fDataMin)
                {
                    fDataMin = g_pfOutBuf[i];
                }
                if (g_pfOutBuf[i] > fDataMax)
                {
                    fDataMax = g_pfOutBuf[i];
                }
            }

            #ifdef DEBUG
            (void) printf("Minimum value of data             : %g\n",
                          fDataMin);
            (void) printf("Maximum value of data             : %g\n",
                          fDataMax);
            #endif

            cpgpanl(1, 2);
            /* erase just before plotting, to reduce flicker */
            cpgeras();
            for (i = 0; i < iOutBlockSize; ++i)
            {
                g_pfXAxis[i] = (float) (dDataSkipTime
                                        + (((iReadBlockCount - 1)
                                            * iOutBlockSize
                                            * dTSampInSec)
                                           + (i * dTSampInSec)));
            }

            cpgsvp(PG_VP_ML, PG_VP_MR, PG_VP_MB, PG_VP_MT);
            cpgswin(g_pfXAxis[0],
                    g_pfXAxis[iOutBlockSize-1],
                    fDataMin,
                    fDataMax);
            cpglab("Time (s)", "", "After Smoothing");
            cpgbox("BCNST", 0.0, 0, "BCNST", 0.0, 0);
            cpgsci(PG_CI_PLOT);
            cpgline(iOutBlockSize, g_pfXAxis, g_pfOutBuf);
            cpgsci(PG_CI_DEF);

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

                            (void) fclose(pFOut);
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
        }

        if (1 == iNumReads)
        {
            cIsLastBlock = YAPP_TRUE;
        }
    }

    (void) printf("DONE!\n");

    /* copy last (iSampsPerWin / 2) time samples to the output */
    (void) memset(g_pfOutBuf, '\0', (sizeof(float) * iOutBlockSize));
    (void) memcpy(g_pfOutBuf,
                  g_pfBuf + iNumSamps - (iSampsPerWin / 2),
                  (iSampsPerWin / 2) * sizeof(float));
    (void) fwrite(g_pfOutBuf,
                  sizeof(float),
                  (long) (iSampsPerWin / 2),
                  pFOut);

    /* print statistics */
    fMeanOrigAll /= iReadBlockCount;
    fRMSOrigAll /= (stYUM.iTimeSamps - (iSampsPerWin - 1) - 1);
    fRMSOrigAll = sqrtf(fRMSOrigAll);
    (void) printf("Original signal mean = %g\n", fMeanOrigAll);
    (void) printf("Original signal RMS = %g\n", fRMSOrigAll);
    fMeanSmoothedAll /= iReadBlockCount;
    fRMSSmoothedAll /= (stYUM.iTimeSamps - (iSampsPerWin - 1) - 1);
    fRMSSmoothedAll = sqrtf(fRMSSmoothedAll);
    (void) printf("Smoothed signal mean = %g\n", fMeanSmoothedAll);
    (void) printf("Smoothed signal RMS = %g\n", fRMSSmoothedAll);

    (void) fclose(pFOut);
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
    (void) printf("    -w  --width                          ");
    (void) printf("Width of boxcar window in milliseconds\n");
    (void) printf("                                         ");
    (void) printf("(default is 1 ms)\n");
    (void) printf("    -g  --graphics                       ");
    (void) printf("Turn on plotting\n");
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


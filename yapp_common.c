/**
 * @file yapp_common.c
 * Commom YAPP functionalities.
 *
 * @author Jayanth Chennamangalam
 * @date 2009.07.06
 */

#include "yapp.h"
#include "yapp_sigproc.h"

extern const double g_aadErfLookup[YAPP_ERF_ENTRIES][3];

void* g_apvMemTable[YAPP_MAX_MEMTABLE] = {0};
int g_iMemTableSize = 0;

/*
 * Determine the file type
 */
int YAPP_GetFileType(char *pcFile)
{
    char *pcExt = NULL;
    int iFormat = YAPP_RET_ERROR; 

    pcExt = strrchr(pcFile, '.');
    if (NULL == pcExt)
    {
        (void) fprintf(stderr,
                       "ERROR: Could not locate extension in file name %s!\n",
                       pcFile);
        return YAPP_RET_ERROR;
    }
    if (0 == strcmp(pcExt, EXT_DYNSPEC))
    {
        iFormat = YAPP_FORMAT_SPEC;
    }
    else if (0 == strcmp(pcExt, EXT_FIL))
    {
        iFormat = YAPP_FORMAT_FIL;
    }
    else
    {
        (void) fprintf(stderr,
                       "ERROR: Unknown file type with extension %s!\n",
                       pcExt);
        return YAPP_RET_ERROR;
    }

    return iFormat;
}


/*
 * Extracts the filename from a given path, minus the extension
 */
char* YAPP_GetFilenameFromPath(char *pcPath, char *pcExt)
{
    char *pcPos = NULL;
    char *pcPosPrev = NULL;
    char *pcFilename = NULL;
    int iSlashCount = 0;

    /* extract the non-extension part of the input file */
    pcPos = pcPath;
    do
    {
        pcPosPrev = pcPos;
        pcPos = strstr((pcPos + 1), "/");
        ++iSlashCount;
    }
    while (pcPos != NULL);
    --iSlashCount;
    pcPos = pcPosPrev + 1;

    if (0 == iSlashCount)   /* there was no slash */
    {
        pcPos = pcPath;
    }

    /* allocate memory for the filename */
    pcFilename = (char *) calloc((strlen(pcPos) - strlen(pcExt) + 1),
                                 sizeof(char));
    if (NULL == pcFilename)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation for filename failed! %s!\n",
                       strerror(errno));
        return NULL;
        /* TODO: handle NULL return in caller */
    }

    /* build the name of the PGPLOT device */
    (void) strncpy(pcFilename, pcPos, (strlen(pcPos) - strlen(pcExt)));

    return pcFilename;
}

/*
 * Calculates the threshold in terms of standard deviation.
 */
double YAPP_CalcThresholdInSigmas(int iTimeSamps)
{
    int iNumOutliers = 0;
    double dPOutlier = 0.0;
    double dErf = 0.0;
    double dNumSigmas = 0.0;
    double dErfRef = 0.0;
    double dErfArgRef = 0.0;
    double dNumSigmasRef = 0.0;
    double dErfDiff = 0.0;
    double dErfDiffMin = 0.0;
    int i = 0;

    /* calculate the threshold */
    /* number of points expected above the threshold, per DM channel */
    iNumOutliers = 10;
    /* calculate the probability of getting iNumOutliers outliers */
    dPOutlier = ((double) iNumOutliers) / iTimeSamps;
    /* calculate the error function */
    dErf = 1 - 2 * dPOutlier;

    dErfRef = g_aadErfLookup[0][0];
    dErfArgRef = g_aadErfLookup[0][1];
    dNumSigmasRef = g_aadErfLookup[0][2];
    dErfDiff = fabs(dErfRef - dErf);
    dErfDiffMin = dErfDiff;
    dNumSigmas = dNumSigmasRef;

    /* get the threshold in terms of standard deviation for the matching error
       function from the error function lookup table */
    for (i = 0; i < YAPP_ERF_ENTRIES; ++i)
    {
        dErfRef = g_aadErfLookup[i][0];
        dErfArgRef = g_aadErfLookup[i][1];
        dNumSigmasRef = g_aadErfLookup[i][2];

        dErfDiff = fabs(dErfRef - dErf);
        if (dErfDiff < dErfDiffMin)
        {
            dErfDiffMin = dErfDiff;
            dNumSigmas = dNumSigmasRef;
        }
    }

    if (dNumSigmas < MIN_THRES_IN_SIGMA)
    {
        (void) printf("Changing threshold from %.10g sigma to 3 sigma.\n",
                      dNumSigmas);
        dNumSigmas = MIN_THRES_IN_SIGMA;
    }

    return dNumSigmas;
}

/*
 * Retrieves the observatory name from its ID
 */
int YAPP_GetObsNameFromID(int iObsID, char *pcObs)
{
    switch (iObsID)
    {
        case YAPP_SP_OBSID_FAKE:
            (void) strcpy(pcObs, YAPP_SP_OBS_FAKE);
            break;

        case YAPP_SP_OBSID_AO:
            (void) strcpy(pcObs, YAPP_SP_OBS_AO);
            break;

        case YAPP_SP_OBSID_ORT:
            (void) strcpy(pcObs, YAPP_SP_OBS_ORT);
            break;

        case YAPP_SP_OBSID_NANCAY:
            (void) strcpy(pcObs, YAPP_SP_OBS_NANCAY);
            break;

        case YAPP_SP_OBSID_PARKES:
            (void) strcpy(pcObs, YAPP_SP_OBS_PARKES);
            break;

        case YAPP_SP_OBSID_JB:
            (void) strcpy(pcObs, YAPP_SP_OBS_JB);
            break;

        case YAPP_SP_OBSID_GBT:
            (void) strcpy(pcObs, YAPP_SP_OBS_GBT);
            break;

        case YAPP_SP_OBSID_GMRT:
            (void) strcpy(pcObs, YAPP_SP_OBS_GMRT);
            break;

        case YAPP_SP_OBSID_EFFELSBERG:
            (void) strcpy(pcObs, YAPP_SP_OBS_EFFELSBERG);
            break;

        default:
            (void) fprintf(stderr,
                           "ERROR: Unknown observatory ID %d!\n",
                           iObsID);
            return YAPP_RET_ERROR;
    }

    return YAPP_RET_SUCCESS;
}

/*
 * Reads metadata from file
 */
int YAPP_ReadMetadata(char *pcFileSpec, YUM_t *pstYUM)
{
    FILE *pFSpec = NULL;
    FILE *pFCfg = NULL;
    char acFileCfg[LEN_GENSTRING] = {0};
    int iFormat = DEF_FORMAT;
    int iChanGoodness = (int) YAPP_TRUE;
    float fFMin = 0.0;
    float fFMax = 0.0;
    float fChanBW = 0.0;
    int iNumChans = 0;
    float fSampSize = 0.0;      /* number of bits that make a sample */
    int iNumGoodChans = 0;
    char cIsBandFlipped = YAPP_FALSE;
    float fStatBW = 0.0;
    float fNoiseRMS = 0.0;
    float fThreshold = 0.0;
    double dNumSigmas = 0.0;
    double dTSamp = 0.0;        /* holds sampling time in ms */
    double dTSampInSec = 0.0;   /* holds sampling time in s */
    YAPP_SIGPROC_HEADER stHeader = {{0}};
    char acLabel[LEN_GENSTRING] = {0};
    int iHeaderLen = 0;
    int iFlagSplicedData = YAPP_FALSE;
    int iNumBands = 0;
    int iLen = 0;
    double dFChan = 0.0;
    float fFCh1 = 0.0;          /* frequency of the first channel */
    int iBytesPerFrame = 0;
    float fFCentre = 0.0;
    float fBW = 0.0;
    int iChanBeg = 0;
    int iChanEnd = 0;
    char acPulsar[LEN_GENSTRING] = {0};
    int iDay = 0;
    int iMonth = 0;
    int iYear = 0;
    int iHour = 0;
    int iMin = 0;
    float fSec = 0.0;
    char acSite[LEN_GENSTRING] = {0};
    double dTNextBF = 0.0;
    double dTBFInt = 0.0;
    int iBFTimeSects = 0;
    float *pfTimeSectGain = NULL;
    int iNumBadTimes = 0;
    int iTimeSamps = 0;
    struct stat stFileStats = {0};
    long lDataSizeTotal = 0;
    int iRet = YAPP_RET_SUCCESS;
    int i = 0;
    int j = 0;
    char *pcIsChanGood = NULL;
    float *pfBFTimeSectMean = NULL;
    float *pfBFGain = NULL;
    double (*padBadTimes)[][NUM_BAD_BOUNDS] = NULL;
    float *pfFreq = NULL;

    /* determine the file type */
    iFormat = YAPP_GetFileType(pcFileSpec);
    if (YAPP_RET_ERROR == iFormat)
    {
        (void) fprintf(stderr,
                       "ERROR: File type determination failed!\n");
        return YAPP_RET_ERROR;
    }
    else if (YAPP_FORMAT_SPEC == iFormat) /* 'spec' format */
    {
        /* NOTE: reading data in Desh's 'spec' file format + associated
           'spec_cfg' file format */
        fSampSize = (float) sizeof(float);  /* spec files are 32-bit floats */

        /* build the 'cfg' file name from the 'spec' file name, and open it */
        (void) strcpy(acFileCfg, pcFileSpec);
        (void) strcat(acFileCfg, SUFFIX_CFG);
        pFCfg = fopen(acFileCfg, "r");
        if (NULL == pFCfg)
        {
            (void) fprintf(stderr,
                           "ERROR: Opening file %s failed! %s.\n",
                           acFileCfg,
                           strerror(errno));
            return YAPP_RET_ERROR;
        }

        /* read the first few parameters from the 'cfg' file */
        iRet = fscanf(pFCfg,
                      " %lf %d %f %f %d %d %s %d %d %d %d %d %f %s %lf %lf",
                      &dTSamp,          /* in ms */
                      &iBytesPerFrame,  /* iNumChans * fSampSize */
                      &fFCentre,        /* in MHz */
                      &fBW,             /* in kHz */
                      &iChanBeg,
                      &iChanEnd,
                      acPulsar,
                      &iDay,
                      &iMonth,
                      &iYear,
                      &iHour,
                      &iMin,
                      &fSec,
                      acSite,
                      &dTNextBF,        /* in s */
                      &dTBFInt);        /* in s */

        /* handle negative bandwidths */
        if (fBW < 0)
        {
            fBW = -fBW;
            cIsBandFlipped = YAPP_TRUE;  /* NOTE: not used, as of now */
        }

        /* convert the bandwidth to MHz */
        fBW /= 1000.0;

        /* store a copy of the sampling interval in s */
        dTSampInSec = dTSamp / 1000.0;

        (void) printf("Field name                        : %s\n", acPulsar);
        (void) printf("Observing site                    : %s\n", acSite);
        (void) printf("Date of observation               : %d.%d.%d\n",
                      iYear,
                      iMonth,
                      iDay);
        (void) printf("Time of observation               : %d:%d:%g\n",
                      iHour,
                      iMin,
                      fSec);
        (void) printf("Bytes per frame                   : %d\n",
                      iBytesPerFrame);
        (void) printf("Centre frequency                  : %g MHz\n", fFCentre);
        (void) printf("Bandwidth                         : %g MHz\n", fBW);
        if (cIsBandFlipped)
        {
            (void) printf("                                    Band flip.\n");
        }
        else
        {
            (void) printf("                                    "
                          "No band flip.\n");
        }
        (void) printf("Sampling interval                 : %.10g ms\n", dTSamp);
        (void) printf("First channel index               : %d\n", iChanBeg);
        (void) printf("Last channel index                : %d\n", iChanEnd);

        /* calculate the number of channels */
        iNumChans = iChanEnd - iChanBeg + 1;
        (void) printf("Number of channels                : %d\n", iNumChans);

        /* calculate the channel bandwidth */
        fChanBW = fBW / iNumChans;  /* in MHz */
        (void) printf("Channel bandwidth                 : %.10g MHz\n",
                      fChanBW);

        /* calculate the absolute min and max frequencies */
        fFMin = fFCentre - (fBW / 2) + (fChanBW / 2);
        (void) printf("Lowest frequency                  : %.10g MHz\n", fFMin);
        fFMax = fFCentre + (fBW / 2) - (fChanBW / 2);
        (void) printf("Highest frequency                 : %.10g MHz\n", fFMax);

        pcIsChanGood = (char *) YAPP_Malloc(iNumChans,
                                            sizeof(char),
                                            YAPP_FALSE);
        if (NULL == pcIsChanGood)
        {
            perror("malloc - pcIsChanGood");
            (void) fclose(pFCfg);
            return YAPP_RET_ERROR;
        }

        /* read the channel goodness flags */
        for (i = 0; i < iNumChans; ++i)
        {
            iRet = fscanf(pFCfg, " %d", &iChanGoodness);
            pcIsChanGood[i] = (char) iChanGoodness;
            if (pcIsChanGood[i])
            {
                ++iNumGoodChans;
            }
        }
        (void) printf("Number of good channels           : %d\n",
                      iNumGoodChans);

        (void) printf("First band flip time              : %.10g s\n",
                      dTNextBF);
        (void) printf("Band flip interval                : %.10g s\n", dTBFInt);

        iRet = fscanf(pFCfg, " %d", &iBFTimeSects);

        (void) printf("Number of band flip time sections : %d\n", iBFTimeSects);

        pfBFTimeSectMean = (float *) YAPP_Malloc(iBFTimeSects,
                                                 sizeof(float),
                                                 YAPP_FALSE);
        if (NULL == pfBFTimeSectMean)
        {
            perror("malloc - pfBFTimeSectMean");
            (void) fclose(pFCfg);
            YAPP_CleanUp();
            return YAPP_RET_ERROR;
        }

        for (i = 0; i < iBFTimeSects; ++i)
        {
            iRet = fscanf(pFCfg, " %f", &pfBFTimeSectMean[i]);
        }

        pfBFGain = (float *) YAPP_Malloc((iNumChans * iBFTimeSects),
                                         sizeof(float),
                                         YAPP_FALSE);
        if (NULL == pfBFGain)
        {
            perror("malloc - pfBFGain");
            (void) fclose(pFCfg);
            YAPP_CleanUp();
            return YAPP_RET_ERROR;
        }

        for (i = 0; i < (iNumChans * iBFTimeSects); ++i)
        {
            iRet = fscanf(pFCfg, " %f", &pfBFGain[i]);
        }
        pfTimeSectGain = pfBFGain;

        iRet = fscanf(pFCfg, " %d", &iNumBadTimes);
        (void) printf("Number of bad time sections       : %d\n", iNumBadTimes);
        padBadTimes = (double(*) [][NUM_BAD_BOUNDS]) YAPP_Malloc(
                                            (iNumBadTimes * NUM_BAD_BOUNDS),
                                            sizeof(double),
                                            YAPP_FALSE);
        if (NULL == padBadTimes)
        {
            perror("malloc - padBadTimes");
            (void) fclose(pFCfg);
            YAPP_CleanUp();
            return YAPP_RET_ERROR;
        }
        for (i = 0; i < iNumBadTimes; ++i)
        {
            for (j = 0; j < NUM_BAD_BOUNDS; ++j)
            {
                iRet = fscanf(pFCfg, " %lf", &((*padBadTimes)[i][j]));
            }
        }

        (void) fclose(pFCfg);

        iRet = stat(pcFileSpec, &stFileStats);
        if (iRet != YAPP_RET_SUCCESS)
        {
            (void) fprintf(stderr,
                           "ERROR: Failed to stat %s: %s!\n",
                           pcFileSpec,
                           strerror(errno));
            YAPP_CleanUp();
            return YAPP_RET_ERROR;
        }
        lDataSizeTotal = (long) stFileStats.st_size;
        (void) printf("Duration of data in\n");
        (void) printf("    Bytes                         : %ld\n",
                      (lDataSizeTotal / iNumChans));
        iTimeSamps = (int) (lDataSizeTotal / (iNumChans * fSampSize));
        (void) printf("    Time samples                  : %d\n", iTimeSamps);
        (void) printf("    Time                          : %g s\n",
                      (iTimeSamps * dTSampInSec));
    }
    else    /* 'fil' format */
    {
        /* open the dynamic spectrum data file for reading */
        pFSpec = fopen(pcFileSpec, "r");
        if (NULL == pFSpec)
        {
            (void) fprintf(stderr,
                           "ERROR: Opening file %s failed! %s.\n",
                           pcFileSpec,
                           strerror(errno));
            YAPP_CleanUp();
            return YAPP_RET_ERROR;
        }

        /* read the parameters from the header section of the file */
        /* start with the 'HEADER_START' label */
        iRet = fread(&iLen, sizeof(iLen), 1, pFSpec);
        iRet = fread(acLabel, sizeof(char), iLen, pFSpec);
        acLabel[iLen] = '\0';
        if (strcmp(acLabel, "HEADER_START") != 0)
        {
            (void) fprintf(stderr,
                           "ERROR: Reading header failed!\n");
            YAPP_CleanUp();
            return YAPP_RET_ERROR;
        }
        iHeaderLen += (sizeof(iLen) + iLen);

        /* parse the rest of the header */
        while (strcmp(acLabel, "HEADER_END") != 0)
        {
            /* read field label length */
            iRet = fread(&iLen, sizeof(iLen), 1, pFSpec);
            /* read field label */
            iRet = fread(acLabel, sizeof(char), iLen, pFSpec);
            acLabel[iLen] = '\0';
            iHeaderLen += (sizeof(iLen) + iLen);
            if (0 == strcmp(acLabel, "source_name"))
            {
                iRet = fread(&iLen, sizeof(iLen), 1, pFSpec);
                iRet = fread(stHeader.acPulsar, sizeof(char), iLen, pFSpec);
                stHeader.acPulsar[iLen] = '\0';
                (void) strcpy(acPulsar, stHeader.acPulsar);
                iHeaderLen += (sizeof(iLen) + iLen);
            }
            else if (0 == strcmp(acLabel, "data_type"))
            {
                iRet = fread(&stHeader.iDataTypeID,
                             sizeof(stHeader.iDataTypeID),
                             1,
                             pFSpec);
                iHeaderLen += sizeof(stHeader.iDataTypeID);
            }
            else if (0 == strcmp(acLabel, "nchans"))
            {
                /* read number of channels */
                iRet = fread(&stHeader.iNumChans,
                             sizeof(stHeader.iNumChans),
                             1,
                             pFSpec);
                iNumChans = stHeader.iNumChans;
                iHeaderLen += sizeof(stHeader.iNumChans);
            }
            else if (0 == strcmp(acLabel, "fch1"))
            {
                /* read frequency of first channel */
                iRet = fread(&stHeader.dFChan1,
                             sizeof(stHeader.dFChan1),
                             1,
                             pFSpec);
                fFCh1 = (float) stHeader.dFChan1;
                iHeaderLen += sizeof(stHeader.dFChan1);
            }
            else if (0 == strcmp(acLabel, "foff"))
            {
                /* read channel bandwidth (labelled frequency offset) */
                iRet = fread(&stHeader.dChanBW,
                             sizeof(stHeader.dChanBW),
                             1,
                             pFSpec);
                fChanBW = (float) stHeader.dChanBW;
                iHeaderLen += sizeof(stHeader.dChanBW);
            }
            else if (0 == strcmp(acLabel, "nbits"))
            {
                /* read number of bits per sample */
                iRet = fread(&stHeader.iNumBits,
                             sizeof(stHeader.iNumBits),
                             1,
                             pFSpec);
                iHeaderLen += sizeof(stHeader.iNumBits);
            }
            else if (0 == strcmp(acLabel, "nifs"))
            {
                /* read number of IFs */
                iRet = fread(&stHeader.iNumIFs,
                             sizeof(stHeader.iNumIFs),
                             1,
                             pFSpec);
                iHeaderLen += sizeof(stHeader.iNumIFs);
            }
            else if (0 == strcmp(acLabel, "tsamp"))
            {
                /* read sampling time in seconds */
                iRet = fread(&stHeader.dTSamp,
                             sizeof(stHeader.dTSamp),
                             1,
                             pFSpec);
                dTSampInSec = stHeader.dTSamp;
                dTSamp = dTSampInSec * 1e3;     /* in ms */
                iHeaderLen += sizeof(stHeader.dTSamp);
            }
            else if (0 == strcmp(acLabel, "tstart"))
            {
                /* read timestamp of first sample (MJD) */
                iRet = fread(&stHeader.dTStart,
                             sizeof(stHeader.dTStart),
                             1,
                             pFSpec);
                iHeaderLen += sizeof(stHeader.dTStart);
            }
            else if (0 == strcmp(acLabel, "telescope_id"))
            {
                /* read telescope ID */
                iRet = fread(&stHeader.iObsID,
                             sizeof(stHeader.iObsID),
                             1,
                             pFSpec);
                (void) YAPP_GetObsNameFromID(stHeader.iObsID, acSite);
                iHeaderLen += sizeof(stHeader.iObsID);
            }
            else if (0 == strcmp(acLabel, "machine_id"))
            {
                /* read backend ID */
                iRet = fread(&stHeader.iBackendID,
                             sizeof(stHeader.iBackendID),
                             1,
                             pFSpec);
                iHeaderLen += sizeof(stHeader.iBackendID);
            }
            else if (0 == strcmp(acLabel, "src_raj"))
            {
                /* read source RA (J2000) */
                iRet = fread(&stHeader.dSourceRA,
                             sizeof(stHeader.dSourceRA),
                             1,
                             pFSpec);
                iHeaderLen += sizeof(stHeader.dSourceRA);
            }
            else if (0 == strcmp(acLabel, "src_dej"))
            {
                /* read source declination (J2000) */
                iRet = fread(&stHeader.dSourceDec,
                             sizeof(stHeader.dSourceDec),
                             1,
                             pFSpec);
                iHeaderLen += sizeof(stHeader.dSourceDec);
            }
            else if (0 == strcmp(acLabel, "az_start"))
            {
                /* read azimuth start */
                iRet = fread(&stHeader.dAzStart,
                             sizeof(stHeader.dAzStart),
                             1,
                             pFSpec);
                iHeaderLen += sizeof(stHeader.dAzStart);
            }
            else if (0 == strcmp(acLabel, "za_start"))
            {
                /* read ZA start */
                iRet = fread(&stHeader.dZAStart,
                             sizeof(stHeader.dZAStart),
                             1,
                             pFSpec);
                iHeaderLen += sizeof(stHeader.dZAStart);
            }
            /* DTS-specific field */
            else if (0 == strcmp(acLabel, "refdm"))
            {
                /* read reference DM */
                iRet = fread(&stHeader.dDM, sizeof(stHeader.dDM), 1, pFSpec);
                iHeaderLen += sizeof(stHeader.dDM);
            }
            else if (0 == strcmp(acLabel, "barycentric"))
            {
                /* read barycentric flag */
                iRet = fread(&stHeader.iFlagBary,
                             sizeof(stHeader.iFlagBary),
                             1,
                             pFSpec);
                iHeaderLen += sizeof(stHeader.iFlagBary);
            }
            else if (0 == strcmp(acLabel, "FREQUENCY_START"))
            {
                iFlagSplicedData = YAPP_TRUE;

                /* read field label length */
                iRet = fread(&iLen, sizeof(iLen), 1, pFSpec);
                /* read field label */
                iRet = fread(acLabel, sizeof(char), iLen, pFSpec);
                acLabel[iLen] = '\0';
                iHeaderLen += (sizeof(iLen) + iLen);
                if (0 == strcmp(acLabel, "nchans"))
                {
                    /* read number of channels */
                    iRet = fread(&stHeader.iNumChans,
                                 sizeof(stHeader.iNumChans),
                                 1,
                                 pFSpec);
                    iNumChans = stHeader.iNumChans;
                    iHeaderLen += sizeof(stHeader.iNumChans);
                }
                else
                {
                    (void) fprintf(stderr,
                                   "ERROR: Unexpected label %s found!",
                                   acLabel);
                    (void) fclose(pFSpec);
                    YAPP_CleanUp();
                    return YAPP_RET_ERROR;
                }

                /* allocate memory for the frequency channel array read from
                   the header */
                pfFreq = (float *) YAPP_Malloc(iNumChans,
                                               sizeof(float),
                                               YAPP_FALSE);
                if (NULL == pfFreq)
                {
                    perror("malloc - pfFreq");
                    (void) fclose(pFSpec);
                    YAPP_CleanUp();
                    return YAPP_RET_ERROR;
                }

                /* store in the reverse order */
                i = iNumChans - 1;
                /* parse frequency channels for spliced data */
                while (strcmp(acLabel, "FREQUENCY_END") != 0)
                {
                    /* read field label length */
                    iRet = fread(&iLen, sizeof(iLen), 1, pFSpec);
                    /* read field label */
                    iRet = fread(acLabel, sizeof(char), iLen, pFSpec);
                    acLabel[iLen] = '\0';
                    iHeaderLen += (sizeof(iLen) + iLen);
                    if (0 == strcmp(acLabel, "fchannel"))
                    {
                        iRet = fread(&dFChan, sizeof(dFChan), 1, pFSpec);
                        pfFreq[i] = (float) dFChan;
                        iHeaderLen += sizeof(dFChan);
                    }
                    else
                    {
                        /* print a warning about encountering unknown field label */
                        if (strcmp(acLabel, "FREQUENCY_END") != 0)
                        {
                            (void) fprintf(stderr,
                                           "WARNING: Unknown field label %s "
                                           "encountered!\n", acLabel);
                            (void) fclose(pFSpec);
                            YAPP_CleanUp();
                            return YAPP_RET_ERROR;
                        }
                    }
                    --i;
                }
            }
            else
            {
                /* print a warning about encountering unknown field label */
                if (strcmp(acLabel, "HEADER_END") != 0)
                {
                    (void) fprintf(stderr,
                                   "WARNING: Unknown field label %s "
                                   "encountered!\n", acLabel);
                }
            }
        }

        /* close the file, it will be opened later for reading data */
        (void) fclose(pFSpec);

        (void) printf("Header length                     : %d\n", iHeaderLen);
        (void) printf("Field name                        : %s\n", acPulsar);
        (void) printf("Observing site                    : %s\n", acSite);
        (void) printf("Number of channels                : %d\n", iNumChans);

        if (fChanBW < 0.0)
        {
            /* make the channel bandwidth positive */
            fChanBW = fabs(fChanBW);
            fFMax = fFCh1;
            fFMin = fFMax - (iNumChans * fChanBW);
        }
        else
        {
            fFMin = fFCh1;
            fFMax = fFMin + (iNumChans * fChanBW);
        }

        if (YAPP_TRUE == iFlagSplicedData)
        {
            /* in spliced data files, the first frequency is always the
               highest - since we have inverted the array, it is the last
               frequency */
            fFMax = pfFreq[iNumChans-1];
            /* get the lowest frequency */
            fFMin = pfFreq[0];
            /* calculate the channel bandwidth */
            fChanBW = pfFreq[1] - pfFreq[0];

			/* TODO: Number-of-bands calculation not accurate */
            for (i = 1; i < iNumChans; ++i)
            {
                /*if (fabsf(pfFreq[i] - pfFreq[i-1]) > fChanBW)*/
                /* kludge: */
                if (fabsf(pfFreq[i] - pfFreq[i-1]) > (2 * fChanBW))
                {
                    ++iNumBands;
                    if (YAPP_MAX_NUM_BANDS == iNumBands)
                    {
                        (void) printf("WARNING: "
                                      "Maximum number of bands reached!\n");
                        break;
                    }
                }
            }
            (void) printf("Estimated number of bands         : %d\n",
                          iNumBands);
        }

        /* TODO: find out the discontinuities and print them as well, also
                 number of spliced bands */

        (void) printf("Channel bandwidth                 : %.10g MHz\n",
                      fChanBW);
        (void) printf("Lowest frequency                  : %.10g MHz\n", fFMin);
        (void) printf("Highest frequency                 : %.10g MHz\n", fFMax);

        (void) printf("Sampling interval                 : %.10g ms\n", dTSamp);
        (void) printf("Timestamp of first sample         : %.16g MJD\n",
                      stHeader.dTStart);
        (void) printf("Number of bits per sample         : %d\n",
                      stHeader.iNumBits);
        (void) printf("Number of IFs                     : %d\n",
                      stHeader.iNumIFs);

        fSampSize = ((float) stHeader.iNumBits) / 8;

        iRet = stat(pcFileSpec, &stFileStats);
        if (iRet != YAPP_RET_SUCCESS)
        {
            (void) fprintf(stderr,
                           "ERROR: Failed to stat %s: %s!\n",
                           pcFileSpec,
                           strerror(errno));
            YAPP_CleanUp();
            return YAPP_RET_ERROR;
        }
        lDataSizeTotal = (long) stFileStats.st_size - iHeaderLen;
        (void) printf("Duration of data in\n");
        (void) printf("    Bytes                         : %ld\n",
                      (lDataSizeTotal / iNumChans));
        iTimeSamps = (int) (lDataSizeTotal / (iNumChans * fSampSize));
        (void) printf("    Time samples                  : %d\n", iTimeSamps);
        (void) printf("    Time                          : %g s\n",
                      (iTimeSamps * dTSampInSec));

        /* set number of good channels to number of channels - no support for
           SIGPROC ignore files yet */
        iNumGoodChans = iNumChans;
    }

    /* calculate the threshold */
    dNumSigmas = YAPP_CalcThresholdInSigmas(iTimeSamps);
    if ((double) YAPP_RET_ERROR == dNumSigmas)
    {
        (void) fprintf(stderr, "ERROR: Threshold calculation failed!\n");
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }
    fStatBW = iNumGoodChans * fChanBW;  /* in MHz */
    (void) printf("Usable bandwidth                  : %g MHz\n", fStatBW);
    fNoiseRMS = 1.0 / sqrt(fStatBW * dTSamp * 1e3);
    (void) printf("Expected noise RMS                : %g\n", fNoiseRMS);
    fThreshold = (float) (dNumSigmas * fNoiseRMS);
    (void) printf("Threshold                         : %g\n", fThreshold);

    return YAPP_RET_SUCCESS;
}

/*
 * Read one block of data from disk
 */
int YAPP_ReadData(FILE *pFSpec,
                  float *pfBuf,
                  float fSampSize,
                  int iTotSampsPerBlock)
{
    char *pcBuf = NULL;
    char *pcCur = NULL;
    int iReadItems = 0;
    int i = 0;
    short int sSample = 0;

    /* allocate memory for the byte buffer, based on the total number of samples
       per block (= number of channels * number of time samples per block) */
    pcBuf = (char *) malloc((int) (iTotSampsPerBlock * fSampSize));
    if (NULL == pcBuf)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation failed for buffer! %s!\n",
                       strerror(errno));
        return YAPP_RET_ERROR;
    }

    /* read data into the byte buffer */
    iReadItems = fread(pcBuf,
                       sizeof(char),
                       (int) (iTotSampsPerBlock * fSampSize),
                       pFSpec);
    if (ferror(pFSpec))
    {
        (void) fprintf(stderr, "ERROR: File read failed!\n");
        free(pcBuf);
        return YAPP_RET_ERROR;
    }
    iReadItems = (int) ((float) iReadItems / fSampSize);

    if (YAPP_SAMPSIZE_32 == (fSampSize * YAPP_BYTE2BIT_FACTOR))
    {
        /* 32-bit/4-byte data */
        /* copy data from the byte buffer to the float buffer */
        /* TODO: check which of these two is right: */
        (void) memcpy(pfBuf, pcBuf, (int) (iTotSampsPerBlock * fSampSize));
        /*(void) memcpy(pfBuf, pcBuf, (int) (iReadItems * fSampSize));*/
    }
    else if (YAPP_SAMPSIZE_16 == (fSampSize * YAPP_BYTE2BIT_FACTOR))
    {
        /* 16-bit/2-byte data */
        /* copy data from the byte buffer to the float buffer */
        for (i = 0; i < iReadItems; ++i)
        {
            pcCur = pcBuf + (int) (i * fSampSize);
            (void) memcpy(&sSample, pcCur, (int) fSampSize);
            pfBuf[i] = (float) sSample;
        }
    }
    else if (YAPP_SAMPSIZE_8 == (fSampSize * YAPP_BYTE2BIT_FACTOR))
    {
        /* 8-bit/1-byte data */
        /* copy data from the byte buffer to the float buffer */
        for (i = 0; i < iReadItems; ++i)
        {
            pfBuf[i] = (float) pcBuf[i];
        }
    }
    else if (YAPP_SAMPSIZE_4 == (fSampSize * YAPP_BYTE2BIT_FACTOR))
    {
        /* 4-bit/0.5-byte data */
        /* copy data from the byte buffer to the float buffer */
        for (i = 0; i < (int) (iReadItems * fSampSize); ++i)
        {
            /* copy lower 4 bits */
            pfBuf[2*i] = pcBuf[i] & 0x0F;
            /* copy upper 4 bits */
            pfBuf[(2*i)+1] = (pcBuf[i] & 0xF0) >> 4;
        }
    }

    /* free the byte buffer */
    free(pcBuf);

    return iReadItems;
}

/*
 * The memory allocator
 */
void* YAPP_Malloc(size_t iNumItems, size_t iSize, int iZero)
{
    void *pvMem = NULL;

    if (YAPP_MAX_MEMTABLE == g_iMemTableSize)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation failed! "
                       "Memory allocation table full!\n");
        return NULL;
    }

    if (YAPP_FALSE == iZero)    /* no need to zero the memory, use malloc() */
    {
        pvMem = malloc(iNumItems * iSize);
    }
    else                        /* zero the memory, use calloc() */
    {
        pvMem = calloc(iNumItems, iSize);
    }
    if (NULL == pvMem)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation failed! %s!\n",
                       strerror(errno));
        return NULL;
    }

    /* add pointer to memory allocation table */
    g_apvMemTable[g_iMemTableSize] = pvMem;
    ++g_iMemTableSize;

    return pvMem;
}

/*
 * The garbage collector - frees all pointers in the memory allocation table
 */
void YAPP_CleanUp()
{
    int i = 0;

    for (i = 0; i < g_iMemTableSize; ++i)
    {
        free(g_apvMemTable[i]);
    }

    return;
}

/*
 * Registers handlers for SIGTERM and CTRL+C
 */
int YAPP_RegisterSignalHandlers()
{
    struct sigaction stSigHandler = {{0}};
    int iRet = YAPP_RET_SUCCESS;

    /* register the CTRL+C-handling function */
    stSigHandler.sa_handler = YAPP_HandleStopSignals;
    iRet = sigaction(SIGINT, &stSigHandler, NULL);
    if (iRet != YAPP_RET_SUCCESS)
    {
        (void) fprintf(stderr,
                       "ERROR: Handler registration failed for signal %d!\n",
                       SIGINT);
        return YAPP_RET_ERROR;
    }

    /* register the SIGTERM-handling function */
    stSigHandler.sa_handler = YAPP_HandleStopSignals;
    iRet = sigaction(SIGTERM, &stSigHandler, NULL);
    if (iRet != YAPP_RET_SUCCESS)
    {
        (void) fprintf(stderr,
                       "ERROR: Handler registration failed for signal %d!\n",
                       SIGTERM);
        return YAPP_RET_ERROR;
    }

    return YAPP_RET_SUCCESS;
}

/*
 * Catches SIGTERM and CTRL+C and cleans up before exiting
 */
void YAPP_HandleStopSignals(int iSigNo)
{
    /* clean up */
    YAPP_CleanUp();

    /* exit */
    exit(YAPP_RET_SUCCESS);

    /* never reached */
    return;
}


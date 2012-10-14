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

/* global memory management */
void* g_apvMemTable[YAPP_MAX_MEMTABLE] = {0};
int g_iMemTableSize = 0;

/* PGPLOT device ID */
int g_iPGDev = 0;

/* data file */
FILE *g_pFSpec = NULL;

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
    pcFilename = (char *) YAPP_Malloc((strlen(pcPos) - strlen(pcExt) + 1),
                                      sizeof(char),
                                      YAPP_TRUE);
    if (NULL == pcFilename)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation for filename failed! %s!\n",
                       strerror(errno));
        return NULL;
        /* TODO: handle NULL return in caller */
    }

    (void) strncpy(pcFilename, pcPos, (strlen(pcPos) - strlen(pcExt)));

    return pcFilename;
}

/*
 * Extracts the filename from a given path, with the extension
 */
char* YAPP_GetFilenameWithExtFromPath(char *pcPath)
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
    pcFilename = (char *) YAPP_Malloc((strlen(pcPos) + 1),
                                      sizeof(char),
                                      YAPP_TRUE);
    if (NULL == pcFilename)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation for filename failed! %s!\n",
                       strerror(errno));
        return NULL;
        /* TODO: handle NULL return in caller */
    }

    (void) strncpy(pcFilename, pcPos, strlen(pcPos));

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
int YAPP_ReadMetadata(char *pcFileSpec, int iFormat, YUM_t *pstYUM)
{
    int iRet = YAPP_RET_SUCCESS;

    switch (iFormat)
    {
        case YAPP_FORMAT_SPEC:
            iRet = YAPP_ReadDASCfg(pcFileSpec, pstYUM);
            if (iRet != YAPP_RET_SUCCESS)
            {
                (void) fprintf(stderr,
                               "ERROR: Reading specfile configuration information "
                               "failed!\n");
                return YAPP_RET_ERROR;
            }
            break;

        case YAPP_FORMAT_FIL:
            iRet = YAPP_ReadSIGPROCHeader(pcFileSpec, pstYUM);
            if (iRet != YAPP_RET_SUCCESS)
            {
                (void) fprintf(stderr,
                               "ERROR: Reading filterbank file header failed!\n");
                return YAPP_RET_ERROR;
            }
            break;

        default:
            (void) fprintf(stderr,
                           "ERROR: Unknown file format %d!\n",
                           iFormat);
            return YAPP_RET_ERROR;
    }

    return YAPP_RET_SUCCESS;
}

int YAPP_ReadDASCfg(char *pcFileSpec, YUM_t *pstYUM)
{
    FILE *pFCfg = NULL;
    char acFileCfg[LEN_GENSTRING] = {0};
    int iChanGoodness = (int) YAPP_TRUE;
    double dTSampInSec = 0.0;   /* holds sampling time in s */
    int iBytesPerFrame = 0;
    int iChanBeg = 0;
    int iChanEnd = 0;
    int iDay = 0;
    int iMonth = 0;
    int iYear = 0;
    int iHour = 0;
    int iMin = 0;
    float fSec = 0.0;
    struct stat stFileStats = {0};
    int iRet = YAPP_RET_SUCCESS;
    int i = 0;
    int j = 0;

    /* NOTE: reading data in Desh's 'spec' file format + associated
       'spec_cfg' file format */
    pstYUM->fSampSize = (float) sizeof(float);  /* spec files are 32-bit floats */
    pstYUM->iNumBits = pstYUM->fSampSize * YAPP_BYTE2BIT_FACTOR;

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
                  &pstYUM->dTSamp,    /* in ms */
                  &iBytesPerFrame,  /* iNumChans * pstYUM->fSampSize */
                  &pstYUM->fFCentre,  /* in MHz */
                  &pstYUM->fBW,       /* in kHz */
                  &iChanBeg,
                  &iChanEnd,
                  pstYUM->acPulsar,
                  &iDay,
                  &iMonth,
                  &iYear,
                  &iHour,
                  &iMin,
                  &fSec,
                  pstYUM->acSite,
                  &pstYUM->dTNextBF,  /* in s */
                  &pstYUM->dTBFInt);  /* in s */

    /* handle negative bandwidths */
    if (pstYUM->fBW < 0)
    {
        pstYUM->fBW = -pstYUM->fBW;
        pstYUM->cIsBandFlipped = YAPP_TRUE;  /* NOTE: not used, as of now */
    }

    /* convert the bandwidth to MHz */
    pstYUM->fBW /= 1000.0;

    /* store a copy of the sampling interval in s */
    dTSampInSec = pstYUM->dTSamp / 1000.0;

    /* calculate the number of channels */
    pstYUM->iNumChans = iChanEnd - iChanBeg + 1;

    /* calculate the channel bandwidth */
    pstYUM->fChanBW = pstYUM->fBW / pstYUM->iNumChans;  /* in MHz */

    /* calculate the absolute min and max frequencies */
    pstYUM->fFMin = pstYUM->fFCentre - (pstYUM->fBW / 2) + (pstYUM->fChanBW / 2);
    pstYUM->fFMax = pstYUM->fFCentre + (pstYUM->fBW / 2) - (pstYUM->fChanBW / 2);

    pstYUM->pcIsChanGood = (char *) YAPP_Malloc(pstYUM->iNumChans,
                                        sizeof(char),
                                        YAPP_FALSE);
    if (NULL == pstYUM->pcIsChanGood)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation for channel goodness "
                       "flag failed! %s!\n",
                       strerror(errno));
        (void) fclose(pFCfg);
        return YAPP_RET_ERROR;
    }

    /* read the channel goodness flags */
    for (i = 0; i < pstYUM->iNumChans; ++i)
    {
        iRet = fscanf(pFCfg, " %d", &iChanGoodness);
        pstYUM->pcIsChanGood[i] = (char) iChanGoodness;
        if (pstYUM->pcIsChanGood[i])
        {
            ++pstYUM->iNumGoodChans;
        }
    }

    iRet = fscanf(pFCfg, " %d", &pstYUM->iBFTimeSects);

    pstYUM->pfBFTimeSectMean = (float *) YAPP_Malloc(pstYUM->iBFTimeSects,
                                             sizeof(float),
                                             YAPP_FALSE);
    if (NULL == pstYUM->pfBFTimeSectMean)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation for time sectioin mean "
                       "failed! %s!\n",
                       strerror(errno));
        (void) fclose(pFCfg);
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }

    for (i = 0; i < pstYUM->iBFTimeSects; ++i)
    {
        iRet = fscanf(pFCfg, " %f", &pstYUM->pfBFTimeSectMean[i]);
    }

    pstYUM->pfBFGain = (float *) YAPP_Malloc((pstYUM->iNumChans * pstYUM->iBFTimeSects),
                                     sizeof(float),
                                     YAPP_FALSE);
    if (NULL == pstYUM->pfBFGain)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation for beam flip gain "
                       "failed! %s!\n",
                       strerror(errno));
        (void) fclose(pFCfg);
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }

    for (i = 0; i < (pstYUM->iNumChans * pstYUM->iBFTimeSects); ++i)
    {
        iRet = fscanf(pFCfg, " %f", &pstYUM->pfBFGain[i]);
    }

    iRet = fscanf(pFCfg, " %d", &pstYUM->iNumBadTimes);

    pstYUM->padBadTimes = (double(*) [][NUM_BAD_BOUNDS]) YAPP_Malloc(
                                        (pstYUM->iNumBadTimes * NUM_BAD_BOUNDS),
                                        sizeof(double),
                                        YAPP_FALSE);
    if (NULL == pstYUM->padBadTimes)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation for bad times failed! "
                       "%s!\n",
                       strerror(errno));
        (void) fclose(pFCfg);
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }
    for (i = 0; i < pstYUM->iNumBadTimes; ++i)
    {
        for (j = 0; j < NUM_BAD_BOUNDS; ++j)
        {
            iRet = fscanf(pFCfg, " %lf", &((*pstYUM->padBadTimes)[i][j]));
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
    pstYUM->lDataSizeTotal = (long) stFileStats.st_size;
    pstYUM->iTimeSamps = (int) (pstYUM->lDataSizeTotal / (pstYUM->iNumChans * pstYUM->fSampSize));

    pstYUM->iNumBands = 1;

    return YAPP_RET_SUCCESS;
}

int YAPP_ReadSIGPROCHeader(char *pcFileSpec, YUM_t *pstYUM)
{
    double dTSampInSec = 0.0;   /* holds sampling time in s */
    char acLabel[LEN_GENSTRING] = {0};
    int iLen = 0;
    double dFChan = 0.0;
    struct stat stFileStats = {0};
    int iRet = YAPP_RET_SUCCESS;
    int i = 0;
    int iDataTypeID = 0;
    double dFChan1 = 0.0;
    float fFCh1 = 0.0;
    double dChanBW = 0.0;
    double dTStart = 0.0;
    int iObsID = 0;

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

    /* read the parameters from the header section of the file */
    /* start with the 'HEADER_START' label */
    iRet = fread(&iLen, sizeof(iLen), 1, g_pFSpec);
    /* if this is a file with header, iLen should be strlen(HEADER_START) */
    if (iLen != strlen("HEADER_START"))
    {
        /* this must be a headerless/header-separated filterbank file, so open
           the external header file */
        iRet = YAPP_ReadSIGPROCHeaderFile(pcFileSpec, pstYUM);
        if (iRet != YAPP_RET_SUCCESS)
        {
            (void) fprintf(stderr,
                           "ERROR: Reading header file failed!\n");
            YAPP_CleanUp();
            return YAPP_RET_ERROR;
        }
        return YAPP_RET_SUCCESS;
    }
    iRet = fread(acLabel, sizeof(char), iLen, g_pFSpec);
    acLabel[iLen] = '\0';
    if (strcmp(acLabel, "HEADER_START") != 0)
    {
        (void) fprintf(stderr,
                       "ERROR: Missing label 'HEADER_START'!\n");
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }
    pstYUM->iHeaderLen += (sizeof(iLen) + iLen);

    /* parse the rest of the header */
    while (strcmp(acLabel, "HEADER_END") != 0)
    {
        /* read field label length */
        iRet = fread(&iLen, sizeof(iLen), 1, g_pFSpec);
        /* read field label */
        iRet = fread(acLabel, sizeof(char), iLen, g_pFSpec);
        acLabel[iLen] = '\0';
        pstYUM->iHeaderLen += (sizeof(iLen) + iLen);
        if (0 == strcmp(acLabel, "source_name"))
        {
            iRet = fread(&iLen, sizeof(iLen), 1, g_pFSpec);
            iRet = fread(pstYUM->acPulsar, sizeof(char), iLen, g_pFSpec);
            pstYUM->acPulsar[iLen] = '\0';
            pstYUM->iHeaderLen += (sizeof(iLen) + iLen);
        }
        else if (0 == strcmp(acLabel, "data_type"))
        {
            iRet = fread(&iDataTypeID,
                         sizeof(iDataTypeID),
                         1,
                         g_pFSpec);
            pstYUM->iHeaderLen += sizeof(iDataTypeID);
        }
        else if (0 == strcmp(acLabel, "nchans"))
        {
            /* read number of channels */
            iRet = fread(&pstYUM->iNumChans,
                         sizeof(pstYUM->iNumChans),
                         1,
                         g_pFSpec);
            pstYUM->iHeaderLen += sizeof(pstYUM->iNumChans);
        }
        else if (0 == strcmp(acLabel, "fch1"))
        {
            /* read frequency of first channel */
            iRet = fread(&dFChan1,
                         sizeof(dFChan1),
                         1,
                         g_pFSpec);
            fFCh1 = (float) dFChan1;
            pstYUM->iHeaderLen += sizeof(dFChan1);
        }
        else if (0 == strcmp(acLabel, "foff"))
        {
            /* read channel bandwidth (labelled frequency offset) */
            iRet = fread(&dChanBW,
                         sizeof(dChanBW),
                         1,
                         g_pFSpec);
            pstYUM->fChanBW = (float) dChanBW;
            pstYUM->iHeaderLen += sizeof(dChanBW);
        }
        else if (0 == strcmp(acLabel, "nbits"))
        {
            /* read number of bits per sample */
            iRet = fread(&pstYUM->iNumBits,
                         sizeof(pstYUM->iNumBits),
                         1,
                         g_pFSpec);
            pstYUM->iHeaderLen += sizeof(pstYUM->iNumBits);
        }
        else if (0 == strcmp(acLabel, "nifs"))
        {
            /* read number of IFs */
            iRet = fread(&pstYUM->iNumIFs,
                         sizeof(pstYUM->iNumIFs),
                         1,
                         g_pFSpec);
            pstYUM->iHeaderLen += sizeof(pstYUM->iNumIFs);
        }
        else if (0 == strcmp(acLabel, "tsamp"))
        {
            /* read sampling time in seconds */
            iRet = fread(&pstYUM->dTSamp,
                         sizeof(pstYUM->dTSamp),
                         1,
                         g_pFSpec);
            dTSampInSec = pstYUM->dTSamp;
            pstYUM->dTSamp = dTSampInSec * 1e3;     /* in ms */
            pstYUM->iHeaderLen += sizeof(pstYUM->dTSamp);
        }
        else if (0 == strcmp(acLabel, "tstart"))
        {
            /* read timestamp of first sample (MJD) */
            iRet = fread(&dTStart,
                         sizeof(dTStart),
                         1,
                         g_pFSpec);
            pstYUM->iHeaderLen += sizeof(dTStart);
        }
        else if (0 == strcmp(acLabel, "telescope_id"))
        {
            /* read telescope ID */
            iRet = fread(&iObsID,
                         sizeof(iObsID),
                         1,
                         g_pFSpec);
            (void) YAPP_GetObsNameFromID(iObsID, pstYUM->acSite);
            pstYUM->iHeaderLen += sizeof(iObsID);
        }
        else if (0 == strcmp(acLabel, "machine_id"))
        {
            /* read backend ID */
            iRet = fread(&pstYUM->iBackendID,
                         sizeof(pstYUM->iBackendID),
                         1,
                         g_pFSpec);
            pstYUM->iHeaderLen += sizeof(pstYUM->iBackendID);
        }
        else if (0 == strcmp(acLabel, "src_raj"))
        {
            /* read source RA (J2000) */
            iRet = fread(&pstYUM->dSourceRA,
                         sizeof(pstYUM->dSourceRA),
                         1,
                         g_pFSpec);
            pstYUM->iHeaderLen += sizeof(pstYUM->dSourceRA);
        }
        else if (0 == strcmp(acLabel, "src_dej"))
        {
            /* read source declination (J2000) */
            iRet = fread(&pstYUM->dSourceDec,
                         sizeof(pstYUM->dSourceDec),
                         1,
                         g_pFSpec);
            pstYUM->iHeaderLen += sizeof(pstYUM->dSourceDec);
        }
        else if (0 == strcmp(acLabel, "az_start"))
        {
            /* read azimuth start */
            iRet = fread(&pstYUM->dAzStart,
                         sizeof(pstYUM->dAzStart),
                         1,
                         g_pFSpec);
            pstYUM->iHeaderLen += sizeof(pstYUM->dAzStart);
        }
        else if (0 == strcmp(acLabel, "za_start"))
        {
            /* read ZA start */
            iRet = fread(&pstYUM->dZAStart,
                         sizeof(pstYUM->dZAStart),
                         1,
                         g_pFSpec);
            pstYUM->iHeaderLen += sizeof(pstYUM->dZAStart);
        }
        /* DTS-specific field */
        else if (0 == strcmp(acLabel, "refdm"))
        {
            /* read reference DM */
            iRet = fread(&pstYUM->dDM, sizeof(pstYUM->dDM), 1, g_pFSpec);
            pstYUM->iHeaderLen += sizeof(pstYUM->dDM);
        }
        else if (0 == strcmp(acLabel, "barycentric"))
        {
            /* read barycentric flag */
            iRet = fread(&pstYUM->iFlagBary,
                         sizeof(pstYUM->iFlagBary),
                         1,
                         g_pFSpec);
            pstYUM->iHeaderLen += sizeof(pstYUM->iFlagBary);
        }
        else if (0 == strcmp(acLabel, "FREQUENCY_START"))
        {
            pstYUM->iFlagSplicedData = YAPP_TRUE;

            /* read field label length */
            iRet = fread(&iLen, sizeof(iLen), 1, g_pFSpec);
            /* read field label */
            iRet = fread(acLabel, sizeof(char), iLen, g_pFSpec);
            acLabel[iLen] = '\0';
            pstYUM->iHeaderLen += (sizeof(iLen) + iLen);
            if (0 == strcmp(acLabel, "nchans"))
            {
                /* read number of channels */
                iRet = fread(&pstYUM->iNumChans,
                             sizeof(pstYUM->iNumChans),
                             1,
                             g_pFSpec);
                pstYUM->iHeaderLen += sizeof(pstYUM->iNumChans);
            }
            else
            {
                (void) fprintf(stderr,
                               "ERROR: Unexpected label %s found!",
                               acLabel);
                YAPP_CleanUp();
                return YAPP_RET_ERROR;
            }

            /* allocate memory for the frequency channel array read from
               the header */
            pstYUM->pfFreq = (float *) YAPP_Malloc(pstYUM->iNumChans,
                                           sizeof(float),
                                           YAPP_FALSE);
            if (NULL == pstYUM->pfFreq)
            {
                (void) fprintf(stderr,
                               "ERROR: Memory allocation for frequency "
                               " channel array failed! %s!\n",
                               strerror(errno));
                YAPP_CleanUp();
                return YAPP_RET_ERROR;
            }

            /* store in the reverse order */
            i = pstYUM->iNumChans - 1;
            /* parse frequency channels for spliced data */
            while (strcmp(acLabel, "FREQUENCY_END") != 0)
            {
                /* read field label length */
                iRet = fread(&iLen, sizeof(iLen), 1, g_pFSpec);
                /* read field label */
                iRet = fread(acLabel, sizeof(char), iLen, g_pFSpec);
                acLabel[iLen] = '\0';
                pstYUM->iHeaderLen += (sizeof(iLen) + iLen);
                if (0 == strcmp(acLabel, "fchannel"))
                {
                    iRet = fread(&dFChan, sizeof(dFChan), 1, g_pFSpec);
                    pstYUM->pfFreq[i] = (float) dFChan;
                    pstYUM->iHeaderLen += sizeof(dFChan);
                }
                else
                {
                    /* print a warning about encountering unknown field label */
                    if (strcmp(acLabel, "FREQUENCY_END") != 0)
                    {
                        (void) fprintf(stderr,
                                       "WARNING: Unknown field label %s "
                                       "encountered!\n", acLabel);
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

    /* close the file, it may be opened later for reading data */
    (void) fclose(g_pFSpec);
    /* set the stream pointer to NULL so that YAPP_CleanUp does not try to
       close it */
    g_pFSpec = NULL;

    if (pstYUM->fChanBW < 0.0)
    {
        /* make the channel bandwidth positive */
        pstYUM->fChanBW = fabs(pstYUM->fChanBW);
        pstYUM->fFMax = fFCh1;
        pstYUM->fFMin = pstYUM->fFMax - (pstYUM->iNumChans * pstYUM->fChanBW);
    }
    else
    {
        pstYUM->fFMin = fFCh1;
        pstYUM->fFMax = pstYUM->fFMin + (pstYUM->iNumChans * pstYUM->fChanBW);
    }
    /* calculate bandwidth and centre frequency */
    pstYUM->fBW = pstYUM->fFMax - pstYUM->fFMin;
    pstYUM->fFCentre = pstYUM->fFMin + (pstYUM->fBW / 2);


    if (YAPP_TRUE == pstYUM->iFlagSplicedData)
    {
        /* in spliced data files, the first frequency is always the
           highest - since we have inverted the array, it is the last
           frequency */
        pstYUM->fFMax = pstYUM->pfFreq[pstYUM->iNumChans-1];
        /* get the lowest frequency */
        pstYUM->fFMin = pstYUM->pfFreq[0];
        /* calculate the channel bandwidth */
        pstYUM->fChanBW = pstYUM->pfFreq[1] - pstYUM->pfFreq[0];

        /* TODO: Number-of-bands calculation not accurate */
        for (i = 1; i < pstYUM->iNumChans; ++i)
        {
            /*if (fabsf(pfFreq[i] - pfFreq[i-1]) > fChanBW)*/
            /* kludge: */
            if (fabsf(pstYUM->pfFreq[i] - pstYUM->pfFreq[i-1]) > (2 * pstYUM->fChanBW))
            {
                ++pstYUM->iNumBands;
                if (YAPP_MAX_NUM_BANDS == pstYUM->iNumBands)
                {
                    (void) printf("WARNING: "
                                  "Maximum number of bands reached!\n");
                    break;
                }
            }
        }
    }
    else
    {
        pstYUM->iNumBands = 1;
    }

    /* TODO: find out the discontinuities and print them as well, also
             number of spliced bands */

    pstYUM->fSampSize = ((float) pstYUM->iNumBits) / YAPP_BYTE2BIT_FACTOR;

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
    pstYUM->lDataSizeTotal = (long) stFileStats.st_size - pstYUM->iHeaderLen;
    pstYUM->iTimeSamps = (int) (pstYUM->lDataSizeTotal / (pstYUM->iNumChans * pstYUM->fSampSize));

    /* set number of good channels to number of channels - no support for
       SIGPROC ignore files yet */
    pstYUM->iNumGoodChans = pstYUM->iNumChans;

    return YAPP_RET_SUCCESS;
}

int YAPP_ReadSIGPROCHeaderFile(char *pcFileSpec, YUM_t *pstYUM)
{
    FILE* pFHdr = NULL;
    char acFileHeader[LEN_GENSTRING] = {0};
    char acTemp[LEN_GENSTRING] = {0};
    size_t iLen = 0;
    double dFChan = 0.0;
    struct stat stFileStats = {0};
    int iRet = YAPP_RET_SUCCESS;
    int i = 0;
    int iDataTypeID = 0;
    double dFChan1 = 0.0;
    float fFCh1 = 0.0;
    double dChanBW = 0.0;
    double dTStart = 0.0;
    int iObsID = 0;
    char *pcExt = NULL;
    char *pcVal = NULL;
    char* pcLine = NULL;

    /* build the header file name */
    (void) strncpy(acFileHeader, pcFileSpec, LEN_GENSTRING);
    pcExt = strrchr(acFileHeader, '.');
    if (NULL == pcExt)
    {
        (void) fprintf(stderr,
                       "ERROR: Could not locate extension in file name %s!\n",
                       acFileHeader);
        return YAPP_RET_ERROR;
    }
    (void) strcpy(pcExt, ".fhd");

    /* open the header file for reading */
    pFHdr = fopen(acFileHeader, "r");
    if (NULL == pFHdr)
    {
        (void) fprintf(stderr,
                       "ERROR: Opening file %s failed! %s.\n",
                       acFileHeader,
                       strerror(errno));
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }

    /* read observing site */
    (void) getline(&pcLine, &iLen, pFHdr); 
    pcVal = strrchr(pcLine, ':');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return YAPP_RET_ERROR;
    }
    (void) sscanf(pcVal, ": %s", pstYUM->acSite);
    /* read field name */
    (void) getline(&pcLine, &iLen, pFHdr); 
    pcVal = strrchr(pcLine, ':');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return YAPP_RET_ERROR;
    }
    (void) sscanf(pcVal, ": %s", pstYUM->acPulsar);
    /* read sampling interval */
    (void) getline(&pcLine, &iLen, pFHdr); 
    pcVal = strrchr(pcLine, ':');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return YAPP_RET_ERROR;
    }
    (void) sscanf(pcVal, ": %lf", &pstYUM->dTSamp);
    /* read number of channels */
    (void) getline(&pcLine, &iLen, pFHdr); 
    pcVal = strrchr(pcLine, ':');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return YAPP_RET_ERROR;
    }
    (void) sscanf(pcVal, ": %d", &pstYUM->iNumChans);
    /* read and ignore number of good channels */
    (void) getline(&pcLine, &iLen, pFHdr); 
    /* read channel bandwidth */
    (void) getline(&pcLine, &iLen, pFHdr); 
    pcVal = strrchr(pcLine, ':');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return YAPP_RET_ERROR;
    }
    (void) sscanf(pcVal, ": %f", &pstYUM->fChanBW);
    /* read lowest frequency */
    (void) getline(&pcLine, &iLen, pFHdr); 
    pcVal = strrchr(pcLine, ':');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return YAPP_RET_ERROR;
    }
    (void) sscanf(pcVal, ": %f", &pstYUM->fFMin);
    /* read highest frequency */
    (void) getline(&pcLine, &iLen, pFHdr); 
    pcVal = strrchr(pcLine, ':');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return YAPP_RET_ERROR;
    }
    (void) sscanf(pcVal, ": %f", &pstYUM->fFMax);
    /* read number of bands */
    (void) getline(&pcLine, &iLen, pFHdr); 
    pcVal = strrchr(pcLine, ':');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return YAPP_RET_ERROR;
    }
    (void) sscanf(pcVal, ": %d", &pstYUM->iNumBands);
    /* read number of bad time sections */
    (void) getline(&pcLine, &iLen, pFHdr); 
    pcVal = strrchr(pcLine, ':');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return YAPP_RET_ERROR;
    }
    (void) sscanf(pcVal, ": %d", &pstYUM->iNumBadTimes);
    /* read number of bits */
    (void) getline(&pcLine, &iLen, pFHdr); 
    pcVal = strrchr(pcLine, ':');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return YAPP_RET_ERROR;
    }
    (void) sscanf(pcVal, ": %d", &pstYUM->iNumBits);
    /* read number of IFs */
    (void) getline(&pcLine, &iLen, pFHdr); 
    pcVal = strrchr(pcLine, ':');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return YAPP_RET_ERROR;
    }
    (void) sscanf(pcVal, ": %d", &pstYUM->iNumIFs);


    /* calculate bandwidth and centre frequency */
    pstYUM->fBW = pstYUM->fFMax - pstYUM->fFMin;
    pstYUM->fFCentre = pstYUM->fFMin + (pstYUM->fBW / 2);

    /* TODO: find out the discontinuities and print them as well, also
             number of spliced bands */

    pstYUM->fSampSize = ((float) pstYUM->iNumBits) / YAPP_BYTE2BIT_FACTOR;

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
    pstYUM->lDataSizeTotal = (long) stFileStats.st_size - pstYUM->iHeaderLen;
    pstYUM->iTimeSamps = (int) (pstYUM->lDataSizeTotal / (pstYUM->iNumChans * pstYUM->fSampSize));

    /* set number of good channels to number of channels - no support for
       SIGPROC ignore files yet */
    pstYUM->iNumGoodChans = pstYUM->iNumChans;

    free(pcLine);

    return YAPP_RET_SUCCESS;
}

/*
 * Read one block of data from disk
 */
int YAPP_ReadData(float *pfBuf,
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
                       g_pFSpec);
    if (ferror(g_pFSpec))
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
            if (pcBuf[i] >= 0)
            {
                pcBuf[i] = 128 - pcBuf[i];
            }
            else
            {
                pcBuf[i] = 128 + pcBuf[i];
            }
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

    /* free all memory on heap */
    for (i = 0; i < g_iMemTableSize; ++i)
    {
        if (g_apvMemTable[i] != NULL)
        {
            free(g_apvMemTable[i]);
            g_apvMemTable[i] = NULL;
        }
    }

    #if 0
    /* BUG: yapp_viewdata crashes here on ^C */
    /* close PGPLOT device, if open */
    if (g_iPGDev > 0)
    {
        cpgslct(g_iPGDev);
        cpgclos();
    }
    #endif

    /* close data file, if open */
    if (g_pFSpec != NULL)
    {
        (void) fclose(g_pFSpec);
        g_pFSpec = NULL;
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
    printf("\n");

    /* clean up */
    YAPP_CleanUp();

    /* exit */
    exit(YAPP_RET_SUCCESS);

    /* never reached */
    return;
}


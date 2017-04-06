/**
 * @file yapp_common.c
 * Commom YAPP functionalities.
 *
 * @author Jayanth Chennamangalam
 * @date 2009.07.06
 */

#include "yapp.h"
#include "yapp_sigproc.h"
#include "yapp_psrfits.h"
#include "yapp_presto.h"
#ifdef HDF5
#include "yapp_hdf5.h"
#endif

#include <fitsio.h>
#ifdef HDF5
#include <hdf5.h>
#endif

const char g_aacSP_ObsNames[YAPP_SP_NUMOBS][LEN_GENSTRING] = {
    YAPP_SP_OBS_FAKE,
    YAPP_SP_OBS_AO,
    YAPP_SP_OBS_ORT,
    YAPP_SP_OBS_NANCAY,
    YAPP_SP_OBS_PARKES,
    YAPP_SP_OBS_JB,
    YAPP_SP_OBS_GBT,
    YAPP_SP_OBS_GMRT,
    YAPP_SP_OBS_EFFELSBERG,
};

extern const double g_aadErfLookup[YAPP_ERF_ENTRIES][2];

/* global memory management */
void* g_apvMemTable[YAPP_MAX_MEMTABLE] = {0};
int g_iMemTableSize = 0;

/* PGPLOT device ID */
int g_iPGDev = 0;

/* data file */
FILE *g_pFData = NULL;

/* data buffer */
static float *g_pfBuf = NULL;

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
    if (0 == strcmp(pcExt, EXT_PSRFITS))
    {
        iFormat =  YAPP_FORMAT_PSRFITS;
    }
    else if (0 == strcmp(pcExt, EXT_DYNSPEC))
    {
        iFormat = YAPP_FORMAT_SPEC;
    }
    else if (0 == strcmp(pcExt, EXT_FIL))
    {
        iFormat = YAPP_FORMAT_FIL;
    }
    else if (0 == strcmp(pcExt, EXT_DEDISPSPEC))
    {
        iFormat = YAPP_FORMAT_DTS_DDS;
    }
    else if (0 == strcmp(pcExt, EXT_TIM))
    {
        iFormat = YAPP_FORMAT_DTS_TIM;
    }
    else if (0 == strcmp(pcExt, EXT_DAT))
    {
        iFormat = YAPP_FORMAT_DTS_DAT;
    }
    else if (0 == strcmp(pcExt, EXT_YM))
    {
        iFormat = YAPP_FORMAT_YM;
    }
#ifdef HDF5
    else if (0 == strcmp(pcExt, EXT_HDF5))
    {
        iFormat = YAPP_FORMAT_HDF5;
    }
#endif
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
char* YAPP_GetFilenameFromPath(char *pcPath)
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

    /* find the extension */
    char *pcExt = strrchr(pcPos, '.');

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
    dNumSigmasRef = g_aadErfLookup[0][1];
    dErfDiff = fabs(dErfRef - dErf);
    dErfDiffMin = dErfDiff;
    dNumSigmas = dNumSigmasRef;

    /* get the threshold in terms of standard deviation for the matching error
       function from the error function lookup table */
    for (i = 0; i < YAPP_ERF_ENTRIES; ++i)
    {
        dErfRef = g_aadErfLookup[i][0];
        dNumSigmasRef = g_aadErfLookup[i][1];

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
int YAPP_SP_GetObsNameFromID(int iObsID, char *pcObs)
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
 * Retrieves the observatory ID from its name
 */
int YAPP_SP_GetObsIDFromName(char *pcObs)
{
    int i = 0;

    for (i = 0; i < YAPP_SP_NUMOBS; ++i)
    {
        if (strcasestr(pcObs, g_aacSP_ObsNames[i]) != NULL)
        {
            return i;
        }
    }

    return YAPP_RET_ERROR;
}


int YAPP_GetExtFromFormat(int iFormat, char *pcExt)
{
    switch (iFormat)
    {
        case YAPP_FORMAT_SPEC:
            (void) strcpy(pcExt, YAPP_FORMATSTR_SPEC);
            break;

        case YAPP_FORMAT_PSRFITS:
            (void) strcpy(pcExt, YAPP_FORMATSTR_PSRFITS);
            break;

        case YAPP_FORMAT_FIL:
            (void) strcpy(pcExt, YAPP_FORMATSTR_FIL);
            break;

        case YAPP_FORMAT_DTS_DDS:
            (void) strcpy(pcExt, YAPP_FORMATSTR_DTS_DDS);
            break;

        case YAPP_FORMAT_DTS_TIM:
            (void) strcpy(pcExt, YAPP_FORMATSTR_DTS_TIM);
            break;

        case YAPP_FORMAT_DTS_DAT:
            (void) strcpy(pcExt, YAPP_FORMATSTR_DTS_DAT);
            break;

        case YAPP_FORMAT_YM:
            (void) strcpy(pcExt, YAPP_FORMATSTR_YM);
            break;

#ifdef HDF5
        case YAPP_FORMAT_HDF5:
            (void) strcpy(pcExt, YAPP_FORMATSTR_HDF5);
            break;
#endif

        default:
            (void) fprintf(stderr,
                           "ERROR: Unknown format %d!\n",
                           iFormat);
            return YAPP_RET_ERROR;
    }

    return YAPP_RET_SUCCESS;
}


int YAPP_GetDescFromFormat(int iFormat, char *pcDesc)
{
    switch (iFormat)
    {
        case YAPP_FORMAT_SPEC:
            (void) strcpy(pcDesc, YAPP_FORMATDESC_SPEC);
            break;

        case YAPP_FORMAT_PSRFITS:
            (void) strcpy(pcDesc, YAPP_FORMATDESC_PSRFITS);
            break;

        case YAPP_FORMAT_FIL:
            (void) strcpy(pcDesc, YAPP_FORMATDESC_FIL);
            break;

        case YAPP_FORMAT_DTS_DDS:
            (void) strcpy(pcDesc, YAPP_FORMATDESC_DTS_DDS);
            break;

        case YAPP_FORMAT_DTS_TIM:
            (void) strcpy(pcDesc, YAPP_FORMATDESC_DTS_TIM);
            break;

        case YAPP_FORMAT_DTS_DAT:
            (void) strcpy(pcDesc, YAPP_FORMATDESC_DTS_DAT);
            break;

        case YAPP_FORMAT_YM:
            (void) strcpy(pcDesc, YAPP_FORMATDESC_YM);
            break;

#ifdef HDF5
        case YAPP_FORMAT_HDF5:
            (void) strcpy(pcDesc, YAPP_FORMATDESC_HDF5);
            break;
#endif

        default:
            (void) fprintf(stderr,
                           "ERROR: Unknown format %d!\n",
                           iFormat);
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
        case YAPP_FORMAT_PSRFITS:
            iRet = YAPP_ReadPSRFITSHeader(pcFileSpec, pstYUM);
            if (iRet != YAPP_RET_SUCCESS)
            {
                (void) fprintf(stderr,
                               "ERROR: Reading PSRFITS header failed!\n");
                return YAPP_RET_ERROR;
            }
            break;

        case YAPP_FORMAT_SPEC:
            iRet = YAPP_ReadDASCfg(pcFileSpec, pstYUM);
            if (iRet != YAPP_RET_SUCCESS)
            {
                (void) fprintf(stderr,
                               "ERROR: "
                               "Reading configuration information failed!\n");
                return YAPP_RET_ERROR;
            }
            break;

        case YAPP_FORMAT_FIL:
        case YAPP_FORMAT_DTS_TIM:
            iRet = YAPP_ReadSIGPROCHeader(pcFileSpec, iFormat, pstYUM);
            if (iRet != YAPP_RET_SUCCESS)
            {
                (void) fprintf(stderr,
                               "ERROR: "
                               "Reading SIGPROC header failed!\n");
                return YAPP_RET_ERROR;
            }
            break;

        case YAPP_FORMAT_DTS_DAT:
            iRet = YAPP_ReadPRESTOHeaderFile(pcFileSpec, pstYUM);
            if (iRet != YAPP_RET_SUCCESS)
            {
                (void) fprintf(stderr,
                               "ERROR: Reading PRESTO metadata failed!\n");
                return YAPP_RET_ERROR;
            }
            break;

#ifdef HDF5
        case YAPP_FORMAT_HDF5:
            iRet = YAPP_ReadHDF5Metadata(pcFileSpec, iFormat, pstYUM);
            if (iRet != YAPP_RET_SUCCESS)
            {
                (void) fprintf(stderr,
                               "ERROR: "
                               "Reading HDF5 metadata failed!\n");
                return YAPP_RET_ERROR;
            }
            break;
#endif

        default:
            (void) fprintf(stderr,
                           "ERROR: Unknown file format %d!\n",
                           iFormat);
            return YAPP_RET_ERROR;
    }

    /* kludge */
    if (0 == pstYUM->iNumIFs)
    {
        (void) fprintf(stderr,
                       "WARNING: Number of IFs is 0! Setting it to 1.\n");
        pstYUM->iNumIFs = 1;
    }
    if (0 == pstYUM->iNumPol)
    {
        (void) fprintf(stderr,
                       "WARNING: Number of polarizations is 0! "
                       "Setting it to number of IFs = %d.\n",
                       pstYUM->iNumIFs);
        pstYUM->iNumPol = pstYUM->iNumIFs;
    }

    return YAPP_RET_SUCCESS;
}


int YAPP_ReadPSRFITSHeader(char *pcFileSpec, YUM_t *pstYUM)
{
    fitsfile *pstFileData = NULL;
    int iStatus = 0;
    char acErrMsg[FLEN_ERRMSG] = {0};
    int iTemp = 0;
    double dTemp = 0.0;
    char acTemp[FLEN_VALUE] = {0};
    float *pfFreq = NULL;
    int iColNum = 0;

    /*  open PSRFITS file */
    (void) fits_open_file(&pstFileData, pcFileSpec, READONLY, &iStatus);
    if  (iStatus != 0)
    {
        fits_get_errstatus(iStatus, acErrMsg);
        (void) fprintf(stderr, "ERROR: Opening file failed! %s\n", acErrMsg);
        return YAPP_RET_ERROR;
    }

    /* read Primary HDU header */
    (void) fits_read_key(pstFileData,
                         TSTRING,
                         YAPP_PF_LABEL_OBSID,
                         pstYUM->acSite,
                         NULL,
                         &iStatus);
    (void) fits_read_key(pstFileData,
                         TSTRING,
                         YAPP_PF_LABEL_SRCNAME,
                         pstYUM->acPulsar,
                         NULL,
                         &iStatus);
    (void) fits_read_key(pstFileData,
                         TINT,
                         YAPP_PF_LABEL_NUMCHANS,
                         &pstYUM->iNumChans,
                         NULL,
                         &iStatus);
    pstYUM->iNumGoodChans = pstYUM->iNumChans;
    pstYUM->iNumBands = 1;
    (void) fits_read_key(pstFileData,
                         TINT,
                         YAPP_PF_LABEL_TSTART,
                         &iTemp,
                         NULL,
                         &iStatus);
    pstYUM->dTStart = (double) iTemp;
    (void) fits_read_key(pstFileData,
                         TINT,
                         YAPP_PF_LABEL_TSTARTSEC,
                         &iTemp,
                         NULL,
                         &iStatus);
    pstYUM->dTStart += (((double) iTemp) / 86400);
    (void) fits_read_key(pstFileData,
                         TDOUBLE,
                         YAPP_PF_LABEL_TSTARTOFF,
                         &dTemp,
                         NULL,
                         &iStatus);
    pstYUM->dTStart += dTemp;
    (void) fits_read_key(pstFileData,
                         TSTRING,
                         YAPP_PF_LABEL_SRCRA,
                         acTemp,
                         NULL,
                         &iStatus);
    pstYUM->dSourceRA = YAPP_RAString2Double(acTemp);
    (void) fits_read_key(pstFileData,
                         TSTRING,
                         YAPP_PF_LABEL_SRCDEC,
                         acTemp,
                         NULL,
                         &iStatus);
    pstYUM->dSourceDec = YAPP_DecString2Double(acTemp);

    /* read SUBINT HDU header */
    (void) fits_movnam_hdu(pstFileData,
                           BINARY_TBL,
                           YAPP_PF_HDUNAME_SUBINT,
                           0,
                           &iStatus);
    if  (iStatus != 0)
    {
        fits_get_errstatus(iStatus, acErrMsg);
        (void) fprintf(stderr,
                       "ERROR: Moving to HDU %s failed! %s\n",
                       YAPP_PF_HDUNAME_SUBINT,
                       acErrMsg);
        (void) fits_close_file(pstFileData, &iStatus);
        return YAPP_RET_ERROR;
    }
    (void) fits_read_key(pstFileData,
                         TINT,
                         YAPP_PF_LABEL_NUMBITS,
                         &pstYUM->iNumBits,
                         NULL,
                         &iStatus);
    (void) fits_read_key(pstFileData,
                         TDOUBLE,
                         YAPP_PF_LABEL_TSAMP,
                         &pstYUM->dTSamp,
                         NULL,
                         &iStatus);
    pstYUM->dTSamp *= 1e3;      /* convert from s to ms */
    (void) fits_read_key(pstFileData,
                         TINT,
                         YAPP_PF_LABEL_NPOL,
                         &pstYUM->iNumPol,
                         NULL,
                         &iStatus);
    (void) fits_read_key(pstFileData,
                         TINT,
                         YAPP_PF_LABEL_NSUBINT,
                         &pstYUM->iTimeSamps,
                         NULL,
                         &iStatus);
    (void) fits_read_key(pstFileData,
                         TINT,
                         YAPP_PF_LABEL_NSBLK,
                         &iTemp,
                         NULL,
                         &iStatus);
    pstYUM->iTimeSamps *= iTemp;

    /* get the frequencies of the first and last bins */
    (void) fits_get_colnum(pstFileData,
                           CASESEN,
                           YAPP_PF_LABEL_DATFREQ,
                           &iColNum,
                           &iStatus);
    if (iStatus != 0)
    {
        fits_get_errstatus(iStatus, acErrMsg);
        (void) fprintf(stderr,
                       "ERROR: Getting column number failed! %s\n",
                       acErrMsg);
        (void) fits_close_file(pstFileData, &iStatus);
        return YAPP_RET_ERROR;
    }
    pfFreq = (float *) YAPP_Malloc(pstYUM->iNumChans, sizeof(float), YAPP_FALSE);
    if (NULL == pfFreq)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation failed! %s!\n",
                       strerror(errno));
        (void) fits_close_file(pstFileData, &iStatus);
        return YAPP_RET_ERROR;
    }
    (void) fits_read_col(pstFileData,
                         TFLOAT,
                         iColNum,
                         1,
                         1,
                         pstYUM->iNumChans,
                         NULL,
                         pfFreq,
                         NULL,
                         &iStatus);

    /* TODO: check for PDEV */
    /* NOTE: PDEV stores an approximate value of channel bandwidth in the
             header, so instead of reading it from the header, compute it */
    pstYUM->fChanBW = pfFreq[1] - pfFreq[0];

    if (pstYUM->fChanBW > 0.0)
    {
        pstYUM->fFMin = pfFreq[0];
        pstYUM->fFMax = pfFreq[pstYUM->iNumChans-1];
    }
    else    /* band is inverted */
    {
        pstYUM->cIsBandFlipped = YAPP_TRUE;
        pstYUM->fChanBW = fabs(pstYUM->fChanBW);
        pstYUM->fFMin = pfFreq[pstYUM->iNumChans-1];
        pstYUM->fFMax = pfFreq[0];
    }

    (void) fits_close_file(pstFileData, &iStatus);

    /* TODO: do only for PDEV */
    /* NOTE: PDEV seems to store approximate values of centre frequency and
             bandwidth in the header, so instead of reading it from the header,
             compute it from the centre frequencies of individual channels */
    pstYUM->fBW = pstYUM->fFMax - pstYUM->fFMin + pstYUM->fChanBW;
    if (0 == (pstYUM->iNumChans % 2))   /* even number of channels */
    {
        pstYUM->fFCentre = (pstYUM->fFMin - (pstYUM->fChanBW / 2))
                           + ((pstYUM->iNumChans / 2) * pstYUM->fChanBW);
    }
    else                                /* odd number of channels */
    {
        pstYUM->fFCentre = pstYUM->fFMin
                           + (((float) pstYUM->iNumChans / 2) * pstYUM->fChanBW);
    }

    /* calculate the size of data */
    pstYUM->lDataSizeTotal = (long) pstYUM->iNumPol
                             * pstYUM->iNumChans
                             * pstYUM->iTimeSamps
                             * ((float) pstYUM->iNumBits / YAPP_BYTE2BIT_FACTOR);

    return YAPP_RET_SUCCESS;
}


double YAPP_RAString2Double(char *pcRA)
{
    double dRA = 0.0;
    char *pcTemp = NULL;

    /* extract hours */
    pcTemp = strtok(pcRA, ":");
    dRA = atof(pcTemp) * YAPP_DEGPERHOUR;
    /* extract minutes */
    pcTemp = strtok(NULL, ":");
    dRA += ((atof(pcTemp) / 60) * YAPP_DEGPERHOUR);
    /* extract seconds */
    pcTemp = strtok(NULL, ":");
    dRA += ((atof(pcTemp) / 3600) * YAPP_DEGPERHOUR);

    return dRA;
}


void YAPP_RADouble2String(double dRA, char *pcRA)
{
    int iHour = 0;
    int iMin = 0;
    double dSec = 0.0;

    /* extract the integer part */
    iHour = (int) (dRA / YAPP_DEGPERHOUR);
    /* extract the fractional part */
    iMin = (int) (((dRA / YAPP_DEGPERHOUR) - iHour) * 60);
    dSec = ((((dRA / YAPP_DEGPERHOUR) - iHour) * 60) - iMin) * 60;

    (void) sprintf(pcRA, "%.2d:%.2d:%.7g", iHour, iMin, dSec);

    return;
}


double YAPP_DecString2Double(char *pcDec)
{
    double dDec = 0.0;
    char *pcTemp = NULL;

    /* extract degrees */
    pcTemp = strtok(pcDec, ":");
    dDec = atof(pcTemp);
    /* extract minutes */
    pcTemp = strtok(NULL, ":");
    if (dDec >= 0.0)
    {
        dDec += (atof(pcTemp) / 60);
    }
    else
    {
        dDec -= (atof(pcTemp) / 60);
    }
    /* extract seconds */
    pcTemp = strtok(NULL, ":");
    if (dDec >= 0.0)
    {
        dDec += (atof(pcTemp) / 3600);
    }
    else
    {
        dDec -= (atof(pcTemp) / 3600);
    }

    return dDec;
}


void YAPP_DecDouble2String(double dDec, char *pcDec)
{
    int iDeg = 0;
    int iMin = 0;
    double dSec = 0.0;
    char acNeg[LEN_GENSTRING] = {0};

    if (dDec < 0.0)
    {
        acNeg[0] = '-';
        dDec = fabs(dDec);
    }
    /* extract the integer part */
    iDeg = (int) dDec;
    /* extract the fractional part */
    iMin = (int) ((dDec - iDeg) * 60);
    dSec = (((dDec - iDeg) * 60) - iMin) * 60;

    (void) sprintf(pcDec, "%s%.2d:%.2d:%.7g", acNeg, iDeg, iMin, dSec);

    return;
}


int YAPP_ReadDASCfg(char *pcFileSpec, YUM_t *pstYUM)
{
    FILE *pFCfg = NULL;
    char acFileCfg[LEN_GENSTRING] = {0};
    int iChanGoodness = (int) YAPP_TRUE;
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
    pstYUM->iTimeSamps = (int) (pstYUM->lDataSizeTotal
                                / (pstYUM->iNumChans * pstYUM->fSampSize));

    pstYUM->iNumBands = 1;

    return YAPP_RET_SUCCESS;
}

#ifdef HDF5
int YAPP_ReadHDF5Metadata(char *pcFileSpec, int iFormat, YUM_t *pstYUM)
{
    hid_t hFile = 0;
    hid_t hGroup = 0;
    hid_t hDataset = 0;
    herr_t hStatus = 0;
    int i = 0;

    assert(YAPP_FORMAT_HDF5 == iFormat);

    /* open .h5 file */
    hFile = H5Fopen(pcFileSpec, H5F_ACC_RDONLY, H5P_DEFAULT);
    if (hFile < 0)
    {
        fprintf(stderr,
                "ERROR: Opening file %s failed! %s.\n",
                pcFileSpec,
                strerror(errno));
        return YAPP_RET_ERROR;
    }

    /* open the group */
    hGroup = H5Gopen(hFile, YAPP_HDF5_DYNSPEC_GROUP, H5P_DEFAULT);
    if (hGroup < 0)
    {
        fprintf(stderr,
                "ERROR: Opening group %s failed!\n",
                YAPP_HDF5_DYNSPEC_GROUP);
        return YAPP_RET_ERROR;
    }

    /* open the dataset */
    hDataset = H5Dopen(hGroup,
                       YAPP_HDF5_DYNSPEC_GROUP YAPP_HDF5_DYNSPEC_DATASET,
                       H5P_DEFAULT);
    if (hDataset < 0)
    {
        fprintf(stderr,
                "ERROR: Opening dataset %s failed!\n",
                YAPP_HDF5_DYNSPEC_GROUP YAPP_HDF5_DYNSPEC_DATASET);
        return YAPP_RET_ERROR;
    }

    /* iterate through the attributes of this dataset */
    hStatus = H5Aiterate(hDataset,
                         H5_INDEX_NAME,
                         H5_ITER_NATIVE,
                         NULL,
                         YAPP_ReadHDF5Attribute,
                         pstYUM);

    (void) H5Dclose(hDataset);
    (void) H5Gclose(hGroup);
    (void) H5Fclose(hFile);

    /* fill in additional info in the metadata structure */

    pstYUM->iNumBands = 1;

    pstYUM->fSampSize = ((float) pstYUM->iNumBits) / YAPP_BYTE2BIT_FACTOR;

    pstYUM->lDataSizeTotal = (long) pstYUM->iNumChans
                                    * pstYUM->iTimeSamps
                                    * pstYUM->fSampSize;

    /* call all channels good - no support for SIGPROC ignore files yet */
    pstYUM->pcIsChanGood = (char *) YAPP_Malloc(pstYUM->iNumChans,
                                                sizeof(char),
                                                YAPP_FALSE);
    if (NULL == pstYUM->pcIsChanGood)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation for channel goodness "
                       "flag failed! %s!\n",
                       strerror(errno));
        return YAPP_RET_ERROR;
    }

    /* read the channel goodness flags */
    for (i = 0; i < pstYUM->iNumChans; ++i)
    {
        pstYUM->pcIsChanGood[i] = YAPP_TRUE;
    }

    /* set number of good channels to number of channels - no support for
       masking yet */
    pstYUM->iNumGoodChans = pstYUM->iNumChans;

    return YAPP_RET_SUCCESS;
    return YAPP_RET_SUCCESS;
}


herr_t YAPP_ReadHDF5Attribute(hid_t hDataset,
                              const char *pcAttrName,
                              const H5A_info_t *pstAttrInfo,
                              void *pstYUM)
{
    hid_t hAttr = 0;
    hid_t hType = 0;
    herr_t hStatus = 0;

    hAttr = H5Aopen(hDataset, pcAttrName, H5P_DEFAULT);

    if (0 == strcmp(pcAttrName, YAPP_HDF5_ATTRNAME_SITE))
    {
        hType = H5Aget_type (hAttr);
        hStatus = H5Aread(hAttr,
                          hType,
                          &(((YUM_t *) pstYUM)->acSite));
    }
    else if (0 == strcmp(pcAttrName, YAPP_HDF5_ATTRNAME_SRCNAME))
    {
        hType = H5Aget_type (hAttr);
        hStatus = H5Aread(hAttr,
                          hType,
                          &(((YUM_t *) pstYUM)->acPulsar));
    }
    else if (0 == strcmp(pcAttrName, YAPP_HDF5_ATTRNAME_TSAMP))
    {
        hStatus = H5Aread(hAttr,
                          H5T_IEEE_F64LE,
                          &(((YUM_t *) pstYUM)->dTSamp));
    }
    else if (0 == strcmp(pcAttrName, YAPP_HDF5_ATTRNAME_FCEN))
    {
        hStatus = H5Aread(hAttr,
                          H5T_IEEE_F32LE,
                          &(((YUM_t *) pstYUM)->fFCentre));
    }
    else if (0 == strcmp(pcAttrName, YAPP_HDF5_ATTRNAME_BW))
    {
        hStatus = H5Aread(hAttr,
                          H5T_IEEE_F32LE,
                          &(((YUM_t *) pstYUM)->fBW));
    }
    else if (0 == strcmp(pcAttrName, YAPP_HDF5_ATTRNAME_NUMCHANS))
    {
        hStatus = H5Aread(hAttr,
                          H5T_STD_I32LE,
                          &(((YUM_t *) pstYUM)->iNumChans));
    }
    else if (0 == strcmp(pcAttrName, YAPP_HDF5_ATTRNAME_CHANBW))
    {
        hStatus = H5Aread(hAttr,
                          H5T_IEEE_F32LE,
                          &(((YUM_t *) pstYUM)->fChanBW));
    }
    else if (0 == strcmp(pcAttrName, YAPP_HDF5_ATTRNAME_TIMESAMPS))
    {
        hStatus = H5Aread(hAttr,
                          H5T_STD_I32LE,
                          &(((YUM_t *) pstYUM)->iTimeSamps));
    }
    else if (0 == strcmp(pcAttrName, YAPP_HDF5_ATTRNAME_NUMBITS))
    {
        hStatus = H5Aread(hAttr,
                          H5T_STD_I32LE,
                          &(((YUM_t *) pstYUM)->iNumBits));
    }
    else if (0 == strcmp(pcAttrName, YAPP_HDF5_ATTRNAME_NUMIFS))
    {
        hStatus = H5Aread(hAttr,
                          H5T_STD_I32LE,
                          &(((YUM_t *) pstYUM)->iNumIFs));
    }
    else if (0 == strcmp(pcAttrName, YAPP_HDF5_ATTRNAME_BACKEND))
    {
        hStatus = H5Aread(hAttr,
                          H5T_STD_I32LE,
                          &(((YUM_t *) pstYUM)->iBackendID));
    }
    else if (0 == strcmp(pcAttrName, YAPP_HDF5_ATTRNAME_SRCRA))
    {
        hStatus = H5Aread(hAttr,
                          H5T_IEEE_F64LE,
                          &(((YUM_t *) pstYUM)->dSourceRA));
    }
    else if (0 == strcmp(pcAttrName, YAPP_HDF5_ATTRNAME_SRCDEC))
    {
        hStatus = H5Aread(hAttr,
                          H5T_IEEE_F64LE,
                          &(((YUM_t *) pstYUM)->dSourceDec));
    }
    else if (0 == strcmp(pcAttrName, YAPP_HDF5_ATTRNAME_FMIN))
    {
        hStatus = H5Aread(hAttr,
                          H5T_IEEE_F32LE,
                          &(((YUM_t *) pstYUM)->fFMin));
    }
    else if (0 == strcmp(pcAttrName, YAPP_HDF5_ATTRNAME_FMAX))
    {
        hStatus = H5Aread(hAttr,
                          H5T_IEEE_F32LE,
                          &(((YUM_t *) pstYUM)->fFMax));
    }
    else if (0 == strcmp(pcAttrName, YAPP_HDF5_ATTRNAME_BFLIP))
    {
        hStatus = H5Aread(hAttr,
                          H5T_STD_I8LE,
                          &(((YUM_t *) pstYUM)->cIsBandFlipped));
    }
    else if (0 == strcmp(pcAttrName, YAPP_HDF5_ATTRNAME_TSTART))
    {
        hStatus = H5Aread(hAttr,
                          H5T_IEEE_F64LE,
                          &(((YUM_t *) pstYUM)->dTStart));
    }
    else
    {
        /* print a warning about encountering unknown attribute */
        (void) fprintf(stderr,
                       "WARNING: Unknown attribute %s encountered!\n",
                       pcAttrName);
    }

    (void) H5Aclose(hAttr);

    return hStatus;
}
#endif

int YAPP_ReadSIGPROCHeader(char *pcFileSpec, int iFormat, YUM_t *pstYUM)
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
    int iObsID = 0;
    char acTemp[LEN_GENSTRING] = {0};

    assert((YAPP_FORMAT_FIL == iFormat) || (YAPP_FORMAT_DTS_TIM == iFormat));

    /* open the data file for reading */
    g_pFData = fopen(pcFileSpec, "r");
    if (NULL == g_pFData)
    {
        (void) fprintf(stderr,
                       "ERROR: Opening file %s failed! %s.\n",
                       pcFileSpec,
                       strerror(errno));
        return YAPP_RET_ERROR;
    }

    /* read the parameters from the header section of the file */
    /* start with the 'HEADER_START' label */
    /* read field label length */
    iRet = fread(&iLen, sizeof(iLen), 1, g_pFData);
    /* if this is a file with header, iLen should be strlen(HEADER_START) */
    if (iLen != strlen(YAPP_SP_LABEL_HDRSTART))
    {
        /* this must be a headerless/header-separated filterbank file, so open
           the external header file */
        iRet = YAPP_ReadSIGPROCHeaderFile(pcFileSpec, pstYUM);
        if (iRet != YAPP_RET_SUCCESS)
        {
            (void) fprintf(stderr,
                           "ERROR: Reading header file failed!\n");
            return YAPP_RET_ERROR;
        }
        return YAPP_RET_SUCCESS;
    }
    /* read field label */
    iRet = fread(acLabel, sizeof(char), iLen, g_pFData);
    acLabel[iLen] = '\0';
    if (strcmp(acLabel, YAPP_SP_LABEL_HDRSTART) != 0)
    {
        (void) fprintf(stderr,
                       "ERROR: Missing label %s!\n", YAPP_SP_LABEL_HDRSTART);
        return YAPP_RET_ERROR;
    }
    pstYUM->iHeaderLen += (sizeof(iLen) + iLen);

    /* parse the rest of the header */
    while (strcmp(acLabel, YAPP_SP_LABEL_HDREND) != 0)
    {
        /* read field label length */
        iRet = fread(&iLen, sizeof(iLen), 1, g_pFData);
        /* read field label */
        iRet = fread(acLabel, sizeof(char), iLen, g_pFData);
        acLabel[iLen] = '\0';
        pstYUM->iHeaderLen += (sizeof(iLen) + iLen);
        if (0 == strcmp(acLabel, YAPP_SP_LABEL_SRCNAME))
        {
            iRet = fread(&iLen, sizeof(iLen), 1, g_pFData);
            iRet = fread(pstYUM->acPulsar, sizeof(char), iLen, g_pFData);
            pstYUM->acPulsar[iLen] = '\0';
            pstYUM->iHeaderLen += (sizeof(iLen) + iLen);
        }
        else if (0 == strcmp(acLabel, YAPP_SP_LABEL_DATATYPE))
        {
            iRet = fread(&iDataTypeID,
                         sizeof(iDataTypeID),
                         1,
                         g_pFData);
            pstYUM->iHeaderLen += sizeof(iDataTypeID);
        }
        else if (0 == strcmp(acLabel, YAPP_SP_LABEL_NUMCHANS))
        {
            /* read number of channels */
            iRet = fread(&pstYUM->iNumChans,
                         sizeof(pstYUM->iNumChans),
                         1,
                         g_pFData);
            pstYUM->iHeaderLen += sizeof(pstYUM->iNumChans);
        }
        else if (0 == strcmp(acLabel, YAPP_SP_LABEL_FCHAN1))
        {
            /* read frequency of first channel */
            iRet = fread(&dFChan1,
                         sizeof(dFChan1),
                         1,
                         g_pFData);
            fFCh1 = (float) dFChan1;
            pstYUM->iHeaderLen += sizeof(dFChan1);
        }
        else if (0 == strcmp(acLabel, YAPP_SP_LABEL_CHANBW))
        {
            /* read channel bandwidth (labelled frequency offset) */
            iRet = fread(&dChanBW,
                         sizeof(dChanBW),
                         1,
                         g_pFData);
            pstYUM->fChanBW = (float) dChanBW;
            pstYUM->iHeaderLen += sizeof(dChanBW);
        }
        else if (0 == strcmp(acLabel, YAPP_SP_LABEL_NUMBEAMS))
        {
            /* read number of beams */
            iRet = fread(&pstYUM->iNumBeams,
                         sizeof(pstYUM->iNumBeams),
                         1,
                         g_pFData);
            pstYUM->iHeaderLen += sizeof(pstYUM->iNumBeams);
        }
        else if (0 == strcmp(acLabel, YAPP_SP_LABEL_BEAMID))
        {
            /* read beam identifier */
            iRet = fread(&pstYUM->iBeamID,
                         sizeof(pstYUM->iBeamID),
                         1,
                         g_pFData);
            pstYUM->iHeaderLen += sizeof(pstYUM->iBeamID);
        }
        else if (0 == strcmp(acLabel, YAPP_SP_LABEL_NUMBITS))
        {
            /* read number of bits per sample */
            iRet = fread(&pstYUM->iNumBits,
                         sizeof(pstYUM->iNumBits),
                         1,
                         g_pFData);
            pstYUM->iHeaderLen += sizeof(pstYUM->iNumBits);
        }
        else if (0 == strcmp(acLabel, YAPP_SP_LABEL_NUMIFS))
        {
            /* read number of IFs */
            iRet = fread(&pstYUM->iNumIFs,
                         sizeof(pstYUM->iNumIFs),
                         1,
                         g_pFData);
            pstYUM->iHeaderLen += sizeof(pstYUM->iNumIFs);
        }
        else if (0 == strcmp(acLabel, YAPP_SP_LABEL_TSAMP))
        {
            /* read sampling time in seconds */
            iRet = fread(&pstYUM->dTSamp,
                         sizeof(pstYUM->dTSamp),
                         1,
                         g_pFData);
            dTSampInSec = pstYUM->dTSamp;
            pstYUM->dTSamp = dTSampInSec * 1e3;     /* in ms */
            pstYUM->iHeaderLen += sizeof(pstYUM->dTSamp);
        }
        else if (0 == strcmp(acLabel, YAPP_SP_LABEL_TSTART))
        {
            /* read timestamp of first sample (MJD) */
            iRet = fread(&pstYUM->dTStart,
                         sizeof(pstYUM->dTStart),
                         1,
                         g_pFData);
            pstYUM->iHeaderLen += sizeof(pstYUM->dTStart);
        }
        else if (0 == strcmp(acLabel, YAPP_SP_LABEL_OBSID))
        {
            /* read telescope ID */
            iRet = fread(&iObsID,
                         sizeof(iObsID),
                         1,
                         g_pFData);
            (void) YAPP_SP_GetObsNameFromID(iObsID, pstYUM->acSite);
            pstYUM->iHeaderLen += sizeof(iObsID);
        }
        else if (0 == strcmp(acLabel, YAPP_SP_LABEL_BEID))
        {
            /* read backend ID */
            iRet = fread(&pstYUM->iBackendID,
                         sizeof(pstYUM->iBackendID),
                         1,
                         g_pFData);
            pstYUM->iHeaderLen += sizeof(pstYUM->iBackendID);
        }
        else if (0 == strcmp(acLabel, YAPP_SP_LABEL_SRCRA))
        {
            /* read source RA (J2000) */
            iRet = fread(&pstYUM->dSourceRA,
                         sizeof(pstYUM->dSourceRA),
                         1,
                         g_pFData);
            /* SIGPROC scales the RA and declination by 10000, so correct for
               it */
            pstYUM->dSourceRA /= YAPP_SP_RADEC_SCALE;
            /* SIGPROC stores the RA in hours, so convert it to degrees */
            pstYUM->dSourceRA *= YAPP_DEGPERHOUR;
            pstYUM->iHeaderLen += sizeof(pstYUM->dSourceRA);
        }
        else if (0 == strcmp(acLabel, YAPP_SP_LABEL_SRCDEC))
        {
            /* read source declination (J2000) */
            iRet = fread(&pstYUM->dSourceDec,
                         sizeof(pstYUM->dSourceDec),
                         1,
                         g_pFData);
            /* SIGPROC scales the RA and declination by 10000, so correct for
               it */
            pstYUM->dSourceDec /= YAPP_SP_RADEC_SCALE;
            pstYUM->iHeaderLen += sizeof(pstYUM->dSourceDec);
        }
        else if (0 == strcmp(acLabel, YAPP_SP_LABEL_AZSTART))
        {
            /* read azimuth start */
            iRet = fread(&pstYUM->dAzStart,
                         sizeof(pstYUM->dAzStart),
                         1,
                         g_pFData);
            pstYUM->iHeaderLen += sizeof(pstYUM->dAzStart);
        }
        else if (0 == strcmp(acLabel, YAPP_SP_LABEL_ZASTART))
        {
            /* read ZA start */
            iRet = fread(&pstYUM->dZAStart,
                         sizeof(pstYUM->dZAStart),
                         1,
                         g_pFData);
            pstYUM->iHeaderLen += sizeof(pstYUM->dZAStart);
        }
        /* DTS-specific field */
        else if (0 == strcmp(acLabel, YAPP_SP_LABEL_DM))
        {
            /* read reference DM */
            iRet = fread(&pstYUM->dDM, sizeof(pstYUM->dDM), 1, g_pFData);
            pstYUM->iHeaderLen += sizeof(pstYUM->dDM);
        }
        else if (0 == strcmp(acLabel, YAPP_SP_LABEL_FLAGBARY))
        {
            /* read barycentric flag */
            iRet = fread(&pstYUM->iFlagBary,
                         sizeof(pstYUM->iFlagBary),
                         1,
                         g_pFData);
            pstYUM->iHeaderLen += sizeof(pstYUM->iFlagBary);
        }
        else if (0 == strcmp(acLabel, YAPP_SP_LABEL_RAWFILENAME))
        {
			/* read raw data file name */
            iRet = fread(&iLen, sizeof(iLen), 1, g_pFData);
            iRet = fread(acTemp, sizeof(char), iLen, g_pFData);
            acTemp[iLen] = '\0';
            pstYUM->iHeaderLen += (sizeof(iLen) + iLen);
        }
        /* to semi-support M. Keith's version of fake (fast_fake) */
        else if (0 == strcmp(acLabel, YAPP_SP_LABEL_SIGNED))
        {
            /* read signed flag */
            char cTemp = 0;
            iRet = fread(&cTemp,
                         sizeof(cTemp),
                         1,
                         g_pFData);
        }
        else if (0 == strcmp(acLabel, YAPP_SP_LABEL_FREQSTART))
        {
            pstYUM->iFlagSplicedData = YAPP_TRUE;

            /* read field label length */
            iRet = fread(&iLen, sizeof(iLen), 1, g_pFData);
            /* read field label */
            iRet = fread(acLabel, sizeof(char), iLen, g_pFData);
            acLabel[iLen] = '\0';
            pstYUM->iHeaderLen += (sizeof(iLen) + iLen);
            if (0 == strcmp(acLabel, YAPP_SP_LABEL_NUMCHANS))
            {
                /* read number of channels */
                iRet = fread(&pstYUM->iNumChans,
                             sizeof(pstYUM->iNumChans),
                             1,
                             g_pFData);
                pstYUM->iHeaderLen += sizeof(pstYUM->iNumChans);
            }
            else
            {
                (void) fprintf(stderr,
                               "ERROR: Unexpected label %s found!",
                               acLabel);
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
                return YAPP_RET_ERROR;
            }

            /* store in the reverse order */
            i = pstYUM->iNumChans - 1;
            /* parse frequency channels for spliced data */
            while (strcmp(acLabel, YAPP_SP_LABEL_FREQEND) != 0)
            {
                /* read field label length */
                iRet = fread(&iLen, sizeof(iLen), 1, g_pFData);
                /* read field label */
                iRet = fread(acLabel, sizeof(char), iLen, g_pFData);
                acLabel[iLen] = '\0';
                pstYUM->iHeaderLen += (sizeof(iLen) + iLen);
                if (0 == strcmp(acLabel, YAPP_SP_LABEL_FREQCHAN))
                {
                    iRet = fread(&dFChan, sizeof(dFChan), 1, g_pFData);
                    pstYUM->pfFreq[i] = (float) dFChan;
                    pstYUM->iHeaderLen += sizeof(dFChan);
                }
                else
                {
                    /* print a warning about encountering unknown field label */
                    if (strcmp(acLabel, YAPP_SP_LABEL_FREQEND) != 0)
                    {
                        (void) fprintf(stderr,
                                       "WARNING: Unknown field label %s "
                                       "encountered!\n", acLabel);
                        return YAPP_RET_ERROR;
                    }
                }
                --i;
            }
        }
        else
        {
            /* print a warning about encountering unknown field label */
            if (strcmp(acLabel, YAPP_SP_LABEL_HDREND) != 0)
            {
                (void) fprintf(stderr,
                               "WARNING: Unknown field label %s "
                               "encountered!\n", acLabel);
            }
        }
    }

    /* close the file, it may be opened later for reading data */
    (void) fclose(g_pFData);
    /* set the stream pointer to NULL so that YAPP_CleanUp does not try to
       close it */
    g_pFData = NULL;

    if (pstYUM->fChanBW < 0.0)
    {
        pstYUM->cIsBandFlipped = YAPP_TRUE;
        /* make the channel bandwidth positive */
        pstYUM->fChanBW = fabs(pstYUM->fChanBW);
        pstYUM->fFMax = fFCh1;
        pstYUM->fFMin = pstYUM->fFMax
                        - ((pstYUM->iNumChans - 1) * pstYUM->fChanBW);
    }
    else
    {
        pstYUM->cIsBandFlipped = YAPP_FALSE;
        pstYUM->fFMin = fFCh1;
        pstYUM->fFMax = pstYUM->fFMin
                        + ((pstYUM->iNumChans - 1) * pstYUM->fChanBW);
    }

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
                    (void) fprintf(stderr,
                                  "WARNING: "
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

    /* calculate bandwidth and centre frequency */
    /* NOTE: max and min are the _centre_ frequencies of the bins, so the
             total bandwidth would be (max+chanbw/2)-(min-chanbw/2) */
    pstYUM->fBW = (pstYUM->fFMax - pstYUM->fFMin) + pstYUM->fChanBW;
    if (0 == (pstYUM->iNumChans % 2))   /* even number of channels */
    {
        pstYUM->fFCentre = (pstYUM->fFMin - (pstYUM->fChanBW / 2))
                           + ((pstYUM->iNumChans / 2) * pstYUM->fChanBW);
    }
    else                                /* odd number of channels */
    {
        pstYUM->fFCentre = pstYUM->fFMin
                           + (((float) pstYUM->iNumChans / 2)
                              * pstYUM->fChanBW);
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
        return YAPP_RET_ERROR;
    }
    pstYUM->lDataSizeTotal = (long) stFileStats.st_size - pstYUM->iHeaderLen;
    if (YAPP_FORMAT_FIL == iFormat)
    {
        pstYUM->iTimeSamps = (int) (pstYUM->lDataSizeTotal
                                    / (pstYUM->iNumChans * pstYUM->fSampSize));
    }
    else
    {
        pstYUM->iTimeSamps = (int) (pstYUM->lDataSizeTotal
                                    / pstYUM->fSampSize);
    }

    if (YAPP_FORMAT_FIL == iFormat)
    {
        /* call all channels good - no support for SIGPROC ignore files yet */
        pstYUM->pcIsChanGood = (char *) YAPP_Malloc(pstYUM->iNumChans,
                                                    sizeof(char),
                                                    YAPP_FALSE);
        if (NULL == pstYUM->pcIsChanGood)
        {
            (void) fprintf(stderr,
                           "ERROR: Memory allocation for channel goodness "
                           "flag failed! %s!\n",
                           strerror(errno));
            return YAPP_RET_ERROR;
        }

        /* read the channel goodness flags */
        for (i = 0; i < pstYUM->iNumChans; ++i)
        {
            pstYUM->pcIsChanGood[i] = YAPP_TRUE;
        }
    }

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
    struct stat stFileStats = {0};
    int iRet = YAPP_RET_SUCCESS;
    char *pcExt = NULL;
    char *pcVal = NULL;
    char* pcLine = NULL;
    int i = 0;

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
    (void) strcpy(pcExt, EXT_YM);

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

    /* read and ignore file format */
    (void) getline(&pcLine, &iLen, pFHdr);
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
    /* read right ascension */
    (void) getline(&pcLine, &iLen, pFHdr);
    pcVal = strrchr(pcLine, ':');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return YAPP_RET_ERROR;
    }
    (void) sscanf(pcVal, ": %lf", &pstYUM->dSourceRA);
    /* read declination */
    (void) getline(&pcLine, &iLen, pFHdr);
    pcVal = strrchr(pcLine, ':');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return YAPP_RET_ERROR;
    }
    (void) sscanf(pcVal, ": %lf", &pstYUM->dSourceDec);
    /* read start MJD */
    (void) getline(&pcLine, &iLen, pFHdr);
    pcVal = strrchr(pcLine, ':');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return YAPP_RET_ERROR;
    }
    (void) sscanf(pcVal, ": %lf", &pstYUM->dTStart);
    /* read centre frequency */
    (void) getline(&pcLine, &iLen, pFHdr);
    pcVal = strrchr(pcLine, ':');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return YAPP_RET_ERROR;
    }
    (void) sscanf(pcVal, ": %f", &pstYUM->fFCentre);
    /* read bandwidth */
    (void) getline(&pcLine, &iLen, pFHdr);
    pcVal = strrchr(pcLine, ':');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return YAPP_RET_ERROR;
    }
    (void) sscanf(pcVal, ": %f", &pstYUM->fBW);
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
    /* read band-flip status */
    (void) getline(&pcLine, &iLen, pFHdr);
    pcVal = strrchr(pcLine, ':');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return YAPP_RET_ERROR;
    }
    (void) sscanf(pcVal, ": %s", acTemp);
    if (0 == strcmp(acTemp, "Yes"))
    {
        pstYUM->cIsBandFlipped = YAPP_TRUE;
    }
    /* read and ignore estimated number of bands */
    (void) getline(&pcLine, &iLen, pFHdr);
    /* read and ignore number of bad time sections */
    (void) getline(&pcLine, &iLen, pFHdr);
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
    /* ignore everything else */

    /* close the header file */
    (void) fclose(pFHdr);

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

    /* call all channels good - no support for SIGPROC ignore files yet */
    pstYUM->pcIsChanGood = (char *) YAPP_Malloc(pstYUM->iNumChans,
                                                sizeof(char),
                                                YAPP_FALSE);
    if (NULL == pstYUM->pcIsChanGood)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation for channel goodness "
                       "flag failed! %s!\n",
                       strerror(errno));
        return YAPP_RET_ERROR;
    }

    /* read the channel goodness flags */
    for (i = 0; i < pstYUM->iNumChans; ++i)
    {
        pstYUM->pcIsChanGood[i] = YAPP_TRUE;
    }

    free(pcLine);

    return YAPP_RET_SUCCESS;
}


int YAPP_ReadPRESTOHeaderFile(char *pcFileData, YUM_t *pstYUM)
{
    size_t iLen = 0;
    char *pcVal = NULL;
    char* pcLine = NULL;
    FILE *pFInf = NULL;
    char acFileInf[LEN_GENSTRING] = {0};
    char acTemp[LEN_GENSTRING] = {0};
    struct stat stFileStats = {0};
    int iRet = YAPP_RET_SUCCESS;
    char *pcExt = NULL;

    /* build the header file name */
    (void) strncpy(acFileInf, pcFileData, LEN_GENSTRING);
    pcExt = strrchr(acFileInf, '.');
    if (NULL == pcExt)
    {
        (void) fprintf(stderr,
                       "ERROR: Could not locate extension in file name %s!\n",
                       acFileInf);
        return YAPP_RET_ERROR;
    }
    (void) strcpy(pcExt, EXT_INF);
    pFInf = fopen(acFileInf, "r");
    if (NULL == pFInf)
    {
        fprintf(stderr,
                "ERROR: Opening file %s failed! %s.\n",
                acFileInf,
                strerror(errno));
        return EXIT_FAILURE;
    }

    while (getline(&pcLine, &iLen, pFInf) != YAPP_RET_ERROR)
    {
        pcVal = strrchr(pcLine, '=');
        if (NULL == pcVal)
        {
            /*(void) printf("WARNING: Failed to parse:\n%s\n", pcLine);*/
            continue;
        }

        if (0 == strncmp(pcLine, YAPP_PR_LABEL_SITE, YAPP_PR_LEN_LABEL))
        {
            (void) sscanf(pcVal, "=  %s", pstYUM->acSite);
        }
        else if (0 == strncmp(pcLine,
                              YAPP_PR_LABEL_SRCNAME,
                              YAPP_PR_LEN_LABEL))
        {
            (void) sscanf(pcVal, "=  %s", pstYUM->acPulsar);
        }
        else if (0 == strncmp(pcLine, YAPP_PR_LABEL_SRCRA, YAPP_PR_LEN_LABEL))
        {
            (void) sscanf(pcVal, "=  %s", acTemp);
            pstYUM->dSourceRA = YAPP_RAString2Double(acTemp);
        }
        else if (0 == strncmp(pcLine, YAPP_PR_LABEL_SRCDEC, YAPP_PR_LEN_LABEL))
        {
            (void) sscanf(pcVal, "=  %s", acTemp);
            pstYUM->dSourceDec = YAPP_DecString2Double(acTemp);
        }
        else if (0 == strncmp(pcLine, YAPP_PR_LABEL_TSTART, YAPP_PR_LEN_LABEL))
        {
            (void) sscanf(pcVal, "=  %lf", &pstYUM->dTStart);
        }
        else if (0 == strncmp(pcLine, YAPP_PR_LABEL_BARY, YAPP_PR_LEN_LABEL))
        {
            (void) sscanf(pcVal, "=  %d", &pstYUM->iFlagBary);
        }
        else if (0 == strncmp(pcLine, YAPP_PR_LABEL_NSAMPS, YAPP_PR_LEN_LABEL))
        {
            (void) sscanf(pcVal, "=  %d", &pstYUM->iTimeSamps);
        }
        else if (0 == strncmp(pcLine, YAPP_PR_LABEL_TSAMP, YAPP_PR_LEN_LABEL))
        {
            (void) sscanf(pcVal, "=  %lf", &pstYUM->dTSamp);
            pstYUM->dTSamp *= 1e3;  /* convert from s to ms */
        }
        else if (0 == strncmp(pcLine, YAPP_PR_LABEL_DM, YAPP_PR_LEN_LABEL))
        {
            (void) sscanf(pcVal, "=  %lf", &pstYUM->dDM);
        }
        else if (0 == strncmp(pcLine, YAPP_PR_LABEL_FMIN, YAPP_PR_LEN_LABEL))
        {
            (void) sscanf(pcVal, "=  %f", &pstYUM->fFMin);
        }
        else if (0 == strncmp(pcLine, YAPP_PR_LABEL_BW, YAPP_PR_LEN_LABEL))
        {
            (void) sscanf(pcVal, "=  %f", &pstYUM->fBW);
        }
        else if (0 == strncmp(pcLine, YAPP_PR_LABEL_NCHANS, YAPP_PR_LEN_LABEL))
        {
            (void) sscanf(pcVal, "=  %d", &pstYUM->iNumChans);
            /* set number of good channels to number of channels */
            pstYUM->iNumGoodChans = pstYUM->iNumChans;
        }
        else if (0 == strncmp(pcLine, YAPP_PR_LABEL_CHANBW, YAPP_PR_LEN_LABEL))
        {
            (void) sscanf(pcVal, "=  %f", &pstYUM->fChanBW);
        }
    }

    /* calculate bandwidth (even though we have read it) and centre
       frequency */
    pstYUM->fFMax = pstYUM->fFMin
                    + ((pstYUM->iNumChans - 1) * pstYUM->fChanBW);
    /* NOTE: max and min are the _centre_ frequencies of the bins, so the
             total bandwidth would be (max+chanbw/2)-(min-chanbw/2) */
    pstYUM->fBW = (pstYUM->fFMax - pstYUM->fFMin) + pstYUM->fChanBW;
    if (0 == (pstYUM->iNumChans % 2))   /* even number of channels */
    {
        pstYUM->fFCentre = (pstYUM->fFMin - (pstYUM->fChanBW / 2))
                           + ((pstYUM->iNumChans / 2) * pstYUM->fChanBW);
    }
    else                                /* odd number of channels */
    {
        pstYUM->fFCentre = pstYUM->fFMin
                           + (((float) pstYUM->iNumChans / 2)
                              * pstYUM->fChanBW);
    }

    pstYUM->iNumBits = YAPP_SAMPSIZE_32;    /* single-precision
                                               floating-point */
    pstYUM->fSampSize = ((float) pstYUM->iNumBits) / YAPP_BYTE2BIT_FACTOR;

    iRet = stat(pcFileData, &stFileStats);
    if (iRet != YAPP_RET_SUCCESS)
    {
        (void) fprintf(stderr,
                       "ERROR: Failed to stat %s: %s!\n",
                       pcFileData,
                       strerror(errno));
        return YAPP_RET_ERROR;
    }
    pstYUM->lDataSizeTotal = (long) stFileStats.st_size;

    free(pcLine);
    (void) fclose(pFInf);

    return EXIT_SUCCESS;
}


/*
 * Read one block of data from disk
 */
int YAPP_ReadData(FILE *pFData,
                  float *pfBuf,
                  float fSampSize,
                  int iTotSampsPerBlock)
{
    static char cIsFirst = YAPP_TRUE;
    static unsigned char *pcBuf = NULL;
    int iReadItems = 0;
    int i = 0;

    /* kludgy way to reset the static variable cIsFirst */
    if (NULL == pFData)
    {
        cIsFirst = YAPP_TRUE;
        return YAPP_RET_SUCCESS;
    }

    if (cIsFirst)
    {
        /* allocate memory for the byte buffer, based on the total number of
           samples per block (= number of channels * number of time samples per
           block) */
        pcBuf = (unsigned char *) YAPP_Malloc((int) (iTotSampsPerBlock
                                                     * fSampSize),
                                                     sizeof(char),
                                                     YAPP_FALSE);
        if (NULL == pcBuf)
        {
            (void) fprintf(stderr,
                           "ERROR: Memory allocation failed for buffer! %s!\n",
                           strerror(errno));
            return YAPP_RET_ERROR;
        }
        cIsFirst = YAPP_FALSE;
    }

    /* read data into the byte buffer */
    iReadItems = fread(pcBuf,
                       sizeof(unsigned char),
                       (int) (iTotSampsPerBlock * fSampSize),
                       pFData);
    if (ferror(pFData))
    {
        (void) fprintf(stderr, "ERROR: File read failed!\n");
        return YAPP_RET_ERROR;
    }
    iReadItems = (int) ((float) iReadItems / fSampSize);

    if (YAPP_SAMPSIZE_32 == (fSampSize * YAPP_BYTE2BIT_FACTOR))
    {
        /* 32-bit/4-byte floating-point data */
        /* copy data from the byte buffer to the float buffer */
        (void) memcpy(pfBuf, pcBuf, (int) (iTotSampsPerBlock * fSampSize));
    }
    else if (YAPP_SAMPSIZE_16 == (fSampSize * YAPP_BYTE2BIT_FACTOR))
    {
        /* 16-bit/2-byte data */
        unsigned short int* psBuf = (unsigned short int*) pcBuf;
        for (i = 0; i < iReadItems; ++i)
        {
            pfBuf[i] = (float) psBuf[i];
        }
    }
    else if (YAPP_SAMPSIZE_8 == (fSampSize * YAPP_BYTE2BIT_FACTOR))
    {
        /* 8-bit/1-byte data */
        /* copy data from the byte buffer to the float buffer */
        #if 0
        //TODO: this works for VEGAS viewdata
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
        #else
        for (i = 0; i < iReadItems; ++i)
        {
            pfBuf[i] = (float) pcBuf[i];
        }
        #endif
    }
    else if (YAPP_SAMPSIZE_4 == (fSampSize * YAPP_BYTE2BIT_FACTOR))
    {
        /* 4-bit/0.5-byte data */
        /* copy data from the byte buffer to the float buffer */
        for (i = 0; i < (iReadItems / 2); ++i)
        {
            /* copy lower 4 bits */
            pfBuf[2*i] = (float) (pcBuf[i] & 0x0F);
            /* copy upper 4 bits */
            pfBuf[(2*i)+1] = (float) ((pcBuf[i] & 0xF0) >> 4);
        }
    }
    else if (YAPP_SAMPSIZE_2 == (fSampSize * YAPP_BYTE2BIT_FACTOR))
    {
        /* 2-bit/0.25-byte data */
        /* copy data from the byte buffer to the float buffer */
        for (i = 0; i < (iReadItems / 4); ++i)
        {
            /* copy lowest 2 bits */
            pfBuf[4*i] = (float) (pcBuf[i] & 0x03);
            /* copy next 2 bits */
            pfBuf[(4*i)+1] = (float) ((pcBuf[i] & 0x0C) >> 2);
            /* copy next 2 bits */
            pfBuf[(4*i)+2] = (float) ((pcBuf[i] & 0x30) >> 4);
            /* copy uppermost 2 bits */
            pfBuf[(4*i)+3] = (float) ((pcBuf[i] & 0xC0) >> 6);
        }
    }

    return iReadItems;
}


#ifdef HDF5
/*
 * Read HDF5 data.
 */
int YAPP_ReadHDF5Data(hid_t hDataspace,
                      hid_t hDataset,
                      hsize_t *hOffset,
                      hsize_t *hCount,
                      hid_t hMemDataspace,
                      hid_t hType,
                      float *pfBuf,
                      float fSampSize,
                      int iTotSampsPerBlock)
{
    static char cIsFirst = YAPP_TRUE;
    static unsigned char *pcBuf = NULL;
    int iReadItems = 0;
    hsize_t hStride[YAPP_HDF5_DYNSPEC_RANK] = {0};
    hsize_t hBlock[YAPP_HDF5_DYNSPEC_RANK] = {0};
    herr_t hStatus = 0;
    int i = 0;

    if (cIsFirst)
    {
        /* allocate memory for the byte buffer, based on the total number of
           samples per block (= number of channels * number of time samples per
           block) */
        pcBuf = (unsigned char *) YAPP_Malloc((int) (iTotSampsPerBlock
                                                     * fSampSize),
                                                     sizeof(char),
                                                     YAPP_FALSE);
        if (NULL == pcBuf)
        {
            (void) fprintf(stderr,
                           "ERROR: Memory allocation failed for buffer! %s!\n",
                           strerror(errno));
            return YAPP_RET_ERROR;
        }
        cIsFirst = YAPP_FALSE;
    }

    /* read data into the byte buffer */
    hStride[0] = 1;
    hStride[1] = 1;
    hBlock[0] = 1;
    hBlock[1] = 1;
    hStatus = H5Sselect_hyperslab(hDataspace,
                                  H5S_SELECT_SET,
                                  hOffset,
                                  hStride,
                                  hCount,
                                  hBlock);
    hStatus = H5Dread(hDataset,
                      hType,
                      hMemDataspace,
                      hDataspace,
                      H5P_DEFAULT,
                      (void *) pcBuf);
    if (hStatus < 0)
    {
        (void) fprintf(stderr, "ERROR: File read failed!\n");
        return YAPP_RET_ERROR;
    }
    iReadItems = (int) (hCount[0] * hCount[1]);

    if (YAPP_SAMPSIZE_32 == (fSampSize * YAPP_BYTE2BIT_FACTOR))
    {
        /* 32-bit/4-byte floating-point data */
        /* copy data from the byte buffer to the float buffer */
        (void) memcpy(pfBuf, pcBuf, (int) (iTotSampsPerBlock * fSampSize));
    }
    else if (YAPP_SAMPSIZE_16 == (fSampSize * YAPP_BYTE2BIT_FACTOR))
    {
        /* 16-bit/2-byte data */
        unsigned short int* psBuf = (unsigned short int*) pcBuf;
        for (i = 0; i < iReadItems; ++i)
        {
            pfBuf[i] = (float) psBuf[i];
        }
    }
    else if (YAPP_SAMPSIZE_8 == (fSampSize * YAPP_BYTE2BIT_FACTOR))
    {
        /* 8-bit/1-byte data */
        /* copy data from the byte buffer to the float buffer */
        #if 0
        //TODO: this works for VEGAS viewdata
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
        #else
        for (i = 0; i < iReadItems; ++i)
        {
            pfBuf[i] = (float) pcBuf[i];
        }
        #endif
    }
    else if (YAPP_SAMPSIZE_4 == (fSampSize * YAPP_BYTE2BIT_FACTOR))
    {
        /* 4-bit/0.5-byte data */
        /* copy data from the byte buffer to the float buffer */
        for (i = 0; i < (iReadItems / 2); ++i)
        {
            /* copy lower 4 bits */
            pfBuf[2*i] = (float) (pcBuf[i] & 0x0F);
            /* copy upper 4 bits */
            pfBuf[(2*i)+1] = (float) ((pcBuf[i] & 0xF0) >> 4);
        }
    }
    else if (YAPP_SAMPSIZE_2 == (fSampSize * YAPP_BYTE2BIT_FACTOR))
    {
        /* 2-bit/0.25-byte data */
        /* copy data from the byte buffer to the float buffer */
        for (i = 0; i < (iReadItems / 4); ++i)
        {
            /* copy lowest 2 bits */
            pfBuf[4*i] = (float) (pcBuf[i] & 0x03);
            /* copy next 2 bits */
            pfBuf[(4*i)+1] = (float) ((pcBuf[i] & 0x0C) >> 2);
            /* copy next 2 bits */
            pfBuf[(4*i)+2] = (float) ((pcBuf[i] & 0x30) >> 4);
            /* copy uppermost 2 bits */
            pfBuf[(4*i)+3] = (float) ((pcBuf[i] & 0xC0) >> 6);
        }
    }

    return iReadItems;
}
#endif

/*
 * Writes header to a data file.
 */
int YAPP_WriteMetadata(char *pcFileData, int iFormat, YUM_t stYUM)
{
    char acLabel[LEN_GENSTRING] = {0};
    int iLen = 0;
    int iTemp = 0;
    double dTemp = 0.0;

    if ((YAPP_FORMAT_FIL == iFormat) || (YAPP_FORMAT_DTS_TIM == iFormat))
    {
        FILE *pFData = fopen(pcFileData, "w");
        if (NULL == pFData)
        {
            fprintf(stderr,
                    "ERROR: Opening file %s failed! %s.\n",
                    pcFileData,
                    strerror(errno));
            return EXIT_FAILURE;
        }

        /* write the parameters to the header section of the file */
        /* start with the 'HEADER_START' label */
        iLen = strlen(YAPP_SP_LABEL_HDRSTART);
        (void) fwrite(&iLen, sizeof(iLen), 1, pFData);
        (void) strcpy(acLabel, YAPP_SP_LABEL_HDRSTART);
        (void) fwrite(acLabel, sizeof(char), iLen, pFData);

        /* write the rest of the header */
        /* write source name */
        /* write field label length */
        iLen = strlen(YAPP_SP_LABEL_SRCNAME);
        (void) fwrite(&iLen, sizeof(iLen), 1, pFData);
        /* write field label */
        (void) strcpy(acLabel, YAPP_SP_LABEL_SRCNAME);
        (void) fwrite(acLabel, sizeof(char), iLen, pFData);
        iLen = strlen(stYUM.acPulsar);
        (void) fwrite(&iLen, sizeof(iLen), 1, pFData);
        (void) fwrite(stYUM.acPulsar, sizeof(char), iLen, pFData);

        /* write data type */
        if (YAPP_FORMAT_FIL == iFormat)
        {
            iTemp = 1;      /* 'filterbank' */
        }
        else if (YAPP_FORMAT_DTS_TIM == iFormat)
        {
            iTemp = 2;      /* 'time series (topocentric)' */
        }
        iLen = strlen(YAPP_SP_LABEL_DATATYPE);
        (void) fwrite(&iLen, sizeof(iLen), 1, pFData);
        (void) strcpy(acLabel, YAPP_SP_LABEL_DATATYPE);
        (void) fwrite(acLabel, sizeof(char), iLen, pFData);
        (void) fwrite(&iTemp,
                      sizeof(iTemp),
                      1,
                      pFData);

        /* NOTE: number of channels, frequency of first channel, and channel
                 bandwidth are not strictly required in a .tim file, but they
                 are required if .tim files for different frequency bands need
                 to be combined */
        /* write number of channels */
        iLen = strlen(YAPP_SP_LABEL_NUMCHANS);
        (void) fwrite(&iLen, sizeof(iLen), 1, pFData);
        (void) strcpy(acLabel, YAPP_SP_LABEL_NUMCHANS);
        (void) fwrite(acLabel, sizeof(char), iLen, pFData);
        (void) fwrite(&stYUM.iNumChans,
                      sizeof(stYUM.iNumChans),
                      1,
                      pFData);

        /* write frequency of first channel */
        iLen = strlen(YAPP_SP_LABEL_FCHAN1);
        (void) fwrite(&iLen, sizeof(iLen), 1, pFData);
        (void) strcpy(acLabel, YAPP_SP_LABEL_FCHAN1);
        (void) fwrite(acLabel, sizeof(char), iLen, pFData);
        if (stYUM.cIsBandFlipped)
        {
            dTemp = (double) stYUM.fFMax;
        }
        else
        {
            dTemp = (double) stYUM.fFMin;
        }
        (void) fwrite(&dTemp,
                      sizeof(dTemp),
                      1,
                      pFData);

        /* write channel bandwidth */
        iLen = strlen(YAPP_SP_LABEL_CHANBW);
        (void) fwrite(&iLen, sizeof(iLen), 1, pFData);
        (void) strcpy(acLabel, YAPP_SP_LABEL_CHANBW);
        (void) fwrite(acLabel, sizeof(char), iLen, pFData);
        if (stYUM.cIsBandFlipped)
        {
            dTemp = (double) (-stYUM.fChanBW);
        }
        else
        {
            dTemp = (double) stYUM.fChanBW;
        }
        (void) fwrite(&dTemp,
                      sizeof(dTemp),
                      1,
                      pFData);

        /* write number of bits per sample */
        iLen = strlen(YAPP_SP_LABEL_NUMBITS);
        (void) fwrite(&iLen, sizeof(iLen), 1, pFData);
        (void) strcpy(acLabel, YAPP_SP_LABEL_NUMBITS);
        (void) fwrite(acLabel, sizeof(char), iLen, pFData);
        (void) fwrite(&stYUM.iNumBits,
                      sizeof(stYUM.iNumBits),
                      1,
                      pFData);

        /* write number of IFs */
        iLen = strlen(YAPP_SP_LABEL_NUMIFS);
        (void) fwrite(&iLen, sizeof(iLen), 1, pFData);
        (void) strcpy(acLabel, YAPP_SP_LABEL_NUMIFS);
        (void) fwrite(acLabel, sizeof(char), iLen, pFData);
        (void) fwrite(&stYUM.iNumIFs,
                      sizeof(stYUM.iNumIFs),
                      1,
                      pFData);

        /* write sampling time in seconds */
        iLen = strlen(YAPP_SP_LABEL_TSAMP);
        (void) fwrite(&iLen, sizeof(iLen), 1, pFData);
        (void) strcpy(acLabel, YAPP_SP_LABEL_TSAMP);
        (void) fwrite(acLabel, sizeof(char), iLen, pFData);
        double dTSampInSec = stYUM.dTSamp / 1e3;
        (void) fwrite(&dTSampInSec,
                      sizeof(dTSampInSec),
                      1,
                      pFData);

        /* write timestamp of first sample (MJD) */
        iLen = strlen(YAPP_SP_LABEL_TSTART);
        (void) fwrite(&iLen, sizeof(iLen), 1, pFData);
        (void) strcpy(acLabel, YAPP_SP_LABEL_TSTART);
        (void) fwrite(acLabel, sizeof(char), iLen, pFData);
        (void) fwrite(&stYUM.dTStart,
                      sizeof(stYUM.dTStart),
                      1,
                      pFData);

        /* write telescope ID */
        iLen = strlen(YAPP_SP_LABEL_OBSID);
        (void) fwrite(&iLen, sizeof(iLen), 1, pFData);
        (void) strcpy(acLabel, YAPP_SP_LABEL_OBSID);
        (void) fwrite(acLabel, sizeof(char), iLen, pFData);
        iTemp = YAPP_SP_GetObsIDFromName(stYUM.acSite);
        if (YAPP_RET_ERROR == iTemp)
        {
            /* could not identify observatory, set it to 'unknown/fake' */
            (void) fprintf(stderr,
                          "WARNING: Identifying observatory failed, setting "
                          "site to %s!\n",
                          YAPP_SP_OBS_FAKE);
            iTemp = YAPP_SP_OBSID_FAKE;
        }
        (void) fwrite(&iTemp,
                      sizeof(iTemp),
                      1,
                      pFData);

        /* write backend ID */
        iLen = strlen(YAPP_SP_LABEL_BEID);
        (void) fwrite(&iLen, sizeof(iLen), 1, pFData);
        (void) strcpy(acLabel, YAPP_SP_LABEL_BEID);
        (void) fwrite(acLabel, sizeof(char), iLen, pFData);
        (void) fwrite(&stYUM.iBackendID,
                      sizeof(stYUM.iBackendID),
                      1,
                      pFData);

        /* write source RA (J2000) */
        iLen = strlen(YAPP_SP_LABEL_SRCRA);
        (void) fwrite(&iLen, sizeof(iLen), 1, pFData);
        (void) strcpy(acLabel, YAPP_SP_LABEL_SRCRA);
        (void) fwrite(acLabel, sizeof(char), iLen, pFData);
        /* SIGPROC requires the RA and declination be scaled by 10000 */
        dTemp = stYUM.dSourceRA * YAPP_SP_RADEC_SCALE;
        /* SIGPROC stores RA in hours, so convert from degrees to hours */
        dTemp /= YAPP_DEGPERHOUR;
        (void) fwrite(&dTemp, sizeof(dTemp), 1, pFData);

        /* write source declination (J2000) */
        iLen = strlen(YAPP_SP_LABEL_SRCDEC);
        (void) fwrite(&iLen, sizeof(iLen), 1, pFData);
        (void) strcpy(acLabel, YAPP_SP_LABEL_SRCDEC);
        (void) fwrite(acLabel, sizeof(char), iLen, pFData);
        /* SIGPROC requires the RA and declination be scaled by 10000 */
        dTemp = stYUM.dSourceDec * YAPP_SP_RADEC_SCALE;
        (void) fwrite(&dTemp, sizeof(dTemp), 1, pFData);

        /* write azimuth start */
        iLen = strlen(YAPP_SP_LABEL_AZSTART);
        (void) fwrite(&iLen, sizeof(iLen), 1, pFData);
        (void) strcpy(acLabel, YAPP_SP_LABEL_AZSTART);
        (void) fwrite(acLabel, sizeof(char), iLen, pFData);
        (void) fwrite(&stYUM.dAzStart,
                      sizeof(stYUM.dAzStart),
                      1,
                      pFData);

        /* write ZA start */
        iLen = strlen(YAPP_SP_LABEL_ZASTART);
        (void) fwrite(&iLen, sizeof(iLen), 1, pFData);
        (void) strcpy(acLabel, YAPP_SP_LABEL_ZASTART);
        (void) fwrite(acLabel, sizeof(char), iLen, pFData);
        (void) fwrite(&stYUM.dZAStart,
                      sizeof(stYUM.dZAStart),
                      1,
                      pFData);

        /* write reference DM */
        if (YAPP_FORMAT_DTS_TIM == iFormat)
        {
            iLen = strlen(YAPP_SP_LABEL_DM);
            (void) fwrite(&iLen, sizeof(iLen), 1, pFData);
            (void) strcpy(acLabel, YAPP_SP_LABEL_DM);
            (void) fwrite(acLabel, sizeof(char), iLen, pFData);
            (void) fwrite(&stYUM.dDM, sizeof(stYUM.dDM), 1, pFData);
        }

        /* write barycentric flag */
        iLen = strlen(YAPP_SP_LABEL_FLAGBARY);
        (void) fwrite(&iLen, sizeof(iLen), 1, pFData);
        (void) strcpy(acLabel, YAPP_SP_LABEL_FLAGBARY);
        (void) fwrite(acLabel, sizeof(char), iLen, pFData);
        (void) fwrite(&stYUM.iFlagBary,
                      sizeof(stYUM.iFlagBary),
                      1,
                      pFData);

        /* write header end tag */
        iLen = strlen(YAPP_SP_LABEL_HDREND);
        (void) fwrite(&iLen, sizeof(iLen), 1, pFData);
        (void) strcpy(acLabel, YAPP_SP_LABEL_HDREND);
        (void) fwrite(acLabel, sizeof(char), iLen, pFData);

        (void) fclose(pFData);
    }
    else if (YAPP_FORMAT_DTS_DAT == iFormat)
    {
        char *pcFilename = NULL;
        FILE *pFInf = NULL;
        char acFileInf[LEN_GENSTRING] = {0};
        char acTemp[LEN_GENSTRING] = {0};

        pcFilename = YAPP_GetFilenameFromPath(pcFileData);

        (void) strcpy(acFileInf, pcFilename);
        (void) strcat(acFileInf, EXT_INF);
        pFInf = fopen(acFileInf, "w");
        if (NULL == pFInf)
        {
            (void) fprintf(stderr,
                           "ERROR: Opening file %s failed! %s.\n",
                           acFileInf,
                           strerror(errno));
            return EXIT_FAILURE;
        }

        (void) fprintf(pFInf,
                       " Data file name without suffix          =  %s\n",
                       pcFilename);
        (void) fprintf(pFInf,
                       " Telescope used                         =  %s\n",
                       stYUM.acSite);
        (void) fprintf(pFInf,
                       " Instrument used                        =  %d\n",
                       stYUM.iBackendID);
        (void) fprintf(pFInf,
                       " Object being observed                  =  %s\n",
                       stYUM.acPulsar);
        YAPP_RADouble2String(stYUM.dSourceRA, acTemp);
        (void) fprintf(pFInf,
                       " J2000 Right Ascension (hh:mm:ss.ssss)  =  %s\n",
                       acTemp);
        YAPP_DecDouble2String(stYUM.dSourceDec, acTemp);
        (void) fprintf(pFInf,
                       " J2000 Declination     (dd:mm:ss.ssss)  =  %s\n",
                       acTemp);
        (void) fprintf(pFInf,
                       " Epoch of observation (MJD)             =  %.15g\n",
                       stYUM.dTStart);
        (void) fprintf(pFInf,
                       " Barycentered?           (1=yes, 0=no)  =  %d\n",
                       stYUM.iFlagBary);
        (void) fprintf(pFInf,
                       " Number of bins in the time series      =  %d\n",
                       stYUM.iTimeSamps);
        (void) fprintf(pFInf,
                       " Width of each time series bin (sec)    =  %.10g\n",
                       stYUM.dTSamp * 1e-3);
        (void) fprintf(pFInf,
                       " Any breaks in the data? (1=yes, 0=no)  =  0\n");
        (void) fprintf(pFInf,
                       " Type of observation (EM band)          =  Radio\n");
        (void) fprintf(pFInf,
                       " Dispersion measure (cm-3 pc)           =  %g\n",
                       stYUM.dDM);
        (void) fprintf(pFInf,
                       " Central freq of low channel (Mhz)      =  %.10g\n",
                       stYUM.fFMin);
        (void) fprintf(pFInf,
                       " Total bandwidth (Mhz)                  =  %.10g\n",
                       stYUM.fBW);
        (void) fprintf(pFInf,
                       " Number of channels                     =  %d\n",
                       stYUM.iNumChans);
        (void) fprintf(pFInf,
                       " Channel bandwidth (Mhz)                =  %.10g\n",
                       stYUM.fChanBW);

        (void) fclose(pFInf);
    }
#ifdef HDF5
    else if (YAPP_FORMAT_HDF5 == iFormat)
    {
        hid_t hFile = 0;
        hid_t hGroup = 0;
        hid_t hDataspace = 0;
        hid_t hDataset = 0;
        hid_t hPropList = 0;
        hid_t hType = 0;
        hsize_t hDims[YAPP_HDF5_DYNSPEC_RANK] = {0};
        unsigned int aiOptions[YAPP_HDF5_SIZE_FILTER_OPTS] = {0};
        herr_t hStatus = 0;

        /* create file */
        hFile = H5Fcreate(pcFileData,
                          H5F_ACC_TRUNC,
                          H5P_DEFAULT,
                          H5P_DEFAULT);
        if (hFile < 0)
        {
            (void) fprintf(stderr,
                           "ERROR: Creating file %s failed!\n",
                           pcFileData);
            return YAPP_RET_ERROR;
        }

        /* create group */
        hGroup = H5Gcreate(hFile,
                           YAPP_HDF5_DYNSPEC_GROUP,
                           H5P_DEFAULT,
                           H5P_DEFAULT,
                           H5P_DEFAULT);
        if (hGroup < 0)
        {
            (void) fprintf(stderr,
                           "ERROR: Creating group %s failed!\n",
                           YAPP_HDF5_DYNSPEC_GROUP);
            return YAPP_RET_ERROR;
        }

        /* create dataspace */
        hDims[1] = stYUM.iNumChans;
        hDims[0] = stYUM.iTimeSamps;
        hDataspace = H5Screate_simple(YAPP_HDF5_DYNSPEC_RANK, hDims, NULL);
        if (hDataspace < 0)
        {
            (void) fprintf(stderr,
                           "ERROR: Creating dataspace failed!\n");
            return YAPP_RET_ERROR;
        }

        switch (stYUM.iNumBits)
        {
            case YAPP_SAMPSIZE_8:
                hType = H5T_STD_I8LE;
                break;

            case YAPP_SAMPSIZE_16:
                hType = H5T_STD_I16LE;
                break;

            case YAPP_SAMPSIZE_32:
                hType = H5T_IEEE_F32LE;
                break;

            default:
                /* we don't expect this */
                assert ((YAPP_SAMPSIZE_8 == stYUM.iNumBits)
                        || (YAPP_SAMPSIZE_16 == stYUM.iNumBits)
                        || (YAPP_SAMPSIZE_32 == stYUM.iNumBits));
                return YAPP_RET_ERROR;
        }

        hPropList = H5Pcreate(H5P_DATASET_CREATE);
        if (hPropList < 0)
        {
            (void) fprintf(stderr,
                           "ERROR: Creating property list failed!\n");
            return YAPP_RET_ERROR;
        }
        hStatus = H5Pset_chunk(hPropList,
                               YAPP_HDF5_DYNSPEC_RANK,
                               (hsize_t *) stYUM.lChunkDims);
        if (hStatus < 0)
        {
            (void) fprintf(stderr,
                           "ERROR: Setting chunk size failed!\n");
            return YAPP_RET_ERROR;
        }
        aiOptions[0] = 0;
        aiOptions[1] = YAPP_HDF5_FILTER_OPTS_LZ4;
        hStatus = H5Pset_filter(hPropList,
                                YAPP_HDF5_FILTER_ID,
                                H5Z_FLAG_MANDATORY,
                                YAPP_HDF5_SIZE_FILTER_OPTS,
                                aiOptions);
        /*hStatus = H5Pset_deflate(hPropList, 6);*/
        if (hStatus < 0)
        {
            (void) fprintf(stderr,
                           "ERROR: Setting filter failed!\n");
            return YAPP_RET_ERROR;
        }

        /* create dataset */
        hDataset = H5Dcreate(hGroup,
                             YAPP_HDF5_DYNSPEC_GROUP YAPP_HDF5_DYNSPEC_DATASET,
                             hType,
                             hDataspace,
                             H5P_DEFAULT,
                             hPropList,
                             H5P_DEFAULT);
        if (hDataset < 0)
        {
            (void) fprintf(stderr,
                           "ERROR: Creating dataset %s failed!\n",
                           YAPP_HDF5_DYNSPEC_GROUP YAPP_HDF5_DYNSPEC_DATASET);
            return YAPP_RET_ERROR;
        }

        /* create attributes */

        /* write telescope site */
        (void) YAPP_WriteHDF5StringAttribute(hDataset,
                                             YAPP_HDF5_ATTRNAME_SITE,
                                             stYUM.acSite);

        /* write source name */
        (void) YAPP_WriteHDF5StringAttribute(hDataset,
                                             YAPP_HDF5_ATTRNAME_SRCNAME,
                                             stYUM.acPulsar);

        /* write sampling time */
        (void) YAPP_WriteHDF5DoubleAttribute(hDataset,
                                             YAPP_HDF5_ATTRNAME_TSAMP,
                                             stYUM.dTSamp);

        /* write centre frequency */
        (void) YAPP_WriteHDF5FloatAttribute(hDataset,
                                            YAPP_HDF5_ATTRNAME_FCEN,
                                            stYUM.fFCentre);

        /* write bandwidth */
        (void) YAPP_WriteHDF5FloatAttribute(hDataset,
                                            YAPP_HDF5_ATTRNAME_BW,
                                            stYUM.fBW);

        /* write number of channels */
        (void) YAPP_WriteHDF5IntAttribute(hDataset,
                                          YAPP_HDF5_ATTRNAME_NUMCHANS,
                                          stYUM.iNumChans);

        /* write channel bandwidth */
        (void) YAPP_WriteHDF5FloatAttribute(hDataset,
                                            YAPP_HDF5_ATTRNAME_CHANBW,
                                            stYUM.fChanBW);

        /* write number of time samples */
        (void) YAPP_WriteHDF5IntAttribute(hDataset,
                                          YAPP_HDF5_ATTRNAME_TIMESAMPS,
                                          stYUM.iTimeSamps);

        /* write number of bits */
        (void) YAPP_WriteHDF5IntAttribute(hDataset,
                                          YAPP_HDF5_ATTRNAME_NUMBITS,
                                          stYUM.iNumBits);

        //TODO: think about this nifs/npols/nbeams?
        /* write number of IFs */
        (void) YAPP_WriteHDF5IntAttribute(hDataset,
                                          YAPP_HDF5_ATTRNAME_NUMIFS,
                                          stYUM.iNumIFs);

        //TODO: think about this (name?)
        /* write backend ID */
        (void) YAPP_WriteHDF5IntAttribute(hDataset,
                                          YAPP_HDF5_ATTRNAME_BACKEND,
                                          stYUM.iBackendID);

        /* write source RA */
        (void) YAPP_WriteHDF5DoubleAttribute(hDataset,
                                            YAPP_HDF5_ATTRNAME_SRCRA,
                                            stYUM.dSourceRA);

        /* write source dec. */
        (void) YAPP_WriteHDF5DoubleAttribute(hDataset,
                                            YAPP_HDF5_ATTRNAME_SRCDEC,
                                            stYUM.dSourceDec);

        //TODO: think about this. should min and max be written at all?
        /* write lowest frequency */
        (void) YAPP_WriteHDF5FloatAttribute(hDataset,
                                            YAPP_HDF5_ATTRNAME_FMIN,
                                            stYUM.fFMin);

        /* write highest frequency */
        (void) YAPP_WriteHDF5FloatAttribute(hDataset,
                                            YAPP_HDF5_ATTRNAME_FMAX,
                                            stYUM.fFMax);

        /* write band flip status */
        (void) YAPP_WriteHDF5CharAttribute(hDataset,
                                           YAPP_HDF5_ATTRNAME_BFLIP,
                                           stYUM.cIsBandFlipped);

        /* write start time */
        (void) YAPP_WriteHDF5DoubleAttribute(hDataset,
                                             YAPP_HDF5_ATTRNAME_TSTART,
                                             stYUM.dTStart);

        /* close stuff */
        (void) H5Dclose(hDataset);
        (void) H5Pclose(hPropList);
        (void) H5Sclose(hDataspace);
        (void) H5Gclose(hGroup);
        (void) H5Fclose(hFile);

    }
#endif

    return YAPP_RET_SUCCESS;
}


#ifdef HDF5
/*
 * Write HDF5 string attribute to a dataset
 */
int YAPP_WriteHDF5StringAttribute(hid_t hDataset,
                                  char *pcAttrName,
                                  char *pcAttrVal)
{
    hid_t hAttrDataspace = 0;
    hid_t hType = 0;
    hid_t hAttr = 0;

    /* create string type */
    hType = H5Tcopy(H5T_C_S1);
    (void) H5Tset_size(hType, strlen(pcAttrVal) + 1);
    (void) H5Tset_strpad(hType, H5T_STR_NULLTERM);

    /* create and write attribute */
    hAttrDataspace = H5Screate(H5S_SCALAR);
    hAttr = H5Acreate(hDataset,
                      pcAttrName,
                      hType,
                      hAttrDataspace,
                      H5P_DEFAULT,
                      H5P_DEFAULT);
    (void) H5Awrite(hAttr, hType, pcAttrVal);

    /* free resources */
    (void) H5Aclose(hAttr);
    (void) H5Tclose(hType);
    (void) H5Sclose(hAttrDataspace);

    return YAPP_RET_SUCCESS;
}


/*
 * Write HDF5 32-bit integer attribute to a dataset
 */
int YAPP_WriteHDF5IntAttribute(hid_t hDataset,
                               char *pcAttrName,
                               int iAttrVal)
{
    hid_t hAttrDataspace = 0;
    hid_t hAttr = 0;

    /* create and write attribute */
    hAttrDataspace = H5Screate(H5S_SCALAR);
    hAttr = H5Acreate(hDataset,
                      pcAttrName,
                      H5T_STD_I32LE,
                      hAttrDataspace,
                      H5P_DEFAULT,
                      H5P_DEFAULT);
    (void) H5Awrite(hAttr, H5T_STD_I32LE, (const void *) &iAttrVal);

    /* free resources */
    (void) H5Aclose(hAttr);
    (void) H5Sclose(hAttrDataspace);

    return YAPP_RET_SUCCESS;
}


/*
 * Write HDF5 float attribute to a dataset
 */
int YAPP_WriteHDF5FloatAttribute(hid_t hDataset,
                                 char *pcAttrName,
                                 float fAttrVal)
{
    hid_t hAttrDataspace = 0;
    hid_t hAttr = 0;

    /* create and write attribute */
    hAttrDataspace = H5Screate(H5S_SCALAR);
    hAttr = H5Acreate(hDataset,
                      pcAttrName,
                      H5T_IEEE_F32LE,
                      hAttrDataspace,
                      H5P_DEFAULT,
                      H5P_DEFAULT);
    (void) H5Awrite(hAttr, H5T_IEEE_F32LE, (const void *) &fAttrVal);

    /* free resources */
    (void) H5Aclose(hAttr);
    (void) H5Sclose(hAttrDataspace);

    return YAPP_RET_SUCCESS;
}


/*
 * Write HDF5 double attribute to a dataset
 */
int YAPP_WriteHDF5DoubleAttribute(hid_t hDataset,
                                  char *pcAttrName,
                                  double dAttrVal)
{
    hid_t hAttrDataspace = 0;
    hid_t hAttr = 0;

    /* create and write attribute */
    hAttrDataspace = H5Screate(H5S_SCALAR);
    hAttr = H5Acreate(hDataset,
                      pcAttrName,
                      H5T_IEEE_F64LE,
                      hAttrDataspace,
                      H5P_DEFAULT,
                      H5P_DEFAULT);
    (void) H5Awrite(hAttr, H5T_IEEE_F64LE, (const void *) &dAttrVal);

    /* free resources */
    (void) H5Aclose(hAttr);
    (void) H5Sclose(hAttrDataspace);

    return YAPP_RET_SUCCESS;
}


/*
 * Write HDF5 char attribute to a dataset
 */
int YAPP_WriteHDF5CharAttribute(hid_t hDataset,
                                char *pcAttrName,
                                char cAttrVal)
{
    hid_t hAttrDataspace = 0;
    hid_t hAttr = 0;

    /* create and write attribute */
    hAttrDataspace = H5Screate(H5S_SCALAR);
    hAttr = H5Acreate(hDataset,
                      pcAttrName,
                      H5T_STD_I8LE,
                      hAttrDataspace,
                      H5P_DEFAULT,
                      H5P_DEFAULT);
    (void) H5Awrite(hAttr, H5T_STD_I8LE, (const void *) &cAttrVal);

    /* free resources */
    (void) H5Aclose(hAttr);
    (void) H5Sclose(hAttrDataspace);

    return YAPP_RET_SUCCESS;
}
#endif


/*
 * Smooth data
 */
int YAPP_Smooth(float* pfInBuf,
                int iBlockSize,
                int iSampsPerWin,
                float* pfOutBuf)
{
    int i = 0;
    int j = 0;
    int k = 0;
    int iLow = -(iSampsPerWin / 2);
    int iHigh = iSampsPerWin / 2;

    for (i = -iLow; i < (iBlockSize + iLow); ++i)
    {
        for (j = iLow; j <= iHigh; ++j)
        {
            pfOutBuf[k] += pfInBuf[i+j];
        }
        pfOutBuf[k] /= iSampsPerWin;
        ++k;
    }

    return YAPP_RET_SUCCESS;
}


/*
 * Decimate data in frequency and time
 */
void YAPP_Decimate(float *pfInBuf,
                   int iBlockSize,
                   int iSampsPerWin,
                   int iNumChans,
                   int iChansPerWin,
                   float *pfOutBuf,
                   int iOutNumChans)
{
    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;
    float *pfInSpectrum = NULL;
    float *pfOutSpectrum = NULL;

    /* decimate in frequency */
    for (i = 0; i < iBlockSize; ++i)
    {
        pfInSpectrum = pfInBuf + i * iNumChans;
        for (j = 0; j < iNumChans - (iChansPerWin - 1); ++j)
        {
            for (k = 1; k < iChansPerWin; ++k)
            {
                pfInSpectrum[j] += pfInSpectrum[j+k];
            }
        }
    }

    /* decimate in time */
    /* NOTE: since we've already decimated in frequency, step by
       iChansPerWin */
    for (i = 0; i < iNumChans; i+= iChansPerWin)
    {
        for (j = 0; j < iBlockSize - (iSampsPerWin - 1); ++j)
        {
            for (k = 1; k < iSampsPerWin; ++k)
            {
                *(pfInBuf + j * iNumChans + i)
                    += *(pfInBuf + (j + k) * iNumChans + i);
            }
        }
    }

    /* scale and downsample */
    for (i = 0, j = 0; i < iBlockSize; i += iSampsPerWin, ++j)
    {
        pfInSpectrum = pfInBuf + i * iNumChans;
        pfOutSpectrum = pfOutBuf + j * iOutNumChans;
        for (k = 0, l = 0; k < iNumChans; k += iChansPerWin, ++l)
        {
            pfOutSpectrum[l] = pfInSpectrum[k]
                                / (iChansPerWin * iSampsPerWin);
        }
    }

    return;
}


void YAPP_Float2Nibble(float *pfBuf,
                       int iLen,
                       float fMin,
                       float fMax,
                       unsigned char *pcBuf)
{
    int i = 0;
    float fRange = fMax - fMin;
    float fIntMax = (float) (powf(2, YAPP_SAMPSIZE_4) - 1.0);
    unsigned char cLo = 0;
    unsigned char cHi = 0;

    for (i = 0; i < iLen; i += 2)
    {
        cLo = (unsigned char) roundf(((pfBuf[i] - fMin) / fRange) * fIntMax);
        cHi = (unsigned char) roundf(((pfBuf[i+1] - fMin) / fRange) * fIntMax);
        pcBuf[i/2] = (cHi << 4) | cLo;
    }

    return;
}


void YAPP_Float2Byte(float *pfBuf,
                     int iLen,
                     float fMin,
                     float fMax,
                     unsigned char *pcBuf)
{
    int i = 0;
    float fRange = fMax - fMin;
    float fIntMax = (float) (powf(2, YAPP_SAMPSIZE_8) - 1.0);

    for (i = 0; i < iLen; ++i)
    {
        pcBuf[i] = (unsigned char) roundf(((pfBuf[i] - fMin) / fRange)
                                          * fIntMax);
    }

    return;
}


void YAPP_Float2Short(float *pfBuf,
                      int iLen,
                      float fMin,
                      float fMax,
                      short int *piBuf)
{
    int i = 0;
    float fRange = fMax - fMin;
    float fIntMax = (float) (powf(2, YAPP_SAMPSIZE_16) - 1.0);

    for (i = 0; i < iLen; ++i)
    {
        piBuf[i] = (short int) roundf(((pfBuf[i] - fMin) / fRange) * fIntMax);
    }

    return;
}


/*
 * Calculate statistics
 */
int YAPP_CalcStats(char *pcFileData, int iFormat, YUM_t *pstYUM)
{
    int iTotSampsPerBlock = 0;  /* iBlockSize */
    int iBlockSize = DEF_SIZE_BLOCK;
    int iNumReads = 0;
    int iReadBlockCount = 0;
    int iReadItems = 0;
    int iNumSamps = 0;
    int iDiff = 0;
    int i = 0;
    float fMean = 0.0;
    float fRMS = 0.0;
    int iNumChansBk = 0;

    /* only support .tim, .dat, and .fil files for now */
    assert((YAPP_FORMAT_DTS_TIM == iFormat)
           || (YAPP_FORMAT_DTS_DAT == iFormat)
           || (YAPP_FORMAT_FIL == iFormat));
    /* make sure the metadata has been read, as we need to skip the header */
    if (YAPP_FORMAT_DTS_TIM == iFormat)
    {
        assert(pstYUM->iHeaderLen != 0);
        iNumChansBk = pstYUM->iNumChans;
        pstYUM->iNumChans = 1;
    }

    /* open the data file for reading */
    g_pFData = fopen(pcFileData, "r");
    if (NULL == g_pFData)
    {
        (void) fprintf(stderr,
                       "ERROR: Opening file %s failed! %s.\n",
                       pcFileData,
                       strerror(errno));
        return YAPP_RET_ERROR;
    }

    iNumReads = (int) ceilf(((float) pstYUM->iTimeSamps) / iBlockSize);

    /* optimisation - store some commonly used values in variables */
    iTotSampsPerBlock = pstYUM->iNumChans * iBlockSize;

    /* allocate memory for the buffer, based on the number of channels and time
       samples */
    g_pfBuf = (float *) YAPP_Malloc((size_t) pstYUM->iNumChans * iBlockSize,
                                    sizeof(float),
                                    YAPP_FALSE);
    if (NULL == g_pfBuf)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation failed! %s!\n",
                       strerror(errno));
        /* close the file, it may be opened later for reading data */
        (void) fclose(g_pFData);
        /* set the stream pointer to NULL so that YAPP_CleanUp does not try to
           close it */
        g_pFData = NULL;
        return YAPP_RET_ERROR;
    }

    (void) fseek(g_pFData, (long) pstYUM->iHeaderLen, SEEK_SET);

    /* set minimum and maximum values */
    pstYUM->fMin = FLT_MAX;
    pstYUM->fMax = -(FLT_MAX);

    while (iNumReads > 0)
    {
        /* read data */
        iReadItems = YAPP_ReadData(g_pFData,
                                   g_pfBuf,
                                   pstYUM->fSampSize,
                                   iTotSampsPerBlock);
        if (YAPP_RET_ERROR == iReadItems)
        {
            (void) fprintf(stderr, "ERROR: Reading data failed!\n");
            /* close the file, it may be opened later for reading data */
            (void) fclose(g_pFData);
            /* set the stream pointer to NULL so that YAPP_CleanUp does not try to
               close it */
            g_pFData = NULL;
            return YAPP_RET_ERROR;
        }
        --iNumReads;
        ++iReadBlockCount;

        if (iReadItems < iTotSampsPerBlock)
        {
            iDiff = (pstYUM->iNumChans * iBlockSize) - iReadItems;

            /* reset remaining elements to '\0' */
            (void) memset((g_pfBuf + iReadItems),
                          '\0',
                          (sizeof(float) * iDiff));
        }

        /* calculate the number of time samples in the block - this may not
           be iBlockSize for the last block, and should be iBlockSize for
           all other blocks */
        iNumSamps = iReadItems / pstYUM->iNumChans;

        /* calculate statistics */
        fMean = YAPP_CalcMean(g_pfBuf, iReadItems, 0, 1);
        pstYUM->fMean += fMean;
        fRMS = YAPP_CalcRMS(g_pfBuf, iReadItems, 0, 1, fMean);
        fRMS *= fRMS;
        fRMS *= (iReadItems - 1);
        pstYUM->fRMS += fRMS;
        for (i = 0; i < iReadItems; ++i)
        {
            if (g_pfBuf[i] < pstYUM->fMin)
            {
                pstYUM->fMin = g_pfBuf[i];
            }
            if (g_pfBuf[i] > pstYUM->fMax)
            {
                pstYUM->fMax = g_pfBuf[i];
            }
        }
    }

    /* print statistics */
    pstYUM->fMean /= iReadBlockCount;
    pstYUM->fRMS /= (pstYUM->iNumChans * pstYUM->iTimeSamps - 1);
    pstYUM->fRMS = sqrtf(pstYUM->fRMS);

    /* kludgy reset of static variable cIsFirst in YAPP_ReadData() */
    /* NOTE: this could be done at the beginning of any read loop, but is done
             here to obviate adding code to all files containing read loops */
    (void) YAPP_ReadData(NULL, NULL, 0.0, 0);

    /* close the file, it may be opened later for reading data */
    (void) fclose(g_pFData);
    /* set the stream pointer to NULL so that YAPP_CleanUp does not try to
       close it */
    g_pFData = NULL;

    /* reset the number of channels */
    if (YAPP_FORMAT_DTS_TIM == iFormat)
    {
        pstYUM->iNumChans = iNumChansBk;
    }

    return YAPP_RET_SUCCESS;
}


/*
 * Calculate signal mean
 */
float YAPP_CalcMean(float *pfBuf, int iLength, int iOffset, int iStride)
{
    float fMean = 0.0;
    int i = 0;

    for (i = iOffset; i < iLength; i += iStride)
    {
        fMean += pfBuf[i];
    }
    fMean /= iLength;

    return fMean;
}


/*
 * Calculate signal standard deviation
 */
float YAPP_CalcRMS(float *pfBuf,
                   int iLength,
                   int iOffset,
                   int iStride,
                   float fMean)
{
    float fRMS = 0.0;
    int i = 0;

    for (i = iOffset; i < iLength; i += iStride)
    {
        fRMS += powf(pfBuf[i] - fMean, 2);
    }
    fRMS /= (iLength - 1);
    fRMS = sqrtf(fRMS);

    return fRMS;
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
    /* close PGPLOT device, if open */
    if (g_iPGDev > 0)
    {
        cpgslct(g_iPGDev);
        /* TODO: uncommenting this causes ^C handling to fail */
        /*cpgclos();*/
        g_iPGDev = 0;
    }
    #endif

    /* close data file, if open */
    if (g_pFData != NULL)
    {
        (void) fclose(g_pFData);
        g_pFData = NULL;
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
    (void) printf("\n");

    /* clean up */
    YAPP_CleanUp();

    /* exit */
    exit(YAPP_RET_SUCCESS);

    /* never reached */
    return;
}


/*
 * @file yapp_p2stim.c
 * Program to convert dedispersed time series data from PRESTO .dat to SIGPROC
 * .tim format.
 *
 * @verbatim
 * Usage: yapp_fold [options] <time-series-data-file>
 *     -h  --help                           Display this usage information
 *     -v  --version                        Display the version @endverbatim
 *
 * @author Jayanth Chennamangalam
 * @date 2012.12.18
 */

#include "yapp.h"
#include "yapp_p2stim.h"
#include "yapp_sigproc.h"   /* for SIGPROC filterbank file format support */

/**
 * The build version string, maintained in the file version.c, which is
 * generated by makever.c.
 */
extern const char *g_pcVersion;

int main(int argc, char *argv[])
{
    char *pcFileData = NULL;
    char *pcFilename = NULL;
    FILE *pFInf = NULL;
    FILE *pFTim = NULL;
    char acFileInf[LEN_GENSTRING] = {0};
    char acFileTim[LEN_GENSTRING] = {0};
    YAPP_SIGPROC_HEADER stHeader = {{0}};
    int iRet = EXIT_SUCCESS;

    /* get the input filename */
    pcFileData = argv[1];

    pcFilename = GetFilenameFromPath(pcFileData, ".dat");

    /* build the header file name and open the file */
    (void) strcpy(acFileInf, pcFilename);
    (void) strcat(acFileInf, ".inf");
    pFInf = fopen(acFileInf, "r");
    if (NULL == pFInf)
    {
        fprintf(stderr,
                "ERROR: Opening file %s failed! %s.\n",
                acFileInf,
                strerror(errno));
        return EXIT_FAILURE;
    }

    iRet = GetMetadata(pFInf, &stHeader);
    if (iRet != EXIT_SUCCESS)
    {
        fprintf(stderr,
                "ERROR: Reading metadata failed!\n");
        (void) fclose(pFInf);
        return EXIT_FAILURE;
    }

    /* open .tim file and write data */
    (void) strcpy(acFileTim, pcFilename);
    free(pcFilename);
    (void) strcat(acFileTim, ".tim");
    pFTim = fopen(acFileTim, "w");
    if (NULL == pFTim)
    {
        fprintf(stderr,
                "ERROR: Opening file %s failed! %s.\n",
                acFileTim,
                strerror(errno));
        (void) fclose(pFInf);
        return EXIT_FAILURE;
    }

    iRet = WriteMetadata(pFTim, stHeader);
    if (iRet != EXIT_SUCCESS)
    {
        fprintf(stderr,
                "ERROR: Writing metadata failed!\n");
        (void) fclose(pFTim);
        (void) fclose(pFInf);
        return EXIT_FAILURE;
    }
    (void) fclose(pFTim);

    /* open again for appending */
    pFTim = fopen(acFileTim, "a");
    if (NULL == pFTim)
    {
        fprintf(stderr,
                "ERROR: Opening file %s failed! %s.\n",
                acFileTim,
                strerror(errno));
        (void) fclose(pFInf);
        return EXIT_FAILURE;
    }
    iRet = CopyData(pcFileData, pFTim);
    if (iRet != EXIT_SUCCESS)
    {
        fprintf(stderr,
                "ERROR: Writing metadata failed!\n");
        (void) fclose(pFTim);
        (void) fclose(pFInf);
        return EXIT_FAILURE;
    }

    (void) fclose(pFTim);
    (void) fclose(pFInf);

    return EXIT_SUCCESS;
}

int GetMetadata(FILE *pFInf, YAPP_SIGPROC_HEADER *pstHeader)
{
    size_t iLen = 0;
    char *pcVal = NULL;
    char* pcLine = NULL;

    /* read data file name and ignore it */
    (void) getline(&pcLine, &iLen, pFInf); 
    pcVal = strrchr(pcLine, '=');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return EXIT_FAILURE;
    }

    /* read telescope name and ignore it */
    (void) getline(&pcLine, &iLen, pFInf); 
    pcVal = strrchr(pcLine, '=');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return EXIT_FAILURE;
    }

    /* read backend name and ignore it */
    (void) getline(&pcLine, &iLen, pFInf); 
    pcVal = strrchr(pcLine, '=');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return EXIT_FAILURE;
    }

    /* read source name */
    (void) getline(&pcLine, &iLen, pFInf); 
    pcVal = strrchr(pcLine, '=');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return EXIT_FAILURE;
    }
    (void) sscanf(pcVal, "=  %s", pstHeader->acPulsar);

    /* read RA */
    (void) getline(&pcLine, &iLen, pFInf); 
    pcVal = strrchr(pcLine, '=');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return EXIT_FAILURE;
    }
    /* TODO: compute numeric value for RA */

    /* read declination */
    (void) getline(&pcLine, &iLen, pFInf); 
    pcVal = strrchr(pcLine, '=');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return EXIT_FAILURE;
    }
    /* TODO: compute numeric value for dec. */

    /* read observer name and ignore it */
    (void) getline(&pcLine, &iLen, pFInf); 
    pcVal = strrchr(pcLine, '=');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return EXIT_FAILURE;
    }

    /* read epoch */
    (void) getline(&pcLine, &iLen, pFInf); 
    pcVal = strrchr(pcLine, '=');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return EXIT_FAILURE;
    }
    (void) sscanf(pcVal, "=  %lf", &pstHeader->dTStart);

    /* read barycentric flag */
    (void) getline(&pcLine, &iLen, pFInf); 
    pcVal = strrchr(pcLine, '=');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return EXIT_FAILURE;
    }
    (void) sscanf(pcVal, "=  %d", &pstHeader->iFlagBary);

    /* read sample count and ignore it */
    (void) getline(&pcLine, &iLen, pFInf); 
    pcVal = strrchr(pcLine, '=');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return EXIT_FAILURE;
    }

    /* read sampling interval */
    (void) getline(&pcLine, &iLen, pFInf); 
    pcVal = strrchr(pcLine, '=');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return EXIT_FAILURE;
    }
    (void) sscanf(pcVal, "=  %lf", &pstHeader->dTSamp);

    /* read break status ignore it */
    (void) getline(&pcLine, &iLen, pFInf); 
    pcVal = strrchr(pcLine, '=');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return EXIT_FAILURE;
    }

    /* read observation type and ignore it */
    (void) getline(&pcLine, &iLen, pFInf); 
    pcVal = strrchr(pcLine, '=');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return EXIT_FAILURE;
    }

    /* read beam diameter and ignore it */
    (void) getline(&pcLine, &iLen, pFInf); 
    pcVal = strrchr(pcLine, '=');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return EXIT_FAILURE;
    }

    /* read DM */
    (void) getline(&pcLine, &iLen, pFInf); 
    pcVal = strrchr(pcLine, '=');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return EXIT_FAILURE;
    }
    (void) sscanf(pcVal, "=  %lf", &pstHeader->dDM);

    /* read centre frequency */
    (void) getline(&pcLine, &iLen, pFInf); 
    pcVal = strrchr(pcLine, '=');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return EXIT_FAILURE;
    }
    /* TODO: compute frequency of first channel */

    /* read total bandwidth and ignore it */
    (void) getline(&pcLine, &iLen, pFInf); 
    pcVal = strrchr(pcLine, '=');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return EXIT_FAILURE;
    }

    /* read number of channels */
    (void) getline(&pcLine, &iLen, pFInf); 
    pcVal = strrchr(pcLine, '=');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return EXIT_FAILURE;
    }
    (void) sscanf(pcVal, "=  %d", &pstHeader->iNumChans);

    /* read channel bandwidth and ignore it */
    (void) getline(&pcLine, &iLen, pFInf); 
    pcVal = strrchr(pcLine, '=');
    if (NULL == pcVal)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading header file failed!\n");
        return EXIT_FAILURE;
    }

    /* fill in other fields */
    pstHeader->iObsID = 6;      /* GBT */
    pstHeader->iDataTypeID = 2; /* time series */
    pstHeader->iNumBits = 32;   /* single-precision floating-point */

    free(pcLine);

    return EXIT_SUCCESS;
}

int WriteMetadata(FILE *pFTim, YAPP_SIGPROC_HEADER stHeader)
{
    char acLabel[LEN_GENSTRING] = {0};
    int iLen = 0;

    /* write the parameters to the header section of the file */
    /* start with the 'HEADER_START' label */
    iLen = strlen("HEADER_START");
    (void) fwrite(&iLen, sizeof(iLen), 1, pFTim);
    (void) strcpy(acLabel, "HEADER_START");
    (void) fwrite(acLabel, sizeof(char), iLen, pFTim);

    /* write the rest of the header */
    /* write source name */
    /* write field label length */
    iLen = strlen("source_name");
    (void) fwrite(&iLen, sizeof(iLen), 1, pFTim);
    /* write field label */
    (void) strcpy(acLabel, "source_name");
    (void) fwrite(acLabel, sizeof(char), iLen, pFTim);
    iLen = strlen(stHeader.acPulsar);
    (void) fwrite(&iLen, sizeof(iLen), 1, pFTim);
    (void) fwrite(stHeader.acPulsar, sizeof(char), iLen, pFTim);

    /* write data type */
    iLen = strlen("data_type");
    (void) fwrite(&iLen, sizeof(iLen), 1, pFTim);
    (void) strcpy(acLabel, "data_type");
    (void) fwrite(acLabel, sizeof(char), iLen, pFTim);
    (void) fwrite(&stHeader.iDataTypeID,
                  sizeof(stHeader.iDataTypeID),
                  1,
                  pFTim);

    //TODO: check if we need this
    /* write number of channels */
    iLen = strlen("nchans");
    (void) fwrite(&iLen, sizeof(iLen), 1, pFTim);
    (void) strcpy(acLabel, "nchans");
    (void) fwrite(acLabel, sizeof(char), iLen, pFTim);
    (void) fwrite(&stHeader.iNumChans,
                  sizeof(stHeader.iNumChans),
                  1,
                  pFTim);

    //TODO: check if we need this
    /* write frequency of first channel */
    iLen = strlen("fch1");
    (void) fwrite(&iLen, sizeof(iLen), 1, pFTim);
    (void) strcpy(acLabel, "fch1");
    (void) fwrite(acLabel, sizeof(char), iLen, pFTim);
    (void) fwrite(&stHeader.dFChan1,
                  sizeof(stHeader.dFChan1),
                  1,
                  pFTim);

    /* write number of bits per sample */
    iLen = strlen("nbits");
    (void) fwrite(&iLen, sizeof(iLen), 1, pFTim);
    (void) strcpy(acLabel, "nbits");
    (void) fwrite(acLabel, sizeof(char), iLen, pFTim);
    (void) fwrite(&stHeader.iNumBits,
                  sizeof(stHeader.iNumBits),
                  1,
                  pFTim);

    /* write number of IFs */
    iLen = strlen("nifs");
    (void) fwrite(&iLen, sizeof(iLen), 1, pFTim);
    (void) strcpy(acLabel, "nifs");
    (void) fwrite(acLabel, sizeof(char), iLen, pFTim);
    (void) fwrite(&stHeader.iNumIFs,
                  sizeof(stHeader.iNumIFs),
                  1,
                  pFTim);

    /* write sampling time in seconds */
    iLen = strlen("tsamp");
    (void) fwrite(&iLen, sizeof(iLen), 1, pFTim);
    (void) strcpy(acLabel, "tsamp");
    (void) fwrite(acLabel, sizeof(char), iLen, pFTim);
    (void) fwrite(&stHeader.dTSamp,
                  sizeof(stHeader.dTSamp),
                  1,
                  pFTim);

    /* write timestamp of first sample (MJD) */
    iLen = strlen("tstart");
    (void) fwrite(&iLen, sizeof(iLen), 1, pFTim);
    (void) strcpy(acLabel, "tstart");
    (void) fwrite(acLabel, sizeof(char), iLen, pFTim);
    (void) fwrite(&stHeader.dTStart,
                  sizeof(stHeader.dTStart),
                  1,
                  pFTim);

    /* write telescope ID */
    iLen = strlen("telescope_id");
    (void) fwrite(&iLen, sizeof(iLen), 1, pFTim);
    (void) strcpy(acLabel, "telescope_id");
    (void) fwrite(acLabel, sizeof(char), iLen, pFTim);
    (void) fwrite(&stHeader.iObsID,
                  sizeof(stHeader.iObsID),
                  1,
                  pFTim);

    /* write backend ID */
    iLen = strlen("machine_id");
    (void) fwrite(&iLen, sizeof(iLen), 1, pFTim);
    (void) strcpy(acLabel, "machine_id");
    (void) fwrite(acLabel, sizeof(char), iLen, pFTim);
    (void) fwrite(&stHeader.iBackendID,
                  sizeof(stHeader.iBackendID),
                  1,
                  pFTim);

    /* write source RA (J2000) */
    iLen = strlen("src_raj");
    (void) fwrite(&iLen, sizeof(iLen), 1, pFTim);
    (void) strcpy(acLabel, "src_raj");
    (void) fwrite(acLabel, sizeof(char), iLen, pFTim);
    (void) fwrite(&stHeader.dSourceRA,
                  sizeof(stHeader.dSourceRA),
                  1,
                  pFTim);

    /* write source declination (J2000) */
    iLen = strlen("src_dej");
    (void) fwrite(&iLen, sizeof(iLen), 1, pFTim);
    (void) strcpy(acLabel, "src_dej");
    (void) fwrite(acLabel, sizeof(char), iLen, pFTim);
    (void) fwrite(&stHeader.dSourceDec,
                  sizeof(stHeader.dSourceDec), 1, pFTim);

    /* write azimuth start */
    iLen = strlen("az_start");
    (void) fwrite(&iLen, sizeof(iLen), 1, pFTim);
    (void) strcpy(acLabel, "az_start");
    (void) fwrite(acLabel, sizeof(char), iLen, pFTim);
    (void) fwrite(&stHeader.dAzStart,
                  sizeof(stHeader.dAzStart),
                  1,
                  pFTim);

    /* write ZA start */
    iLen = strlen("za_start");
    (void) fwrite(&iLen, sizeof(iLen), 1, pFTim);
    (void) strcpy(acLabel, "za_start");
    (void) fwrite(acLabel, sizeof(char), iLen, pFTim);
    (void) fwrite(&stHeader.dZAStart,
                  sizeof(stHeader.dZAStart),
                  1,
                  pFTim);

    //TODO: check if we need this
    /* write reference DM */
    iLen = strlen("refdm");
    (void) fwrite(&iLen, sizeof(iLen), 1, pFTim);
    (void) strcpy(acLabel, "refdm");
    (void) fwrite(acLabel, sizeof(char), iLen, pFTim);
    (void) fwrite(&stHeader.dDM, sizeof(stHeader.dDM), 1, pFTim);

    /* write barycentric flag */
    iLen = strlen("barycentric");
    (void) fwrite(&iLen, sizeof(iLen), 1, pFTim);
    (void) strcpy(acLabel, "barycentric");
    (void) fwrite(acLabel, sizeof(char), iLen, pFTim);
    (void) fwrite(&stHeader.iFlagBary,
                  sizeof(stHeader.iFlagBary),
                  1,
                  pFTim);

    /* write header end tag */
    iLen = strlen("HEADER_END");
    (void) fwrite(&iLen, sizeof(iLen), 1, pFTim);
    (void) strcpy(acLabel, "HEADER_END");
    (void) fwrite(acLabel, sizeof(char), iLen, pFTim);

    return EXIT_SUCCESS;
}

int CopyData(char *pcFileData, FILE *pFTim)
{
    FILE *pFData = NULL;
    struct stat stFileStats = {0};
    off_t lByteCount = 0;
    char *pcBuf = NULL;
    int iRet = EXIT_SUCCESS;
    int iWrit = 0;

    /* get the file size */
    iRet = stat(pcFileData, &stFileStats);
    if (iRet != EXIT_SUCCESS)
    {
        (void) fprintf(stderr,
                       "ERROR: Failed to stat %s: %s!\n",
                       pcFileData,
                       strerror(errno));
        (void) fclose(pFData);
        return EXIT_FAILURE;
    }

    /* open the file */
    pFData = fopen(pcFileData, "r");
    if (NULL == pFData)
    {
        fprintf(stderr,
                "ERROR: Opening file %s failed! %s.\n",
                pcFileData,
                strerror(errno));
        return EXIT_FAILURE;
    }

    /* allocate memory for the buffer */
    pcBuf = (char *) malloc(SIZE_BUF);
    if (NULL == pcBuf)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation for buffer failed! %s!\n",
                       strerror(errno));
        (void) fclose(pFData);
        return EXIT_FAILURE;
    }

    /* copy data */
    do
    {
        iRet = fread(pcBuf, 1, SIZE_BUF, pFData);
        lByteCount += iRet;
        iWrit = fwrite(pcBuf, 1, iRet, pFTim);
    }
    while (SIZE_BUF == iRet);

    free(pcBuf);

    /* check if all data has been copied */
    if (lByteCount != stFileStats.st_size)
    {
        (void) fprintf(stderr, "ERROR: Data copy failed!\n");
        printf("%ld, %ld\n", lByteCount, stFileStats.st_size);
        (void) fclose(pFData);
        return EXIT_FAILURE;
    }

    (void) fclose(pFData);

    return EXIT_SUCCESS;
}

char* GetFilenameFromPath(char *pcPath, char *pcExt)
{
    char *pcPos = NULL;
    char *pcPosPrev = NULL;
    char *pcFilename = NULL;
    int iSlashCount = 0;

    /* extract the non-extension part of the input file */
    pcPos = pcPath;
#if 0
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
#endif

    /* allocate memory for the filename */
    pcFilename = (char *) malloc((strlen(pcPos) - strlen(pcExt) + 1) * sizeof(char));
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


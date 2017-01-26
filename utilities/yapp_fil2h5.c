/*
 * @file yapp_fil2h5.c
 * Program to convert SIGPROC filterbank .fil format to HDF5 format.
 *
 * @verbatim
 * Usage: yapp_fil2h5 [options] <data-file>
 *     -h  --help                           Display this usage information
 *     -v  --version                        Display the version @endverbatim
 *
 * @author Jayanth Chennamangalam
 * @date 2017.01.24
 */

#include "yapp.h"
#include "yapp_sigproc.h"
#include "yapp_hdf5.h"
#include "yapp_fil2h5.h"
#include <hdf5.h>

/**
 * The build version string, maintained in the file version.c, which is
 * generated by makever.c.
 */
extern const char *g_pcVersion;

int main(int argc, char *argv[])
{
    char *pcFileData = NULL;
    char *pcFilename = NULL;
    hid_t hFile = 0;
    char acFileH5[LEN_GENSTRING] = {0};
    int iFormat = DEF_FORMAT;
    YUM_t stYUM = {{0}};
    int iRet = EXIT_SUCCESS;

    const char *pcProgName = NULL;
    int iNextOpt = 0;
    /* valid short options */
    const char* const pcOptsShort = "hv";
    /* valid long options */
    const struct option stOptsLong[] = {
        { "help",                   0, NULL, 'h' },
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
    if (iFormat != YAPP_FORMAT_FIL)
    {
        (void) fprintf(stderr,
                       "ERROR: Invalid file type!\n");
        return YAPP_RET_ERROR;
    }

    iRet = YAPP_ReadMetadata(pcFileData, iFormat, &stYUM);
    if (iRet != EXIT_SUCCESS)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading metadata failed!\n");
        return EXIT_FAILURE;
    }
#if 0
    /* current support only for single-precision floating point values */
    if (stYUM.iNumBits != YAPP_SAMPSIZE_32)
    {
        (void) fprintf(stderr,
                       "ERROR: Unsupported data type! %s only supports "
                       "single-precision floating point, while input data is "
                       "%d bits.\n",
                       pcProgName,
                       stYUM.iNumBits);
        return EXIT_FAILURE;
    }
#endif

    pcFilename = YAPP_GetFilenameFromPath(pcFileData);
    (void) strcpy(acFileH5, pcFilename);
    (void) strcat(acFileH5, EXT_HDF5);
    iRet = YAPP_WriteMetadata(acFileH5, YAPP_FORMAT_HDF5, stYUM);
    if (iRet != EXIT_SUCCESS)
    {
        fprintf(stderr,
                "ERROR: Writing metadata failed!\n");
        return EXIT_FAILURE;
    }

    /* open .h5 file and write data */
    hFile = H5Fopen(acFileH5, H5F_ACC_RDWR, H5P_DEFAULT);
    if (hFile < 0)
    {
        fprintf(stderr,
                "ERROR: Opening file %s failed! %s.\n",
                acFileH5,
                strerror(errno));
        return EXIT_FAILURE;
    }

    iRet = YAPP_CopyData(pcFileData,
                         stYUM.iHeaderLen,
                         hFile,
                         stYUM.iNumChans,
                         stYUM.iTimeSamps);
    if (iRet != EXIT_SUCCESS)
    {
        fprintf(stderr,
                "ERROR: Writing data failed!\n");
        (void) H5Fclose(hFile);
        return EXIT_FAILURE;
    }

    (void) H5Fclose(hFile);

    return EXIT_SUCCESS;
}


int YAPP_CopyData(char *pcFileData,
                  int iOffset,
                  hid_t hFile,
                  int iNumChans,
                  int iTimeSamps)
{
    FILE *pFData = NULL;
    struct stat stFileStats = {0};
    off_t lByteCount = 0;
    char *pcBuf = NULL;
    hid_t hGroup = 0;
    hid_t hDataspace = 0;
    hid_t hMemDataspace = 0;
    hid_t hDataset = 0;
    hsize_t hMemDims[YAPP_HDF5_DYNSPEC_RANK] = {0};
    hsize_t hOffset[YAPP_HDF5_DYNSPEC_RANK] = {0};
    hsize_t hStride[YAPP_HDF5_DYNSPEC_RANK] = {0};
    hsize_t hCount[YAPP_HDF5_DYNSPEC_RANK] = {0};
    hsize_t hBlock[YAPP_HDF5_DYNSPEC_RANK] = {0};
    herr_t hStatus = 0;
    int iRet = EXIT_SUCCESS;

    /* get the file size */
    iRet = stat(pcFileData, &stFileStats);
    if (iRet != EXIT_SUCCESS)
    {
        (void) fprintf(stderr,
                       "ERROR: Failed to stat %s: %s!\n",
                       pcFileData,
                       strerror(errno));
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

    /* skip header */
    fseek(pFData, iOffset, SEEK_SET);

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

    /* open the group */
    hGroup = H5Gopen(hFile, YAPP_HDF5_DYNSPEC_GROUP, H5P_DEFAULT);

    /* open the dataset */
    hDataset = H5Dopen(hGroup,
                       YAPP_HDF5_DYNSPEC_GROUP YAPP_HDF5_DYNSPEC_DATASET,
                       H5P_DEFAULT);
    /* get the dataspace in which this dataset exists */
    hDataspace = H5Dget_space(hDataset);

    /* create a memory dataspace */
    hMemDims[0] = iNumChans;
    //TODO: make this error-proof
    hMemDims[1] = SIZE_BUF / iNumChans;
    hMemDataspace = H5Screate_simple(YAPP_HDF5_DYNSPEC_RANK, hMemDims, NULL);

    /* define stride, count, and block */
    hOffset[0] = 0;
    hOffset[1] = 0;
    hStride[0] = 1;
    hStride[1] = 1;
    hCount[0] = iNumChans;
    hCount[1] = SIZE_BUF / iNumChans;
    hBlock[0] = 1;
    hBlock[1] = 1;

    /* copy data */
    do
    {
        iRet = fread(pcBuf, 1, SIZE_BUF, pFData);
        lByteCount += iRet;
        //printf("````````%d, %d, %ld\n", iRet, iRet / iNumChans, lByteCount);
        hStatus = H5Sselect_hyperslab(hDataspace, H5S_SELECT_SET, hOffset,
                                                  hStride, hCount, hBlock);
        hStatus = H5Dwrite(hDataset,
                           H5T_STD_I8LE,
                           hMemDataspace,
                           hDataspace,
                           H5P_DEFAULT,
                           pcBuf);

        /* set offset for next copy*/
        hOffset[1] += hCount[1];
        //printf("------%d, %d\n", hOffset[1], hOffset[1] + hCount[1]);
        if (hOffset[1] + hCount[1] > iTimeSamps) {
            hCount[1] = iTimeSamps - hOffset[1];
            //printf("========%d, %d\n", hOffset[1], hOffset[1] + hCount[1]);
            hMemDims[1] = hCount[1];
            (void) H5Sclose(hMemDataspace);
            hMemDataspace = H5Screate_simple(YAPP_HDF5_DYNSPEC_RANK, hMemDims, NULL);
        }

    }
    while (SIZE_BUF == iRet);

    /* close HDF5 resources */
    (void) H5Sclose(hMemDataspace);
    (void) H5Dclose(hDataset);
    (void) H5Sclose(hDataspace);
    (void) H5Gclose(hGroup);

    free(pcBuf);

    /* check if all data has been copied */
    if (lByteCount != (stFileStats.st_size - iOffset))
    {
        (void) fprintf(stderr, "ERROR: Data copy failed!\n");
        printf("%ld, %ld\n", lByteCount, stFileStats.st_size);
        (void) fclose(pFData);
        return EXIT_FAILURE;
    }

    (void) fclose(pFData);

    return EXIT_SUCCESS;
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
    (void) printf("    -v  --version                        ");
    (void) printf("Display the version\n");

    return;
}


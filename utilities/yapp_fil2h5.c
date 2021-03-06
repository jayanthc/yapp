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
    hid_t hType = 0;
    char acFileH5[LEN_GENSTRING] = {0};
    int iFormat = DEF_FORMAT;
    YUM_t stYUM = {{0}};
    long int lChunkDimX = 0;
    long int lChunkDimY = 0;
    int iRet = YAPP_RET_SUCCESS;
    const char *pcProgName = NULL;
    int iNextOpt = 0;
    /* valid short options */
    const char* const pcOptsShort = "hvx:y:";
    /* valid long options */
    const struct option stOptsLong[] = {
        { "help",                   0, NULL, 'h' },
        { "version",                0, NULL, 'v' },
        { "chunk-dim-x",            1, NULL, 'x' },
        { "chunk-dim-y",            1, NULL, 'y' },
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

            case 'x':   /* -x or --chunk-dim-x */
                /* set option */
                lChunkDimX = atol(optarg);
                break;
            case 'y':   /* -y or --chunk-dim-y */
                /* set option */
                lChunkDimY = atol(optarg);
                break;

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
    if (iRet != YAPP_RET_SUCCESS)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading metadata failed!\n");
        return YAPP_RET_ERROR;
    }
    /* current support only for 8-bit integer, 16-bit integer, and 32-bit
       single-precision floating point values */
    if (!((YAPP_SAMPSIZE_8 == stYUM.iNumBits)
          || (YAPP_SAMPSIZE_16 == stYUM.iNumBits)
          || (YAPP_SAMPSIZE_32 == stYUM.iNumBits)))
    {
        (void) fprintf(stderr,
                       "ERROR: Unsupported data type! %s only supports "
                       "8-bit ints, 16-bit ints, and 32-bit floats, "
                       "while input data is %d bits.\n",
                       pcProgName,
                       stYUM.iNumBits);
        return YAPP_RET_ERROR;
    }

    /* set up the chunking scheme */
    if (lChunkDimX > stYUM.iNumChans)
    {
        (void) fprintf(stderr,
                       "ERROR: Chunk X-dimension %ld not less than number of "
                       "channels %d!\n",
                       lChunkDimX,
                       stYUM.iNumChans);
        return YAPP_RET_ERROR;
    }
    else if (0 == lChunkDimX)
    {
        (void) fprintf(stderr,
                       "WARNING: Chunk X-dimension not provided, using %d\n",
                       stYUM.iNumChans);
        lChunkDimX = stYUM.iNumChans;
    }
    if (0 == lChunkDimY)
    {
        (void) fprintf(stderr,
                       "WARNING: Chunk Y-dimension not provided, using %d\n",
                       YAPP_HDF5_CHUNK_DIM_Y);
        lChunkDimY = YAPP_HDF5_CHUNK_DIM_Y;
    }
    stYUM.lChunkDims[0] = lChunkDimY;
    stYUM.lChunkDims[1] = lChunkDimX;

    pcFilename = YAPP_GetFilenameFromPath(pcFileData);
    (void) strcpy(acFileH5, pcFilename);
    (void) strcat(acFileH5, EXT_HDF5);
    iRet = YAPP_WriteMetadata(acFileH5, YAPP_FORMAT_HDF5, stYUM);
    if (iRet != YAPP_RET_SUCCESS)
    {
        fprintf(stderr,
                "ERROR: Writing metadata failed!\n");
        return YAPP_RET_ERROR;
    }

    /* open .h5 file and write data */
    hFile = H5Fopen(acFileH5, H5F_ACC_RDWR, H5P_DEFAULT);
    if (hFile < 0)
    {
        fprintf(stderr,
                "ERROR: Opening file %s failed! %s.\n",
                acFileH5,
                strerror(errno));
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

    iRet = YAPP_CopyData(pcFileData,
                         stYUM.iHeaderLen,
                         hFile,
                         stYUM.iNumChans,
                         stYUM.iTimeSamps,
                         stYUM.iNumBits,
                         hType);
    if (iRet != YAPP_RET_SUCCESS)
    {
        fprintf(stderr,
                "ERROR: Copying data failed!\n");
        (void) H5Fclose(hFile);
        return YAPP_RET_ERROR;
    }

    (void) H5Fclose(hFile);

    return YAPP_RET_SUCCESS;
}


int YAPP_CopyData(char *pcFileData,
                  int iOffset,
                  hid_t hFile,
                  int iNumChans,
                  int iTimeSamps,
                  int iNumBits,
                  hid_t hType)
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
    int iRet = YAPP_RET_SUCCESS;

    /* get the file size */
    iRet = stat(pcFileData, &stFileStats);
    if (iRet != YAPP_RET_SUCCESS)
    {
        (void) fprintf(stderr,
                       "ERROR: Failed to stat %s: %s!\n",
                       pcFileData,
                       strerror(errno));
        return YAPP_RET_ERROR;
    }

    /* open the file */
    pFData = fopen(pcFileData, "r");
    if (NULL == pFData)
    {
        fprintf(stderr,
                "ERROR: Opening file %s failed! %s.\n",
                pcFileData,
                strerror(errno));
        return YAPP_RET_ERROR;
    }

    /* skip header */
    fseek(pFData, iOffset, SEEK_SET);

    /* allocate memory for the buffer */
    pcBuf = (char *) YAPP_Malloc(SIZE_BUF
                                  * ((float) iNumBits) / YAPP_BYTE2BIT_FACTOR,
                                 sizeof(char),
                                 YAPP_FALSE);
    if (NULL == pcBuf)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation for buffer failed! %s!\n",
                       strerror(errno));
        (void) fclose(pFData);
        return YAPP_RET_ERROR;
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
    hMemDims[1] = iNumChans;
    if (iTimeSamps < SIZE_BUF / iNumChans)
    {
        /* there is only one read */
        hMemDims[0] = iTimeSamps;
    }
    else
    {
        hMemDims[0] = SIZE_BUF / iNumChans;
    }
    hMemDataspace = H5Screate_simple(YAPP_HDF5_DYNSPEC_RANK, hMemDims, NULL);

    /* define stride, count, and block */
    hOffset[0] = 0;
    hOffset[1] = 0;
    hStride[0] = 1;
    hStride[1] = 1;
    hCount[1] = iNumChans;
    if (iTimeSamps < SIZE_BUF / iNumChans)
    {
        /* there is only one read */
        hCount[0] = iTimeSamps;
    }
    else
    {
        hCount[0] = SIZE_BUF / iNumChans;
    }
    hBlock[0] = 1;
    hBlock[1] = 1;

    /* copy data */
    do
    {
        iRet = fread(pcBuf,
                     sizeof(char),
                     SIZE_BUF * (((float) iNumBits) / YAPP_BYTE2BIT_FACTOR),
                     pFData);
        lByteCount += iRet;
        if (0 == iRet)
        {
            break;
        }
        hStatus = H5Sselect_hyperslab(hDataspace,
                                      H5S_SELECT_SET,
                                      hOffset,
                                      hStride,
                                      hCount,
                                      hBlock);
        if (hStatus < 0)
        {
            (void) fprintf(stderr,
                           "ERROR: Selecting hyperslab failed!\n");
            return YAPP_RET_ERROR;
        }
        hStatus = H5Dwrite(hDataset,
                           hType,
                           hMemDataspace,
                           hDataspace,
                           H5P_DEFAULT,
                           (const void *) pcBuf);
        if (hStatus < 0)
        {
            (void) fprintf(stderr,
                           "ERROR: Writing data failed!\n");
            return YAPP_RET_ERROR;
        }

        /* set offset for next copy*/
        hOffset[0] += hCount[0];
        if (hOffset[0] + hCount[0] > iTimeSamps) {
            hCount[0] = iTimeSamps - hOffset[0];
            hMemDims[0] = hCount[0];
            (void) H5Sclose(hMemDataspace);
            hMemDataspace = H5Screate_simple(YAPP_HDF5_DYNSPEC_RANK,
                                             hMemDims,
                                             NULL);
        }

    }
    while (SIZE_BUF * (((float) iNumBits) / YAPP_BYTE2BIT_FACTOR) == iRet);

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
        (void) fclose(pFData);
        return YAPP_RET_ERROR;
    }

    (void) fclose(pFData);

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
    (void) printf("    -x  --chunk-dim-x                    ");
    (void) printf("Chunk X-dimension\n");
    (void) printf("    -y  --chunk-dim-y                    ");
    (void) printf("Chunk Y-dimension\n");
    (void) printf("    -v  --version                        ");
    (void) printf("Display the version\n");

    return;
}


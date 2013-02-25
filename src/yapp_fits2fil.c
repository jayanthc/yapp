/*
 * @file yapp_fits2fil.c
 * Program to convert PSRFITS to filterbank format.
 *
 * @verbatim
 * Usage: yapp_fits2fil [options] <data-file>
 *     -h  --help                           Display this usage information
 *     -n  --nfiles <nfiles>                Number of files in the sequence to
 *                                          be converted
 *                                          (default is all)
 *     -v  --version                        Display the version @endverbatim
 *
 * @author Jayanth Chennamangalam
 * @date 2012.02.25
 */

#include "yapp.h"
#include "yapp_sigproc.h"   /* for SIGPROC filterbank file format support */

/**
 * The build version string, maintained in the file version.c, which is
 * generated by makever.c.
 */
extern const char *g_pcVersion;

/* data file */
extern FILE *g_pFSpec;

/* the following are global only to enable cleaning up in case of abnormal
   termination, such as those triggered by SIGINT or SIGTERM */
char *g_pcIsTimeGood = NULL;
float *g_pfBuf = NULL;

int main(int argc, char *argv[])
{
    char *pcFileSpec = NULL;
    int iFormat = DEF_FORMAT;
    YUM_t stYUM = {{0}};
    int iBlockSize = DEF_SIZE_BLOCK;
    int iTotSampsPerBlock = 0;  /* iNumChans * iBlockSize */
    int iDataSizePerBlock = 0;  /* fSampSize * iNumChans * iBlockSize */
    double dTSampInSec = 0.0;   /* holds sampling time in s */
    int iNumReads = 0;
    int iTotNumReads = 0;
    int iReadBlockCount = 0;
    char cIsLastBlock = YAPP_FALSE;
    int iRet = YAPP_RET_SUCCESS;
    char cIsFirst = YAPP_TRUE;
    int iReadItems = 0;
    int iNumSamps = 0;
    int iDiff = 0;
    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;
    int m = 0;
    const char *pcProgName = NULL;
    int iNextOpt = 0;
    /* valid short options */
    const char* const pcOptsShort = "hs:p:n:c:m:iev";
    /* valid long options */
    const struct option stOptsLong[] = {
        { "help",                   0, NULL, 'h' },
        { "nfiles",                 1, NULL, 'n' },
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

            case 'n':   /* -n or --nfiles */
                /* set option */
                iNumFiles = atoi(optarg);
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
    if (!((YAPP_FORMAT_FIL == iFormat)
          || (YAPP_FORMAT_SPEC == iFormat)
          || (YAPP_FORMAT_DTS_TIM == iFormat)))
    {
        (void) fprintf(stderr,
                       "ERROR: Invalid file type!\n");
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
    (void) printf("    -n  --nfiles <nfiles>                ");
    (void) printf("Number of files in the sequence to be\n"
    (void) printf("                                         ");
    (void) printf("converted\n");
    (void) printf("                                         ");
    (void) printf("(default is all)\n");
    (void) printf("    -v  --version                        ");
    (void) printf("Display the version\n");

    return;
}


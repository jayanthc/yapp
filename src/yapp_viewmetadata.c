/**
 * @file yapp_viewmetadata.c
 * Reads the config file/header for a given dynamic spectrum data and prints
 * relevant configuration information
 *
 * @verbatim
 * Usage: yapp_viewmetadata [options] <data-file>
 *     -h  --help                           Display this usage information
 *     -v  --version                        Display the version @endverbatim
 *
 * @author Jayanth Chennamangalam
 * @date 2009.06.25
 */

#include "yapp.h"
#include "yapp_sigproc.h"   /* for SIGPROC filterbank file format support */

/**
 * The build version string, maintained in the file version.c, which is
 * generated by makever.c.
 */
extern const char *g_pcVersion;

int main(int argc, char *argv[])
{
    char *pcFileSpec = NULL;
    int iFormat = DEF_FORMAT;
    int iRet = YAPP_RET_SUCCESS;
    YUM_t stYUM = {{0}};
    char acExt[LEN_GENSTRING]  = {0};
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

    /* handle expanded wildcards */
    iNextOpt = optind;
    while ((argc - iNextOpt) != 0)
    {
        /* get the input filename */
        pcFileSpec = argv[iNextOpt];

        if ((argc - optind) > 1)    /* more than one input file */
        {
            (void) printf("File: %s\n", pcFileSpec);
        }

        /* determine the file type */
        iFormat = YAPP_GetFileType(pcFileSpec);
        if (YAPP_RET_ERROR == iFormat)
        {
            (void) fprintf(stderr,
                           "ERROR: File type determination failed!\n");
            return YAPP_RET_ERROR;
        }
        if (!((YAPP_FORMAT_PSRFITS == iFormat)
              || (YAPP_FORMAT_FIL == iFormat)
              || (YAPP_FORMAT_SPEC == iFormat)
              || (YAPP_FORMAT_DTS_TIM == iFormat)
              || (YAPP_FORMAT_DTS_DAT == iFormat)))
        {
            (void) fprintf(stderr,
                           "ERROR: Invalid file type!\n");
            return YAPP_RET_ERROR;
        }
        iRet = YAPP_GetExtFromFormat(iFormat, acExt);  
        if (iRet != YAPP_RET_SUCCESS)
        {
            (void) fprintf(stderr,
                           "ERROR: Getting extension failed for file %s!\n",
                           pcFileSpec);
            ++iNextOpt;
            continue;
        }

        iRet = YAPP_ReadMetadata(pcFileSpec, iFormat, &stYUM);
        if (iRet != YAPP_RET_SUCCESS)
        {
            (void) fprintf(stderr,
                           "ERROR: Reading metadata failed for file %s!\n",
                           pcFileSpec);
            ++iNextOpt;
            continue;
        }

        (void) printf("File format                       : %s\n", acExt);
        (void) printf("Observing site                    : %s\n",
                      stYUM.acSite);
        (void) printf("Field name                        : %s\n",
                      stYUM.acPulsar);
        (void) printf("Right Ascension                   : %g degrees\n",
                      stYUM.dSourceRA);
        (void) printf("Declination                       : %g degrees\n",
                      stYUM.dSourceDec);
        (void) printf("Start time                        : %.15g MJD\n",
                      stYUM.dTStart);
        (void) printf("Centre frequency                  : %.10g MHz\n",
                      stYUM.fFCentre);
        (void) printf("Bandwidth                         : %.10g MHz\n",
                      stYUM.fBW);
        (void) printf("Sampling interval                 : %.10g ms\n",
                      stYUM.dTSamp);
        (void) printf("Number of channels                : %d\n",
                      stYUM.iNumChans);
        (void) printf("Number of good channels           : %d\n",
                      stYUM.iNumGoodChans);
        (void) printf("Channel bandwidth                 : %.10g MHz\n",
                      stYUM.fChanBW);
        (void) printf("Lowest frequency                  : %.10g MHz\n",
                      stYUM.fFMin);
        (void) printf("Highest frequency                 : %.10g MHz\n",
                      stYUM.fFMax);
        if ((YAPP_FORMAT_SPEC == iFormat)
            || (YAPP_FORMAT_FIL == iFormat)
            || (YAPP_FORMAT_PSRFITS == iFormat))
        {
            (void) printf("Flipped band                      : %s\n",
                           stYUM.cIsBandFlipped ? "Yes" : "No");
            (void) printf("Estimated number of bands         : %d\n",
                          stYUM.iNumBands);
        }
        if (stYUM.iBFTimeSects != 0)    /* no beam-flip information */
        {
            (void) printf("First beam-flip time              : %.10g s\n",
                          stYUM.dTNextBF);
            (void) printf("Beam-flip interval                : %.10g s\n",
                          stYUM.dTBFInt);
            (void) printf("Number of beam-flip time sections : %d\n",
                          stYUM.iBFTimeSects);
        }
        (void) printf("Number of bad time sections       : %d\n",
                      stYUM.iNumBadTimes);
        (void) printf("Number of bits per sample         : %d\n",
                      stYUM.iNumBits);
        if (stYUM.iNumIFs != 0)
        {
            (void) printf("Number of IFs                     : %d\n",
                          stYUM.iNumIFs);
        }
        if (stYUM.iNumPol != 0)
        {
            (void) printf("Number of polarizations           : %d\n",
                          stYUM.iNumPol);
        }
        if ((stYUM.dDM != 0.0)
            || (YAPP_FORMAT_DTS_TIM == iFormat)
            || (YAPP_FORMAT_DTS_DAT == iFormat))
        {
            (void) printf("DM used in dedispersion           : %g\n",
                          stYUM.dDM);
        }
        (void) printf("Barycentric                       : %s\n",
                      stYUM.iFlagBary ? "Yes" : "No");
        (void) printf("Duration of data in\n");
        if (!((YAPP_FORMAT_DTS_TIM == iFormat)
              || (YAPP_FORMAT_DTS_DAT == iFormat)))
        {
            (void) printf("    Bytes                         : %ld\n",
                          (stYUM.lDataSizeTotal / stYUM.iNumChans));
        }
        else
        {
            (void) printf("    Bytes                         : %ld\n",
                          stYUM.lDataSizeTotal);
        }
        (void) printf("    Time samples                  : %d\n",
                      stYUM.iTimeSamps);
        (void) printf("    Time                          : %g s\n",
                      (stYUM.iTimeSamps * (stYUM.dTSamp / 1e3)));
        /* print statistics */
        if ((YAPP_FORMAT_DTS_TIM == iFormat)
            || (YAPP_FORMAT_DTS_DAT == iFormat))
        {
            (void) printf("Minimum value                     : %g\n",
                          stYUM.fMin);
            (void) printf("Maximum value                     : %g\n",
                          stYUM.fMax);
            (void) printf("Mean                              : %g\n",
                          stYUM.fMean);
            (void) printf("RMS                               : %g\n",
                          stYUM.fRMS);
        }
        if (!((YAPP_FORMAT_PSRFITS == iFormat)
              || (YAPP_FORMAT_SPEC == iFormat)
              || (YAPP_FORMAT_DTS_DAT == iFormat)))
        {
            (void) printf("Length of header                  : %d\n",
                          stYUM.iHeaderLen);
        }

        if ((argc - iNextOpt) != 1)
        {
            (void) printf("----------------------------------------");
            (void) printf("---------------------------------------\n");
        }
        ++iNextOpt;

        /* clear stYUM before loading it again */
        (void) memset(&stYUM, '\0', sizeof(YUM_t));
    }

    /* clean up */
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
    (void) printf("    -h  --help                          ");
    (void) printf("Display this usage information\n");
    (void) printf("    -v  --version                       ");
    (void) printf("Display the version\n");

    return;
}


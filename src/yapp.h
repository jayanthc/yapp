/**
 * @file yapp.h
 * Common header file for Yet Another Pulsar Processor
 *
 * @author Jayanth Chennamangalam
 * @date 2009.06.03
 */

#ifndef __YAPP_H__
#define __YAPP_H__

/**
 * @brief This code conforms to the ISO C99 standard, and uses functions such as
 * roundf().
 *      #define _ISOC99_SOURCE
 * 
 * This code uses glibc feature test macros, such as usleep().
 *      #define _BSD_SOURCE || _XOPEN_SOURCE >= 500
 * */
#define _GNU_SOURCE

#include <stdio.h>
#include <getopt.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <float.h>

#include <cpgplot.h>
#ifdef HDF5
#include <hdf5.h>
#endif


/**
 * @defgroup YAPPRet Standard YAPP return values.
 */
/* @{ */
#define YAPP_RET_SUCCESS            0   /**< @brief Success */
#define YAPP_RET_ERROR              -1  /**< @brief General failure */
/* @} */

/**
 * @defgroup YAPPBool Standard YAPP booleans.
 */
/* @{ */
#define YAPP_FALSE                  ((char) 0)  /**< @brief Boolean false */
#define YAPP_TRUE                   ((char) 1)  /**< @brief Boolean true */
/* @} */

/**
 * @defgroup YAPPFormat Data format strings.
 */
/* @{ */
#define YAPP_FORMATSTR_PSRFITS      "fits"
#define YAPP_FORMATSTR_SPEC         "spec"
#define YAPP_FORMATSTR_FIL          "fil"
#define YAPP_FORMATSTR_DTS_DDS      "dds"
#define YAPP_FORMATSTR_DTS_TIM      "tim"
#define YAPP_FORMATSTR_DTS_DAT      "dat"
#define YAPP_FORMATSTR_DTS_DAT_INF  "inf"
#define YAPP_FORMATSTR_YM           "ym"
#ifdef HDF5
#define YAPP_FORMATSTR_HDF5         "h5"
#endif
/* @} */

#define YAPP_FORMATDESC_PSRFITS     "PSRFITS (.fits)"
#define YAPP_FORMATDESC_SPEC        "DAS dynamic spectrum (.spec)"
#define YAPP_FORMATDESC_FIL         "SIGPROC filterbank (.fil)"
#define YAPP_FORMATDESC_DTS_DDS     "DAS time series (.dds)"
#define YAPP_FORMATDESC_DTS_TIM     "SIGPROC time series (.tim)"
#define YAPP_FORMATDESC_DTS_DAT     "PRESTO time series (.dat)"
#define YAPP_FORMATDESC_DTS_DAT_INF "PRESTO time series metadata (.inf)"
#define YAPP_FORMATDESC_YM          "YAPP text metadata (.ym)"
#ifdef HDF5
#define YAPP_FORMATDESC_HDF5        "Breakthrough Listen-ish HDF5 (.h5)"
#endif

#define EXT_PSRFITS                 ".fits"
#define EXT_DYNSPEC                 ".spec"
#define EXT_DEDISPSPEC              ".dds"
#define EXT_DEDISPSPECCFG           ".cfg"
#define EXT_PS                      ".ps"
#define EXT_PNG                     ".png"
#define EXT_GIF                     ".gif"
#define EXT_FIL                     ".fil"
#define EXT_TIM                     ".tim"
#define EXT_DAT                     ".dat"
#define EXT_INF                     ".inf"
#define EXT_YM                      ".ym"
#define EXT_YAPP_PROFILE            ".yp"
#ifdef HDF5
#define EXT_HDF5                    ".h5"
#endif

enum tagFileFormats
{
    /* dynamic spectrum formats */
    YAPP_FORMAT_SPEC = 0,       /* Desh's specfile format */
    YAPP_FORMAT_PSRFITS,        /* PSRFITS spectrometer data */
    YAPP_FORMAT_FIL,            /* SIGPROC filterbank file format */
#ifdef HDF5
    YAPP_FORMAT_HDF5,           /* HDF5 file containing dynamic spectrum */
#endif
    /* dedispersed time series formats */
    YAPP_FORMAT_DTS_DDS,        /* Desh's dedispersed data format */
    YAPP_FORMAT_DTS_TIM,        /* SIGPROC time series format */
    YAPP_FORMAT_DTS_DAT,        /* PRESTO time series format */
    /* metadata */
    YAPP_FORMAT_YM,             /* YAPP metadata text file */
    YAPP_FORMAT_NUMSUPPORTED    /* this gives the number of supported types */
};

/* sample sizes in number of bits */
#define YAPP_SAMPSIZE_1             1
#define YAPP_SAMPSIZE_2             2
#define YAPP_SAMPSIZE_4             4
#define YAPP_SAMPSIZE_8             8
#define YAPP_SAMPSIZE_16            16
#define YAPP_SAMPSIZE_32            32

/* number of bits in a byte */
#define YAPP_BYTE2BIT_FACTOR        8

#define INFIX_SMOOTH                "smooth"
#define INFIX_DECIMATE              "dec"
#define INFIX_DECIMATE_FREQ         "f"
#define INFIX_DECIMATE_TIME         "t"
#define INFIX_DECIMATE_BITS         "b"
#define INFIX_SUB                   "sub"
#define INFIX_FILTER                "filt"
#define INFIX_DEDISPERSE            "dm"
#define INFIX_SUBBAND               "band"
#define INFIX_ADD                   "sum"
#define INFIX_FOLD                  "fold"
#define INFIX_STACK                 "stack"
#define INFIX_SPLIT                 "split"
#define INFIX_HEADER                "header"

#define SUFFIX_CFG                  "_cfg"

#define LEN_GENSTRING       256 /**< @brief Length of a generic string */

/**
 * @defgroup BadTime Bad time section array parameters
 */
/* @{ */
#define NUM_BAD_BOUNDS      2   /**< @brief Length of array for each bad time
                                     section */
#define BADTIME_BEG         0   /**< @brief Index of bad section starting
                                     time */
#define BADTIME_END         1   /**< @brief Index of bad section ending time */
/* @} */

/* TODO: If the size of a block is less than the maximum delay, dedispersion (at
   best the plots) doesn't happen properly */
#define MAX_SIZE_BLOCK      65536   /**< @brief Maximum data read size */
#define MAX_SIZE_BLOCK_FOLD 1048576  /**< @brief Maximum data read size */

#define MAX_SNR_BINS        50      /**< @brief Number of SNR bins */
#define MAX_PNUM_BINS       50

#define MAX_LEN_PSRNAME     12      /* Example: J1748-2446AI*/
#define YAPP_MAX_NUM_BANDS  16

/**
 * @defgroup Defaults Default values
 */
/* @{ */
#define DEF_LAW             2.0
#define DEF_FORMAT          YAPP_FORMAT_SPEC
#define DEF_OUT_FORMAT      YAPP_FORMAT_DTS_TIM
#define DEF_SIZE_BLOCK      4096    /**< @brief Default block size */

#define DEF_WINDOWS         1000    /**< @brief Default number of windows */
#define DEF_FOLD_PULSES     1000    /**< @brief Default number of pulses to fold */
#define DEF_THRESHOLD       8.0     /**< @brief Default threshold for sifting */
/* @} */

/**
 * @defgroup DataBufs Data buffers
 */
/* @{ */
#define BUF_0               0   /**< @brief Primary buffer */
#define BUF_1               1   /**< @brief Secondary buffer */
/* @} */

/**
 * @defgroup PlotDevs PGPLOT plotting devices
 */
/* @{ */
#define PG_DEV              "1/XS"  /**< @brief Device for screen plotting */
#define PG_DEV_PS           "/CPS"  /**< @brief Device suffix for plotting to a
                                         PostScript file*/
#define PG_DEV_PNG          "/PNG"  /**< @brief Device suffix for plotting to a
                                         PNG file*/
#define PG_DEV_GIF          "/GIF"  /**< @brief Device suffix for plotting to a
                                         GIF file*/
/* @} */

#define PLOT_DDS_SUFFIX     "_dds"  /**< @brief Dedispersed data plot */

/**
 * @defgroup PlotMargins PGPLOT viewport margins
 */
/* @{ */
#define PG_VP_ML                0.06    /**< @brief Left margin */
#define PG_VP_MR                0.98    /**< @brief Right margin */
#define PG_VP_MB                0.14    /**< @brief Bottom margin */
#define PG_VP_MT                0.90    /**< @brief Top margin */
/* @} */

/* viewport margins for the colour wedge */
#define PG_WEDG_VP_ML       0.78
#define PG_WEDG_VP_MR       0.97
#define PG_WEDG_VP_MB       PG_2D_VP_MB
#define PG_WEDG_VP_MT       0.15

#define PG_CH                   1.0     /**< @brief Default character height */
#define PG_CH_2P                1.5     /**< @brief Character height for two panels */
#define PG_CH_3P                1.5     /**< @brief Character height for three panels */

#define PG_CH_SCALEFACTOR       1.4

#define PG_TICK_STEPS_X         10      /**< @brief tick marks on the x-axis */
#define PG_TICK_STEPS_Y         5       /**< @brief tick marks on the y-axis */

#define PG_VP_BUT_ML            0.86    /**< @brief Button left margin */
#define PG_VP_BUT_MR            1.00    /**< @brief Button right margin */
#define PG_VP_BUT_MB            0.00    /**< @brief Button bottom margin */
#define PG_VP_BUT_MT            0.06    /**< @brief Button top margin */

#define PG_BUT_L                0.00
#define PG_BUT_R                1.00
#define PG_BUT_B                0.00
#define PG_BUT_T                1.00

#define PG_BUTNEXT_L            0.00
#define PG_BUTNEXT_R            0.48
#define PG_BUTNEXT_B            0.00
#define PG_BUTNEXT_T            1.00
#define PG_BUTNEXT_TEXT_L       0.12
#define PG_BUTNEXT_TEXT_B       0.36
#define PG_BUTNEXT_CL_TEXT_L    0.14
#define PG_BUTNEXT_CL_TEXT_B    0.32

#define PG_BUTEXIT_L            0.52
#define PG_BUTEXIT_R            1.00
#define PG_BUTEXIT_B            0.00
#define PG_BUTEXIT_T            1.00
#define PG_BUTEXIT_TEXT_L       0.64
#define PG_BUTEXIT_TEXT_B       0.36
#define PG_BUTEXIT_CL_TEXT_L    0.66
#define PG_BUTEXIT_CL_TEXT_B    0.32

#define PG_BUT_FILLCOL          1
#define PG_BUT_CL_SLEEP         100000  /* in microseconds, 100 ms */

#define PG_PLOT_SLEEP           500000  /* in microseconds, 500 ms */

#define PG_CI_DEF               1
#define PG_CI_PLOT              11

#define PLOT_WATERFALL          1
#define PLOT_WATERFALL_GS       2
#define WATERFALL_OFFSET_SCALE  0.5   

#define YAPP_ERF_ENTRIES    1000    /* number of entries in the erf()
                                       lookup table */

#define DM_TOL_PERCENT      10

#define MIN_THRES_IN_SIGMA  3.0l    /**< @brief Minimum threshold in sigmas */

/**
 * @defgroup ProcModes Data processing specification modes
 */
/* @{ */
#define PROC_SPEC_NOTSEL    0   /**< @brief Data processing mode is not
                                     specified */
#define PROC_SPEC_PERCENT   1   /**< @brief Data processing mode is
                                     percentage */
#define PROC_SPEC_TIME      2   /**< @brief Data processing mode is time */
/* @} */

/**
 * @ingroup Defaults
 */
/* @{ */
#define DEF_PROC_SPEC       PROC_SPEC_PERCENT   /**< @brief Default data
                                                     processing parameter
                                                     specification */

/* default power law step size */
#define DEF_LAW_STEP        0.1 /**< @brief Default power law step size */
/* default power law minimum and maximum */
#define DEF_LAW_MIN         2.0 /**< @brief Default power law minimum */
#define DEF_LAW_MAX         DEF_LAW_MIN /**< @brief Default power law maximum */
/* @} */

#ifdef DEBUG
#define YAPP_FILE_DELAYS_QUAD    "./del_quad"
#define YAPP_FILE_DELAYS_LIN     "./del_lin"
#define YAPP_FILE_DELAYS_OTHER   "./del_other"
#endif

#define YAPP_MAX_MEMTABLE       1024

#define YAPP_DEGPERHOUR         15              /* degrees per hour */

#ifdef HDF5
/* HDF5 */
/* dimensionality of dynamic spectrum */
#define YAPP_HDF5_DYNSPEC_RANK      2
#endif

/**
 * YAPP Unified Metadata (YUM) definition
 * YUM abstracts the metadata storage schemes employed by different file
 * formats
 */
typedef struct YUM_s
{
    char acSite[LEN_GENSTRING];
    char acPulsar[MAX_LEN_PSRNAME];
    double dTSamp;          /* in ms */
    float fFCentre;         /* in MHz */
    float fBW;              /* in MHz */
    int iNumChans;
    float fChanBW;          /* in MHz */
    long int lDataSizeTotal;
    int iTimeSamps;
    int iNumGoodChans;
    double dTNextBF;        /* in s */
    double dTBFInt;         /* in s */
    char *pcIsChanGood;
    int iBFTimeSects;
    float *pfBFTimeSectMean;
    float *pfBFGain;
    int iNumBadTimes;
    double (*padBadTimes)[][NUM_BAD_BOUNDS];
    int iNumBeams;
    int iBeamID;
    int iNumBits;
    int iNumIFs;
    int iBackendID;
    double dSourceRA;
    double dSourceDec;
    double dAzStart;
    double dZAStart;
    double dDM;
    int iFlagBaryCen;
    int iFlagPsrCen;
    int iNumBands;
    float *pfFreq;      /* in MHz */
    float fFMin;        /* in MHz */
    float fFMax;        /* in MHz */
    char cIsBandFlipped;
    int iFlagSplicedData;
    int iHeaderLen;
    double dTStart;     /* in MJD */
    float fSampSize;
    int iNumPol;
    float fMin;
    float fMax;
    float fMean;
    float fMedian;
    float fRMS;
#ifdef HDF5
    /* for HDF5 chunking */
    long int lChunkDims[YAPP_HDF5_DYNSPEC_RANK];
#endif

#if 0
    /* DAS configuration information */
    double dTSamp;          /* in ms */
    int iBytesPerFrame;     /* iNumChans * fSampSize */
    float fFCentre;         /* in MHz */
    float fBW;              /* in kHz */
    int iChanBeg;
    int iChanEnd;
    char acPulsar[MAX_LEN_PSRNAME];
    int iDay;
    int iMonth;
    int iYear;
    int iHour;
    int iMin;
    float fSec;
    char acSite[LEN_GENSTRING];
    double dTNextBF;        /* in s */
    double dTBFInt;         /* in s */
    char *pcIsChanGood;
    int iBFTimeSects;
    float *pfBFTimeSectMean;
    float *pfBFGain;
    int iNumBadTimes;
    double (*padBadTimes)[][NUM_BAD_BOUNDS];
    /* DAS derived */
    float fBW;              /* in MHz */
    char cIsBandFlipped;
    double dTSampInSec;     /* in s */
    int iNumChans;
    int iNumGoodChans;
    float fChanBW;          /* in MHz */
    float fFMin;            /* in MHz */
    float fFMax;            /* in MHz */
    long int lDataSizeTotal;
    int iTimeSamps;

    /* SIGPROC header fields */
    char acPulsar[MAX_LEN_PSRNAME];
    int iDataTypeID;
    int iNumChans;
    double dFChan1;     /* in MHz */
    double dChanBW;     /* in MHz */
    int iNumBits;
    int iNumIFs;
    double dTSamp;      /* in seconds */
    double dTStart;     /* in MJD */
    int iObsID;
    int iBackendID;
    double dSourceRA;
    double dSourceDec;
    double dAzStart;
    double dZAStart;
    double dDM;
    int iFlagBaryCen;
    /* SIGPROC derived */
    float fFCh1;        /* in MHz */
    float fChanBW;      /* in MHz */
    float fFMin;        /* in MHz */
    float fFMax;        /* in MHz */
    double dTSamp;      /* in ms */
    char acSite[LEN_GENSTRING];
    float *pfFreq;      /* in MHz? */
    int iNumBands;
    long int lDataSizeTotal;
    int iTimeSamps;
    int iNumGoodChans;

    /* derived, common to both DAS and SIGPROC */
    double dNumSigmas;
    float fStatBW;      /* in MHz */
    float fNoiseRMS;
    float fThreshold;
#endif
} YUM_t;
/* TODO: call this YAPP_YUM */

#if 0
/**
 * The candidate information structure.
 */
#pragma pack(push)
#pragma pack(4)
typedef struct tagCandidates
{
    int iBlockNumber;
    int iLawIndex;
    int iDMChan;
    int iTimeSamp;
    double dTime;
    double dDM;
    float fSNR;
} CAND_INFO;
#pragma pack(pop)
#endif

/**
 * Sets the colourmap for the time vs. DM map. This is an external Fortran
 * function.
 */
#ifdef _FC_F77_     /* if using Fortran 77 compiler */
void set_colours__(int *piFlagBW, float *pfDataMin, float *pfDataMax);
#else               /* for Fortran 95 */
void set_colours_(int *piFlagBW, float *pfDataMin, float *pfDataMax);
#endif

int YAPP_GetFileType(char *pcFile);

/**
 * Extracts the filename from the given path, minus the extension. Caller is
 * responsible for freeing the returned memory.
 *
 * @param[in]   pcPath      Path to the file
 */
char* YAPP_GetFilenameFromPath(char *pcPath);

/**
 * Extracts the filename from the given path, with the extension. Caller is
 * responsible for freeing the returned memory.
 *
 * @param[in]   pcPath      Path to the file
 */
char* YAPP_GetFilenameWithExtFromPath(char *pcPath);

/**
 * Retrieves the observatory name from its ID.
 *
 * @param[in]   iObsID      Observatory ID
 * @param[out]  pcObs       Observatory name
 */
int YAPP_SP_GetObsNameFromID(int iObsID, char *pcObs);

/**
 * Retrieves the observatory ID from its name.
 *
 * @param[out]  pcObs       Observatory name
 */
int YAPP_SP_GetObsIDFromName(char *pcObs);

int YAPP_GetExtFromFormat(int iFormat, char *pcExt);
int YAPP_GetDescFromFormat(int iFormat, char *pcDesc);

/**
 * Read metadata from file
 *
 * @param[in]       pcFileSpec          Data filename
 * @param[in]       iFormat             Data file format
 * @param[out]      pstYUM              YUM structure
 */
int YAPP_ReadMetadata(char *pcFileSpec, int iFormta, YUM_t *pstYUM);

/**
 * Read header from the input PSRFITS file
 *
 * @param[in]       pcFileSpec          Data filename
 * @param[out]      pstYUM              YUM structure
 */
int YAPP_ReadPSRFITSHeader(char *pcFileSpec, YUM_t *pstYUM);

/**
 * Read configuration information corresponding to a DAS '.spec' file
 *
 * @param[in]       pcFileSpec          Data filename
 * @param[out]      pstYUM              YUM structure
 */
int YAPP_ReadDASCfg(char *pcFileSpec, YUM_t *pstYUM);

/**
 * Read configuration information corresponding to a SIGPROC '.fil' file
 *
 * @param[in]       pcFileSpec          Data filename
 * @param[in]       iFormat             Data file type
 * @param[out]      pstYUM              YUM structure
 */
int YAPP_ReadSIGPROCHeader(char *pcFileSpec, int iFormat, YUM_t *pstYUM);

/**
 * Read configuration information corresponding to a SIGPROC '.fil' file, from a separate header file
 *
 * @param[in]       pcFileSpec          Data filename
 * @param[out]      pstYUM              YUM structure
 */
int YAPP_ReadSIGPROCHeaderFile(char *pcFileSpec, YUM_t *pstYUM);

/**
 * Read configuration information corresponding to a PRESTO '.dat' file, from a separate header file
 *
 * @param[in]       pcFileData          Data filename
 * @param[out]      pstYUM              YUM structure
 */
int YAPP_ReadPRESTOHeaderFile(char *pcFileData, YUM_t *pstYUM);

#ifdef HDF5
/**
 * Read configuration information corresponding to an HDF5 ('.h5') file
 *
 * @param[in]       pcFileSpec          Data filename
 * @param[in]       iFormat             Data file type
 * @param[out]      pstYUM              YUM structure
 */
int YAPP_ReadHDF5Metadata(char *pcFileSpec, int iFormat, YUM_t *pstYUM);
#endif

/**
 * Read one block of data from disk
 *
 * @param[inout]    pfBuf               Output data buffer
 * @param[in]       fSampSize           Size of a sample, in bytes
 * @param[in]       iTotSampsPerBlock   Number of samples per block
 */
int YAPP_ReadData(FILE *pFData,
                  float *pfBuf,
                  float fSampSize,
                  int iTotSampsPerBlock);

#ifdef HDF5
/**
 * Read one block of data from HDF5 file on disk
 */
int YAPP_ReadHDF5Data(hid_t hDataspace,
                      hid_t hDataset,
                      hsize_t *hOffset,
                      hsize_t *hCount,
                      hid_t hMemDataspace,
                      hid_t hType,
                      float *pfBuf,
                      float fSampSize,
                      int iTotSampsPerBlock);
#endif

int YAPP_WriteMetadata(char *pcFileData, int iFormat, YUM_t stYUM);

/**
 * Calculates the threshold in terms of standard deviation.
 *
 * @param[in]   iTimeSamps  Number of time samples that are to be processed
 */
double YAPP_CalcThresholdInSigmas(int iTimeSamps);

/**
 * Calculate dispersion delays for correction.
 *
 * @param[in]   dDM             DM in pc cm^-3
 * @param[in]   stYUM           Metadata
 * @param[in]   fLaw            Dispersion law
 * @param[out]  ppiOffsetTab    Delay table
 * @param[out]  piMaxOffset     Maximum correction in terms of samples
 */
int YAPP_CalcDelays(double dDM,
                    YUM_t stYUM,
                    float fLaw,
                    int** ppiOffsetTab,
                    int* piMaxOffset);

/**
 * Decimate data
 * @param[in]   pfInBuf         Input buffer
 * @param[in]   iBlockSize      Number of samples in input buffer
 * @param[in]   iSampsPerWin    Number of time samples in window
 * @param[in]   iNumChans       Number of input channels
 * @param[in]   iChansPerWin    Number of channels in window
 * @param[out]  pfOutBuf        Output buffer
 * @param[in]   iOutNumChans    Number of output channels
 */
void YAPP_Decimate(float *pfInBuf,
                   int iBlockSize,
                   int iSampsPerWin,
                   int iNumChans,
                   int iChansPerWin,
                   float *pfOutBuf,
                   int iOutNumChans);

void YAPP_Float2Nibble(float *pfBuf,
                       int iLen,
                       float fMin,
                       float fMax,
                       unsigned char *pcBuf);

void YAPP_Float2Byte(float *pfBuf,
                     int iLen,
                     float fMin,
                     float fMax,
                     unsigned char *pcBuf);

void YAPP_Float2Short(float *pfBuf,
                      int iLen,
                      float fMin,
                      float fMax,
                      short int *piBuf);

/**
 * Smooth data
 * @param[in]   pfInBuf         Input buffer
 * @param[in]   iBlockSize      Number of samples in input buffer
 * @param[in]   iSampsPerWin    Number of samples in window
 * @param[out]  pfOutBuf        Output buffer
 */
int YAPP_Smooth(float* pfInBuf,
                int iBlockSize,
                int iSampsPerWin,
                float* pfOutBuf);

double YAPP_RAString2Double(char *pcRA);
double YAPP_DecString2Double(char *pcDec);
void YAPP_RADouble2String(double dRA, char *pcRA);
void YAPP_DecDouble2String(double dDec, char *pcDec);
int YAPP_CalcStats(char *pcFileData, int iFormat, YUM_t *pstYUM);
float YAPP_CalcMean(float *pfBuf, int iLength, int iOffset, int iStride);
float YAPP_CalcRMS(float *pfBuf,
                   int iLength,
                   int iOffset,
                   int iStride,
                   float fMean);
/*
 * The memory allocator
 */
void* YAPP_Malloc(size_t iNumItems, size_t iSize, int iZero);

/**
 * The garbage collector - frees all pointers in the memory allocation table
 */
void YAPP_CleanUp(void);

/**
 * Registers handlers for SIGTERM and CTRL+C
 */
int YAPP_RegisterSignalHandlers(void);

/**
 * Catches SIGTERM and CTRL+C and cleans up before exiting.
 */
void YAPP_HandleStopSignals(int iSigNo);

/**
 * Prints usage information.
 *
 * @param[in]   pcProgName  The name of the yapp binary file
 */
void PrintUsage(const char *pcProgName);

#endif  /* __YAPP_H__ */


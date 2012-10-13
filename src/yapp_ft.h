/**
 * @file yapp_ft.h
 * Header file for yapp_ft
 *
 * @author Jayanth Chennamangalam
 * @date 2012.08.27
 */

#ifndef __YAPP_FT_H__
#define __YAPP_FT_H__

#include <sys/types.h>  /* for open() */
#include <sys/stat.h>   /* for open() */
#include <fcntl.h>      /* for open() */
#include <fftw3.h>

#define NUM_BYTES_PER_SAMP  4
#define NUM_TAPS            8           /* number of multiples of g_iNFFT */
#define DEF_NFFT            1024        /* default value for g_iNFFT */

#define DEF_SIZE_READ       33554432    /* 32 MB - block size in VEGAS input
                                           buffer */

#define DEF_ACC             1           /* default number of spectra to
                                           accumulate */
/* for PFB */
#define NUM_TAPS            8           /* number of multiples of g_iNFFT */
#define FILE_COEFF_PREFIX   "coeff"
#define FILE_COEFF_DATATYPE "float"
#define FILE_COEFF_SUFFIX   ".dat"

#define DEF_NUM_SUBBANDS    1       /* NOTE: no support for > 1 */

#define USEC2SEC            1e-6

#define YAPP_RET_DATADONE   1

typedef struct tagPFBData
{
    signed char *pcData;        /* raw data, LEN_DATA long*/
    fftwf_complex *pfcDataX;    /* unpacked pol-X data, g_iNFFT long */
    fftwf_complex *pfcDataY;    /* unpacked pol-Y data, g_iNFFT long */
    int iNextIdx;               /* index of next element in PFB ring buffer */
} PFB_DATA;

/**
 * Initialises the PFB
 */
int InitPFB(int iNTaps, int iNFFT);

/**
 * Reads all data from the input file and loads it into memory.
 */
int LoadDataToMem(int iNFFT);

/**
 * Reads one block (32MB) of data form memory.
 */
int ReadData(char cIsFirst, int iNTaps, int iNFFT);

/*
 * Perform polyphase filtering.
 */
int DoPFB(int iNTaps, int iNFFT);
int CopyDataForFFT(int iNFFT);
int DoFFT(void);
void CleanUp(int iNTaps);

/* PGPLOT function declarations */
int InitPlot(void);
void Plot(void);

#endif  /* __YAPP_FT_H__ */


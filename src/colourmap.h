/**
 * @file colourmap.h
 * Colourmap header file
 *
 * @author Jayanth Chennamangalam
 * @date 2011.03.21
 */

#ifndef __COLOURMAP_H__
#define __COLOURMAP_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cpgplot.h>

/* TODO: this block is a duplicate of that in yapp.h */
/**
 * @defgroup PlotMargins PGPLOT viewport margins
 */
/* @{ */
#define PG_2D_VP_ML         0.06    /**< @brief Left margin */
#define PG_2D_VP_MR         0.98    /**< @brief Right margin */
#define PG_2D_VP_MB         0.14    /**< @brief Bottom margin */
#define PG_2D_VP_MT         0.90    /**< @brief Top margin */
/* @} */

/* TODO: this block is a duplicate of that in yapp.h */
/* viewport margins for the colour wedge */
#define PG_WEDG_VP_ML       0.78
#define PG_WEDG_VP_MR       0.97
#define PG_WEDG_VP_MB       PG_2D_VP_MB
#define PG_WEDG_VP_MT       0.15

#define LEN_GENSTRING       256
#define CMAP_LEVELS         64
#define NUM_CMAPS           19

typedef enum CMap_e
{
    CMAP_AUTUMN = 0,
    CMAP_BLUE,
    CMAP_BONE,
    CMAP_COOL,
    CMAP_COPPER,
    CMAP_GRAY,
    CMAP_GRAY_INV,
    CMAP_HOT,
    CMAP_JET,
    CMAP_PINK,
    CMAP_SPRING,
    CMAP_SUMMER,
    CMAP_WINTER
} CMap_t;

#define CMAP_STR_AUTUMN     "autumn"
#define CMAP_STR_BLUE       "blue"
#define CMAP_STR_BONE       "bone"
#define CMAP_STR_COOL       "cool"
#define CMAP_STR_COPPER     "copper"
#define CMAP_STR_GRAY       "gray"
#define CMAP_STR_GRAY_INV   "gray-inv"
#define CMAP_STR_HOT        "hot"
#define CMAP_STR_JET        "jet"
#define CMAP_STR_PINK       "pink"
#define CMAP_STR_SPRING     "spring"
#define CMAP_STR_SUMMER     "summer"
#define CMAP_STR_WINTER     "winter"

#define DEF_CMAP            CMAP_JET
#define DEF_CMAP_STR        "jet"

int SetColourMap(int iCMap, int iIsColInv, float fColMin, float fColMax);
int GetColourMapFromName(char *pcCMapName);
/*
 * Wrapper for PGPLOT grayscale plotting
 */
void Plot2D(float* pfBuf, float fDataMin, float fDataMax,
            float* pfX, int iLenX, float fXStep,
            float* pfY, int iLenY, float fYStep,
            char* pcXLabel, char* pcYLabel, char* pcTitle,
            int iColourMap);

#endif  /* __COLOURMAP_H__ */


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

#define LEN_GENSTRING       256
#define CMAP_LEVELS         64
#define NUM_CMAPS           19

typedef enum CMap_e
{
    CMAP_AUTUMN = 0,
    CMAP_BONE,
    CMAP_COLORCUBE,
    CMAP_COOL,
    CMAP_COPPER,
    CMAP_FLAG,
    CMAP_GRAY,
    CMAP_HOT,
    CMAP_HOT_INV,
    CMAP_HSV,
    CMAP_JET,
    CMAP_LINES,
    CMAP_PINK,
    CMAP_PRISM,
    CMAP_SPRING,
    CMAP_SUMMER,
    CMAP_WHITE,
    CMAP_WINTER,
    CMAP_BLUE
} CMap_t;

#define CMAP_STR_AUTUMN     "autumn"
#define CMAP_STR_BONE       "bone"
#define CMAP_STR_COLORCUBE  "colorcube"
#define CMAP_STR_COOL       "cool"
#define CMAP_STR_COPPER     "copper"
#define CMAP_STR_FLAG       "flag"
#define CMAP_STR_GRAY       "gray"
#define CMAP_STR_HOT        "hot"
#define CMAP_STR_HOT_INV    "hot-inv"
#define CMAP_STR_HSV        "hsv"
#define CMAP_STR_JET        "jet"
#define CMAP_STR_LINES      "lines"
#define CMAP_STR_PINK       "pink"
#define CMAP_STR_PRISM      "prism"
#define CMAP_STR_SPRING     "spring"
#define CMAP_STR_SUMMER     "summer"
#define CMAP_STR_WHITE      "white"
#define CMAP_STR_WINTER     "winter"
#define CMAP_STR_BLUE       "blue"

#define DEF_CMAP            CMAP_JET
#define DEF_CMAP_STR        "jet"

int SetColourMap(int iCMap, int iIsColInv, float fColMin, float fColMax);
int GetColourMapFromName(char *pcCMapName);

#endif  /* __COLOURMAP_H__ */


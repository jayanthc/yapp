/**
 * @file yapp_fits2fil.h
 * Header file for yapp_fits2fil
 *
 * @author Jayanth Chennamangalam
 * @date 2013.03.06
 */

#ifndef __YAPP_FITS2FIL_H__
#define __YAPP_FITS2FIL_H__

#include <stdarg.h>


#define YAPP_MAX_NPOL           2

#define YAPP_POL_X              "X"
#define YAPP_POL_Y              "Y"
#define YAPP_POL_XPLUSY         "X+Y"
#define YAPP_POL_XANDY          "X&Y"

enum tagPolFormats
{
    YAPP_POL_FORMAT_X = 0,
    YAPP_POL_FORMAT_Y,
    YAPP_POL_FORMAT_XPLUSY,
    YAPP_POL_FORMAT_XANDY
}

#define YAPP_DEF_POL_FORMAT     YAPP_POL_FORMAT_XANDY

#define YAPP_LEN_POL_STR        3   /* maximum length among the above strings */

int YAPP_WritePolSelection(int iNumBits,
                           long int lLen,
                           int iPolFormat,
                           FILE *pFDef, ...);

#endif  /* __YAPP_FITS2FIL_H__ */


#ifndef __P2STIM_H__
#define __P2STIM_H__

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_LEN_PSRNAME     12      /* Example: J1748-2446AI*/

#include "yapp_sigproc.h"

#define LEN_GENSTRING       256
#define SIZE_BUF            1048576 /* 1 MB */

int GetMetadata(FILE *pFInf, YAPP_SIGPROC_HEADER *pstHeader);
int WriteMetadata(FILE *pFTim, YAPP_SIGPROC_HEADER stHeader);
int CopyData(char *pcFileData, FILE *pFTim);
char* GetFilenameFromPath(char *pcPath, char *pcExt);

#endif  /* __P2STIM_H__ */


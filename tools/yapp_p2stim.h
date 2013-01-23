/**
 * @file yapp_p2stim.h
 * Header file for yapp_p2stim
 *
 * @author Jayanth Chennamangalam
 * @date 2012.12.18
 */

#ifndef __P2STIM_H__
#define __P2STIM_H__

#define SIZE_BUF    1048576 /* 1 MB */

int GetMetadata(FILE *pFInf, YAPP_SIGPROC_HEADER *pstHeader);
int WriteMetadata(FILE *pFTim, YAPP_SIGPROC_HEADER stHeader);
int CopyData(char *pcFileData, FILE *pFTim);
char* GetFilenameFromPath(char *pcPath, char *pcExt);

#endif  /* __P2STIM_H__ */


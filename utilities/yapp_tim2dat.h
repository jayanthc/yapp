/**
 * @file yapp_tim2dat.h
 * Header file for yapp_tim2dat
 *
 * @author Jayanth Chennamangalam
 * @date 2013.04.12
 */

#ifndef __YAPP_TIM2DAT_H__
#define __YAPP_TIM2DAT_H__

#define SIZE_BUF    1048576 /* 1 MB */

int YAPP_CopyData(char *pcFileData, FILE *pFDat, int iOffset);

#endif  /* __YAPP_TIM2DAT_H__ */


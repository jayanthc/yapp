/**
 * @file yapp_fil2h5.h
 * Header file for yapp_fil2h5
 *
 * @author Jayanth Chennamangalam
 * @date 2017.01.24
 */

#ifndef __YAPP_FIL2H5_H__
#define __YAPP_FIL2H5_H__

#define SIZE_BUF    16777216    /* 16 MB */

int YAPP_CopyData(char *pcFileData,
                  int iOffset,
                  hid_t hFileID,
                  int iNumChans,
                  int iTimeSamps);

#endif  /* __YAPP_FIL2H5_H__ */


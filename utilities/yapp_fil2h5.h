/**
 * @file yapp_fil2h5.h
 * Header file for yapp_fil2h5
 *
 * @author Jayanth Chennamangalam
 * @date 2017.01.24
 */

#ifndef __YAPP_FIL2H5_H__
#define __YAPP_FIL2H5_H__

#define SIZE_BUF    (8192 * iNumChans)

int YAPP_CopyData(char *pcFileData,
                  int iOffset,
                  hid_t hFileID,
                  int iNumChans,
                  int iTimeSamps,
                  hid_t hType);

#endif  /* __YAPP_FIL2H5_H__ */


/**
 * @file yapp_psrfits.h
 * Header file for PSRFITS support
 *
 * @author Jayanth Chennamangalam
 * @date 2013.02.27
 */

#ifndef __YAPP_PSRFITS_H__
#define __YAPP_PSRFITS_H__

#define YAPP_PF_HDUNAME_AOGEN       "AOGEN"
#define YAPP_PF_HDUNAME_PDEV        "PDEV"
#define YAPP_PF_HDUNAME_SUBINT      "SUBINT"

#define YAPP_PF_LABEL_SRCNAME       "SRC_NAME"
#define YAPP_PF_LABEL_DATATYPE      "OBS_MODE"
#define YAPP_PF_LABEL_NUMCHANS      "OBSNCHAN"
#define YAPP_PF_LABEL_FCENTRE       "OBSFREQ"
#define YAPP_PF_LABEL_BW            "OBSBW"

#define YAPP_PF_LABEL_NUMIFS        "NUMIFS"

#define YAPP_PF_LABEL_TSAMP         "TBIN"
#define YAPP_PF_LABEL_CHANBW        "CHAN_BW"
#define YAPP_PF_LABEL_NUMBITS       "NBITS"
#define YAPP_PF_LABEL_TSTART        "STT_IMJD"  /* MJD integer (UTC days) */
#define YAPP_PF_LABEL_TSTARTSEC     "STT_SMJD"  /* seconds past UTC 0h */
#define YAPP_PF_LABEL_TSTARTOFF     "STT_OFFS"  /* fraction of seconds */
#define YAPP_PF_LABEL_TLEN          "SCANLEN"
#define YAPP_PF_LABEL_OBSID         "TELESCOP"
#define YAPP_PF_LABEL_BEID          "BACKEND"
#define YAPP_PF_LABEL_SRCRA         "RA"
#define YAPP_PF_LABEL_SRCDEC        "DEC"
#define YAPP_PF_LABEL_NSBLK         "NSBLK"
#define YAPP_PF_LABEL_NSUBINT       "NAXIS2"    /* in SUBINT HDU */

#endif  /* __YAPP_PSRFITS_H__ */


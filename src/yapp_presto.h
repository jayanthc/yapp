/**
 * @file yapp_presto.h
 * Header file for PRESTO support
 *
 * @author Jayanth Chennamangalam
 * @date 2013.04.13
 */

#ifndef __YAPP_PRESTO_H__
#define __YAPP_PRESTO_H__

/* length of label string */
#define YAPP_PR_LEN_LABEL           40

#define YAPP_PR_LABEL_SITE          " Telescope used                         "
#define YAPP_PR_LABEL_BACKEND       " Instrument used                        "
#define YAPP_PR_LABEL_SRCNAME       " Object being observed                  "
#define YAPP_PR_LABEL_SRCRA         " J2000 Right Ascension (hh:mm:ss.ssss)  "
#define YAPP_PR_LABEL_SRCDEC        " J2000 Declination     (dd:mm:ss.ssss)  "
#define YAPP_PR_LABEL_TSTART        " Epoch of observation (MJD)             "
#define YAPP_PR_LABEL_BARY          " Barycentered?           (1=yes, 0=no)  "
#define YAPP_PR_LABEL_NSAMPS        " Number of bins in the time series      "
#define YAPP_PR_LABEL_TSAMP         " Width of each time series bin (sec)    "
#define YAPP_PR_LABEL_DM            " Dispersion measure (cm-3 pc)           "
#define YAPP_PR_LABEL_FMIN          " Central freq of low channel (Mhz)      "
#define YAPP_PR_LABEL_BW            " Total bandwidth (Mhz)                  "
#define YAPP_PR_LABEL_NCHANS        " Number of channels                     "
#define YAPP_PR_LABEL_CHANBW        " Channel bandwidth (Mhz)                "

#endif  /* __YAPP_PRESTO_H__ */


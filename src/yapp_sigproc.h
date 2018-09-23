/**
 * @file yapp_sigproc.h
 * Header file for SIGPROC support
 *
 * @author Jayanth Chennamangalam
 * @date 2010.09.15
 */

#ifndef __YAPP_SIGPROC_H__
#define __YAPP_SIGPROC_H__

typedef struct tagSIGPROCHeader
{
    char acPulsar[MAX_LEN_PSRNAME];
    int iDataTypeID;
    int iNumChans;
    double dFChan1;     /* in MHz */
    double dChanBW;     /* in MHz */
    int iNumBeams;
    int iBeamID;
    int iNumBits;
    int iNumIFs;
    double dTSamp;      /* in seconds */
    double dTStart;     /* in MJD */
    int iObsID;
    int iBackendID;
    double dSourceRA;
    double dSourceDec;
    double dAzStart;
    double dZAStart;
    double dDM;
    int iFlagBary;
} YAPP_SIGPROC_HEADER;

#define YAPP_SP_LABEL_HDRSTART      "HEADER_START"
#define YAPP_SP_LABEL_HDREND        "HEADER_END"
#define YAPP_SP_LABEL_RAWFILENAME   "rawdatafile"
#define YAPP_SP_LABEL_SRCNAME       "source_name"
#define YAPP_SP_LABEL_DATATYPE      "data_type"
#define YAPP_SP_LABEL_NUMCHANS      "nchans"
#define YAPP_SP_LABEL_FCHAN1        "fch1"
#define YAPP_SP_LABEL_CHANBW        "foff"
#define YAPP_SP_LABEL_NUMBEAMS      "nbeams"
#define YAPP_SP_LABEL_BEAMID        "ibeam"
#define YAPP_SP_LABEL_NUMBITS       "nbits"
#define YAPP_SP_LABEL_NUMIFS        "nifs"
#define YAPP_SP_LABEL_TSAMP         "tsamp"
#define YAPP_SP_LABEL_TSTART        "tstart"
#define YAPP_SP_LABEL_OBSID         "telescope_id"
#define YAPP_SP_LABEL_BEID          "machine_id"
#define YAPP_SP_LABEL_SRCRA         "src_raj"
#define YAPP_SP_LABEL_SRCDEC        "src_dej"
#define YAPP_SP_LABEL_AZSTART       "az_start"
#define YAPP_SP_LABEL_ZASTART       "za_start"
#define YAPP_SP_LABEL_DM            "refdm"
#define YAPP_SP_LABEL_FLAGBARYCEN   "barycentric"
#define YAPP_SP_LABEL_FLAGPSRCEN    "pulsarcentric"
#define YAPP_SP_LABEL_FREQSTART     "FREQUENCY_START"
#define YAPP_SP_LABEL_FREQEND       "FREQUENCY_END"
#define YAPP_SP_LABEL_FREQCHAN      "fchannel"
/* to semi-support M. Keith's version of fake (fast_fake) that adds a field for
   8-bit files */
#define YAPP_SP_LABEL_SIGNED        "signed"

enum tagObservatory
{
    YAPP_SP_OBSID_FAKE = 0,
    YAPP_SP_OBSID_AO,
    YAPP_SP_OBSID_ORT,
    YAPP_SP_OBSID_NANCAY,
    YAPP_SP_OBSID_PARKES,
    YAPP_SP_OBSID_JB,
    YAPP_SP_OBSID_GBT,
    YAPP_SP_OBSID_GMRT,
    YAPP_SP_OBSID_EFFELSBERG
};

#define YAPP_SP_NUMOBS          9   /* number of supported sites */

#define YAPP_SP_OBS_FAKE        "Fake"
#define YAPP_SP_OBS_AO          "Arecibo"
#define YAPP_SP_OBS_ORT         "Ooty"
#define YAPP_SP_OBS_NANCAY      "Nancay"
#define YAPP_SP_OBS_PARKES      "Parkes"
#define YAPP_SP_OBS_JB          "Jodrell"
#define YAPP_SP_OBS_GBT         "GBT"
#define YAPP_SP_OBS_GMRT        "GMRT"
#define YAPP_SP_OBS_EFFELSBERG  "Effelsberg"

#define YAPP_SP_RADEC_SCALE     10000

#endif  /* __YAPP_SIGPROC_H__ */


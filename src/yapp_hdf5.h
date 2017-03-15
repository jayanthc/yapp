/**
 * @file yapp_hdf5.h
 * Header file for HDF5 support
 *
 * @author Jayanth Chennamangalam
 * @date 2017.01.24
 */

#ifndef __YAPP_HDF5_H__
#define __YAPP_HDF5_H__

#include <hdf5.h>


#define YAPP_HDF5_DYNSPEC_GROUP     "/DynamicSpectrum"
#define YAPP_HDF5_DYNSPEC_DATASET   "/Data"

/* bitshuffle + LZ4 filter options */
/* filter ID from https://support.hdfgroup.org/services/contributions.html */
#define YAPP_HDF5_FILTER_ID            32008
#define YAPP_HDF5_SIZE_FILTER_OPTS     2
#define YAPP_HDF5_FILTER_OPTS_LZ4      2

#define YAPP_HDF5_ATTRNAME_SITE         "Site"
#define YAPP_HDF5_ATTRNAME_SRCNAME      "Source"
#define YAPP_HDF5_ATTRNAME_TSAMP        "Sampling interval (s)"
#define YAPP_HDF5_ATTRNAME_FCEN         "Centre frequency (MHz)"
#define YAPP_HDF5_ATTRNAME_BW           "Bandwidth (MHz)"
#define YAPP_HDF5_ATTRNAME_NUMCHANS     "Number of channels"
#define YAPP_HDF5_ATTRNAME_CHANBW       "Channel bandwidth (MHz)"
#define YAPP_HDF5_ATTRNAME_TIMESAMPS    "Number of time samples"
#define YAPP_HDF5_ATTRNAME_NUMBITS      "Number of bits"
#define YAPP_HDF5_ATTRNAME_NUMIFS       "Number of IFs"
#define YAPP_HDF5_ATTRNAME_BACKEND      "Backend"
#define YAPP_HDF5_ATTRNAME_SRCRA        "Source RA (deg.)"
#define YAPP_HDF5_ATTRNAME_SRCDEC       "Source Dec. (deg.)"
#define YAPP_HDF5_ATTRNAME_FMIN         "Lowest frequency (MHz)"
#define YAPP_HDF5_ATTRNAME_FMAX         "Highest frequency (MHz)"
#define YAPP_HDF5_ATTRNAME_BFLIP        "Is band flipped?"
#define YAPP_HDF5_ATTRNAME_TSTART       "Start time (MJD)"

herr_t YAPP_ReadHDF5Attribute(hid_t hDataset,
                              const char *pcAttrName,
                              const H5A_info_t *pstAttrInfo,
                              void *pvOpData);
int YAPP_WriteHDF5StringAttribute(hid_t hDataset,
                                  char *pcAttrName,
                                  char *pcAttrVal);
int YAPP_WriteHDF5IntAttribute(hid_t hDataset,
                               char *pcAttrName,
                               int iAttrVal);
int YAPP_WriteHDF5FloatAttribute(hid_t hDataset,
                                 char *pcAttrName,
                                 float fAttrVal);
int YAPP_WriteHDF5DoubleAttribute(hid_t hDataset,
                                  char *pcAttrName,
                                  double dAttrVal);
int YAPP_WriteHDF5CharAttribute(hid_t hDataset,
                                char *pcAttrName,
                                char cAttrVal);

#endif  /* __YAPP_HDF5_H__ */


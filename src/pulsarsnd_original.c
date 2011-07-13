
/*
   pulsarsnd.c

   usage: pulsarsnd <de-dispersed data file> <samples per sec>

   Converts de-dispersed data from a pulsar to WAVE data.

   Created by Jayanth Chennamangalam
*/


#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "sndfile.h"

#define ARG_COUNT       	3
#define CHANNEL_COUNT		1
#define AMP_FACTOR			10000   //1000
#define MAX_STRING			256
#define FILE_EXT			".wav"

int main(int argc, char **argv)
{
	SF_INFO SFInfo = {0};
	SNDFILE *pSndFile = NULL;
	char sFile[MAX_STRING] = {0};
    char *sDataFile = NULL;
    FILE *pDataFile = NULL;
	int *piData = NULL;
	int i = 0;
	int iRet = 0;
	int iMax = 0;
	int iMin = 0;
	int iMean = 0;
	int iFileSize = 0;
	int iSamplingRate = 0;

    /* input reading and validation */
    if (argc != ARG_COUNT)
    {
        printf("ERROR: Incorrect number of arguments!\n");
        printf("Usage: pulsarsnd <de-dispersed data file> "
			   "<samples per sec>\n");
        return -1;
    }
	sDataFile = argv[1];
	(void) strncat(sFile, sDataFile, MAX_STRING);
	(void) strncat(sFile, FILE_EXT, MAX_STRING);
	iSamplingRate = atoi(argv[2]);
	if (0 == iSamplingRate)
	{
        printf("ERROR: Incorrect argument!\n");
        printf("Usage: pulsarsnd <de-dispersed data file> "
			   "<samples per sec>\n");
        return -1;
	}

	/* open the file containing the de-dispersed data */
	pDataFile = fopen(sDataFile, "r");
    if (NULL == pDataFile)
	{
		perror("ERROR: Cannot open data file");
		return -1;
	}

	/* get the size of the file */
	iRet = fseek(pDataFile, 0, SEEK_END);
	if (iRet != 0)
	{
		printf("ERROR: Cannot set file position to end!\n");
		return -1;
	}
	iFileSize = (int) ftell(pDataFile);
	if (-1 == iFileSize)
	{
		printf("ERROR: Cannot retrieve file position!\n");
		return -1;
	}
	iRet = fseek(pDataFile, 0, SEEK_SET);
	if (iRet != 0)
	{
		printf("ERROR: Cannot set file position to beginning!\n");
		return -1;
	}

	/* allocate memory and read in the data */
	piData = (int *) malloc(iFileSize * sizeof(int));
	for (i = 0; i < iFileSize; ++i)
	{
		piData[i] = fgetc(pDataFile);
	}

	/* calculate the mean of the data, subtract the mean from the
	   samples, and multiply the signal by AMP_FACTOR */
	iMax = piData[0];
	iMin = piData[0];
	for (i = 1; i < iFileSize; ++i)
	{
		if (piData[i] > iMax)
		{
			iMax = piData[i];
		}
		else
		{
			iMin = piData[i];
		}
	}
	iMean = (iMax + iMin) / 2;
        printf("min,max: %d %d; Mean %d\n",iMin,iMax,iMean);
	for (i = 0; i < iFileSize; ++i)
	{
		piData[i] = piData[i] - iMean;
		piData[i] = piData[i] * AMP_FACTOR;
//                printf("i,piData   %d %d\n",i,piData[i]);
	}

	/* set the parameters for the WAVE file */
	SFInfo.samplerate = iSamplingRate;
	SFInfo.channels = CHANNEL_COUNT;
	SFInfo.format = (SF_FORMAT_WAV | SF_FORMAT_PCM_32);

	/* open the WAVE file and write the processed data */
	pSndFile = sf_open(sFile, SFM_WRITE, &SFInfo);
	if (NULL == pSndFile)
	{
		printf("ERROR: Cannot create WAVE file!\n");
		puts(sf_strerror(pSndFile));
		free(piData);
		return -1;
	}
	if (sf_write_int(pSndFile, piData, iFileSize) != iFileSize)
	{
		printf("ERROR: Cannot write to WAVE file!\n");
		puts(sf_strerror(pSndFile));
		free(piData);
		sf_close(pSndFile);
		return -1;
	}

	/* clean up */
	sf_close(pSndFile);
	free(piData);
	fclose(pDataFile);

	return 0;
}


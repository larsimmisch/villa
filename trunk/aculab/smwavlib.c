/*------------------------------------------------------------*/
/* Copyright ACULAB plc. (c) 1996-2000                        */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : smwavlib.c                             */
/*                                                            */
/*           Purpose : Wav handling library for SMhlib        */
/*                                                            */
/*            Author : Phil Cambridge/Peter Dain              */
/*                                                            */
/*       Create Date : 5th November 1997                      */
/*                                                            */
/*             Tools : CC compiler                            */
/*                                                            */
/*                                                            */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/

#ifdef _PROSDLL
#include "proslib.h"
#endif

/*
 * By default uses standard C RTL fread/fwrite calls.
 * Make sure you use multithreaded C RTL if required. 
 * or compile with define __SMWIN32HLIB__ in order to use WIN32 calls.
 */
#include <stdio.h>
#include "smbesp.h"
#include "smhlib.h"
#include "smwavlib.h"
#include "smfwcaps.h"


#define FOURCC_RIFF     mmioFOURCC('R', 'I', 'F', 'F')
#define FOURCC_WAVE     mmioFOURCC('W', 'A', 'V', 'E')
#define FOURCC_fmt      mmioFOURCC('f', 'm', 't', ' ')
#define FOURCC_data     mmioFOURCC('d', 'a', 't', 'a')

struct wav_info{
	tSM_UT32		data_size;
    tSM_UT32		data_start;
    WAVEFORMATEX 	waveformat;
};

#ifdef __SMWIN32HLIB__
typedef HANDLE tSMWVFD;
#else
typedef FILE*  tSMWVFD;
#endif

#ifdef __SMWIN32HLIB__

/*
 * Return non-zero on error.
 */
static int seekOctets( tSMWVFD fp, long offset, int mode, tSM_UT32* newpos )
{
	int 		result;
	tSM_UT32	pos;

	result = 0;

	if (mode == 0)
	{
		pos = SetFilePointer(fp,offset,NULL,FILE_CURRENT);
	}
	else if (mode > 0)
	{
		pos = SetFilePointer(fp,offset,NULL,FILE_END);
	}
	else
	{
		pos = SetFilePointer(fp,offset,NULL,FILE_BEGIN);
	}	

	if (pos == 0xffffffff)
	{
		result = -1;
    }
	else if (newpos != 0)
	{
		*newpos = pos;
	}

	return result;
}


/*
 * Return non-zero on error.
 */
static int getOctets( tSM_UT8* buffer, int n, tSMWVFD fh )
{
	int		result;
	DWORD 	readOctets;

	result = 0;

	if (!ReadFile(fh,(LPVOID) buffer,n,&readOctets,NULL))
	{
        result = -1;
	}
	else if (((int) readOctets) != n)
	{
        result = -1;
	}

	return result;
}

/*
 * Return non-zero on error.
 */
static int putOctets( tSM_UT8* buffer, int n, tSMWVFD fh )
{
	int		result;
	DWORD 	writtenOctets;

	result = 0;

	if (!WriteFile(fh,(LPCVOID) buffer,n,&writtenOctets,NULL))
	{
        result = -1;
	}
	else if (((int)writtenOctets) != n)
	{
        result = -1;
	}

	return result;
}

#else


/*
 * Return non-zero on error.
 */
static int seekOctets( tSMWVFD fp, long offset, int mode, tSM_UT32* newpos )
{
	int			result;
	int			rc;

	result = 0;

	if (mode == 0)
	{
		rc = fseek(fp,offset, SEEK_CUR);
	}
	else if (mode > 0)
	{
    	rc = fseek(fp,offset,SEEK_END);
   	}
	else
	{
    	rc = fseek(fp,offset,SEEK_SET);
	}

    if (rc != 0)
	{
		result = -1;
	}
	else if (newpos != 0)
	{
		*newpos = ftell(fp);
	}

	return result;
}


/*
 * Return non-zero on error.
 */
static int getOctets( tSM_UT8* buffer, int n, tSMWVFD fp )
{
	int		result;
	int 	readOctets;

	result = 0;

    readOctets = fread(buffer, 1, n, fp);

	if (readOctets != n)
	{
        result = -1;
	}

	return result;
}


/*
 * Return non-zero on error.
 */
static int putOctets( tSM_UT8* buffer, int n, tSMWVFD fp )
{
	int		result;
	int 	writtenOctets;

	result = 0;

	writtenOctets = fwrite(buffer,1,n,fp);         
  
	if (writtenOctets != n)
	{
        result = -1;
	}

	return result;
}

#endif


/*
 * Return non-zero on failure.
 */
static int getUT32( tSM_UT32* value, tSMWVFD fh )
{
	int		result;
	tSM_UT8	octets[4];

	result = getOctets(&octets[0],4,fh);

	if (result == 0)
	{
		*value = ((tSM_UT32) octets[0]) + (((tSM_UT32) octets[1])<<8)  + (((tSM_UT32) octets[2])<<16)  + (((tSM_UT32) octets[3])<<24);
	}

	return result;
}


/*
 * Return non-zero on failure.
 */
static int putUT32( tSM_UT32 value, tSMWVFD fh )
{
	tSM_UT8	octets[4];

	octets[0] = (tSM_UT8) (value       & 0x0ff);
	octets[1] = (tSM_UT8) ((value>>8)  & 0x0ff);
	octets[2] = (tSM_UT8) ((value>>16) & 0x0ff);
	octets[3] = (tSM_UT8) ((value>>24) & 0x0ff);
	
	return putOctets(&octets[0],4,fh);
}


static int wav_in_open(tSMWVFD fp, struct wav_info* wavInfo)
{
    tSM_UT32 	fourcc;
    tSM_UT32 	riff_size;
    tSM_UT32 	chunk_size;
    tSM_UT32 	fmt_size;
    tSM_UT32	file_length;
    int 		rc;
    WORD        wSamplesPerBlock;   /* for IMA ADPCM only */

	/* 
	 * All RIFF files start with the FOURCC (RIFF)  
	 */
    rc = getUT32(&fourcc,fp);

    if (rc != 0)
	{
        return WAV_FILE_READ_ERROR;
    }
    
	if (fourcc != FOURCC_RIFF)
	{
        return WAV_FILE_NOT_RIFF_FORMAT;
    }

	/* 
	 * Size of the RIFF chunk includes size of the WAVE-fmt chunk. 
	 */
    rc = getUT32(&riff_size,fp);
    
	if (rc != 0)
	{
        return WAV_FILE_READ_ERROR;
    }

	/* 
	 * Get hold of the form identifier which is hopefully a WAVE form.
	 */
    rc = getUT32(&fourcc,fp);
    
	if (rc != 0)
	{
        return WAV_FILE_READ_ERROR;
    }

    if (fourcc != FOURCC_WAVE)
	{
        return WAV_FILE_NOT_WAV_FORMAT;
    }

	/* 
	 * Get hold of the next sub-chunk, which is hopefully a "fmt " chunk. 
	 */
    rc = getUT32(&fourcc,fp);

    if (rc != 0)
	{
        return WAV_FILE_READ_ERROR;
    }

	/* 
	 * If not a "fmt " chunk simply ignore it... 
	 */
    while (fourcc != FOURCC_fmt)
	{
       	rc = getUT32(&chunk_size,fp);
       
		if (rc != 0)
		{
			return WAV_FILE_READ_ERROR;
       	}

       	chunk_size /= 2;

       	if (seekOctets(fp,chunk_size*2,0,0) != 0)
		{
           return WAV_FILE_SEEK_ERROR;
       	}

       	rc = getUT32(&fourcc,fp);
       
		if (rc != 0)
		{
           return WAV_FILE_READ_ERROR;
       	}
    }

	/* 
	 * Get the length in bytes of the fmt chunk 
	 */
    rc = getUT32(&fmt_size,fp);

    if (rc != 0)
	{
       return WAV_FILE_READ_ERROR;
    }

	/* 
	 * Read the fmt chunk as WAVEFORMATEX (common to all WAVE fmt chunks) 
	 */
	rc = getOctets((tSM_UT8*)(&wavInfo->waveformat),sizeof(WAVEFORMATEX),fp);

    if (rc != 0)
	{
       return WAV_FILE_READ_ERROR;
    }

    switch(wavInfo->waveformat.wFormatTag)
    {
	    case WAVE_FORMAT_IMA_ADPCM:
    	    rc = getOctets((tSM_UT8*)(&wSamplesPerBlock),sizeof(WORD),fp);

        	if(rc)
			{
	            return WAV_FILE_READ_ERROR;
    	    }
        
			fmt_size -= sizeof(WORD);   /* for the sake of (fmt_size != sizeof(WAVEFORMATEX)) */

	        /* Prosody IMA assumes block size of 256 bytes, and 505 blocks per sample */
    	    if( (wSamplesPerBlock != 505) || (wavInfo->waveformat.nBlockAlign != 256) )
        	{
            	return WAV_FILE_READ_ERROR;
        	}
			break;
    }

    if (fmt_size != sizeof(WAVEFORMATEX))
	{
		if (seekOctets(fp,fmt_size-sizeof(WAVEFORMATEX),0,0) != 0)
		{
            return WAV_WAVEFORMAT_SEEK_ERROR;
        }
    }

	/* 
	 * Get hold of the next sub-chunk, which is hopefully a "data" chunk. 
	 */
    rc = getUT32(&fourcc,fp);

    if (rc != 0)
	{
       return WAV_FILE_READ_ERROR;
    }

	/* 
	 * If not a "data" chunk simply ignore it... 
	 */
    while (fourcc != FOURCC_data)
	{
       	rc = getUT32(&chunk_size,fp);

	    if (rc != 0)
		{
	       return WAV_FILE_READ_ERROR;
	    }

       	chunk_size /= 2;

		if (seekOctets(fp,chunk_size*2,0,0) != 0)
		{
            return WAV_CHUNK_SEEK_ERROR;
       	}

    	rc = getUT32(&fourcc,fp);

	    if (rc != 0)
		{
	       return WAV_FILE_READ_ERROR;
	    }
	}

	/* 
	 * Get number of bytes in data block.
	 */
    rc = getUT32(&wavInfo->data_size,fp);

    if (rc != 0)
	{
       return WAV_FILE_READ_ERROR;
    }


	if (seekOctets(fp,0,0,&(wavInfo->data_start)) != 0)
	{
		return WAV_FILE_READ_ERROR;
	}

	if (seekOctets(fp,0,1,&file_length) != 0)
	{
        return WAV_WAVEFORMAT_SEEK_ERROR;
    }

	/* 
	 * Check that all the data exists. 
	 */
    if (file_length < wavInfo->data_start + wavInfo->data_size)
	{
        wavInfo->data_size = (file_length - wavInfo->data_start);
    }

    return 0;
}

static int sm_replay_wav_common( 	char*				filename,
#ifdef __SMWIN32HLIB__
									HANDLE*				pfd,
#else
									FILE**				pfd,
#endif
									int*				pType,
								    struct wav_info* 	pWavInfo	)
{
	int result;
    int	rc;

	result = 0;

    if (!filename)
	{
        result = ERR_SM_BAD_PARAMETER;
    }
	else if (!filename[0])
	{
        result = ERR_SM_BAD_PARAMETER;
    }
	else
	{
#ifdef __SMWIN32HLIB__
		*pfd = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

		if (*pfd == INVALID_HANDLE_VALUE)
		{
			result = ERR_SM_FILE_ACCESS;
	    }
#else
		*pfd = fopen(filename, "rb");

		if (!(*pfd))
		{
			result = ERR_SM_FILE_ACCESS;
		}
#endif
		if (result == 0)
		{
			rc = wav_in_open(*pfd,pWavInfo);

			if (rc)
			{
				result = ERR_SM_FILE_FORMAT;
			}

			if (result == 0)
			{
				switch(pWavInfo->waveformat.wFormatTag)
				{
					case WAVE_FORMAT_ALAW:
						switch(pWavInfo->waveformat.nSamplesPerSec)
						{
							case 8000:  
								*pType = kSMDataFormat8KHzALawPCM; 
								break;
							case 6000:  
								*pType = kSMDataFormat6KHzPCM; 
								break;
						}
						break;

					case WAVE_FORMAT_MULAW:
						switch(pWavInfo->waveformat.nSamplesPerSec)
						{
							case 8000:  
								*pType = kSMDataFormat8KHzULawPCM; 
								break;
							case 6000:  
								*pType = kSMDataFormat6KHzPCM; 
								break;
						}
						break;

					case WAVE_FORMAT_OKI_ADPCM:
						switch(pWavInfo->waveformat.nSamplesPerSec)
						{
							case 8000:  
								*pType = kSMDataFormat8KHzOKIADPCM; 
								break;
							case 6000:  
								*pType = kSMDataFormat6KHzOKIADPCM; 
								break;
						}
						break;

					case WAVE_FORMAT_ACURATE_16: 
					case WAVE_FORMAT_PROSODY_1612:
						switch(pWavInfo->waveformat.nSamplesPerSec)
						{
							case 8000:  
								*pType = kSMDataFormat8KHzACUBLKPCM; 
								break;
							case 6000:  
								*pType = kSMDataFormat6KHzACUBLKPCM; 
								break;
						}
						break;

					case WAVE_FORMAT_PROSODY_8KBPS:
						*pType = kSMDataFormatCELP8KBPS; 
						break;

					case WAVE_FORMAT_G721_ADPCM: /* case WAVE_FORMAT_G726_ADPCM: doesnt exist yet in mmreg.h */
                        switch(pWavInfo->waveformat.wBitsPerSample)
                        {
                            case 6:
						        *pType = kSMDataFormatG726_48KBPS; 
                                break;
                            case 4:
						        *pType = kSMDataFormatG726_32KBPS; 
                                break;
                            case 3:
						        *pType = kSMDataFormatG726_24KBPS; 
                                break;
                            case 2:
						        *pType = kSMDataFormatG726_16KBPS; 
                                break;
							default:
								result = ERR_SM_FILE_FORMAT;
								break;
                        }
						break;

                    case WAVE_FORMAT_PCM:
                        if(pWavInfo->waveformat.nChannels == 1)
                        {
                            switch (pWavInfo->waveformat.wBitsPerSample)
                            {
                            case 16:
                                *pType = kSMDataFormat8KHz16bitMono;
                                break;
                            case 8:
                                *pType = kSMDataFormat8KHz8bitMono;
                                break;
                            default:
                                result = ERR_SM_FILE_FORMAT;
                            }
                        }
                        else
                        {
                            result = ERR_SM_FILE_FORMAT;
                        }
                        break;

					case WAVE_FORMAT_IMA_ADPCM:
						*pType = kSMDataFormatIMAADPCM; 
						break;

					default:
						result = ERR_SM_FILE_FORMAT;
				}
			}

			if (result != 0)
			{
#ifdef __SMWIN32HLIB__
				CloseHandle(*pfd);
#else
				fclose(*pfd);
#endif
			}
		}
	}

	return result;
}


#ifdef _PROSDLL
ACUDLL 
#endif
int sm_replay_wav_start( char *filename, struct sm_file_replay_parms* file_parms )
{
	int				result;
    struct wav_info wavInfo;
	int				apiDataFormat;

	file_parms->status = 0;

	result = sm_replay_wav_common(filename,&(file_parms->fd),&apiDataFormat,&wavInfo);

	if (result == 0)
	{
		/*
		 * FD left open in this case only.
		 */
		file_parms->replay_parms.type = apiDataFormat;

		if (file_parms->replay_parms.data_length == 0)
		{
    		file_parms->replay_parms.data_length = wavInfo.data_size;

			if (file_parms->offset <= file_parms->replay_parms.data_length)
			{
				file_parms->replay_parms.data_length -= file_parms->offset;
			}
			else
			{
				result = ERR_SM_FILE_ACCESS;
			}
		}

		if (result == 0)
		{
			if (file_parms->replay_parms.data_length == 0)
			{
				result = ERR_SM_FILE_ACCESS;
			}
			else
			{
				file_parms->offset += wavInfo.data_start;

				result = sm_replay_file_start(file_parms);
			}
		}

		if (result != 0)
		{
#ifdef __SMWIN32HLIB__
			CloseHandle(file_parms->fd);
#else
			fclose(file_parms->fd);
#endif
		}
	}

	if (result != 0)
	{
		file_parms->status = result;
	}

    return result;
}


/*
 * Determine Prosody API replay type from header of WAV file - may need to do this
 * in order to locate module capable of playing this format.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_replay_wav_get_type( char* filename, int* replay_type )
{
	int				result;
    struct wav_info wavInfo;
#ifdef __SMWIN32HLIB__
	HANDLE			fd;
#else
	FILE*			fd;
#endif


	result = sm_replay_wav_common(filename,&fd,replay_type,&wavInfo);

	if (result == 0)
	{
#ifdef __SMWIN32HLIB__
		CloseHandle(fd);
#else
		fclose(fd);
#endif
	}

	return result;
}


/*
 * Close file according to model (being inside same DLL also saves trouble)
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_replay_wav_close( struct sm_file_replay_parms* fileparms )
{
	int result;

	result = 0;

#ifdef __SMWIN32HLIB__
	if (CloseHandle(fileparms->fd) == 0)
	{
		result = ERR_SM_FILE_ACCESS;
	}
#else
	if (fclose(fileparms->fd) != 0)
	{
		result = ERR_SM_FILE_ACCESS;
	}
#endif

	return result;
}


/* 
 * Dump a simple wav header (no INFO LISTs etc yet - just the bare essentials)
 * Before calling this routine, wavInfo.waveformat.wFormatTag and wavInfo.waveformat.nSamplesPerSec
 * should be appropriately set.
 */
static int wav_dump_header( struct wav_info* wavInfo, tSMWVFD fp, int markAsUnknown )
{
    int			rc;
    FOURCC		fourcc;
    DWORD		riff_size;
    DWORD		fmt_size;
    tSM_UT32	header_size;
 	WORD		wFormatTag;
	WORD		wSamplesPerBlock;

	wFormatTag = wavInfo->waveformat.wFormatTag;

	if (markAsUnknown)
	{
	    wavInfo->waveformat.wFormatTag = WAVE_FORMAT_UNKNOWN; 
	}

	switch(wFormatTag)
	{
		case WAVE_FORMAT_IMA_ADPCM:
			wavInfo->waveformat.nSamplesPerSec 	= 8000;
			wavInfo->waveformat.wBitsPerSample  = 4;
			wavInfo->waveformat.nChannels	    = 1;
			wavInfo->waveformat.nBlockAlign     = 256;
			wavInfo->waveformat.cbSize		    = 2;
			wavInfo->waveformat.nAvgBytesPerSec = 4000;

			fmt_size = sizeof(WAVEFORMATEX) + sizeof(WORD);
			break;

		default:
		    wavInfo->waveformat.nChannels	= 1;
		    wavInfo->waveformat.nBlockAlign = 1;
		    wavInfo->waveformat.cbSize		= 0;

		    wavInfo->waveformat.nAvgBytesPerSec = wavInfo->waveformat.nSamplesPerSec
		                                        * wavInfo->waveformat.wBitsPerSample / 8;
		    fmt_size = sizeof(WAVEFORMATEX);
			break;
	}

	/* 
	 * Write all the header stuff to the file.
	 */
    fmt_size = 2*((fmt_size+1)/2);

    riff_size = 					/* Omit 4+4 for "RIFF" and riff_size 	*/
                4 +            		/* "WAVE" 								*/
                4 +            		/* "fmt " 								*/
                4 +            		/* fmt_size 							*/
                fmt_size +     		/* waveformat itself 					*/
                4 +            		/* "DATA" 								*/
                4 +            		/* data_size							*/
                wavInfo->data_size; /* data itself 							*/

	if (seekOctets(fp,0,-1,0) != 0)
	{
        return WAV_WAVEFORMAT_SEEK_ERROR;
    }

    fourcc = FOURCC_RIFF;

    rc = putUT32(fourcc,fp);          /* FOURCC_RIFF */
 
	if (rc != 0)
	{
       return WAV_FILE_WRITE_ERROR;
    }

    rc = putUT32(riff_size,fp);       /* size of RIFF chunk */

    if (rc != 0)
	{
       return WAV_FILE_WRITE_ERROR;
    }

    fourcc = FOURCC_WAVE;
    
	rc = putUT32(fourcc,fp);          /* FOURCC_WAVE */
    
	if (rc != 0)
	{
       return WAV_FILE_WRITE_ERROR;
    }

    fourcc = FOURCC_fmt;

    rc = putUT32(fourcc,fp);          /* FOURCC_fmt */

    if (rc != 0)
	{
       return WAV_FILE_WRITE_ERROR;
    }

    rc = putUT32(fmt_size,fp);        /* size of fmt chunk */
    
	if (rc != 0)
	{
       return WAV_FILE_WRITE_ERROR;
    }

	rc = putOctets((tSM_UT8*) (&wavInfo->waveformat), sizeof(WAVEFORMATEX),fp);

    if (rc != 0)
	{
       return WAV_FILE_WRITE_ERROR;
    }

	switch(wFormatTag)
	{
		case WAVE_FORMAT_IMA_ADPCM:
		    /* Prosody IMA assumes block size of 256 bytes, and 505 blocks per sample */
	    	wSamplesPerBlock = 505;

	    	rc = putOctets((tSM_UT8*)(&wSamplesPerBlock),sizeof(WORD),fp);

	    	if(rc)
	    	{
	        	return WAV_FILE_WRITE_ERROR;
	    	}
	    	break;
	}

    fourcc = FOURCC_data;

    rc = putUT32(fourcc,fp);  /* FOURCC_data */

    if (rc != 0)
	{
       return WAV_FILE_WRITE_ERROR;
    }

	if (seekOctets(fp,0,0,&header_size) != 0)
	{
		return WAV_FILE_WRITE_ERROR;
	}

	/* 
	 * data_size is originally file size.
	 */
	if (wavInfo->data_size != 0)
	{
		wavInfo->data_size -= (header_size+4); 
	}

	/* 
	 * size of fmt chunk
	 */
    if (putUT32(wavInfo->data_size,fp) != 0)
	{
		return WAV_FILE_WRITE_ERROR;
    }

    return 0;
}


static int sm_wav_lib_is_alaw_module( int moduleId )
{
	int				result;
	SM_FWCAPS_PARMS fwcaps;
	tSM_UT32* 		up;
	int				version;
	int				length;

	result = 1;

	fwcaps.module = moduleId;

	if (sm_get_firmware_caps(&fwcaps) == 0)
	{
		up = (tSM_UT32*) smdPSSMFFindSection(&fwcaps,-1,"BESP",&version,&length);

		if (up != 0)
		{
			if (*(up+8) == 2)
			{
				result = 0;
			}
		}
	}

	return result;
}


static int get_waveformat_type(int type, struct wav_info *wavInfo, tSMChannelId channel)
{
	int	moduleIx;
    int result;

	result = 0;
    
    switch (type)
	{
		case kSMDataFormat8KHzPCM:
			moduleIx = sm_get_channel_module_ix(channel);

            wavInfo->waveformat.wFormatTag 		= sm_wav_lib_is_alaw_module(moduleIx) ? WAVE_FORMAT_ALAW : WAVE_FORMAT_MULAW;
            wavInfo->waveformat.nSamplesPerSec 	= 8000;
			wavInfo->waveformat.wBitsPerSample   = 8;
			break;
			
    	case kSMDataFormat8KHzALawPCM: 
            wavInfo->waveformat.wFormatTag 		= WAVE_FORMAT_ALAW;
            wavInfo->waveformat.nSamplesPerSec 	= 8000;
			wavInfo->waveformat.wBitsPerSample   = 8;
            break;

	    case kSMDataFormat6KHzPCM: 
            wavInfo->waveformat.wFormatTag 		= WAVE_FORMAT_ALAW;
            wavInfo->waveformat.nSamplesPerSec 	= 6000;
			wavInfo->waveformat.wBitsPerSample   = 8;
            break;

	    case kSMDataFormat8KHzULawPCM:
            wavInfo->waveformat.wFormatTag 		= WAVE_FORMAT_MULAW;
            wavInfo->waveformat.nSamplesPerSec 	= 8000;
			wavInfo->waveformat.wBitsPerSample   = 8;
            break;
   
	 	case kSMDataFormat8KHzOKIADPCM:
            wavInfo->waveformat.wFormatTag 		= WAVE_FORMAT_OKI_ADPCM;
            wavInfo->waveformat.nSamplesPerSec 	= 8000;
			wavInfo->waveformat.wBitsPerSample   = 4;
            break;

    	case kSMDataFormat6KHzOKIADPCM:
            wavInfo->waveformat.wFormatTag 		= WAVE_FORMAT_OKI_ADPCM;
            wavInfo->waveformat.nSamplesPerSec 	= 6000;
			wavInfo->waveformat.wBitsPerSample   = 4;
            break;

    	case kSMDataFormat8KHzACUBLKPCM:
            wavInfo->waveformat.wFormatTag 		= WAVE_FORMAT_PROSODY_1612;
            wavInfo->waveformat.nSamplesPerSec 	= 8000;
			wavInfo->waveformat.wBitsPerSample   = 2;
            break;

    	case kSMDataFormat6KHzACUBLKPCM:
            wavInfo->waveformat.wFormatTag 		= WAVE_FORMAT_PROSODY_1612;
            wavInfo->waveformat.nSamplesPerSec 	= 6000;
			wavInfo->waveformat.wBitsPerSample   = 2;
            break;

    	case kSMDataFormatCELP8KBPS:
            wavInfo->waveformat.wFormatTag 		= WAVE_FORMAT_PROSODY_8KBPS;
            wavInfo->waveformat.nSamplesPerSec 	= 8000;
			wavInfo->waveformat.wBitsPerSample   = 1;
            break;

        case kSMDataFormatG726_48KBPS:
            wavInfo->waveformat.wFormatTag 		= WAVE_FORMAT_G721_ADPCM;
            wavInfo->waveformat.nSamplesPerSec 	= 8000;
            wavInfo->waveformat.wBitsPerSample   = 6;
            break;
        case kSMDataFormatG726_32KBPS:
            wavInfo->waveformat.wFormatTag 		= WAVE_FORMAT_G721_ADPCM;
            wavInfo->waveformat.nSamplesPerSec 	= 8000;
            wavInfo->waveformat.wBitsPerSample   = 4;
            break;
        case kSMDataFormatG726_24KBPS:
            wavInfo->waveformat.wFormatTag 		= WAVE_FORMAT_G721_ADPCM;
            wavInfo->waveformat.nSamplesPerSec 	= 8000;
            wavInfo->waveformat.wBitsPerSample   = 3;
            break;
        case kSMDataFormatG726_16KBPS:
            wavInfo->waveformat.wFormatTag 		= WAVE_FORMAT_G721_ADPCM;
            wavInfo->waveformat.nSamplesPerSec 	= 8000;
            wavInfo->waveformat.wBitsPerSample   = 2;
            break;

        case kSMDataFormatIMAADPCM:
            wavInfo->waveformat.wFormatTag 		= WAVE_FORMAT_IMA_ADPCM;
            break;

        default:
			result = ERR_SM_BAD_PARAMETER;
			break;
    }

    return result;
}


#ifdef _PROSDLL
ACUDLL 
#endif
int sm_record_wav_start( char *filename, struct sm_file_record_parms* file_parms )
{
	int				result;
    struct wav_info wavInfo;

	result = 0;

	file_parms->status = 0;

    if (!filename)
	{
        file_parms->status = ERR_SM_BAD_PARAMETER;

        result =  file_parms->status;
    }

	if (result == 0)
	{
#ifdef __SMWIN32HLIB__ 
		file_parms->fd = CreateFile(filename,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
 
    	if (file_parms->fd == INVALID_HANDLE_VALUE)
		{
        	file_parms->status = ERR_SM_FILE_ACCESS;

        	result = file_parms->status;
    	}
#else
		file_parms->fd = fopen(filename, "wb");

    	if (!file_parms->fd)
		{
        	file_parms->status = ERR_SM_FILE_ACCESS;

        	result = file_parms->status;
    	}
#endif
	}

	if (result == 0)
	{
	    wavInfo.data_size = 0;

		result = get_waveformat_type(file_parms->record_parms.type, &wavInfo, file_parms->record_parms.channel);

		/*
		 * The header will become valid when sm_record_wav_close is called.
		 */
	    if (wav_dump_header(&wavInfo,file_parms->fd,1) != 0)
		{
	        file_parms->status = ERR_SM_FILE_ACCESS;

	        result =  file_parms->status;
	    }    

		if (result == 0)
		{
	    	result =  sm_record_file_start(file_parms);
		}

		if (result != 0)
		{
#ifdef __SMWIN32HLIB__
			CloseHandle(file_parms->fd); 
#else
			fclose(file_parms->fd);
#endif
		}
	}

	return result;
}


#ifdef _PROSDLL
ACUDLL 
#endif
int sm_record_wav_trim_close( struct sm_file_record_parms *file_parms, tSM_UT32 trimLength )
{
	int				result;
    int 			rc;
    struct wav_info wavInfo;

	result = 0;

    file_parms->status = 0;

    result = get_waveformat_type(file_parms->record_parms.type, &wavInfo, file_parms->record_parms.channel);

	if (result == 0)
	{
		if (seekOctets(file_parms->fd,0,1,&(wavInfo.data_size)) != 0)
		{
			file_parms->status = ERR_SM_FILE_ACCESS;

			result = file_parms->status;
		}

		if (trimLength <= wavInfo.data_size)
		{
			wavInfo.data_size -= trimLength;
		}
		else
		{
			wavInfo.data_size = 0;
		}

		if (result == 0)
		{
			/* 
			 * This is the file size and will be modified by wav_dump_header.
			 */
   			rc = wav_dump_header(&wavInfo,file_parms->fd,0);

			if (rc != 0)
			{
				file_parms->status = ERR_SM_FILE_ACCESS;

				result = file_parms->status;
			}
		}
	}
	else
	{
        file_parms->status = result;
	}

#ifdef __SMWIN32HLIB__
	if (result == 0)
	{
		if (!FlushFileBuffers(file_parms->fd))
		{
	        file_parms->status = ERR_SM_FILE_ACCESS;

	        result = file_parms->status;
		} 
	}

	CloseHandle(file_parms->fd); 
#else
    rc = fclose(file_parms->fd);

    if (rc && (result == 0))
	{
        file_parms->status = ERR_SM_FILE_ACCESS;

        result = file_parms->status;
    }    
#endif

	return result;
}


#ifdef _PROSDLL
ACUDLL 
#endif
int sm_record_wav_close( struct sm_file_record_parms *file_parms )
{
	return sm_record_wav_trim_close(file_parms,0);
}


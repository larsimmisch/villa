/*------------------------------------------------------------*/
/* Copyright ACULAB plc. (c) 1996-1997                        */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : smwavlib.H                             */
/*                                                            */
/*           Purpose : Wav handling library for SMhlib        */
/*                                                            */
/*            Author : Phil Cambridge                         */
/*                                                            */
/*       Create Date : 5th November 1997                      */
/*                                                            */
/*             Tools : CC compiler                            */
/*                                                            */
/*                                                            */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/

/*
 * By default uses standard C RTL fread/fwrite calls.
 * Make sure you use multithreaded C RTL if required. 
 * or compile with define __SMWIN32HLIB__ in order to use WIN32 calls.
 */



/* smwavlib.h : #defines etc for wave (RIFF) file opening and closing */
/* No support for reading and writing of data.     */
/* Just opening and closing of files with headers. */

#ifndef __SMWAVLIB__
#define __SMWAVLIB__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef __SMHLIB__
#include "smhlib.h"
#endif
#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif  /* RC_INVOKED */

#ifndef _DWORD_DEFINED
typedef unsigned long DWORD;
#define _DWORD_DEFINED
#endif
#ifndef _WORD_DEFINED
typedef unsigned short WORD;
#define _WORD_DEFINED
#endif

#ifndef FOURCC
#define FOURCC DWORD
#endif

#ifndef MKFOURCC
#define MKFOURCC( ch0, ch1, ch2, ch3 )                                    \
		( (DWORD)(tSM_UT8)(ch0) | ( (DWORD)(tSM_UT8)(ch1) << 8 ) |	\
		( (DWORD)(tSM_UT8)(ch2) << 16 ) | ( (DWORD)(tSM_UT8)(ch3) << 24 ) )
#endif

#if !defined(_INC_MMSYSTEM)
    #define mmioFOURCC MKFOURCC
#endif

#define WAVE_FORMAT_DEVELOPMENT         (0xFFFF)

#ifndef WAVE_FORMAT_PCM

/* general waveform format structure (information common to all formats) */
typedef struct waveformat_tag {
    WORD    wFormatTag;        /* format type */
    WORD    nChannels;         /* number of channels (i.e. mono, stereo...) */
    DWORD   nSamplesPerSec;    /* sample rate */
    DWORD   nAvgBytesPerSec;   /* for buffer estimation */
    WORD    nBlockAlign;       /* block size of data */
} WAVEFORMAT;

/* flags for wFormatTag field of WAVEFORMAT */
#define WAVE_FORMAT_PCM     1

/* specific waveform format structure for PCM data */
typedef struct pcmwaveformat_tag {
    WAVEFORMAT  wf;
    WORD        wBitsPerSample;
} PCMWAVEFORMAT;

#endif /* WAVE_FORMAT_PCM */

/* general extended waveform format structure
   Use this for all NON PCM formats
   (information common to all formats)
*/
#ifndef _WAVEFORMATEX_
#define _WAVEFORMATEX_
typedef struct tWAVEFORMATEX
{
    WORD    wFormatTag;        /* format type */
    WORD    nChannels;         /* number of channels (i.e. mono, stereo...) */
    DWORD   nSamplesPerSec;    /* sample rate */
    DWORD   nAvgBytesPerSec;   /* for buffer estimation */
    WORD    nBlockAlign;       /* block size of data */
    WORD    wBitsPerSample;    /* Number of bits per sample of mono data */
    WORD    cbSize;            /* The count in bytes of the size of
                                    extra information (after cbSize) */

} WAVEFORMATEX;
#endif /* _WAVEFORMATEX_ */

#ifndef  WAVE_FORMAT_UNKNOWN
#define  WAVE_FORMAT_UNKNOWN    0x0000  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_ADPCM      0x0002  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_ALAW       0x0006  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_MULAW      0x0007  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_OKI_ADPCM  0x0010  /*  OKI  */
#define  WAVE_FORMAT_DSPGROUP_TRUESPEECH        0x0022  /*  DSP Group, Inc  */
#define  WAVE_FORMAT_PROSODY_1612  0x0027 /* Registered tag for Prosody 16kbps and 12kbps data compression */
#define  WAVE_FORMAT_PROSODY_8KBPS 0x0094 /* Registered tag for Prosody 8kbps data compression */
#define  WAVE_FORMAT_ACURATE_16 0xAC16   /* Defunct development tag, until registered with Microsoft */
#define  WAVE_FORMAT_G721_ADPCM 0x0040   /* Antex Electronics Corporation  */
#define  WAVE_FORMAT_DVI_ADPCM  0x0011  /*  Intel Corporation  */
#define  WAVE_FORMAT_IMA_ADPCM  (WAVE_FORMAT_DVI_ADPCM) /*  Intel Corporation  */

#endif

#define WAV_FILE_DOESNT_EXIST    1
#define WAV_FILE_READ_ERROR      2
#define WAV_FILE_SEEK_ERROR      3
#define WAV_FILE_WRITE_ERROR     4
#define WAV_FILE_NOT_RIFF_FORMAT 8
#define WAV_FILE_NOT_WAV_FORMAT  9
#define WAV_WAVEFORMAT_SEEK_ERROR 10
#define WAV_CHUNK_SEEK_ERROR     11
#define WAV_UNKNOWN_WAV_FORMAT   12

#ifndef RC_INVOKED
#pragma pack()          /* Revert to default packing */
#endif  /* RC_INVOKED */

#ifdef __cplusplus
extern "C" {
#endif

ACUDLL  int sm_replay_wav_start( 
    char* filename,
	struct sm_file_replay_parms* fileparms
);

ACUDLL  int sm_replay_wav_get_type( 
    char* filename,
    int* replay_type
);

ACUDLL  int sm_replay_wav_close( 
	struct sm_file_replay_parms* fileparms
);

ACUDLL  int sm_record_wav_start( 
    char* filename,
	struct sm_file_record_parms* fileparms
);

ACUDLL  int sm_record_wav_close( 
	struct sm_file_record_parms* fileparms
);

ACUDLL  int sm_record_wav_trim_close( 
	struct sm_file_record_parms* fileparms,
	tSM_UT32
);

#ifdef __cplusplus
}
#endif

#endif

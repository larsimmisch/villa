/*------------------------------------------------------------*/
/* Copyright ACULAB plc. (c) 1996-1997                        */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : smhlib.h 	                      	  */
/*                                                            */
/*           Purpose : SHARC High level library header file   */
/*                                                            */
/*            Author : Peter Dain                             */
/*                                                            */
/*       Create Date : 11th October 1995                      */
/*                                                            */
/*                                                            */
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

#ifndef __SMHLIB__
#define __SMHLIB__

#ifndef __SMBESP__
#include "smbesp.h"
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>


typedef struct sm_file_replay_parms {
#ifdef __SMWIN32HLIB__
	HANDLE		 	fd;
#else
	FILE*		 	fd;
#endif
	tSM_UT32		offset;
	SM_REPLAY_PARMS replay_parms;
	int				status;
	int				completing;
	tSM_UT32		private_length;
	int				data_length_in_buffer;
	char			buffer[kSMMaxReplayDataBufferSize];
} SM_FILE_REPLAY_PARMS;


typedef struct sm_file_record_parms {
#ifdef __SMWIN32HLIB__
	HANDLE		 	fd;
#else
	FILE*		 	fd;
#endif
	SM_RECORD_PARMS record_parms;
	int				status;
	int				completing;
	tSM_UT32		private_length;
	char			buffer[kSMMaxRecordDataBufferSize];
} SM_FILE_RECORD_PARMS;


#ifdef __cplusplus
extern "C" {
#endif

ACUDLL  int sm_replay_file_start( 
	struct sm_file_replay_parms* file_parms 
);

ACUDLL  int sm_replay_file_progress( 
	struct sm_file_replay_parms* file_parms 
);

ACUDLL  int sm_replay_file_progress_istatus( 
	struct sm_file_replay_parms* file_parms, int 
);

ACUDLL  int sm_replay_file_complete( 
	struct sm_file_replay_parms* fileparms
);

ACUDLL  int sm_replay_file_stop( 
	struct sm_file_replay_parms* file_parms 
);

ACUDLL  int sm_record_file_start( 
	struct sm_file_record_parms* file_parms 
);

ACUDLL  int sm_record_file_progress( 
	struct sm_file_record_parms* file_parms 
);

ACUDLL  int sm_record_file_progress_istatus( 
	struct sm_file_record_parms* file_parms,int 
);

ACUDLL  int sm_record_file_complete(
	struct sm_file_record_parms*	fileparms
);

ACUDLL  int sm_record_file_stop(
	struct sm_file_record_parms* file_parms
);

#ifdef __cplusplus
}
#endif

#endif


/*------------------------------------------------------------*/
/* Copyright ACULAB plc. (c) 1996-1997                        */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : smosintf.h 	                      	  */
/*                                                            */
/*           Purpose : Interface to operating system          */
/*                                                            */
/*            Author : Peter Dain                             */
/*                                                            */
/*       Create Date : 21st February 1997                     */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Change History                                             */
/*                                                            */
/* cur:  1.00   21/01/97    pgd   First issue                 */
/* rev:  1.00   21/01/97    pgd   First issue                 */
/*                                                            */
/*------------------------------------------------------------*/

#ifndef __SMOSINTF__
#define __SMOSINTF__

#ifdef __cplusplus
extern "C" {
#endif

tSMDevHandle smd_open_ctl_dev(void);

ACUDLL void smd_close_ctl_dev( void );

tSMDevHandle smd_open_chnl_dev(tSMChannelId);

void smd_close_chnl_dev(tSMDevHandle);

int  smd_ioctl_dev_generic(tSM_INT, SMIOCTLU *, tSMDevHandle, tSM_INT );

int smd_ioctl_dev_fwapi( tSM_INT, SMIOCTLU*, tSMDevHandle, tSM_INT, tSM_INT, tSM_INT);

tSMFileHandle smd_file_open( char* );

int  smd_file_read( tSMFileHandle, char *, tSM_INT );

int  smd_file_close( tSMFileHandle);


ACUDLL int smd_read_dev( tSMDevHandle, char*, tSM_INT* );

ACUDLL int smd_write_dev( tSMDevHandle, char*, tSM_INT );

ACUDLL int smd_ev_create( 
	tSMEventId*, 
	tSMChannelId,
	tSM_INT,
	tSM_INT
);

ACUDLL int smd_ev_free( 
	tSMEventId
);

ACUDLL int smd_ev_wait( 
	tSMEventId 
);


#ifdef UNIX_SYSTEM

int smd_ev_abort(int,int,int);

#ifdef SM_CLONE_UNIX

ACUDLL int smd_ev_create_allkinds_any( tSMEventId* eventId );
ACUDLL int smd_ev_create_allkinds_specific( tSMEventId* eventId, tSMChannelId channelId );
ACUDLL int smd_ev_allkinds_any_wait( tSMEventId eventId, int* isWrite, int* isRead, int* isRecog );
ACUDLL int smd_ev_allkinds_specific_wait( tSMEventId eventId, int* isWrite, int* isRead, int* isRecog );

#endif
#endif


ACUDLL int smd_yield( 
	void 
);

ACUDLL int  smd_initialize_critical_section(
	tSMCriticalSection*
);

ACUDLL int smd_delete_critical_section(
	tSMCriticalSection*
);

ACUDLL int smd_enter_critical_section(
	tSMCriticalSection*
);

ACUDLL int smd_leave_critical_section(
	tSMCriticalSection*
);


#ifdef __cplusplus
}
#endif


#endif

/*------------------------------------------------------------*/
/* ACULAB plc                                                 */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : smclib.h                               */
/*                                                            */
/*           Purpose : Conference Library Routines            */
/*                                                            */
/*            Author : Jonathan Pound                         */
/*                                                            */
/*       Create Date : 26th february 1996                     */
/*                                                            */
/*                                                            */
/*                                                            */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/

#ifndef __SMCLIB__
#define __SMCLIB__

#ifndef __SMBESP__
#include "smbesp.h"
#endif

#define CONF_MAX_PARTY 60

typedef struct sm_conference *tSMConference;

typedef struct sm_conference_add_party_parms {
    tSMConference conf;
    tSMChannelId channelIn;
    tSMChannelId channelOut;
    int out_volume;
    int out_agc;
    int in_volume;
    int in_agc;
} SM_CONFERENCE_ADD_PARTY_PARMS;

typedef struct sm_conference_remove_party_parms {
    tSMConference conf;
    tSMChannelId channelIn;
    tSMChannelId channelOut;
} SM_CONFERENCE_REMOVE_PARTY_PARMS;

typedef struct sm_conference_info_parms {
    tSMConference conf;
	int			  activeChannelCount;
    tSMChannelId* activeChannelList;
} SM_CONFERENCE_INFO_PARMS;

extern char *smConfError;



#ifdef __cplusplus
extern "C" {
#endif
	
/* Setup a conference */
ACUDLL  tSMConference sm_conference_create(
    void
);

/* Add a new party to the conference */
ACUDLL  int sm_conference_add_party(
    SM_CONFERENCE_ADD_PARTY_PARMS *
);

/* Remove a party from the conference */
ACUDLL  int sm_conference_remove_party( 
    SM_CONFERENCE_REMOVE_PARTY_PARMS *
);

/* Get conference information */
ACUDLL  int sm_conference_info( 
    SM_CONFERENCE_INFO_PARMS *
);

/* delete a conference */
ACUDLL  void sm_conference_delete(
    tSMConference conf
);


#ifdef __cplusplus
}
#endif


#endif

/*------------------------------------------------------------*/
/* ACULAB plc                                                 */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : smclib.c                               */
/*                                                            */
/*           Purpose : Conferencing High Level Library        */
/*                     provides simple, N party to N party    */
/*                     conferencing.                          */
/*                                                            */
/*            Author : Jonathan Pound                         */
/*                                                            */
/*       Create Date : 26th Febraray 1997                     */
/*                                                            */
/*                                                            */
/*                                                            */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/

#ifdef _PROSDLL
#include "proslib.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "smclib.h"

#ifndef __SMOSINTF__
#include "smosintf.h"
#endif



/* 
 * This is a single party in a conference.
 */
typedef struct sm_conference_party_struct {
    tSMChannelId    channelIn;          
    tSMChannelId    channelOut;        
    int             out_volume;             
    int             out_agc;                
    int             in_volume;             
    int             in_agc;                
    char            id;
} SM_CONFERENCE_PARTY_STRUCT;


/* 
 * This is a conference.
 */
struct sm_conference {
    tSMCriticalSection csect;
    int Nmembers;
    SM_CONFERENCE_PARTY_STRUCT parties[CONF_MAX_PARTY];
};



/* Local Constants */

char *smConfError;

static void show_parties(tSMConference conf)
{
    int p;
    for (p=0; p < conf->Nmembers; p++) {
        int q;
        printf("%x -> %x: ", conf->parties[p].channelIn, conf->parties[p].channelOut);
        for (q=0; q < conf->Nmembers; q++) {
            if (p == q) printf("x ");
            else printf("%d ", conf->parties[q].id);
        }
        printf("\n");
    }
}

        


/*
 * A useful intermediate function.
 */
static int sm_add( tSMChannelId Out, tSMChannelId In )
{
    int result;
    SM_CONF_PRIM_ADD_PARMS  parms;

    parms.channel       = Out;
    parms.participant   = In;
    
    result = sm_conf_prim_add ( &parms );
    
    if ( result != 0 ) {
        smConfError = "sm_conf_prim_add()";
        return result;
    }

    return parms.id;
}



/*
 * The actions performed by the external calls.
 */
static int unsafe_sm_conference_add_party(SM_CONFERENCE_ADD_PARTY_PARMS *add_params)
{
    tSMConference 				 	conf;
    int                          	result;
    int                          	party;
	SM_SET_SIDETONE_CHANNEL_PARMS	sidetoneParms;
	SM_CONDITION_INPUT_PARMS		condParms;
	SM_CONF_PRIM_CLONE_PARMS    	cloneParms;
	SM_CONF_PRIM_ADJ_OUTPUT_PARMS 	adjOutParms;
	SM_CONF_PRIM_ADJ_INPUT_PARMS 	adjustInputParms;

	conf = add_params->conf;

    for (party=0; party<conf->Nmembers; party++) 
	{
    	if (conf->parties[party].channelIn == add_params->channelIn) 
		{
	    	smConfError = "party already in this conference";
	    	return ERR_SM_BAD_PARAMETER;
    	}
    }

    if (conf->Nmembers >= CONF_MAX_PARTY) 
	{
        smConfError = "Conference has CONF_MAX_PARTY parties already";
        return ERR_SM_BAD_PARAMETER;
    }

    conf->parties[conf->Nmembers].channelOut = add_params->channelOut;
    conf->parties[conf->Nmembers].channelIn  = add_params->channelIn;
    conf->parties[conf->Nmembers].in_agc     = add_params->in_agc;
    conf->parties[conf->Nmembers].out_agc    = add_params->out_agc;
    conf->parties[conf->Nmembers].in_volume  = add_params->in_volume;
    conf->parties[conf->Nmembers].out_volume = add_params->out_volume;

	cloneParms.channel = add_params->channelOut;

	if (!conf->Nmembers) 
	{
		cloneParms.model = 0;
	} 
	else 
	{
		cloneParms.model = conf->parties[0].channelOut;
	}
	
	result = sm_conf_prim_clone( &cloneParms );

	if ( result != 0 ) 
	{
	    smConfError = "sm_conf_prim_clone()";
	    return result;
	}

	if (conf->Nmembers) 
	{		
		/* 
		 * Not first - add in channel that was cloned.
		 */
        result = sm_add(add_params->channelOut, conf->parties[0].channelIn); 
        
		if (result < 0) 
		{
			return result;
		}

        conf->parties[0].id = result; 
 	}    

	adjOutParms.channel = add_params->channelOut;
	adjOutParms.volume  = add_params->out_volume;
	adjOutParms.agc     = add_params->out_agc;

	result = sm_conf_prim_adj_output( &adjOutParms );

	if ( result != 0 ) 
	{
	    smConfError = "sm_conf_prim_adj_output()";
	    return result;
	}

	/* 
	 * Add the new party to the other parties' low-level conferences. 
	 */
    for (party=0; party<conf->Nmembers; party++) 
	{
        result = sm_add ( conf->parties[party].channelOut, add_params->channelIn ); 

        if (result < 0)
		{
			return result;
		}
    }

    conf->parties[conf->Nmembers].id = result; 
 
    conf->Nmembers++;
     
	adjustInputParms.channel = add_params->channelIn;
	adjustInputParms.agc     = add_params->in_agc;
	adjustInputParms.volume  = add_params->in_volume;

	result = sm_conf_prim_adj_input ( &adjustInputParms );

	if (result != 0) 
	{
	    smConfError = "sm_conf_prim_adj_input()";
	    return result;
	}

	/*
	 * Attempt to use echo cancelation in conference.
	 */
	condParms.channel				= add_params->channelIn;
	condParms.reference				= add_params->channelOut;
	condParms.reference_type		= kSMInputCondRefUseOutput;
	condParms.conditioning_type		= kSMInputCondEchoCancelation;
	condParms.conditioning_param	= 0;
	condParms.alt_data_dest			= 0;
	condParms.alt_dest_type			= kSMInputCondAltDestNone;

	result = sm_condition_input(&condParms);
	
	if (result == ERR_SM_WRONG_FIRMWARE_TYPE)
	{
		/*
		 * Fall back to sidetone suppression.
		 */
		sidetoneParms.channel   = add_params->channelIn;
		sidetoneParms.output    = add_params->channelOut;

		result = sm_set_sidetone_channel ( &sidetoneParms );

		if (result != 0) 
		{
	            smConfError = "sm_set_sidetone_channel()";
	            return result;
		}
	}
	else if (result != 0)
	{
		smConfError = "sm_condition_input()";
	    return result;
	}

    return 0;
}


static int unsafe_sm_conference_info( SM_CONFERENCE_INFO_PARMS *info_parms )
{
    tSMConference 				 	conf;
    int                          	result;
	SM_CONF_PRIM_INFO_PARMS			confPrimInfo0;
	SM_CONF_PRIM_INFO_PARMS			confPrimInfo1;
	tSMChannelId*					pActive;
	int								i,j,k;
	tSM_UT8							v;
	int								id;
	int								id_found;

	result = 0;

	conf = info_parms->conf;

	info_parms->activeChannelCount = 0;

	if (conf->Nmembers >= 2)
	{
		confPrimInfo0.channel = conf->parties[0].channelOut;

		result = sm_conf_prim_info( &confPrimInfo0 );

		if (result != 0) 
		{
	        smConfError = "sm_conf_prim_info()";
	        return result;
		}

		confPrimInfo1.channel = conf->parties[1].channelOut;

		result = sm_conf_prim_info( &confPrimInfo1 );

		if (result != 0) 
		{
	        smConfError = "sm_conf_prim_info()";
	        return result;
		}

		pActive = info_parms->activeChannelList;

		if (pActive == 0)
		{
			result = ERR_SM_BAD_PARAMETER;
		}

		for (i = 0; (result == 0) && (i < sizeof(confPrimInfo0.speakers)); i++)
		{
			v = (tSM_UT8) (confPrimInfo0.speakers[i] | confPrimInfo1.speakers[i]);

			for (j = 0; j < 8; j++)
			{
				if ((v & 1<<j) != 0)
				{
					id = (i*8) + j;

					id_found = 0;

					for (k = 0; !id_found && (k < conf->Nmembers); k++)
					{
						if (conf->parties[k].id == id)
						{
							*(pActive+info_parms->activeChannelCount) = conf->parties[k].channelIn;

							info_parms->activeChannelCount += 1;

							id_found = 1;
						}
					}
				}
			}
		}
	}

	return result;
}
 
static int unsafe_sm_conference_remove_party(tSMConference conf, tSMChannelId channelIn, tSMChannelId channelOut)
{
    int 							result;
    int 							party;
    int 							leavingParty;
	SM_CONF_PRIM_ADJ_INPUT_PARMS 	adjustInputParms;
    SM_SET_SIDETONE_CHANNEL_PARMS	sidetoneParms;
	SM_CONDITION_INPUT_PARMS		condParms;

	/* find the party to be removed */
    for (leavingParty=0; ; leavingParty++) {
        if (leavingParty >= conf->Nmembers) {
	        smConfError = "Party not found in this conference";
	        return ERR_SM_BAD_PARAMETER;
        }
        if (conf->parties[leavingParty].channelIn == channelIn) break;
    }
    
	    /* check we have the right one */
    if(conf->parties[leavingParty].channelOut != channelOut){
        smConfError = "channelOut is not related to channelIn";
        return ERR_SM_BAD_PARAMETER;
    }

	/*
	 * Mute input from this party during leaving process.
	 * Avoids leaving burst of noise.
	 */
	adjustInputParms.channel = conf->parties[leavingParty].channelIn;
	adjustInputParms.agc     = 0;
	adjustInputParms.volume  = kSMConfAdjInputVolumeMute;

	sm_conf_prim_adj_input ( &adjustInputParms );

    
    /* We now need to do the opposite of joining: delete the
     * leaving party from the low-level conferences belonging to
     * each of the remaining parties, and delete all parties from
     * the low-level conference belonging to the leaving party.
     * This second action is automatically performed when we abort
     * the low-level conference, so we just have to do the first
     * of these.
	 */
    for (party = 0; party < conf->Nmembers; party++) {
        if (party != leavingParty) {
	    SM_CONF_PRIM_LEAVE_PARMS parms;
	    parms.channel    = conf->parties[party].channelOut;
	    parms.id         = conf->parties[leavingParty].id;
	    result = sm_conf_prim_leave( &parms );
	    if ( result != 0 ) {
		smConfError = "sm_conf_prim_leave()";
		return result;
	    }
        }
    }

    sm_conf_prim_abort(conf->parties[leavingParty].channelOut);

	/*
	 * Disable EC.
	 */
	condParms.channel				= conf->parties[leavingParty].channelIn;
	condParms.reference				= kSMNullChannelId;
	condParms.reference_type		= kSMInputCondRefNone;
	condParms.conditioning_type		= kSMInputCondNone;
	condParms.conditioning_param	= 0;
	condParms.alt_data_dest			= 0;
	condParms.alt_dest_type			= kSMInputCondAltDestNone;

	result = sm_condition_input(&condParms);
	
	if (result == ERR_SM_WRONG_FIRMWARE_TYPE)
	{
		/*
		 * Fall back to sidetone suppression.
		 */
		sidetoneParms.channel   = conf->parties[leavingParty].channelIn;
		sidetoneParms.output    = kSMNullChannelId;

		result = sm_set_sidetone_channel ( &sidetoneParms );

		if (result != 0) 
		{
            smConfError = "sm_set_sidetone_channel()";
            return result;
		}
	}
	else if (result != 0)
	{
		smConfError = "sm_condition_input()";
	    return result;
	}

    conf->Nmembers--;

	    /* move up end party to fill gap */
    conf->parties[leavingParty] = conf->parties[conf->Nmembers];

    return 0;
}



/* The external entry points themselves. While these are thread-safe,
 * the actions defined above are not.
 */

/* ---------------------------------------------------------------------
 * sm_conference_create:
 * ---------------------------------------------------------------------
 * input : none
 * output: none
 * return: new conference
 * ------------------------------------------------------------------ */
#ifdef _PROSDLL
ACUDLL 
#endif
tSMConference sm_conference_create(void)
{
    tSMConference conf;
    conf = (tSMConference)malloc(sizeof(*conf));
    if (!conf) {
        smConfError = "Failure to allocate conference structure";
        return 0;
    }
    conf->Nmembers = 0;
    smd_initialize_critical_section(&conf->csect);
    return conf;
}

/* ---------------------------------------------------------------------
 * sm_conference_add_party : Add a new conference party
 * ---------------------------------------------------------------------
 * input : SM_CONFERENCE_PARTY_PARMS *add_params
 * output: none
 * return: -ve value is ERR_SM_
 * ------------------------------------------------------------------ */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_conference_add_party(SM_CONFERENCE_ADD_PARTY_PARMS *add_params)
{
    int result;
    smd_enter_critical_section(&add_params->conf->csect);
    result = unsafe_sm_conference_add_party(add_params);
    smd_leave_critical_section(&add_params->conf->csect);
    return result;
}

#ifdef _PROSDLL
ACUDLL 
#endif
int sm_conference_info( SM_CONFERENCE_INFO_PARMS *info_params )
{
    int result;
    smd_enter_critical_section(&info_params->conf->csect);
    result = unsafe_sm_conference_info(info_params);
    smd_leave_critical_section(&info_params->conf->csect);
    return result;
}


/* ---------------------------------------------------------------------
 * sm_conference_remove_party : Remove a party from an existing conference
 * ---------------------------------------------------------------------
 * input : SM_CONFERENCE_PARTY_PARMS* remove_params
 * output: none
 * return: -ve value is ERR_SM_
 * ------------------------------------------------------------------ */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_conference_remove_party( SM_CONFERENCE_REMOVE_PARTY_PARMS* remove_params)
{
    int result;
    smd_enter_critical_section(&remove_params->conf->csect);
    result = unsafe_sm_conference_remove_party(
		remove_params->conf,
		remove_params->channelIn,
		remove_params->channelOut);
    smd_leave_critical_section(&remove_params->conf->csect);
    return result;
}

/* ---------------------------------------------------------------------
 * sm_conference_delete:
 * ---------------------------------------------------------------------
 * input : conference to be deleted
 * output: none
 * return: none
 * ------------------------------------------------------------------ */
#ifdef _PROSDLL
ACUDLL 
#endif
void sm_conference_delete(tSMConference conf)
{
 int i;
 smd_enter_critical_section(&conf->csect);
 for (i=0; i<conf->Nmembers; i++) {
    sm_conf_prim_abort(conf->parties[i].channelOut);
 }   
 smd_leave_critical_section(&conf->csect);

 smd_delete_critical_section(&conf->csect);
 free(conf);
}


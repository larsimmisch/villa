/*------------------------------------------------------------*/
/* ACULAB plc.                                                */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : swlib.c                                */
/*                                                            */
/*           Purpose : Switch control library programs for    */
/*                     Multiple drivers (generic part)        */
/*                                                            */
/*            Author : Alan Rust                              */
/*                                                            */
/*       Create Date : 19th October 1992                      */
/*                                                            */
/*             Tools : CC compiler                            */
/*                                                            */
/*                                                            */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Change History                                             */
/*                                                            */
/* cur:  3.30   swunix.c   Generic switch library for all o/s */
/*                                                            */
/* rev:  1.00   10/10/92   agr   File created                 */
/* rev:  1.01   10/02/93   agr   Removed Statics              */
/* rev:  1.02   16/03/93   agr   tristate_switch added        */
/* rev:  1.03   06/04/93   agr   multiple driver support added*/
/* rev:  1.04   05/01/93   agr   set_idle function modified   */
/* rev:  2.10   14/02/96   pgd   First SCbus switch release   */
/* rev:  2.20   18/06/96   pgd   BR net streams>=32 release   */
/* rev:  2.30   17/10/96   pgd   Migrate to V4 generic etc.   */
/* rev:  3.01   31/03/98   pgd   Version in source file.      */
/* rev:  3.03   16/06/98   pgd   Eliminate __NEWC__           */
/* rev:  3.30   22/03/99   pgd   Aligned with release 3.1.4   */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/

#ifdef _ACUSWITCHDLL
#include "acusxlib.h"
#endif

#define SW_IOCTL_CODES
#include "mvswdrvr.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <winioctl.h>
#endif

#ifdef SW_TRACK_API
#include <time.h>
#endif

/*---------------------------------------------*/

int swopendev( void );

#ifdef _WIN32
int  swioctl( unsigned int, SWIOCTLU *, HANDLE, int );
#else
int  swioctl( unsigned int, SWIOCTLU *, int, int );
#endif

/*---------------------------------------------*/

#ifdef _WIN32
extern HANDLE swcard[];
#else
extern int  swcard[];
#endif

extern int nswitch;



/*-----------------------------------------------------*/
/*------------- Multiple Driver Functions -------------*/
/*-----------------------------------------------------*/


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int ACU_WINAPI sw_ver_switch( int swdrvr, struct swver_parms * verp )
{
	int  result;

	result = swopendev ( );

	if ( result == 0 )
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl( 	SWVER_SWITCH,
								(SWIOCTLU *) verp,
								swcard[swdrvr],
								sizeof ( SWVER_PARMS ) );
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
	{
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}

#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int sw_card_info( int swdrvr, struct swcard_info_parms * cardinfop )
{
	int  result;

	result = swopendev ( );

	if ( result == 0 )
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl( 	SWCARD_INFO,
								(SWIOCTLU *) cardinfop,
								swcard[swdrvr],
								sizeof ( SWCARD_INFO_PARMS ) );
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
	{
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int ACU_WINAPI sw_mode_switch( int swdrvr, struct swmode_parms * modep )
{
	int  result;

	result = swopendev ( );

	if ( result == 0 )
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl(	SWMODE_SWITCH,
								(SWIOCTLU *) modep,
								swcard[swdrvr],
								sizeof(SWMODE_PARMS) );
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
	{
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int ACU_WINAPI sw_reinit_switch( int swdrvr )
{
	int  result;

	result = swopendev ( );

	if ( result == 0 )
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl(	REINIT_SWITCH, 
								(SWIOCTLU *) 0,
								swcard[swdrvr],
								0 				);
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
	{
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int ACU_WINAPI sw_reset_switch( int swdrvr )
{
	int  result;

	result = swopendev ( );

	if ( result == 0 )
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl(	RESET_SWITCH, 
								(SWIOCTLU *) 0,
								swcard[swdrvr],
								0 					);
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
	{
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int ACU_WINAPI sw_query_switch_caps( int swdrvr, struct capabilities_parms * capabilitiesp )
{
	int  result;

	result = swopendev ( );

	if ( result == 0 )
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl(	QUERY_SWITCH_CAPS,
								(SWIOCTLU *) capabilitiesp,
								swcard[swdrvr],
								sizeof(CAP_PARMS) 			);
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
	{
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int ACU_WINAPI sw_set_output( int swdrvr, struct output_parms * outputp )
{
	int  result;

	result = swopendev ( );

	if ( result == 0 )
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl(	SET_OUTPUT, 
								(SWIOCTLU *) outputp,
								swcard[swdrvr], 
								sizeof ( OUTPUT_PARMS ) );
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}	
	}
	else
	{
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int ACU_WINAPI sw_query_output( int swdrvr, struct output_parms * queryp ) 
{
	int  result;

	result = swopendev ( );

	if (result == 0)
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl( 	QUERY_OUTPUT,
								(SWIOCTLU *) queryp,
								swcard[swdrvr],
								sizeof(OUTPUT_PARMS)	);
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
	{
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int ACU_WINAPI sw_sample_input( int swdrvr, struct sample_parms * samplep )
{
	int  result;

	result = swopendev();

	if (result == 0)
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl(	SAMPLE_INPUT, 
								(SWIOCTLU *) samplep,
								swcard[swdrvr], 
								sizeof(SAMPLE_PARMS) 	);
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
	{
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int ACU_WINAPI sw_sample_input0( int swdrvr, struct sample_parms * samplep )
{
	int  result;

	result = swopendev ( );

	if ( result == 0 )
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl(	SAMPLE_INPUT0, 
								(SWIOCTLU *) samplep,
								swcard[swdrvr], 
								sizeof(SAMPLE_PARMS)	);
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
	{
		result = MVIP_DEVICE_ERROR;
	}
	
	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int ACU_WINAPI sw_clock_control( int swdrvr, int clockmode )
{
	int  	result;
	ACU_INT	localInt;

	result = swopendev ( );

	if ( result == 0 )
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			localInt = clockmode;

			result = swioctl(	CONFIG_CLOCK, 
								(SWIOCTLU *) &localInt,
								swcard[swdrvr],
								sizeof(ACU_INT)			);
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
	{
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int ACU_WINAPI sw_query_clock_control( int swdrvr, struct query_clkmode_parms * qclkmodep )
{
	int  result;
   
	result = swopendev ( );
   
	if ( result == 0 )
    {
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl( 	QUERY_CLOCK_MODE, 
								(SWIOCTLU *) qclkmodep,
								swcard[swdrvr],
								sizeof(QUERY_CLKMODE_PARMS)	);
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
	{
		result = MVIP_DEVICE_ERROR;
	}
	
	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int ACU_WINAPI sw_tristate_switch( int swdrvr, int tristate )
{
	int  	result;
	ACU_INT	localInt;

	result = swopendev ( );

	if ( result == 0 )
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			localInt = tristate;

			result = swioctl(	TRISTATE_SWITCH,
								(SWIOCTLU *) &localInt,
								swcard[swdrvr],
								sizeof(ACU_INT) 			);
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
	{
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int ACU_WINAPI sw_dump_switch( int swdrvr, struct dump_parms * dumpp )
{
	int  result;

	result = swopendev ( );

	if (result == 0)
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl(	DUMP_SWITCH, 
								(SWIOCTLU *) dumpp,
								swcard[swdrvr],
								sizeof(DUMP_PARMS) 	);
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
	{
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int ACU_WINAPI sw_clock_scbus_master(void)
{
   	int  			result;
   	int  			i;
	int				someothercard;
	int				found;
	QCLOCK_PARMS 	qclock;

	result = swopendev ( );

	if ( result == 0 )
	{
		someothercard = 0;
		found         = 0;

		for (i = 0; (result == 0) && (found == 0) && (i < nswitch); i++)
		{
			result = swioctl( QUERY_SCM,
							  (SWIOCTLU *) &qclock,
							  swcard[i],
							  sizeof(QCLOCK_PARMS) 			);

			if (result == 0)
			{
				if (qclock.driving_scclk)
				{
					found  = 1;
					result = i;
				}
				else if (qclock.scclk_driven)
				{
					someothercard = 1;
				}
			}
		}

		if ((result >= 0) && (found == 0))
		{
			result = (someothercard) ? MVIP_OTHER_SCBUS_CLOCK : MVIP_NO_SCBUS_CLOCK;
		}
	}
	else
	{
		result = MVIP_DEVICE_ERROR;
	}

	return ( result );
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int sw_set_trace( int swdrvr, struct trace_parms * tracep )
{
	int  result;

	result = swopendev ( );

	if ( result == 0 )
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl(	SET_TRACE, 
		 						(SWIOCTLU *) tracep,
								swcard[swdrvr],
		 						sizeof(TRACE_PARMS)		);
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
	{
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int sw_track_switch( int swdrvr, struct track_parms * trackp )
{
	int  result;

	result = swopendev ( );

	if ( result == 0 )
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl(	TRACK_SWITCH,
								(SWIOCTLU *) trackp,
								swcard[swdrvr],
								sizeof(TRACK_PARMS)		);
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
	{
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int sw_set_verify( int swdrvr, int verifymode )
{
	int  	result;
	ACU_INT	localInt;

	result = swopendev ( );

	if ( result == 0 )
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			localInt = verifymode;

			result = swioctl(	SET_VERIFY, 
		 						(SWIOCTLU *) &localInt,
		 						swcard[swdrvr],
		 						sizeof(ACU_INT)  			);
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
	{
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int ACU_WINAPI sw_get_drvrs(void)
{
	int result;

	result = swopendev();

	if (result == 0)
  	{
  		result = nswitch;                 /* return number of switch drivers */
  	}
	else
	{
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int ACU_WINAPI sw_switch_override_mode( int swdrvr, int overridemode )
{ 
	int  	result;
	ACU_INT	localInt;

   	result = swopendev ( );
   
   	if ( result == 0 )
   	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			localInt = overridemode;

	      	result = swioctl( 	SWITCH_OVERRIDE_MODE, 
				 				(SWIOCTLU *) &localInt,
				 				swcard[swdrvr],
				 				sizeof (ACU_INT)  			);
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
    }
   	else
    {
      	result = MVIP_DEVICE_ERROR;
    }

   	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int sw_register_event( 	int 							swdrvr, 
						struct register_event_parms*   	reparms	) 	
{
	int  result;

	result = swopendev ( );

	if (result == 0)
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl(	REGISTER_EVENT,
								(SWIOCTLU *) reparms,
								swcard[swdrvr],
								sizeof(REGISTER_EVENT_PARMS) );
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
    {
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


/*---------------------------------------*/


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int ACU_WINAPI sw_h100_config_board_clock( int										swdrvr,
								struct h100_config_board_clock_parms*   setH100ClockParms	)
{
	int  result;

	result = swopendev ( );

	if (result == 0)
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl(	H100_CONFIG_BOARD_CLOCK,
								(SWIOCTLU *) setH100ClockParms,
								swcard[swdrvr],
								sizeof(H100_CONFIG_BOARD_CLOCK_PARMS) );
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
    {
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int ACU_WINAPI sw_h100_config_netref_clock(	int								swdrvr,
									struct h100_netref_clock_parms*	setH100NetrefParms	)   
{
	int  result;

	result = swopendev ( );

	if (result == 0)
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl(	H100_CONFIG_NETREF_CLOCK,
								(SWIOCTLU *) setH100NetrefParms,
								swcard[swdrvr],
								sizeof(H100_NETREF_CLOCK_PARMS) );
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
    {
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int ACU_WINAPI sw_h100_query_board_clock(	int										swdrvr,
								struct h100_query_board_clock_parms*	queryH100ClockParms	)
{
	int  result;

	result = swopendev ( );

	if (result == 0)
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl(	H100_QUERY_BOARD_CLOCK,
								(SWIOCTLU *) queryH100ClockParms,
								swcard[swdrvr],
								sizeof(H100_QUERY_BOARD_CLOCK_PARMS) );
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
    {
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int ACU_WINAPI sw_h100_query_netref_clock(	int								swdrvr,
								struct h100_netref_clock_parms*	queryH100NetrefParms	)   
{
	int  result;

	result = swopendev ( );

	if (result == 0)
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl(	H100_QUERY_NETREF_CLOCK,
								(SWIOCTLU *) queryH100NetrefParms,
								swcard[swdrvr],
								sizeof(H100_NETREF_CLOCK_PARMS) );
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
    {
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int sw_mc3_config_bert( int 							swdrvr, 
						struct mc3_config_bert_parms*	bparms	    )
{
	int  result;

	result = swopendev ( );

	if (result == 0)
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl(	MC3_CONFIG_BERT,
								(SWIOCTLU *) bparms,
								swcard[swdrvr],
								sizeof(MC3_CONFIG_BERT_PARMS) );
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
    {
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int sw_mc3_bert_status( int 							swdrvr, 
						struct mc3_bert_status_parms*	bsparms    	)
{
	int  result;

	result = swopendev ( );

	if (result == 0)
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl(	MC3_BERT_STATUS,
								(SWIOCTLU *) bsparms,
								swcard[swdrvr],
								sizeof(MC3_BERT_STATUS_PARMS) );
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
    {
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int sw_mc3_bypass_fibre( int swdrvr, int bypass_mode )
{
	int  	result;
	ACU_INT	localInt;

   	result = swopendev ( );
   
   	if ( result == 0 )
   	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			localInt = bypass_mode;

    	  	result = swioctl( 	MC3_BYPASS_FIBRE, 
				 				(SWIOCTLU *) &localInt,
				 				swcard[swdrvr],
			 					sizeof (ACU_INT)  			);
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
    }
   	else
    {
      	result = MVIP_DEVICE_ERROR;
    }

   	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int sw_mc3_resync_ring( int swdrvr, int resync_mode )
{
	int  	result;
	ACU_INT	localInt;

   	result = swopendev ( );
   
   	if ( result == 0 )
   	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			localInt = resync_mode;

	      	result = swioctl( 	MC3_RESYNC_RING, 
				 				(SWIOCTLU *) &localInt,
				 				swcard[swdrvr],
				 				sizeof (ACU_INT)  			);
	    }
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
   	else
    {
      	result = MVIP_DEVICE_ERROR;
    }

   	return result;
}
 

#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int sw_mc3_ring_status( int 							swdrvr, 
						struct mc3_ring_status_parms*   rsparms	) 	
{
	int  result;

	result = swopendev ( );

	if (result == 0)
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl(	MC3_RING_STATUS,
								(SWIOCTLU *) rsparms,
								swcard[swdrvr],
								sizeof(MC3_RING_STATUS_PARMS) );
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
    {
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int sw_mc3_ring_latched_status( int                             swdrvr, 
                                struct mc3_ring_status_parms*   rsparms ) 
{
	int  result;

	result = swopendev ( );

	if (result == 0)
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl(   MC3_RING_LATCHED_STATUS,
			                    (SWIOCTLU *) rsparms,
			                    swcard[swdrvr],
			                    sizeof(MC3_RING_STATUS_PARMS) );
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
    {
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int sw_mc3_event_criteria( 	int 								swdrvr, 
							struct mc3_event_criteria_parms*   	ecparms	) 	
{
	int  result;

	result = swopendev ( );

	if (result == 0)
	{
		if ((swdrvr < nswitch) && (swdrvr >= 0))
		{
			result = swioctl(	MC3_EVENT_CRITERIA,
								(SWIOCTLU *) ecparms,
								swcard[swdrvr],
								sizeof(MC3_EVENT_CRITERIA_PARMS) );
		}
		else
		{
			result = ERR_SW_INVALID_SWITCH;
		}
	}
	else
    {
		result = MVIP_DEVICE_ERROR;
	}

	return result;
}


/*-----------------------------------------------------*/
/*------------- Old Single Driver Functions -----------*/
/*-----------------------------------------------------*/


/*----------- ver_switch        ---------*/
/* switch driver version                 */
/*                                       */
int ver_switch( struct swver_parms * verp )
   {
   return ( sw_ver_switch ( 0, verp ));
   }
/*---------------------------------------*/


/*------------ reinit_switch -------------*/
/* reinitialise the switch driver         */
/*                                        */
int reinit_switch(void)
{
	return(sw_reinit_switch(0));
}
/*---------------------------------------*/


/*-------------- reset_switch -----------*/
/* reset the switch driver               */
/*                                       */
int reset_switch(void)
   {
   return ( sw_reset_switch ( 0 ));
   }
/*---------------------------------------*/


/*----------- query_switch_caps ---------*/
/* reset the switch driver               */
/*                                       */
int query_switch_caps( struct capabilities_parms * capabilitiesp )
   {
   return ( sw_query_switch_caps ( 0, capabilitiesp ));
   }
/*---------------------------------------*/


/*-------------- set_output -------------*/
/* set up the switch output              */
/*                                       */
int set_output( struct output_parms * outputp )
   {
   return ( sw_set_output ( 0, outputp ));
   }
/*---------------------------------------*/


/*------------ query_output -------------*/
/* query the switch output               */
/*                                       */
int query_output( struct output_parms * queryp )
   {
   return ( sw_query_output ( 0, queryp ));
   }
/*---------------------------------------*/


/*------------ sample_input -------------*/
/* sample the switch output              */
/*                                       */
int sample_input( struct sample_parms * samplep )
   {
   return ( sw_sample_input ( 0, samplep ));
   }
/*---------------------------------------*/


/*------------ sample_input0 ------------*/
/* sample the switch output - zero delay */
/*                                       */
int sample_input0( struct sample_parms * samplep )
   {
   return ( sw_sample_input0 ( 0, samplep ));
   }
/*---------------------------------------*/


/*-------------- clock_control ----------*/
/* switch the clock                      */
/*                                       */
int clock_control( int clockmode )
   {
   return ( sw_clock_control ( 0, clockmode ));
   }
/*---------------------------------------*/


/*---------- query_clock_control --------*/
/* query the clock                       */
/*                                       */
int query_clock_control( struct query_clkmode_parms * qclkmodep )
{
	return ( sw_query_clock_control ( 0, qclkmodep ));
}
/*---------------------------------------*/



/*------------ tristate_switch ----------*/
/* tristate the switch block             */
/*                                       */
int tristate_switch( int tristate )
   {
   return ( sw_tristate_switch ( 0, tristate ));
   }
/*---------------------------------------*/


/*-------------- dump_switch ------------*/
/* get switch contents                   */
/*                                       */
int dump_switch( struct dump_parms * dumpp )
   {
   return ( sw_dump_switch (0, dumpp ));
   }
/*---------------------------------------*/


/*---------------- set_trace ------------*/
/* set diagnostic                        */
/*                                       */
int set_trace( struct trace_parms * tracep )
   {
   return ( sw_set_trace ( 0, tracep ));
   }
/*---------------------------------------*/


/*--------------- set_verify ------------*/
/* set verify flag                       */
/*                                       */
int set_verify( int verifymode )
   {
   return ( sw_set_verify ( 0, verifymode ));
   }
/*---------------------------------------*/


/*----- switch_override_mode ------------*/
/* set MVIP MUX override mode            */
/*                                       */
int switch_override_mode( int overridemode )
	{ 
   	return(sw_switch_override_mode(0,overridemode));
	}
/*---------------------------------------*/


#ifdef SW_TRACK_API

#ifdef _ACUSWITCHDLL
ACUDLL
#endif
void sw_crack_result( int result, char* buffer )
{
	char* p;

	p = buffer;

	switch(result)
	{
		case ERR_SW_INVALID_COMMAND:
			sprintf(p,"ERR_SW_INVALID_COMMAND");
			break;

		case ERR_SW_DEVICE_ERROR:
			sprintf(p,"ERR_SW_DEVICE_ERROR");
			break;

		case ERR_SW_NO_RESOURCES:
			sprintf(p,"ERR_SW_NO_RESOURCES");
			break;

		case ERR_SW_INVALID_CLOCK_PARM:
			sprintf(p,"ERR_SW_INVALID_CLOCK_PARM");
			break;

		case ERR_SW_INVALID_MODE:
			sprintf(p,"ERR_SW_INVALID_MODE");
			break;

		case ERR_SW_INVALID_MINOR_SWITCH:
			sprintf(p,"ERR_SW_INVALID_MINOR_SWITCH");
			break;

		case ERR_SW_INVALID_PARAMETER:
			sprintf(p,"ERR_SW_INVALID_PARAMETER");
			break;

		case ERR_SW_NO_PATH:
			sprintf(p,"ERR_SW_NO_PATH");
			break;

		case ERR_SW_NO_SCBUS_CLOCK:
			sprintf(p,"ERR_SW_NO_SCBUS_CLOCK");
			break;

		case ERR_SW_OTHER_SCBUS_CLOCK:
			sprintf(p,"ERR_SW_OTHER_SCBUS_CLOCK");
			break;

		case ERR_SW_PATH_BLOCKED:
			sprintf(p,"ERR_SW_PATH_BLOCKED");
			break;

		default:
			sprintf(p,"%d",result);
			break;
	}
}


static void crackH100ClockMode( int clockMode, char* buffer )
{
	char* p;

	p = buffer;

	switch(clockMode)
	{
		case H100_SLAVE:
			sprintf(p,"SLAVE");
			break;

		case H100_MASTER_A:
			sprintf(p,"MASTER-A");
			break;

		case H100_MASTER_B:
			sprintf(p,"MASTER-B");
			break;

		default:
		    sprintf(p,"0x%04x",(int) clockMode);
			break;
	}
}

static void crackH100ClockSource( int clockSource, int network, char* buffer )
{
	char* p;

	p = buffer;

	switch(clockSource)
	{
		case H100_SOURCE_INTERNAL:
			sprintf(p,"INTERNAL");
			break;
		case H100_SOURCE_NETWORK:
			sprintf(p,"NETWORK(%d)",network);
			break;
		case H100_SOURCE_H100_A:
			sprintf(p,"H100-A");
			break;
		case H100_SOURCE_H100_B:
			sprintf(p,"H100-B");
			break;
		case H100_SOURCE_NETREF:
			sprintf(p,"NETREF");
			break;
		case H100_SOURCE_NETREF_2:
			sprintf(p,"NETREF2");
			break;
		default:
		    sprintf(p,"0x%04x",(int) clockSource);
			break;
	}
}


static void crackClockMode( int clockMode, char* buffer )
{
	char* p;

	p = buffer;

	switch((int) clockMode & (~(HI_MVIPCLK|DRIVE_SCBUS|MC3_MVIP_B)))
	{
		case CLOCK_REF_MVIP:
	        sprintf(p,"CLOCK_REF_MVIP");
			break;

		case CLOCK_REF_H100:
	        sprintf(p,"CLOCK_REF_H100");
			break;

		case CLOCK_REF_RING1:
	        sprintf(p,"CLOCK_REF_RING1");
			break;

		case CLOCK_REF_RING2:
	        sprintf(p,"CLOCK_REF_RING2");
			break;

		case CLOCK_REF_SEC8K:
	        sprintf(p,"CLOCK_REF_SEC8K");
			break;

		case CLOCK_REF_LOCAL:
	        sprintf(p,"CLOCK_REF_LOCAL");
			break;

		case CLOCK_REF_NET1:
	        sprintf(p,"CLOCK_REF_NET1");
			break;

		case CLOCK_REF_NET2:
	        sprintf(p,"CLOCK_REF_NET2");
			break;

		case CLOCK_REF_NET3:
	        sprintf(p,"CLOCK_REF_NET3");
			break;

		case CLOCK_REF_NET4:
	        sprintf(p,"CLOCK_REF_NET4");
			break;

		case CLOCK_REF_SCBUS:
	        sprintf(p,"CLOCK_REF_SCBUS");
			break;

		case CLOCK_PRIVATE:
	        sprintf(p,"CLOCK_PRIVATE");
			break;

		case SEC8K_DRIVEN_BY_LOCAL:
	        sprintf(p,"SEC8K_DRIVEN_BY_LOCAL");
			break;

		case SEC8K_DRIVEN_BY_NET1:
	        sprintf(p,"SEC8K_DRIVEN_BY_NET1");
			break;

		case SEC8K_DRIVEN_BY_NET2:
	        sprintf(p,"SEC8K_DRIVEN_BY_NET2");
			break;

		case SEC8K_DRIVEN_BY_NET3:
	        sprintf(p,"SEC8K_DRIVEN_BY_NET3");
			break;

		case SEC8K_DRIVEN_BY_NET4:
			sprintf(p,"SEC8K_DRIVEN_BY_NET4");
			break;

		case SEC8K_DRIVEN_BY_OTHER_BUS_SEC8K:
			sprintf(p,"SEC8K_DRIVEN_BY_OTHER_BUS_SEC8K");
			break;

		case SEC8K_DRIVEN_BY_RING1:
			sprintf(p,"SEC8K_DRIVEN_BY_RING1");
			break;

		case SEC8K_DRIVEN_BY_RING2:
			sprintf(p,"SEC8K_DRIVEN_BY_RING2");
			break;

		default:
		    sprintf(p,"0x%04x",(int) clockMode);
			break;
	}

	p += strlen(p);

	if ((clockMode & MC3_MVIP_B) != 0)
	{
		sprintf(p,"+MC3_MVIP_B");

		p += strlen(p);
	}

	if ((clockMode & DRIVE_SCBUS) != 0)
	{
		sprintf(p,"+DRIVE_SCBUS");

		p += strlen(p);
	}

	if ((clockMode & HI_MVIPCLK) != 0)
	{
		sprintf(p,"+HI_MVIPCLK");
	}
}


static void crackOutputParms( struct track_parms* trackParms, char* buffer )
{
	char* p;

	p = buffer;

	switch((int) trackParms->words[4])
	{
		case DISABLE_MODE:
			sprintf(	p,
						"{ost=%d,ots=%d,mode=DISABLE_MODE}",
						(int) trackParms->words[2],
						(int) trackParms->words[3]				);
			break;
			
		case PATTERN_MODE:
			sprintf(	p,
						"{ost=%d,ots=%d,mode=PATTERN_MODE,pattern=0x%02x}",
						(int) trackParms->words[2],
						(int) trackParms->words[3],
						(int) trackParms->words[7]							);
			break;
			
		case CONNECT_MODE:
			sprintf(	p,
						"{ost=%d,ots=%d,mode=CONNECT_MODE,ist=%d,its=%d}",
						(int) trackParms->words[2],
						(int) trackParms->words[3],
						(int) trackParms->words[5],
						(int) trackParms->words[6]								);
			break;

		case FRAMED_CONNECT_MODE:
			sprintf(	p,
						"{ost=%d,ots=%d,mode=FRAMED_CONNECT_MODE,ist=%d,its=%d}",
						(int) trackParms->words[2],
						(int) trackParms->words[3],
						(int) trackParms->words[5],
						(int) trackParms->words[6]								);
			break;

		default:
			sprintf(	p,
						"{ost=%d,ots=%d,mode=mode=%d(\?\?\?)}",
						(int) trackParms->words[2],
						(int) trackParms->words[3],
						(int) trackParms->words[4]				);
			break;
	}
}


static void crackTrack( struct track_parms* trackParms, char* buffer )
{
	char* p;

	p = buffer;

	if (trackParms->tracking_on >= kSWDiagTrackCmdTrackAPIWithTimestamp)
	{
		sprintf(p,"%08ld: ",trackParms->timestamp);

		p += strlen(p);
	}

	if (trackParms->is_tracked_data && (trackParms->tracked_type == kSWTrackTypeAppAPICall))
	{
		sw_crack_result(trackParms->words[0],p);

		p += strlen(p);

		sprintf(p," <- ");

		p += strlen(p);

		switch((int) (trackParms->words[1]))
		{
			case RESET_SWITCH:
				sprintf(p,"sw_reset_switch(%d)",trackParms->swdrvr);
				break;

			case QUERY_SWITCH_CAPS:
				sprintf(p,"sw_query_switch_caps(%d,...)",trackParms->swdrvr);
				break;

			case REINIT_SWITCH:
				sprintf(p,"sw_reinit_switch(%d)",trackParms->swdrvr);
				break;

			case SET_OUTPUT:
				sprintf(p,"sw_set_output(%d,",trackParms->swdrvr);

				p += strlen(p);

				crackOutputParms(trackParms,p);

				p += strlen(p);
	
				sprintf(p,")");
				break;

			case QUERY_OUTPUT:
				sprintf(p,"sw_query_output(%d,",trackParms->swdrvr);
				p += strlen(p);

				crackOutputParms(trackParms,p);

				p += strlen(p);
	
				sprintf(p,")");
				break;

			case SAMPLE_INPUT:
				sprintf(	p,
							"sw_sample_input(%d,{ist=%d,its=%d,sample=0x%02x})",
							trackParms->swdrvr,
							(int) trackParms->words[2], 
							(int) trackParms->words[3],
							(int) trackParms->words[4]								);
				break;

			case SAMPLE_INPUT0:
				sprintf(	p,
							"sw_sample_input0(%d,{ist=%d,its=%d,sample=0x%02x})",
							trackParms->swdrvr,
							(int) trackParms->words[2], 
							(int) trackParms->words[3],
							(int) trackParms->words[4]								);
				break;

			case CONFIG_CLOCK:
				sprintf(p,"sw_clock_control(%d,",trackParms->swdrvr);

				p += strlen(p);

				crackClockMode(trackParms->words[2],p);

				p += strlen(p);

				sprintf(p,")");
				break;

			case QUERY_SCM:
				sprintf(p,"sw_clock_scbus_master()");
				break;

			case QUERY_CLOCK_MODE:
				sprintf(	p,
							"sw_query_clock_control(%d,{last_clock_mode=%d,sysinit_clock_mode=%d})",
							trackParms->swdrvr,
							(int) trackParms->words[2], 
							(int) trackParms->words[3]													);
				break;

			case SWVER_SWITCH:
				sprintf(p,"sw_ver_switch(%d,...)",trackParms->swdrvr);
				break;

			case DUMP_SWITCH:
				sprintf(p,"sw_dump_switch(%d,...)",trackParms->swdrvr);
				break;

			case SET_TRACE:
				sprintf(p,"sw_set_trace(%d,...)",trackParms->swdrvr);
				break;

			case TRISTATE_SWITCH:
				sprintf(p,"sw_tristate_switch(%d,%d)",trackParms->swdrvr,(int)(trackParms->words[2]));
				break;

			case SWITCH_OVERRIDE_MODE:
				sprintf(p,"sw_switch_override_mode(%d,%d)",trackParms->swdrvr,(int)(trackParms->words[2]));
				break;

			case H100_CONFIG_BOARD_CLOCK:
				sprintf(	p,
							"sw_h100_config_board_clock(%d,",
							trackParms->swdrvr					);

				p += strlen(p);

				crackH100ClockSource(	(int)(trackParms->words[2]),
										(int)(trackParms->words[3]),
										p								);

				p += strlen(p);

				sprintf(p,",%d,",(int)(trackParms->words[3]));

				p += strlen(p);

				crackH100ClockMode((int)(trackParms->words[4]),p);

				p += strlen(p);

				sprintf(	p,
							",%d,%d)",
							(int)(trackParms->words[5]),
							(int)(trackParms->words[6])		);

				break;

			case H100_CONFIG_NETREF_CLOCK:
				sprintf(p,
						"sw_h100_config_netref_clock(%d,%d,%d,%d)",
						trackParms->swdrvr,
						(int)(trackParms->words[2]),
						(int)(trackParms->words[3]),
						(int)(trackParms->words[4])					);
				break;

			case H100_QUERY_BOARD_CLOCK:
				sprintf(p,"sw_h100_query_board_clock(%d,...)",trackParms->swdrvr);
				break;

			case H100_QUERY_NETREF_CLOCK:
				sprintf(p,"sw_h100_query_netref_clock(%d,...)",trackParms->swdrvr);
				break;

			case MC3_BERT_STATUS:
				sprintf(p,"sw_mc3_bert_status(%d,...)",trackParms->swdrvr);
				break;

			case MC3_CONFIG_BERT:
				sprintf(p,"sw_mc3_config_bert(%d,...)",trackParms->swdrvr);
				break;

			case MC3_BYPASS_FIBRE:
				sprintf(p,"sw_mc3_bypass_fibre(%d,...)",trackParms->swdrvr);
				break;

			case MC3_RING_STATUS:
				sprintf(p,"sw_mc3_ring_status(%d,...)",trackParms->swdrvr);
				break;

			case MC3_EVENT_CRITERIA:
				sprintf(p,"sw_mc3_event_criteria(%d,...)",trackParms->swdrvr);
				break;

			case REGISTER_EVENT:
				sprintf(p,"sw_register_event(%d,...)",trackParms->swdrvr);
				break;

			case SWMODE_SWITCH:
				sprintf(p,"sw_mode_switch(%d,...)",trackParms->swdrvr);
				break;

			case SWCARD_INFO:
				sprintf(p,"sw_card_info(%d,...)",trackParms->swdrvr);
				break;

			case DIAGTRACE:
				sprintf(p,"diagnostic output(%d,%x,%x,%x,%x,%x,%x)",trackParms->swdrvr,(int)(trackParms->words[2]),(int)(trackParms->words[3]),(int)(trackParms->words[4]),(int)(trackParms->words[5]),(int)(trackParms->words[6]),(int)(trackParms->words[7]));
				break;

			default:
				sprintf(p,"sw_??%d(%d)",(int) (trackParms->words[1]),trackParms->swdrvr);
				break;
		}
	}
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
void sw_track_api_calls( int tracking_on )
{
	char 				crackBuffer[128];
	int	 				n;
	int					i;
	struct track_parms 	trackParms[NSWITCH];
	int					oldestSwitchTraceIx;
	time_t				long_time;
	struct tm*			newtime;      
	int					collectionIx;
	int					trackParmsCollectionIx[NSWITCH];

	n = sw_get_drvrs();

	for (i = 0; i < n; i++)
	{
		trackParms[i].is_tracked_data = 0;
	}

	collectionIx = 0;

	do
	{
		oldestSwitchTraceIx = -1;

		for (i = 0; i < n; i++)
		{
			if (trackParms[i].is_tracked_data == 0)
			{
				trackParms[i].tracking_on = tracking_on;

				if (sw_track_switch(i,&trackParms[i]) != 0)
				{
					trackParms[i].is_tracked_data = 0;
				}
				else if (trackParms[i].is_tracked_data)
				{
					trackParmsCollectionIx[i] = collectionIx;

					collectionIx += 1;
				}
			}

			if (trackParms[i].is_tracked_data)
			{
				if (oldestSwitchTraceIx == -1)
				{
					oldestSwitchTraceIx = i;
				}
				else if (trackParms[i].timestamp < trackParms[oldestSwitchTraceIx].timestamp)
				{
					oldestSwitchTraceIx = i;
				}
				else if (	(trackParms[i].timestamp == trackParms[oldestSwitchTraceIx].timestamp)
						 && (trackParmsCollectionIx[i] < trackParmsCollectionIx[oldestSwitchTraceIx]) )
				{
					oldestSwitchTraceIx = i;
				}
			}
		}

		if (oldestSwitchTraceIx != -1)
		{
			if ((trackParms[oldestSwitchTraceIx].tracked_type == kSWTrackTypeAppAPICall))
			{
				crackTrack(&trackParms[oldestSwitchTraceIx],&crackBuffer[0]);

				time( &long_time );

				newtime = localtime( &long_time );

				printf("[%02d:%02d:%02d]%s\n",newtime->tm_hour,newtime->tm_min,newtime->tm_sec,&crackBuffer[0]);
			}

			trackParms[oldestSwitchTraceIx].is_tracked_data = 0;
		}
	} while (oldestSwitchTraceIx != -1);

	fflush(stdout);
}

#endif




/*------------------------------------------------------------*/
/* ACULAB plc                                                 */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : common.c                               */
/*                                                            */
/*           Purpose : Common useful routines                 */
/*                                                            */
/*       Create Date : 21st May 1993                          */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Change History                                             */
/*                                                            */
/*                                                            */
/* rev:  1.15   12/02/2000   labelled for 5.4.0 Release       */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/

#include "mvcldrvr.h"
#define SW_IOCTL_CODES
#include "mvswdrvr.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>


#define FALSE  0
#define TRUE   1


/*----------- Function Prototypes ---------------*/
/*                                               */
/* defined in mvcldrvr.h                         */
/*                                               */
/*-----------------------------------------------*/

extern int verbose;


static int timeslot_valid ( int, int );
static int get_info       ( void );
int network_side   ( int  );
static void prev5clkinit  ( int  );
static void v5clkinit     ( int  );
static int  Scbusmode     ( void );


ACUDLL void   port_init          ( int );
ACUDLL int    set_idle           ( int );
ACUDLL int    set_sigsys         ( int );



typedef struct ncfg {
		    int    nettype;
		    char * typep;
		    char * namep;
		    char * filep;
		    } NCFG;


		/* E1 ISDN Primary Rate */

NCFG ncfg[] = { S_1TR6,       "S_1TR6",      "1TR6",    "1TR6_USR.RAM",
		S_1TR6NET,    "S_1TR6NET",   "1TR6NET", "1TR6_NET.RAM",
		S_AUSTEL,     "S_AUSTEL",    "AUSTEL",  "AUST_USR.RAM",
		S_AUSTNET,    "S_AUSTNET",   "AUSTNET", "AUST_NET.RAM",
		S_ETS300,     "S_ETS300",    "ETS300",  "ETS_USR.RAM",
		S_ETSNET,     "S_ETSNET",    "ETSNET",  "ETS_NET.RAM",
		S_ETS300_T1,  "S_ETS300_T1", "ETS300 T1","ETSTSUPU.RAM",
		S_ETSNET_T1,  "S_ETSNET_T1", "ETSNET T1","ETSTSUPN.RAM",
		S_SWETS300,   "S_SWETS300",  "SWED300", "SWED_USR.RAM",
		S_SWETSNET,   "S_SWETSNET",  "SWEDNET", "SWED_NET.RAM",
		S_VN3,        "S_VN3",       "VN3",     "VN3_USR.RAM",
		S_VN3NET,     "S_VN3NET",    "VN3NET",  "VN3_NET.RAM",
		S_TNA_NZ,     "S_TNA_NZ",    "TNA_NZ",  "TNA_USR.RAM",
		S_TNANET,     "S_TNANET",    "TNA_NET", "TNA_NET.RAM",
		S_FETEX_150,  "S_FETEX_150", "FETX150", "FETX_USR.RAM",
		S_FETEXNET,   "S_FETEXNET",  "FETXNET", "FETX_NET.RAM",
		S_ATT,        "S_ATT",       "AT&T",    "ATT_EUSR.RAM",
		S_ATTNET,     "S_ATTNET",    "AT&TNET", "ATT_ENET.RAM",
		S_ATT_T1,     "S_ATT_T1",    "ATT1",    "ATT_TUSR.RAM",
		S_ATTNET_T1,  "S_ATTNET_T1", "ATT1NET", "ATT_TNET.RAM",
		S_DASS,       "S_DASS",      "DASS2",   "DASS_USR.RAM",
		S_DASSNET,    "S_DASSNET",   "DASSNET", "DASS_NET.RAM",
		S_DPNSS,      "S_DPNSS",     "DPNSS",   "M1DPNSS.RAM",
		S_QSIG,       "S_QSIG",      "QSIG",    "QSIG.RAM",

		/* CAS signalling Systems */

		S_CAS,        "S_CAS",       "R2B2P",   "M1R2P2B.RAM",
		S_CAS,        "S_CAS",       "CAS",     "M1R2P2B.RAM",
		S_CAS,        "S_CAS",       "BTCU",    "M1BTCU.RAM",
		S_CAS,        "S_CAS",       "BTCN",    "M1BTCN.RAM",
		S_CAS,        "S_CAS",       "PTVU",    "M1PTVU.RAM",
		S_CAS,        "S_CAS",       "PTVN",    "M1PTVN.RAM",
		S_CAS,        "S_CAS",       "PD1D",    "M1PD1D.RAM",
		S_CAS,        "S_CAS",       "PD1U",    "M1PD1U.RAM",
		S_CAS,        "S_CAS",       "PD1N",    "M1PD1N.RAM",
		S_CAS,        "S_CAS",       "R2L",     "M1R2L.RAM",
		S_CAS,        "S_CAS",       "P8",      "M1P8.RAM",
		S_CAS,        "S_CAS",       "EM",      "M1EM.RAM",
		S_CAS,        "S_CAS",       "BEZEQ",   "M1BEZEQ.RAM",

		/* tone signalling system */

		S_CAS_TONE,   "S_CAS_TONE",  "R2T",     "M1R2T.RAM",
		S_CAS_TONE,   "S_CAS_TONE",  "R2T1",    "M1R2T1.RAM",
		S_CAS_TONE,   "S_CAS_TONE",  "ALSU",    "M1ALSUT.RAM",
		S_CAS_TONE,   "S_CAS_TONE",  "ALSN",    "M1ALSNT.RAM",
		S_CAS_TONE,   "S_CAS_TONE",  "BELGU",   "M1BELGU.RAM",
		S_CAS_TONE,   "S_CAS_TONE",  "BELGN",   "M1BELGN.RAM",
		S_CAS_TONE,   "S_CAS_TONE",  "EFRAT",   "M1EFRAT.RAM",
		S_CAS_TONE,   "S_CAS_TONE",  "EEMA",    "M1EEMA.RAM",
		S_CAS_TONE,   "S_CAS_TONE",  "PD1",     "M1PD1.RAM",
		S_CAS_TONE,   "S_CAS_TONE",  "PD1DD",   "M1PD1DD.RAM",
		S_CAS_TONE,   "S_CAS_TONE",  "PD1UD",   "M1PD1UD.RAM",
		S_CAS_TONE,   "S_CAS_TONE",  "PD1ND",   "M1PD1ND.RAM",
		S_CAS_TONE,   "S_CAS_TONE",  "BTMC",    "M1BTMC.RAM",
		S_CAS_TONE,   "S_CAS_TONE",  "OTE2",    "M1OTE2.RAM",
		S_CAS_TONE,   "S_CAS_TONE",  "FMFS",    "M1FMFS.RAM",
		S_CAS_TONE,   "S_CAS_TONE",  "SMFS",    "M1SMFS.RAM",
		S_CAS_TONE,   "S_CAS_TONE",  "I701",    "M1I701.RAM",

		S_SS5_TONE,   "S_SS5_TONE",  "SS5",     "M1SS5.RAM",

		/* T1 - ISDN Signalling Systems */

		S_IDAP,       "S_IDAP",      "IDAP",    "IDAP_USR.RAM",
		S_IDAPNET,    "S_IDAPNET",   "IDAPNET", "IDAP_NET.RAM",
		S_NI2,        "S_NI2",       "NI2",     "NI2_USR.RAM",
		S_NI2NET,     "S_NI2NET",    "NI2NET",  "NI2_NET.RAM",

		S_INS,        "S_INS",       "INS",     "INS_USR.RAM",
		S_INSNET,     "S_INSNET",    "INSNET",  "INS_NET.RAM",

		/* T1 - CAS tone signalling */

		S_T1CAS_TONE, "S_T1CAS_TONE","F12",     "T1RB_USR.RAM",

		/* Basic Rate Signalling Systems */

		BR_ETS300,    "BR_ETS300",   "BETS300", "ETS_BUSR.RAM",
		BR_ETSNET,    "BR_ETSNET",   "BETSNET", "ETS_BNET.RAM",
		BR_NI1,       "BR_NI1",      "BNI1",    "NI1_BUSR.RAM",
		BR_NI1NET,    "BR_NI1NET",   "BNI1NET", "NI1_BNET.RAM",
		BR_ATT,       "BR_ATT",      "BATT",    "ATT_BUSR.RAM",
		BR_ATTNET,    "BR_ATTNET",   "BATTNET", "ATT_BNET.RAM",

		/* SS7 signalling */
		S_ISUP,       "S_ISUP",      "ISUP",    "ISUP.RAM",

		/* end of table */

		0,            "",            "",        ""  };      /* end of table */



static struct siginfo_xparms info[MAXPORT];

static int old_clock = 0;

/*-----------------------------------------------------*/
/*------------ System Initialisation Function ---------*/
/*-----------------------------------------------------*/




/*------------- v5clkinit ---------------*/
/* configure default clocks in system    */
/*                                       */

static void v5clkinit( int n )
{
	int                         swnum;
	struct query_clkmode_parms  swqclk_parms;
	struct sysinfo_xparms       sysinfo;
	int                         drivingMVIP;
	int                         drivingSCBus;
	int                         clockmode;
	int                         port;
	int                                                     initialLoneSwitchCount;
	int                                                     inInitialSwitchRun;
#ifdef SWMODE_SWITCH
	struct swmode_parms             swmode;
#endif

	initialLoneSwitchCount = 0;
	inInitialSwitchRun     = 1;

	for (swnum = 0; swnum < n; swnum++)
	{
		if (inInitialSwitchRun)
		{
#ifdef SWMODE_SWITCH
			if (sw_mode_switch(swnum,&swmode) == 0)
			{
				if ((swmode.ct_buses & (1<<SWMODE_CTBUS_MC3)) != 0)
				{
					initialLoneSwitchCount += 1;
				}
				else
				{
					inInitialSwitchRun = 0;
				}
			}
			else
			{
				inInitialSwitchRun = 0;
			}
#else
			inInitialSwitchRun = 0;
#endif
		}

	sw_query_clock_control(swnum,&swqclk_parms);

	drivingMVIP  = (      (((swqclk_parms.last_clock_mode & ~DRIVE_SCBUS)) != CLOCK_REF_MVIP)
			  &&  (((swqclk_parms.last_clock_mode & ~DRIVE_SCBUS)) != CLOCK_PRIVATE)
			  &&  (((swqclk_parms.last_clock_mode & HI_MVIPCLK))   == 0)                  );

	drivingSCBus = ((swqclk_parms.last_clock_mode & DRIVE_SCBUS) != 0);

	if (swnum != 0)
	{
		if (drivingMVIP || drivingSCBus)
	    {
		sw_clock_control ( swnum, CLOCK_PRIVATE );
	    }
	}
    }

	if (initialLoneSwitchCount != 0)
	{
		call_set_net0_swnum(initialLoneSwitchCount);
	}

	port = 0;

	for ( swnum = 0; swnum < n; swnum++ )
    {
		sw_query_clock_control(swnum,&swqclk_parms);

	if (swqclk_parms.sysinit_clock_mode == -1)
	{
			if (swnum == 0)
	    {
		if ( network_side ( 0 ))           /* check for network side protocol */
		{
			clockmode = CLOCK_REF_LOCAL;    /* Network supplies clock */
		}
		else
		{
			clockmode = CLOCK_REF_NET1;     /* User references clock */
		}

		if ( Scbusmode ( ))                /* check if card in SCbus Mode */
		{
			clockmode += DRIVE_SCBUS;       /* default to driving the SCbus Clock */
		}
	    }
		else
	    {
		clockmode = CLOCK_REF_MVIP;     /* User references clock */
	    }
		}
	else
	{
			clockmode = swqclk_parms.sysinit_clock_mode;
	}

	sw_clock_control ( swnum, clockmode );  /* set reference the bus */

	sysinfo.net = port;

	call_system_info ( &sysinfo );

	port += sysinfo.phys_port;

	sw_reset_switch ( swnum );      /* result all switch drivers */

	sw_reinit_switch ( swnum );     /* re-initialise switch */
	}
}


/*------------- port_init ---------------*/
/* initialise switch and call systems    */
/*                                       */
ACUDLL void port_init ( int portnum )
{
    set_idle   ( portnum );         /* idle the time slot */
    set_sigsys ( portnum );         /* set up the signalling system streams */
}

/*------------- system_init -------------*/
/* initialise switch and call systems    */
/*                                       */
ACUDLL int system_init ( void )
   {
   int                         result;
   int                         portnum;
   int                         nswitch;
   int                         nports;
   struct init_xparms          init_xparms;
	
   init_xparms.ournum[0]    = '\0';
   init_xparms.responsetime = -1;

   result = call_init ( &init_xparms );     /* set up the drivers */


   if ( (result == 0) || ( result==ERR_DRV_CALLINIT ))
      {
      result = 0;
      nswitch = sw_get_drvrs ( );           /* get the number of switch drivers */

      if ( nswitch >= 0  )                  /* check for error */
	 {

	 v5clkinit(nswitch);

	 old_clock = 0;                     /* last port used was 0 */

	 nports = call_nports ( );          /* get the number of network ports */

	 for ( portnum = 0; portnum < nports; portnum++ )
	    {
	    port_init(portnum) ;
	    }
	 }
      else
	 {
	 result = nswitch;              /* return error */
	 }
      }

   return ( result );

   }
/*---------------------------------------*/


/*---------- network_side ---------------*/
/*check if the protocol is a network side*/
/*                                       */
/* returns TRUE if network side          */
/*                                       */
int network_side ( int portnum )
   {
   switch ( call_type ( portnum ))      /* check for network end */
      {
      case S_1TR6NET:   
      case S_AUSTNET:   
      case S_ETSNET:
      case S_ETSNET_T1:
      case S_SWETSNET:  
      case S_VN3NET:    
      case S_TNANET:    
      case S_FETEXNET:  
      case S_ATTNET:    
      case S_ATTNET_T1: 
      case S_DASSNET:   
      case S_IDAPNET:   
      case S_NI2NET:    
      case S_INSNET:    
      case BR_ETSNET:   
      case BR_NI1NET:   
      case BR_ATTNET:   
	 return TRUE;

      default:
	 return FALSE;
      }
   }
/*---------------------------------------*/

/*------------- Scbusmode ---------------*/
/* check if the switch driver is in SCbus*/
/* or PEB mode                           */
/*                                       */
/* Returns TRUE for SCbus Mode           */
/*                                       */
static int Scbusmode ( void )
   {
   int  error;
   struct output_parms query;

#ifdef SWMODE_SWITCH
    struct swmode_parms swmode;

	if (sw_mode_switch(0,&swmode) == 0)
	{
		if ((swmode.ct_buses & (1<<SWMODE_CTBUS_SCBUS)) != 0)
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	else
#endif
	{
   /* Talk to the switch driver using the SCbus stream        */
   /* If the switch driver returns error then assume PEB mode */

   query.ost  = 24;                        /* use the SCbus Stream */
   query.ots  =  0;                        /* use any timeslot */

   error = sw_query_output ( 0, &query );  /* set up switch */

   if ( error == 0 )
      {
      return ( TRUE );
      }
   else
      {
      return ( FALSE );
      }
   }
   }
/*---------------------------------------*/

/*-----------------------------------------------------*/
/*---------------- Clock Control Functions ------------*/
/*-----------------------------------------------------*/

/*------------- swap_clock --------------*/
/* change clocks between cards which are */
/* interconnected by the MVIP bus.       */
/*                                       */
/* The procedure is described in:        */
/*                                       */
/* Application Note 1 - Card Clocking    */
/* issues in Complex Networks            */
/*                                       */
/* The function assumes that the clock is*/
/* always changed using swap_clock()     */
/*                                       */
ACUDLL void swap_clock ( int portnum )
   {
   int  new_swdrvr, old_swdrvr;
   int  new_stream;


   if ( portnum != old_clock )
      {
      new_swdrvr = call_port_2_swdrvr ( portnum );
      new_stream = call_port_2_stream ( portnum );
      old_swdrvr = call_port_2_swdrvr ( old_clock );

      if ( new_swdrvr == 0 )
	 {
	 if ( new_swdrvr != old_swdrvr )
	    {
	    sw_clock_control (old_swdrvr, CLOCK_REF_MVIP);
	    }

	 if ( new_stream != kSW_LI_E1_NET2 )
	    {
	    sw_clock_control (new_swdrvr, CLOCK_REF_NET1);
	    }
	 else
	    {
	    sw_clock_control (new_swdrvr, CLOCK_REF_NET2);
	    }
	 }
      else
	 {
	 if ( new_swdrvr != old_swdrvr )
	    {
	    if ( old_swdrvr==0 )
	       {
	       sw_clock_control (old_swdrvr, CLOCK_REF_SEC8K);
	       }
	    else
	       {
	       sw_clock_control (old_swdrvr, CLOCK_REF_MVIP);
	       }
	    }

	 if ( new_stream != kSW_LI_E1_NET2 )
	    {
	    sw_clock_control (new_swdrvr, SEC8K_DRIVEN_BY_NET1);
	    }
	 else
	    {
	    sw_clock_control (new_swdrvr, SEC8K_DRIVEN_BY_NET2);
	    }
	 }
      }

   old_clock = portnum;                 /* update new source */
   }
/*---------------------------------------*/

/*-----------------------------------------------------*/
/*------------- Auxillary Switch functions ------------*/
/*-----------------------------------------------------*/

/*--------------- set_sigsys -------------*/
/* sets up the signalling system streams  */
/* and timeslots for the network provided */
/*                                        */
/* For the R2 tone signalling, nail up all*/
/* the network streams to the HDLC stream */
/*                                        */
ACUDLL int set_sigsys ( int portnum )
   {
   int  dspts, ts;
   int  stream;
   int  swdrvr;
   int  result = 0;
   struct output_parms output;
   struct tsinfo_xparms tsinfo;
   struct sysinfo_xparms sysinfo;
   int sig_ts = -1; /*Not yet known */
   long validvector = 0L;
   long signalvector = 0L;

   sysinfo.net = portnum;
   result = call_system_info ( &sysinfo );

   if (result != 0)
      return ( result );

   switch (sysinfo.cardtype)
      {
      case C_PM4:

	 /* first get the stream number from the   */
	 /* port number, and then get the switch   */
	 /* driver number from network port number */

	 swdrvr = call_port_2_swdrvr ( portnum );           /* get switch number */
	 stream = call_port_2_stream ( portnum );           /* get stream number 16, 18 */


	 /* Now see if the driver can tell us which timeslot to use for the d-channel */
	 /* or if we should default it.... */
	 tsinfo.net = portnum;
	 tsinfo.modify = 0;

	 if (call_tsinfo(&tsinfo) == 0) 
	    {
	    validvector = tsinfo.validvector ;
	    signalvector = tsinfo.signalvector ;
	    }

	 if (signalvector) /* Firmware has requested specific signal vector */
	    {
	    for (sig_ts = 0 ; sig_ts < 32 ; sig_ts++)
	       if (signalvector & 1)
		  break;
	       else
		   signalvector>>= 1;
	    }

	 switch ( call_line ( portnum ) )
	    {
	    /* setup for T1 - ISDN cards */

	    case L_T1_ISDN:
	       break;

	    /* setup for T1 - ISDN cards */

	    case L_T1_CAS:
	       break;

	    /* setup for E1 cards */

	    case L_E1:

	       for ( ts = 1; ts < 32 && result == 0; ts++ )
		  {
		  int x_ts;

		  /* Check for Timeslot 16 OR */
		  /* check if tone signalling */

		  if ( ts == 16)
		     continue;      /* Skip HDLC channel for PM4 */
		  else if (is_tone ( portnum ))
		     x_ts = ts;
		  else
		     continue;

		  output.ost  = (ACU_INT) stream;
		  output.ots  = (ACU_INT) ts;
		  output.ist  = (ACU_INT) stream + 8;          /* network stream number */
		  output.its  = (ACU_INT) x_ts;                /* network timeslot number */
		  output.mode = CONNECT_MODE;                  /* connect a timeslot */
		  result = sw_set_output ( swdrvr, &output );  /* set up switch */

		  /* now to make a bi-directional connection */

		  output.ost  = (ACU_INT) stream + 8;          /* network stream number */
		  output.ots  = (ACU_INT) x_ts;                /* network timeslot number */
		  output.ist  = (ACU_INT) stream;
		  output.its  = (ACU_INT) ts;
		  output.mode = CONNECT_MODE;
		  result |= sw_set_output ( swdrvr, &output ); /* set up switch */

		  }
	    break;
	    }
      break;

      case C_REV4:
      case C_REV5:
      case C_BR4:
      case C_BR8:

	 /* first get the stream number from the   */
	 /* port number, and then get the switch   */
	 /* driver number from network port number */

	 swdrvr = call_port_2_swdrvr ( portnum );           /* get switch number */
	 stream = call_port_2_stream ( portnum );           /* get stream number 16, 18 */


	 /* Now see if the driver can tell us which timeslot to use for the d-channel */
	 /* or if we should default it.... */
	 tsinfo.net = portnum;
	 tsinfo.modify = 0;
	 if ( (call_tsinfo(&tsinfo) == 0) && tsinfo.signalvector)
	    {
	    unsigned long vector = tsinfo.signalvector ;
	    /* Use the lowest bit set in signal vector as the sig channel */

	    for (sig_ts = 0 ; sig_ts < 32 ; sig_ts++)
	       if (vector & 1)
		  break;
	       else
		   vector>>= 1;
	    }

	 switch ( call_line ( portnum ) )
	    {
	    /* setup for T1 - ISDN cards */

	    case L_T1_ISDN:

	       /* If we haven't yet got a signalling timeslot, and ts 23 is
	       available (not used as a bearer), then use ts 23 for signalling 
	       else it must be an NFAS bearer-only trunk with no sig. ts.  */

	       if ( (sig_ts <0) && ( !(validvector & (1L << 23))))
		 sig_ts = 23 ;

	       output.ost  = (ACU_INT) stream;
	       output.ots  = (ACU_INT) sig_ts;
	       output.ist  = (ACU_INT) stream + 1;              /* network stream number */
	       output.its  = (ACU_INT) 16;                      /* network timeslot number */
	       output.mode = CONNECT_MODE;                  /* connect a timeslot */
	       result = sw_set_output ( swdrvr, &output );  /* set up switch */

	       /* now to make a bi-directional connection */

	       output.ost  = (ACU_INT) stream + 1;              /* network stream number */
	       output.ots  = (ACU_INT) 16;                      /* network timeslot number */
	       output.ist  = (ACU_INT) stream;
	       output.its  = (ACU_INT) sig_ts;
	       output.mode = CONNECT_MODE;
	       result |= sw_set_output ( swdrvr, &output ); /* set up switch */
	    break;

	    /* setup for T1 - CAS cards */

	    case L_T1_CAS:
	       for ( ts = 0, dspts = 0; ts < 24 && result == 0; ts++ )
		  {
		  if ( is_tone ( portnum ))
		     {
		     dspts = ts;

		     if ( ts >= 16 )
			{
			dspts++;
			}

		     output.ost  = (ACU_INT) stream;
		     output.ots  = (ACU_INT) ts;
		     output.ist  = (ACU_INT) stream + 1;              /* network stream number */
		     output.its  = (ACU_INT) dspts;                   /* network timeslot number */
		     output.mode = CONNECT_MODE;                  /* connect a timeslot */
		     result = sw_set_output ( swdrvr, &output );  /* set up switch */

		     /* now to make a bi-directional connection */

		     output.ost  = (ACU_INT) stream + 1;              /* network stream number */
		     output.ots  = (ACU_INT) dspts;                   /* network timeslot number */
		     output.ist  = (ACU_INT) stream;
		     output.its  = (ACU_INT) ts;
		     output.mode = CONNECT_MODE;
		     result |= sw_set_output ( swdrvr, &output ); /* set up switch */
		     }
		  }
	    break;

	    /* setup for E1 cards */

	    case L_E1:
	       /* If we haven't yet got a signalling timeslot, and ts 16 is
	       available (not used as a bearer), then use ts 16 for signalling 
	       else it must be an NFAS bearer-only trunk with no sig. ts.  */

	       if ( (sig_ts <0) && ( !(validvector & (1L << 16))))
		 sig_ts = 16 ;

	       for ( ts = 1; ts < 32 && result == 0; ts++ )
		  {
		  int x_ts;

		  /* Check for Timeslot 16 OR */
		  /* check if tone signalling */

		  if ( ts == sig_ts)
		      x_ts = 16;  /* HDLC channel */
		  else if (is_tone ( portnum ))
		      x_ts = ts ;
		  else
		     continue;

		  output.ost  = (ACU_INT) stream;
		  output.ots  = (ACU_INT) ts;
		  output.ist  = (ACU_INT) stream + 1;              /* network stream number */
		  output.its  = (ACU_INT) x_ts;                      /* network timeslot number */
		  output.mode = CONNECT_MODE;                  /* connect a timeslot */
		  result = sw_set_output ( swdrvr, &output );  /* set up switch */

		  /* now to make a bi-directional connection */

		  output.ost  = (ACU_INT) stream + 1;              /* network stream number */
		  output.ots  = (ACU_INT) x_ts;                      /* network timeslot number */
		  output.ist  = (ACU_INT) stream;
		  output.its  = (ACU_INT) ts;
		  output.mode = CONNECT_MODE;
		  result |= sw_set_output ( swdrvr, &output ); /* set up switch */
		  }
	    break;

	    /* basic rate */

	    case L_BASIC_RATE:
	       result = 0;
	    break;
	    }
      }

   return ( result );
   }
/*---------------------------------------*/

/*--------------- set_idle --------------*/
/* set all time slots to idle on a given */
/* network port                          */
/*                                       */
ACUDLL int set_idle ( int portnum )
   {
   int  result = 0;
   int  ts;


   for ( ts = 0; ts < 32; ts++ )
      {
      if ( timeslot_valid ( portnum, ts ))   /* check if valid timeslot */
	 {
	 idle_net_ts ( portnum, ts );
	 }
      }

   return ( result );
   }
/*---------------------------------------*/


/*--------------- nailup ----------------*/
/* nailup the incoming network timeslots */
/* to the PEB bus or MVIP bus            */
/*                                       */
/* NOTE                                  */
/* ----                                  */
/* The nailup stream may be either the   */
/* MVIP or PEB bus streams               */
/*                                       */
ACUDLL int nailup ( int portnum, int nailup_stream )
   {
   struct output_parms output;          /* switch structure      */
   int  ts;                             /* timeslot number       */
   int  swdrvr;                         /* switch driver number  */
   int  net_st;                         /* network stream number */
   int  result = 0;


   net_st = call_port_2_stream ( portnum );    /* get network stream number */

   if ( net_st < 0 )                           /* check for errors  */ 
      {
      result = net_st;                         /* pick up the error code */ 
      }
   else
      {
      swdrvr = call_port_2_swdrvr ( portnum ); /* get network switch number */

      if ( swdrvr < 0 )                        /* check for errors  */ 
	 {
	 result = swdrvr;                      /* pick up the error code */
	 }
      else
	 {
	 for ( ts = 0; ts < 32; ts++ )
	    {
	    if ( timeslot_valid ( portnum, ts ))
	       {
	       output.ost  = nailup_stream;    /* PEB or MVIP stream  */
	       output.ots  = ts;               /* associated timeslot */
	       output.ist  = net_st;           /* network port stream */
	       output.its  = ts;               /* associated timeslot */
	       output.mode = CONNECT_MODE;     /* make a connection   */

	       sw_set_output ( swdrvr, &output );
	       }
	    }
	 }
      }

   return ( result );

   }
/*---------------------------------------*/

/*------------ handle_switch ------------*/
/*do the connection of the digital switch*/
/*MVIP bus only*/
/*                                       */
ACUDLL void handle_switch ( struct detail_xparms * detailsp, int your_mvip_stream, int your_mvip_timeslot )
   {
   struct output_parms output;
   int  swdrvr;
   int  portnum;

   /* first get the network port number from   */
   /* the call handle, and then get the switch */
   /* driver number from network port number   */

   portnum = call_handle_2_port ( detailsp->handle );
   swdrvr  = call_port_2_swdrvr ( portnum );

   output.ist  = detailsp->stream;      /* network stream number */
   output.its  = detailsp->ts;          /* network timeslot number */
   output.ost  = your_mvip_stream;
   output.ots  = your_mvip_timeslot;
   output.mode = CONNECT_MODE;          /* connect a timeslot */
   sw_set_output ( swdrvr, &output );   /* set up switch */

   /* now to make a bi-directional connection */

   output.ost  = detailsp->stream;      /* network stream number */
   output.ots  = detailsp->ts;          /* network timeslot number */
   output.ist  = your_mvip_stream;
   output.its  = your_mvip_timeslot;
   output.mode = CONNECT_MODE;
   sw_set_output ( swdrvr, &output );   /* set up switch */
   }
/*---------------------------------------*/

/*-------------- idle_net_ts ------------*/
/*now idle the network stream            */
/*                                       */
ACUDLL void idle_net_ts ( int portnum, int timeslot )
   {
   struct output_parms   output;
   struct sysinfo_xparms sysinfo;
   int  swdrvr;
   int  stream;
   int  dspts;
   int  dspstreamoffset;
   int  result;
   ACU_UCHAR idle_pattern;

   sysinfo.net = portnum;
   result = call_system_info ( &sysinfo );

   if (result != 0)
      return;

   switch (sysinfo.cardtype)
      {
      case C_PM4:
	 dspstreamoffset= 8;
	 idle_pattern = IDLE;
      break;

      case C_BR4:
      case C_BR8:
	 dspstreamoffset= 1;
	 idle_pattern = 0xff;           /* basic rate idle pattern (0xff) */
      break;

      case C_REV4:
      case C_REV5:
      default:
	 dspstreamoffset= 1;
	 idle_pattern = IDLE;           /* idle pattern (ccitt 0x54) */
      break;
      }

   if ( portnum < call_nports ( ) )
      {
      /* first get the stream number from the   */
      /* port number, and then get the switch   */
      /* driver number from network port number */

      stream = call_port_2_stream ( portnum );
      swdrvr = call_port_2_swdrvr ( portnum );

      /* if its tone signalling then reconnect the DSP */
      /* otherwise send the idle pattern               */

      switch ( call_type ( portnum ) )
	 {
	 /* T1 CAS Tone Signalling */

	 case S_T1CAS_TONE:
	    dspts = timeslot;

	    if ( timeslot >= 16 )
	       {
	       dspts++;
	       }

	    output.ost  = stream;
	    output.ots  = timeslot;
	    output.ist  = stream + dspstreamoffset;          /* network stream number */
	    output.its  = dspts;                             /* network timeslot number */
	    output.mode = CONNECT_MODE;                      /* connect a timeslot */
	    sw_set_output ( swdrvr, &output );               /* set up switch */

	    /* now to make a bi-directional connection */

	    output.ost  = stream + dspstreamoffset;          /* network stream number */
	    output.ots  = dspts;                             /* network timeslot number */
	    output.ist  = stream;
	    output.its  = timeslot;
	    output.mode = CONNECT_MODE;
	    sw_set_output ( swdrvr, &output ); /* set up switch */
	 break;


	 case S_CAS_TONE:
	 case S_SS5_TONE:
	    output.ost  = stream;
	    output.ots  = timeslot;
	    output.ist  = stream + dspstreamoffset;          /* network stream number */
	    output.its  = timeslot;                          /* network timeslot number */
	    output.mode = CONNECT_MODE;                      /* connect a timeslot */
	    sw_set_output ( swdrvr, &output );               /* set up switch */

	    /* now to make a bi-directional connection */

	    output.ost  = stream + dspstreamoffset;          /* network stream number */
	    output.ots  = timeslot;                          /* network timeslot number */
	    output.ist  = stream;
	    output.its  = timeslot;
	    output.mode = CONNECT_MODE;
	    sw_set_output ( swdrvr, &output );               /* set up switch */
	 break;

	 default:
	    output.ost  = stream;                            /* network stream number */
	    output.ots  = timeslot;                          /* network timeslot number */
	    output.mode = PATTERN_MODE;                      /* connect a timeslot */
	    output.pattern = idle_pattern;                   /* idle pattern */
	    sw_set_output ( swdrvr, &output );               /* set up switch */
	 break;
	 }
      }
   }
/*---------------------------------------*/

/*------------ verify_ddi ---------------*/
/*check that we support the ddi digits   */
/*                                       */
/*This function is called to check if our*/
/*database can support the received ddi  */
/*digits.                                */
/* In our case we always return TRUE     */
/*                                       */
ACUDLL int verify_ddi ( char * ddip )
   {
   ddip = ddip;

   return ( TRUE );
   }
/*---------------------------------------*/

/*---------------- is_tone --------------*/
/* check if tone signalling              */
/* return FALSE if not tone signalling   */
/*                                       */
ACUDLL int is_tone ( int portnum )
   {
   int  result;
   
   result = call_type ( portnum );
   
   if ( result >= 0 )
      {
      if ( result == S_CAS_TONE   || 
	   result == S_T1CAS_TONE ||
	   result == S_SS5_TONE )
	 {
	 result = TRUE;
	 }
      else
	 {
	 result = FALSE;
	 }
      }
   
   return ( result );
   }
/*---------------------------------------*/

/*------------ port_2_filename ----------*/
/* return the file name for the network  */
/* port                                  */
/*                                       */
ACUDLL char * port_2_filename ( int portnum )
   {
   int  type, line;
   int  i;
   static char noname[] = { "" };
   char * filep = noname;


   type = call_type ( portnum );               /* get network type */
   line = call_line ( portnum );

   if ( type != S_CAS        && 
	type != S_CAS_TONE   &&
	type != S_SS5_TONE   &&
	type != S_T1CAS      &&
	type != S_T1CAS_TONE &&
	type != S_BASE       &&
	line != L_BASIC_RATE )
      {
      for ( i = 0; ncfg[i].nettype != 0; i++ ) /* scan for type */
	 {
	 if ( ncfg[i].nettype == type )
	    {
	    filep = ncfg[i].filep;             /* pick up the filename */
	    break;
	    }
	 }
      }
   else
      {
      /*------------------------------------------------*/
      /* CAS and BASIC RATE are special cases. There is */
      /* one CAS driver and one Basic Rate driver that  */
      /* support several types of signalling system     */
      /* the user must therefore specify the file name  */
      /*------------------------------------------------*/

      if ( verbose )
	 {
	 printf ( "\r\nMvcldnld cannot determine the file to load, please specify" );
	 }
      }

   return ( filep );

   }
/*---------------------------------------*/


/*---------- sigtype_2_string -----------*/
/* return a string representing the      */
/* sigtype running on the card           */
/*                                       */
ACUDLL char * sigtype_2_string ( int sigtype )
   {
   char * typep = "Unknown Type";
   int  i;


   for ( i = 0; ncfg[i].nettype != 0; i++ ) /* scan for type */
      {
      if ( ncfg[i].nettype == sigtype )
	 {
	 typep = ncfg[i].typep;             /* pick up the filename */
	 break;
	 }
      }

   return ( typep );

   }
/*---------------------------------------*/


/*--------- timeslot_valid --------------*/
/* check if timeslot is valid            */
/* returns TRUE of FALSE                 */
/*                                       */
static int timeslot_valid ( int portnum, int timeslot )
   {
   int  result;


   result = get_info ( );

   if ( result == 0 )
      {
      if ( info[portnum].validvector & (1L << timeslot ))
	 {
	 result = TRUE;
	 }
      else
	 {
	 result = FALSE;
	 }
      }
   else
      {
      result = FALSE;
      }

   return ( result );

   }
/*---------------------------------------*/


/*------------- get_info ----------------*/
/* get the signalling information        */
/*                                       */
static int get_info ( void )
   {
   int  result = 0;
   static int  info_available = FALSE;

   
   if ( ! info_available )
      {
      result = call_signal_info ( &info[0] );  /* get signalling information */

      if ( result == 0 )
	 {
	 info_available = TRUE;
	 }
      }

   return ( result );
   }
/*---------------------------------------*/

/*------------error_2_string-------------*/
/* convert error code to string          */
/*                                       */
ACUDLL char * error_2_string( int error )
{
	char* error_string;

	switch(error)
       {
       /************************/
       /* Miscellaneous Errors */
       /************************/
       case ERR_HANDLE:
	  error_string="ERR_HANDLE - Illegal handle, or out of handles";
       break;
       case ERR_COMMAND:
	  error_string="ERR_COMMAND - Illegal command";
       break;
       case ERR_NET:
	  error_string="ERR_NET - Illegal network number";
       break;
       case ERR_PARM:
	  error_string="ERR_PARM - Illegal parameter";
       break;
       case ERR_RESPONSE:
	  error_string="ERR_RESPONSE - ";
       break;
       case ERR_NOCALLIP:
	  error_string="ERR_NOCALLIP - No call in progress";
       break;
       case ERR_TSBAR:
	  error_string="ERR_TSBAR - Timeslot barred";
       break;
       case ERR_TSBUSY:
	  error_string="ERR_TSBUSY - Timeslot busy";
       break;
       case ERR_CFAIL:
	  error_string="ERR_CFAIL - Command failed to execute";
       break;
       case ERR_SERVICE:
	  error_string="ERR_SERVICE - Invalid service code";
       break;
       case ERR_BUFF_FAIL:
	  error_string="ERR_BUFF_FAIL - Out of buffer resources";
       break;
       /*******************/
       /* Download Errors */
       /*******************/
       case ERR_DNLD_ZAP:
	  error_string="ERR_DNLD_ZAP - Debug monitor running";
       break;
       case ERR_DNLD_NOCMD:
	  error_string="ERR_DNLD_NOCMD - Firmware not loaded - command denied";
       break;
       case ERR_DNLD_NODNLD:
	  error_string="ERR_DNLD_NODNLD - Firmware installed - download denied";
       break;
       case ERR_DNLD_GEN:
	  error_string="ERR_DNLD_GEN - General failure during download";
       break;
       case ERR_DNLD_NOSIG:
	  error_string="ERR_DNLD_NOSIG - Download firmware failed to start";
       break;
       case ERR_DNLD_NOEXEC:
	  error_string="ERR_DNLD_NOEXEC - Download firmware is not executing";
       break;
       case ERR_DNLD_NOCARD:
	  error_string="ERR_DNLD_NOCARD - Download firmware is not executing";
       break;
       case ERR_DNLD_SYSTAT:
	  error_string="ERR_DNLD_SYSTAT - Download firmware detected an error";
       break;
       case ERR_DNLD_BADTLS:
	  error_string="ERR_DNLD_BADTLS - Driver does not support the firmware";
       break;
       case ERR_DNLD_POST:
	  error_string="ERR_DNLD_POST - Board failed power on self test";
       break;
       case ERR_DNLD_SW:
	  error_string="ERR_DNLD_SW - Switch setup error";
       break;
       case ERR_DNLD_MEM:
	  error_string="ERR_DNLD_MEM - Could not allocate memory for download";
       break;
       case ERR_DNLD_FILE:
	  error_string="ERR_DNLD_FILE - Could not find the file to download";
       break;
       case ERR_DNLD_TYPE:
	  error_string="ERR_DNLD_TYPE - The file is not downloadable";
       break;
       /************************/
       /* Compatibility Errors */
       /************************/
       case ERR_LIB_INCOMPAT:
	  error_string="ERR_LIB_INCOMPAT - Incompatible library used";
       break;
       case ERR_DRV_INCOMPAT:
	  error_string="ERR_DRV_INCOMPAT - Incompatible driver used(pre v4.0)";
       break;
       /************************/
       /* Switch Driver Errors */
       /************************/
       case MVIP_INVALID_COMMAND:
	  error_string="MVIP_INVALID_COMMAND - Command code is not supported";
	  break;
       case MVIP_DEVICE_ERROR:
	  error_string="MVIP_DEVICE_ERROR - An error was returned from a device called by this driver";
	  break;
       case MVIP_NO_RESOURCES:
	  error_string="MVIP_NO_RESOURCES - An internal driver resource has been exhausted";
	  break;
       case MVIP_INVALID_STREAM:
	  error_string="MVIP_INVALID_STREAM - Stream number in parameter list is out of range";
	  break;
       case MVIP_INVALID_TIMESLOT:
	  error_string="MVIP_INVALID_TIMESLOT - Timeslot in parameter list is out of range";
	  break;
       case MVIP_INVALID_CLOCK_PARM:
	  error_string="MVIP_INVALID_CLOCK_PARM - Invalid clock configuration parameter";
	  break;
       case MVIP_INVALID_MODE:
	  error_string="MVIP_INVALID_MODE - Incorrect SET_OUTPUT or QUERY_OUTPUT mode";
	  break;
       case MVIP_INVALID_MINOR_SWITCH:
	  error_string="MVIP_INVALID_MINOR_SWITCH - Minor(internal) switch error";
	  break;
       case MVIP_INVALID_PARAMETER:
	  error_string="MVIP_INVALID_PARAMETER - General invalid parameter error";
	  break;
       case MVIP_NO_PATH:
	  error_string="MVIP_NO_PATH - Connection cannot be made due to blocking or other switch limitation";
	  break;
       case MVIP_NO_SCBUS_CLOCK:
	  error_string="MVIP_NO_SCBUS_CLOCK - No card driving SCbus clock";
	  break;
       case MVIP_OTHER_SCBUS_CLOCK:
	  error_string="MVIP_OTHER_SCBUS_CLOCK - Another non-MVSWDEV controlled card";
	  break;

       default:
	  error_string="ERR_UNKNOWN - Unknown error value";
       break;
	}

	return error_string;

}

/*---------------------------------------*/
/*-------------- end of file ------------*/

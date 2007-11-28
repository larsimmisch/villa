/*------------------------------------------------------------*/
/* ACULAB plc                                                 */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : cllib.c                                */
/*                                                            */
/*           Purpose : Call control library programs for      */
/*                     multiple device drivers                */
/*                                                            */
/*       Create Date : 19th October 1992                      */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Change History                                             */
/*                                                            */
/* rev: 5.10.0    07/03/2003 Updated for 5.10.0 Release       */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/

#include "mvcldrvr.h"

#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


ACU_INT first_voip_card;                         /* card number of first voip card */
static ACU_INT call_driver_installed=TRUE;       /* identifies existance of call driver code */

/*----------- Function Prototypes ---------------*/
/*                                               */
/* defined in mvcldrvr.h                         */
/*                                               */
/*-----------------------------------------------*/


/*---------------- local functions --------------*/

ACUDLL int  download_file      ( struct download_xparms * );
ACUDLL int  download_pm4_file  ( struct download_xparms * );
ACUDLL ACU_INT  clopendev      ( void );
ACUDLL ACU_INT  opendevdrv     ( void );

ACUDLL void handle_decomp      ( ACU_INT, DC * );
ACUDLL ACU_INT  net_2_unet     ( ACU_INT card, ACU_INT net );
ACUDLL ACU_INT  unet_2_net     ( ACU_INT );
ACUDLL ACU_INT  unet_2_card    ( ACU_INT );
ACUDLL ACU_INT  unet_2_switch  ( ACU_INT );
ACUDLL ACU_INT  unet_2_stream  ( ACU_INT );

ACUDLL ACU_INT  handle2net     ( ACU_INT );
ACUDLL ACU_INT  handle2ch      ( ACU_INT );
ACUDLL ACU_INT  handle2io      ( ACU_INT );
ACUDLL ACU_INT  nch2handle     ( ACU_INT, ACU_INT );
ACUDLL ACU_INT  patch_handle   ( ACU_INT, ACU_INT );
ACUDLL int      update_ss_type ( ACU_INT );
ACUDLL ACU_INT  card_2_voipcard ( ACU_INT );
ACUDLL ACU_INT  card_2_net      ( ACU_INT );

#ifdef ACUC_CLONED
/* Emid for new-style event operations */
static int app_emid ;
#endif

static int  keypad_info      ( int, KEYPAD_XPARMS * );
static int  openout_enquiry  ( int, OUT_XPARMS * );
static char isallowed_string ( char * );

static int    config_assoc_net (ACU_INT, ACU_INT) ;
static ACU_INT xlate_assoc (ACU_INT * ) ;
static ACU_INT call_add_assoc_net  ( ACUC_ASSOC_NET_XPARMS * );

static int  feature_openout_enquiry ( int, FEATURE_OUT_XPARMS * );
static ACU_INT call_maintenance_cmd(ACUC_MAINTENANCE_XPARMS *, int );
int parse_config_str ( char * config_string, char * ip_address1, char * ip_address2,
                       char *subnet_address1, char * subnet_address2, char * gateway_address, int * Download_DHCP);
void parse_string (char *config_string, char search_string[4],char *found_string);

/*------------- external functions --------------*/

ACUDLL int      clopen         ( char * );
ACUDLL ACU_INT  clioctl        ( ACU_INT, IOCTLU *, int, int, int);
ACUDLL ACU_INT  clpblock_ioctl ( ACU_INT, V5_PBLOCK_IOCTLU *, int, int );
ACUDLL void     clclose        ( void );
ACUDLL ACU_INT  cldosioctl     ( int, char *, int );
ACUDLL void     clspecial      ( void );
ACUDLL char     * cldev        ( void );

extern ACU_INT srvioctl       ( ACU_INT  function, IOCTLU  *pioctl, int len , int board_card_number, int voip_protocol);
extern int     pipe_client_send_application_terminated( int board_card_number );
extern int     create_pipe_admin_thread  ( void );
extern int     is_voip_card ( int swdrvr );
extern void    init_card_info            ( int );
extern void    flag_pipe_not_running     ( void );
extern ACU_INT is_pipe_running           ( void );
extern const ACU_INT* get_voip_protocol_index_array ( int* num_of_protocols );

ACUDLL extern int  clfileopen  ( char * );
ACUDLL extern int  clfileread  ( int, char *, unsigned int );
ACUDLL extern int  clfileclose ( int );

/*--------------- local data --------------------*/

int  clopened = FALSE;
int  verbose  = FALSE;      /* display download messages */


static ACU_INT  tnets  = 0;
int    ncards = 0;
CARD   clcard[NCARDS];                  /* card control structure */
static int   switchbase = 0;

static ACU_UCHAR default_codecs[MAXCODECS+1];     /* default codec list            */
static DEFAULT_RAS_CONFIG default_ras_info;   /* default structure for RAS */
static const int MANAGEMENT_INIT_TIMEOUT = 25 ;

/*------------- init_reg_api -------------*/
/* initialise the library version         */
/* details that are passed to the driver. */
/* This is so the drivers can handle      */
/* older version of libraries and keeping */
/* backwards compatibility                */
/*                                        */

ACUDLL void init_api_reg ( struct api_register *api_regp, ACU_LONG size )
   {
   api_regp->signaturea= SIGNATUREA;
   api_regp->libversion= ACU_API_VERSION;
   api_regp->structsize= size;
   api_regp->signatureb= SIGNATUREB;
   }

/*--------------- call_init -------------*/
/* initialise all device drivers in the  */
/* system                                */
/*                                       */
/* call_init should only ever be called  */
/* once for any number of network cards  */
/*                                       */

ACUDLL int ACU_WINAPI call_init ( struct init_xparms * initp )
   {
   ACU_INT  result;
   int  i;
   int voip_card_number=1;    /* numbers voip cards 1,2,3 etc. */
   struct set_sysinfo_xparms set_sysinfo;
   struct sysinfo_xparms    sysinfo;
   int total_network_ports=0;
   const ACU_INT* usable_voip_protocol_indexes;
   int voip_protocol=0;
   int num_of_voip_ports_per_ip_port=0;

   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      /* initialise all cards in the system */

      /* figure out if there's any voip protocols */
      usable_voip_protocol_indexes = get_voip_protocol_index_array(&num_of_voip_ports_per_ip_port);
       
       
      for ( i = 0; i < ncards; i++ )
         {
         /* keep a count of the boards nets */
         total_network_ports = total_network_ports + clcard[i].nnets;
         if (clcard[i].voipservice == ACU_VOIP_ACTIVE)
            {  
            strcpy(clcard[i].board_ip_address,initp->board_ip_address);
            for(voip_protocol = 0; voip_protocol < num_of_voip_ports_per_ip_port; voip_protocol++)
               {
               initp->nnets = clcard[i].v1bmi_card_num;

               /* net number of the H323 port */
               set_sysinfo.net = total_network_ports - num_of_voip_ports_per_ip_port + voip_protocol; 
               set_sysinfo.board_number = voip_card_number;
               set_sysinfo.v1bmi_card_num = clcard[i].v1bmi_card_num;
               strcpy(set_sysinfo.board_ip_address, initp->board_ip_address);
               call_set_system_info ( &set_sysinfo,&sysinfo );

               result = srvioctl ( CALL_INIT, (IOCTLU *) initp, sizeof ( INIT_XPARMS ), card_2_voipcard(i), usable_voip_protocol_indexes[voip_protocol]);

               if (result != 0)  /* if a board fails what to we do? */
                  {
                  if (result != ERR_BOARD_UNLOADED)  /* ERR_BOARD_UNLOADED could happen  so don't abort */
                     {
                     return ( result );  /* otherwise error talking to a board so exit */
                     }
                  }
               }
               voip_card_number++;
            }
         else
            {
            result = clioctl ( CALL_INIT, 
                              (IOCTLU *) initp, 
                               i, 
                               -1,
                               sizeof ( INIT_XPARMS ));
            }
         }
      }
   else
      {
      result = ERR_CFAIL;
      } 
   
   initp->nnets = tnets;                /* put in total networks */
   
   return ( result );
   }
/*---------------------------------------*/

/*------------ DPNSS Feature Library ---------*/

/*------------- dpns_openout -------------*/
/* initialise dpnss outgoing feature call */
/*                                        */
ACUDLL int ACU_WINAPI dpns_openout ( struct dpns_out_xparms * outdetailsp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;

   unet = outdetailsp->net;                          /* save user net */

   result = clopendev ( );

   if ( result == 0 )
      {
      card = unet_2_card ( unet );                   /* get the card number */

      if ( card >= 0 )                               /* check for error */
         {

         /* command not supported for VoIP */
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }

         outdetailsp->net = unet_2_net ( unet );     /* and the network port */

         if ( (outdetailsp->net) >= 0 )              /* check for error */
            {

            result = clioctl ( DPNS_OPENOUT,
                               (IOCTLU *) outdetailsp,
                               card, unet, 
                               sizeof (DPNS_OUT_XPARMS));

            if ( result == 0 )
               {
               /* if result ok then save the handle */


               /* now patch the handle to reflect the network */

               outdetailsp->handle = patch_handle (outdetailsp->handle, unet );

               }
            }
         else
            {
            result = outdetailsp->net;  /* return error code */
            }
         }
      else
         {
         result = card;                 /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   outdetailsp->net = unet;             /* restore user parameter */

   return ( result );
   }
/*---------------------------------------*/

/*----------- dpns_send_overlap ---------*/
/* send DPNSS overlap digits             */
/*                                       */

ACUDLL int ACU_WINAPI dpns_send_overlap ( struct dpns_overlap_xparms * overlapp )
   {
   ACU_INT  uhandle;
   DC   dc;


   uhandle = overlapp->handle;             /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( overlapp->handle, &dc );

      if ( dc.result >= 0 )                  /* check for error */
         {

         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }

         overlapp->handle = dc.handle;       /* get the fixed up handle */

         if ( ! isallowed_string ( overlapp->destination_addr ) )
            {
            dc.result = ERR_PARM;
            }
         else
            {
            dc.result = clioctl ( DPNS_SEND_OVERLAP,
                                 (IOCTLU *) overlapp,
                                 dc.card, -1,
                                 sizeof (DPNS_OVERLAP_XPARMS));
            }
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   overlapp->handle = uhandle;

   return ( dc.result );
   }
/*--------------------------------------------*/

/*------------ dpns_call_details -------------*/
/* get dpnss call details                     */
/*                                            */

ACUDLL int ACU_WINAPI dpns_call_details (struct dpns_detail_xparms *detailsp)
{
   ACU_INT  uhandle;
   DC   dc;
   
   uhandle = detailsp->handle;               /* save user handle */

   dc.result = clopendev();

   if (dc.result == 0)
      {
      handle_decomp(detailsp->handle, &dc);

      if (dc.result >= 0)                  /* check for error */
         {
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return ERR_COMMAND;
            }

         detailsp->handle = dc.handle;       /* get the fixed up handle */

         dc.result = clioctl ( DPNS_CALL_DETAILS,
                               (IOCTLU *) detailsp,
                               dc.card, -1, 
                               sizeof (DPNS_DETAIL_XPARMS));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   detailsp->handle = uhandle;

   return dc.result;
}
/*---------------------------------------*/


/*--------- dpns_incoming_ringing -------*/
/* send the incoming ringing message     */
/*                                       */

ACUDLL int ACU_WINAPI dpns_incoming_ringing ( struct dpns_incoming_ring_xparms  *inringp )
   {
   DC   dc;

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( inringp->handle, &dc );

      if ( dc.result >= 0 )             /* check for error */
         {
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }

         inringp->handle = dc.handle;   /* get the fixed up handle */

         dc.result = clioctl ( DPNS_INCOMING_RINGING,
                              (IOCTLU *) inringp,
                               dc.card, -1,
                               sizeof (DPNS_INCOMING_RING_XPARMS));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   return ( dc.result );
   }
/*---------------------------------------*/

/*----------- dpns_call_accept ----------*/
/* send the connect message              */
/*                                       */
ACUDLL int ACU_WINAPI dpns_call_accept ( struct dpns_call_accept_xparms  *acceptp )
   {
   DC   dc;

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( acceptp->handle, &dc );

      if ( dc.result >= 0 )             /* check for error */
         {
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }

         acceptp->handle = dc.handle;   /* get the fixed up handle */

         dc.result = clioctl ( DPNS_CALL_ACCEPT,
                               (IOCTLU *) acceptp,
                               dc.card, -1,
                               sizeof ( DPNS_CALL_ACCEPT_XPARMS));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   return ( dc.result );
   }
/*---------------------------------------*/


/*---------- dpns_send_feat_info --------*/
/* send the dpnss feature information    */
/*                                       */

ACUDLL int ACU_WINAPI dpns_send_feat_info ( struct dpns_feature_xparms  *featurep )
   {
   DC   dc;

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( featurep->handle, &dc );

      if ( dc.result >= 0 )             /* check for error */
         {
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }

         featurep->handle = dc.handle;   /* get the fixed up handle */

         dc.result = clioctl ( DPNS_SEND_FEAT_INFO,
                               (IOCTLU *) featurep,
                               dc.card, -1,
                               sizeof (DPNS_FEATURE_XPARMS));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   return ( dc.result );
   }
/*----------------------------------*/

/*--------- dpns_set_transit -------*/
/* set dpnss transit working        */
/*                                  */
ACUDLL int ACU_WINAPI dpns_set_transit ( int acchandle )
   {
   struct dpns_set_transit_xparms transit;
   DC   dc;

   memset (&transit,0,sizeof (struct dpns_set_transit_xparms));

   transit.handle = acchandle;

   dc.result = xdpns_set_transit (&transit);

   return ( dc.result );
   }
/*---------------------------------------*/
/*--------- dpns_set_transit -------*/
/* set dpnss transit working        */
/*                                  */

ACUDLL int xdpns_set_transit ( struct dpns_set_transit_xparms  *transitp )
   {
   DC   dc;


   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( transitp->handle, &dc );

      if ( dc.result >= 0 )             /* check for error */
         {

         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }

         dc.result = clioctl ( DPNS_SET_TRANSIT,
                              (IOCTLU *) transitp,
                              dc.card, -1,
                     sizeof (DPNS_SET_TRANSIT_XPARMS));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   return ( dc.result );
   }
/*---------------------------------------*/


/*-------- dpnss_transit_details --------*/
/* get dpnss transit msg details         */
/*                                       */

ACUDLL int ACU_WINAPI dpns_transit_details ( struct dpns_transit_xparms  *transitp )
   {
   ACU_INT  uhandle;
   DC   dc;


   uhandle = transitp->handle;               /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( transitp->handle, &dc );

      if ( dc.result >= 0 )                  /* check for error */
         {

         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
         transitp->handle = dc.handle;       /* get the fixed up handle */

         dc.result = clioctl ( DPNS_TRANSIT_DETAILS,
                               (IOCTLU *) transitp,
                               dc.card, -1, 
                               sizeof(DPNS_TRANSIT_XPARMS));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   transitp->handle = uhandle;

   return ( dc.result );
   }
/*---------------------------------------*/


/*--------- dpns_send_transit -------*/
/* send dpnss transit message        */
/*                                   */

ACUDLL int ACU_WINAPI dpns_send_transit ( struct dpns_transit_xparms  *transitp )
   {
   DC   dc;

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( transitp->handle, &dc );

      if ( dc.result >= 0 )              /* check for error */
         {
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }

         transitp->handle = dc.handle;   /* get the fixed up handle */

         dc.result = clioctl ( DPNS_SEND_TRANSIT,
                              (IOCTLU *) transitp,
                              dc.card, -1,
                              sizeof (DPNS_TRANSIT_XPARMS));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   return ( dc.result );
   }
/*---------------------------------------*/


/*------------- dpns_disconnect ---------*/
/* disconnect dpnss channel              */
/*                                       */
ACUDLL int ACU_WINAPI dpns_disconnect ( struct dpns_cause_xparms * causep )
   {
   ACU_INT  uhandle;
   DC   dc;


   uhandle = causep->handle;            /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( causep->handle, &dc );

      if ( dc.result >= 0 )            /* check for error */
         {
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
         causep->handle = dc.handle;

         dc.result = clioctl ( DPNS_DISCONNECT,
                               (IOCTLU *) causep,
                               dc.card, -1,
                               sizeof (DPNS_CAUSE_XPARMS));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   causep->handle = uhandle;            /* save user handle */

   return ( dc.result );
   }
/*---------------------------------------*/

/*------------- dpns_release ------------*/
/* release dpnss channel                 */
/*                                       */

ACUDLL int ACU_WINAPI dpns_release ( struct dpns_cause_xparms * causep )
   {
   ACU_INT  uhandle;
   DC   dc;

   uhandle = causep->handle;            /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( causep->handle, &dc );

      if ( dc.result >= 0 )            /* check for error */
         {
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
         causep->handle = dc.handle;

         /* clear the entry before going to the driver */
         /* this will stop us calling the driver from  */
         /* the exit list routine                      */

         dc.result = clioctl ( DPNS_RELEASE,
                               (IOCTLU *) causep,
                               dc.card, -1,
                               sizeof (DPNS_CAUSE_XPARMS));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   causep->handle = uhandle;          /* save user handle */

   return ( dc.result );
   }
/*---------------------------------------*/

/*------------- dpns_getcause -----------*/
/* get DPNSS clearing details            */
/*                                       */

ACUDLL int ACU_WINAPI dpns_getcause ( struct dpns_cause_xparms * causep )
   {
   ACU_INT  uhandle;
   DC   dc;


   uhandle = causep->handle;            /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( causep->handle, &dc );

      if ( dc.result >= 0 )            /* check for error */
         {
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
         causep->handle = dc.handle;

         dc.result = clioctl ( DPNS_GETCAUSE,
                               (IOCTLU *) causep,
                               dc.card, -1,
                               sizeof (DPNS_CAUSE_XPARMS));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   causep->handle = uhandle;            /* save user handle */

   return ( dc.result );
   }
/*----------------------------------*/

/*--------- dpns_set_l2_ch ---------*/
/* set dpnss layer2 channel state   */
/*                                  */

ACUDLL int ACU_WINAPI dpns_set_l2_ch ( struct dpns_l2_xparms * dpns_l2_parms )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT   unet;


   unet = dpns_l2_parms->net;                           /* Save user net */

   result = clopendev ( );                              /* Open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );                     /* Get the card number */

      if ( card >= 0 )                                 /* Check for error */
         {
         /* command not supported for VoIP */
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
         dpns_l2_parms->net = unet_2_net ( unet );     /* Get network port */

         if ( dpns_l2_parms->net >= 0 )                /* Check for error */
            {
            result = clioctl ( DPNS_SET_L2_CH,
                               (IOCTLU *) dpns_l2_parms,
                               card, -1,
                               sizeof (DPNS_L2_XPARMS));
            }
         else
            {
            result = dpns_l2_parms->net;               /* Return error code */
            }
         }
      else
         {
         result = card;                                /* Return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   dpns_l2_parms->net = unet;                            /* Restore user parameter */

   return ( result );
   }

/*----------------------------------*/

/*--------- dpns_l2_state ----------*/
/* get dpnss layer2 channel state   */
/*                                  */

ACUDLL int ACU_WINAPI dpns_l2_state ( struct dpns_l2_xparms * dpns_l2_parms )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT   unet;

   unet = dpns_l2_parms->net;                           /* Save user net */

   result = clopendev ( );                              /* Open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );                     /* Get the card number */

      if ( card >= 0 )                                 /* Check for error */
         {
         /* command not supported for VoIP */
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
         dpns_l2_parms->net = unet_2_net ( unet );     /* Get network port */

         if ( dpns_l2_parms->net >= 0 )                /* Check for error */
            {
            result = clioctl ( DPNS_L2_STATE,
                               (IOCTLU *) dpns_l2_parms,
                               card, -1,
                               sizeof (DPNS_L2_XPARMS));
            }
         else
            {
            result = dpns_l2_parms->net;               /* Return error code */
            }
         }
      else
         {
         result = card;                                /* Return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   dpns_l2_parms->net = unet;                          /* Restore user parameter */

   return ( result );
   }

/*----------------------------------*/


/*--------- dpns_watchdog ----------*/
/* tick the dpns watchdog timer     */
/*                                  */
ACUDLL int ACU_WINAPI dpns_watchdog ( struct dpns_wd_xparms * dpns_wd_parms )
   {
   struct watchdog_xparms watch;
   DC   dc;

   memset (&watch,0,sizeof (struct watchdog_xparms));
   watch.net    = dpns_wd_parms->net;
   watch.timeout = dpns_wd_parms->timeout;
   watch.alarm  = ALARM_RRA;

   dc.result = call_watchdog (&watch);
   return ( dc.result );

   }

/*---------------------------------------*/
/*-----End of DPNSS Enhanced Functions---*/


/*---------- call_signal_info -----------*/
/* get the signalling information        */
/*                                       */
ACUDLL int ACU_WINAPI call_signal_info ( struct siginfo_xparms * siginfop )
   {
   ACU_INT  result;
   int  i;
   int increment_ptr = 0;
   struct siginfo_xparms * lsigp;

   result = clopendev ( );

   if ( result == 0 )
      {
      lsigp = siginfop;       /* keep a local copy of the info pointer */

      /* iterate through the cards in the system */
      for ( i = 0; i < ncards; i++ )
         {
         /* for a non VoIP card call to driver */
         if ( clcard[i].voipservice != ACU_VOIP_ACTIVE )
            {
            result = clioctl ( CALL_SIGNAL_INFO,
                               (IOCTLU *) lsigp,
                               i, -1,
                               sizeof ( SIGINFO_XPARMS ) * MAXPPC );

            increment_ptr = lsigp->nnets;
            } 
         else  /* for VoIP card */
            {
            /* iterate through the nets on VoIPcard calling clioctl for each */
            int j = 0;
            int cards_first_net = card_2_net ( i );
            for ( j = 0; j < clcard[i].nnets; j++ )
               {
               result = clioctl ( CALL_SIGNAL_INFO,
                                  (IOCTLU *) &lsigp[j],
                                  i, cards_first_net + j,
                                  sizeof ( SIGINFO_XPARMS ) * MAXPPC );
               }
            increment_ptr = clcard[i].nnets;
            }

         if ( result == 0 )
            {
            /* move the info pointer by the number */
            /* of networks on the card             */
            lsigp = &lsigp[increment_ptr];
            }
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   siginfop->nnets = tnets;             /* copy the number of networks */
   
   
   return ( result );
   }
/*---------------------------------------*/



/*---------- call_system_info -----------*/
/* get the system information            */
/*                                       */

ACUDLL int ACU_WINAPI call_system_info ( struct sysinfo_xparms * sysinfop )
{
   ACU_INT  result;   
   ACU_INT  card;
   ACU_INT  unet;


   unet = sysinfop->net;                      /* save user net    */

   result = clopendev ( );                    /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );            /* get the card number */
       
      if ( card >= 0 )                        /* check for error */
         {
         sysinfop->net = unet_2_net ( unet ); /* and the network port */
   
         if ( sysinfop->net >= 0 )            /* check for error */
            {
     
            result = clioctl ( CALL_SYSTEM_INFO,
                               (IOCTLU *) sysinfop,
                               card, unet, 
                               sizeof (SYSINFO_XPARMS ));
     
            if ( result != 0 )
               {
               result = ERR_CFAIL;
               }
            }
         else
            {
            result = sysinfop->net;     /* return error code */
            }
         }
      else
         {
         result = card;                 /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   return ( result );
}
/*---------------------------------------*/



/*---------- call_set_system_info -------*/
/* set the system information            */
/*                                       */
ACUDLL int ACU_WINAPI call_set_system_info ( struct set_sysinfo_xparms * set_sysinfop, struct sysinfo_xparms * sysinfop)
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;

   unet = set_sysinfop->net;                       /* save user net    */

   result = clopendev ( );                         /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );                 /* get the card number */
       
      if ( card >= 0 )                             /* check for error */
         {
         set_sysinfop->net = unet_2_net ( unet );  /* and the network port */

         if ( set_sysinfop->net >= 0 )             /* check for error */
            {
            /* If VoIP card then thread identification is required */
            if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
               {
               result = clioctl ( CALL_SET_SYSTEM_INFO,
                                  (IOCTLU *) set_sysinfop,
                                  card, unet,
                                  sizeof (SET_SYSINFO_XPARMS ));
               if ( result != 0 )
                  {
                  result = ERR_CFAIL;
                  }
               }
            else 
               {
               /* command only supported for VoIP */
               result = ERR_COMMAND;
               }
            }
         else
            {
            result = set_sysinfop->net;     /* return error code */
            }
         }
      else
         {
         result = card;                   /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   return ( result );
   }
/*---------------------------------------*/


/*--------- call_endcode_devts ----------*/
/* endcode device / timeslot field       */
/*                                       */
ACUDLL int call_encode_devts ( struct endec_xparms * endecp )
   {

   switch ( call_type ( endecp->net ))
      {
      case BR_ATT:
         endecp->endec = (((endecp->device << 8) & 0x1f00) |
                            (endecp->ts & 0xff) | 0xa000 );
      break;

      default:
         endecp->endec = endecp->ts;
      break;
      }

   return ( 0 );                        /* return error */
   }
/*---------------------------------------*/


/*--------- call_decode_devts -----------*/
/* decode device / timeslot field        */
/*                                       */
ACUDLL int call_decode_devts ( struct endec_xparms * endecp )
   {
   endecp->ts     = endecp->endec & (unsigned char) 0xff;
   endecp->device = 0;

   switch ( call_type ( endecp->net ))
      {
      case BR_ATT:
         endecp->device = (((endecp->device & 0x1f00) >> 8) & 0x1f );
      break;
      }

   return ( 0 );
   }
/*---------------------------------------*/


/*------- call_endpoint_initialise ------*/
/*                                       */
/*                                       */

ACUDLL int call_endpoint_initialise ( struct send_spid_xparms * send_spidp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;



   result = clopendev ( );               /* open the device */

   unet = send_spidp->net;               /* save user net    */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );       /* get the card number */

      if ( card >= 0 )                   /* check for error */
         {
         /* command not supported for VoIP */
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
         send_spidp->net = unet_2_net ( unet ); /* and the network port */

         if ( send_spidp->net >= 0 )     /* check for error */
            {
            result = clioctl ( CALL_ENDPOINT_INITIALISE,
                               (IOCTLU *) send_spidp,
                               card, unet,
                               sizeof (SEND_SPID_XPARMS));
            }
         else
            {
            result = send_spidp->net;    /* return error code */
            }
         }
      else
         {
         result = card;                 /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   send_spidp->net = unet;               /* restore user parameter */

   return ( result );
   }
/*---------------------------------------*/


/*------ call_get_endpoint_status -------*/
/*                                       */
/*                                       */

ACUDLL int call_get_endpoint_status ( struct send_spid_xparms * sendspidp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;


   result = clopendev ( );               /* open the device */

   unet = sendspidp->net;                /* save user net    */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );       /* get the card number */

      if ( card >= 0 )                   /* check for error */
         {
         /* command not supported for VoIP */
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
         sendspidp->net = unet_2_net ( unet ); /* and the network port */

         if ( sendspidp->net >= 0 )     /* check for error */
            {
            result = clioctl ( CALL_GET_ENDPOINT_STATUS,
                               (IOCTLU *) sendspidp,
                               card, unet, 
                               sizeof (SEND_SPID_XPARMS));
            }
         else
            {
            result = sendspidp->net;    /* return error code */
            }
         }
      else
         {
         result = card;                 /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   sendspidp->net = unet;               /* restore user parameter */

   return ( result );
   }
/*---------------------------------------*/


/*------------ call_get_spid ------------*/
/*                                       */
/*                                       */

ACUDLL int call_get_spid ( struct get_spid_xparms * get_spidp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;



   result = clopendev ( );               /* open the device */

   unet = get_spidp->net;                /* save user net    */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );       /* get the card number */

      if ( card >= 0 )                   /* check for error */
         {
         /* command not supported for VoIP */
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
         get_spidp->net = unet_2_net ( unet ); /* and the network port */

         if ( get_spidp->net >= 0 )     /* check for error */
            {
            result = clioctl ( CALL_GET_SPID,
                               (IOCTLU *) get_spidp,
                               card, unet,
                               sizeof (GET_SPID_XPARMS));
            }
         else
            {
            result = get_spidp->net;    /* return error code */
            }
         }
      else
         {
         result = card;                 /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   get_spidp->net = unet;               /* restore user parameter */

   return ( result );
   }
/*---------------------------------------*/

/*------- call_send_endpoint_id ---------*/
/*                                       */
/*                                       */

ACUDLL int call_send_endpoint_id ( struct send_endpoint_id_xparms *send_endpointp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;


   result = clopendev ( );               /* open the device */

   unet = send_endpointp->net;                /* save user net    */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );       /* get the card number */

      if ( card >= 0 )                   /* check for error */
         {
         /* command not supported for VoIP */
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }

         send_endpointp->net = unet_2_net ( unet ); /* and the network port */

         if ( send_endpointp->net >= 0 )     /* check for error */
            {
            result = clioctl ( CALL_SEND_ENDPOINT_ID,
                               (IOCTLU *) send_endpointp,
                               card, unet,
                               sizeof (SEND_ENDPOINT_ID_XPARMS));
            }
         else
            {
            result = send_endpointp->net;    /* return error code */
            }
         }
      else
         {
         result = card;                 /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   send_endpointp->net = unet;               /* restore user parameter */

   return ( result );
   }
/*---------------------------------------*/

/*------ call_send_keypad_info ----------*/
/* send keypad information               */
/*                                       */
ACUDLL int ACU_WINAPI call_send_keypad_info ( struct keypad_xparms * keypadp )
   {
   return ( keypad_info ( CALL_SEND_KEYPAD_INFO, keypadp ));
   }
/*---------------------------------------*/

/*------------- keypad_info -------------*/
/* send keypad information               */
/*                                       */
static int keypad_info ( int cmd, struct keypad_xparms * keypadp )
   {
   ACU_INT  card;
   ACU_INT  unet = 0;
   ACU_INT  uhandle = 0;
   DC       dc;


   dc.result = clopendev ( );    /* open the device */


   if ( dc.result == 0 )
      {
      handle_decomp ( keypadp->handle, &dc );

      switch( call_type(handle2net(keypadp->handle)) )
         {
         case S_H323:

            /* check keypad ie length is valid */
            if (keypadp->unique_xparms.sig_h323.keypad.ie[0] >= MAXKEYPAD)
               {
               dc.result = ERR_PARM;
               }

            if (dc.result >= 0)
               {
               uhandle = keypadp->handle;
               keypadp->handle = dc.handle;

               dc.result = clioctl ( cmd,
                                     ( IOCTLU *) keypadp,
                                     dc.card, handle2net(uhandle), 
                                     sizeof (KEYPAD_XPARMS) );
               }
         break;

         case S_SIP:

            /* Not supported by SIP */
            dc.result = ERR_COMMAND;
         break;

         default :   /* Q.931 */

            unet    = keypadp->unique_xparms.sig_q931.net;  /* save user net    */
            uhandle = keypadp->handle;

            if (uhandle == 0)
               {
               card = unet_2_card ( unet );       /* get the card number */
               keypadp->unique_xparms.sig_q931.net = unet_2_net ( unet ); /* and the network port */

               if ( card < 0 )                    /* check for error */
                  dc.result= card;

               if (keypadp->unique_xparms.sig_q931.net < 0)
                  dc.result= keypadp->unique_xparms.sig_q931.net;

               if (dc.result >= 0)
                  {
                  dc.result = clioctl ( cmd,
                                        ( IOCTLU *) keypadp,
                                        card, unet,
                                        sizeof (KEYPAD_XPARMS));
                  }
               }
            else
               {
               handle_decomp ( keypadp->handle, &dc );

               if (dc.result >= 0)
                  {
                  keypadp->handle = dc.handle;

                  dc.result = clioctl ( cmd,
                                        ( IOCTLU *) keypadp,
                                        dc.card, unet,
                                        sizeof (KEYPAD_XPARMS));
                  }
               }
            break;
         } /* switch */
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   /* restore user parameters */
   keypadp->unique_xparms.sig_q931.net = unet;      
   keypadp->handle = uhandle;    
   
   return ( dc.result );
   }
/*---------------------------------------*/

/*------------ call_openin --------------*/
/* initialise incoming call              */
/*                                       */
ACUDLL int ACU_WINAPI call_openin ( struct in_xparms * indetailsp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;

   unet = indetailsp->net;               /* save user net    */


   indetailsp->cnf |= CNF_ENABLE_V5API;  /* Enable V5 drivers events & features not found with V4 drivers */

   result = clopendev ( );               /* open the device */

   if ( result == 0 )
      {
      /* Check for VoIP card */
      card = unet_2_card (indetailsp->net);
      if ( card >= 0 && clcard[card].voipservice == ACU_VOIP_ACTIVE)
         {
         ACU_INT sig_net  = indetailsp->net ;  /* Fixed up by xlate_assoc */
         indetailsp->net = unet_2_net ( sig_net ); /* and the network port */


         result = clioctl ( CALL_OPENIN,
                            ( IOCTLU *) indetailsp,
                            card, unet,
                            sizeof (IN_XPARMS));
         if ( result == 0 )
            {
            indetailsp->handle = patch_handle (indetailsp->handle, sig_net );
            }
         else
            {
            return (result);  /* return error code */
            }
         }
      else
         {
         /* Translate user's bearer network to actual signalling network... */
         /* and associated bearer network port...                           */
         ACU_INT assoc_net = xlate_assoc(&indetailsp->net) ;
         ACU_INT sig_net  = indetailsp->net ;  /* Fixed up by xlate_assoc   */

         /* Now send the IOCTL to the signaling net rather than the bearer net */
         card = unet_2_card ( sig_net );       /* get the card number */

         if ( card >= 0 )                   /* check for error */
            {
            indetailsp->net = unet_2_net ( sig_net ); /* and the network port */

            if ( indetailsp->net >= 0 )     /* check for error */
               {
               /* We encode associated bearer net in upper bits of sig_net... */
               indetailsp->net |= ((assoc_net +1) << 8) ;
               result = clioctl ( CALL_OPENIN,
                                  (IOCTLU *) indetailsp,
                                  card, sig_net,
                                  sizeof (IN_XPARMS));

               if ( result == 0 )
                  {
                  /* now patch the handle to reflect the network */
                  indetailsp->handle = patch_handle (indetailsp->handle, sig_net );
                  }
               }
            else
               {
               result = indetailsp->net;   /* return error code */
               }
            }
         else
            {
            result = card;                 /* return error code */
            }
         } /* if */
      }
   else
      {
      result = ERR_CFAIL;
      }

   indetailsp->net = unet;              /* restore user parameter */


   return ( result );
   }
/*---------------------------------------*/

/*------------ call_openout -------------*/
/* initialise outgoing call              */
/*                                       */
ACUDLL int ACU_WINAPI call_openout ( struct out_xparms * outdetailsp )
   {
   return ( openout_enquiry ( CALL_OPENOUT, outdetailsp ));
   }
/*---------------------------------------*/

/*------------ call_enquiry -------------*/
/* initialise outgoing call              */
/*                                       */
ACUDLL int ACU_WINAPI call_enquiry ( struct out_xparms * enquiryp )
   {
   return ( openout_enquiry ( CALL_ENQUIRY, enquiryp ));
   }
/*---------------------------------------*/


/*---------- openout_enquiry ------------*/
/* run openout or enquiry                */
/*                                       */

static int openout_enquiry ( int mode, struct out_xparms * calloutp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;
   ACU_INT  i;

   unet = calloutp->net;            /* save user net */

   calloutp->cnf |= CNF_ENABLE_V5API;     /* Enable V5 drivers events & features not found with V4 drivers */

   result = clopendev ( );

   if ( result == 0 )
      {
      card = unet_2_card (calloutp->net);

     /* Check for VoIP card */
      if (card >= 0 && clcard[card].voipservice == ACU_VOIP_ACTIVE)
         {
         ACU_INT sig_net  = calloutp->net ;   /* Fixed up by xlate_assoc */
         calloutp->net = unet_2_net ( sig_net ); /* and the network port */

         if ( S_H323 == call_type(unet) )
            {
            /* check codec assignment */
            if ( (calloutp->unique_xparms.sig_h323.codecs[0] == 0) &&
               default_codecs[0] )
               {
               for ( i = 0; i < MAXCODECS; i++ )
                  {
                  calloutp->unique_xparms.sig_h323.codecs[i] = default_codecs[i];
                  /* 0 signifies end of codec list so stop assignment here for efficiency */
                  if (default_codecs[i] == 0 )
                     break;
                  }
               }

            /* check GK assignment */
            if ( default_ras_info.request_admission )
               {
               /* load default GK info */
               calloutp->unique_xparms.sig_h323.request_admission = default_ras_info.request_admission;
               calloutp->unique_xparms.sig_h323.gk_addr = default_ras_info.gk_addr;
               for ( i = 0; i < default_ras_info.endpoint_identifier_length; i++ )
                  {
                  calloutp->unique_xparms.sig_h323.endpoint_identifier[i] = default_ras_info.endpoint_identifier[i];
                  }
                  calloutp->unique_xparms.sig_h323.endpoint_identifier_length = default_ras_info.endpoint_identifier_length;
               }

            }

         result = clioctl ( mode,
                            (IOCTLU *) calloutp,
                            card,sig_net,
                            sizeof (OUT_XPARMS));

         if (result == 0)
            {
            /* now patch the handle to reflect the network */
            calloutp->handle = patch_handle (calloutp->handle, sig_net );
            }

         return(result);
         }
      else
         {
         /* Translate user's bearer network to actual signalling network... */
         /* and associated bearer network port...                           */
         ACU_INT assoc_net = xlate_assoc(&calloutp->net) ;
         ACU_INT sig_net  = calloutp->net ;     /* Fixed up by xlate_assoc */

         card = unet_2_card ( sig_net );        /* get the card number */

         if ( card >= 0 )                    /* check for error */
            {
            calloutp->net = unet_2_net ( sig_net ); /* and the network port */

            if ( calloutp->net >= 0 )     /* check for error */
               {
               switch (call_type(sig_net))
                  {
                  case S_ISUP :
                     if (calloutp->unique_xparms.sig_isup.bearer.ie[0] >= MAXBEARER)
                        {
                        result = ERR_SERVICE;
                        }
                     else if (calloutp->unique_xparms.sig_isup.lolayer.ie[0] >= MAXLOLAYER)
                        {
                        result = ERR_SERVICE;
                        }
                     else if (calloutp->unique_xparms.sig_isup.hilayer.ie[0] >= MAXHILAYER)
                        {
                        result = ERR_SERVICE;
                        }
                     else if (calloutp->unique_xparms.sig_isup.progress_indicator.ie[0] >= MAXPROGRESS)
                        {
                        result = ERR_PARM;
                        }
                  break ;

                  default :
                     if (calloutp->unique_xparms.sig_q931.bearer.ie[0] >= MAXBEARER)
                        {
                        result = ERR_SERVICE;
                        }
                     else if (calloutp->unique_xparms.sig_q931.lolayer.ie[0] >= MAXLOLAYER)
                        {
                        result = ERR_SERVICE;
                        }
                     else if (calloutp->unique_xparms.sig_q931.hilayer.ie[0] >= MAXHILAYER)
                        {
                        result = ERR_SERVICE;
                        }
                     else if (calloutp->unique_xparms.sig_q931.progress_indicator.ie[0] >= MAXPROGRESS)
                        {
                        result = ERR_PARM;
                        }
                  break ;

                  }
               if (result == 0)
                  {
                  /* We encode associated bearer net in upper bits of sig_net... */
                  calloutp->net |= ((assoc_net +1) << 8) ;

                  if (  !isallowed_string ( calloutp->destination_addr )       /* check if the characaters in the string are allowed */
                     || !isallowed_string ( calloutp->originating_addr ))
                     {
                     result = ERR_PARM;
                     }
                  else
                     result = clioctl ( mode,
                                        (IOCTLU *) calloutp,
                                        card,sig_net,
                                        sizeof (OUT_XPARMS));

                  if ( result == 0 )
                     {
                     /* now patch the handle to reflect the network */
                     calloutp->handle = patch_handle (calloutp->handle, sig_net );
                     }
                  }
               }
            else
               {
               result = calloutp->net;  /* return error code */
               }
            }
         else
            {
            result = card;                 /* return error code */
            }
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   calloutp->net = unet;             /* restore user parameter */

   return ( result );
   }
/*---------------------------------------*/

/*------------ call_state ---------------*/
/* get a current state or wait for change*/
/*                                       */

ACUDLL int ACU_WINAPI call_state ( struct state_xparms * statep )
   {
   ACU_INT  uhandle;
   DC   dc;


   uhandle = statep->handle;                /* save user handle */

   dc.result = clopendev ( );               /* open the device  */
   
   if ( dc.result == 0 )
      {
      handle_decomp ( statep->handle, &dc );
       
      if ( dc.result >= 0 )                 /* check for error */
         {
         statep->handle = dc.handle;
      
         dc.result = clioctl ( CALL_STATE,
                               (IOCTLU *) statep,
                               dc.card, handle2net(uhandle),
                               sizeof (STATE_XPARMS));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }
   
   statep->handle = uhandle;
   
   return ( dc.result );
   }
/*---------------------------------------*/


#ifdef ACUC_CLONED
/*------------ call_event ---------------*/
/* get an event if one present           */
/*                                       */
ACUDLL int ACU_WINAPI call_event ( struct state_xparms * eventp )
   {
   ACU_INT  result;
   ACU_INT  unet;
   ACU_INT  card;
   ACUC_EVENT_IF_XPARMS event_if_xparms ;

   memset (&event_if_xparms,0,sizeof (event_if_xparms));

   result = clopendev ( );

   if ( result == 0 )
      {
      if ( eventp->handle != 0 )
         {
         unet = handle2net ( eventp->handle );
         card  = unet_2_card ( unet );
         event_if_xparms.cmd = ACUC_EVENT_POLL_SPECIFIC ;
         event_if_xparms.timeout = eventp->timeout ;

         result = clioctl ( CALL_EVENT_IF,
                            (IOCTLU *) &event_if_xparms,
                            card, unet,
                            sizeof (event_if_xparms));

         }
      else
         {
         /* Global event polling mode. */
         int event_possible ;
         do
            {

            event_if_xparms.emid = app_emid ;
            event_if_xparms.cmd = ACUC_EVENT_POLL_GLOBAL ;
            event_if_xparms.timeout = eventp->timeout ;

            result = clioctl ( CALL_EVENT_IF,
                               (IOCTLU *) &event_if_xparms,
                               0, -1,
                               sizeof (event_if_xparms));

            if (result !=0)
               break ;

            if (event_if_xparms.cnum >=0)
               {
               event_possible = 1 ;

               /* Drv 0 has reported a possible event on nominated cnum.  See if
               it's real... */

               event_if_xparms.cmd = ACUC_EVENT_POLL_SPECIFIC ;

               result = clioctl ( CALL_EVENT_IF,
                                  (IOCTLU *) &event_if_xparms,
                                  event_if_xparms.cnum,-1,
                                  sizeof (event_if_xparms));

               if (event_if_xparms.handle != 0)
                  {
                  break ;         /* Got valid event! */
                  }

               }
            else
               event_possible = 0 ;

            } while (event_possible) ;
         }
      }

   if (result == 0)
      {
      eventp->state = event_if_xparms.state ;
      eventp->extended_state = event_if_xparms.extended_state ;
      eventp->handle = event_if_xparms.handle ;
      }
   return result ;
   }
#else
/*------------ call_event ---------------*/
/* get an event if one present           */
/*                                       */
ACUDLL int ACU_WINAPI call_event ( struct state_xparms * eventp )
   {
   ACU_INT  result;   
   ACU_INT  unet;
   LEQ      leq;


   result = clopendev ( );
   
   if ( result == 0 )
      {
      if ( eventp->handle != 0 )
         {
         /* Handle mode - Handle based events */
         /* decompose the handle into its components */
         
         unet = handle2net ( eventp->handle );
         
         leq.card  = unet_2_card ( unet ); 
         leq.net   = unet_2_net  ( unet ); 
         leq.ch    = handle2ch   ( eventp->handle );
         leq.io    = handle2io   ( eventp->handle );
         leq.valid = TRUE;
         }
      else
         {
         /* Global Mode - Driver based events */	
         leq.valid = FALSE;
         leq.net = -1;
         leq.card = 0;   /* require a sensible value for use with clioctl */
         }

      eventp->handle = 0;               /* clear for the call */
      leq.timeout    = eventp->timeout; /* patch the timeout */


      if ( call_driver_installed ) 
         {
          
          /* call driver installed somewhere in the system so we can call clioctl */
         result = clioctl ( CALL_LEQ, 
                            (IOCTLU *) &leq, 
                            0, leq.net,
                            sizeof (LEQ ));  
         }
      else
         {
          /* no call driver installed so we must deal with it in the user space */
         result = srvioctl ( CALL_LEQ, 
                            (IOCTLU *) &leq,
                            sizeof (LEQ ), net_2_unet(leq.card, leq.net),
                            card_2_voipcard(leq.card));
         }


      if ( result == 0 )
         {
         if ( leq.valid )
            {
            eventp->handle  = nch2handle ( leq.net, leq.ch );
            eventp->handle |= leq.io;                               /* add the direction */
            eventp->timeout = 0;
 
            result = clioctl ( CALL_LEQ_S, 
                               (IOCTLU *) eventp, 
                               leq.card, net_2_unet(leq.card, leq.net),
                               sizeof (STATE_XPARMS ));
               
            eventp->timeout = leq.timeout;

            if ( result == 0 )
               {
               eventp->handle = patch_handle (eventp->handle, card_2_net(leq.card) + leq.net );
               }
            }
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   return ( result );

   }
#endif
/*---------------------------------------*/


/*------------ call_details -------------*/
/* get call details                      */
/*                                       */

ACUDLL int ACU_WINAPI call_details ( struct detail_xparms * detailsp )
   {
   ACU_INT  uhandle;
   DC   dc;


   uhandle = detailsp->handle;               /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( detailsp->handle, &dc );
       
      if ( dc.result >= 0 )                  /* check for error */
         {

         detailsp->handle = dc.handle;       /* get the fixed up handle */

         dc.result = clioctl ( CALL_DETAILS,
                               (IOCTLU *) detailsp,
                               dc.card, handle2net(uhandle),
                               sizeof (DETAIL_XPARMS));
   
         if (dc.result == 0)
            {
            if (call_type(handle2net(uhandle)) == S_ISUP)
               {
               /* Avoidance for ISUP NFAS driver problem whereby driver may
                  be unable to resolve the stream correctly */
               detailsp->stream = call_port_2_stream (call_handle_2_port(uhandle)) ;
               }
            }
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }
   
   detailsp->handle = uhandle;
   
   return ( dc.result );
   }
/*---------------------------------------*/



/*------------ call_get_charge ----------*/
/* get the call charge                   */
/*                                       */

ACUDLL int ACU_WINAPI call_get_charge ( struct get_charge_xparms * chargep )
   {
   ACU_INT  uhandle;
   DC   dc;


   uhandle = chargep->handle;             /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( chargep->handle, &dc );

      if ( dc.result >= 0 )                  /* check for error */
         {

         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
         chargep->handle = dc.handle;       /* get the fixed up handle */

         dc.result = clioctl ( CALL_GET_CHARGE,
                               (IOCTLU *) chargep,
                               dc.card, handle2net(chargep->handle),
                               sizeof (GET_CHARGE_XPARMS ));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   chargep->handle = uhandle;

   return ( dc.result );
   }
/*---------------------------------------*/


/*------------ call_put_charge ----------*/
/* put the call charge                   */
/*                                       */

ACUDLL int ACU_WINAPI call_put_charge ( struct put_charge_xparms * chargep )
   {
   ACU_INT  uhandle;
   DC   dc;


   uhandle = chargep->handle;             /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( chargep->handle, &dc );

      if ( dc.result >= 0 )                  /* check for error */
         {

         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }

         chargep->handle = dc.handle;       /* get the fixed up handle */

         dc.result = clioctl ( CALL_PUT_CHARGE,
                               (IOCTLU *) chargep,
                               dc.card, handle2net(chargep->handle),
                               sizeof (PUT_CHARGE_XPARMS));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   chargep->handle = uhandle;

   return ( dc.result );
   }
/*---------------------------------------*/



/*----------- call_send_overlap ---------*/
/* send the overlap digits               */
/*                                       */
ACUDLL int ACU_WINAPI call_send_overlap ( struct overlap_xparms * overlapp )
   {
   ACU_INT  uhandle;
   DC   dc;


   uhandle = overlapp->handle;             /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( overlapp->handle, &dc );

      if ( dc.result >= 0 )                  /* check for error */
         {

         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }

         overlapp->handle = dc.handle;       /* get the fixed up handle */

         if ( ! isallowed_string ( overlapp->destination_addr ) )
            {
            dc.result = ERR_PARM;
            }
         else
            {
            dc.result = clioctl ( CALL_SEND_OVERLAP,
                                  (IOCTLU *) overlapp,
                                  dc.card,  handle2net(overlapp->handle),
                                  sizeof (OVERLAP_XPARMS));
            }
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   overlapp->handle = uhandle;

   return ( dc.result );
   }
/*---------------------------------------*/


/*------------ call_accept --------------*/
/* connect the call (old version)        */
/*                                       */
ACUDLL int ACU_WINAPI call_accept ( int acchandle )
   {
   struct accept_xparms acceptp;
   DC   dc;

   memset (&acceptp,0,sizeof (struct accept_xparms));

   acceptp.handle = acchandle;

   dc.result = xcall_accept (&acceptp);

   return (dc.result);
}

/*------------ xcall_accept -------------*/
/* connect the call                      */
/*                                       */

ACUDLL int ACU_WINAPI xcall_accept ( struct accept_xparms *acceptp )
   {
   DC   dc;
   ACU_INT i;
   ACU_INT unet;

   dc.result = clopendev ( );
   unet = handle2net(acceptp->handle);

   if ( dc.result == 0 )
      {
      handle_decomp ( acceptp->handle, &dc );

      switch(call_type(unet))
         {
         case S_H323:

            if ( (acceptp->unique_xparms.sig_h323.codecs[0] == 0) &&
                  default_codecs[0] )
               {
               /* load default codecs here */
               for ( i = 0; i < MAXCODECS; i++ )
                  {
                  acceptp->unique_xparms.sig_h323.codecs[i] = default_codecs[i];
                  /* 0 signifies end of codec list so stop assignment here for efficiency */
                  if (default_codecs[i] == 0 )
                     break;
                  }
               }
               /* check GK assignment */
            if ( default_ras_info.request_admission )
               {
               /* load default GK info */
               acceptp->unique_xparms.sig_h323.request_admission = default_ras_info.request_admission;
               acceptp->unique_xparms.sig_h323.gk_addr = default_ras_info.gk_addr;
               for ( i = 0; i < default_ras_info.endpoint_identifier_length; i++ )
                  {
                  acceptp->unique_xparms.sig_h323.endpoint_identifier[i] = default_ras_info.endpoint_identifier[i];
                  }
               acceptp->unique_xparms.sig_h323.endpoint_identifier_length = default_ras_info.endpoint_identifier_length;
               }
            break;
          case S_ISUP :
             if (acceptp->unique_xparms.sig_isup.progress_indicator.ie[0] >= MAXPROGRESS)  /* check progress_indicator length */
                 {
                 dc.result = ERR_PARM;
                 }
             else if (acceptp->unique_xparms.sig_isup.lolayer.ie[0] >= MAXLOLAYER)  /* check lolayer length */
                 {
                 dc.result = ERR_PARM;
                 }
             break;
          default :
             if (acceptp->unique_xparms.sig_q931.progress_indicator.ie[0] >= MAXPROGRESS)  /* check progress_indicator length */
                 {
                 dc.result = ERR_PARM;
                 }
             else if (acceptp->unique_xparms.sig_q931.lolayer.ie[0] >= MAXLOLAYER)  /* check lolayer length */
                 {
                 dc.result = ERR_PARM;
                 }
             else if (acceptp->unique_xparms.sig_q931.display.ie[0] >= MAXDISPLAY)  /* check display length */
                 {
                 dc.result = ERR_PARM;
                 }
             break;
          }

      if ( dc.result >= 0 )             /* check for error */
         {
         acceptp->handle = dc.handle;           /* get the fixed up handle */

         dc.result = clioctl ( CALL_ACCEPT,
                               (IOCTLU *) acceptp,
                               dc.card, unet,
                               sizeof ( ACCEPT_XPARMS ));
          
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   return ( dc.result );
   }
/*---------------------------------------*/

/*-------------- call_hold --------------*/
/* put call on hold                      */
/*                                       */
ACUDLL int ACU_WINAPI call_hold ( int holdhandle )
   {
   struct hold_xparms holdp;
   DC   dc;

   memset (&holdp,0,sizeof(struct hold_xparms));

   holdp.handle = holdhandle;

   dc.result = xcall_hold (&holdp);

   return ( dc.result );
   }
/*---------------------------------------*/

/*-------------- call_reconnect --------------*/
/* get call on hold back                      */
/*                                            */
ACUDLL int ACU_WINAPI call_reconnect ( int holdhandle )
   {
   struct hold_xparms holdp;
   DC   dc;

   memset (&holdp,0,sizeof(struct hold_xparms));

   holdp.handle = holdhandle;

   dc.result = xcall_reconnect (&holdp);

   return ( dc.result );
   }
/*---------------------------------------*/


/*-------------- xcall_hold -------------*/
/* put call on hold                      */
/*                                       */

ACUDLL int ACU_WINAPI xcall_hold ( HOLD_XPARMS *holdp )
   {
   DC   dc;
   ACU_INT unet;


   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( holdp->handle, &dc );

      if ( dc.result >= 0 )             /* check for error */
         {
         unet = handle2net(holdp->handle);  
         holdp->handle = dc.handle;     /* get the fixed up handle */
       
         dc.result = clioctl ( CALL_HOLD,
                               (IOCTLU *) holdp,
                               dc.card,unet,
                               sizeof ( HOLD_XPARMS ));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }
   
   return ( dc.result );
   }

/*---------- xcall_reconnect ------------*/
/* get call on hold                      */
/*                                       */

ACUDLL int ACU_WINAPI xcall_reconnect ( HOLD_XPARMS *holdp )
   {
   DC   dc;
   ACU_INT unet;

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( holdp->handle, &dc );

      if ( dc.result >= 0 )             /* check for error */
         {
         unet = handle2net(holdp->handle);
         holdp->handle = dc.handle;     /* get the fixed up handle */

         dc.result = clioctl ( CALL_RECONNECT,
                               (IOCTLU *) holdp,
                               dc.card,unet,
                               sizeof ( HOLD_XPARMS ));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }
   
   return ( dc.result );
   }
/*---------------------------------------*/


/*------------- call_transfer -----------*/
/* put call on hold                      */
/*                                       */

ACUDLL int ACU_WINAPI call_transfer ( TRANSFER_XPARMS * transferp )
   {
   ACU_INT  uhandlea;
   ACU_INT  uhandlec;
   DC   dc;
   ACU_INT unet;


   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      dc.result = ERR_PARM;

      /* must be on the same port */
      unet = handle2net ( transferp->handlea );

      if ( handle2net ( transferp->handlea ) == handle2net (  transferp->handlec ))
         {
         uhandlea = transferp->handlea;             /* save user handle */
         uhandlec = transferp->handlec;             /* save user handle */

         dc.result = clopendev ( );

         if ( dc.result == 0 )
            {
            handle_decomp ( transferp->handlea, &dc );

            if ( dc.result >= 0 )                  /* check for error */
               {

               transferp->handlea = dc.handle;       /* get the fixed up handle */

               handle_decomp ( transferp->handlec, &dc );

               if ( dc.result >= 0 )                  /* check for error */
                  {
                  transferp->handlec = dc.handle;       /* get the fixed up handle */

                  dc.result = clioctl ( CALL_TRANSFER,
                                        (IOCTLU *) transferp,
                                        dc.card,unet,
                                        sizeof (TRANSFER_XPARMS));
                  }
               }
            }
         else
            {
            dc.result = ERR_CFAIL;
            }
 
         transferp->handlea = uhandlea;
         transferp->handlec = uhandlec;
         }
      }
   
   return ( dc.result );
   }
/*---------------------------------------*/


/*------------ call_answercode ----------*/
/* set the answer code value             */
/*                                       */

ACUDLL int ACU_WINAPI call_answercode ( struct cause_xparms * causep )
   {
   ACU_INT  uhandle;
   DC   dc;

   uhandle = causep->handle;            /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( causep->handle, &dc );

      if ( dc.result >= 0 )            /* check for error */
         {
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
         causep->handle = dc.handle;

         dc.result = clioctl ( CALL_ANSWERCODE,
                               (IOCTLU *) causep,
                               dc.card,handle2net(uhandle),
                               sizeof ( CAUSE_XPARMS ));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   causep->handle = uhandle;            /* save user handle */

   return ( dc.result );
   }
/*---------------------------------------*/


/*---------- call_incoming_ringing ------*/
/* send the incoming ringing message     */
/* (old version)                         */
/*                                       */

ACUDLL int ACU_WINAPI call_incoming_ringing ( int ringhandle )
   {
   struct incoming_ringing_xparms incoming_ringingp;
   DC   dc;

   memset(&incoming_ringingp, 0, sizeof(struct incoming_ringing_xparms));

   incoming_ringingp.handle = ringhandle;

   dc.result = xcall_incoming_ringing ( &incoming_ringingp );

   return ( dc.result );
   }
/*---------------------------------------*/

/*---------- call_incoming_ringing ------*/
/* send the incoming ringing message     */
/*                                       */

ACUDLL int ACU_WINAPI xcall_incoming_ringing ( struct incoming_ringing_xparms *incoming_ringingp )
   {
   DC   dc;
   ACU_INT unet;

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( incoming_ringingp->handle, &dc );

      unet = handle2net(incoming_ringingp->handle);
      switch(call_type(unet))
          {
          case S_H323 :
             if ( (incoming_ringingp->unique_xparms.sig_h323.codecs[0] == 0) &&
                   default_codecs[0] )
                {
                /* load default codecs here */
                int i;
                for ( i = 0; i < MAXCODECS; i++ )
                   {
                   incoming_ringingp->unique_xparms.sig_h323.codecs[i] = default_codecs[i];
                   /* 0 signifies end of codec list so stop assignment here for efficiency */
                   if (default_codecs[i] == 0 )
                      break;
                   }
                }
            /* check GK assignment */
            if ( default_ras_info.request_admission )
               {
               int i;
               /* load default GK info */
               incoming_ringingp->unique_xparms.sig_h323.request_admission = default_ras_info.request_admission;
               incoming_ringingp->unique_xparms.sig_h323.gk_addr = default_ras_info.gk_addr;
               for ( i = 0; i < default_ras_info.endpoint_identifier_length; i++ )
                  {
                  incoming_ringingp->unique_xparms.sig_h323.endpoint_identifier[i] = default_ras_info.endpoint_identifier[i];
                  }
                  incoming_ringingp->unique_xparms.sig_h323.endpoint_identifier_length = default_ras_info.endpoint_identifier_length;
               }
          break;

          case S_ISUP :
             if (incoming_ringingp->unique_xparms.sig_isup.progress_indicator.ie[0] >= MAXPROGRESS)  /* check progress_indicator length */
                 {
                 dc.result = ERR_PARM;
                 }
             break;
          default :
             if (incoming_ringingp->unique_xparms.sig_q931.progress_indicator.ie[0] >= MAXPROGRESS)  /* check progress_indicator length */
                 {
                 dc.result = ERR_PARM;
                 }
             else if (incoming_ringingp->unique_xparms.sig_q931.display.ie[0] >= MAXDISPLAY)  /* check display length */
                 {
                 dc.result = ERR_PARM;
                 }
             break;
          }

      if ( dc.result >= 0 )             /* check for error */
         {
         incoming_ringingp->handle = dc.handle;           /* get the fixed up handle */

         dc.result = clioctl ( CALL_INCOMING_RINGING,
                               (IOCTLU *) incoming_ringingp,
                               dc.card,unet,
                               sizeof ( INCOMING_RINGING_XPARMS ));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }
   
   return ( dc.result );
   }
/*---------------------------------------*/

/*------ call_get_originating_addr ------*/
/* get call details                      */
/*                                       */
ACUDLL int ACU_WINAPI call_get_originating_addr ( int acchandle )
   {
   struct get_originating_addr_xparms get_originating_addrp;
   DC   dc;

   memset (&get_originating_addrp,0,sizeof(struct get_originating_addr_xparms));

   get_originating_addrp.handle= acchandle;

   dc.result = xcall_get_originating_addr ( &get_originating_addrp );

   return ( dc.result );
   }
/*---------------------------------------*/


/*------ xcall_get_originating_addr -----*/
/* get call details                      */
/*                                       */

ACUDLL int ACU_WINAPI xcall_get_originating_addr ( struct get_originating_addr_xparms *get_originating_addrp )
   {
   DC   dc;


   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( get_originating_addrp->handle, &dc );

      if ( dc.result >= 0 )             /* check for error */
         {
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
         get_originating_addrp->handle = dc.handle;   /* get the fixed up handle */

         dc.result = clioctl ( CALL_GET_ORIGINATING_ADDR,
                               (IOCTLU *) get_originating_addrp,
                               dc.card, -1, 
                               sizeof ( GET_ORIGINATING_ADDR_XPARMS ));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   return ( dc.result );
   }
/*---------------------------------------*/


/*------------- call_release ------------*/
/* release channel                       */
/*                                       */
ACUDLL int ACU_WINAPI call_disconnect ( struct cause_xparms * causep )
   {
   struct disconnect_xparms disconnectp;
   DC     dc;

   memset (&disconnectp, 0 , sizeof (struct disconnect_xparms));

   disconnectp.handle = causep->handle;
   disconnectp.cause  = causep->cause;

   switch(call_type(handle2net(disconnectp.handle)))
       {
       case S_ISUP :
           disconnectp.unique_xparms.sig_isup.raw = causep->raw;

           /* Set location to -1 to tell the driver to use the default location */
           disconnectp.unique_xparms.sig_isup.location = -1;
           break;
       default :
           disconnectp.unique_xparms.sig_q931.raw = causep->raw;
           break;
       }

   dc.result = xcall_disconnect (&disconnectp);

   return ( dc.result );
}


/*------------- xcall_disconnect --------*/
/* release channel                       */
/*                                       */

ACUDLL int ACU_WINAPI xcall_disconnect ( struct disconnect_xparms * disconnectp )
   {
   ACU_INT  uhandle;
   DC   dc;
   ACU_INT unet = 0;

   uhandle = disconnectp->handle;            /* save user handle */

   dc.result = clopendev ( );

   if (dc.result >= 0)
       {
       handle_decomp ( disconnectp->handle, &dc );
       unet = handle2net(disconnectp->handle);

       switch(call_type(unet))
           {
           case S_H323 :

              break;
           case S_ISUP :
               if (disconnectp->unique_xparms.sig_isup.progress_indicator.ie[0] >= MAXPROGRESS)  /* check progress_indicator length */
                   {
                   dc.result = ERR_PARM;
                   }
               break;
           default :
               if (disconnectp->unique_xparms.sig_q931.progress_indicator.ie[0] >= MAXPROGRESS)  /* check progress_indicator length */
                   {
                   dc.result = ERR_PARM;
                   }
               else if (disconnectp->unique_xparms.sig_q931.display.ie[0] >= MAXDISPLAY)  /* check display length */
                   {
                   dc.result = ERR_PARM;
                   }
               break;
           }
       }

   if ( dc.result == 0 )
      {

      if ( dc.result >= 0 )            /* check for error */
         {
         disconnectp->handle = dc.handle;

         dc.result = clioctl ( CALL_DISCONNECT,
                               (IOCTLU *) disconnectp,
                               dc.card,unet,
                               sizeof ( DISCONNECT_XPARMS ));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   disconnectp->handle = uhandle;            /* save user handle */

   return ( dc.result );
   }
/*---------------------------------------*/


/*------------- call_getcause ------------*/
/* get clearing cause (old version)       */
/*                                        */
ACUDLL int ACU_WINAPI call_getcause ( struct cause_xparms * causep )
   {
   struct disconnect_xparms disconnectp;
   DC   dc;

   memset (&disconnectp, 0, sizeof (struct disconnect_xparms));

   disconnectp.handle = causep->handle;

   dc.result = xcall_getcause (&disconnectp);

   causep->handle = disconnectp.handle;
   causep->cause  = disconnectp.cause;

   switch(call_type(handle2net(disconnectp.handle)))
      {
      case S_ISUP :
         causep->raw = disconnectp.unique_xparms.sig_isup.raw;
         break;
      case S_H323:
         causep->raw = disconnectp.unique_xparms.sig_h323.raw;
         break;
      default :
         causep->raw = disconnectp.unique_xparms.sig_q931.raw;
         break;
      }

   return ( dc.result );
}


/*------------- xcall_getcause ----------*/
/* get clearing cause                    */
/*                                       */

ACUDLL int ACU_WINAPI xcall_getcause ( struct disconnect_xparms * disconnectp )
   {
   ACU_INT  uhandle;
   DC   dc;


   uhandle = disconnectp->handle;            /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( disconnectp->handle, &dc );

      if ( dc.result >= 0 )            /* check for error */
         {
         disconnectp->handle = dc.handle;

         dc.result = clioctl ( CALL_GETCAUSE,
                               (IOCTLU *) disconnectp,
                               dc.card, handle2net(uhandle),
                               sizeof ( DISCONNECT_XPARMS ));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   disconnectp->handle = uhandle;            /* save user handle */

   return ( dc.result );
   }
/*---------------------------------------*/



/*------------- call_release ------------*/
/* release channel                       */
/*                                       */
ACUDLL int ACU_WINAPI call_release ( struct cause_xparms * causep )
   {
   struct disconnect_xparms disconnectp;
   DC   dc;

   memset (&disconnectp,0,sizeof (struct disconnect_xparms));

   disconnectp.handle = causep->handle;
   disconnectp.cause  = causep->cause;

   switch(call_type(handle2net(disconnectp.handle)))
       {
       case S_ISUP :
           disconnectp.unique_xparms.sig_isup.raw = causep->raw;

           /* Set location to -1 to tell the driver to use the default location */
           disconnectp.unique_xparms.sig_isup.location = -1;
           break;
       default :
           disconnectp.unique_xparms.sig_q931.raw = causep->raw;
           break;
       }

   dc.result = xcall_release (&disconnectp);

   return ( dc.result );
}

/*------------- xcall_release -----------*/
/* release channel                       */
/*                                       */

ACUDLL int ACU_WINAPI xcall_release ( struct disconnect_xparms * disconnectp )
   {
   ACU_INT  uhandle;
   DC   dc;
   ACU_INT unet;


   uhandle = disconnectp->handle;            /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( disconnectp->handle, &dc );
      unet = handle2net(disconnectp->handle);  
      switch(call_type(unet))
          {
          case S_ISUP :
              if (disconnectp->unique_xparms.sig_isup.progress_indicator.ie[0] >= MAXPROGRESS)  /* check progress_indicator length */
                  {
                  dc.result = ERR_PARM;
                  }
              break;
          default :
              if (disconnectp->unique_xparms.sig_q931.progress_indicator.ie[0] >= MAXPROGRESS)  /* check progress_indicator length */
                  {
                  dc.result = ERR_PARM;
                  }
              else if (disconnectp->unique_xparms.sig_q931.display.ie[0] >= MAXDISPLAY)  /* check display length */
                  {
                  dc.result = ERR_PARM;
                  }
              break;
          }

      if ( dc.result >= 0 )            /* check for error */
         {
         disconnectp->handle = dc.handle;

         /* clear the entry before going to the driver */
         /* this will stop us calling the driver from  */
         /* the exit list routine                      */

         dc.result = clioctl ( CALL_RELEASE,
                              (IOCTLU *) disconnectp,
                              dc.card,unet,
                              sizeof ( DISCONNECT_XPARMS ));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   disconnectp->handle = uhandle;          /* save user handle */

   return ( dc.result );
   }
/*---------------------------------------*/



/*---------- call_progress --------------*/
/* send the progress message             */
/*                                       */

ACUDLL int ACU_WINAPI call_progress ( struct progress_xparms * progressp )
   {
   ACU_INT  uhandle;
   DC       dc;
   ACU_INT unet;


   uhandle = progressp->handle;            /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( progressp->handle, &dc );
      unet = handle2net(progressp->handle);

      switch(call_type(unet))
         {
        case S_SIP:
            dc.result = ERR_COMMAND;
            break;
          
         case S_H323 :
            /* also check display length */
            if (progressp->unique_xparms.sig_h323.display.ie[0] >= MAXDISPLAY)
               {
               dc.result = ERR_PARM;
               }
            break;

         case S_ISUP :
            if (progressp->unique_xparms.sig_isup.progress_indicator.ie[0] >= MAXPROGRESS)  /* check progress_indicator length */
                {
                dc.result = ERR_PARM;
                }
            break;
         default :
            if (progressp->unique_xparms.sig_q931.progress_indicator.ie[0] == 0x00 ||
                progressp->unique_xparms.sig_q931.progress_indicator.ie[0] >= MAXPROGRESS)  /* check progress_indicator length */
                {
                dc.result = ERR_PARM;
                }
            else if (progressp->unique_xparms.sig_q931.display.ie[0] >= MAXDISPLAY)         /* check display length */
                {
                dc.result = ERR_PARM;
                }
            break;
         }


      if ( dc.result >= 0 )               /* check for error */
         {
         progressp->handle = dc.handle;

         dc.result = clioctl ( CALL_PROGRESS,
                               (IOCTLU *) progressp,
                               dc.card,unet,
                               sizeof ( PROGRESS_XPARMS ));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   progressp->handle = uhandle;          /* save user handle */

   return ( dc.result );
   }
/*---------------------------------------*/



/*---------- call_proceeding-------------*/
/* send the call proceeding message      */
/*                                       */
ACUDLL int ACU_WINAPI call_proceeding ( struct proceeding_xparms * proceedingp )
   {
   ACU_INT  uhandle;
   DC       dc;
   ACU_INT   i;
   ACU_INT unet;

   uhandle = proceedingp->handle;            /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( proceedingp->handle, &dc );
      unet = handle2net(proceedingp->handle);

      switch(call_type(unet))
          {
        case S_SIP:
            dc.result = ERR_COMMAND;
            break;

          case S_H323 :
             if ( (proceedingp->unique_xparms.sig_h323.codecs[0] == 0) &&
                   default_codecs[0] )
                {
                /* load default codecs here */
                for ( i = 0; i < MAXCODECS; i++ )
                   {
                   proceedingp->unique_xparms.sig_h323.codecs[i] = default_codecs[i];
                   /* 0 signifies end of codec list so stop assignment here for efficiency */
                   if (default_codecs[i] == 0 )
                      break;
                   }
                }
                /* check GK assignment */
             if ( default_ras_info.request_admission )
                {
                /* load default GK info */
                proceedingp->unique_xparms.sig_h323.request_admission = default_ras_info.request_admission; /* required after change to defaults */
                proceedingp->unique_xparms.sig_h323.gk_addr = default_ras_info.gk_addr;
                for ( i = 0; i < default_ras_info.endpoint_identifier_length; i++ )
                   {
                   proceedingp->unique_xparms.sig_h323.endpoint_identifier[i] = default_ras_info.endpoint_identifier[i];
                   }
                 proceedingp->unique_xparms.sig_h323.endpoint_identifier_length = default_ras_info.endpoint_identifier_length;
                }

             break;

          case S_ISUP :
              if (proceedingp->unique_xparms.sig_isup.progress_indicator.ie[0] >= MAXPROGRESS)  /* check progress_indicator length */
                  {
                  dc.result = ERR_PARM;
                  }
              break;
          default :
              if (proceedingp->unique_xparms.sig_q931.progress_indicator.ie[0] >= MAXPROGRESS)  /* check progress_indicator length */
                  {
                  dc.result = ERR_PARM;
                  }
              else if (proceedingp->unique_xparms.sig_q931.display.ie[0] >= MAXDISPLAY)         /* check display length */
                  {
                  dc.result = ERR_PARM;
                  }
              break;
          }

      if ( dc.result >= 0 )            /* check for error */
         {
         /* load default codecs here */  /* is this still required was removed in DLL code. */
         for ( i = 0; i < MAXCODECS; i++ )
            {
            proceedingp->unique_xparms.sig_h323.codecs[i] = default_codecs[i];
            /* 0 signifies end of codec list so stop assignment here for efficiency */
            if (default_codecs[i] == 0 )
               break;
            }
         proceedingp->handle = dc.handle;

         dc.result = clioctl ( CALL_PROCEEDING,
                               (IOCTLU *) proceedingp,
                               dc.card, unet,
                               sizeof ( PROCEEDING_XPARMS ));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   proceedingp->handle = uhandle;          /* save user handle */

   return ( dc.result );
   }

/*---------------------------------------*/



/*---------- call_setup_ack-------------*/
/* send the call setup_ack message      */
/*                                      */
ACUDLL int ACU_WINAPI call_setup_ack ( struct setup_ack_xparms * setup_ackp )
   {
   ACU_INT  uhandle;
   DC       dc;

   uhandle = setup_ackp->handle;            /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( setup_ackp->handle, &dc );

      switch(call_type(handle2net(setup_ackp->handle)))
         {
        case S_SIP:
            dc.result = ERR_COMMAND;
            break;

         default :
             if (setup_ackp->unique_xparms.sig_q931.progress_indicator.ie[0] >= MAXPROGRESS)  /* check progress_indicator length */
                 {
                 dc.result = ERR_PARM;
                 }
             else if (setup_ackp->unique_xparms.sig_q931.display.ie[0] >= MAXDISPLAY)  /* check display length */
                 {
                 dc.result = ERR_PARM;
                 }
             break;
         }


      if ( dc.result >= 0 )            /* check for error */
         {
         setup_ackp->handle = dc.handle;

         dc.result = clioctl ( CALL_SETUP_ACK,
                               (IOCTLU *) setup_ackp,
                               dc.card, handle2net(uhandle),
                               sizeof ( SETUP_ACK_XPARMS ));
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   setup_ackp->handle = uhandle;          /* save user handle */

   return ( dc.result );
   }

/*---------------------------------------*/



/*------------ call_notify --------------*/
/* send the notify message               */
/*                                       */
ACUDLL int ACU_WINAPI call_notify ( struct notify_xparms * notifyp )
   {
   ACU_INT  uhandle;
   DC       dc;


   uhandle = notifyp->handle;            /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( notifyp->handle, &dc );

      if ( dc.result >= 0 )            /* check for error */
         {

         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }

         if (notifyp->unique_xparms.sig_q931.notify_indicator.ie[0] == 0x00 ||
             notifyp->unique_xparms.sig_q931.notify_indicator.ie[0] >= MAXNOTIFY)  /* check notify_indicator length */
             {
             dc.result = ERR_PARM;
             }
         else
             {
             notifyp->handle = dc.handle;

             dc.result = clioctl ( CALL_NOTIFY,
                                   (IOCTLU *) notifyp,
                                   dc.card, handle2net(uhandle),
                                   sizeof ( NOTIFY_XPARMS ));
             }
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   notifyp->handle = uhandle;          /* save user handle */

   return ( dc.result );
   }

/*---------------------------------------*/



/*------------ call_feature_openout ----------------------*/
/* initialise outgoing call with feature information      */
/*                                                        */
ACUDLL int ACU_WINAPI call_feature_openout ( struct feature_out_xparms * outdetailsp )
   {
   return ( feature_openout_enquiry ( CALL_FEATURE_OPENOUT, outdetailsp ));
   }
/*---------------------------------------*/


/*------------ call_feature_enquiry -----------------------*/
/* initialise outgoing call  with feature information      */
/*                                                         */
ACUDLL int ACU_WINAPI call_feature_enquiry ( struct feature_out_xparms * enquiryp )
   {
   return ( feature_openout_enquiry ( CALL_FEATURE_ENQUIRY, enquiryp ));
   }
/*---------------------------------------*/

/*---------- feature_openout_enquiry ------------*/
/* run feature_openout or feature_enquiry        */
/*                                               */
static int feature_openout_enquiry ( int mode, struct feature_out_xparms * calloutp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;
   ACU_INT  i;
   ACU_INT sig_net  = calloutp->net ;	/* Fixed up by xlate_assoc */
	   
   card = unet_2_card ( sig_net );        /* get the card number */
	   

   unet = calloutp->net;            /* save user net */

   calloutp->cnf |= CNF_ENABLE_V5API;     /* Enable V5 drivers events & features not found with V4 drivers */

   result = clopendev ( );

   if ( result == 0 )
      {
      /* Check for VoIP card */
      if (card >= 0 && clcard[card].voipservice == ACU_VOIP_ACTIVE)
         {
         calloutp->net = unet_2_net ( sig_net ); /* and the network port */

         /* check codec assignment */
         if ( (calloutp->unique_xparms.sig_h323.codecs[0] == 0) &&
               default_codecs[0] )
            {
            for ( i = 0; i < MAXCODECS; i++ )
               {
               calloutp->unique_xparms.sig_h323.codecs[i] = default_codecs[i];
               /* 0 signifies end of codec list so stop assignment here for efficiency */
               if (default_codecs[i] == 0 )
                  break;
               }
            }

         /* check GK assignment */
         if ( default_ras_info.request_admission )
            {

            /* load default GK info */
            calloutp->unique_xparms.sig_h323.request_admission = default_ras_info.request_admission;
            calloutp->unique_xparms.sig_h323.gk_addr = default_ras_info.gk_addr;
            for ( i = 0; i < default_ras_info.endpoint_identifier_length; i++ )
               {
               calloutp->unique_xparms.sig_h323.endpoint_identifier[i] = default_ras_info.endpoint_identifier[i];
               }
            calloutp->unique_xparms.sig_h323.endpoint_identifier_length = default_ras_info.endpoint_identifier_length;
            }

         switch (calloutp->feature_information) 
            {
            case FEATURE_NON_STANDARD:
               /* Too much data anywhere? */
               if(calloutp->feature.non_standard.length> MAXRAWDATA)
                  {
                  result = ERR_PARM;
                  }
               switch(calloutp->feature.non_standard.id_type) 
                  {
                  case NON_STANDARD_ID_TYPE_H221:
                  break;
                  case NON_STANDARD_ID_TYPE_OBJECT:
                     if(calloutp->feature.non_standard.id.object_id.length> MAXOID)
                        {
                        result = ERR_PARM;
                        }
                  break;
                  default:
                     result = ERR_PARM;
                  }
               break;
               default:
                  result = ERR_PARM;
               break;
            }

         if (result == 0) 
            {
            result = clioctl ( mode,
                               (IOCTLU *) calloutp,
                               card, sig_net,
                               sizeof (FEATURE_OUT_XPARMS));
            if (result == 0)
               {
               /* now patch the handle to reflect the network */
               calloutp->handle = patch_handle (calloutp->handle, sig_net );
               }
            }

            return(result);
         }
      else 
         {
         /* Translate user's bearer network to actual signalling network... */
         /* and associated bearer network port...                           */
         ACU_INT assoc_net = xlate_assoc(&calloutp->net) ;

         if ( card >= 0 )                    /* check for error */
            {

            calloutp->net = unet_2_net ( sig_net ); /* and the network port */

            if ( calloutp->net >= 0 )     /* check for error */
               {
               switch (call_type(calloutp->net))
                  {
                  case S_ISUP :
                     if (calloutp->unique_xparms.sig_isup.bearer.ie[0] >= MAXBEARER)
                        {
                        result = ERR_SERVICE;
                        }
                     else if (calloutp->unique_xparms.sig_isup.lolayer.ie[0] >= MAXLOLAYER)
                        {
                        result = ERR_SERVICE;
                        }
                     else if (calloutp->unique_xparms.sig_isup.hilayer.ie[0] >= MAXHILAYER)
                        {
                        result = ERR_SERVICE;
                        }
                     else if (calloutp->unique_xparms.sig_isup.progress_indicator.ie[0] >= MAXPROGRESS)
                        {
                        result = ERR_PARM;
                        }
                  break ;

                  default :
                     if (calloutp->unique_xparms.sig_q931.bearer.ie[0] >= MAXBEARER)
                        {
                        result = ERR_SERVICE;
                        }
                     else if (calloutp->unique_xparms.sig_q931.lolayer.ie[0] >= MAXLOLAYER)
                        {
                        result = ERR_SERVICE;
                        }
                        else if (calloutp->unique_xparms.sig_q931.hilayer.ie[0] >= MAXHILAYER)
                        {
                        result = ERR_SERVICE;
                        }
                    else if (calloutp->unique_xparms.sig_q931.progress_indicator.ie[0] >= MAXPROGRESS)
                        {
                        result = ERR_PARM;
                        }
                   
                    if((calloutp->feature_information & FEATURE_FACILITY) == FEATURE_FACILITY)
                       {
                       if(calloutp->feature.facility.length> MAXFACILITY_INFO)
                          {
                          result = ERR_PARM;
                          }
                       }
                    if((calloutp->feature_information & FEATURE_USER_USER) == FEATURE_USER_USER)
                       {
                       if(calloutp->feature.uui.length> MAXUUI_INFO)
                          {
                          result = ERR_PARM;
                          }
                       }
                  
                    if((calloutp->feature_information & FEATURE_RAW_DATA) == FEATURE_RAW_DATA)
                       {
                       if(calloutp->feature.raw_data.length> MAXRAWDATA)
                          {
                          result = ERR_PARM;
                          }
                       }
                   
                    if((calloutp->feature_information & FEATURE_MLPP) == FEATURE_MLPP)
                       {
                       if( (calloutp->feature.mlpp.Prec_level<0) ||
                           (calloutp->feature.mlpp.Prec_level>4)     )
                          {
                          result = ERR_PARM;
                          }
                       if( (calloutp->feature.mlpp.LFB_Indictn<0) ||
                           (calloutp->feature.mlpp.LFB_Indictn>2)     )
                          {
                          result = ERR_PARM;
                          }
                       for (i=0;i<5;i++)      /* if any part of Svc_Domn is 0 */
                          {
                          if (calloutp->feature.mlpp.MLPP_Svc_Domn[i]==0) 
                             result = ERR_PARM;
                          }
                       }
                   break;
                   }
                
               if (result == 0)
                  {
                  /* We encode associated bearer net in upper bits of sig_net... */
                  calloutp->net |= ((assoc_net +1) << 8) ;

                  if (  !isallowed_string ( calloutp->destination_addr )       /* check if the characaters in the string are allowed */
                     || !isallowed_string ( calloutp->originating_addr ))
                      {
                      result = ERR_PARM;
                      }
                  else
                      {
                      result = clioctl ( mode,
                                         (IOCTLU *) calloutp,
                                         card, sig_net,
                                         sizeof (FEATURE_OUT_XPARMS));
                      }

                  if ( result == 0 )
                     {
                     /* now patch the handle to reflect the network */
                     calloutp->handle = patch_handle (calloutp->handle, sig_net );
                     }
                  }
               }
            else
               {
               result = calloutp->net;  /* return error code */
               }
            }
         else
            {
            result = card;                 /* return error code */
            }
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   calloutp->net = unet;             /* restore user parameter */
   
   return ( result );
   }
/*---------------------------------------*/

/*------------ call_feature_details-----------*/
/* get feature details                        */
/*                                            */
ACUDLL int ACU_WINAPI call_feature_details ( struct feature_detail_xparms * detailsp )
   {
   ACU_INT  uhandle;
   DC   dc;


   uhandle = detailsp->handle;               /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( detailsp->handle, &dc );

      if ( dc.result >= 0 )                  /* check for error */
         {

         detailsp->handle = dc.handle;       /* get the fixed up handle */
         if(detailsp->feature_type == 0)
            {
            dc.result = ERR_PARM;
            }
         if ( dc.result >= 0 )               /* check for error */
            {

            dc.result = clioctl ( CALL_FEATURE_DETAILS,
                                  (IOCTLU *) detailsp,
                                  dc.card, handle2net(uhandle),
                                  sizeof (FEATURE_DETAIL_XPARMS));
            }
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   detailsp->handle = uhandle;

   return ( dc.result );
   }
/*---------------------------------------*/

/*------------ call_feature_send--------------*/
/* transmit feature information               */
/*                                            */
ACUDLL int ACU_WINAPI call_feature_send ( struct feature_detail_xparms * detailsp )
   {
   ACU_INT  uhandle;
   DC   dc;


   uhandle = detailsp->handle;               /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( detailsp->handle, &dc );

      if ( dc.result >= 0 )                  /* check for error */
         {

         /* Most feature_types not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE
             && detailsp->feature_type != FEATURE_NON_STANDARD
			 && S_SIP != call_type(call_handle_2_port(detailsp->handle)))
            {
            dc.result = ERR_PARM;
            }

         detailsp->handle = dc.handle;       /* get the fixed up handle */
         switch(detailsp->feature_type)
            {
            case 0:
            case FEATURE_HOLD_RECONNECT:
            case FEATURE_TRANSFER:
            case FEATURE_DIVERSION:
            case FEATURE_MLPP:
               break;
            case FEATURE_FACILITY:
               /* return error if length of facility data exceeds limit */
               if(detailsp->feature.facility.length> MAXFACILITY_INFO
                   || (detailsp->feature.facility.length == 0) )
                  {
                  dc.result = ERR_PARM;
                  }
               break;
            case FEATURE_USER_USER:
               /* return error if length of user to user data exceeds limit */
               if(detailsp->feature.uui.length> MAXUUI_INFO)
                  {
                  dc.result = ERR_PARM;
                  }
               break;
            case FEATURE_RAW_DATA:
               /* return error if length of raw data exceeds limit */
               if(detailsp->feature.raw_data.length> MAXRAWDATA)
                  {
                  dc.result = ERR_PARM;
                  }
               break;
            case FEATURE_NON_STANDARD:
               /* Too much data anywhere? */
               if(detailsp->feature.non_standard.length> MAXRAWDATA)
                  {
                  dc.result = ERR_PARM;
                  }
               switch(detailsp->feature.non_standard.id_type) 
                  {
                  case NON_STANDARD_ID_TYPE_H221:
                  break;
                  case NON_STANDARD_ID_TYPE_OBJECT:
                     if(detailsp->feature.non_standard.id.object_id.length> MAXOID)
                        {
                        dc.result = ERR_PARM;
                        }
                  break;
                  default:
                     dc.result = ERR_PARM;
                  }
               break;
            default:
               dc.result = ERR_PARM;
               break;
         }

         if ( dc.result >= 0 )                  /* check for error */
            {
            dc.result = clioctl ( CALL_FEATURE_SEND,
                                  (IOCTLU *) detailsp,
                                  dc.card, handle2net(uhandle),
                                  sizeof (FEATURE_DETAIL_XPARMS));
            }
         }
      }
   else
      {
      dc.result = ERR_CFAIL;
      }

   detailsp->handle = uhandle;

   return ( dc.result );
   }
/*---------------------------------------*/

/*  call_send_connectionless
 * A connectionless transport mechanism is used to send a single message
 * to a destination user
 * See ETS300 196 8.3.2.2,8.3.2.4 - EuroISDN
 * See ISO/IEC 11582 7.2          - QSIG
 */

ACUDLL int ACU_WINAPI call_send_connectionless ( FEATURE_DETAIL_XPARMS *fdetailsp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;


   result = clopendev ( );               /* open the device */

   unet = fdetailsp->net;                /* save user net    */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );       /* get the card number */

      if ( card >= 0 )                   /* check for error */
         {

         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }

         fdetailsp->net = unet_2_net ( unet ); /* and the network port */

         if ( fdetailsp->net >= 0 )     /* check for error */
            {
            switch(fdetailsp->feature_type)
               {
               case FEATURE_FACILITY:
                  if ( (fdetailsp->feature.facility.length > MAXFACILITY_INFO)
                    || (fdetailsp->feature.facility.length == 0) )
                     {
                     result = ERR_PARM;
                     }
               break;
               case FEATURE_RAW_DATA:
                  if( (fdetailsp->feature.raw_data.length > MAXRAWDATA)
                     || (fdetailsp->feature.raw_data.length == 0) )
                      {
                      result = ERR_PARM;
                      }
               break;

               default:
                  result = ERR_PARM;
               break;
               }

            if (result==0)
               {
               result = clioctl ( CALL_SEND_CONNECTIONLESS,
                                  (IOCTLU *) fdetailsp,
                                  card, unet,
                                  sizeof (FEATURE_DETAIL_XPARMS));
               }
            }
         else
            {
            result = fdetailsp->net;    /* return error code */
            }
         }
      else
         {
         result = card;                 /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   fdetailsp->net = unet;               /* restore user parameter */

   return ( result );
   }
/*---------------------------------------*/



/*  call_get_connectionless
 * Retrieve information from a connectionless message
 * See ETS300 196 8.3.2.2,8.3.2.4 - EuroISDN
 * See ISO/IEC 11582 7.2          - QSIG
 */
ACUDLL int ACU_WINAPI call_get_connectionless ( FEATURE_DETAIL_XPARMS *fdetailsp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;


   result = clopendev ( );               /* open the device */

   unet = fdetailsp->net;                /* save user net    */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );       /* get the card number */

      if ( card >= 0 )                   /* check for error */
         {

         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }

         fdetailsp->net = unet_2_net ( unet ); /* and the network port */

         if ( fdetailsp->net >= 0 )     /* check for error */
            {
            result = clioctl ( CALL_GET_CONNECTIONLESS,
                               (IOCTLU *) fdetailsp,
                               card, unet,
                               sizeof (FEATURE_DETAIL_XPARMS));
            }
         else
            {
            result = fdetailsp->net;    /* return error code */
            }
         }
      else
         {
         result = card;                 /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   fdetailsp->net = unet;               /* restore user parameter */

   return ( result );
   }
/*---------------------------------------*/


/*----------- call_send_alarm -----------*/
/* send an alarm through layer 1         */
/*                                       */
ACUDLL int ACU_WINAPI call_send_alarm ( struct alarm_xparms * alarmp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;


   unet = alarmp->net;                  /* save user net */

   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );      /* get the card number */

      if ( card >= 0 )                  /* check for error */
         {
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }

         alarmp->net = unet_2_net ( unet ); /* and the network port */

         if ( alarmp->net >= 0 )        /* check for error */
            {
            result = clioctl ( CALL_SEND_ALARM,
                               (IOCTLU *) alarmp,
                               card, unet,
                               sizeof (ALARM_XPARMS ));

            }
         else
            {
            result = alarmp->net;       /* return error code */
            }
         }
      else
         {
         result = card;                 /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   alarmp->net = unet;                  /* restore user parameter */

   return ( result );
   }
/*---------------------------------------*/


/*----------- call_send_q921 -----------*/
/* send an raw q921 message             */
/*                                      */
ACUDLL int call_send_q921 ( struct q921_xparms * q921p )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;


   unet = q921p->net;                  /* save user net */

   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );      /* get the card number */

      if ( card >= 0 )                  /* check for error */
         {
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }

         q921p->net = unet_2_net ( unet ); /* and the network port */

         if ( q921p->net >= 0 )        /* check for error */
            {

            result = clioctl ( CALL_SEND_Q921,
                               (IOCTLU *) q921p,
                               card, unet,
                               sizeof (Q921_XPARMS ));

            }
         else
            {
            result = q921p->net;       /* return error code */
            }
         }
      else
         {
         result = card;                 /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   q921p->net = unet;                  /* restore user parameter */

   return ( result );
   }
/*---------------------------------------*/


/*------------ call_get_q921 -------------*/
/* get call get q921 packet               */
/*                                        */
ACUDLL int call_get_q921 ( struct q921_xparms * q921p )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;


   unet = q921p->net;                  /* save user net */

   result = clopendev ( );

   if ( result == 0 )
      {
      card = unet_2_card ( unet );      /* get the card number */

      if ( card >= 0 )                  /* check for error */
         {
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }

         q921p->net = unet_2_net ( unet ); /* and the network port */

         if ( q921p->net >= 0 )        /* check for error */
            {
            result = clioctl ( CALL_GET_Q921,
                               (IOCTLU *) q921p,
                               card, unet,
                               sizeof (Q921_XPARMS));
            }
         else
            {
            result = q921p->net;       /* return error code */
            }
         }
      }
   else
      {
        result = ERR_CFAIL;
      }


   return ( result );
   }
/*---------------------------------------*/

/*----------- call_watchdog -----------*/
/* send watchdog tick                  */
/*                                     */
ACUDLL int ACU_WINAPI call_watchdog ( struct watchdog_xparms * watchp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;


   unet = watchp->net;                  /* save user net */

   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );      /* get the card number */

      if ( card >= 0 )                  /* check for error */
         {
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }
         watchp->net = unet_2_net ( unet ); /* and the network port */

         if ( watchp->net >= 0 )        /* check for error */
            {
            result = clioctl ( CALL_WATCHDOG,
                               (IOCTLU *) watchp,
                               card, unet,
                               sizeof (WATCHDOG_XPARMS ));

            }
         else
            {
            result = watchp->net;       /* return error code */
            }
         }
      else
         {
         result = card;                 /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   watchp->net = unet;                  /* restore user parameter */

   return ( result );
   }
/*---------------------------------------*/

#ifdef ACU_TEST
/*----------- send_dpr_command -----------*/
/* send dpr command                       */
/*                                        */
ACUDLL int send_dpr_command ( struct dpr_xparms * dprp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;


   unet = dprp->net;                  /* save user net */

   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );      /* get the card number */

      if ( card >= 0 )                  /* check for error */
         {
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }

         dprp->net = unet_2_net ( unet ); /* and the network port */

         if ( dprp->net >= 0 )        /* check for error */
            {
            result = clioctl ( SEND_DPR_COMMAND,
                               (IOCTLU *) dprp,
                               card, unet,
                               sizeof (DPR_XPARMS ));

            }
         else
            {
            result = dprp->net;       /* return error code */
            }
         }
      else
         {
         result = card;                 /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   dprp->net = unet;                  /* restore user parameter */

   return ( result );
   }
/*---------------------------------------*/

/*----------- receive_dpr_event -----------*/
/* send dpr command                       */
/*                                        */
ACUDLL int receive_dpr_event ( struct dpr_xparms * dprp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;


   unet = dprp->net;                  /* save user net */

   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );      /* get the card number */

      if ( card >= 0 )                  /* check for error */
         {
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }

         dprp->net = unet_2_net ( unet ); /* and the network port */

         if ( dprp->net >= 0 )        /* check for error */
            {
            result = clioctl ( RECEIVE_DPR_EVENT,
                               (IOCTLU *) dprp,
                               card, unet,
                               sizeof (DPR_XPARMS ));

            }
         else
            {
            result = dprp->net;       /* return error code */
            }
         }
      else
         {
         result = card;                 /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   dprp->net = unet;                  /* restore user parameter */

   return ( result );
   }


/*---------------------------------------*/
#endif

/*------------- call_l1_stats -----------*/
/* get layer 1 statistics                */
/*                                       */
ACUDLL int ACU_WINAPI call_l1_stats ( struct l1_xstats * l1_xstatsp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;



   unet = l1_xstatsp->net;               /* save user net */

   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );      /* get the card number */

      if ( card >= 0 )                  /* check for error */
         {
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }

         l1_xstatsp->net = unet_2_net ( unet ); /* and the network port */

         if ( l1_xstatsp->net >= 0 )     /* check for error */
            {
            result = clioctl ( CALL_L1_STATS,
                               (IOCTLU *) l1_xstatsp,
                               card, unet,
                               sizeof (L1_XSTATS ));

            }
         else
            {
            result = l1_xstatsp->net;    /* return error code */
            }
         }
      else
         {
         result = card;                 /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   l1_xstatsp->net = unet;              /* restore user parameter */

   return ( result );
   }
/*---------------------------------------*/


/*------------- call_l2_state -----------*/
/* get layer 1 statistics                */
/*                                       */
ACUDLL int ACU_WINAPI call_l2_state ( struct l2_xstate * l2_xstatep )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;



   unet = l2_xstatep->net;               /* save user net */

   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      /* Translate user's bearer network to actual signalling network... */
      /* and associated bearer network port...                           */
      ACU_INT assoc_net = xlate_assoc(&l2_xstatep->net) ;
      ACU_INT sig_net  = l2_xstatep->net ;	/* Fixed up by xlate_assoc */

      card = unet_2_card ( sig_net );      /* get the card number */

      if ( card >= 0 )                  /* check for error */
         {
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }

         l2_xstatep->net = unet_2_net ( sig_net ); /* and the network port */

         if ( l2_xstatep->net >= 0 )    /* check for error */
            {
            /* We encode associated bearer net in upper bits of sig_net... */
            l2_xstatep->net |= ((assoc_net +1) << 8) ;

            result = clioctl ( CALL_L2_STATE,
                               (IOCTLU *) l2_xstatep,
                               card, unet,
                               sizeof (L2_XSTATE ));

            }
         else
            {
            result = l2_xstatep->net;   /* return error code */
            }
         }
      else
         {
         result = card;                /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   l2_xstatep->net = unet;              /* restore user parameter */

   return ( result );
   }
/*---------------------------------------*/


/*---------- call_br_l1_stats -----------*/
/* get layer 1 statistics                */
/*                                       */
ACUDLL int call_br_l1_stats ( struct br_l1_xstats * br_l1_xstatsp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;



   unet = br_l1_xstatsp->net;            /* save user net */

   result = clopendev ( );               /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );       /* get the card number */

      if ( card >= 0 )                   /* check for error */
         {

         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }

         br_l1_xstatsp->net = unet_2_net ( unet ); /* and the network port */

         if ( br_l1_xstatsp->net >= 0 )  /* check for error */
            {
            result = clioctl ( CALL_BR_L1_STATS,
                               (IOCTLU *) br_l1_xstatsp,
                               card, unet,
                               sizeof (BR_L1_XSTATS ));

            }
         else
            {
            result = br_l1_xstatsp->net; /* return error code */
            }
         }
      else
         {
         result = card;                  /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   br_l1_xstatsp->net = unet;            /* restore user parameter */

   return ( result );
   }
/*---------------------------------------*/


/*--------------- call_dcba -------------*/
/* get CAS abcd bits                     */
/*                                       */
ACUDLL int ACU_WINAPI call_dcba ( struct dcba_xparms * dcbap )
   {
   int  result;
   int  card;
   int  unet;



   unet = dcbap->net;                   /* save user net */

   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );      /* get the card number */

      if ( card >= 0 )                  /* check for error */
         {

         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }

         dcbap->net = unet_2_net ( unet ); /* and the network port */

         if ( dcbap->net >= 0 )    /* check for error */
            {
            result = clioctl ( CALL_DCBA,
                               (IOCTLU *) dcbap,
                               card, unet,
                               sizeof (DCBA_XPARMS ));

            }
         else
            {
            result = dcbap->net;        /* return error code */
            }
         }
      else
         {
         result = card;                 /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   dcbap->net = unet;              /* restore user parameter */

   return ( result );
   }
/*---------------------------------------*/


/*-------- call_protocol_trace ----------*/
/* get the protocol trace from dpr       */
/*                                       */
ACUDLL int ACU_WINAPI call_protocol_trace ( struct log_xparms * logp )
   {
   int  result;
   int  card;
   int  unet;


   unet = logp->net;                   /* save user net */

   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );      /* get the card number */

      if ( card >= 0 )                  /* check for error */
         {
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }

         logp->net = unet_2_net ( unet ); /* and the network port */

         if ( logp->net >= 0 )    /* check for error */
            {
            result = clioctl ( CALL_PROTOCOL,
                               (IOCTLU *) logp,
                               card, unet,
                               sizeof (LOG_XPARMS ));

            }
         else
            {
            result = logp->net;        /* return error code */
            }
         }
      else
         {
         result = card;                 /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   logp->net = unet;              /* restore user parameter */

   return ( result );
   }
/*---------------------------------------*/


/*------------- call_pblock -------------*/
/* copy pblock to call driver            */
/*                                       */
ACUDLL int call_pblock ( struct v5_pblock_xparms * pblockp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;

   unet = pblockp->net;                 /* save user net */

   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );      /* get the card number */

      if ( card >= 0 )                  /* check for error */
         {
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }

         pblockp->net = unet_2_net ( unet ); /* and the network port */

         if ( pblockp->net >= 0 )    /* check for error */
            {
            result = clpblock_ioctl ( CALL_V5PBLOCK,
                                     (V5_PBLOCK_IOCTLU *) pblockp,
                                      card,
                                      sizeof (V5_PBLOCK_XPARMS));
            }
         else
            {
            result = pblockp->net;   /* return error code */
            }
         }
      else
         {
         result = card;                /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   pblockp->net = unet;                /* restore user parameter */

   return ( result );
   }
/*---------------------------------------*/


/*-------------- call_tcmd --------------*/
/* send tcmd to call driver              */
/*                                       */
ACUDLL int call_tcmd ( struct tcmd_xparms * tcmdp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;


   unet = tcmdp->net;                   /* save user net */

   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );      /* get the card number */

      if ( card >= 0 )                  /* check for error */
         {
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }
         tcmdp->net = unet_2_net ( unet ); /* and the network port */

         if ( tcmdp->net >= 0 )    /* check for error */
            {
            result = clioctl ( CALL_TCMD,
                               (IOCTLU *) tcmdp,
                               card, unet,
                               sizeof (TCMD_XPARMS));

            }
         else
            {
            result = tcmdp->net;        /* return error code */
            }
         }
      else
         {
         result = card;                 /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   tcmdp->net = unet;                   /* restore user parameter */

   return ( result );
   }
/*---------------------------------------*/

/*-------------- call_dsp_config---------*/
/* get dsp configuration                 */
/*                                       */

ACUDLL int ACU_WINAPI call_dsp_config ( struct dsp_xparms * pdsp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;


   unet = pdsp->net;                   /* save user net */

   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );      /* get the card number */

      if ( card >= 0 )                  /* check for error */
         {
         pdsp->net = unet_2_net ( unet ); /* and the network port */

         if ( pdsp->net >= 0 )    /* check for error */
            {

            result = clioctl ( CALL_DSP_CONFIG,
                               (IOCTLU *) pdsp,
                               card, unet,
                               sizeof (DSP_XPARMS));
            }
         else
            {
            result = pdsp->net;        /* return error code */
            }
         }
      else
         {
         result = card;                /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   pdsp->net = unet;                   /* restore user parameter */

   return ( result );

   }
/*---------------------------------------*/




/*-------------- call_codec_config---------*/
/* Set codec preferences for VoIP          */
/*                                         */
ACUDLL int call_codec_config ( struct codec_xparms * codecp )
   {
   ACU_INT  i;

   /* load default codecs here */
   for ( i = 0; i < MAXCODECS; i++ )
      {
      default_codecs[i] = codecp->unique_xparms.sig_h323.codecs[i];
      /* 0 signifies end of codec list so stop assignment here for efficiency */
      if (default_codecs[i] == 0 )
         break;
      }

   return ( 0 );

   }
/*---------------------------------------*/


/*----------- call_get_configuration-------*/
/* get board configuration                 */
/*                                         */
ACUDLL int call_get_configuration ( struct get_configuration_xparms * pdsp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;

   unet = pdsp->net;                    /* save user net */

   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );      /* get the card number */
      pdsp->board_ip_address = clcard[card].board_ip_address;

      if ( card >= 0 )                  /* check for error */
         {
         /* If VoIP card then thread identification is required */
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            pdsp->net = unet_2_net ( unet ); /* and the network port */

            if ( pdsp->net >= 0 )    /* check for error */
               {
               result = clioctl ( CALL_GET_CONFIGURATION, 
                                  (IOCTLU *) pdsp,
                                  card, unet,
                                  sizeof (GET_CONFIGURATION_XPARMS));
               }
            else
               {
               result = pdsp->net;        /* return error code */
               }
            }
         else
            {
            return (ERR_COMMAND);
            }

         }
      else
         {
         result = card;                /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   pdsp->net = unet;                   /* restore user parameter */

   return ( result );

   }
/*-----------------------------------------*/

/*--------- call_set_configuration --------*/
/* Set board configuration                 */
/*                                         */
ACUDLL int call_set_configuration ( struct set_configuration_xparms * pdsp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;


   unet = pdsp->net;                    /* save user net */

   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );      /* get the card number */

      if ( card >= 0 )                  /* check for error */
         {
         /* If VoIP card then thread identification is required */
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            pdsp->net = unet_2_net ( unet ); /* and the network port */
            pdsp->board_ip_address = clcard[card].board_ip_address;

            if ( pdsp->net >= 0 )    /* check for error */
               {
               result = clioctl ( CALL_SET_CONFIGURATION,
                                  (IOCTLU *) pdsp,
                                  card, unet, 
                                  sizeof (SET_CONFIGURATION_XPARMS));
               }
            else
               {
               result = pdsp->net;        /* return error code */
               }
            }
         else
            {
            return ( ERR_COMMAND);
            }
         }
      else
         {
         result = card;                /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   pdsp->net = unet;                   /* restore user parameter */
   
   return ( result );

   }
/*---------------------------------------*/



/*-------------- call_get_stats-----------*/
/* get board stats                        */
/*                                        */
ACUDLL int call_get_stats ( struct voip_get_stats_xparms * pdsp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;


   unet = pdsp->net;                    /* save user net */

   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );      /* get the card number */

      if ( card >= 0 )                  /* check for error */
         {
         /* If VoIP card then thread identification is required */
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            pdsp->net = unet_2_net ( unet ); /* and the network port */
            pdsp->board_ip_address = clcard[card].board_ip_address;
   
            if ( pdsp->net >= 0 )    /* check for error */
               {
               result = clioctl ( CALL_GET_STATS,
                                  (IOCTLU *) pdsp,
                                  card, unet, 
                                  sizeof (VOIP_GET_STATS_XPARMS));
               }
            else
               {
               result = pdsp->net;        /* return error code */
               }
            }
         else
            {
            return ( ERR_COMMAND);
            }
         }
      else
         {
         result = card;                /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }
   
   pdsp->net = unet;                   /* restore user parameter */
   
   return ( result );
   
   }
/*---------------------------------------*/



/* VOIP only */
/*-------------- call_set_debug------------*/
/* get board configuration                 */
/*                                         */
ACUDLL int call_set_debug ( struct set_debug_xparms * pdsp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;


   unet = pdsp->net;                    /* save user net */

   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );      /* get the card number */

      if ( card >= 0 )                  /* check for error */
         {
         /* If VoIP card then thread identification is required */
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            pdsp->net = unet_2_net ( unet ); /* and the network port */
            pdsp->board_ip_address = clcard[card].board_ip_address;

            if ( pdsp->net >= 0 )    /* check for error */
               {
               result = clioctl ( CALL_SET_DEBUG,
                                  (IOCTLU *) pdsp,
                                  card, unet,
                                  sizeof (SET_DEBUG_XPARMS));
               }
            else
               {
               result = pdsp->net;        /* return error code */
               }
            }
         else
            {
            return (ERR_COMMAND);
            }
         }
      else
         {
         result = card;                /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }
   
   pdsp->net = unet;                   /* restore user parameter */
   
   return ( result );

   }
/*---------------------------------------*/



/*-------------- call_open_voip_admin_chan---------*/
/* Opens the VoIP administration channel           */
/*                                                 */
ACUDLL int call_open_voip_admin_chan()
{

  /* dummy function - admin channel no longer exists */
  return 0;
  
}
/*--------------------------------------------------*/

/*-------------- call_close_voip_admin_chan---------*/
/* Close the VoIP administration channel            */
/*                                                  */
ACUDLL int call_close_voip_admin_chan()
{
  /* dummy function - admin channel no longer exists */
  return 0;


}
/*---------------------------------------*/

/*--------------- call_send_voip_admin_msg ----------------*/
/* Send a VoIP administration message to the admin channel */
/*                                                         */
ACUDLL int call_send_voip_admin_msg( struct voip_admin_out_xparms * adminp)
{
   ACU_INT result;

   if (adminp->admin_msg != 0) 
   {
      /* Sequence numbers are more constrained than their type may indicate */
      if (adminp->admin_msg->sequence_number < 1 || 
	  adminp->admin_msg->sequence_number > 65535) 
      {
	 return ERR_PARM;
      }
   }
   else
   {
      return ERR_PARM;
   }

   /* VoIP admin channel exists so retrieve message */
   /* 
     Currently no way of determining whether this is for the SIP or the H323 
     service, so assume its for the H323 
   */
   result = srvioctl ( CALL_SEND_RAS_MSG, 
                       (IOCTLU *) adminp, 
                       sizeof ( VOIP_ADMIN_OUT_XPARMS ), 
                       1, 
                       0);

   return (result);
}
/*---------------------------------------------------------*/

/*------------------ call_get_voip_admin_msg --------------------*/
/* Retrieve a VoIP administration message from the admin channel */
/*                                                               */
ACUDLL int call_get_voip_admin_msg( struct voip_admin_in_xparms * adminp)
{
   ACU_INT result;

   /* VoIP admin channel exists so retrieve message */
   result = srvioctl ( CALL_GET_RAS_MSG, 
                       (IOCTLU *) adminp, 
                       sizeof ( VOIP_ADMIN_IN_XPARMS ),
                       1, 0);
   
   return (result);
}
/*---------------------------------------------------------*/


/*-------------- call_sfwm --------------*/
/* start the firmware after a download   */
/*                                       */

ACUDLL int call_sfmw ( int portnum, int mode, char * confp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;
   ACU_INT  usavenet;
   SFMW_XPARMS sfmw;

   usavenet = unet = (ACU_INT) portnum;

   result = clopendev ( );              /* open the device */

   if ( result == 0 )                   /* check the result */
      {
      card = unet_2_card ( unet );      /* get the card number */

      if ( card >= 0 )                  /* check for error */
         {
          unet = unet_2_net ( unet );    /* and the network port */

         if ( unet >= 0 )               /* check for error */
            {
            sfmw.net     = mode | unet;

#ifdef ACU_SOLARIS_SPARC
            sfmw.bufp_confstr = 0L;
#else
            sfmw.bufp_confstr = confp ;
#endif

            /* For V5 xparms, we supply a copy of confstr as well as a ptr to it,
            then the driver is free to use either... */

            if (confp != (char *) 0)
               {
               strcpy (sfmw.confstr, confp);
               }
            else
               {
               sfmw.confstr[0] = '\0';
               }

            sfmw.regapp  = 0;
            sfmw.ss_vers = 0;


            result = clioctl ( CALL_SFMW,
                               (IOCTLU *) &sfmw,
                               card,usavenet,
                               sizeof (SFMW_XPARMS));

            if ( result == 0 )
               {
               result = update_ss_type ( portnum );
               }
            }
         else
            {
            result = unet;              /* return error code */
            }
         }
      else
         {
         result = card;                 /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   return ( result );
   }
/*---------------------------------------*/


/*-------------- call_type --------------*/
/* return the type of signalling system  */
/*                                       */
ACUDLL int ACU_WINAPI call_type ( int portnum )
   {
   ACU_INT  card;
   ACU_INT  net;
   int  result;


   result = clopendev ( );                 /* open the call drivers */

   if ( result == 0 )
      {
      card = unet_2_card ((ACU_INT) portnum ); /* get the card number */

      net = unet_2_net ((ACU_INT) portnum );   /* and the network port */

      if ( card >= 0 && net >= 0 )         /* check if valid */
         {
         result = clcard[card].types[net]; /* return the type */
         }
      else
         {
         result = 0;
         }
      }
   else
      {
      result = 0;
      }

   return ( result );                      /* return the type */
   }
/*---------------------------------------*/

/*-------------- call_line --------------*/
/* return the type of signalling system  */
/*                                       */
ACUDLL int ACU_WINAPI call_line ( int portnum )
   {
   ACU_INT  card;
   ACU_INT  net;
   int  result;


   result = clopendev ( );                 /* open the call drivers */

   if ( result == 0 )
      {
      card = unet_2_card ((ACU_INT) portnum ); /* get the card number */

      net = unet_2_net ((ACU_INT) portnum );   /* and the network port */

      if ( card >= 0 && net >= 0 )         /* check if valid */
         {
         result = clcard[card].lines[net]; /* return the type */
         }
      else
         {
         result = 0;
         }
      }
   else
      {
      result = 0;
      }

   return ( result );                      /* return the type */
   }
/*---------------------------------------*/


/*-------- call_restart_fwm -------------*/
/* restarts the firmware after a download*/
/*                                       */

ACUDLL int ACU_WINAPI call_restart_fmw ( RESTART_XPARMS * restartp )
   {
   ACU_INT  result = 0;
   struct download_xparms download;
   char ip_address1[IP_ADDRESS_SIZE];
   char subnet_address1[IP_ADDRESS_SIZE];
   char ip_address2[IP_ADDRESS_SIZE];
   char subnet_address2[IP_ADDRESS_SIZE];
   char gateway_address[IP_ADDRESS_SIZE];
   struct set_sysinfo_xparms set_sysinfo;
   struct sysinfo_xparms     sysinfo;
   int using_dhcp;
   int card_number;

   card_number = unet_2_card (restartp->net);

   if (clcard[card_number].voipservice == ACU_VOIP_ACTIVE)
      {
      int voip_card_number;

       voip_card_number = card_2_voipcard(card_number);

      /* call function which extracts ip addresses from config string */
      result = parse_config_str(restartp->config_stringp,
                                ip_address1,
                                ip_address2,
                                subnet_address1,
                                subnet_address2,
                                gateway_address,
                               &using_dhcp);

      /* check that ip info was supplied */
      if (( ip_address1[0] == 0 ) || (subnet_address1[0] == 0)|| (gateway_address[0] == 0))
            return ERR_CFAIL;

      strcpy(clcard[card_number].board_ip_address,ip_address1);

      result = call_sfmw ( restartp->net,
                           SFMWMODE_RESTART,
                           restartp->config_stringp ); /* reset the card */

      download.net = restartp->net;
      download.filenamep = restartp->filenamep;

      if ( result == 0 )
         result = call_download_fmw ( &download );  /* transfer the file */
      
      set_sysinfo.net = restartp->net;
      set_sysinfo.board_number = voip_card_number;
      set_sysinfo.v1bmi_card_num = clcard[card_number].v1bmi_card_num;
      call_set_system_info ( &set_sysinfo, &sysinfo );

      pipe_client_send_application_terminated(voip_card_number);

      return ( result );
      }
   else
      {
		ACU_INT attempts = 2;

      download.net = restartp->net;
      download.filenamep = restartp->filenamep;

      if ( verbose ) printf ( "\r\nResetting Card ..." );

      (void)config_assoc_net(restartp->net,0) ;	/* CLear any NFAS-style associations */

      while (attempts > 0)
         {
         result = call_sfmw ( restartp->net,
                              SFMWMODE_RESTART,
                              restartp->config_stringp ); /* reset the card */

         if ( result == 0 )
            {
            result = call_download_fmw ( &download );  /* transfer the file */

            if (result == 0)
                {
                break;
                }
            }
         else
            {
            break;
            }
         --attempts;
         }

      return ( result );
      }
   }
/*---------------------------------------*/



/*------------ call_nports --------------*/
/* return the number of network ports    */
/*                                       */
ACUDLL int ACU_WINAPI call_nports ( void )
   {
   ACU_INT  result;

   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      result = tnets;                   /* get number of network ports */
      }

   return ( result );
   }
/*---------------------------------------*/


/*------------ call_version -------------*/
/* return the version number             */
/*                                       */
ACUDLL int ACU_WINAPI call_version ( int portnum )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;


   unet = (ACU_INT) portnum;

   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );      /* get the card number */

      if ( card >= 0 )                  /* check for error */
         {
         result = clcard[card].version;
         }
      else
         {
         result = card;                 /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   return ( result );
   }
/*---------------------------------------*/


/*---------- call_port_2_swdrvr ---------*/
/* return switch driver number for user  */
/* network port number                   */
/*                                       */
ACUDLL int ACU_WINAPI call_port_2_swdrvr ( int portnum )
   {
   ACU_INT  result;


   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      result = unet_2_switch( (ACU_INT) portnum ); /* get the card number */
      }

   return ( result );
   }
/*---------------------------------------*/


/*--------- call_port_2_stream ----------*/
/* return switch driver number for user  */
/* network port number                   */
/*                                       */
ACUDLL int ACU_WINAPI call_port_2_stream ( int portnum )
   {
   ACU_INT  result;


   result = clopendev ( );               /* open the device */

   if ( result == 0 )
      {
      result = unet_2_stream ( (ACU_INT) portnum ); /* get the card number */
      }

   return ( result );
   }
/*---------------------------------------*/


/*--------- call_handle_2_port ----------*/
/* return network port number for a given*/
/* call handle                           */
/*                                       */

ACUDLL int ACU_WINAPI call_handle_2_port ( int handle )
   {
   int result ;

   result = clopendev ( );                    /* open the device */

   if ( result == 0 )
      {
      DC       dc;
      HANDLE_2_PORT_XPARMS h2p ;

      handle_decomp ( handle, &dc );

      if ( dc.result >= 0 )                        /* check for error */
         {
         if (dc.card >= 0 && clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            result = handle2net( (ACU_INT) handle);
            }
         else
            {
            h2p.handle = dc.handle ;

            /* The following IOCTL may fail either if the driver is unable to
               perform the conversion on the supplied netwok, in which case the
               simple library translation will suffice */

            result = clioctl ( CALL_HANDLE_2_PORT,
                              (IOCTLU *) &h2p,
                               dc.card, -1,
                               sizeof (HANDLE_2_PORT_XPARMS));
            if (result >=0)
               result = h2p.port ; /* Driver has done the conversion */
            else
               result = handle2net((ACU_INT)handle); /* Old-style library conversion */
            }
         }
      }
   return (result) ;
   }
/*---------------------------------------*/


/*--------- call_handle_2_chan ----------*/
/* return channel number for a given     */
/* call handle                           */
/*                                       */
ACUDLL int ACU_WINAPI call_handle_2_chan ( int handle )
   {
   return ( handle2ch ( (ACU_INT) handle ));
   }
/*---------------------------------------*/


/*--------- call_handle_2_io ------------*/
/* return channel number for a given     */
/* call handle                           */
/*                                       */
ACUDLL int ACU_WINAPI call_handle_2_io ( int handle )
   {
   return ( handle2io ( (ACU_INT) handle ));
   }
/*---------------------------------------*/


/*----------- call_is_download ----------*/
/* check if downloadable port            */
/*                                       */
ACUDLL int ACU_WINAPI call_is_download ( int portnum )
   {
   int  result;
   struct siginfo_xparms info[MAXPORT];


   result = call_signal_info ( &info[0] );  /* get the signalling information */


   if ( result == 0 )
      {
      if ( portnum < info[0].nnets )        /* check if port number valid */
         {
         if ( info[portnum].sigsys[0] == '?' )  /* check for downloadable */
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
         result = ERR_NET;              /* illegal port number */
         }
      }

   return ( result );
   }
/*---------------------------------------*/


/*----------- call_download_dsp ---------*/
/* load the dsp firmware onto the card   */
/*                                       */
ACUDLL int ACU_WINAPI call_download_dsp ( struct download_xparms * download_xparmsp )
   {
   return ( download_file ( download_xparmsp ));
   }
/*---------------------------------------*/


/*----------- call_download_fmw ---------*/
/* load the firmware onto the card       */
/*                                       */
ACUDLL int ACU_WINAPI call_download_fmw ( struct download_xparms * download_xparmsp )
   {
   int  error;
   struct sysinfo_xparms    sysinfo;
   char * board_ip_address;

   /* call_set_sys_info sets up the card_info structure for this board. It is required */
   /* to be called before call_system_info which reads the card_info structure */
   sysinfo.net = download_xparmsp->net;
   error = call_system_info ( &sysinfo );


   if ( error >= 0 )
      {
      switch (sysinfo.cardtype)
         {
         case C_REV4:
         case C_REV5:
         case C_BR4:
         case C_BR8:
            error = call_is_download ( download_xparmsp->net );     /* check if port downloadable */

            if ( error >= 0 )                                       /* check for error */
               {
               if ( error == TRUE )                                 /* check if downloadable */
                  {
                  error = download_file ( download_xparmsp );       /* transfer the file */

                  if ( error == 0 )                                 /* start the driver  */
                     {
                     error = call_sfmw ( download_xparmsp->net, 0, (char *) 0 ); /* start the firmware */
                     }
                  }
               }
         break;
         case C_PM4:
            error = call_is_download ( download_xparmsp->net );     /* check if port downloadable */

            error = TRUE;

            if ( error >= 0 )                                       /* check for error */
               {
               if ( error == TRUE )                                 /* check if downloadable */
                  {
                  error = download_pm4_file ( download_xparmsp );   /* transfer the file */

                  if ( error == 0 )                                 /* start the driver  */
                     {
                     error = call_sfmw ( download_xparmsp->net, 0, (char *) 0 ); /* start the firmware */
                     }
                  }
               }
         break;
         case C_VOIP:
	   error = call_sfmw ( download_xparmsp->net, SFMWMODE_ZAPDNLD, download_xparmsp->filenamep);
	   board_ip_address = clcard[unet_2_card (download_xparmsp->net)].board_ip_address;
	   return (error);
         break;
         default:
            error = ERR_DNLD_NOCARD;
         break;
         }


        if (error == 0)
           error = config_assoc_net(download_xparmsp->net,1) ;	/* Add any NFAS associations... */
      }

   return ( error );
   }
/*---------------------------------------*/



/*----------- call_download_brdsp -------*/
/* load the firmware onto the card       */
/*                                       */
ACUDLL int call_download_brdsp ( struct download_xparms * download_xparmsp )
   {
   int       readh;
   int       error;
   int       size;
   ACU_INT   card;
   ACU_INT   unet;
   struct    v5_pblock_xparms *pblock_xparms;


   readh = clfileopen ( download_xparmsp->filenamep );

   if ( readh <= 0 )
      {
      error = ERR_DNLD_FILE;        /* cannot find the file */
      }
   else
      {
      pblock_xparms = (V5_PBLOCK_XPARMS *) calloc ( sizeof(V5_PBLOCK_XPARMS), sizeof (char) ); /* transfer buffer size */

      if ( pblock_xparms == (V5_PBLOCK_XPARMS *) 0 )      /* got the memory ? */
         {
         error = ERR_DNLD_MEM;                            /* no memory available */
         }
      else
         {
         size = clfileread ( readh, pblock_xparms->datap, 0x3c );        /* read header */

         if (size != 0x3c)
            {
            error = ERR_DNLD_TYPE;
            }
         else
            {
            size = clfileread ( readh, pblock_xparms->datap, XFERSIZE );        /* start reading blocks */

            if ((size > 0) && (size <= XFERSIZE))
               {
               /* ok.. so we have a firmware file */

               unet = download_xparmsp->net;       /* save user net */

               error = clopendev ( );              /* open the device */

               if ( error == 0 )
                  {
                  card = unet_2_card ( unet );      /* get the card number */

                  if ( card < 0 )                  /* check for error */
                     {
                     error = card;                /* return error code */
                     }
                  else
                     {
                     pblock_xparms->net = unet_2_net ( unet ); /* and the network port */

                     if ( pblock_xparms->net >= 0 )    /* check for error */
                        {
                        pblock_xparms->len = (ACU_INT) 0;

                        error = clpblock_ioctl( CALL_BRDSPBLOCK,
                                               (V5_PBLOCK_IOCTLU *) pblock_xparms,
                                                card,
                                                sizeof(V5_PBLOCK_XPARMS) );

                        while ((error == 0) && ((size != 0) || (pblock_xparms->len == 0)))
                           {
                           pblock_xparms->len = size;

                           error = clpblock_ioctl( CALL_BRDSPBLOCK,
                                                   (V5_PBLOCK_IOCTLU *) pblock_xparms,
                                                   card,
                                                   sizeof(V5_PBLOCK_XPARMS) );

                           if ((size == XFERSIZE) && (error == 0))
                              {
                              size = clfileread ( readh, pblock_xparms->datap, XFERSIZE );
                              }
                           else
                              {
                              size = 0;
                              }
                           }

                        if (error == 0)
                           {
                           pblock_xparms->len = -1;

                           error = clpblock_ioctl ( CALL_BRDSPBLOCK,
                                                    (V5_PBLOCK_IOCTLU *) pblock_xparms,
                                                     card,
                                                     sizeof(V5_PBLOCK_XPARMS) );
                           }
                        }
                     else
                        {
                        error = pblock_xparms->net;
                        }
                     }
                  }
               else
                  {
                  error = ERR_CFAIL;
                  }
               }
            else
               {
               error = ERR_DNLD_TYPE;     /* not a firmware file */
               }
            }
         free ( pblock_xparms );          /* free the memory */
         }
      clfileclose ( readh );              /* close the file */
      }
      return ( error );
   }
/*---------------------------------------*/


/*------------- download_file -----------*/
/* load the firmware onto the card       */
/* for Rev4/5, BR4, BR8 cards.           */
ACUDLL int download_file ( struct download_xparms * download_xparmsp )
   {
   int  readh;
   int  result;
   int  error = 0;
   int  size;
   struct v5_pblock_xparms *pblock_xparms;
   struct tcmd_xparms       tcmd_xparms;
   struct sysinfo_xparms    sysinfo;
   int  off;
   int  seg;


   sysinfo.net = download_xparmsp->net;
   result = call_system_info ( &sysinfo );

   if (result != 0 )
      return ( result );

   readh = clfileopen ( download_xparmsp->filenamep );

   if ( readh >= 0 )
      {
      pblock_xparms = (V5_PBLOCK_XPARMS *) calloc ( sizeof (V5_PBLOCK_XPARMS), sizeof(char) ); /* transfer buffer size */

      if ( pblock_xparms != (V5_PBLOCK_XPARMS *) 0 )                        /* go the memory ? */
         {
         size = clfileread ( readh, pblock_xparms->datap, XFERSIZE );        /* start reading blocks */

         if ((size > 0 ) &&                             /* check signature      */
             (* pblock_xparms->datap ==  (char ) 0xeb  ) &&              /* check for jmps short */
             (strncmp (pblock_xparms->datap + 3, "ACULAB", 6 ) == 0))  /* check if good file   */
            {
            /* ok.. so we have a firmware file */

            off = 0;
            seg = 0x4000;            /* start segment for code */

            if ( verbose )  printf ("\r\nDownloading File ");

            while ( 1 )              /* do all blocks */
               {
               if ( verbose )  printf (".");

               if ( size != 0 )
                  {
                  pblock_xparms->net   = download_xparmsp->net;  /* port number  */
                  pblock_xparms->len   = (ACU_INT) size;         /* block size   */

                  error = call_pblock ( pblock_xparms );

                  if ( error == 0 )
                     {
                     tcmd_xparms.net = download_xparmsp->net;

                     switch (sysinfo.cardtype)
                        {
                        case C_BR8:
                           sprintf( (char*)tcmd_xparms.str,
                                   "C E000:2000,%X:%X,%X\r"
                                   ,seg, off, size);         /* make command string  */
                        break;

                        case C_REV4:
                        case C_REV5:
                        case C_BR4:
                        default:
                           sprintf( (char*)tcmd_xparms.str,
                                    "C 8000:2000,%X:%X,%X\r"
                                    ,seg, off, size);        /* make command string  */
                        break;
                        }

                     error = call_tcmd ( &tcmd_xparms );

                     seg += XFERSIZE / 16;              /* next segment */
                     }

                  if ( size == XFERSIZE && error == 0 )
                     {
                     size = clfileread ( readh, pblock_xparms->datap, XFERSIZE );
                     }
                  else
                     {
                     break;
                     }
                  }
               else
                  {
                  break;
                  }
               }

            if ( error == 0 )                            /* check if downloaded ok */
               {
               if ( verbose ) printf ("\r\nStarting Firmware ...");

               tcmd_xparms.net = download_xparmsp->net;

               strcpy ( (char*)tcmd_xparms.str, "G 4000:0\r" );  /* make start command */

               error = call_tcmd ( &tcmd_xparms );        /* send the start command */
               }
            }
         else
            {
            error = ERR_DNLD_TYPE;  /* not a firmware file */
            }

         free ( pblock_xparms );            /* free the memory */

         }
      else
         {
         error = ERR_DNLD_MEM;      /* no memory available */
         }
      clfileclose ( readh );              /* close the file */

      }
   else
      {
      error = ERR_DNLD_FILE;        /* cannot find the file */
      }

   return ( error );
   }
/*---------------------------------------*/





/*--------- download_pm4_file -----------*/
/* load the firmware onto the card       */
/* for PM4 module                        */

ACUDLL int download_pm4_file ( struct download_xparms * download_xparmsp )
   {
   int  readh;
   int  error;
   int  result;
   int  size;
   struct v5_pblock_xparms *pblock_xparms;
   struct tcmd_xparms       tcmd_xparms;
   struct sysinfo_xparms    sysinfo;
   int  off;
   int  seg;

   sysinfo.net = download_xparmsp->net;
   result = call_system_info ( &sysinfo );

   if (result != 0 )
      return ( result );

   readh = clfileopen ( download_xparmsp->filenamep );

   if ( readh >= 0 )
      {
      pblock_xparms = (V5_PBLOCK_XPARMS *) calloc ( sizeof (V5_PBLOCK_XPARMS), sizeof(char) ); /* transfer buffer size */

      if ( pblock_xparms != (V5_PBLOCK_XPARMS *) 0 )                        /* go the memory ? */
         {
         size = clfileread ( readh, pblock_xparms->datap, PM4ZAPSIZE );     /* start reading blocks */

         if (size > 0 )
            {
            if ( verbose )  printf ("\r\nDownloading ZAP ");

            pblock_xparms->net   = download_xparmsp->net;                   /* port number  */
            pblock_xparms->len   = (ACU_INT) size;                          /* block size   */

            error = call_pblock ( pblock_xparms );

            if (error !=0 )
                return ( error );

            result = call_sfmw ( download_xparmsp->net,
                                 SFMWMODE_ZAPDNLD,
                                 (char *) 0 );   /* start the ZAP the card */
            if (result != 0)
                return ( result );

            if ( verbose )  printf (".");

            size = clfileread ( readh, pblock_xparms->datap, XFERSIZE );        /* start reading blocks */

            if ((size > 0 ) &&                             /* check signature      */
                (* pblock_xparms->datap ==  (char ) 0xeb  ) &&              /* check for jmps short */
                (strncmp (pblock_xparms->datap + 3, "ACULAB", 6 ) == 0))  /* check if good file   */
               {
               /* ok.. so we have a firmware file */

               off = 0;
               seg = 0x8000;            /* start segment for code */

               if ( verbose )  printf ("\r\nDownloading firmware ");

               while ( 1 )              /* do all blocks */
                  {
                  if ( verbose )  printf (".");

                  if ( size != 0 )
                     {
                     pblock_xparms->net   = download_xparmsp->net;  /* port number  */
                     pblock_xparms->len   = (ACU_INT) size;         /* block size   */

                     error = call_pblock ( pblock_xparms );

                     if ( error == 0 )
                        {
                        tcmd_xparms.net = download_xparmsp->net;

                        sprintf( (char*)tcmd_xparms.str,
                                       "C FC00:0000,%X:%X,%X\r"
                                       ,seg, off, size);         /* make command string  */

                        error = call_tcmd ( &tcmd_xparms );

                        seg += XFERSIZE / 16;              /* next segment */
                        }

                     if ( size == XFERSIZE && error == 0 )
                        {
                        size = clfileread ( readh, pblock_xparms->datap, XFERSIZE );
                        }
                     else
                        {
                        break;
                        }
                     }
                  else
                     {
                     break;
                     }
                  }
               }

            if ( error == 0 )                            /* check if downloaded ok */
               {
               if ( verbose ) printf ("\r\nStarting Firmware ...");

               tcmd_xparms.net = download_xparmsp->net;

               strcpy ( (char*)tcmd_xparms.str, "G 8000:0\r" );  /* make start command */

               error = call_tcmd ( &tcmd_xparms );        /* send the start command */
               }
            }
         else
            {
            error = ERR_DNLD_TYPE;  /* not a firmware file */
            }

         free ( pblock_xparms );            /* free the memory */

         }
      else
         {
         error = ERR_DNLD_MEM;      /* no memory available */
         }
      clfileclose ( readh );              /* close the file */

      }
   else
      {
      error = ERR_DNLD_FILE;        /* cannot find the file */
      }

   return ( error );
   }
/*---------------------------------------*/





/*------------ call_trace ---------------*/
/* turn on driver trace                  */
/*                                       */
ACUDLL int ACU_WINAPI call_trace ( int portnum, int traceflag )
   {
   struct trace_xparms tracep;
   ACU_INT             result;

   memset (&tracep,0,sizeof(struct trace_xparms));

   tracep.handle    = 0;
   tracep.card      = 0;
   tracep.unet      = (ACU_INT) portnum;
   tracep.traceflag = traceflag;

   result = xcall_trace (&tracep);

   return ( result );
   }
/*---------------------------------------*/


/*------------ call_trace ---------------*/
/* turn on driver trace                  */
/*                                       */

ACUDLL int ACU_WINAPI xcall_trace ( struct trace_xparms *tracep )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT unet;

   unet=tracep->unet;

   result = clopendev ( );                 /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( tracep->unet );  /* get the card number */

      if ( card >= 0 )                     /* check for error */
         {
         result = clioctl ( CALL_TRACE,
                            (IOCTLU *) tracep,
                            card, unet,
                            sizeof (TRACE_XPARMS));
         }
      else
         {
         result = card;                    /* return error code */
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

   return ( result );
   }
/*---------------------------------------*/




/*---------------------------------------*/
/* miscellaneous functions to provide    */
/* support for the switch                */
/*---------------------------------------*/

/*---------- handle_decomp --------------*/
/* get an event if one present           */
/*                                       */
ACUDLL void handle_decomp ( ACU_INT handle, DC * dcp )
   {
   ACU_INT  unet, net;


   dcp->result = 0;                     /* pre-empt no error */

   unet = handle2net ( handle );        /* recover the net */

   dcp->card = unet_2_card ( unet );    /* get the card number */

   if ( dcp->card >= 0 )                /* check for error */
      {
      net = unet_2_net ( unet );        /* and the network port */

      if ( net >= 0 )                   /* check for error */
         {
         dcp->handle = patch_handle ( handle, net );
         }
      else
         {
         dcp->result = net;             /* return error code */
         }
      }
   else
      {
      dcp->result = dcp->card;          /* return error code */
      }
   }
/*---------------------------------------*/


/*------------- unet_2_card -------------*/
/* returns the card number for the       */
/* network port provided                 */
/*                                       */
ACUDLL ACU_INT  unet_2_card ( ACU_INT  unet )
   {
   ACU_INT  result = ERR_NET;
   ACU_INT  i;


   result = clopendev ( );              /* open the call drivers */

   if ( result == 0 )
      {
      if ( unet < tnets )               /* check valid number */
         {
         i = 0;

         while ( unet >= clcard[i].nnets )
            {
            unet -= clcard[i].nnets;    /* subtract number of nets on card */
            i++;
            }

         result = i;                    /* the index is the card number */
         }
      }

   return ( (ACU_INT) result );

   }
/*---------------------------------------*/



/*------------- unet_2_switch -----------*/
/* returns the switch number for the     */
/* network port provided                 */
/*                                       */
ACUDLL ACU_INT unet_2_switch ( ACU_INT unet )
   {
   ACU_INT  result = ERR_NET;
   ACU_INT  i;


   result = clopendev ( );              /* open the call drivers */

   if ( result == 0 )
      {
      if ( unet < tnets )               /* check valid number */
         {
         i = unet_2_card(unet);

         result = clcard[i].v1bmi_card_num;
         result += switchbase;
         }
      }

   return ( (ACU_INT) result );
   }


/*------------- call_set_net0_swnum ------------------------*/
/* sets an offset for port number so MC3's can be first     */
/*                                                          */
/*                                                          */

ACUDLL void call_set_net0_swnum ( int initialLoneSwitchCount )
{
    switchbase = initialLoneSwitchCount;
}
/*---------------------------------------*/

/*------------- net_2_unet --------------*/
/* returns the unet number when card and */
/* net number are provided               */
/*                                       */
ACUDLL ACU_INT  net_2_unet ( ACU_INT  card, ACU_INT net )
   {

   ACU_INT  result , i;

   result =0;
   
   for (i=0;i<card;i++)
      {
      result +=clcard[i].nnets;
      }

   result += net;

   return ( result );
   }
/*---------------------------------------*/


/*------------- unet_2_net --------------*/
/* returns the network port for (0,1,2,3)*/
/* the network port provided             */
/*                                       */
ACUDLL ACU_INT unet_2_net ( ACU_INT unet )
   {
   ACU_INT  result;
   ACU_INT  i;


   result = clopendev ( );              /* open the call drivers */

   if ( result == 0 )
      {
      result = ERR_NET;                 /* invalid network number */

      if ( unet < tnets )               /* check valid number */
         {
         i = 0;

         while ( unet >= clcard[i].nnets )
            {
            unet -= clcard[i].nnets;    /* subtract number of nets on card */
            i++;
            }

         result = unet;                 /* the remainder is the network port */
         }
      }

   return ( result );
   }
/*---------------------------------------*/

/*------------- unet_2_stream -----------*/
/* returns the stream number for the     */
/* network port provided                 */
/*                                       */
ACUDLL ACU_INT unet_2_stream ( ACU_INT unet )
   {
   ACU_INT  result;
   ACU_INT  net;
   ACU_INT  card;
   struct sysinfo_xparms    sysinfo;

   card = unet_2_card ( unet );         /* get the card number */

   net = unet_2_net ( unet );           /* get the network port */

   sysinfo.net = unet;                  /* port number */
   result = call_system_info ( &sysinfo );

   if (result == 0)
      {
      if ( net >= 0 )                      /* check for error */
         {
         switch (sysinfo.cardtype)
            {
            case C_BR4:
            case C_BR8:
            case C_PM4:
                net = 32 + net;                /* virtual stream number */
            break;
            case C_REV4:
            case C_REV5:
                net = 16 + (net * 2);          /* make into network stream */
            break;
            case C_VOIP:
                net = 48;   /* virtual stream number - same for VoIP ports */
            break;
            }
         }
      }
   else
      {
      return ( result );
      }

   return ( net );
   }
/*---------------------------------------*/


/*------------- card_2_voipcard --------------*/
/* returns the voip card number for the       */
/* card number provided                       */
/*                                            */
ACUDLL ACU_INT  card_2_voipcard ( ACU_INT  card )
   {

   ACU_INT  result ;

   result = card-first_voip_card+1;

   return ( result );
   }
/*---------------------------------------*/


/*------------- card_2_net --------------*/
/* returns the first net number for the  */
/* card number provided                  */
/*                                       */
ACUDLL ACU_INT  card_2_net ( ACU_INT  card )
   {

   ACU_INT  result , i;

   result =0;
   for (i=0;i<card;i++)
   {
      result +=clcard[i].nnets;
   }

   return ( result );
   }
/*---------------------------------------*/





/*---------- handle functions -----------*/
/* these may change in different versions*/
/* of the device driver, but may be      */
/* called if the information is required */
/*---------------------------------------*/

/*------------- patch_handle ------------*/
/* convert handle to a network number    */
/*                                       */
ACUDLL ACU_INT patch_handle ( ACU_INT handle, ACU_INT net )
   {
   return ( (handle & ~0x3f00) | (net << 8 ) );
   }
/*---------------------------------------*/


/*------------- handle2net --------------*/
/* convert handle to a network number    */
/*                                       */
ACUDLL ACU_INT handle2net ( ACU_INT handle )
   {
   handle &= ~(INCH + OUCH);

   return ( ((handle >> 8) & 0x00ff) );
   }
/*---------------------------------------*/

/*------------- handle2ch ---------------*/
/* convert handle to a channel number    */
/*                                       */
ACUDLL ACU_INT handle2ch ( ACU_INT handle )
   {
   handle &= ~(INCH + OUCH);

   return ( ((handle & 0x00ff) - 1) );
   }
/*---------------------------------------*/

/*------------- handle2io ---------------*/
/* convert handle to a in/out direction  */
/*                                       */
/*   15..........8   7............0      */
/*    Network Port /  Timeslot + 1       */
/*                                       */
ACUDLL ACU_INT handle2io ( ACU_INT handle )
   {
   switch ( handle & (INCH + OUCH))
      {
      case INCH:
         return 0;

      case OUCH:
         return 1;

      case ENQH:
         return 2;
      }
   return 0;
   }
/*---------------------------------------*/

/*------------- nch2handle --------------*/
/* convert network number and timeslot   */
/* to a handle number                    */
/*                                       */
/* the handle is implemented as:         */
/*                                       */
/*   15..........8   7............0      */
/*    Network Port /  Timeslot + 1       */
/*                                       */
ACUDLL ACU_INT nch2handle ( ACU_INT net, ACU_INT ch )
   {
   return ( (((net << 8 ) & 0xff00) + ((ch + 1) & 0x00ff)));
   }
/*---------------------------------------*/


/*------------ update_ss_type -----------*/
/* update the signalling system types    */
/*                                       */

ACUDLL int update_ss_type ( ACU_INT portnum )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  net;
   SFMW_XPARMS sfmw;



   card = unet_2_card ( portnum );      /* get the card number */

   net = unet_2_net ( portnum );        /* and the network port */

   memset(&sfmw, 0, sizeof(sfmw));
   
   sfmw.net        = SFMWMODE_SSTYPE | net;
   sfmw.confstr[0] = '\0';
   sfmw.regapp     = 0;
   sfmw.ss_vers    = 0;

   result = clioctl ( CALL_SFMW,
                      (IOCTLU *) &sfmw,
                      card, portnum, 
                      sizeof (SFMW_XPARMS) );
   
   if ( result == 0 )
      {
      clcard[card].types[net] = sfmw.ss_type;
      clcard[card].lines[net] = sfmw.ss_line;
      clcard[card].version    = sfmw.ss_vers;
      }


   return ( result );
   }
/*---------------------------------------*/

/*--------- isallowed_string ------------*/
/* check if the string is allowed        */
/*                                       */
static char isallowed_string ( char * strp )
   {
   char  ch;


   while ((ch = * strp) != '\0' )
      {
      switch ( ch & 0xff )
         {
         case '0':
         case '1':
         case '2':
         case '3':
         case '4':
         case '5':
         case '6':
         case '7':
         case '8':
         case '9':
         case '!':
         case '#':
         case '*':
         case ',':
         case 'a':      /* digit 10 for SS7 */
         case 'A':
         case 'b':      /* digit 11 for SS7 */
         case 'B':
         case 'd':      /* digit 13 for SS7 */
         case 'D':
         case 'e':      /* digit 14 for SS7 */
         case 'E':
         case 'g':      /* ss5 for code 11      */
         case 'G':
         case 'h':      /* ss5 for code 12      */
         case 'H':
         case 'c':      /* language code for R2 */
         case 'C':
         case 'f':      /* end of send for R2   */
         case 'F':
             break;

         default:
            return 0;
         }

      strp++;
      }
   return 1;
   }
/*---------------------------------------*/

/*------------- clopendev ---------------*/
/* open up all device drivers in the     */
/* system                                */
/*                                       */
ACUDLL ACU_INT clopendev ( void )
   {
   ACU_INT  result;

   if ( clopened == FALSE )
      {
      result = opendevdrv( );
      }
   else
      {
      result = 0;                       /* already open */
      }

   return ( result );

   }
/*---------------------------------------*/


/*---------------------------------------*/
/* used by clopendev to open up drivers  */
/* in the system                         */

ACUDLL ACU_INT opendevdrv( void )
   {
   ACU_INT  result = 0;
   int  i;
   int  addrnum;
   char cext;
   SFMW_XPARMS sfmw;
   struct siginfo_xparms   lsig[MAXPPC];
   struct siginfo_xparms * lsigp;
   char   cldevname[16];

   /* VoIP specific */
   int                   voip_card_number;    /* numbers voip cards 1,2,3 etc. */

   clspecial ( );                                /* do special case */

   /* now try to open up all known device drivers */
   /* they will configure themselves sequentially */
   /* initialise the driver control structures    */

   tnets = 0;                                    /* initialise network counter */
   lsigp = &lsig[0];


   for ( ncards = 0; ncards < NCARDS; ncards++ )
      {
      clcard[ncards].nnets = 0;                  /* clear number of networks */

      strcpy ( cldevname, cldev ( ));

      addrnum = (int)strlen ( cldevname ) - 1;        /* position to address digit */

      if ( ncards <= 9 )                         /* sort out driver extension */
         {
         cext = (char) (ncards + '0');
         }
      else
         {
         cext = (char) (ncards + 'A' - 10 );
         }

      cldevname[addrnum] = cext;                 /* set device name */

      clcard[ncards].clh = clopen ( cldevname );



      if ( clcard[ncards].clh <  0 )             /* check for error */
         {
         break;
         }
      }



   if ( ncards != 0 )
      {
      /* opened at least one card */
      /* now see how many nets are in use */

      for ( i = 0; i < ncards; i++ )
         {
         sfmw.net     = SFMWMODE_REGISTER;
         sfmw.confstr[0] = '\0';
         sfmw.regapp  = REGISTER_XAPI;      /* register extended API */
         sfmw.ss_vers = 0;



         result = clioctl ( CALL_SFMW,
                            (IOCTLU *) &sfmw,
                            i, -1,
                            sizeof (SFMW_XPARMS ));


         result = clioctl ( CALL_SIGNAL_INFO,
                            (IOCTLU *) lsigp,
                            i, -1,
                            sizeof (SIGINFO_XPARMS) * MAXPPC );

         if ( result == 0 )
         {
         clcard[i].v1bmi_card_num = i; /* store driver and switch number */
         clcard[i].nnets = lsigp->nnets;   /* register number of networks */
         tnets += lsigp->nnets;            /* count total networks        */
         }
         else
         {
            break;
         }
         }


      if ( result == 0 )
         {
         /* the opened flag must be set here    */
         /* otherwise update_ss_type will cause */
         /* clopendev to recurse                */

         clopened = TRUE;                      /* library opened */

         /* now fix up all of the signalling types */

         for ( i = 0; i < tnets && result == 0; i++ )
            {
            result = update_ss_type ( i );
            }

         result = 0;                 /* all ok */
         }
      else
         {
         result = ERR_CFAIL;
         }
      }
   else
      {
      call_driver_installed = FALSE;
      result = ERR_CFAIL;
      }

#ifdef ACUC_CLONED
   /* Now request emid from driver zero and notify it to all other devices... */
   if (result == 0 )
      {
      ACUC_EVENT_IF_XPARMS event_if_xparms ;

      memset (&event_if_xparms,0,sizeof (event_if_xparms));
      event_if_xparms.cmd = ACUC_EVENT_ALLOC_EMID ;

      result = clioctl ( CALL_EVENT_IF,
                         (IOCTLU *) &event_if_xparms,
                         0,
                         sizeof (event_if_xparms));


      if (result == 0)
         {
         /* Notify all other cards of our newly acquired emid */
         app_emid = event_if_xparms.emid ;

         event_if_xparms.cmd = ACUC_EVENT_SET_EMID ;

         for ( i = 0; i < ncards; i++ )
            {
            event_if_xparms.cnum = i ;

            result = clioctl ( CALL_EVENT_IF,
                               (IOCTLU *) &event_if_xparms,
                               0,
                               sizeof (event_if_xparms));
            if (result != 0)
                break ;
            }
         }
      }
#endif

   /*
    * create pipe administration thread - used to monitor communication with service
    * if we fail to launch the pipe administration thread then we assume voip is not
    * present in the system
    */

  if ( result == 0 )
    {
     if(0 == create_pipe_admin_thread())
     {
      int swdrvr,isvoip;
      int card_number = ncards;
      int voip_protocol = 0;
      int num_of_voip_ports_per_ip_port = 0;
      const ACU_INT* usable_voip_protocol_indexes = 0;

      /* 
       * find out how many voip protocols are running and get an array of indexes which may be 
       * used as the protocol argument to srvioctl
       */
       usable_voip_protocol_indexes = get_voip_protocol_index_array(&num_of_voip_ports_per_ip_port);

      voip_card_number = 1;
      first_voip_card = -1;
        /*  We have found all the non-voip cards. Now look for all the voip cards */
        /*  by iteration through all the switch drivers looking for those that belong*/
        /*  to a VOIP card */
      for ( swdrvr = 0 ; swdrvr < NCARDS; swdrvr++)
      {
        isvoip = is_voip_card( swdrvr );
        if ( isvoip <0 ) break;
        if ( isvoip )
        {
               /* set up information relating to this VoIP board */ 
               if (first_voip_card == -1)
                 first_voip_card = card_number;

               clcard[card_number].v1bmi_card_num = swdrvr;  /* store driver and switch number */
               clcard[card_number].voipservice = ACU_VOIP_ACTIVE;

               /* now setup information relating to each VoIP protocol that is running */
               for(voip_protocol = 0; voip_protocol < num_of_voip_ports_per_ip_port; voip_protocol++)
               {
                   /* bump up network port count for VOIP for each iteration of this loop */ 
                   clcard[card_number].nnets += 1; 
		   memset(&sfmw, 0, sizeof(sfmw));

                   /* send SMFW message to stop ERR_DRV_INCOMPAT errors */
                   sfmw.net     = SFMWMODE_REGISTER;
                   sfmw.confstr[0] = '\0';
                   sfmw.regapp  = REGISTER_XAPI;      /* register extended API */
                   sfmw.ss_vers = 0;
                   result = srvioctl ( CALL_SFMW,
                                       (IOCTLU *) &sfmw,
                                       sizeof (SFMW_XPARMS ),
                                       voip_card_number, 
                                       usable_voip_protocol_indexes[voip_protocol]);

                   /* read net info from card */
                   sfmw.net     = SFMWMODE_SSTYPE;             /* ask for signalling type */
                   sfmw.confstr[0] = '\0';
                   result = srvioctl ( CALL_SFMW,
                                       (IOCTLU *) &sfmw,
                                       sizeof (SFMW_XPARMS ),
                                       voip_card_number, 
                                       usable_voip_protocol_indexes[voip_protocol]);

                   clcard[card_number].types[voip_protocol] = sfmw.ss_type;    /* should be S_H323*/
                   clcard[card_number].lines[voip_protocol] = sfmw.ss_line;    /* should be L_PSN */
                   clcard[card_number].version  = sfmw.ss_vers;    /* read from acu_system_version.h */
                   tnets+=1;                       /* increment total number of ports by one */
               }

               card_number++;
               voip_card_number++;
        
        }
      }
      ncards = card_number;
     }
   }
 
   clopened = TRUE;                /* library opened */
   

   return ( result );

   }  /* end of function opendevdrv */


/*------------- call_tsinfo -------------*/
/* Set/get timeslot and d-channel        */
/* configuration                         */
/*                                       */
ACUDLL ACU_INT call_tsinfo ( struct tsinfo_xparms * tsinfop )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;


   unet = tsinfop->net;                      /* save user net    */

   result = clopendev ( );                    /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( unet );            /* get the card number */

      if ( card >= 0 )                        /* check for error */
         {
         tsinfop->net = unet_2_net ( unet ); /* and the network port */

         if ( tsinfop->net >= 0 )            /* check for error */
            {
            result = clioctl ( CALL_TSINFO,
                               (IOCTLU *) tsinfop,
                               card, unet,
                               sizeof (TSINFO_XPARMS ));

            }
         else
            result = ERR_NET ;
         }
      else
         result = ERR_NET ;
      }
      return (result) ;
   }


/* ----------------- call_maintenance_cmd ----------------- */
/*                                                          */
/* Set up the net/unet fields for a maintenance command and */
/* send it to the driver.                                   */
/*                                                          */

static
ACU_INT call_maintenance_cmd(ACUC_MAINTENANCE_XPARMS * maint, int xlate)
   {
   ACU_INT result ;
   ACU_INT card ;
   ACU_INT unet;
   
   result = clopendev ( );                    /* open the device */

   if ( result == 0 )
      {
      if (xlate)
        /* Fix up 'net' to signalling net, unet to user net  or -1 */
        maint->unet = xlate_assoc(&maint->net) ;
      else
        maint->unet = -1 ;

      /* The maintenance cmd is sent to the sig net, rather than unser net... */
      card = unet_2_card ( maint->net );            /* get the card number */
      if ( card >= 0 )                        /* check for error */
         {
         unet = maint->net;   
         maint->net = unet_2_net ( maint->net ); /* and the network port */

         if ( maint->net >= 0 )            /* check for error */
            {
            result = clioctl ( CALL_MAINTENANCE,
                               (IOCTLU *) maint,
                               card, unet,
                               sizeof (ACUC_MAINTENANCE_XPARMS ));
            }
         else
            result = ERR_NET ;
         }
      else
         result = ERR_NET ;

      }
   return (result) ;
   }



/*---------- port_unblocking ------------*/
/* Protocol-unblock a group of timeslots */
/*                                       */
ACUDLL ACU_INT ACU_WINAPI
call_maint_port_unblock ( PORT_BLOCKING_XPARMS * blockingp)
   {
   ACUC_MAINTENANCE_XPARMS maint ;
   
   memset (&maint , 0 , sizeof(maint)) ;

   maint.net = blockingp->net ;
   maint.type = blockingp->type ;
   maint.flags = blockingp->flags ;

   maint.cmd = ACUC_MAINT_PORT_UNBLOCK ;

   switch (call_type(blockingp->net))
      {

      /* Other protocols may use this switch one day... ? */
      case S_ISUP :
      default :
        maint.ts_mask = blockingp->unique_xparms.ts_mask ;
        break ;
      }

   return call_maintenance_cmd(&maint,1) ;
}
/*---------------------------------------*/


/*---------- port_blocking --------------*/
/* Protocol-block a group of timeslots   */
/*                                       */
ACUDLL ACU_INT ACU_WINAPI
call_maint_port_block ( PORT_BLOCKING_XPARMS * blockingp)
   {
   ACUC_MAINTENANCE_XPARMS maint ;

   memset (&maint , 0 , sizeof(maint)) ;
   
   maint.net = blockingp->net ;
   maint.type = blockingp->type ;
   maint.flags = blockingp->flags ;
   maint.cmd = ACUC_MAINT_PORT_BLOCK ;

   switch (call_type(blockingp->net))
      {

      /* Other protocols may use this switch one day... ? */
      case S_ISUP :
      default :
        maint.ts_mask = blockingp->unique_xparms.ts_mask ;
        break ;
      }

   return call_maintenance_cmd(&maint , 1) ;
   }


/*---------- port_reset -----------------*/
/* Protocol-reset a goup of timeslots    */
/*                                       */
ACUDLL ACU_INT ACU_WINAPI
call_maint_port_reset ( PORT_RESET_XPARMS * resetp)
   {
   ACUC_MAINTENANCE_XPARMS maint ;

   memset (&maint , 0 , sizeof(maint)) ;

   maint.net = resetp->net ;
   maint.flags = resetp->flags ;
   maint.cmd = ACUC_MAINT_PORT_RESET ;

   switch (call_type(resetp->net))
      {

      /* Other protocols may use this switch one day... ? */
      case S_ISUP :
      default :
        maint.ts_mask = resetp->unique_xparms.ts_mask ;
        break ;
      }

   return call_maintenance_cmd(&maint , 1) ;
   }


/*---------- ts_blocking ----------------*/
/* Protocol-block a timeslot             */
/*                                       */
ACUDLL ACU_INT ACU_WINAPI
call_maint_ts_block ( TS_BLOCKING_XPARMS * blockingp)
   {
   ACUC_MAINTENANCE_XPARMS maint ;

   memset (&maint , 0 , sizeof(maint)) ;

   maint.net = blockingp->net ;
   maint.ts = blockingp->ts ;
   maint.flags = blockingp->flags ;
   maint.cmd = ACUC_MAINT_TS_BLOCK ;
   return call_maintenance_cmd(&maint , 1) ;
   }


/*---------- ts_blocking ----------------*/
/* Protocol-block a timeslot             */
/*                                       */
ACUDLL ACU_INT ACU_WINAPI
call_maint_ts_unblock ( TS_BLOCKING_XPARMS * blockingp)

   {
   ACUC_MAINTENANCE_XPARMS maint ;

   memset (&maint , 0 , sizeof(maint)) ;

   maint.net = blockingp->net ;
   maint.ts = blockingp->ts ;
   maint.flags = blockingp->flags ;
   maint.cmd = ACUC_MAINT_TS_UNBLOCK ;
   return call_maintenance_cmd(&maint , 1) ;
   }

/*---- Resolve a link from a linkset---*/
/*                                       */
ACUDLL ACU_INT 
ACU_WINAPI call_maint_mtp3_resolve_linkset(struct mtp3_resolve_linkset_xparms * lsp)
   {
   ACU_INT resp = 0 ;
   
   ACUC_MAINTENANCE_XPARMS maint ;

   ACU_UCHAR * p = &maint.unique_xparms.sig_isup.generic[1]  ;  /* [0] is for length */
   
   memset (&maint , 0 , sizeof(maint)) ;

   maint.net = lsp->net ;
   maint.cmd = ACUC_MAINT_FW_GENERIC ;
   
   maint.unique_xparms.sig_isup.action = ACUC_MAINT_GETOPT ;
   maint.unique_xparms.sig_isup.module  = ACUC_MAINT_IF_MTP3 ;
   maint.unique_xparms.sig_isup.option  = ACUC_MAINT_MTP3_RESOLVE_LINKSET ;
   
   /* fill the parameters in generic data area... */
   *p++ = (ACU_UCHAR) (lsp->dpc & 0xff );
   *p++ = (ACU_UCHAR) ((lsp->dpc >> 8) & 0xff) ;
   *p++ = (ACU_UCHAR) ((lsp->dpc >> 16) & 0xff) ;
   *p++ = (ACU_UCHAR) ((lsp->dpc >> 24) & 0xff) ;
   
   *p++ = lsp->sys_flags ;
   
   *p++ = lsp->slc ;
   
   
   maint.unique_xparms.sig_isup.generic[0] = p - maint.unique_xparms.sig_isup.generic ;
   
   
   resp = call_maintenance_cmd(&maint , 1) ;
   
   if (resp ==0)
      {
      /* success - pass return values back to caller... */
      p = &maint.unique_xparms.sig_isup.generic[1]  ;  /* [0] is for length */
      
      lsp->dpc = *p++ ;
      lsp->dpc += *p++ << 8 ;
      lsp->dpc += *p++ << 16 ;
      lsp->dpc += *p++ << 24 ;
      
      lsp->sys_flags = *p++ ;
      
      lsp->slc = *p ;
      }
      
   return resp ;
   }


/*---- Resolve a route from a routeset---*/
/*                                       */
ACUDLL ACU_INT 
ACU_WINAPI call_maint_mtp3_resolve_routeset(struct mtp3_resolve_routeset_xparms * rsp)
   {
   ACU_INT resp = 0 ;
   
   ACUC_MAINTENANCE_XPARMS maint ;
   ACU_UCHAR * p = &maint.unique_xparms.sig_isup.generic[1]  ;  /* [0] is for length */

   memset (&maint , 0 , sizeof(maint)) ;

   maint.net = rsp->net ;
   maint.cmd = ACUC_MAINT_FW_GENERIC ;
   
   maint.unique_xparms.sig_isup.action = ACUC_MAINT_GETOPT ;
   maint.unique_xparms.sig_isup.module  = ACUC_MAINT_IF_MTP3 ;
   maint.unique_xparms.sig_isup.option  = ACUC_MAINT_MTP3_RESOLVE_ROUTESET ;
   
   /* fill the parameters in generic data area... */
   *p++ = (ACU_UCHAR) (rsp->dpc & 0xff );
   *p++ = (ACU_UCHAR) ((rsp->dpc >> 8) & 0xff) ;
   *p++ = (ACU_UCHAR) ((rsp->dpc >> 16) & 0xff) ;
   *p++ = (ACU_UCHAR) ((rsp->dpc >> 24) & 0xff) ;
   
   *p++ = rsp->sys_flags ;
   
   *p++ = (ACU_UCHAR) (rsp->apc & 0xff) ;
   *p++ = (ACU_UCHAR) ((rsp->apc >> 8) & 0xff) ;
   *p++ = (ACU_UCHAR) ((rsp->apc >> 16) & 0xff) ;
   *p++ = (ACU_UCHAR) ((rsp->apc >> 24) & 0xff) ;
   
   
   maint.unique_xparms.sig_isup.generic[0] = p - maint.unique_xparms.sig_isup.generic ;
   
   resp = call_maintenance_cmd(&maint , 1) ;
   
   if (resp ==0)
      {
      /* success - pass return values back to caller... */
      p = &maint.unique_xparms.sig_isup.generic[1]  ;  /* [0] is for length */
      
      rsp->dpc = *p++ ;
      rsp->dpc += *p++ << 8 ;
      rsp->dpc += *p++ << 16 ;
      rsp->dpc += *p++ << 24 ;
      
      rsp->sys_flags = *p++ ;
      
      rsp->apc = *p++ ;
      rsp->apc += *p++ << 8 ;
      rsp->apc += *p++ << 16 ;
      rsp->apc += *p++ << 24 ;
      }
      
   return resp ;
   }
   
/*---- Get status of a signalling point--*/
/*                                       */
ACUDLL ACU_INT 
ACU_WINAPI call_maint_mtp3_get_sp_status(struct mtp3_sp_status_xparms * sp)
   {
   ACU_INT resp = 0 ;
   
   ACUC_MAINTENANCE_XPARMS maint ;
   ACU_UCHAR * p = &maint.unique_xparms.sig_isup.generic[1]  ;  /* [0] is for length */

   memset (&maint , 0 , sizeof(maint)) ;

   maint.net = sp->net ;
   maint.cmd = ACUC_MAINT_FW_GENERIC ;
   
   maint.unique_xparms.sig_isup.action = ACUC_MAINT_GETOPT ;
   maint.unique_xparms.sig_isup.module  = ACUC_MAINT_IF_MTP3 ;
   maint.unique_xparms.sig_isup.option  = ACUC_MAINT_MTP3_SP_STATUS ;
   
   /* fill the parameters in generic data area... */
   *p++ = (ACU_UCHAR) (sp->dpc & 0xff );
   *p++ = (ACU_UCHAR) ((sp->dpc >> 8) & 0xff) ;
   *p++ = (ACU_UCHAR) ((sp->dpc >> 16) & 0xff) ;
   *p++ = (ACU_UCHAR) ((sp->dpc >> 24) & 0xff) ;
   
   maint.unique_xparms.sig_isup.generic[0] = p - maint.unique_xparms.sig_isup.generic ;
   
   resp = call_maintenance_cmd(&maint , 1) ;
   
   if (resp ==0) 
      {
      /* success - pass status back to caller */
      sp->acc_status = maint.unique_xparms.sig_isup.generic[5]  ; 
      sp->con_status  = maint.unique_xparms.sig_isup.generic[6]  ; 
      }
      
   return resp ;
   }
   
   
/*---- Get status of a signalling route--*/
/*                                       */
ACUDLL ACU_INT 
ACU_WINAPI call_maint_mtp3_get_route_status(struct mtp3_route_status_xparms * rp)
   {
   ACU_INT resp = 0 ;
   
   ACUC_MAINTENANCE_XPARMS maint ;
   ACU_UCHAR * p = &maint.unique_xparms.sig_isup.generic[1]  ;  /* [0] is for length */

   memset (&maint , 0 , sizeof(maint)) ;

   maint.net = rp->net ;
   maint.cmd = ACUC_MAINT_FW_GENERIC ;
   
   maint.unique_xparms.sig_isup.action = ACUC_MAINT_GETOPT ;
   maint.unique_xparms.sig_isup.module  = ACUC_MAINT_IF_MTP3 ;
   maint.unique_xparms.sig_isup.option  = ACUC_MAINT_MTP3_ROUTE_STATUS ;
   
   /* fill the parameters in generic data area... */
   *p++ = (ACU_UCHAR) (rp->dpc & 0xff );
   *p++ = (ACU_UCHAR) ((rp->dpc >> 8) & 0xff) ;
   *p++ = (ACU_UCHAR) ((rp->dpc >> 16) & 0xff) ;
   *p++ = (ACU_UCHAR) ((rp->dpc >> 24) & 0xff) ;
   
   *p++ = (ACU_UCHAR) (rp->apc & 0xff );
   *p++ = (ACU_UCHAR) ((rp->apc >> 8) & 0xff) ;
   *p++ = (ACU_UCHAR) ((rp->apc >> 16) & 0xff) ;
   *p++ = (ACU_UCHAR) ((rp->apc >> 24) & 0xff) ;
   
   
   maint.unique_xparms.sig_isup.generic[0] = p - maint.unique_xparms.sig_isup.generic ;
   
   resp = call_maintenance_cmd(&maint , 1) ;
   
   if (resp ==0) 
      {
      /* success - pass status back to caller */
      rp->rt_state  = maint.unique_xparms.sig_isup.generic[9]  ; 
      rp->rt_priority  = maint.unique_xparms.sig_isup.generic[10]  ; 
      }
      
   return resp ;
   }

   

/*---- Get status of a signalling link --*/
/*                                       */
ACUDLL ACU_INT 
ACU_WINAPI call_maint_mtp3_get_link_status(struct mtp3_link_status_xparms * lp)
   {
   int resp = 0 ;
   
   ACUC_MAINTENANCE_XPARMS maint ;
   ACU_UCHAR * p = &maint.unique_xparms.sig_isup.generic[1]  ;  /* [0] is for length */

   memset (&maint , 0 , sizeof(maint)) ;

   maint.net = lp->net ;
   maint.cmd = ACUC_MAINT_FW_GENERIC ;
   
   maint.unique_xparms.sig_isup.action = ACUC_MAINT_GETOPT ;
   maint.unique_xparms.sig_isup.module  = ACUC_MAINT_IF_MTP3 ;
   maint.unique_xparms.sig_isup.option  = ACUC_MAINT_MTP3_LINK_STATUS ;
   
   /* fill the parameters in generic data area... */
   *p++ = (ACU_UCHAR) (lp->dpc & 0xff );
   *p++ = (ACU_UCHAR) ((lp->dpc >> 8) & 0xff) ;
   *p++ = (ACU_UCHAR) ((lp->dpc >> 16) & 0xff) ;
   *p++ = (ACU_UCHAR) ((lp->dpc >> 24) & 0xff) ;
   
   *p++ = lp->slc ;
   
   maint.unique_xparms.sig_isup.generic[0] = p - maint.unique_xparms.sig_isup.generic ;
   
   resp = call_maintenance_cmd(&maint , 1) ;
   
   if (resp ==0) 
      {
      /* success - pass status back to caller */
      lp->lk_flags  = maint.unique_xparms.sig_isup.generic[6]  ; 
      }
      
   return resp ;
   }
   
ACUDLL int ACU_WINAPI call_maint_isup_net_2_dpc(struct isup_net_2_dpc_xparms * net2dpc) 
   {
   int resp = 0 ;
   ACUC_MAINTENANCE_XPARMS maint ;
   maint.unique_xparms.sig_isup.generic[0]  = 0 ; /* No generic data with this one */

   memset (&maint , 0 , sizeof(maint)) ;

   maint.net = net2dpc->net ;
   maint.cmd = ACUC_MAINT_NET_2_DPC  ;
   maint.flags = ACUC_MAINT_TLS ;
   
   resp = (ACU_LONG) call_maintenance_cmd(&maint , 0 ) ;
   
   if (resp >=0) 
      {
        net2dpc->dpc  = maint.unique_xparms.sig_isup.dpc ;
      }
      
   return resp ;
   }


/*--------- call_del_assoc_net  ---------*/
/*  Add an associated (NFAS) network     */
/*  port to a signalling port.           */

static ACU_INT
call_del_assoc_net  ( struct acuc_assoc_net_xparms * assocp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  snet;

   snet = assocp->net;

   result = clopendev ( );                    /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( snet );            /* get the card number */

      if ( card >= 0 )                        /* check for error */
         {
         assocp->net = unet_2_net ( snet ); /* and the network port */

         if ( assocp->net >= 0 )            /* check for error */
            {
            if (result ==0)
               {
               assocp->mode = ACUC_ASSOC_CLEAR ;
               result = clioctl ( CALL_ASSOC_NET,
                               (IOCTLU *) assocp,
                               card, snet,
                               sizeof (ACUC_ASSOC_NET_XPARMS ));

               /* Clear the translation tables maintained in driver 0 */
               if (result == 0)
                  {
                  assocp->net = -1 ;	/* Indicates no snet for this unet */
                  assocp->mode = ACUC_ASSOC_SET ;
                  result = clioctl ( CALL_ASSOC_NET,
                               (IOCTLU *) assocp,
                               0, snet,
                               sizeof (ACUC_ASSOC_NET_XPARMS ));
                  }
               }
            }
         else
            result = ERR_NET ;
         }
      else
         result = ERR_NET ;
      }
   return (result) ;
   }
/*---------------------------------------*/


/*--------- call_add_assoc_net  ---------*/
/*  Add an associated (NFAS) network     */
/*  port to a signalling port.           */

static ACU_INT
call_add_assoc_net  ( struct acuc_assoc_net_xparms * assocp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  snet;

   snet = assocp->net;

   result = clopendev ( );                    /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( snet );            /* get the card number */

      if ( card >= 0 )                        /* check for error */
         {
         /* if unet and net are the same, then set 'local' flag */
         if (assocp->net == assocp->unet)
           assocp->flags |= ACUC_ASSOC_LOCAL ;

         if ( assocp->net >= 0 )            /* check for error */
            {
            if (result ==0)
               {
               assocp->net = unet_2_net ( snet ); /* and the network port */
               assocp->mode = ACUC_ASSOC_CONF ;
               result = clioctl ( CALL_ASSOC_NET,
                                  (IOCTLU *) assocp,
                                  card, snet,
                                  sizeof (ACUC_ASSOC_NET_XPARMS ));

               /* Set the translation tables maintained in driver 0 */
               if (result == 0)
                  {
                  assocp->net = snet ;	/* Configure the new network params */
                  assocp->mode = ACUC_ASSOC_SET ;
                  result = clioctl ( CALL_ASSOC_NET,
                                     (IOCTLU *) assocp,
                                     0, snet,
                                     sizeof (ACUC_ASSOC_NET_XPARMS ));
                  }
               }
            }
         else
            result = ERR_NET ;
         }
      else
         result = ERR_NET ;
      }
   return (result) ;
   }
/*----------------------------------------*/


/*------------- Check_assoc --------------*/
/* Called after restart to see if there   */
/* is an external signalling port that    */
/* needs to be re-configured.             */

static int
config_assoc_net (ACU_INT net, ACU_INT add)
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  bnet;

   struct acuc_assoc_net_xparms assoc_net ;

   memset (&assoc_net,0,sizeof (assoc_net));

   result = clopendev ( );                    /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( net );            /* get the card number */

      if ( card >= 0 )
         {
         assoc_net.net = bnet = unet_2_net ( net ); /* Fixup network port */

         if ( assoc_net.net >= 0 )
            {
            assoc_net.unet = 0 ;                   /* not used, but must be in range -1 - nnets */
            assoc_net.mode = ACUC_ASSOC_QUERY ;

            result = clioctl ( CALL_ASSOC_NET,
                               (IOCTLU *) &assoc_net,
                               card, net,
                               sizeof (ACUC_ASSOC_NET_XPARMS ));


            if  ( (result == 0)  /* Port is up & running & supports NFAS ?*/
                && (assoc_net.net >= 0))  /* External signalling network req'd ?*/
               {
               /* Configure caller's net as a 'user' of its signalling net... */
               assoc_net.unet = net ;
               assoc_net.bnet = bnet ;

               /* The driver needs to know if the two nets share the same
               card in order to check validity */
               if (unet_2_card(assoc_net.net) != card)
                  assoc_net.flags |= ACUC_ASSOC_OFF_CARD ;

               /* Ignore return values - an error probably just means a non NFAS driver */
               if (add)
                  result = call_add_assoc_net(&assoc_net) ;
               else
                  (void)call_del_assoc_net(&assoc_net) ;
               }
            else
               result = 0 ;
            }
         }
      }

   return result ;
   }


/*------------- xlate_assoc --------------*/
/* Used for applicatins with NFAS drivers.*/
/*                                        */
/* Fixes up supplied user net to be the   */
/* corresponding signalling net, and      */
/* returns either the original user net,  */
/* or -1 if the driver is non-NFAS.       */

static ACU_INT
xlate_assoc (ACU_INT * unet)
   {
   ACU_INT assoc_net ;
   ACUC_ASSOC_NET_XPARMS  assoc ;

   assoc.unet = *unet ;
   assoc.mode = ACUC_ASSOC_LOCATE ;

   /* Request driver 0 to translate the user's signalling net to an
   associated signalling net */

   if ( (*unet < tnets) &&
       (clioctl ( CALL_ASSOC_NET,
                 (IOCTLU *) &assoc,
                 0, -1,
                 sizeof (ACUC_ASSOC_NET_XPARMS)) == 0))
     {
      /* Driver supports NFAS-style bearers, and this is an NFAS network, */
      assoc_net = *unet ;	/* Associated net is original user net */
      *unet = assoc.net ;	/*...and user net becomes signalling net */
      }
   else
      assoc_net = -1 ;	/* Indicate non-use of NFAS */

   return (assoc_net) ;
   }


/*---------------------------------------*/
/* call_ncards                           */
/*                                       */
/* Returns the number of cards in        */
/* the system the driver has control of. */
/*                                       */

ACUDLL int call_ncards ( void )
{
   ACU_INT  result;

   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      result = ncards;                  /* get number of network cards */
      }

   return ( result );
}
/*---------------------------------------*/


/*---------------------------------------*/
/* call_expose_fd                        */
/*                                       */
/* Returns the file descriptor for a     */
/* particular card                       */

ACUDLL int call_expose_fd(int card)
{
   ACU_INT r = (ACU_INT) clopendev();

   if (card < ncards)
      {
      return r == 0 ? clcard[card].clh : r;
      }
   else
      {
      return ERR_CFAIL;
      }
}
/*-----------------------------------------*/


/*---------- call_signal_apievent ---------*/
/* Used to signal events to kernel driver  */
/*   -*strictly for Aculab use only*-      */
/*                                         */
ACUDLL ACU_INT call_signal_apievent ( struct signal_apievent_xparms *signal_apieventp )
   {
   ACU_INT  result;
   ACU_INT  card;

   result = clopendev ( );                    /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( 0 );               /* get the card 0 number */

      if ( card >= 0 )                        /* check for error */
         {
         result = clioctl ( CALL_SIGNAL_APIEVENT,
                            (IOCTLU *) signal_apieventp,
                            card, -1,
                            sizeof (SIGNAL_APIEVENT_XPARMS ));
         }
      else
         {
         result = ERR_NET;
         }
      }

      return (result);

   }
/*---------------------------------------*/



/*--------------------------------------------------------------------------------------------------*/
/* this function searchs through the config string and extracts the ip_address, subnet_address and  */
/* gateway_address. It expects that they are preceded by the correct flags, are separated by a space*/
/* but that they are in no specific order                                                           */
/* config parameter options  -A1=  board address1, -A2=  board address2, -B1 subnet mask1,          */
/*                           -B2 subnet mask1, -C default gateway, -D use DHCP 1= yes               */
/*--------------------------------------------------------------------------------------------------*/
int parse_config_str(char * config_string, char * ip_address1, char * ip_address2,
                     char * subnet_address1, char * subnet_address2,
                     char * gateway_address, int *Download_DHCP)
{
   int number_found;
   int found;
   char dhcp_string[16];

   number_found = 0;
   found = FALSE;


   parse_string (config_string,"-A1",ip_address1);
   if (ip_address1[0] != 0 ) number_found++;
   parse_string (config_string,"-A2",ip_address2);
   if (ip_address2[0] != 0 ) number_found++;
   parse_string (config_string,"-B1",subnet_address1);
   if (subnet_address1[0] != 0 ) number_found++;
   parse_string (config_string,"-B2",subnet_address2);
   if (subnet_address2[0] != 0 ) number_found++;
   parse_string (config_string,"-C",gateway_address);
   if (gateway_address[0] != 0 ) number_found++;
   parse_string (config_string,"-D",dhcp_string);
   if (dhcp_string[0] != 0 ) number_found++;
   if (dhcp_string[0] == '1')
      *Download_DHCP=1;
   else
      *Download_DHCP=0;

   return number_found;
}



/* This function searches through a string, config_string, looking for the contents of       */
/* search_string. If search string is found the characters after it up to the first space    */
/* are loaded into found_string. If search_string is not found an empty string is returned   */
/* in found_string                                                                           */
void parse_string (char *config_string, char search_string[4],char *found_string)
{
   char * position;
   char * cp;
   int i;

   found_string[0] = 0;

   position = strstr(config_string,search_string); /* find search string in config_str */
   if ( position != 0)                             /* if found copy string to found string */
      {
      cp=position+strlen(search_string);              /* increment pointer to get past flag   */
      for (i=0;((*cp != ' ')&&(*cp != 0));i++)
         {
         found_string[i] = *cp;
         cp++;
         }
      found_string[i] = 0;                               /* flag end of string */
   }
}

/*---------------------------------------*/
/* call_free_admin_msg                   */
/*                                       */
/* free the memory allocated dynamically */
/* in the admin_msg.                     */

ACUDLL int call_free_admin_msg(voip_admin_msg *adminp)
   {
   if (adminp != NULL)
      {
      if ((adminp->endpoint_alias != NULL) ||
          (adminp->prefixes != NULL))
         {
         free ( adminp->endpoint_alias );
         adminp->endpoint_alias = NULL;
         free ( adminp->prefixes );
         adminp->prefixes = NULL;;
         }
      return 0;
      }
   else
      {
      return ERR_PARM;
      }
   }

/*-------------- call_gk_default_config----------------*/
/* Set Gk address and endpoint identifier for VoIP     */
/*                                                     */
ACUDLL int call_set_default_gk_config ( struct default_ras_config *ras_info )
{
   int i;

   if ( ras_info != NULL )
      {
      default_ras_info.request_admission = ras_info->request_admission;

      /* Assign the default GateKeeper address */

      default_ras_info.gk_addr = ras_info->gk_addr;

      /* load endpoint here */

      if (ras_info->endpoint_identifier_length < 128)
         {
         for ( i = 0; i < ras_info->endpoint_identifier_length; i++ )
            {
            default_ras_info.endpoint_identifier[i] = ras_info->endpoint_identifier[i];
            }
         default_ras_info.endpoint_identifier_length = ras_info->endpoint_identifier_length;
         }
         return ( 0 );
      }
   else
      {
      return ERR_PARM;
      }
}
/*   -------------- IPTOS ----------------                */
/* Return the IP TOS field                                */
/*                                                        */
/* Input unsigned integer PRE : precedence value          */
/*       unsigned integer TOS : type of service requested */ 
/*                                                        */
/* Output unsigned integer : IP TOS composed of PRE,TOS   */
/*                                                        */
/* NOTE to indicate that this is not a default zero value */
/* the last bit of the first byte is always set           */
/*  ----------------------------------------              */
ACUDLL ACU_UINT call_generate_iptos ( unsigned int pre, unsigned int tos )
{
    /* ignore all but the last three bits */
    pre &= 0x07;
    tos &= 0x07; 

    switch (tos)
    {
        case ACU_PREC_TOS_HIGH_RELIABILITY:
        case ACU_PREC_TOS_HIGH_THROUGHPUT:
        case ACU_PREC_TOS_LOW_DELAY:
            break;
        default:
            tos = ACU_PREC_TOS_DEFAULT;
    }

    /* if all bits are 0 then normal service */
    if (tos == ACU_PREC_TOS_DEFAULT) pre = ACU_PREC_TOS_DEFAULT; 

    tos <<= 2;
    pre <<= 5;

    /* stitch them together and always set the next bit */
    return (pre | tos | 0x100 );
}

/*   -------------- DSCP ----------------                  */
/* Return the DSCP field                                   */
/*                                                         */
/* Input unsigned integer CP : code point value	           */
/*                                                         */
/* Output unsigned integer : DSCP composed of CP,2bits(=0) */
/*                                                         */
/* NOTE as with iptos above we set the next bit to indicate*/
/* that this is not a default zero value                   */
/*  ----------------------------------------               */
ACUDLL ACU_UINT call_generate_dscp ( unsigned int cp )
{
    cp &= 0x3F;
    cp <<= 2;
    return (cp | 0x100);
}



/*-------------- call_register_system ---------------------*/
/*   Register a VoIP endpoint with a server                */
/*                                                         */
/*   Currently this only works for SIP                     */
/*                                                         */

ACUDLL int ACU_WINAPI call_register_system ( struct register_xparms * reg_parms )
{
    if( S_SIP != call_type(reg_parms->net) )
    {
        return ERR_COMMAND;
    }

    return clioctl ( VOIP_NEW_REGISTRATION, (IOCTLU *) reg_parms,
                       unet_2_card ( reg_parms->net ), reg_parms->net, sizeof ( REGISTER_XPARMS ) );
}

/*-------------- call_unregister_system -------------------*/
/*   Unregister a VoIP endpoint with a server              */
/*                                                         */
/*   Currently this only works for SIP                     */
/*                                                         */
ACUDLL int ACU_WINAPI call_unregister_system ( struct register_xparms * reg_parms )
{
    if( S_SIP != call_type(reg_parms->net) )
    {
        return ERR_COMMAND;
    }

    return clioctl ( VOIP_DELETE_REGISTRATION, (IOCTLU *) reg_parms,
                       unet_2_card ( reg_parms->net ), reg_parms->net, sizeof ( REGISTER_XPARMS ) );
}

/*-------------- call_query_register_system ---------------*/
/*   Fetch the registrations associated with a particular  */
/*   endpoint                                              */
/*                                                         */
/*   Currently this only works for SIP. And is only        */
/*   partially implemented                                 */

ACUDLL int ACU_WINAPI call_query_register_system ( struct register_xparms * reg_parms )
{
    if( S_SIP != call_type(reg_parms->net) )
    {
        return ERR_COMMAND;
    }

    return clioctl ( VOIP_FETCH_REGISTRATION, (IOCTLU *) reg_parms,
                       unet_2_card ( reg_parms->net ), reg_parms->net, sizeof ( REGISTER_XPARMS ) );
}


/*------------ end of file --------------*/


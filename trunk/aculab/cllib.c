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
/* rev:  5.8.0    17/10/2001  Libs for V5.8.0                 */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/

#include "mvcldrvr.h"


#ifdef ACU_VOIP_CC
/*--- VoIP specific ---*/
#include "common.h"
#include "vcc_os_threads.h"
#include "generic_tls.h"
#include "h323_thread.h"
#include "admin_channel.h"
#include "tls_ras_thread_c_if.h"
#include "tls_conn_mgr_c_if.h"
#include "tls_h323.h"                 /* required for tls_h323_init */
#include "call_record.h"              /* call record function declarations */
#include "tls_rcm.h"                  /* required for tls_rcm_init */
#include "pipe_client.h"
#include "v1bmi.h"
#include "cc_log.h"
/*----------------------*/
#endif


#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#ifdef ACU_VOIP_CC
int first_voip_card;                   /* card number of first voip card */
bool call_driver_installed=TRUE;       /* identifies existance of call driver code */
#endif

#define FALSE  0
#define TRUE   1


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
ACUDLL int call_free_admin_msg ( voip_admin_msg * adminp);
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

static int  feature_openout_enquiry ( int, FEATURE_OUT_XPARMS * );
ACUDLL int ACU_WINAPI call_feature_details ( struct feature_detail_xparms * );
static ACU_INT call_maintenance_cmd(ACUC_MAINTENANCE_XPARMS * ) ;

#ifdef ACU_VOIP_CC
static int process_voip_admin_msg (uint32 msg_type, IOCTLU *pioctlu); /* VoIP only */
int start_rpc_sessions ( char * board_ip_address , int card_num ); /* VoIP only */
int parse_config_str ( char * config_string, char * ip_address1, char * ip_address2,
                       char *subnet_address1, char * subnet_address2, char * gateway_address, int * Download_DHCP);
void parse_string (char *config_string, char search_string[4],char *found_string);
#endif

/*------------- external functions --------------*/

ACUDLL int      clopen         ( char * );
ACUDLL ACU_INT  clioctl        ( ACU_INT, IOCTLU *, int, int);
ACUDLL ACU_INT  clpblock_ioctl ( ACU_INT, V5_PBLOCK_IOCTLU *, int, int );
ACUDLL void     clclose        ( void );
ACUDLL ACU_INT  cldosioctl     ( int, char *, int );
ACUDLL void     clspecial      ( void );
ACUDLL char     * cldev        ( void );


#ifdef ACU_VOIP_CC
extern ACUDLL ACU_INT  voipioctl      ( ACU_INT, IOCTLU *, int, int , int );
extern void init_card_info ( int );
extern acu_error management_init( char * boardipaddress, unsigned int board_number);
extern acu_error session_init( char * boardipaddress);
extern flag_pipe_not_running();
extern bool is_pipe_running();

extern ACU_VCC_THREAD_ID pipe_client_thread[MAX_NUM_BOARDS+1];
extern done_init[];

extern int        xxcall_event   ( LEQ far * );      
#endif



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
#ifdef ACU_VOIP_CC

static uint8 default_codecs[MAXCODECS+1];     /* default codec list            */
static DEFAULT_RAS_CONFIG default_ras_info;   /* default structure for RAS */
static const int MANAGEMENT_INIT_TIMEOUT = 25 ;
#endif

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
#ifdef ACU_VOIP_CC
   int voip_card_number=0;    /* numbers voip cards 1,2,3 etc. */
   ACU_VCC_THREAD_ID this_thread;           /* VoIP specific */
   V1BMI_CARD_INFO_PARMS cardinfo ;
   struct set_sysinfo_xparms set_sysinfo;
   struct sysinfo_xparms    sysinfo;
   char   board_addr[32] = "";
   int    board_net_number =-1;   /* -1 so first net is 0 */
#endif

   result = clopendev ( );              /* open the device */

   if ( result == 0 )
      {
      /* initialise all cards in the system */

      for ( i = 0; i < ncards; i++ )
         {
#ifdef ACU_VOIP_CC
         board_net_number = board_net_number+clcard[i].nnets;     /* keep a count of the boards nets */
         if (clcard[i].voipservice == ACU_VOIP_ACTIVE)
            {  
            /* copy the ip address into clcard */
            result =  v1bmi_card_info( clcard[i].v1bmi_card_num , &cardinfo );

            if (result != 0)
               {
               ACU_LOG(call_get_voip_admin_msg, warn,("Error: failed to access card %d\n",i));
               return(ERR_CFAIL);
               }
            if (cardinfo.ip_is_configured!=0)
               {
               sprintf(board_addr, "%d.%d.%d.%d",    
                       cardinfo.ip_addr1[0],
                       cardinfo.ip_addr1[1],
                       cardinfo.ip_addr1[2],
                       cardinfo.ip_addr1[3]);

               strcpy(clcard[i].board_ip_address,board_addr);

        /* 
         * new method for obtaining voip card number - this now works 
         * when no PM2 module is present
         */
         
               voip_card_number++;

               set_sysinfo.net = board_net_number;
               set_sysinfo.board_number = voip_card_number;
               strcpy(set_sysinfo.board_ip_address , board_addr);
 
               result = start_rpc_sessions(board_addr,voip_card_number);
               
               if (result == ACU_ERR_WRONG_BOARD_VER) return(ERR_BOARD_VERSION);
               
               if (result != 0) return(ERR_NO_BOARD);
               
               call_set_system_info ( &set_sysinfo,&sysinfo );
               }
            else
               {    /* the driver has not been configured so VOIPLDR has not been run.*/
               ACU_LOG(call_get_voip_admin_msg, warn,("Error: Software not downloaded for card %d\n",i));
               return(ERR_BOARD_UNLOADED);
               }
            /* assign this thread id to init_xparms*/

            vcc_thread_get_id(&this_thread);
            initp->unique_xparms.sig_h323.calling_thread = this_thread;
            result = voipioctl ( CALL_INIT, 
                                 (IOCTLU *) initp, 
                                 clcard[i].clh,
                                 sizeof ( INIT_XPARMS ),
                                 card_2_voipcard(i));
            }
         else
            {
            result = clioctl ( CALL_INIT, 
                               (IOCTLU *) initp, 
                               clcard[i].clh,
                               sizeof ( INIT_XPARMS ));
            }
#else
         result = clioctl ( CALL_INIT,
                           (IOCTLU *) initp,
                            clcard[i].clh,
                            sizeof ( INIT_XPARMS ));
#endif
         }
      }
   else
      {
      result = ERR_CFAIL;
      }

#ifdef ACU_VOIP_CC
   if ((result != 0)&&(!is_pipe_running()))     /* check that the service is up and running */
      {
      for (i=0;i<20;i++)
         {
         if (is_pipe_running())
            break;
         Sleep(500);
         }
      if (!is_pipe_running())       /* if not up after 5 seconds then flag an error */
         {
         result = ERR_NO_SERVICE;
         ACU_LOG(call_get_voip_admin_msg, critical,("Error: failed to access service \n"));
         }
      }

#endif

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

#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif

         outdetailsp->net = unet_2_net ( unet );     /* and the network port */

         if ( (outdetailsp->net) >= 0 )              /* check for error */
            {

            result = clioctl ( DPNS_OPENOUT,
                               (IOCTLU *) outdetailsp,
                               clcard[card].clh,
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

#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif

         overlapp->handle = dc.handle;       /* get the fixed up handle */

         if ( ! isallowed_string ( overlapp->destination_addr ) )
            {
            dc.result = ERR_PARM;
            }
         else
            {
            dc.result = clioctl ( DPNS_SEND_OVERLAP,
                                  (IOCTLU *) overlapp,
                                   clcard[dc.card].clh,
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

ACUDLL int ACU_WINAPI dpns_call_details ( detailsp )
struct dpns_detail_xparms * detailsp;
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
#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif
         detailsp->handle = dc.handle;       /* get the fixed up handle */

         dc.result = clioctl ( DPNS_CALL_DETAILS,
                              (IOCTLU *) detailsp,
                               clcard[dc.card].clh,
                               sizeof (DPNS_DETAIL_XPARMS));
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
#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif

         inringp->handle = dc.handle;   /* get the fixed up handle */

         dc.result = clioctl ( DPNS_INCOMING_RINGING,
                              (IOCTLU *) inringp,
                               clcard[dc.card].clh,
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
#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif

         acceptp->handle = dc.handle;   /* get the fixed up handle */

         dc.result = clioctl ( DPNS_CALL_ACCEPT,
                               (IOCTLU *) acceptp,
                               clcard[dc.card].clh,
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
#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif

         featurep->handle = dc.handle;   /* get the fixed up handle */

         dc.result = clioctl ( DPNS_SEND_FEAT_INFO,
                               (IOCTLU *) featurep,
                               clcard[dc.card].clh,
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

#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif

         dc.result = clioctl ( DPNS_SET_TRANSIT,
                              (IOCTLU *) transitp,
                              clcard[dc.card].clh,
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

#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif
         transitp->handle = dc.handle;       /* get the fixed up handle */

         dc.result = clioctl ( DPNS_TRANSIT_DETAILS,
                               (IOCTLU *) transitp,
                               clcard[dc.card].clh,
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
#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif

         transitp->handle = dc.handle;   /* get the fixed up handle */

         dc.result = clioctl ( DPNS_SEND_TRANSIT,
                              (IOCTLU *) transitp,
                              clcard[dc.card].clh,
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
#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif
         causep->handle = dc.handle;

         dc.result = clioctl ( DPNS_DISCONNECT,
                               (IOCTLU *) causep,
                               clcard[dc.card].clh,
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
#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif
         causep->handle = dc.handle;

         /* clear the entry before going to the driver */
         /* this will stop us calling the driver from  */
         /* the exit list routine                      */

         dc.result = clioctl ( DPNS_RELEASE,
                               (IOCTLU *) causep,
                               clcard[dc.card].clh,
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
#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif
         causep->handle = dc.handle;

         dc.result = clioctl ( DPNS_GETCAUSE,
                               (IOCTLU *) causep,
                               clcard[dc.card].clh,
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
#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif
         dpns_l2_parms->net = unet_2_net ( unet );     /* Get network port */

         if ( dpns_l2_parms->net >= 0 )                /* Check for error */
            {
            result = clioctl ( DPNS_SET_L2_CH,
                               (IOCTLU *) dpns_l2_parms,
                               clcard[card].clh,
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
#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif
         dpns_l2_parms->net = unet_2_net ( unet );     /* Get network port */

         if ( dpns_l2_parms->net >= 0 )                /* Check for error */
            {
            result = clioctl ( DPNS_L2_STATE,
                               (IOCTLU *) dpns_l2_parms,
                               clcard[card].clh,
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
   struct siginfo_xparms * lsigp;
#ifdef ACU_VOIP_CC
   ACU_VCC_THREAD_ID this_thread;           /* VoIP specific */
#endif

   result = clopendev ( );

   if ( result == 0 )
      {
      lsigp = siginfop;       /* keep a local copy of the info pointer */

      for ( i = 0; i < ncards; i++ )
         {
#ifndef ACU_VOIP_CC
         if ( 0 )
            {
#else
         if (clcard[i].voipservice == ACU_VOIP_ACTIVE)
            {
            vcc_thread_get_id(&this_thread);
            lsigp->unique_xparms.sig_h323.calling_thread = this_thread;

            result = voipioctl ( CALL_SIGNAL_INFO,
                                (IOCTLU *) lsigp,
                                 clcard[i].clh,
                                 sizeof ( SIGINFO_XPARMS ) * MAXPPC ,
                                 card_2_voipcard(i));
#endif
            }
         else
            {
            result = clioctl ( CALL_SIGNAL_INFO,
                               (IOCTLU *) lsigp,
                               clcard[i].clh,
                               sizeof ( SIGINFO_XPARMS ) * MAXPPC );
            }

         if ( result == 0 )
            {
            /* move the info pointer by the number */
            /* of networks on the card             */

            lsigp = &lsigp[lsigp->nnets];
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
#ifdef ACU_VOIP_CC
   ACU_VCC_THREAD_ID this_thread;           /* VoIP specific */
#endif


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
#ifndef ACU_VOIP_CC
            if ( 0 )
               {
          /* If VoIP card then thread identification is required */
#else
            if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
               {
               vcc_thread_get_id(&this_thread);
               sysinfop->unique_xparms.sig_h323.calling_thread = this_thread; /* assign this thread id */
               result = voipioctl ( CALL_SYSTEM_INFO,
                                    (IOCTLU *) sysinfop,
                                    clcard[card].clh,
                                    sizeof (SYSINFO_XPARMS ),
                                    card_2_voipcard(card));
#endif
               }
            else
               {
               result = clioctl ( CALL_SYSTEM_INFO,
                                  (IOCTLU *) sysinfop,
                                  clcard[card].clh,
                                  sizeof (SYSINFO_XPARMS ));
               }
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



#ifdef ACU_VOIP_CC
/*---------- call_set_system_info -------*/
/* set the system information            */
/*                                       */
ACUDLL int ACU_WINAPI call_set_system_info ( struct set_sysinfo_xparms * set_sysinfop, struct sysinfo_xparms * sysinfop)
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;
   ACU_VCC_THREAD_ID this_thread;                  /* VoIP specific */

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
               vcc_thread_get_id(&this_thread);
               set_sysinfop->unique_xparms.sig_h323.calling_thread = this_thread; /* assign this thread id */
               }

               result = voipioctl ( CALL_SET_SYSTEM_INFO,
                                    (IOCTLU *) set_sysinfop,
                                    clcard[card].clh,
                                    sizeof (SET_SYSINFO_XPARMS ),
                                    card_2_voipcard(card));
               if ( result != 0 )
                  {
                  result = ERR_CFAIL;
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
#endif


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
#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif
         send_spidp->net = unet_2_net ( unet ); /* and the network port */

         if ( send_spidp->net >= 0 )     /* check for error */
            {
            result = clioctl ( CALL_ENDPOINT_INITIALISE,
                               (IOCTLU *) send_spidp,
                               clcard[card].clh,
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
#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif
         sendspidp->net = unet_2_net ( unet ); /* and the network port */

         if ( sendspidp->net >= 0 )     /* check for error */
            {
            result = clioctl ( CALL_GET_ENDPOINT_STATUS,
                               (IOCTLU *) sendspidp,
                               clcard[card].clh,
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
#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif
         get_spidp->net = unet_2_net ( unet ); /* and the network port */

         if ( get_spidp->net >= 0 )     /* check for error */
            {
            result = clioctl ( CALL_GET_SPID,
                               (IOCTLU *) get_spidp,
                               clcard[card].clh,
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
#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif

         send_endpointp->net = unet_2_net ( unet ); /* and the network port */

         if ( send_endpointp->net >= 0 )     /* check for error */
            {
            result = clioctl ( CALL_SEND_ENDPOINT_ID,
                               (IOCTLU *) send_endpointp,
                               clcard[card].clh,
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
   ACU_INT  unet;
   ACU_INT  uhandle;
   DC       dc;
#ifdef ACU_VOIP_CC
   ACU_VCC_THREAD_ID this_thread;                 /* VoIP specific */
#endif


   dc.result = clopendev ( );                                      /* open the device */


   if ( dc.result == 0 )
      {
      handle_decomp ( keypadp->handle, &dc );

      switch( call_type(handle2net(keypadp->handle)) )
         {
#ifdef ACU_VOIP_CC
         case S_H323:
         /* If VoIP card then thread identification is required */
            vcc_thread_get_id(&this_thread);
            keypadp->unique_xparms.sig_h323.calling_thread = this_thread; /* assign thread id */

         /* also check keypad ie length is valid */
         if (keypadp->unique_xparms.sig_h323.keypad.ie[0] >= MAXKEYPAD)
            {
            dc.result = ERR_PARM;
            }

         if (dc.result >= 0)
            {
            keypadp->handle = dc.handle;

            dc.result = voipioctl ( cmd,
                                     ( IOCTLU *) keypadp,
                                       clcard[dc.card].clh,
                                       sizeof (KEYPAD_XPARMS),
                                       card_2_voipcard(dc.card) );
            }
         break;
#endif

         default :   /* Q.931 */

            unet    = keypadp->unique_xparms.sig_q931.net;               /* save user net    */
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
                                        clcard[card].clh,
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
                                        clcard[dc.card].clh,
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

   keypadp->unique_xparms.sig_q931.net = unet;               /* restore user parameter */
   keypadp->handle = uhandle;         /* restore user parameter */

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
#ifndef ACU_VOIP_CC
      if ( 0 )
         {
#else
      /* Check for VoIP card */
      card = unet_2_card (indetailsp->net);
      if ( card >= 0 && clcard[card].voipservice == ACU_VOIP_ACTIVE)
         {
         ACU_INT sig_net  = indetailsp->net ;  /* Fixed up by xlate_assoc */
         ACU_VCC_THREAD_ID this_thread;

         vcc_thread_get_id(&this_thread);
         indetailsp->unique_xparms.sig_h323.calling_thread = this_thread; /* assign this thread id */

         indetailsp->net = unet_2_net ( sig_net ); /* and the network port */


         result = voipioctl ( CALL_OPENIN,
                            ( IOCTLU *) indetailsp,
                              clcard[card].clh,
                              sizeof (IN_XPARMS),
                              card_2_voipcard(card));
         if ( result == 0 )
            {
            indetailsp->handle = patch_handle (indetailsp->handle, sig_net );
            }
         else
            {
               return (result);  /* return error code */
            }
#endif
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
                                  clcard[card].clh,
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
#ifdef ACU_VOIP_CC
   ACU_INT  i;
#endif

   unet = calloutp->net;            /* save user net */

   calloutp->cnf |= CNF_ENABLE_V5API;     /* Enable V5 drivers events & features not found with V4 drivers */

   result = clopendev ( );

   if ( result == 0 )
      {
      card = unet_2_card (calloutp->net);

#ifndef ACU_VOIP_CC
      if ( 0 )
         {
#else
     /* Check for VoIP card */
      if (card >= 0 && clcard[card].voipservice == ACU_VOIP_ACTIVE)
         {
         ACU_INT sig_net  = calloutp->net ;   /* Fixed up by xlate_assoc */

         ACU_VCC_THREAD_ID this_thread;
         vcc_thread_get_id(&this_thread);
         calloutp->unique_xparms.sig_h323.calling_thread = this_thread; /* put in this thread id */

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

         result = voipioctl ( mode,
                              (IOCTLU *) calloutp,
                              clcard[card].clh,
                              sizeof (OUT_XPARMS),
                              card_2_voipcard(card));

         if (result == 0);
            {
            /* now patch the handle to reflect the network */
            calloutp->handle = patch_handle (calloutp->handle, sig_net );
            }

         return(result);
#endif
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
                                        clcard[card].clh,
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
#ifdef ACU_VOIP_CC
   ACU_VCC_THREAD_ID this_thread;
#endif


   uhandle = statep->handle;                /* save user handle */

   dc.result = clopendev ( );               /* open the device  */

   if ( dc.result == 0 )
      {
      handle_decomp ( statep->handle, &dc );

      if ( dc.result >= 0 )                 /* check for error */
         {
         statep->handle = dc.handle;

#ifndef ACU_VOIP_CC
         if ( 0 )
            {
#else
         if ( dc.card >= 0 && clcard[dc.card].voipservice == ACU_VOIP_ACTIVE )
            {
            /* VoIP specific */
            vcc_thread_get_id(&this_thread);
            statep->unique_xparms.sig_h323.calling_thread = this_thread; /* put in this thread id */

            dc.result = voipioctl ( CALL_STATE,
                                   (IOCTLU *) statep,
                                   clcard[dc.card].clh,
                                   sizeof (STATE_XPARMS),
                                   card_2_voipcard(dc.card));

#endif
            }
         else
            {
            dc.result = clioctl ( CALL_STATE,
                                  (IOCTLU *) statep,
                                  clcard[dc.card].clh,
                                  sizeof (STATE_XPARMS));
            }
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
      card  = unet_2_card ( unet );
      if ( eventp->handle != 0 )
         {
         unet = handle2net ( eventp->handle );

         event_if_xparms.cmd = ACUC_EVENT_POLL_SPECIFIC ;
         event_if_xparms.timeout = eventp->timeout ;

         result = clioctl ( CALL_EVENT_IF,
                            (IOCTLU *) &event_if_xparms,
                            clcard[card].clh,
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
                            clcard[0].clh,
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
                            clcard[event_if_xparms.cnum].clh,
                            sizeof (event_if_xparms));

              if (event_if_xparms.handle != 0)
                 {
                 break ;		/* Got valid event! */
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
   static  ACU_INT fsm = 0;
   ACU_INT  result;   
   ACU_INT  unet;
   LEQ      leq;

#ifdef ACU_VOIP_CC
   ACU_VCC_THREAD_ID this_thread;   /* required for VoIP messaging */
   /* obtain and store this thread id for VoIP */
   vcc_thread_get_id(&this_thread);
   leq.unique_xparms.sig_h323.calling_thread = this_thread;    
   eventp->unique_xparms.sig_h323.calling_thread = this_thread;
#endif

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
         }

      eventp->handle = 0;               /* clear for the call */
      leq.timeout    = eventp->timeout; /* patch the timeout */

#ifdef ACU_VOIP_CC
      if ( call_driver_installed ) 
         {

      /* call driver installed somewhere in the system so we can call clioctl */
         result = clioctl ( CALL_LEQ, 
                           (IOCTLU *) &leq, 
                           clcard[0].clh,
                           sizeof (LEQ ));
         }
      else 
         {
         /* no call driver so must deal with it in the user space */
         result = xxcall_event ( &leq );
         }
#else
      result = clioctl ( CALL_LEQ, 
                         (IOCTLU *) &leq, 
                         clcard[0].clh,
                         sizeof (LEQ ));

#endif

      if ( result == 0 )
         {
         if ( leq.valid )
            {
            eventp->handle  = nch2handle ( leq.net, leq.ch );
            eventp->handle |= leq.io;                               /* add the direction */
            eventp->timeout = 0;

#ifndef ACU_VOIP_CC
            if ( 0 )
               {
#else
            if (clcard[leq.card].voipservice == ACU_VOIP_ACTIVE)
               {
               result = voipioctl ( CALL_LEQ_S, 
                                   (IOCTLU *) eventp, 
                                   clcard[leq.card].clh,
                                   sizeof (STATE_XPARMS ),
                                   card_2_voipcard(leq.card));
#endif
               }
            else
               {
               result = clioctl ( CALL_LEQ_S, 
                                  (IOCTLU *) eventp, 
                                  clcard[leq.card].clh,
                                  sizeof (STATE_XPARMS ));
               }

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
#ifdef ACU_VOIP_CC
   ACU_VCC_THREAD_ID this_thread;
#endif

   uhandle = detailsp->handle;               /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( detailsp->handle, &dc );

      if ( dc.result >= 0 )                  /* check for error */
         {
#ifdef ACU_VOIP_CC
         switch(call_type(handle2net(detailsp->handle)))
            {
            case S_H323 :
            /* assign this thread id */
               vcc_thread_get_id(&this_thread);
               detailsp->unique_xparms.sig_h323.calling_thread = this_thread;
               detailsp->handle = dc.handle;       /* get the fixed up handle */

               dc.result = voipioctl ( CALL_DETAILS,
                                      (IOCTLU *) detailsp,
                                       clcard[dc.card].clh,
                                       sizeof (DETAIL_XPARMS),
                                       card_2_voipcard(dc.card));
            break;

            default :
               detailsp->handle = dc.handle;       /* get the fixed up handle */

               dc.result = clioctl ( CALL_DETAILS,
                                    (IOCTLU *) detailsp,
                                     clcard[dc.card].clh,
                                     sizeof (DETAIL_XPARMS));
            break;
            }
#else
         detailsp->handle = dc.handle;       /* get the fixed up handle */

         dc.result = clioctl ( CALL_DETAILS,
                               (IOCTLU *) detailsp,
                                clcard[dc.card].clh,
                                sizeof (DETAIL_XPARMS));
#endif
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

#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif
         chargep->handle = dc.handle;       /* get the fixed up handle */

         dc.result = clioctl ( CALL_GET_CHARGE,
                               (IOCTLU *) chargep,
                                clcard[dc.card].clh,
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

#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif

         chargep->handle = dc.handle;       /* get the fixed up handle */

         dc.result = clioctl ( CALL_PUT_CHARGE,
                               (IOCTLU *) chargep,
                                clcard[dc.card].clh,
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

#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif

         overlapp->handle = dc.handle;       /* get the fixed up handle */

         if ( ! isallowed_string ( overlapp->destination_addr ) )
            {
            dc.result = ERR_PARM;
            }
         else
            {
            dc.result = clioctl ( CALL_SEND_OVERLAP,
                                  (IOCTLU *) overlapp,
                                   clcard[dc.card].clh,
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
#ifdef ACU_VOIP_CC
   ACU_VCC_THREAD_ID this_thread; /* VoIP specific */
   ACU_INT i;
#endif

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( acceptp->handle, &dc );

      switch(call_type(handle2net(acceptp->handle)))
         {
#ifdef ACU_VOIP_CC
         case S_H323:
            /*
             * VoIP specific - assign this thread id
             * used by the generic/tls thread to send ACK message back
             */
            vcc_thread_get_id(&this_thread);
            acceptp->unique_xparms.sig_h323.calling_thread = this_thread;

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
#endif
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

#ifndef ACU_VOIP_CC
         if ( 0 )
            {
#else
         if (dc.card >= 0 && clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            dc.result = voipioctl ( CALL_ACCEPT,
                                   (IOCTLU *) acceptp,
                                    clcard[dc.card].clh,
                                    sizeof ( ACCEPT_XPARMS ),
                                    card_2_voipcard(dc.card));
#endif
            }
         else
            {
            dc.result = clioctl ( CALL_ACCEPT,
                                 (IOCTLU *) acceptp,
                                  clcard[dc.card].clh,
                                  sizeof ( ACCEPT_XPARMS ));
            }
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


/*-------------- call_hold --------------*/
/* put call on hold                      */
/*                                       */

ACUDLL int ACU_WINAPI xcall_hold ( HOLD_XPARMS *holdp )
   {
   DC   dc;
#ifdef ACU_VOIP_CC
   ACU_VCC_THREAD_ID this_thread;
#endif

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( holdp->handle, &dc );

      if ( dc.result >= 0 )             /* check for error */
         {
         holdp->handle = dc.handle;     /* get the fixed up handle */


#ifndef ACU_VOIP_CC
         if ( 0 )
            {
#else
         if (dc.card >= 0 && clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            /*
             * VoIP specific - assign this thread id
             * used by the generic/tls thread to send ACK message back
             */
            vcc_thread_get_id(&this_thread);
            holdp->unique_xparms.sig_h323.calling_thread = this_thread;

            dc.result = voipioctl ( CALL_HOLD,
                                 (IOCTLU *) holdp,
                                  clcard[dc.card].clh,
                                  sizeof ( HOLD_XPARMS ),
                                  card_2_voipcard(dc.card));
#endif
            }
         else
            {
            dc.result = clioctl ( CALL_HOLD,
                               (IOCTLU *) holdp,
                               clcard[dc.card].clh,
                               sizeof ( HOLD_XPARMS ));
            }
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
#ifdef ACU_VOIP_CC
   ACU_VCC_THREAD_ID this_thread;
#endif


   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( holdp->handle, &dc );

      if ( dc.result >= 0 )             /* check for error */
         {
         holdp->handle = dc.handle;     /* get the fixed up handle */

#ifndef ACU_VOIP_CC
         if ( 0 )
            {
#else
         if (dc.card >= 0 && clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            /*
             * VoIP specific - assign this thread id
             * used by the generic/tls thread to send ACK message back
             */
            vcc_thread_get_id(&this_thread);
            holdp->unique_xparms.sig_h323.calling_thread = this_thread;
            dc.result = voipioctl ( CALL_RECONNECT,
                                  (IOCTLU *) holdp,
                                  clcard[dc.card].clh,
                                  sizeof ( HOLD_XPARMS ),
                                  card_2_voipcard(dc.card));
#endif
            }
         else
            {
            dc.result = clioctl ( CALL_RECONNECT,
                                  (IOCTLU *) holdp,
                                  clcard[dc.card].clh,
                                  sizeof ( HOLD_XPARMS ));
            }
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


   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      dc.result = ERR_PARM;

      /* must be on the same port */

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

#ifdef ACU_VOIP_CC
               /* command not supported for VoIP */
               if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
                  {
                  return (ERR_COMMAND);
               }
#endif
               transferp->handlea = dc.handle;       /* get the fixed up handle */

               handle_decomp ( transferp->handlec, &dc );

               if ( dc.result >= 0 )                  /* check for error */
                  {
                  transferp->handlec = dc.handle;       /* get the fixed up handle */

                  dc.result = clioctl ( CALL_TRANSFER,
                                        (IOCTLU *) transferp,
                                         clcard[dc.card].clh,
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
#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif
         causep->handle = dc.handle;

         dc.result = clioctl ( CALL_ANSWERCODE,
                               (IOCTLU *) causep,
                               clcard[dc.card].clh,
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
#ifdef ACU_VOIP_CC
   ACU_VCC_THREAD_ID this_thread;
#endif

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( incoming_ringingp->handle, &dc );

      switch(call_type(handle2net(incoming_ringingp->handle)))
          {
#ifdef ACU_VOIP_CC
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
#endif

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
#ifndef ACU_VOIP_CC
         if ( 0 )
            {
#else
         if (dc.card >= 0 && clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            /*
             * VoIP specific - assign this thread id
             * used by the generic/tls thread to send ACK message back
             */
             vcc_thread_get_id(&this_thread);
             incoming_ringingp->unique_xparms.sig_h323.calling_thread = this_thread;

             dc.result = voipioctl ( CALL_INCOMING_RINGING,
                                    (IOCTLU *) incoming_ringingp,
                                     clcard[dc.card].clh,
                                     sizeof ( INCOMING_RINGING_XPARMS ),
                                     card_2_voipcard(dc.card));
#endif
             }
         else
             {
             dc.result = clioctl ( CALL_INCOMING_RINGING,
                                  (IOCTLU *) incoming_ringingp,
                                   clcard[dc.card].clh,
                                   sizeof ( INCOMING_RINGING_XPARMS ));
             }
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
#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif
         get_originating_addrp->handle = dc.handle;   /* get the fixed up handle */

         dc.result = clioctl ( CALL_GET_ORIGINATING_ADDR,
                               (IOCTLU *) get_originating_addrp,
                               clcard[dc.card].clh,
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


/*------------- call_disconnect ---------*/
/* release channel                       */
/*                                       */

ACUDLL int ACU_WINAPI xcall_disconnect ( struct disconnect_xparms * disconnectp )
   {
   ACU_INT  uhandle;
   DC   dc;
#ifdef ACU_VOIP_CC
   ACU_VCC_THREAD_ID this_thread;
#endif

   uhandle = disconnectp->handle;            /* save user handle */

   dc.result = clopendev ( );

   if (dc.result >= 0)
       {
       handle_decomp ( disconnectp->handle, &dc );

       switch(call_type(handle2net(disconnectp->handle)))
           {
#ifdef ACU_VOIP_CC
           case S_H323 :
              /* assign this thread id */
              vcc_thread_get_id(&this_thread);
              disconnectp->unique_xparms.sig_h323.calling_thread = this_thread;
              break;
#endif
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

#ifndef ACU_VOIP_CC
         if ( 0 )
            {
#else
         if (dc.card >= 0 && clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            dc.result = voipioctl ( CALL_DISCONNECT,
                                  (IOCTLU *) disconnectp,
                                  clcard[dc.card].clh,
                                  sizeof ( DISCONNECT_XPARMS ),
                                  card_2_voipcard(dc.card));
#endif
            }
         else
            {
            dc.result = clioctl ( CALL_DISCONNECT,
                                  (IOCTLU *) disconnectp,
                                  clcard[dc.card].clh,
                                  sizeof ( DISCONNECT_XPARMS ));
            }
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
#ifdef ACU_VOIP_CC
   ACU_VCC_THREAD_ID this_thread;
#endif

   uhandle = disconnectp->handle;            /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( disconnectp->handle, &dc );

      if ( dc.result >= 0 )            /* check for error */
         {
         disconnectp->handle = dc.handle;

#ifndef ACU_VOIP_CC
         if ( 0 )
            {
#else
         if (dc.card >= 0 && clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* If VoIP call then assign calling thread */
            /* identity otherwise do nothing           */
            vcc_thread_get_id(&this_thread);
            disconnectp->unique_xparms.sig_h323.calling_thread = this_thread;

            dc.result = voipioctl ( CALL_GETCAUSE,
                                 (IOCTLU *) disconnectp,
                                  clcard[dc.card].clh,
                                  sizeof ( DISCONNECT_XPARMS ),
                                  card_2_voipcard(dc.card));
#endif
            }
         else
            {
            dc.result = clioctl ( CALL_GETCAUSE,
                                  (IOCTLU *) disconnectp,
                                  clcard[dc.card].clh,
                                  sizeof ( DISCONNECT_XPARMS ));
            }
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

/*------------- call_release ------------*/
/* release channel                       */
/*                                       */

ACUDLL int ACU_WINAPI xcall_release ( struct disconnect_xparms * disconnectp )
   {
   ACU_INT  uhandle;
   DC   dc;
#ifdef ACU_VOIP_CC
   ACU_VCC_THREAD_ID this_thread;
#endif

   uhandle = disconnectp->handle;            /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( disconnectp->handle, &dc );

      switch(call_type(handle2net(disconnectp->handle)))
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

#ifndef ACU_VOIP_CC
         if ( 0 )
            {
#else
         if (dc.card >= 0 && clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            vcc_thread_get_id(&this_thread);
            disconnectp->unique_xparms.sig_h323.calling_thread = this_thread;

            dc.result = voipioctl ( CALL_RELEASE,
                                  (IOCTLU *) disconnectp,
                                  clcard[dc.card].clh,
                                  sizeof ( DISCONNECT_XPARMS ),
                                  card_2_voipcard(dc.card));
#endif
            }
         else
            {
            dc.result = clioctl ( CALL_RELEASE,
                                  (IOCTLU *) disconnectp,
                                  clcard[dc.card].clh,
                                  sizeof ( DISCONNECT_XPARMS ));
            }
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
#ifdef ACU_VOIP_CC
   ACU_VCC_THREAD_ID this_thread;              /* VoIP specific */
#endif

   uhandle = progressp->handle;            /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( progressp->handle, &dc );

      switch(call_type(handle2net(progressp->handle)))
         {
#ifdef ACU_VOIP_CC
         case S_H323 :
            /* also check display length */
            if (progressp->unique_xparms.sig_h323.display.ie[0] >= MAXDISPLAY)
               {
               dc.result = ERR_PARM;
               }
            break;
#endif

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

#ifndef ACU_VOIP_CC
         if ( 0 )
            {
#else
         if (dc.card >= 0 && clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* If VoIP card then thread identification is required */
            vcc_thread_get_id(&this_thread);
            progressp->unique_xparms.sig_h323.calling_thread = this_thread; /* assign thread id */

            dc.result = voipioctl ( CALL_PROGRESS,
                                  (IOCTLU *) progressp,
                                  clcard[dc.card].clh,
                                  sizeof ( PROGRESS_XPARMS ),
                                  card_2_voipcard(dc.card));
#endif
            }
         else
            {
            dc.result = clioctl ( CALL_PROGRESS,
                                  (IOCTLU *) progressp,
                                  clcard[dc.card].clh,
                                  sizeof ( PROGRESS_XPARMS ));
            }
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
#ifdef ACU_VOIP_CC
   ACU_VCC_THREAD_ID this_thread;                /* VoIP specific */
   ACU_INT   i;
#endif

   uhandle = proceedingp->handle;            /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( proceedingp->handle, &dc );

      switch(call_type(handle2net(proceedingp->handle)))
          {
#ifdef ACU_VOIP_CC
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
#endif

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
#ifdef ACU_VOIP_CC
         /* load default codecs here */  /* is this still required was removed in DLL code. */
         for ( i = 0; i < MAXCODECS; i++ )
            {
            proceedingp->unique_xparms.sig_h323.codecs[i] = default_codecs[i];
            /* 0 signifies end of codec list so stop assignment here for efficiency */
            if (default_codecs[i] == 0 )
               break;
            }
#endif
         proceedingp->handle = dc.handle;

#ifndef ACU_VOIP_CC
         if ( 0 )
            {
#else
         if (dc.card >= 0 && clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* If VoIP card then thread identification is required */
            vcc_thread_get_id(&this_thread);
            proceedingp->unique_xparms.sig_h323.calling_thread = this_thread; /* assign thread id */

            dc.result = voipioctl ( CALL_PROCEEDING,
                                  (IOCTLU *) proceedingp,
                                   clcard[dc.card].clh,
                                   sizeof ( PROCEEDING_XPARMS ),
                                   card_2_voipcard(dc.card));

#endif
            }
         else
            {
            dc.result = clioctl ( CALL_PROCEEDING,
                                  (IOCTLU *) proceedingp,
                                   clcard[dc.card].clh,
                                   sizeof ( PROCEEDING_XPARMS ));
            }
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
#ifdef ACU_VOIP_CC
   ACU_VCC_THREAD_ID this_thread;               /* VoIP specific */
#endif

   uhandle = setup_ackp->handle;            /* save user handle */

   dc.result = clopendev ( );

   if ( dc.result == 0 )
      {
      handle_decomp ( setup_ackp->handle, &dc );

      switch(call_type(handle2net(setup_ackp->handle)))
         {
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

#ifndef ACU_VOIP_CC
         if ( 0 )
            {
#else
         if (dc.card >= 0 && clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* If VoIP card then thread identification is required */
            vcc_thread_get_id(&this_thread);
            setup_ackp->unique_xparms.sig_h323.calling_thread = this_thread; /* assign thread id */

            dc.result = voipioctl ( CALL_SETUP_ACK,
                                    (IOCTLU *) setup_ackp,
                                    clcard[dc.card].clh,
                                    sizeof ( SETUP_ACK_XPARMS ),
                                    card_2_voipcard(dc.card));
#endif
            }
         else
            {
            dc.result = clioctl ( CALL_SETUP_ACK,
                                  (IOCTLU *) setup_ackp,
                                  clcard[dc.card].clh,
                                  sizeof ( SETUP_ACK_XPARMS ));
            }
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

#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif

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
                                   clcard[dc.card].clh,
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

   unet = calloutp->net;            /* save user net */

   calloutp->cnf |= CNF_ENABLE_V5API;     /* Enable V5 drivers events & features not found with V4 drivers */

   result = clopendev ( );

   if ( result == 0 )
      {
      /* Translate user's bearer network to actual signalling network... */
      /* and associated bearer network port...                           */
      ACU_INT assoc_net = xlate_assoc(&calloutp->net) ;
      ACU_INT sig_net  = calloutp->net ;	/* Fixed up by xlate_assoc */

      card = unet_2_card ( sig_net );        /* get the card number */

      if ( card >= 0 )                    /* check for error */
         {

#ifdef ACU_VOIP_CC
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }
#endif

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
                  result = clioctl ( mode,
                                    (IOCTLU *) calloutp,
                                     clcard[card].clh,
                                     sizeof (FEATURE_OUT_XPARMS));

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

#ifdef ACU_VOIP_CC
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }
#endif

         detailsp->handle = dc.handle;       /* get the fixed up handle */
         if(detailsp->feature_type == 0)
            {
            dc.result = ERR_PARM;
            }
         if ( dc.result >= 0 )               /* check for error */
            {
            dc.result = clioctl ( CALL_FEATURE_DETAILS,
                               (IOCTLU *) detailsp,
                                clcard[dc.card].clh,
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

#ifdef ACU_VOIP_CC
         if (clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }
#endif

         detailsp->handle = dc.handle;       /* get the fixed up handle */
         switch(detailsp->feature_type)
         {
            case 0:
            case FEATURE_HOLD_RECONNECT:
            case FEATURE_TRANSFER:
            case FEATURE_DIVERSION:
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
            default:
               dc.result = ERR_PARM;
               break;
         }

         if ( dc.result >= 0 )                  /* check for error */
            {
            dc.result = clioctl ( CALL_FEATURE_SEND,
                                  (IOCTLU *) detailsp,
                                   clcard[dc.card].clh,
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

#ifdef ACU_VOIP_CC
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }
#endif

         fdetailsp->net = unet_2_net ( unet ); /* and the network port */

         if ( fdetailsp->net >= 0 )     /* check for error */
            {
                  switch(fdetailsp->feature_type)
                     {
                     case FEATURE_FACILITY:
                         if( (fdetailsp->feature.facility.length > MAXFACILITY_INFO)
                                || (fdetailsp->feature.facility.length == 0) )
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
                               clcard[card].clh,
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

#ifdef ACU_VOIP_CC
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }
#endif

         fdetailsp->net = unet_2_net ( unet ); /* and the network port */

         if ( fdetailsp->net >= 0 )     /* check for error */
            {
            result = clioctl ( CALL_GET_CONNECTIONLESS,
                               (IOCTLU *) fdetailsp,
                               clcard[card].clh,
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
#ifdef ACU_VOIP_CC
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }
#endif

         alarmp->net = unet_2_net ( unet ); /* and the network port */

         if ( alarmp->net >= 0 )        /* check for error */
            {
            result = clioctl ( CALL_SEND_ALARM,
                               (IOCTLU *) alarmp,
                               clcard[card].clh,
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
#ifdef ACU_VOIP_CC
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }
#endif

         q921p->net = unet_2_net ( unet ); /* and the network port */

         if ( q921p->net >= 0 )        /* check for error */
            {

            result = clioctl ( CALL_SEND_Q921,
                               (IOCTLU *) q921p,
                               clcard[card].clh,
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
#ifdef ACU_VOIP_CC
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }
#endif

         q921p->net = unet_2_net ( unet ); /* and the network port */

         if ( q921p->net >= 0 )        /* check for error */
            {

               result = clioctl ( CALL_GET_Q921,
                               (IOCTLU *) q921p,
                                clcard[card].clh,
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
#ifdef ACU_VOIP_CC
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }
#endif
         watchp->net = unet_2_net ( unet ); /* and the network port */

         if ( watchp->net >= 0 )        /* check for error */
            {
            result = clioctl ( CALL_WATCHDOG,
                               (IOCTLU *) watchp,
                               clcard[card].clh,
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
#ifdef ACU_VOIP_CC
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }
#endif

         dprp->net = unet_2_net ( unet ); /* and the network port */

         if ( dprp->net >= 0 )        /* check for error */
            {
            result = clioctl ( SEND_DPR_COMMAND,
                               (IOCTLU *) dprp,
                               clcard[card].clh,
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
#ifdef ACU_VOIP_CC
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }
#endif

         dprp->net = unet_2_net ( unet ); /* and the network port */

         if ( dprp->net >= 0 )        /* check for error */
            {
            result = clioctl ( RECEIVE_DPR_EVENT,
                               (IOCTLU *) dprp,
                               clcard[card].clh,
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
#ifdef ACU_VOIP_CC
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }
#endif

         l1_xstatsp->net = unet_2_net ( unet ); /* and the network port */

         if ( l1_xstatsp->net >= 0 )     /* check for error */
            {
            result = clioctl ( CALL_L1_STATS,
                               (IOCTLU *) l1_xstatsp,
                               clcard[card].clh,
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
#ifdef ACU_VOIP_CC
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }
#endif

         l2_xstatep->net = unet_2_net ( sig_net ); /* and the network port */

         if ( l2_xstatep->net >= 0 )    /* check for error */
            {
            /* We encode associated bearer net in upper bits of sig_net... */
            l2_xstatep->net |= ((assoc_net +1) << 8) ;

            result = clioctl ( CALL_L2_STATE,
                               (IOCTLU *) l2_xstatep,
                               clcard[card].clh,
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

#ifdef ACU_VOIP_CC
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }
#endif

         br_l1_xstatsp->net = unet_2_net ( unet ); /* and the network port */

         if ( br_l1_xstatsp->net >= 0 )  /* check for error */
            {
            result = clioctl ( CALL_BR_L1_STATS,
                               (IOCTLU *) br_l1_xstatsp,
                               clcard[card].clh,
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

#ifdef ACU_VOIP_CC
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }
#endif

         dcbap->net = unet_2_net ( unet ); /* and the network port */

         if ( dcbap->net >= 0 )    /* check for error */
            {
            result = clioctl ( CALL_DCBA,
                               (IOCTLU *) dcbap,
                               clcard[card].clh,
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
#ifdef ACU_VOIP_CC
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }
#endif

         logp->net = unet_2_net ( unet ); /* and the network port */

         if ( logp->net >= 0 )    /* check for error */
            {
            result = clioctl ( CALL_PROTOCOL,
                               (IOCTLU *) logp,
                               clcard[card].clh,
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
#ifdef ACU_VOIP_CC
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }
#endif

         pblockp->net = unet_2_net ( unet ); /* and the network port */

         if ( pblockp->net >= 0 )    /* check for error */
            {
            result = clpblock_ioctl ( CALL_V5PBLOCK,
                                     (V5_PBLOCK_IOCTLU *) pblockp,
                                      clcard[card].clh,
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
#ifdef ACU_VOIP_CC
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            /* command not supported for VoIP */
            return (ERR_COMMAND);
            }
#endif
         tcmdp->net = unet_2_net ( unet ); /* and the network port */

         if ( tcmdp->net >= 0 )    /* check for error */
            {
            result = clioctl ( CALL_TCMD,
                               (IOCTLU *) tcmdp,
                               clcard[card].clh,
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

#ifdef ACU_VOIP_CC
   ACU_VCC_THREAD_ID this_thread;
#endif


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

#ifndef ACU_VOIP_CC
            if ( 0 )
               {
#else
            if (card >= 0 && clcard[card].voipservice == ACU_VOIP_ACTIVE)
               {
               vcc_thread_get_id(&this_thread);
               pdsp->unique_xparms.sig_h323.calling_thread = this_thread;

               result = voipioctl ( CALL_DSP_CONFIG,
                                    (IOCTLU *) pdsp,
                                    clcard[card].clh,
                                    sizeof (DSP_XPARMS),
                                    card_2_voipcard(card));
#endif
               }
            else
               {
               result = clioctl ( CALL_DSP_CONFIG,
                                  (IOCTLU *) pdsp,
                                  clcard[card].clh,
                                  sizeof (DSP_XPARMS));
               }
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


#ifdef ACU_VOIP_CC

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
   ACU_VCC_THREAD_ID this_thread;           /* VoIP specific */

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
            vcc_thread_get_id(&this_thread);
            pdsp->unique_xparms.sig_h323.calling_thread = this_thread;
            }
         else
            {
            return (ERR_COMMAND);
            }

         pdsp->net = unet_2_net ( unet ); /* and the network port */

         if ( pdsp->net >= 0 )    /* check for error */
            {
            result = voipioctl ( CALL_GET_CONFIGURATION,
                              (IOCTLU *) pdsp,
                               clcard[card].clh,
                               sizeof (GET_CONFIGURATION_XPARMS),
                               card_2_voipcard(card));
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
/*-----------------------------------------*/

/*--------- call_set_configuration --------*/
/* Set board configuration                 */
/*                                         */
ACUDLL int call_set_configuration ( struct set_configuration_xparms * pdsp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;
   ACU_VCC_THREAD_ID this_thread;           /* VoIP specific */

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
            vcc_thread_get_id(&this_thread);
            pdsp->unique_xparms.sig_h323.calling_thread = this_thread;
            }
         else
            {
            return ( ERR_COMMAND);
            }

         pdsp->net = unet_2_net ( unet ); /* and the network port */
         pdsp->board_ip_address = clcard[card].board_ip_address;

         if ( pdsp->net >= 0 )    /* check for error */
            {
            result = voipioctl ( CALL_SET_CONFIGURATION,
                              (IOCTLU *) pdsp,
                               clcard[card].clh,
                               sizeof (SET_CONFIGURATION_XPARMS),
                               card_2_voipcard(card));
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



/*-------------- call_get_stats-----------*/
/* get board stats                        */
/*                                        */
ACUDLL int call_get_stats ( struct voip_get_stats_xparms * pdsp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;
   ACU_VCC_THREAD_ID this_thread;           /* VoIP specific */

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
            vcc_thread_get_id(&this_thread);
            pdsp->unique_xparms.sig_h323.calling_thread = this_thread;
            }
         else
            {
            return ( ERR_COMMAND);
            }

         pdsp->net = unet_2_net ( unet ); /* and the network port */
         pdsp->board_ip_address = clcard[card].board_ip_address;

         if ( pdsp->net >= 0 )    /* check for error */
            {
            result = voipioctl ( CALL_GET_STATS,
                               (IOCTLU *) pdsp,
                               clcard[card].clh,
                               sizeof (VOIP_GET_STATS_XPARMS),
                               card_2_voipcard(card));
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


#ifdef ACU_VOIP_CC

/* VOIP only */
/*-------------- call_set_debug------------*/
/* get board configuration                 */
/*                                         */
ACUDLL int call_set_debug ( struct set_debug_xparms * pdsp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;
   ACU_VCC_THREAD_ID this_thread;           /* VoIP specific */

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
            vcc_thread_get_id(&this_thread);
            pdsp->unique_xparms.sig_h323.calling_thread = this_thread;
            }
         else
            {
            return (ERR_COMMAND);
            }

         pdsp->net = unet_2_net ( unet ); /* and the network port */
         pdsp->board_ip_address = clcard[card].board_ip_address;

         if ( pdsp->net >= 0 )    /* check for error */
            {
            result = voipioctl ( CALL_SET_DEBUG,
                                 (IOCTLU *) pdsp,
                                 clcard[card].clh,
                                 sizeof (SET_DEBUG_XPARMS),
                                 card_2_voipcard(card));
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
#endif



/*-------------- call_open_voip_admin_chan---------*/
/* Opens the VoIP administration channel           */
/*                                                 */
ACUDLL int call_open_voip_admin_chan()
   {
   acu_error result;
   ACU_VCC_THREAD_ID this_thread;

   vcc_thread_get_id( &this_thread );

   result = create_admin_channel( &admin_channel_thread, &this_thread);

   if (result != ACU_ERR_OK)
      {
      return ERR_CFAIL;
      }
   else
      {
      return 0;
      }
   }
/*--------------------------------------------------*/

/*-------------- call_close_voip_admin_chan---------*/
/* Close the VoIP administration channel            */
/*                                                  */
ACUDLL int call_close_voip_admin_chan()
{
   acu_error result;
   ACU_VCC_THREAD_ID this_thread;
   generic_tls_msg *admin_msg = NULL;

   vcc_thread_get_id(&this_thread);


   /* send msg to admin chan to stop thread */
   result = api_admin_chan_send_msg(API_AC_STOP,
                                    NULL,
                                   &admin_channel_thread,
                                   &this_thread);

   if (result != ACU_ERR_OK)
      {
      result = ERR_CFAIL;
      }
   else
      {
      /* wait for acknowledgement of sent message */
      api_admin_chan_wf_msg(admin_msg, &this_thread);
      if (admin_msg->type != API_AC_SEND_MSG_ACK)
         {
         result = ERR_CFAIL;
         }
      else
         {
         result = 0; /* success */
         }
      free (admin_msg);
      }

   return (result);

}
/*---------------------------------------*/

/*--------------- call_send_voip_admin_msg ----------------*/
/* Send a VoIP administration message to the admin channel */
/*                                                         */
ACUDLL int call_send_voip_admin_msg( struct voip_admin_out_xparms * adminp)
{
   acu_error result;
   ACU_VCC_THREAD_ID this_thread;

   vcc_thread_get_id(&this_thread);

   if (admin_channel_thread.id)
      {
      /* VoIP admin channel exists so retrieve message */
      adminp->calling_thread = this_thread;
      result = process_voip_admin_msg(API_SEND_RAS_MSG, (IOCTLU *)adminp);
      }
   else
      {
      /* admin channel does not exist */
      ACU_LOG(call_get_voip_admin_msg, warn,
             ("VoIP admin channel has not been opened so cannot send message\n"));
      result = ERR_CFAIL;
      }

   return (result);

}
/*---------------------------------------------------------*/

/*------------------ call_get_voip_admin_msg --------------------*/
/* Retrieve a VoIP administration message from the admin channel */
/*                                                               */
ACUDLL int call_get_voip_admin_msg( struct voip_admin_in_xparms * adminp)
{
   acu_error result;
   ACU_VCC_THREAD_ID this_thread;

   vcc_thread_get_id(&this_thread);

   if (admin_channel_thread.id)
      {
      /* VoIP admin channel exists so retrieve message */
      adminp->calling_thread = this_thread;
      result = process_voip_admin_msg(API_GET_RAS_MSG, (IOCTLU *)adminp);

      }
   else
      {
      /* admin channel does not exist */
      ACU_LOG(call_get_voip_admin_msg, warn,
              ("VoIP admin channel has not been opened so cannot retrieve message\n"));
      result = ERR_CFAIL;
      }
   return (result);
}
/*---------------------------------------------------------*/

/*---------------process_voip_admin_msg -------------------*/
/* send supplied message to VoIP administration channel    */
/*                                                         */
static int process_voip_admin_msg (uint32 msg_type, IOCTLU *pioctlu)
{
   acu_error result;
   ACU_VCC_THREAD_ID this_thread;
   generic_tls_msg admin_msg;

   vcc_thread_get_id(&this_thread);

   /* send message to admin channel */
   result = api_admin_chan_send_msg( msg_type,
                                     pioctlu,
                                    &admin_channel_thread,
                                    &this_thread );

   if (result != ACU_ERR_OK)
      {
      result = ERR_CFAIL;
      }
   else
      {
      /* wait for acknowledgement of sent message */
      api_admin_chan_wf_msg(&admin_msg, &this_thread);
      if (admin_msg.type != API_AC_SEND_MSG_ACK)
         {
         result = ERR_CFAIL;
         }
      else
         {
         result = 0; /* success */
         }
      }
   return (result);
}
/*-----------------------------------------------------------*/

#endif  /* ACU_VOIP_CC */

/*-------------- call_sfwm --------------*/
/* start the firmware after a download   */
/*                                       */

ACUDLL int call_sfmw ( int portnum, int mode, char * confp )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  unet;
   SFMW_XPARMS sfmw;



   unet = (ACU_INT) portnum;

   result = clopendev ( );              /* open the device */

   if ( result == 0 )                   /* check the result */
      {
      card = unet_2_card ( unet );      /* get the card number */

      if ( card >= 0 )                  /* check for error */
         {
#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif
         unet = unet_2_net ( unet );    /* and the network port */

         if ( unet >= 0 )               /* check for error */
            {
            sfmw.net     = mode | unet;
            sfmw.bufp_confstr = confp ;

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
                               clcard[card].clh,
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
   ACU_INT  result;
   struct download_xparms download;

#ifdef ACU_VOIP_CC
   char ip_address1[IP_ADDRESS_SIZE];
   char subnet_address1[IP_ADDRESS_SIZE];
   char ip_address2[IP_ADDRESS_SIZE];
   char subnet_address2[IP_ADDRESS_SIZE];
   char gateway_address[IP_ADDRESS_SIZE];
   V1BMI_BOARD_ADDR_PARMS vbmi_parms;
   int card;
   V1BMI_RESTART_PARMS vbmi_rstrt_parms;
   struct set_sysinfo_xparms set_sysinfo;
   struct sysinfo_xparms     sysinfo;
   int using_dhcp;
   V1BMI_DEBUG_MODE_PARMS debug;
#endif


#ifndef ACU_VOIP_CC
   if ( 0 )
      {
#else
   if (clcard[unet_2_card (restartp->net)].voipservice == ACU_VOIP_ACTIVE)
      {
      /* call function which extracts ip addresses from config string */
      result = parse_config_str(restartp->config_stringp,
                                ip_address1,
                                ip_address2,
                                subnet_address1,
                                subnet_address2,
                                gateway_address,
                               &using_dhcp);

      set_sysinfo.net = restartp->net;
      set_sysinfo.board_number = unet_2_card (restartp->net-1);
      strcpy(set_sysinfo.board_ip_address , ip_address1);
      call_set_system_info ( &set_sysinfo, &sysinfo );
      strcpy(clcard[unet_2_card (restartp->net)].board_ip_address,ip_address1);

      if (( ip_address1[0] == 0) || ( subnet_address1[0] == 0) || ( gateway_address[0] == 0))
      /* if not found three ip address's and DHCP then exit */
         {
         ACU_LOG(call_restart_fmw, warn, ("call_restart_fmw  not provided with enough config data \n"));
         return 0;
         }

      sscanf(ip_address1,
             "%d.%d.%d.%d",
             &vbmi_parms.ip_addr1[0],
             &vbmi_parms.ip_addr1[1],
             &vbmi_parms.ip_addr1[2],
             &vbmi_parms.ip_addr1[3]);

      sscanf(ip_address2,
             "%d.%d.%d.%d",
             &vbmi_parms.ip_addr2[0],
             &vbmi_parms.ip_addr2[1],
             &vbmi_parms.ip_addr2[2],
             &vbmi_parms.ip_addr2[3]);

      sscanf(subnet_address1,
             "%d.%d.%d.%d",
             &vbmi_parms.subnet_mask1[0],
             &vbmi_parms.subnet_mask1[1],
             &vbmi_parms.subnet_mask1[2],
             &vbmi_parms.subnet_mask1[3]);

      sscanf(subnet_address2,"%d.%d.%d.%d",
             &vbmi_parms.subnet_mask2[0],
             &vbmi_parms.subnet_mask2[1],
             &vbmi_parms.subnet_mask2[2],
             &vbmi_parms.subnet_mask2[3]);

      sscanf(gateway_address,"%d.%d.%d.%d",
             &vbmi_parms.default_gateway[0],
             &vbmi_parms.default_gateway[1],
             &vbmi_parms.default_gateway[2],
             &vbmi_parms.default_gateway[3]);

      vbmi_parms.get_ip_addrs_using_dhcp = using_dhcp;

      card = unet_2_card (restartp->net-1);
      vbmi_rstrt_parms.filename = restartp->filenamep;

      result = v1bmi_set_board_addr(card,&vbmi_parms);

      if (result != 0)
         {
         ACU_LOG(session, critical, ("Failed to send board config info to driver \n"));
         return(result);
         }

      debug.debug_mode = 4;
      v1bmi_set_debug_mode( card, &debug );

      result = v1bmi_restart(card,&vbmi_rstrt_parms);

      if (result != 0)
         {
         ACU_LOG(session, critical, ("Failed to download board software \n"));
         return(result);
         }

      return ( result );

#endif
      }
   else
      {

      download.net = restartp->net;
      download.filenamep = restartp->filenamep;

      if ( verbose ) printf ( "\r\nResetting Card ..." );

      (void)config_assoc_net(restartp->net,0) ;	/* CLear any NFAS-style associations */

      result = call_sfmw ( restartp->net,
                           SFMWMODE_RESTART,
                           restartp->config_stringp ); /* reset the card */

      if ( result == 0 )
         result = call_download_fmw ( &download );  /* transfer the file */

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
#ifndef ACU_VOIP_CC
         if ( 0 )
            {
#else
         if (dc.card >= 0 && clcard[dc.card].voipservice == ACU_VOIP_ACTIVE)
            {
            result = handle2net( (ACU_INT) handle);
#endif
            }
         else
            {
            h2p.handle = dc.handle ;

            /* The following IOCTL may fail either if the driver is unable to
               perform the conversion on the supplied netwok, in which case the
               simple library translation will suffice */

            result = clioctl ( CALL_HANDLE_2_PORT,
                                  (IOCTLU *) &h2p,
                                  clcard[dc.card].clh,
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
#ifdef ACU_VOIP_CC
    int card;
	V1BMI_RESTART_PARMS vbmi_rstrt_parms;
	char * board_ip_address;
#endif

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
#ifdef ACU_VOIP_CC
         case C_VOIP:
            card = unet_2_card (download_xparmsp->net-1);

            vbmi_rstrt_parms.filename = download_xparmsp->filenamep;
            error = v1bmi_restart(card,&vbmi_rstrt_parms);
            if (error != 0)
               {
               ACU_LOG(session, critical, ("Failed to download board software \n"));
               return(error);
               }

            board_ip_address = clcard[unet_2_card (download_xparmsp->net)].board_ip_address;

         break;
#endif
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
                                                clcard[card].clh,
                                                sizeof(V5_PBLOCK_XPARMS) );

                        while ((error == 0) && ((size != 0) || (pblock_xparms->len == 0)))
                           {
                           pblock_xparms->len = size;

                           error = clpblock_ioctl( CALL_BRDSPBLOCK,
                                                   (V5_PBLOCK_IOCTLU *) pblock_xparms,
                                                   clcard[card].clh,
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
                                                     clcard[card].clh,
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
   int  error;
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


   result = clopendev ( );                 /* open the device */

   if ( result == 0 )
      {
      card = unet_2_card ( tracep->unet );  /* get the card number */

#ifdef ACU_VOIP_CC
         /* command not supported for VoIP */
         if (clcard[card].voipservice == ACU_VOIP_ACTIVE)
            {
            return (ERR_COMMAND);
            }
#endif

      if ( card >= 0 )                     /* check for error */
         {
         result = clioctl ( CALL_TRACE,
                            (IOCTLU *) tracep,
                            clcard[card].clh,
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

#ifdef ACU_VOIP_CC
         result = clcard[i].v1bmi_card_num;
#else
         result = i;                    /* the index is the card number */
#endif
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
#ifdef ACU_VOIP_CC
            case C_VOIP:
                net = 48 + net;                /* virtual stream number */
            break;
#endif
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

#ifdef ACU_VOIP_CC

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
#endif
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

#ifndef ACU_VOIP_CC
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
#endif

/*------------ update_ss_type -----------*/
/* update the signalling system types    */
/*                                       */

ACUDLL int update_ss_type ( ACU_INT portnum )
   {
   ACU_INT  result;
   ACU_INT  card;
   ACU_INT  net;
   SFMW_XPARMS sfmw;
#ifdef ACU_VOIP_CC
   ACU_VCC_THREAD_ID this_thread;           /* VoIP specific */
#endif


   card = unet_2_card ( portnum );      /* get the card number */

   net = unet_2_net ( portnum );        /* and the network port */

   sfmw.net        = SFMWMODE_SSTYPE | net;
   sfmw.confstr[0] = '\0';
   sfmw.regapp     = 0;
   sfmw.ss_vers    = 0;

#ifndef ACU_VOIP_CC
   if ( 0 )
      {
#else
      if (card >= 0 && clcard[card].voipservice == ACU_VOIP_ACTIVE)
         {
         /* If VoIP card then thread identification is required */
         vcc_thread_get_id(&this_thread);
         sfmw.unique_xparms.sig_h323.calling_thread = this_thread; /* assign this thread id */

         result = voipioctl ( CALL_SFMW,
                              (IOCTLU *) &sfmw,
                              clcard[card].clh,
                              sizeof (SFMW_XPARMS),
                              card_2_voipcard(card));
#endif
         }
      else
         {
         result = clioctl ( CALL_SFMW,
                            (IOCTLU *) &sfmw,
                            clcard[card].clh,
                            sizeof (SFMW_XPARMS));
         }


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
   ACU_INT  result;
   int  i;
   int  addrnum;
   char cext;
   SFMW_XPARMS sfmw;
   struct siginfo_xparms   lsig[MAXPPC];
   struct siginfo_xparms * lsigp;
   char   cldevname[16];

#ifdef ACU_VOIP_CC
   /* VoIP specific */
   int                   voip_card_number;    /* numbers voip cards 1,2,3 etc. */
   acu_error             vresult;
   ACU_VCC_THREAD_ID     this_thread;
   tls_test_thread_info  *thread_info;
   void                  *conn_mgr = NULL;
   int                   rc;
   V1BMI_CARD_INFO_PARMS cardInfoParms;
   int                   nvcards = 0;

#endif

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

      addrnum = strlen ( cldevname ) - 1;        /* position to address digit */

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
                            clcard[i].clh,
                            sizeof (SFMW_XPARMS ));



         result = clioctl ( CALL_SIGNAL_INFO,
                            (IOCTLU *) lsigp,
                            clcard[i].clh,
                            sizeof (SIGINFO_XPARMS) * MAXPPC );





         if ( result == 0 )
            {
            /* move the info pointer by the number */
            /* of networks on the card             */

            clcard[i].nnets = lsigp->nnets;   /* register number of networks */
            tnets += lsigp->nnets;            /* count total networks        */
#ifdef ACU_VOIP_CC
            clcard[i].v1bmi_card_num = i;     /* store the driver and switch number */
#endif
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
#ifdef ACU_VOIP_CC
      call_driver_installed = FALSE;
#endif
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
                            clcard[0].clh,
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
                            clcard[0].clh,
                            sizeof (event_if_xparms));
            if (result != 0)
                break ;
            }
         }
      }
#endif


#ifdef ACU_VOIP_CC
   /****************************************************************************/
   /*
    * Carry out thread initialisation rqd for VoIP.  This must
    * be performed at the earliest opportunity.
    */

   vcc_thread_get_id(&this_thread);  /* obtain this thread id */

   /* create and initialise VoIP call control components      */
   /* create thread_info and initialise memory region to zero */

   thread_info = malloc(sizeof(tls_test_thread_info));
   memset(thread_info, 0, sizeof(tls_test_thread_info));

   vresult = create_call_records(thread_info);

   if ( vresult != ACU_ERR_OK )
      {
      ACU_LOG(opendevdrv, critical, ("failed to create call records\n"));
      return(ERR_CFAIL);
      }

   /* assign thread id and handle for this thread to thread_info
    * where this thread represents the application thread
    */

   vcc_thread_get_id(&thread_info->this_thread);


#if CALL_CONTROL_LOG_THREAD

   /*
    * create Log Thread
    */

   cc_log_init();
   while (log_thread_created() == FALSE)
      {
      Sleep(10);
      }; /* wait for logging to start */
#endif

   /* Now go looking for voip drivers. If one is found add to clcards in the next position. */
   ACU_DEBUG_LOG(opendevdrv,("Creating Generic/Thin Layer thread\n"));
   first_voip_card = -1;

   for ( nvcards = 0; nvcards < NCARDS; nvcards++ )
      {
      rc = v1bmi_card_info(nvcards,&cardInfoParms);

      /* if a voip card load up clcard and start the tls threads and pipe_threads for that thread */
      if (rc == 0)
         {
         if ( cardInfoParms.ip_is_configured == 0) /* board software not loaded so fail.*/
            {
            ACU_LOG(ServiceStart, critical,
                    ("Software not loaded for driver: %d  board: %d \n", nvcards,nvcards+1));
            return(ERR_BOARD_UNLOADED);
            }

         if (first_voip_card == -1)
            first_voip_card = ncards;

         clcard[ncards].nnets = 1;            /* register 1 network port for VOIP */
         clcard[ncards].types[0] = S_H323;    /* assign signalling type to port 0 */
         clcard[ncards].lines[0] = L_PSN;
         clcard[ncards].version  = 0x520;
         clcard[ncards].v1bmi_card_num = nvcards;  /* store the driver and switch number */
         clcard[ncards].voipservice = ACU_VOIP_ACTIVE;

         /*
          * create Generic/TLS threads
          */

         voip_card_number = ncards-first_voip_card+1;

         done_init[voip_card_number] = FALSE;
         vresult = generic_tls_thread_init(&generic_tls_thread[voip_card_number], &this_thread);

         if ( vresult != ACU_ERR_OK )
            {
            ACU_LOG(opendevdrv, critical, ("failed to launch generic/tls thread %d\n",voip_card_number));
            return(ERR_CFAIL);
            }

         /* block until message is received on this thread */
         generic_tls_wf_init_complete(&this_thread);

         /* send SMFW message to stop ERR_DRV_INCOMPAT errors */

         sfmw.net     = SFMWMODE_REGISTER;
         sfmw.confstr[0] = '\0';
         sfmw.regapp  = REGISTER_XAPI;      /* register extended API */
         sfmw.ss_vers = 0;

         vcc_thread_get_id(&this_thread);
         sfmw.unique_xparms.sig_h323.calling_thread = this_thread;

         result = voipioctl ( CALL_SFMW,
                              (IOCTLU *) &sfmw,
                              clcard[ncards].clh,
                              sizeof (SFMW_XPARMS ),
                              voip_card_number);
         /*
          * create pipe client thread - used by service
          */

         vresult = create_pipe_client_thread(&pipe_client_thread[voip_card_number],voip_card_number); /* pipes number from 1 */

         if ( vresult != ACU_ERR_OK )
            {
            ACU_LOG(opendevdrv, critical, ("failed to launch pipe client thread %d\n",voip_card_number));
            return(ERR_CFAIL);
            }

         clopened = TRUE;                /* library opened */
         ncards++;
         tnets+=1;                       /* increment total number of ports by one */
         }  /* end of if (rc==0) */
      else  /* no voip driver for this position. If we have already found a voip card then we */
         {    /* have found all the voip cards */
         if (first_voip_card != -1) break;
         }


      /* dont allow more cards than we can cope with. */
      if (ncards == MAX_NUM_BOARDS+first_voip_card) break;

      } /* end of for loop */


    /* assign generic_tli_thread id from newly created
    * generic/tls array - rqd by create_h323_thread
    */
   if (first_voip_card == -1)    /* no voip card found */
      {
      ACU_LOG(opendevdrv,critical,("No VOIP card found\n"));
      }

   ACU_DEBUG_LOG(opendevdrv, ("----- VoIP Thread Initialisation Complete -----\n"));

#endif  /* if ACU_VOIP_CC */

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
                               clcard[card].clh,
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
ACU_INT call_maintenance_cmd(ACUC_MAINTENANCE_XPARMS * maint)
   {
   ACU_INT result ;
   ACU_INT card ;

   result = clopendev ( );                    /* open the device */

   if ( result == 0 )
      {
      /* Fix up 'net' to signalling net, unet to user net  or -1 */
      maint->unet = xlate_assoc(&maint->net) ;

      /* The maintenance cmd is sent to the sig net, rather than unser net... */
      card = unet_2_card ( maint->net );            /* get the card number */
      if ( card >= 0 )                        /* check for error */
         {
         maint->net = unet_2_net ( maint->net ); /* and the network port */

         if ( maint->net >= 0 )            /* check for error */
            {
            result = clioctl ( CALL_MAINTENANCE,
                               (IOCTLU *) maint,
                               clcard[card].clh,
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

   return call_maintenance_cmd(&maint) ;
}
/*---------------------------------------*/


/*---------- port_blocking --------------*/
/* Protocol-block a group of timeslots   */
/*                                       */
ACUDLL ACU_INT ACU_WINAPI
call_maint_port_block ( PORT_BLOCKING_XPARMS * blockingp)
   {
   ACUC_MAINTENANCE_XPARMS maint ;

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

   return call_maintenance_cmd(&maint) ;
   }


/*---------- port_reset -----------------*/
/* Protocol-reset a goup of timeslots    */
/*                                       */
ACUDLL ACU_INT ACU_WINAPI
call_maint_port_reset ( PORT_RESET_XPARMS * resetp)
   {
   ACUC_MAINTENANCE_XPARMS maint ;

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

   return call_maintenance_cmd(&maint) ;
   }


/*---------- ts_blocking ----------------*/
/* Protocol-block a timeslot             */
/*                                       */
ACUDLL ACU_INT ACU_WINAPI
call_maint_ts_block ( TS_BLOCKING_XPARMS * blockingp)
   {
   ACUC_MAINTENANCE_XPARMS maint ;

   maint.net = blockingp->net ;
   maint.ts = blockingp->ts ;
   maint.flags = blockingp->flags ;
   maint.cmd = ACUC_MAINT_TS_BLOCK ;
   return call_maintenance_cmd(&maint) ;
   }


/*---------- ts_blocking ----------------*/
/* Protocol-block a timeslot             */
/*                                       */
ACUDLL ACU_INT ACU_WINAPI
call_maint_ts_unblock ( TS_BLOCKING_XPARMS * blockingp)

   {
   ACUC_MAINTENANCE_XPARMS maint ;

   maint.net = blockingp->net ;
   maint.ts = blockingp->ts ;
   maint.flags = blockingp->flags ;
   maint.cmd = ACUC_MAINT_TS_UNBLOCK ;
   return call_maintenance_cmd(&maint) ;
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
                               clcard[card].clh,
                               sizeof (ACUC_ASSOC_NET_XPARMS ));

               /* Clear the translation tables maintained in driver 0 */
               if (result == 0)
                  {
                  assocp->net = -1 ;	/* Indicates no snet for this unet */
                  assocp->mode = ACUC_ASSOC_SET ;
                  result = clioctl ( CALL_ASSOC_NET,
                               (IOCTLU *) assocp,
                               clcard[0].clh,
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

ACUDLL ACU_INT
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
                               clcard[card].clh,
                               sizeof (ACUC_ASSOC_NET_XPARMS ));

               /* Set the translation tables maintained in driver 0 */
               if (result == 0)
                  {
                  assocp->net = snet ;	/* Configure the new network params */
                  assocp->mode = ACUC_ASSOC_SET ;
                  result = clioctl ( CALL_ASSOC_NET,
                               (IOCTLU *) assocp,
                               clcard[0].clh,
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
                               clcard[card].clh,
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
/* returns either the original user net,  *
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
                clcard[0].clh,
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
                            clcard[card].clh,
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



/* This function starts the management and session RPC sessions */
/* if it doesnt succeed the first time it waits a second and tries again */
#ifdef ACU_VOIP_CC
int start_rpc_sessions ( char * board_ip_address , int card_num )
{
   int  error = 0;
   int attempt;
   acu_error result;

   ACU_DEBUG_LOG(Management, ("Initialising client side management library\n"));
   for (attempt=0;attempt<30;attempt++)
      {
      result = management_init(board_ip_address,card_num);
      if (result == 0)
         {
         ACU_DEBUG_LOG(Management, ("Management Server %s initialised\n", board_ip_address));
         break;
         }
      else if ((result == ACU_ERR_WRONG_BOARD_VER)||(result == ACU_ERR_NO_BOARD))
         {
         return result;
         }
      else
         Sleep(1000);
      }
   if (attempt == 30)
      {
      ACU_LOG(management, critical, ("Failed to connect to server%s\n", board_ip_address));
      error = ERR_MANAGEMENT_RPC;
      }
   return ( error );
}




/*--------------------------------------------------------------------------------------------------*/
/* this function searchs through the config string and extracts the ip_address, subnet_address and  */
/* gateway_address. It expects that they are preceded by the correct flags, are separated by a space*/
/* but that they are in no specific order                                                           */
/* config parameter options  -A1=  board address1, -A2=  board address2, -B1 subnet mask1,          */
/*                           -B2 subnet mask1, -C default gateway, -D use DHCP 1= yes               */
/*--------------------------------------------------------------------------------------------------*/
#ifdef ACU_VOIP_CC
int parse_config_str(char * config_string, char * ip_address1, char * ip_address2,
                     char * subnet_address1, char * subnet_address2,
                     char * gateway_address, int *Download_DHCP)
{
   int number_found;
   bool found;
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

#endif


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
      return ACU_ERR_OK;
      }
   else
      {
      return ACU_ERR_INVALID_ARGUMENT;
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
      return ACU_ERR_INVALID_ARGUMENT;
      }
}
#endif

/*------------ end of file --------------*/

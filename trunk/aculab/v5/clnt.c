/*------------------------------------------------------------*/
/* ACULAB plc                                                 */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : clnt.c                                 */
/*                                                            */
/*           Purpose : Operating System Specifics for Call    */
/*                     control library                        */
/*                                                            */
/*       Create Date : 19th October 1992                      */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Change History                                             */
/*                                                            */
/* rev:  v5.10.0     07/03/2003 labelled for V5.10.0 Release  */
/*                                                            */
/*------------------------------------------------------------*/

#ifndef WIN32_LEAN_AND_MEAN 
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <winioctl.h>
#include <process.h>

#include "ras_info.h"
#include "pipe_interface.h"
#include "mvcldrvr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>



#define FALSE  0
#define TRUE   1


/*---------- Local Function Prototypes ----------*/
/*                                               */
/* Many are operating system specific.           */
/*                                               */
/*-----------------------------------------------*/

int       clopen         ( char * );
ACU_INT   clioctl        ( ACU_INT  function, IOCTLU  *pioctl, ACU_INT card, ACU_INT unet, int len );
ACU_INT   clpblock_ioctl ( ACU_INT, V5_PBLOCK_IOCTLU *, int, int );
void      clclose        ( void );
void      clspecial      ( void );
int       clfileopen     ( char * );
int       clfileread     ( int, char *, unsigned int );
int       clfileclose    ( int );

ACU_INT   srvioctl       ( ACU_INT  function, IOCTLU  *pioctl, int len , int board_card_number, int voip_protocol);

static void PipeAdminThread(void* pv);
static int  init_pipe_admin_thread( int );
int         create_pipe_admin_thread( void );
int         stop_pipe_admin_thread( void );
int         pipe_client_send_application_terminated (ACU_INT board_card_number);

static ACU_INT is_pipe_running(int voip_protocol);
const  ACU_INT* get_voip_protocol_index_array( int* num_of_protocols );

extern int first_voip_card;


/*--------------- voip data+types ---------------*/
  
struct AdminThreadControl
{
    HANDLE hStartEvent;
    int pipe_index;
};

struct PipeControl
{
    HANDLE hUp;
    HANDLE hDown;
    HANDLE hDownLock;
    int    is_running;
};

#define H323_PIPE_INDEX 0
#define SIP_PIPE_INDEX 1
#define MAX_NUM_OF_PIPES 2

static struct PipeControl pipe_control[MAX_NUM_OF_PIPES] = {{0,0,0,0},{0,0,0,0}};

/*--------------- local data --------------------*/

char cldevname[] = { "\\\\.\\MVIP$SS0" };

/*------------- external data -------------------*/

extern int clopened;
extern int ncards;
extern CARD clcard[NCARDS];

extern void init_card_info ( int lcnum );
extern ACUDLL ACU_INT  card_2_voipcard ( ACU_INT  card );


/*------------ OS specifics -------------*/
/* Operating systems specific functions  */
/*---------------------------------------*/

/*----------------- clopen --------------*/
/* open the driver                       */
/*                                       */
int clopen (char * cldevnp )
   {
   HANDLE  clh;
   int result;

   SECURITY_ATTRIBUTES security_attributes;

   security_attributes.nLength = sizeof ( SECURITY_ATTRIBUTES );
   security_attributes.lpSecurityDescriptor = NULL;
   security_attributes.bInheritHandle = TRUE;

   clh = CreateFile ( cldevnp,
                      GENERIC_READ|GENERIC_WRITE,
                      FILE_SHARE_READ|FILE_SHARE_WRITE,
                     &security_attributes,
                      OPEN_EXISTING,
                      FILE_FLAG_OVERLAPPED,
                      NULL );

   if ( clh != INVALID_HANDLE_VALUE )
      {
      result = (int)clh;
      }
   else
      {
      result = -1;
      }

   return (result );
   }
/*---------------------------------------*/

/*----------------- cldev ---------------*/
/* return a pointer to the device name   */
/*                                       */
char * cldev ( )
   {
   return ( cldevname );
   }
/*---------------------------------------*/

/*--------------- clclose ---------------*/
/* close the driver                      */
/*                                       */
void clclose ( )
   {
   int  i, limit;
   int voipCardNo;

   if (first_voip_card >= 0) {
     /*
      * These are 1...rather than 0 enumerated
      */
     voipCardNo = 1;
     
     for (i = first_voip_card; i < ncards; i++) {
       pipe_client_send_application_terminated(voipCardNo);
       voipCardNo += 1;
     }

     stop_pipe_admin_thread();
   }

   if (first_voip_card >= 0) {
     limit = first_voip_card;
   }
   else {
     limit = ncards;
   }

   for ( i = 0; i < limit; i++ ) {
     CloseHandle ( (HANDLE)clcard[i].clh );
   }

   clopened = FALSE;

   }
/*---------------------------------------*/


/*---------------- clioctl --------------*/
/* call the nt ioctl function            */
/* return 0 if ok else return error      */
/*                                       */
ACU_INT clioctl ( ACU_INT  function, IOCTLU  *pioctl, ACU_INT card, ACU_INT unet, int len )
   {
   int        result;
   int        BytesReturned;
   int        i;
   int        clh;
   char      *pointer;

   OVERLAPPED overlapped;
   BOOL       complete;

   NTIOCTL    ntioctl;
   int        ioctlsize;

   /* Check for VoIP card */
   if (card >= 0 && clcard[card].voipservice == ACU_VOIP_ACTIVE) 
      {
      int voip_protocol;

      /* establish whether or not to call to the SIP or H323 service */
      switch (call_type(unet)) 
         {
         case S_H323:
            voip_protocol = H323_PIPE_INDEX;
         break;
         case S_SIP:
            voip_protocol = SIP_PIPE_INDEX;
         break;
         default:
            /* We've got a logic error somewhere */
            return ERR_NET;
         }

      result = srvioctl ( function,
                             pioctl,
                             len,
                             card_2_voipcard(card), voip_protocol);
      }
   else 
      {
      clh = clcard[card].clh;
     
      init_api_reg (&pioctl->api_reg, len);
     
      overlapped.hEvent = CreateEvent ( NULL,
                                       TRUE,
                                       FALSE,
                                       NULL
                                             );
     
      if ( overlapped.hEvent == NULL )
         {
         return (ACU_INT)ERR_CFAIL;
         }
     
      pointer = (char*) &ntioctl.ioctlu;
     
      ntioctl.command = function;
     
      for ( i=0; i<len; i++ )
         {
         pointer[i] = ((char*)pioctl)[i];
         }
     
      ntioctl.error = 0;
     
      ioctlsize = len + (3 * sizeof (int));     /* size of structures command, error, space, + ioctlu */
     
      result = DeviceIoControl (
                               (HANDLE) clh,
                               (DWORD)  CALL_IOCTL,
                              &ntioctl,
                               ioctlsize,
                              &ntioctl,
                               ioctlsize,
                               (unsigned long *)&BytesReturned,
                              &overlapped
                                          );
     
      WaitForSingleObject(
                         overlapped.hEvent,
                         INFINITE
                         );
     
      complete = GetOverlappedResult (
                                     (HANDLE)clh,
                                     &overlapped,
                                     (unsigned long *)&BytesReturned,
                                     TRUE
                                          );
   
      if ( !complete )
         {
         return (ACU_INT) ERR_CFAIL;
         }
     
      if ( !CloseHandle ( overlapped.hEvent ) )
         {
         return (ACU_INT) ERR_CFAIL;
         }
     
      for ( i=0; i<len; i++ )
         {
         ((char*)pioctl)[i] = pointer[i];
         }
     
      if ( result == TRUE )
         {
         result = ntioctl.error;
         }
      else
         {
         result = ERR_CFAIL;
         }

      }

   return ( (ACU_INT) result );
   }
/*---------------------------------------*/


/*-------------clpblock_ioctl -----------*/
/* call the nt ioctl function            */
/* return 0 if ok else return error      */
/*                                       */
ACU_INT clpblock_ioctl ( ACU_INT function, V5_PBLOCK_IOCTLU *pioctl, int card, int len)
   {
   int        result;
   int        BytesReturned;
   int        clh;

   OVERLAPPED overlapped;
   BOOL       complete;

   init_api_reg (&pioctl->api_reg, len);

   clh = clcard[card].clh;


   switch (function)
      {
      case CALL_V5PBLOCK:
      case CALL_BRDSPBLOCK:

      break;

      default:
         return (ACU_INT) ERR_COMMAND;
      break;
      }


   overlapped.hEvent = CreateEvent ( NULL,
                                     TRUE,
                                     FALSE,
                                     NULL
                                   );

   if ( overlapped.hEvent == NULL )
       {
       return (ACU_INT)ERR_CFAIL;
       }

   pioctl->pblock_xparms.command = function;
   pioctl->pblock_xparms.error = 0;

   result = DeviceIoControl (
                             (HANDLE) clh,
                             (DWORD)  CALL_PBLOCK_IOCTL,
                              pioctl,
                              sizeof ( V5_PBLOCK_IOCTLU ),
                              pioctl,
                              sizeof ( V5_PBLOCK_IOCTLU ),
                              (unsigned long *)&BytesReturned,
                              &overlapped
                            );

   WaitForSingleObject(
                       overlapped.hEvent,
                       INFINITE
                      );

   complete = GetOverlappedResult (
                                   (HANDLE)clh,
                                   &overlapped,
                                   (unsigned long *)&BytesReturned,
                                   TRUE
                                  );

   if ( !complete )
       {
       return (ACU_INT) ERR_CFAIL;
       }

   if ( !CloseHandle ( overlapped.hEvent ) )
       {
       return (ACU_INT) ERR_CFAIL;
       }

   if ( result == TRUE )
       {
       result = pioctl->pblock_xparms.error;
       }
   else
       {
       result = ERR_CFAIL;
       }

   return ( (ACU_INT) result );
   }
/*---------------------------------------*/


/*--------------- clfileopen ------------*/
/* open a disk file                      */
/*                                       */
int clfileopen ( char * fnamep )
   {
   return ( open ( fnamep, O_RDONLY + O_BINARY ));
   }
/*---------------------------------------*/



/*--------------- clfileread ------------*/
/* read a disk file                      */
/*                                       */
int clfileread ( int fh, char *buffp, unsigned len )
   {
   return ( read ( fh, buffp, len ));
   }
/*---------------------------------------*/

/*--------------- clfileclose -----------*/
/* open the driver                       */
/*                                       */
int clfileclose ( int fh )
   {
   return ( close ( fh ));  
   }
/*---------------------------------------*/

/*------------- clspecial ---------------*/
/* hook up to the exit list              */
/*                                       */
void clspecial ( )
   {

   }
/*---------------------------------------*/

#ifdef NT_WOS
/*--------------------------------------------------*/
/* Group of functions to manipulate NT wait objects */
/*                                                  */

/*------------- mvcl_ev_create ---------------*/
/* create NT wait object for global event     */
/*                                            */
int mvcl_ev_create ( tMVEventId *eventId )
   {
   int rc;
   tMVEventId	ev;
   char eventName[64];
   
   rc = 0;
   sprintf(&eventName[0],"%s",kMVNTEvBaseName );
   
   ev = (tMVEventId) OpenEvent(SYNCHRONIZE,FALSE,&eventName[0]);

   if ( ev==NULL)
      {
      rc = ERR_NO_SYS_RES;
      *eventId = 0;
      }
   else
      {
      *eventId = ev;
      rc = 0;
      }
   
   return rc;
   }
/*--------------------------------------------*/


/*------------- mvcl_l1_ev_create ---------------*/
/* create NT wait object for layer 1 change      */
/*                                               */
int mvcl_l1_ev_create (  tMVEventId *eventId )
   {
   int rc;
   tMVEventId     ev;
   char           eventName[64];
   rc = 0;
   
   sprintf(&eventName[0],"%s",kMVNTL1EvBaseName );

   ev = (tMVEventId) OpenEvent(SYNCHRONIZE,FALSE,&eventName[0]);
   if ( ev==NULL)
      {
      rc = ERR_NO_SYS_RES;
      *eventId = 0;
      }
   else
      {
      *eventId = ev;
      rc = 0;
      }
   return rc;
   }
/*-----------------------------------------------*/


/*------------- mvcl_l2_ev_create ---------------*/
/* create NT wait object for layer 2 change      */
/*                                               */
int mvcl_l2_ev_create (  tMVEventId *eventId )
   {
   int rc;
   tMVEventId	ev;
   char eventName[64];
   
   rc = 0;
   sprintf(&eventName[0],"%s",kMVNTL2EvBaseName );
   ev = (tMVEventId) OpenEvent(SYNCHRONIZE,FALSE,&eventName[0]);
   if ( ev==NULL)
      {
      rc = ERR_NO_SYS_RES;
      *eventId = 0;
      }
   else
      {
      *eventId = ev;
      rc = 0;
      }
   return rc;
}
/*-----------------------------------------------*/


/*------------- mvcl_ev_free -----------------*/
/* free NT wait object handle                 */
/*                                            */
int mvcl_ev_free ( tMVEventId *eventId )
   {
   if ( eventId != 0 )
      {
      CloseHandle ( eventId );
      }
   return 0;
   }
/*--------------------------------------------*/


/*------------- mvcl_ev_wait ---------------*/
/* wait for event or layer 1 change         */
/*                                          */
int mvcl_ev_wait ( tMVEventId *eventId )
   {
   WaitForSingleObject ( *eventId, INFINITE );
   return 0;
   }
/*------------------------------------------*/
#endif

/*
 * create_pipe_admin_thread : 
 *
 * Description : Create, initialise and start the pipe administration thread.
 * 
 * Output : 0 if OK, ERR_CFAIL if fail to acquire resource 
 *
 */
int create_pipe_admin_thread(void)
{
#ifdef _MT
   struct AdminThreadControl threadControl;
   HANDLE hStartupEvent;
#endif
   int voip_protocol;
   int voip_protocols_running=0;

#ifdef _MT
   if(0 == (hStartupEvent = CreateEvent(0, 0, 0, 0)))
     {
       return ERR_CFAIL;
     }

   memset(&threadControl, 0, sizeof(struct AdminThreadControl));
   threadControl.hStartEvent = hStartupEvent;
#endif
 
   /* 
    * Connect to any services.
    */
   for(voip_protocol = 0; voip_protocol < MAX_NUM_OF_PIPES; voip_protocol++)
      {
#ifdef _MT
	threadControl.pipe_index = voip_protocol;  
        if(-1 == _beginthread(PipeAdminThread, 0, &threadControl))
           {
	     /* oh dear */
	     CloseHandle(hStartupEvent);
	     return ERR_CFAIL;
           }

	/* sit around until the (H323/SIP)PipeAdminThread has initialised */
	WaitForSingleObject(hStartupEvent, INFINITE);
#else
	/* Ignore the return code - we check elsewhere to see if it's open */
	init_pipe_admin_thread(voip_protocol);
#endif
       }

#ifdef _MT
    CloseHandle(hStartupEvent);
#endif

    return 0;
}

/*
 * stop_pipe_admin_thread : 
 *
 * Description : Close all pipes and terminate the pipe administration thread.
 * 
 * Output : 0 if OK, ERR_CFAIL if fail to close objects.
 *
 */
int stop_pipe_admin_thread(void)
{
   int pipe_index;

   for(pipe_index = 0; pipe_index < MAX_NUM_OF_PIPES; pipe_index++) {

     /* close the down pipe handle */
     if (pipe_control[pipe_index].hDown != INVALID_HANDLE_VALUE &&
	 pipe_control[pipe_index].hDown != 0)
       if (!CloseHandle(pipe_control[pipe_index].hDown))
	 return ERR_CFAIL;

     /* closing the up pipe will also cause the associated admin thread 
      * to terminate.  Note that as we are using _beginthread to create
      * the admin thread, _endthread is called automatically on thread
      * termination which will subsequently close the thread handle.
      */
     if (pipe_control[pipe_index].hUp != INVALID_HANDLE_VALUE &&
	 pipe_control[pipe_index].hUp != 0)
       if (!CloseHandle(pipe_control[pipe_index].hUp))
	 return ERR_CFAIL;

     pipe_control[pipe_index].is_running = 0;
   }
   
   return 0;
}

/*
 * init_pipe_admin_thread: Set up the pipes for the admin thread.
 *
 * Output: Returns 0 for success.
 */
int init_pipe_admin_thread(int pipe_index)
{
   char pszDownPipeName[64];
   char pszUpPipeName[64];
   PSECURITY_DESCRIPTOR pSD = 0;
   SECURITY_ATTRIBUTES  sa;
   char *protocol_name = 0;

   /* 
    * Compose the down-pipe name. Examine the iPipeControlIndex to see if 
    * we're talking to SIP or H323
    */
   switch (pipe_index) {
   case H323_PIPE_INDEX:
     protocol_name = H323_PIPE;
     break;
   case SIP_PIPE_INDEX:
     protocol_name = SIP_PIPE;
     break;
   default:
     return ERR_CFAIL;
   }

   sprintf(pszDownPipeName, "\\\\.\\pipe\\%sR", protocol_name);

   /* Now attempt to open this pipe */
   if(!WaitNamedPipe(pszDownPipeName, NMPWAIT_USE_DEFAULT_WAIT)) 
      {  
      return ERR_CFAIL;
      }
   
   pipe_control[pipe_index].hDown
     = CreateFile(pszDownPipeName,GENERIC_WRITE,0,0,OPEN_EXISTING,0,0);

   if (INVALID_HANDLE_VALUE == pipe_control[pipe_index].hDown)
      {
      return ERR_CFAIL;
      }

   /* 
     Compose the up-pipe name. Examine the pipe_index to see if 
     we're talking to SIP or H323. Also append pid to end of pipe-name.
    */
   sprintf(pszUpPipeName, "\\\\.\\pipe\\%sW%d", 
           H323_PIPE_INDEX==pipe_index ? 
           H323_PIPE : SIP_PIPE, getpid());


   pSD = malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
   if ( pSD == NULL ) 
      {
      goto fail_down_pipe;
      }

   if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)) 
      {
	goto fail_sd;
      }

   /* add a NULL disc. ACL to the security descriptor. */
   if (!SetSecurityDescriptorDacl(pSD, TRUE, (PACL) NULL, FALSE)) 
      {
	goto fail_sd;
      }

   sa.nLength = sizeof(sa);	
   sa.lpSecurityDescriptor = pSD;
   sa.bInheritHandle = TRUE;

   pipe_control[pipe_index].hUp = 
     CreateNamedPipe(pszUpPipeName,PIPE_ACCESS_INBOUND, 
                     PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_WAIT,
                     PIPE_UNLIMITED_INSTANCES,sizeof(ACU_SERVICE_MSG),
                     sizeof(ACU_SERVICE_MSG),5000,&sa);

   if(INVALID_HANDLE_VALUE == pipe_control[pipe_index].hUp)
      {
	goto fail_sd;
      }

   /* flag that this pipe/service is operational with magic number 1*/
   pipe_control[pipe_index].is_running = 1;

   free(pSD);

   return 0;

 fail_sd:
   free(pSD);
 fail_down_pipe:
   CloseHandle(pipe_control[pipe_index].hDown);

   return ERR_CFAIL;

}

/*
 * PipeAdminThread : 
 *
 * Description : 
 *
 * Input : param - pointer to an AdminThreadControl struct.
 *
 * Output : Thread exit code
 *
 */
void PipeAdminThread(void* pv)
{
   struct AdminThreadControl* pAdminThreadControl = (struct AdminThreadControl*) pv;
   int pipe_index = pAdminThreadControl->pipe_index;
   ACU_SERVICE_MSG      acu_service_msg;
   DWORD bytes=0;
   int ret;

   /* create a mutex to guard the pipes */
   pipe_control[pipe_index].hDownLock = CreateMutex(0, 0, 0);
   if (pipe_control[pipe_index].hDownLock == INVALID_HANDLE_VALUE) {
     SetEvent(pAdminThreadControl->hStartEvent);
     return;
   }

   ret = init_pipe_admin_thread(pipe_index);

   /* wake up our blocked parent */
   SetEvent(pAdminThreadControl->hStartEvent);

   if (ret != 0) {
     CloseHandle(pipe_control[pipe_index].hDownLock);
     return;
   }

   /* enter main processing loop of pipe administration thread */
   while(1)
      {
      /*
       * Ensure that the pipe is connected prior to each read
       */
      if(!ConnectNamedPipe(pipe_control[pipe_index].hUp, 0)) 
         {
         if(ERROR_PIPE_CONNECTED != GetLastError())
            {
            continue;
            }
         }

      bytes = 0;

      /* ReadFile is used to allow blocking on pipe but note that we don't
       * actually read any data off the pipe. This has to be carried out
       * by the thread making the API call.
       */
      if (!ReadFile(pipe_control[pipe_index].hUp, NULL, 0, &bytes, 0)) 
         {
         if(GetLastError() != ERROR_MORE_DATA) 
            {
            return;
            } 
         }

      if (!PeekNamedPipe(pipe_control[pipe_index].hUp, &acu_service_msg, sizeof(ACU_SERVICE_MSG), &bytes, 0, 0))
         {
            /* problem reading pipe so translate error - er right */
         }
      else 
         {
         if (bytes)
            {
            SetEvent(acu_service_msg.pendingMsgEvent);
            WaitForSingleObject(acu_service_msg.readMsgEvent, INFINITE);
            CloseHandle(acu_service_msg.readMsgEvent);
            }
         }
      }
}

/*
 * pipe_client_send_application_terminated : 
 *         Write APPLICATION_TERMINATED to pipe providing board number.
 *
 * Input : board_card_number - 
 *
 * Output : None 
 *
 */  
int pipe_client_send_application_terminated(int board_card_number)
{
   int             result = 0;
   ACU_SERVICE_MSG acu_service_msg;
   DWORD           bytes;

   /* TODO should this not really call srvioctl? */

   acu_service_msg.voip_card = board_card_number;
   acu_service_msg.message_type = APPLICATION_TERMINATED;

   if(is_pipe_running(H323_PIPE_INDEX))
      {
      if (!WriteFile(pipe_control[H323_PIPE_INDEX].hDown, &acu_service_msg, sizeof(ACU_SERVICE_MSG), &bytes, 0)) 
         {
         result = ERR_CFAIL;
         }
      }
  
   if(is_pipe_running(SIP_PIPE_INDEX))
      {
      if (!WriteFile(pipe_control[SIP_PIPE_INDEX].hDown, &acu_service_msg, sizeof(ACU_SERVICE_MSG), &bytes, 0)) 
         {
         result = ERR_CFAIL;
         }
      }
   return result;
}

/*
 * is_pipe_running : 
 *
 * Description : return true if the pipe relating to a 
 * particular voip protocol is running
 *
 * Input : 0 if checking for existence of H323, 1 for 
 * existence of SIP
 *
 * Output : 1/TRUE if required protocol exists
 *
 */
ACU_INT is_pipe_running(int voip_protocol)
{
    if(0 > voip_protocol || voip_protocol >= MAX_NUM_OF_PIPES)
    {
        return 0;
    }

    return pipe_control[voip_protocol].is_running;
}


/*
 * get_voip_protocol_index_array :
 *
 * Description : Returns an array of those indexes into pipe_control
 * which represent *usuable* VoIP protocols. The array elements may be
 * used as the voip_protocol arg. into srvioctl function.
 *
 * Also populates the variable at num_of_protocols with the number of these
 * indexes which represent usable VoIP protocols
 */
const  ACU_INT* get_voip_protocol_index_array(int* num_of_protocols)
{
    static int loaded_voip_protocol_index_array[MAX_NUM_OF_PIPES] = {-1,-1};
    int i=0;

    *num_of_protocols = 0;
    for(i = 0; i < MAX_NUM_OF_PIPES; i++)
    {
        if(pipe_control[i].is_running)
        {
            loaded_voip_protocol_index_array[(*num_of_protocols)++] = i;
        }
    }

    return loaded_voip_protocol_index_array;
}

/*---------------- srvioctl --------------*/
/* VoIP equivalent of clioctl function.   */
/* Return result of API command request   */
/*                                        */
ACU_INT srvioctl ( ACU_INT  function, IOCTLU  *pioctl, int len , 
                   int board_card_number, int voip_protocol )
{
   ACU_SERVICE_MSG    acu_service_msg;
   ACU_INT            result;
   DWORD              bytes;
#ifdef _MT
   HANDLE             pendingMsgEvent, readMsgEvent;
#endif
   DWORD              thisProcessId;
   voip_admin_msg    *admin_msg = pioctl->voip_admin_out_xparms.admin_msg;

   if (!is_pipe_running(voip_protocol)) {
      return ERR_CFAIL;
   }

   thisProcessId = GetCurrentProcessId(); /* obtain current process id */

   init_api_reg (&pioctl->api_reg, len);

   memset(&acu_service_msg, 0, sizeof(ACU_SERVICE_MSG));

#ifdef _MT
   if (function != CALL_SEND_RAS_MSG) 
      { 
	/* CALL_SEND_RAS_MSG doesn't require event objects yet */
	pendingMsgEvent = CreateEvent ( NULL, TRUE, FALSE, NULL );
	readMsgEvent = CreateEvent ( NULL, TRUE, FALSE, NULL );
    
	if ( (pendingMsgEvent  == NULL) || (readMsgEvent == NULL) ) 
	  {
	    return ERR_CFAIL;
	  }
	
	acu_service_msg.pendingMsgEvent = pendingMsgEvent;
	acu_service_msg.readMsgEvent = readMsgEvent;
      }
#endif

   acu_service_msg.voip_card = board_card_number;
   acu_service_msg.srcProcessId = thisProcessId;

   switch (function) 
      {
      case CALL_GET_RAS_MSG:
      case CALL_SEND_RAS_MSG:
         acu_service_msg.message_type = ADMIN_CHAN_RAS_MSG;
      break;

      default:  /* every other API call */
         acu_service_msg.message_type = TLS_MSG_GENERIC_TLS;
      } /* end of case */

   acu_service_msg.function = function;

#ifdef _MT
   /* The pipe is mine, all mine! */
   if (WaitForSingleObject( pipe_control[voip_protocol].hDownLock, INFINITE ) != WAIT_OBJECT_0) 
      {
	CloseHandle(pendingMsgEvent);
	CloseHandle(readMsgEvent);
	return ERR_CFAIL;
      }
#endif

  /* write service message to downstream pipe */
   if (!WriteFile(pipe_control[voip_protocol].hDown, &acu_service_msg, sizeof(ACU_SERVICE_MSG), &bytes, 0)) 
      {
      goto fail_handles;
      }

   switch (acu_service_msg.message_type) 
     {
     case TLS_MSG_GENERIC_TLS:
       if (!WriteFile(pipe_control[voip_protocol].hDown, 
                        pioctl, 
                        len, 
                        &bytes, 
                        0)) 
            {    
            goto fail_handles;
            }
      break;

      case ADMIN_CHAN_RAS_MSG:
         switch (function) 
            {
            case CALL_SEND_RAS_MSG:
               if (!WriteFile(pipe_control[voip_protocol].hDown, 
                              admin_msg, 
                              sizeof(voip_admin_msg), 
                              &bytes, 
                              0)) 
                  {     
                  goto fail_handles;
                  }

               if (admin_msg->endpoint_alias_count > 0) 
                  {
                  if (!WriteFile(pipe_control[voip_protocol].hDown, 
                                 admin_msg->endpoint_alias, 
                                 sizeof(alias_address) * admin_msg->endpoint_alias_count,
                                 &bytes, 
                                  0)) 
                     {     
                     goto fail_handles;
                     }
                  }

               if (admin_msg->prefix_count > 0) 
                  {
                  if (!WriteFile(pipe_control[voip_protocol].hDown, 
                                admin_msg->prefixes, 
                                sizeof(alias_address) * admin_msg->prefix_count,
                                &bytes, 
                                0)) 
                     {
                     goto fail_handles;
                     }
                 }

               /* Postpone the free() to here since if we return an error the
                  user will expect to have to deallocate the buffers
                  themselves.  free() on NULL is safe. */
               free(admin_msg->endpoint_alias);
               free(admin_msg->prefixes);

#ifdef _MT
               ReleaseMutex( pipe_control[voip_protocol].hDownLock );
#endif
               return 0;
      
            case CALL_GET_RAS_MSG:
               /* Nothing to do here */
            break;

            default:
               /* This Never Happens */
               goto fail_handles;
            }
         break;

      default:
         goto fail_handles;
      }

#ifdef _MT
   /* Done with the pipe */
   ReleaseMutex( pipe_control[voip_protocol].hDownLock );
  
   /* wait for return message */
   WaitForSingleObject( pendingMsgEvent, INFINITE );
#endif
  
   if (!ReadFile(pipe_control[voip_protocol].hUp, &acu_service_msg, sizeof(ACU_SERVICE_MSG), &bytes, 0)) 
      {
      goto fail_read;
      }

   switch (acu_service_msg.message_type) 
      {
      case ADMIN_CHAN_RAS_MSG:
         pioctl->voip_admin_in_xparms.valid = acu_service_msg.valid;
         result = acu_service_msg.command_error;

         if (!acu_service_msg.valid)
            break;

         /* Must be a read. */
         if (!ReadFile(pipe_control[voip_protocol].hUp, 
                       admin_msg, 
                       sizeof(*admin_msg), 
                       &bytes, 0)) 
            {
            goto fail_read;
            }

         /* Potentially with aliases... */
         
         if (admin_msg->endpoint_alias_count > 0) 
            {
            admin_msg->endpoint_alias = malloc(admin_msg->endpoint_alias_count * sizeof(alias_address));
            if (admin_msg->endpoint_alias == NULL) 
               {
               goto fail_read;
               }

            if (!ReadFile(pipe_control[voip_protocol].hUp, 
                          admin_msg->endpoint_alias,
                          admin_msg->endpoint_alias_count * sizeof(alias_address),
                         &bytes, 0)) 
               {
               free(admin_msg->endpoint_alias);
               goto fail_read;
               }
            }

         /* ...and prefixes */
    
         if (admin_msg->prefix_count > 0) 
            {
            admin_msg->prefixes = malloc(admin_msg->prefix_count * sizeof(alias_address));
            if (admin_msg->prefixes == NULL) 
               {
               free(admin_msg->endpoint_alias);
               goto fail_read;
               }

            if (!ReadFile(pipe_control[voip_protocol].hUp, 
                          admin_msg->prefixes,
                          admin_msg->prefix_count * sizeof(alias_address),
                         &bytes, 0)) 
               {
               free(admin_msg->endpoint_alias);
               free(admin_msg->prefixes);
               goto fail_read;
            }
         }
      break;
    
      case TLS_MSG_GENERIC_TLS:
         if (!ReadFile(pipe_control[voip_protocol].hUp, pioctl, len, &bytes, 0)) 
            {
            goto fail_read;
            }
         result = acu_service_msg.command_error;
      break;

      default:
         /* invalid type */
         goto fail_read;
      }
  
#ifdef _MT
   SetEvent(readMsgEvent); /* allow pipe admin thread to continue */
  
   if ( !CloseHandle ( pendingMsgEvent ) ) 
      {
      return ERR_CFAIL;
      }
#endif

   return result;  

 fail_read:
#ifdef _MT
   SetEvent(readMsgEvent);
   CloseHandle(pendingMsgEvent);
#endif
   return ERR_CFAIL;
 fail_handles:
#ifdef _MT
   CloseHandle(pendingMsgEvent);
   CloseHandle(readMsgEvent);
   ReleaseMutex( pipe_control[voip_protocol].hDownLock );
#endif
   return ERR_CFAIL;
}
/*--------- end of file -------*/

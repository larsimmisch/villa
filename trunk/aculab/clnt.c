/*------------------------------------------------------------*/
/* ACULAB plc                                                 */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : clnt.c                                 */
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
/* rev:  2.03   10/02/00                                      */
/*                                                            */
/*                                                            */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/

#include "mvcldrvr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>

#include <windows.h>             
#include <winioctl.h>           

#define FALSE  0
#define TRUE   1

#define NNETS      2        /* number of network ports */
#define NCH       30        /* number of channels      */

/*----------- Function Prototypes ---------------*/
/*                                               */
/* defined in mvcldrvr.h                         */
/*                                               */
/*-----------------------------------------------*/

/*--------- operating system specifics -------------*/

int        clopen            ( char * );
ACU_INT    clioctl           ( ACU_INT, IOCTLU *, int, int );
ACU_INT    clpblock_ioctl    ( ACU_INT, V5_PBLOCK_IOCTLU *, int, int );
void       clclose    ( void );
void       clspecial  ( void );

int  clfileopen  ( char * );
int  clfileread  ( int, char *, unsigned int );
int  clfileclose ( int );

/*--------------- local data --------------------*/

char cldevname[] = { "\\\\.\\MVIP$SS0" };

/*------------- external data -------------------*/

extern int clopened;
extern int ncards;
extern CARD clcard[NCARDS];

/*------------ OS specifics -------------*/
/* Operating systems specific fucntions  */
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

   clh = CreateFile (
    cldevnp,
	GENERIC_READ|GENERIC_WRITE,
	FILE_SHARE_READ|FILE_SHARE_WRITE,
	&security_attributes,
	OPEN_EXISTING,
	FILE_FLAG_OVERLAPPED,
	NULL
	);

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
   int  i;


   clopened = FALSE;

   for ( i = 0; i < ncards; i++ )
      {
      CloseHandle ( (HANDLE)clcard[i].clh );
      }
   }
/*---------------------------------------*/


/*---------------- clioctl --------------*/
/* call the nt ioctl function            */
/* return 0 if ok else return error      */
/*                                       */
ACU_INT clioctl ( ACU_INT  function, IOCTLU  *pioctl, int clh, int len )
   {
   int        result;
   int        BytesReturned;
   int        i;
   char      *pointer;

   OVERLAPPED overlapped;
   BOOL       complete;

   NTIOCTL    ntioctl;
   int        ioctlsize;

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
                              &BytesReturned,
                              &overlapped
                            );

   WaitForSingleObject(
                       overlapped.hEvent,
                       INFINITE
                      );
	
   complete = GetOverlappedResult (
                                   (HANDLE)clh,
                                   &overlapped,
                                   &BytesReturned,
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

   return ( (ACU_INT) result );
   }
/*---------------------------------------*/


/*-------------clpblock_ioctl -----------*/
/* call the nt ioctl function            */
/* return 0 if ok else return error      */
/*                                       */
ACU_INT clpblock_ioctl ( ACU_INT function, V5_PBLOCK_IOCTLU *pioctl, int clh, int len)
   {
   int        result;
   int        BytesReturned;

   OVERLAPPED overlapped;
   BOOL       complete;

   init_api_reg (&pioctl->api_reg, len);


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
                              &BytesReturned,
                              &overlapped
                            );

   WaitForSingleObject(
                       overlapped.hEvent,
                       INFINITE
                      );
	
   complete = GetOverlappedResult (
                                   (HANDLE)clh,
                                   &overlapped,
                                   &BytesReturned,
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
    tMVEventId	ev;
	char eventName[64];
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



/*--------- end of file -------*/


/*------------------------------------------------------------*/
/* ACULAB Ltd                                                 */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : swnt.c                                 */
/*                                                            */
/*           Purpose : Switch control library programs for    */
/*                     Multiple drivers                       */
/*                                                            */
/*            Author : Marcus Bullingham                      */
/*                                                            */
/*       Create Date : JANUARY 1995                           */
/*                                                            */
/*             Tools : Microsoft C                            */
/*                                                            */
/*                                                            */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Change History                                             */
/*                                                            */
/*                                                            */
/* cur:  3.40   26/11/99   pgd   Overlapped i/o               */
/* rev:  1.00   30/03/93   agr   File created                 */
/* rev:  1.01   16/03/93   agr   tristate_switch added        */
/* rev:  1.02   06/04/93   agr   Updated for Multiple Drivers */
/* rev:  1.03   05/01/93   agr   set_idle function modified   */
/* rev:  2.10   14/02/96   pgd   First SCbus switch release   */
/* rev:  2.20   18/06/96   pgd   BR net streams>=32 release   */
/* rev:  2.30   17/10/96   pgd   Migrate to V4 generic etc.   */
/* rev:  3.01   31/03/98   pgd   V3 version                   */
/* rev:  3.02   21/05/98   pgd   Remove .. from mvswdrvr path */
/* rev:  3.03   16/06/98   pgd   Eliminate __NEWC__           */
/* rev:  3.30   22/01/99   pgd   Align with release 1.3.4     */
/* rev:  3.40   26/11/99   pgd   Overlapped i/o               */
/*                                                            */
/*------------------------------------------------------------*/

#ifdef _ACUSWITCHDLL
#include "acusxlib.h"
#endif

#include "mvswdrvr.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <windows.h>
#include <winioctl.h>

#define CATEGORY  0xe0
#define FUNCTION  0x40


int    swopendev   ( void );
HANDLE swopen      ( char * );
int    swioctl     ( int, SWIOCTLU *, HANDLE, int );
void   swclose     ( void );

/*---------------------------------------------*/


int    nswitch = 0;
int    swopened = 0;
HANDLE swcard[NSWITCH];

char   swdevname[] = { "\\\\.\\MVIP$SW0" };   


HANDLE swopen ( char * swdevnp )
{
	HANDLE  			result;
	HANDLE				swh;
	SECURITY_ATTRIBUTES security_attributes;

	security_attributes.nLength 				= sizeof ( SECURITY_ATTRIBUTES );
	security_attributes.lpSecurityDescriptor 	= NULL;
	security_attributes.bInheritHandle 			= TRUE;

	swh = CreateFile(	swdevnp,
						GENERIC_READ|GENERIC_WRITE,
						FILE_SHARE_READ|FILE_SHARE_WRITE,
						&security_attributes,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,
						NULL											);

	if ( swh != INVALID_HANDLE_VALUE )
	{
		result = swh;
	}
	else
	{
		result = INVALID_HANDLE_VALUE;
	}

	return result;
}


int swioctl ( int function, SWIOCTLU* pioctl, HANDLE swh, int size )
{
	int 		result;
	int  		bytesReturned;
	OVERLAPPED	overlapped;
	HANDLE		opHandle;
	BOOL		ioResult;
	SXNTIOCTL 	swntioctl;


   	swntioctl.command = function;
	swntioctl.error   = 0;
   
	if (size != 0)
	{
		memcpy(( char* )(&swntioctl.ioctlu),(char*)pioctl,size);
	}

	opHandle 		  		= swh;
	overlapped.hEvent 		= CreateEvent(NULL,TRUE,FALSE,NULL);
	overlapped.Offset		= 0;     
	overlapped.OffsetHigh	= 0;

	if (overlapped.hEvent == NULL)
	{
		ioResult = FALSE;
	}
	else
	{
		ioResult = DeviceIoControl(	opHandle,
									(DWORD)SX_IOCTL,
									&swntioctl, 
									sizeof(SXNTIOCTL),
									&swntioctl, 
									sizeof(SXNTIOCTL),
									(unsigned long *)&bytesReturned,
									&overlapped			);
 		if (!ioResult)
		{
			if (GetLastError() == ERROR_IO_PENDING)
			{
				WaitForSingleObject(overlapped.hEvent,INFINITE);
		
				ioResult = GetOverlappedResult(	(HANDLE)opHandle,
												&overlapped,
												(unsigned long *)&bytesReturned,
												TRUE				);
			}
		}

		if (ioResult)
		{
			if (size != 0)
			{
				memcpy((char*)pioctl,(char*)(&swntioctl.ioctlu),size);
			}
		}

		CloseHandle(overlapped.hEvent);
	}


	if (ioResult)
	{
		result = swntioctl.error;
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
void swclose ( void )
{
	int  i;

	swopened = FALSE;

	for ( i = 0; i < nswitch; i++ )
	{
	  CloseHandle ( swcard[i] );
	}
}


int swopendev( void )
{
	int result;
	int addrnum;

	if (swopened == FALSE)
	{
		for (nswitch = 0; nswitch < NSWITCH; nswitch++)
		{
			addrnum = strlen (swdevname) - 1;

			if (nswitch >= 10)
			{
				swdevname[addrnum] = (char) ((nswitch - 10) + 'A');  /* set device name */
			}
			else
			{
				swdevname[addrnum] = (char) (nswitch + '0');  /* set device name */
			}

			swcard[nswitch] = swopen(swdevname);

			if (swcard[nswitch] == INVALID_HANDLE_VALUE)             /* check for error */
			{
				break;                              /* device not there */
			}
		}

		if ( nswitch != 0 )
		{
			swopened = TRUE;

			result = 0;          /* some cards have opened */
		}
		else
		{
			result = MVIP_DEVICE_ERROR;
		}
	}
	else
	{
		result = 0;             /* already open */
	}

	return result;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int sw_ev_create( int swdrvr, tSWEventId* eventId )
{
	int			rc;
	tSWEventId 	ev;
	char		eventName[64];

	rc = 0;

	sprintf(&eventName[0],"%s%d",kSWNTEvBasisName,swdrvr);

	ev = (tSWEventId) OpenEvent(SYNCHRONIZE,FALSE,&eventName[0]);

	if (ev == NULL)
	{
		rc = ERR_SW_NO_RESOURCES; 

		*eventId = 0;
	}
	else
	{
		*eventId = ev;

		rc = 0;
	}

	return rc;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int sw_ev_free( int swdrvr, tSWEventId eventId )
{
	if (eventId != 0)
	{
		CloseHandle(eventId);		
	}

	return 0;
}


#ifdef _ACUSWITCHDLL
ACUDLL
#endif
int sw_ev_wait( int swdrvr, tSWEventId eventId )
{
	WaitForSingleObject(eventId,INFINITE);

	return 0;
}


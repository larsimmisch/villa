/*------------------------------------------------------------*/
/* Copyright ACULAB plc. (c) 1996-1997                        */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : smnt.c                                 */
/*                                                            */
/*           Purpose : SHARC module driver library            */
/*                     (NT specific)                          */
/*                                                            */
/*            Author : Peter Dain                             */
/*                                                            */
/*       Create Date : 21st February 1997                     */
/*                                                            */
/*                                                            */					    
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

#ifdef _PROSDLL
#include "proslib.h"
#endif

#include "smdrvr.h"
#include "smosintf.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <io.h>

#include <windows.h>
#include <winioctl.h>
#include <winerror.h>


tSMDevHandle smdControlDevHandle = INVALID_HANDLE_VALUE;


/*
 * SMDOPEN
 *
 * Open a driver device.
 */
static tSMDevHandle smdopen( char * smdevnp )
{
	HANDLE				smh;
	SECURITY_ATTRIBUTES security_attributes;

	security_attributes.nLength 				= sizeof(SECURITY_ATTRIBUTES);
	security_attributes.lpSecurityDescriptor 	= NULL;
	security_attributes.bInheritHandle 			= TRUE;

	/*
	 * Note we cannot have DIRECT_IO device drivere and specify FILE_FLAG_OVERLAPPED
	 * with FILE_ATTRIBUTE_NORMAL it seems.
	 */
	smh = CreateFile(	smdevnp,
						GENERIC_READ|GENERIC_WRITE,
						FILE_SHARE_READ|FILE_SHARE_WRITE,
						&security_attributes,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,
						NULL											);

   	return smh;
}


static void rootName( char* root, int isEventRootName )
{
	OSVERSIONINFO osvi;

	ZeroMemory(&osvi,sizeof(OSVERSIONINFO));

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	GetVersionEx (&osvi);

	if ( osvi.dwMajorVersion > 4 )
	{
		if (isEventRootName)
		{
			strcpy(root,"Global\\");
		}
		else
		{
			strcpy(root,"\\\\.\\Global\\");
		}
	}
	else
	{
		if (isEventRootName)
		{
			*root = 0;
		}
		else
		{
			strcpy(root,"\\\\.\\");
		}
	}
}


/*
 * SMD_OPEN_CTL_DEV
 *
 * Open master (control) device for driver.
 * Handle for this device is stored in global:
 * 
 *         smdControlDevHandle
 *
 * and is used for IOCTL interactions etc. 
 */
tSMDevHandle smd_open_ctl_dev( void )
{
	int  isGoodDevice;
	char controlDeviceName[64];

	if (!(isGoodDevice = (smdControlDevHandle != INVALID_HANDLE_VALUE)))
   	{
		rootName(&controlDeviceName[0],0);

		strcpy(&controlDeviceName[strlen(&controlDeviceName[0])],kSMDNTDevControlName);

		smdControlDevHandle = smdopen(&controlDeviceName[0]);

		isGoodDevice = (smdControlDevHandle != INVALID_HANDLE_VALUE);
	}

	return (isGoodDevice) ? smdControlDevHandle : 0;
}


/*
 * Close down access to device driver.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
void smd_close_ctl_dev( void )
{
	if (smdControlDevHandle != INVALID_HANDLE_VALUE)
   	{
		CloseHandle((HANDLE) smdControlDevHandle);

		smdControlDevHandle = INVALID_HANDLE_VALUE;
	}
}


/*
 * SMD_OPEN_CHNL_DEV
 *
 * Allocate an O/S handle for a specific channel whose
 * integer index 1..n is supplied as channel.
 */
tSMDevHandle smd_open_chnl_dev( tSMChannelId channel )
{
	char			channelDeviceName[64];
	tSMChannelId 	result;
	HANDLE			handle;

	/*
	 * In order to use NT read/write facilities,
	 * translate returned channel to a logical device name,
	 * and open that device.
	 *
	 * For the user, the channel id is identified with 
	 * this new handle.
	 */
	rootName(&channelDeviceName[0],0);

	strcpy(&channelDeviceName[strlen(&channelDeviceName[0])],kSMDNTDevBasisName);

	sprintf(	&channelDeviceName[strlen(&channelDeviceName[0])],
				"%d",
				(((int)(channel))-1)									);

	handle = smdopen(&channelDeviceName[0]);

	if (handle == INVALID_HANDLE_VALUE)
	{
		result = 0;
	}
	else
	{
		SetHandleInformation(handle,HANDLE_FLAG_INHERIT,0);

		result = (tSMChannelId) handle;
	}

	return result;
}


/*
 * SMD_CLOSE_CHNL_DEV
 *
 * Release a previously allocated handle for a channel.
 */
void smd_close_chnl_dev( tSMDevHandle handle )
{
	if (handle != 0)	 
	{
		CloseHandle((HANDLE) handle);
 	}
}


static int smd_map_win32_err( void )
{
	int			result;
	DWORD 		lastError;

	lastError = GetLastError();

	switch(lastError)
	{
		case ERROR_NOT_ENOUGH_QUOTA:
		case ERROR_NOT_ENOUGH_MEMORY:
		case ERROR_NO_MORE_FILES:
			result = ERR_SM_OS_RESOURCE_PROBLEM;
			break;

		default:
			result = ERR_SM_DEVERR;
			break;
	}
	    	
	return result;
}


/* 
 * SMD_IOCTL_DEV_GENERIC 
 *
 * Invoke IOCTL request to control driver.
 */
int  smd_ioctl_dev_generic( tSM_INT function, SMIOCTLU* pioctl, tSMDevHandle smh, tSM_INT size )
{
	int 		result;
	int  		bytesReturned;
	SKNTIOCTL 	skntioctl;
	OVERLAPPED	overlapped;
	HANDLE		opHandle;
	BOOL		ioResult;

   	skntioctl.command		= function;
   	skntioctl.module		= -1;	/* Module specific only if f/w specific API call*/
   	skntioctl.apiLibVersion = ((kSMDVersionMaj<<8) + kSMDVersionMin);
   	skntioctl.fwLibVersion  = -1;	/* Inducates generic API call. */
   	skntioctl.error   		= 0;

	if (size != 0)
	{
		memcpy((char*) &skntioctl.ioctlu,(char*)pioctl,size);
	}

   	bytesReturned = 0;

#ifdef __SM_VALIDATE_HANDLE
	if ((smh != 0) && (smh != smdControlDevHandle) && (function != SMIO_CHANNEL_VALIDATE_ID) && (function != SMIO_STORE_APP_CHANNEL_ID))
	{
		result = sm_channel_validate_id(smh);
	}
	else
	{
		result = 0;
	}

	if (result == 0)
	{
#endif
		overlapped.Offset		= 0;     
		overlapped.OffsetHigh	= 0;
		overlapped.hEvent 		= CreateEvent(NULL,TRUE,FALSE,NULL);

		if (overlapped.hEvent == NULL)
		{
			ioResult = FALSE;
		}
		else
		{
			opHandle = (smh == 0) ? smdControlDevHandle : smh;

			ioResult = DeviceIoControl(	opHandle,
										(DWORD)SM_IOCTL,
										&skntioctl, 
										sizeof(SKNTIOCTL),
										&skntioctl, 
										sizeof(SKNTIOCTL),
										&bytesReturned,
										&overlapped				);

			if (!ioResult)
			{
				if (GetLastError() == ERROR_IO_PENDING)
				{
					WaitForSingleObject(overlapped.hEvent,INFINITE);
		
					ioResult = GetOverlappedResult(	(HANDLE)opHandle,
													&overlapped,
													&bytesReturned,
													TRUE				);
				}
			}

			CloseHandle ( overlapped.hEvent );
	   }

		if (ioResult)
		{
			if (size != 0)
			{
				memcpy((char*)pioctl,(char*) &skntioctl.ioctlu,size);
			}

	    	result = skntioctl.error;
	    }
	   	else
	    {
	    	result = smd_map_win32_err();
	    }
#ifdef __SM_VALIDATE_HANDLE
	}
#endif
			  
	return result;
}


/* 
 * SMD_IOCTL_FWAPI 
 *
 * Invoke f/w specific IOCTL request to control driver.
 */
int smd_ioctl_dev_fwapi( tSM_INT function, SMIOCTLU * pioctl, tSMDevHandle smh, tSM_INT size, tSM_INT module, tSM_INT fwVersion )
{
	int 		result;
	int  		bytesReturned;
	SKNTIOCTL 	skntioctl;
	OVERLAPPED	overlapped;
	HANDLE		opHandle;
	BOOL		ioResult;

#ifdef __SM_VALIDATE_HANDLE
	if ((smh != 0) && (smh != smdControlDevHandle))
	{
		result = sm_channel_validate_id(smh);
	}
	else
	{
		result = 0;
	}

	if (result == 0)
	{
#endif
		overlapped.Offset		= 0;     
		overlapped.OffsetHigh	= 0;
		overlapped.hEvent 		= CreateEvent(NULL,TRUE,FALSE,NULL);

		if (overlapped.hEvent == NULL)
		{
			ioResult = FALSE;
		}
		else
		{
	   		skntioctl.command		= function;
	   		skntioctl.module		= module;
	   		skntioctl.apiLibVersion = ((kSMDVersionMaj<<8) + kSMDVersionMin);
	   		skntioctl.fwLibVersion  = fwVersion;
	   		skntioctl.error   		= 0;

			if (size != 0)
			{
				memcpy((char*) &skntioctl.ioctlu,(char*)pioctl,size);
			}

	   		bytesReturned = 0;

			opHandle = (smh == 0) ? smdControlDevHandle : smh;

			ioResult = DeviceIoControl(	opHandle,
										(DWORD)SM_IOCTL,
										&skntioctl, 
										sizeof(SKNTIOCTL),	
										&skntioctl, 
										sizeof(SKNTIOCTL),
										&bytesReturned,
										&overlapped				);

			if (!ioResult)
			{
				if (GetLastError() == ERROR_IO_PENDING)
				{
					WaitForSingleObject(overlapped.hEvent,INFINITE);
		
					result = GetOverlappedResult(	(HANDLE)opHandle,
													&overlapped,
													&bytesReturned,
													TRUE				);
				}
			}

			CloseHandle ( overlapped.hEvent );
		}

		if (ioResult)
		{
			if (size != 0)
			{
				memcpy((char*)pioctl,(char*) &skntioctl.ioctlu,size);
			}

	    	result = skntioctl.error;
	    }
	   	else
	    {
	    	result = smd_map_win32_err();
	    }
#ifdef __SM_VALIDATE_HANDLE
	}
#endif
			  
	return result;
}


/* 
 * SMD_READ_DEV 
 *
 * Invoke read request to driver.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int  smd_read_dev( HANDLE smh, char* data, tSM_INT* length )
{
	int			result;
	DWORD 		readOctets;
	OVERLAPPED	overlapped;
	BOOL		ioResult;

#ifdef __SM_VALIDATE_HANDLE
	result = sm_channel_validate_id(smh);

	if (result == 0)
	{
#endif
		overlapped.Offset		= 0;     
		overlapped.OffsetHigh	= 0;
		overlapped.hEvent 		= CreateEvent(NULL,TRUE,FALSE,NULL);

		if (overlapped.hEvent == NULL)
		{
			ioResult = FALSE;
		}
		else
		{
			ioResult = ReadFile(smh,(LPVOID) data,*length,&readOctets,&overlapped);
			
			if (!ioResult)
			{
				if (GetLastError() == ERROR_IO_PENDING)
				{
				   WaitForSingleObject(overlapped.hEvent,INFINITE);
		
				   ioResult = GetOverlappedResult(	(HANDLE)smh,
													&overlapped,
													&readOctets,
													TRUE			);
				}
			}

			CloseHandle ( overlapped.hEvent );
		}

		result = (!ioResult) ? ERR_SM_DEVERR : 0;
		
#ifdef __SM_VALIDATE_HANDLE
	}
#endif

	if (result == 0)
	{
		*length = readOctets;
	}
	else
	{
		*length = 0;
	}

	return result;
}


/* 
 * SMD_WRITE_DEV 
 *
 * Invoke write request to control driver.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int smd_write_dev( HANDLE smh, char* data, tSM_INT length )
{
	int 		result;
	DWORD 		writtenOctets;
	OVERLAPPED	overlapped;
	BOOL		ioResult;

#ifdef __SM_VALIDATE_HANDLE
	result = sm_channel_validate_id(smh);

	if (result == 0)
	{
#endif
		/*
		 * Note: error could be reported here because another thread invokes
		 * replay abort occurs between write initiate and complete.
		 */
		overlapped.Offset		= 0;     
		overlapped.OffsetHigh	= 0;
		overlapped.hEvent 		= CreateEvent(NULL,TRUE,FALSE,NULL);

		if (overlapped.hEvent == NULL)
		{
			ioResult = FALSE;
		}
		else
		{
			ioResult = WriteFile(smh,(LPCVOID) data,length,&writtenOctets,&overlapped); 

			if (!ioResult)
			{
				if (GetLastError() == ERROR_IO_PENDING)
				{
					WaitForSingleObject(overlapped.hEvent,INFINITE);
	
					ioResult = GetOverlappedResult (	(HANDLE)smh,
														&overlapped,
														&writtenOctets,
														TRUE			);
				}
			}

			CloseHandle ( overlapped.hEvent );
		}

		result = (!ioResult) ? ERR_SM_DEVERR : 0;

#ifdef __SM_VALIDATE_HANDLE
	}
#endif

	if ((result == 0) && (length != 0) && (writtenOctets == 0))
	{
		result = ERR_SM_NO_CAPACITY;
	}

	return result;
}


/*
 * SMD_FILE_OPEN
 *
 * Open a file for firmware download.
 */
tSMFileHandle smd_file_open( char* fnamep )
{
	return open(fnamep,O_RDONLY + O_BINARY);
}


/*
 * SMD_FILE_READ
 *
 * Read data for firmware download.
 */
int smd_file_read(	tSMFileHandle fh, char* buffp, tSM_INT len )
{
	return read(fh,buffp,len);
}


/*
 * SMFILECLOSE
 *
 * Close file after firmware download completed.
 */
int smd_file_close( tSMFileHandle fh )
{
	return close(fh);  
}


#ifdef _PROSDLL
ACUDLL 
#endif
int smd_ev_create( tSMEventId* eventId, tSMChannelId channelId, int eventType, int eventChannelBinding )
{
	int			rc;
	tSMEventId 	ev;
	char		eventName1[64];
	tSM_INT 	channelIx;

	rc = 0;

	if (eventChannelBinding == kSMChannelSpecificEvent)
	{
		channelIx = sm_get_channel_ix(channelId);

		if (channelIx < 0)
		{
			rc = ERR_SM_DEVERR;
		}
	}
	
	if (rc == 0)
	{
		rootName(&eventName1[0],1);

		if ((eventType == kSMEventTypeWriteData) || (eventType == kSMEventTypeReadData))
		{
			if (sm_get_ev_mech() < 1)
			{
				if (eventChannelBinding == kSMChannelSpecificEvent)
				{
					sprintf(&eventName1[strlen(&eventName1[0])],"%s%d",kSMDNTDataEvBasisName,channelIx);
				}
				else
				{
					sprintf(&eventName1[strlen(&eventName1[0])],"%s",kSMDNTCtlDataEvBasisName);
				}
			}
			else
			{
				if (eventChannelBinding == kSMChannelSpecificEvent)
				{
					sprintf(&eventName1[strlen(&eventName1[0])],"%s%d",(eventType == kSMEventTypeWriteData) ? kSMDNTWrDataEvBasisName : kSMDNTRdDataEvBasisName,channelIx);
				}
				else
				{
					sprintf(&eventName1[strlen(&eventName1[0])],"%s",(eventType == kSMEventTypeWriteData) ? kSMDNTCtlWrDataEvBasisName : kSMDNTCtlRdDataEvBasisName);
				}
			}
		}
		else if (eventType == kSMEventTypeRecog)
		{
			if (eventChannelBinding == kSMChannelSpecificEvent)
			{
				sprintf(&eventName1[strlen(&eventName1[0])],"%s%d",kSMDNTRecogEvBasisName,channelIx);
			}
			else
			{
				sprintf(&eventName1[strlen(&eventName1[0])],"%s",kSMDNTCtlRecogEvBasisName);
			}

		}
		else
		{
			rc = ERR_SM_NO_RESOURCES;
		}

		if (rc == 0)
		{
			ev = (tSMEventId) OpenEvent(SYNCHRONIZE,FALSE,&eventName1[0]);

			if (ev == NULL)
			{
				rc = ERR_SM_NO_RESOURCES; 

				*eventId = 0;
			}
			else
			{
				*eventId = ev;

				rc = 0;
			}
		}
	}

	return rc;
}


#ifdef _PROSDLL
ACUDLL 
#endif
int smd_ev_free( tSMEventId eventId )
{
	if (eventId != 0)
	{
		CloseHandle(eventId);		
	}

	return 0;
}


#ifdef _PROSDLL
ACUDLL 
#endif
int smd_ev_wait( tSMEventId eventId )
{
	WaitForSingleObject(eventId,INFINITE);

	return 0;
}


#ifdef _PROSDLL
ACUDLL 
#endif
int smd_yield( void )
{
	Sleep(0);

	return 0;
}


#ifdef _PROSDLL
ACUDLL 
#endif
int smd_initialize_critical_section( tSMCriticalSection* csect )
{
	InitializeCriticalSection(csect);

	return 0;
}


#ifdef _PROSDLL
ACUDLL 
#endif
int smd_delete_critical_section( tSMCriticalSection* csect )
{
	DeleteCriticalSection(csect);

	return 0;
}


#ifdef _PROSDLL
ACUDLL 
#endif
int smd_enter_critical_section( tSMCriticalSection* csect )
{
	EnterCriticalSection(csect);

	return 0;
}


#ifdef _PROSDLL
ACUDLL 
#endif
int smd_leave_critical_section( tSMCriticalSection* csect )
{
	LeaveCriticalSection(csect);

	return 0;
}


/*------------------------------------------------------------*/
/* Copyright ACULAB plc. (c) 1996-1998                        */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : smlib.c                                */
/*                                                            */
/*           Purpose : SHARC module control library programs  */
/*                     for multiple drivers (generic part)    */
/*                                                            */
/*            Author : Peter Dain                             */
/*                                                            */
/*       Create Date : 21st February 1997                     */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/

#ifdef _PROSDLL
#include "proslib.h"
#endif

#include "smdrvr.h"
#include "smosintf.h"


/* memcpy definitions. */
#include <string.h>

#ifdef SM_CRACK_API
#include <stdio.h>
#endif


/*
 *******************************************************************
 ********* Entry points into SM low level API. *********************
 *******************************************************************
 */


/*
 * SM_GET_DRIVER_INFO
 *
 * Return driver version information.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_get_driver_info
( struct sm_driver_info_parms* drvinfop )
{
	int 							result;
	tSMDevHandle 					smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_generic( 	SMIO_GET_DRIVER_INFO, 
										   	(SMIOCTLU *) drvinfop,
											smControlDevice,
											sizeof(struct sm_driver_info_parms) );
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_GET_CARDS
 *
 * Return no. of SM cards present in system and known to driver.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_get_cards
( void )
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_generic(	SMIO_GET_CARDS,
										(SMIOCTLU *) 0,
										smControlDevice,
										0		 				);
	}
	else
	{
		result = ERR_SM_DEVERR;
	}

	return result;
}


/*
 * SM_GET_CARD_INFO 
 * 
 * Get information pertaining to current state of card.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_get_card_info
( struct sm_card_info_parms* cardinfop )
{
	int 							result;
	tSMDevHandle 					smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_generic( 	SMIO_GET_CARD_INFO, 
										   	(SMIOCTLU *) cardinfop,
											smControlDevice,
											sizeof(struct sm_card_info_parms) );
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_GET_CARD_REV 
 * 
 * Get revision info pertaining to card.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_get_card_rev
( struct sm_card_rev_parms* cardrevp )
{
	int 							result;
	tSMDevHandle 					smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_generic( 	SMIO_GET_CARD_REV, 
										   	(SMIOCTLU *) cardrevp,
											smControlDevice,
											sizeof(struct sm_card_rev_parms) );
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_GET_MODULES
 *
 * Return no. of SM modules present in system and known to driver.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_get_modules
( void )
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_generic(	SMIO_GET_MODULES,
										(SMIOCTLU *) 0,
										smControlDevice,
										0		 				);
	}
	else
	{
		result = ERR_SM_DEVERR;
	}

	return result;
}


/*
 * SM_RESET_MODULE
 *
 * Reset a SM module ready for a firmware download.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_reset_module
( int module )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSM_INT			localModule;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		/*
		 *   For maximum portability do not
		 *   take address of value stack parameters.
		 */
		localModule = module;

		result = smd_ioctl_dev_generic( SMIO_RESET_MODULE,
										(SMIOCTLU *) &localModule,
										smControlDevice,
										sizeof(localModule)			);
	}
	else
	{
		result = ERR_SM_DEVERR;
	}

	return result;
}


/*
 * SM_DOWNLOAD_FMW 
 * 
 * Download firmware to SM module.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_download_fmw
( struct sm_download_parms* downloadp )
{
	int  								result;
	tSMFileHandle  						readh;
	int  								size;
	struct sm_download_buffer_parms 	buffer_parms;
	struct sm_download_complete_parms 	complete_parms;
	int									bufferSize;
	int									complete_result;
	tSMDevHandle 						smControlDevice;
	int									i;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = 0;

		readh = smd_file_open(downloadp->filename);

		if (readh > 0)
		{
			result = smd_ioctl_dev_generic(	SMIO_DOWNLOAD_INIT,
											(SMIOCTLU *) (&downloadp->module),
											smControlDevice,
											sizeof(downloadp->module)			 );

			if (result == 0)
			{
				i = 0;

				bufferSize = sizeof(buffer_parms.fwbuffer);

				size = smd_file_read(readh,&buffer_parms.fwbuffer[0],bufferSize);

				if (size > 0)
				{
					while (result == 0)
					{
						if (size != 0)
						{
							buffer_parms.module  = downloadp->module;
							buffer_parms.length  = size;

							result = smd_ioctl_dev_generic(	SMIO_DOWNLOAD_BUFFER,
															(SMIOCTLU *) (&buffer_parms),
															smControlDevice,
															sizeof(struct sm_download_buffer_parms) );

							if ((size == bufferSize) && (result == 0))
							{
								size = smd_file_read(readh,&buffer_parms.fwbuffer[0],bufferSize);
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

					/*
					 * Do this even on failure.
					 */
					complete_parms.module = downloadp->module;
					complete_parms.abort  = (result != 0);
					complete_parms.id	  = downloadp->id;

					complete_result = smd_ioctl_dev_generic(	SMIO_DOWNLOAD_COMPLETE,
																(SMIOCTLU *) (&complete_parms),
																smControlDevice,
																sizeof(struct sm_download_complete_parms) );

					if (result == 0)
					{
						result = complete_result;
					}
				}
				else
				{
					result = ERR_SM_FILE_FORMAT;
				}

				smd_file_close(readh);
			}
		}
		else
		{
			result = ERR_SM_FILE_ACCESS;
		}
	}
	else
	{
		result = ERR_SM_DEVERR;
	}

	return result;
}


/*
 * SM_GET_FIRMWARE_CAPS 
 * 
 * Get capabilities of firmware running on module.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_get_firmware_caps
( struct sm_fwcaps_parms* fwcapsp )
{
	int 					result;
	tSMDevHandle 			smControlDevice;
	SM_FWCAPS_BUFFER_PARMS	capsBuffer;

	fwcapsp->caps_length = 0;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		capsBuffer.module = fwcapsp->module;
		capsBuffer.length = sizeof(capsBuffer.buffer);
		capsBuffer.offset = 0;

		do
		{
			result = smd_ioctl_dev_generic( 	SMIO_FIRMWARE_CAPS_BUFFER, 
					                        	(SMIOCTLU *) &capsBuffer,
												smControlDevice,
												sizeof(SM_FWCAPS_BUFFER_PARMS) );

			if ((result == 0) && (capsBuffer.length > 0))
			{
				memcpy(&(fwcapsp->caps[capsBuffer.offset]),&capsBuffer.buffer[0],capsBuffer.length);

				capsBuffer.offset += capsBuffer.length;

				fwcapsp->caps_length += capsBuffer.length;
			}
		} while ((result == 0) && (capsBuffer.length == sizeof(capsBuffer.buffer)));
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CONFIG_MODULE_SWITCHING 
 * 
 * Configure way external bus timeslots allocated by module.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_config_module_switching
( struct sm_config_module_sw_parms* configmoduleswp )
{
	int 							result;
	tSMDevHandle 					smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_generic( 	SMIO_CONFIG_MODULE_SW, 
										   	(SMIOCTLU *) configmoduleswp,
											smControlDevice,
											sizeof(struct sm_config_module_sw_parms) );
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_GET_MODULE_INFO 
 * 
 * Get information pertaining to current state of module.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_get_module_info
( struct sm_module_info_parms* moduleinfop )
{
	int 							result;
	tSMDevHandle 					smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_generic( 	SMIO_GET_MODULE_INFO, 
										   	(SMIOCTLU *) moduleinfop,
											smControlDevice,
											sizeof(struct sm_module_info_parms) );
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_GET_EV_MECH
 *
 * Get event mechanism 
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_get_ev_mech
( void )
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_generic( 	SMIO_GET_EV_MECH, 
							                (SMIOCTLU *) 0,
											smControlDevice,
											0					 		);	
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_GET_CHANNEL_IX
 *
 * Get integer index for channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_get_channel_ix
( tSMChannelId channel )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSMChannelId	localChannel;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		/*
		 *   For maximum portability do not
		 *   take address of value stack parameters.
		 */
		localChannel = channel;

		result = smd_ioctl_dev_generic( 	SMIO_GET_CHANNEL_IX, 
							                (SMIOCTLU *) &localChannel,
											(tSMDevHandle) channel,
											sizeof(localChannel) 		);	
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CHANNEL_ALLOC
 *
 * Allocate a channel
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_channel_alloc( struct sm_channel_alloc_parms* channelp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSMDevHandle 	smChannelDevice;
	tSMChannelId	localChannel;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		/*
		 * Open a channel to obtain handle for reads and writes.
		 */
		result = smd_ioctl_dev_generic( 	SMIO_CHANNEL_ALLOC, 
											(SMIOCTLU *) channelp,
											smControlDevice,
											sizeof(struct sm_channel_alloc_parms) );

		if (result == 0)
		{
			if ((smChannelDevice = smd_open_chnl_dev(channelp->channel)) == kSMNullDevHandle)
			{
				result = ERR_SM_NO_RESOURCES;

				localChannel = (tSMChannelId) ((((int) (channelp->channel)) & 0xfff)-1);

				smd_ioctl_dev_generic( 	SMIO_CHANNEL_RELEASE, 
									   (SMIOCTLU *) &localChannel,
										smControlDevice,
										sizeof(channelp->channel) 		  );
			}
		}
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	if (result == 0)
	{
		/*
		 * Must let driver know application id (handle) for channel.
		 * We need to do this so that when an API call is made with 
		 * an "any channel parameter", the driver
		 * will be able to specify a handle for the channel it actually occured on.
		 *
		 * Note in NT handles only valid for same process that opened it.
		 */
		channelp->channel = (tSMChannelId) smChannelDevice;

		smd_ioctl_dev_generic( 	SMIO_STORE_APP_CHANNEL_ID, 
								(SMIOCTLU *) &channelp->channel,
								smChannelDevice,
								sizeof(channelp->channel) 		  );
	}
	else
	{
		channelp->channel = kSMNullChannelId;
	}

	return result;
}


/*
 * SM_CHANNEL_ALLOC_PLACED
 *
 * Allocate a channel on a specific module
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_channel_alloc_placed( struct sm_channel_alloc_placed_parms* channelp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSMDevHandle 	smChannelDevice;
	tSMChannelId	localChannel;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		/*
		 * Open a channel to obtain handle for reads and writes.
		 */
		result = smd_ioctl_dev_generic( 	SMIO_CHANNEL_ALLOC_PLACED, 
											(SMIOCTLU *) channelp,
											smControlDevice,
											sizeof(struct sm_channel_alloc_placed_parms) );

		if (result == 0)
		{
			if ((smChannelDevice = smd_open_chnl_dev(channelp->channel)) == kSMNullDevHandle)
			{
				result = ERR_SM_NO_RESOURCES;

				localChannel = (tSMChannelId) ((((int) (channelp->channel)) & 0xfff)-1);

				smd_ioctl_dev_generic( 	SMIO_CHANNEL_RELEASE, 
									   (SMIOCTLU *) &localChannel,
										smControlDevice,
										sizeof(channelp->channel) 		  );
			}
		}
	}
	else
    {
		channelp->channel = kSMNullChannelId;
		result            = ERR_SM_DEVERR;
    }

	if (result == 0)
	{
		/*
		 * Must let driver know application id (handle) for channel.
		 * We need to do this so that when an API call is made with 
		 * an "any channel parameter", the driver
		 * will be able to specify a handle for the channel it actually occured on.
		 *
		 * Note in NT handles only valid for same process that opened it.
		 */
		channelp->channel = (tSMChannelId) smChannelDevice;

		smd_ioctl_dev_generic( 	SMIO_STORE_APP_CHANNEL_ID, 
								(SMIOCTLU *) &channelp->channel,
								smChannelDevice,
								sizeof(channelp->channel) 		  );
	}

	return result;
}


/*
 * SM_SWITCH_CHANNEL_OUTPUT
 *
 * Switch a previously allocated channel to specific
 * (as opposed to driver assigned) external bus timeslot.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_switch_channel_output 
(struct sm_switch_channel_parms* switchp)
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_generic( 	SMIO_SWITCH_CHANNEL_OUTPUT, 
									    	(SMIOCTLU *) switchp,
											(tSMDevHandle) switchp->channel,
											sizeof(struct sm_switch_channel_parms) );
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}

/*
 * SM_SWITCH_CHANNEL_INPUT
 *
 * Switch a previously allocated channel to specific
 * (as opposed to driver assigned) external bus timeslot.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_switch_channel_input 
(struct sm_switch_channel_parms* switchp)
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_generic( 	SMIO_SWITCH_CHANNEL_INPUT, 
									    	(SMIOCTLU *) switchp,
											(tSMDevHandle) switchp->channel,
											sizeof(struct sm_switch_channel_parms) );
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_SWITCH_CHANNEL_IX
 *
 * Switch a previously allocated channel to specific
 * (as opposed to driver assigned) external bus timeslot.
 * Channel referenced through channel index
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_switch_channel_ix 
(struct sm_switch_channel_ix_parms* switchp)
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_generic( 	SMIO_SWITCH_CHANNEL_IX, 
									    	(SMIOCTLU *) switchp,
											(tSMDevHandle) smControlDevice,
											sizeof(struct sm_switch_channel_ix_parms) );
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_FIND_CHANNEL
 *
 * Locate a channel given its switch matrix/bus termination point.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_find_channel 
(struct sm_find_channel_parms* findp)
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_generic( 	SMIO_FIND_CHANNEL, 
									    	(SMIOCTLU *) findp,
											smControlDevice,
											sizeof(struct sm_find_channel_parms) );
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CHANNEL_RELEASE
 *
 * Release a previously allocated channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_channel_release
( tSMChannelId channel )
{
	int 						result;
	tSMDevHandle 				smControlDevice;
	tSMChannelId				localChannel;

	smControlDevice = smd_open_ctl_dev();

	if (smControlDevice != kSMNullDevHandle)
	{
		/*
		 *   For maximum portability do not
		 *   take address of value stack parameters.
		 */
		localChannel = (tSMDevHandle) (sm_get_channel_ix(channel));

		/*
		 * Do this first to release associated device in case 
		 * O/S needs to delete channel device.
		 */
		smd_close_chnl_dev(channel);

		result = smd_ioctl_dev_generic( 	SMIO_CHANNEL_RELEASE, 
										    (SMIOCTLU *) &localChannel,
											smControlDevice,
											sizeof(localChannel) 			);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_GET_CHANNEL_TYPE
 *
 * Get type of channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_get_channel_type
( tSMChannelId channel )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSMChannelId	localChannel;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		/*
		 *   For maximum portability do not
		 *   take address of value stack parameters.
		 */
		localChannel = channel;

		result = smd_ioctl_dev_generic( 	SMIO_GET_CHANNEL_TYPE, 
							                (SMIOCTLU *) &localChannel,
											(tSMDevHandle) channel,
											sizeof(localChannel) 		);	
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CHANNEL_VALIDATE_ID
 *
 * Validate correctness of channel id.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_channel_validate_id
( tSMChannelId channel )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSMChannelId	localChannel;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		/*
		 *   For maximum portability do not
		 *   take address of value stack parameters.
		 */
		localChannel = channel;

		result = smd_ioctl_dev_generic( 	SMIO_CHANNEL_VALIDATE_ID, 
							                (SMIOCTLU *) &localChannel,
											smControlDevice,
											sizeof(localChannel) 		);	
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_GET_CHANNEL_MODULE_IX
 *
 * Get integer index for module hosting channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_get_channel_module_ix
( tSMChannelId channel )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSMChannelId	localChannel;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		/*
		 *   For maximum portability do not
		 *   take address of value stack parameters.
		 */
		localChannel = channel;

		result = smd_ioctl_dev_generic( 	SMIO_GET_CHANNEL_MODULE_IX, 
							                (SMIOCTLU *) &localChannel,
											(tSMDevHandle) channel,
											sizeof(localChannel) 		);	
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_GET_CHANNEL_IX_MODULE_IX
 *
 * Get integer index for module hosting channel with given index.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_get_channel_ix_module_ix
( tSM_INT channel_ix )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSM_INT			localChannelIx;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		/*
		 *   For maximum portability do not
		 *   take address of value stack parameters.
		 */
		localChannelIx = channel_ix;

		result = smd_ioctl_dev_generic( 	SMIO_GET_CHANNEL_IX_MODULE_IX, 
							                (SMIOCTLU *) &localChannelIx,
											(tSMDevHandle) smControlDevice,
											sizeof(localChannelIx) 				);	
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_GET_MODULE_CARD_IX
 *
 * Get integer index for card hosting module.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_get_module_card_ix
( tSM_INT module )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSM_INT			localModule;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		/*
		 *   For maximum portability do not
		 *   take address of value stack parameters.
		 */
		localModule = module;

		result = smd_ioctl_dev_generic( SMIO_GET_MODULE_CARD_IX,
										(SMIOCTLU *) &localModule,
										smControlDevice,
										sizeof(localModule)			);
	}
	else
	{
		result = ERR_SM_DEVERR;
	}

	return result;
}


/*
 * SM_GET_CARD_SWITCH_IX
 *
 * Get integer index for switch driver associated with given card.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_get_card_switch_ix( tSM_INT card )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSM_INT			localCard;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		/*
		 *   For maximum portability do not
		 *   take address of value stack parameters.
		 */
		localCard = card;

		result = smd_ioctl_dev_generic( SMIO_GET_CARD_SWITCH_IX,
										(SMIOCTLU *) &localCard,
										smControlDevice,
										sizeof(localCard)			);
	}
	else
	{
		result = ERR_SM_DEVERR;
	}

	return result;
}


/*
 * SM_CHANNEL_INFO
 *
 * Obtain information pertaining to a channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_channel_info
( struct sm_channel_info_parms* infop )
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_generic( 	SMIO_CHANNEL_INFO, 
										   	(SMIOCTLU *) infop,
											(tSMDevHandle) infop->channel,
											sizeof(struct sm_channel_info_parms) );
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CHANNEL_INFO
 *
 * Obtain information pertaining to a channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_channel_ix_info( struct sm_channel_ix_info_parms* infop )
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_generic( 	SMIO_CHANNEL_IX_INFO, 
										   	(SMIOCTLU *) infop,
											(tSMDevHandle) smControlDevice,
											sizeof(struct sm_channel_ix_info_parms) );
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CHANNEL_SET_EVENT
 *
 * Associate an operating system event with a channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_channel_set_event
( struct sm_channel_set_event_parms* eventp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSMDevHandle 	smEvDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		smEvDevice = (tSMDevHandle) ((eventp->channel != kSMNullChannelId) ? ((tSMDevHandle) eventp->channel) : ((tSMDevHandle) smControlDevice));

		result = smd_ioctl_dev_generic( SMIO_CHANNEL_SET_EVENT, 
									  	(SMIOCTLU *) eventp,
										(tSMDevHandle) smEvDevice,
										sizeof(struct sm_channel_set_event_parms) );
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_RESET_CHANNEL
 *
 * Reset channel to default state.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_reset_channel
( tSMChannelId channel )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSMChannelId 	localChannel;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		/*
		 *   For maximum portability do not
		 *   take address of value stack parameters.
		 */
		localChannel = channel;

		result = smd_ioctl_dev_generic( 	SMIO_RESET_CHANNEL,
				                            (SMIOCTLU *) &localChannel,
											(tSMDevHandle) channel,
											sizeof(localChannel) 		);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_ENABLE_LICENCE
 *
 * Enable licence for f/w.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_enable_licence
( struct sm_enable_licence_parms* licencep )
{
	int 							result;
	tSMDevHandle 					smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_generic( 	SMIO_ENABLE_LICENCE, 
										   	(SMIOCTLU *) licencep,
											smControlDevice,
											sizeof(struct sm_enable_licence_parms) );
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


#ifdef _PROSDLL
ACUDLL 
#endif
int sm_channel_ix_dump
( struct sm_channel_ix_dump_parms* dumpp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_generic( 	SMIO_CHANNEL_IX_DUMP, 
							                (SMIOCTLU *) dumpp,
											(tSMDevHandle) smControlDevice,
											sizeof(struct sm_channel_ix_dump_parms) );	
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


#ifdef SM_CRACK_API

void sm_crack_result( int result, char* buffer )
{
	char* p;

	p = buffer;

	switch(result)
	{
		case ERR_SM_DEVERR:
			sprintf(p,"ERR_SM_DEVERR");
			break;
		case ERR_SM_PENDING:
			sprintf(p,"ERR_SM_PENDING");
			break;
		case ERR_SM_MORE:
			sprintf(p,"ERR_SM_MORE");
			break;
		case ERR_SM_NO_ACTION:
			sprintf(p,"ERR_SM_NO_ACTION");
			break;
		case ERR_SM_NOT_IMPLEMENTED:
			sprintf(p,"ERR_SM_NOT_IMPLEMENTED");
			break;
		case ERR_SM_BAD_PARAMETER:
			sprintf(p,"ERR_SM_BAD_PARAMETER");
			break;
		case ERR_SM_NO_SUCH_MODULE:
			sprintf(p,"ERR_SM_NO_SUCH_MODULE");
			break;
		case ERR_SM_MODULE_ACCESS:
			sprintf(p,"ERR_SM_MODULE_ACCESS");
			break;
		case ERR_SM_FILE_ACCESS:
			sprintf(p,"ERR_SM_FILE_ACCESS");
			break;
		case ERR_SM_FILE_FORMAT:
			sprintf(p,"ERR_SM_FILE_FORMAT");
			break;
		case ERR_SM_DOWNLOAD:
			sprintf(p,"ERR_SM_DOWNLOAD");
			break;
		case ERR_SM_NO_RESOURCES:
			sprintf(p,"ERR_SM_NO_RESOURCES");
			break;
		case ERR_SM_FIRMWARE_NOT_RUNNING:
			sprintf(p,"ERR_SM_FIRMWARE_NOT_RUNNING");
			break;
		case ERR_SM_MODULE_ALREADY_RUNNING:
			sprintf(p,"ERR_SM_MODULE_ALREADY_RUNNING");
			break;
		case ERR_SM_NO_SUCH_CHANNEL:
			sprintf(p,"ERR_SM_NO_SUCH_CHANNEL");
			break;
		case ERR_SM_NO_DATA_AVAILABLE:
			sprintf(p,"ERR_SM_NO_DATA_AVAILABLE");
			break;
		case ERR_SM_NO_RECORD_IN_PROGRESS:
			sprintf(p,"ERR_SM_NO_RECORD_IN_PROGRESS");
			break;
		case ERR_SM_NO_CAPACITY:
			sprintf(p,"ERR_SM_NO_CAPACITY");
			break;
		case ERR_SM_NO_REPLAY_IN_PROGRESS:
			sprintf(p,"ERR_SM_NO_REPLAY_IN_PROGRESS");
			break;
		case ERR_SM_BAD_DATA_LENGTH:
			sprintf(p,"ERR_SM_BAD_DATA_LENGTH");
			break;
		case ERR_SM_WRONG_CHANNEL_STATE:
			sprintf(p,"ERR_SM_WRONG_CHANNEL_STATE");
			break;
		case ERR_SM_WRONG_CHANNEL_TYPE:
			sprintf(p,"ERR_SM_WRONG_CHANNEL_TYPE");
			break;
		case ERR_SM_NO_SUCH_GROUP:
			sprintf(p,"ERR_SM_NO_SUCH_GROUP");
			break;
		case ERR_SM_NO_SUCH_FIRMWARE:
			sprintf(p,"ERR_SM_NO_SUCH_FIRMWARE");
			break;
		case ERR_SM_NO_SUCH_DIGIT:
			sprintf(p,"ERR_SM_NO_SUCH_DIGIT");
			break;
		case ERR_SM_CHANNEL_ALLOCATED:
			sprintf(p,"ERR_SM_CHANNEL_ALLOCATED");
			break;
		case ERR_SM_NO_DIGIT:
			sprintf(p,"ERR_SM_NO_DIGIT");
			break;
		case ERR_SM_NOT_SAME_MODULE:
			sprintf(p,"ERR_SM_NOT_SAME_MODULE");
			break;
		case ERR_SM_INCOMPATIBLE_DRIVER:
			sprintf(p,"ERR_SM_INCOMPATIBLE_DRIVER");
			break;
		case ERR_SM_INCOMPATIBLE_APP:
			sprintf(p,"ERR_SM_INCOMPATIBLE_APP");
			break;
		case ERR_SM_WRONG_MODULE_TYPE:
			sprintf(p,"ERR_SM_WRONG_MODULE_TYPE");
			break;
		case ERR_SM_NOT_SAME_GROUP:
			sprintf(p,"ERR_SM_NOT_SAME_GROUP");
			break;
		case ERR_SM_NO_LICENCE:
			sprintf(p,"ERR_SM_NO_LICENCE");
			break;
		case ERR_SM_WRONG_FIRMWARE_TYPE:
			sprintf(p,"ERR_SM_WRONG_FIRMWARE_TYPE");
			break;
		case ERR_SM_FIRMWARE_PROBLEM:
			sprintf(p,"ERR_SM_FIRMWARE_PROBLEM");
			break;
		case ERR_SM_OS_RESOURCE_PROBLEM:
			sprintf(p,"ERR_SM_OS_RESOURCE_PROBLEM");
			break;
		case ERR_SM_WRONG_MODULE_STATE:
			sprintf(p,"ERR_SM_WRONG_MODULE_STATE");
			break;
		case ERR_SM_NO_ASSOCIATED_SWITCH:
			sprintf(p,"ERR_SM_NO_ASSOCIATED_SWITCH");
			break;
		default:
			sprintf(p,"%d",result);
			break;
	}
}

int sm_crack_generic_ioctl( int ioctl, char* buffer )
{
	int		result;
	char* 	p;

	result = 0;

	p = buffer;

	switch(ioctl)
	{
		case SMIO_TRACE_CONTROL:
       		sprintf(buffer,"TRACE_CONTROL");
			break;

		case SMIO_GET_MODULES:
        	sprintf(buffer,"GET_MODULES");
			break;

		case SMIO_RESET_MODULE:
        	sprintf(buffer,"RESET_MODULE");
			break;

		case SMIO_DOWNLOAD_INIT:
	        sprintf(buffer,"DOWNLOAD_INIT");
			break;

		case SMIO_DOWNLOAD_BUFFER:
	        sprintf(buffer,"DOWNLOAD_BUFFER");
			break;

		case SMIO_DOWNLOAD_COMPLETE:
	        sprintf(buffer,"DOWNLOAD_COMPLETE");
			break;

		case SMIO_FIRMWARE_CAPS_BUFFER:
	        sprintf(buffer,"FIRMWARE_CAPS_BUFFER");
			break;

		case SMIO_GET_MODULE_INFO:
        	sprintf(buffer,"GET_MODULE_INFO");
			break;

		case SMIO_CONFIG_MODULE_SW:
        	sprintf(buffer,"CONFIG_MODULE_SW");
			break;

		case SMIO_GET_CHANNEL_IX_MODULE_IX:
        	sprintf(buffer,"GET_CHANNEL_IX_MODULE_IX");
			break;

		case SMIO_GET_EV_MECH:
        	sprintf(buffer,"GET_EV_MECH");
			break;

		case SMIO_CHANNEL_ALLOC:
        	sprintf(buffer,"CHANNEL_ALLOC");
			break;

		case SMIO_STORE_APP_CHANNEL_ID:
        	sprintf(buffer,"STORE_APP_CHANNEL_ID");
			break;

		case SMIO_CHANNEL_SET_EVENT:
        	sprintf(buffer,"CHANNEL_SET_EVENT");
			break;

		case SMIO_CHANNEL_RELEASE:
        	sprintf(buffer,"CHANNEL_RELEASE");
			break;

		case SMIO_CHANNEL_INFO:
        	sprintf(buffer,"CHANNEL_INFO");
			break;

		case SMIO_RESET_CHANNEL:
        	sprintf(buffer,"RESET_CHANNEL");
			break;

		case SMIO_GET_CHANNEL_IX:
        	sprintf(buffer,"GET_CHANNEL_IX");
			break;

		case SMIO_SWITCH_CHANNEL_OUTPUT:
        	sprintf(buffer,"SWITCH_CHANNEL_OUTPUT");
			break;

		case SMIO_SWITCH_CHANNEL_INPUT:
        	sprintf(buffer,"SWITCH_CHANNEL_INPUT");
			break;

		case SMIO_GET_RECOGNISED_GN:
       		sprintf(buffer,"GET_RECOGNISED_GN");
			break;

		case SMIO_GET_CARDS:
        	sprintf(buffer,"GET_CARDS");
			break;

		case SMIO_GET_CARD_INFO:
        	sprintf(buffer,"GET_CARD_INFO");
			break;

		case SMIO_GET_CHANNEL_MODULE_IX:
        	sprintf(buffer,"GET_CHANNEL_MODULE_IX");
			break;

		case SMIO_GET_MODULE_CARD_IX:
        	sprintf(buffer,"GET_MODULE_CARD_IX");
			break;

		case SMIO_GET_DRIVER_INFO:
       		sprintf(buffer,"GET_DRIVER_INFO");
			break;

		case SMIO_CHANNEL_VALIDATE_ID:
        	sprintf(buffer,"CHANNEL_VALIDATE_ID");
			break;

		case SMIO_GET_CARD_REV:
        	sprintf(buffer,"GET_CARD_REV");
			break;

		case SMIO_GET_CHANNEL_TYPE:
        	sprintf(buffer,"GET_CHANNEL_TYPE");
			break;

		case SMIO_CHANNEL_ALLOC_PLACED:
        	sprintf(buffer,"CHANNEL_ALLOC_PLACED");
			break;

		case SMIO_GET_CARD_SWITCH_IX:
        	sprintf(buffer,"GET_CARD_SWITCH_IX");
			break;
			
		case SMIO_ENABLE_LICENCE:
        	sprintf(buffer,"ENABLE_LICENCE");
			break;

		case SMIO_FIND_CHANNEL:
        	sprintf(buffer,"FIND_CHANNEL");
			break;

		case SMIO_CHANNEL_IX_INFO:
        	sprintf(buffer,"CHANNEL_IX_INFO");
			break;

		case SMIO_SWITCH_CHANNEL_IX:
        	sprintf(buffer,"SWITCH_CHANNEL_IX");
			break;

		case SMIO_CHANNEL_IX_DUMP:
        	sprintf(buffer,"CHANNEL_IX_DUMP");
			break;

		default:
			result = -1;
			break;
	}

	return result;
}

#endif


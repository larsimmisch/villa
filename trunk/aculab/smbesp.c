/*------------------------------------------------------------*/
/* Copyright ACULAB plc. (c) 1996-2000                        */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : smbesp.c                               */
/*                                                            */
/*           Purpose : SHARC module control library programs  */
/*                     for Basic and Enhanced Speech          */
/*                     processing.                            */
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

#include "smbesp.h"
#include "smosintf.h"
#include <math.h>

#include <stdio.h>

#define kFWLIBVersion ((kBESPFWSpecificAPIId<<12)+(kBESPVersionMaj<<8)+kBESPVersionMin)


/*
 *******************************************************************
 * Entry points for Basic/Enhanced Speech Processing Low Level API.*
  *******************************************************************
 */

/*
 * SM_ADD_OUTPUT_FREQ 
 * 
 * Add component output frequency to repetoire for module.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_add_output_freq( struct sm_output_freq_parms* freqp )
{
	int 							result;
	tSMDevHandle 					smControlDevice;
	struct sm_output_freq_drv_parms drv_parms;
	double 							pi = 3.141532768;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		*((tSMIEEE32Bit754854Float*) (&drv_parms.freq[0])) = (tSMIEEE32Bit754854Float) 
			(2.0 * cos((2.0*pi*freqp->freq)/8000.0));

		*((tSMIEEE32Bit754854Float*) (&drv_parms.amplitude[0])) = (tSMIEEE32Bit754854Float) 
			(	sin((2.0*pi*freqp->freq)/8000.0) 
			  * 2853.4 
			  * exp(2.302585092994*((freqp->amplitude)/20.0)) );

		drv_parms.module = freqp->module;
		drv_parms.id     = 0;

		result = smd_ioctl_dev_fwapi( 	BESPIO_ADD_OUTPUT_FREQ, 
			                        	(SMIOCTLU *) &drv_parms,
										smControlDevice,
										sizeof(struct sm_output_freq_drv_parms),
										drv_parms.module,
										kFWLIBVersion								);

		if (result == 0)
		{
			freqp->id = drv_parms.id;
		}
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_ADD_OUTPUT_TONE 
 * 
 * Add dual-component tone to repetoire for module.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_add_output_tone( struct sm_output_tone_parms* tonep )
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		tonep->id = 0;

		result = smd_ioctl_dev_fwapi( 	BESPIO_ADD_OUTPUT_TONE, 
			                        	(SMIOCTLU *) tonep,
										smControlDevice,
										sizeof(struct sm_output_tone_parms),
										tonep->module,
										kFWLIBVersion								);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_ADD_INPUT_FREQ_COEFFS 
 * 
 * Add pair of rejection limits for frequency recognition to module.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_add_input_freq_coeffs( struct sm_input_freq_coeffs_parms* freqcoeffsp )
{
	int										result;
	tSMDevHandle 							smControlDevice;
	struct sm_input_freq_coeffs_drv_parms 	drv_parms;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		*((tSMIEEE32Bit754854Float*) (&drv_parms.upper_limit[0])) = (tSMIEEE32Bit754854Float) freqcoeffsp->upper_limit;
		*((tSMIEEE32Bit754854Float*) (&drv_parms.lower_limit[0])) = (tSMIEEE32Bit754854Float) freqcoeffsp->lower_limit;

		drv_parms.id     = 0;
		drv_parms.module = freqcoeffsp->module;

		result = smd_ioctl_dev_fwapi( 	BESPIO_ADD_INPUT_FREQ_COEFFS, 
			                        	(SMIOCTLU *) &drv_parms,
										smControlDevice,
										sizeof(struct sm_input_freq_coeffs_drv_parms),
										drv_parms.module,
										kFWLIBVersion									);

		if (result == 0)
		{
			freqcoeffsp->id = drv_parms.id;
		}
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_ADD_INPUT_TONE_SET 
 * 
 * Add set of input dual-tones that are to be recognised by module.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_add_input_tone_set( struct sm_input_tone_set_parms* tonesetp )
{
	int									result;
	tSMDevHandle 						smControlDevice;
	struct sm_input_tone_set_drv_parms 	drv_parms;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		*((tSMIEEE32Bit754854Float*) (&drv_parms.req_third_peak[0])) 			= (tSMIEEE32Bit754854Float) tonesetp->req_third_peak;
		*((tSMIEEE32Bit754854Float*) (&drv_parms.req_signal_to_noise_ratio[0])) = (tSMIEEE32Bit754854Float) tonesetp->req_signal_to_noise_ratio;
		*((tSMIEEE32Bit754854Float*) (&drv_parms.req_minimum_power[0])) 		= (tSMIEEE32Bit754854Float) tonesetp->req_minimum_power;
		*((tSMIEEE32Bit754854Float*) (&drv_parms.req_twist_for_dual_tone[0])) 	= (tSMIEEE32Bit754854Float) tonesetp->req_twist_for_dual_tone;

		drv_parms.id     					 = 0;
		drv_parms.module                     = tonesetp->module;
		drv_parms.band1_first_freq_coeffs_id = tonesetp->band1_first_freq_coeffs_id;
		drv_parms.band1_freq_count           = tonesetp->band1_freq_count;
		drv_parms.band2_first_freq_coeffs_id = tonesetp->band2_first_freq_coeffs_id;
		drv_parms.band2_freq_count           = tonesetp->band2_freq_count;

		result = smd_ioctl_dev_fwapi( 	BESPIO_ADD_INPUT_TONE_SET, 
			                        	(SMIOCTLU *) &drv_parms,
										smControlDevice,
										sizeof(struct sm_input_tone_set_drv_parms),
										drv_parms.module,
										kFWLIBVersion									);

		if (result == 0)
		{
			tonesetp->id = drv_parms.id;
		}
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_ADD_INPUT_TONE_SET 
 * 
 * Adjust set of input dual-tones that are to be recognised by module.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_adjust_input_tone_set( struct sm_adjust_tone_set_parms* tonesetp )
{
	int									result;
	tSMDevHandle 						smControlDevice;
	struct sm_adjust_tone_set_drv_parms drv_parms;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		drv_parms.module        = tonesetp->module;
		drv_parms.tone_set_id   = tonesetp->tone_set_id;
		drv_parms.parameter_id  = tonesetp->parameter_id;

		*((tSMIEEE32Bit754854Float*) (&drv_parms.fp_value[0])) = (tSMIEEE32Bit754854Float) tonesetp->parameter_value.fp_value;

		if (	(tonesetp->parameter_id == kAdjustToneSetFPParamIdStartFreq)
			||	(tonesetp->parameter_id == kAdjustToneSetFPParamIdStopFreq)  )
		{
			drv_parms.int_value = (tSM_INT)(tonesetp->parameter_value.fp_value/31.25);
		}
		else
		{
			drv_parms.int_value = tonesetp->parameter_value.int_value;;
		}

		result = smd_ioctl_dev_fwapi( 	BESPIO_ADJUST_INPUT_TONE_SET, 
			                        	(SMIOCTLU *) &drv_parms,
										smControlDevice,
										sizeof(struct sm_adjust_tone_set_drv_parms),
										drv_parms.module,
										kFWLIBVersion									);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_RESET_INPUT_CPTONES 
 * 
 * Reset modules repertoire of recognised call progress tones.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_reset_input_cptones( struct sm_reset_input_cptones_parms* resetp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_fwapi( 	BESPIO_RESET_INPUT_CPTONES,
					                    (SMIOCTLU *) resetp,
										smControlDevice,
										sizeof(struct sm_reset_input_cptones_parms),
										resetp->module,
										kFWLIBVersion									);
	}
	else
	{
		result = ERR_SM_DEVERR;
	}

	return result;
}


/*
 * SM_ADD_INPUT_CPTONE 
 * 
 * Add a new call progress tone to repertoire of those recognised by module.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_add_input_cptone( struct sm_input_cptone_parms* cptonep )
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_fwapi( 	BESPIO_ADD_INPUT_CPTONE, 
			                        	(SMIOCTLU *) cptonep,
										smControlDevice,
										sizeof(struct sm_input_cptone_parms),
										cptonep->module,
										kFWLIBVersion									);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_REPLAY_START
 *
 * Initiate replay on output channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_replay_start( struct sm_replay_parms* replayp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSM_INT			channelIx;
	tSMChannelId	background;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		channelIx = -1;

		if ((background = replayp->background) != kSMNullChannelId)
		{
			channelIx = sm_get_channel_ix(background);
		}

		replayp->background = (tSMChannelId) channelIx;

		result = smd_ioctl_dev_fwapi( 	BESPIO_REPLAY_START1, 
			                        	(SMIOCTLU *) replayp,
										(tSMDevHandle) replayp->channel,
										sizeof(struct sm_replay_parms),
										-1,
										kFWLIBVersion						);

		replayp->background = background;
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_PUT_REPLAY_DATA
 *
 * Output replay data to channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_put_replay_data( struct sm_ts_data_parms* datap )
{
	int 					result;
	int 					statusResult;
	tSMDevHandle 			smControlDevice;
	SM_REPLAY_STATUS_PARMS	statusParms;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_write_dev((tSMDevHandle) datap->channel,datap->data,datap->length);

		if (result == ERR_SM_DEVERR)
		{
			/*
			 * As difficult to signal back explicit errors through O/S write,
			 * check problem not due to no replay in progress through
			 * replay underrun.  If so, do not signal device error.
			 */
			statusParms.channel = datap->channel;

			statusResult = smd_ioctl_dev_fwapi( 	BESPIO_REPLAY_STATUS, 
			                        				(SMIOCTLU *) (&statusParms),
													(tSMDevHandle) (datap->channel),
													sizeof(struct sm_replay_status_parms),
													-1,
													kFWLIBVersion							);

			if ((statusResult == 0) && (statusParms.status == kSMReplayStatusComplete))
			{
				result = 0;
			} 
		}
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_PUT_LAST_REPLAY_DATA
 *
 * Output replay data to channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_put_last_replay_data( struct sm_ts_data_parms* datap )
{
	int 							result;
	tSMDevHandle 					smControlDevice;
	SM_LAST_TS_DATA_LENGTH_PARMS	lastDataLength;
	tSM_UT8							lastBlockBuffer[128];
	SM_TS_DATA_PARMS				dataBlock1;
	SM_TS_DATA_PARMS				dataBlock2;
	int								padding;
	int								excess;
	int								fillCount;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		lastDataLength.channel = datap->channel;
		lastDataLength.length  = datap->length;

		result = smd_ioctl_dev_fwapi( 	BESPIO_PUT_LAST_REPLAY_DATA, 
		                   				(SMIOCTLU *) (&lastDataLength),
										(tSMDevHandle) (datap->channel),
										sizeof(struct sm_last_ts_data_length_parms),
										-1,
										kFWLIBVersion									);

		if (result == 0)
		{
			excess = (datap->length % 4);

			memcpy(&dataBlock1,datap,sizeof(struct sm_ts_data_parms));

			dataBlock2.channel = datap->channel;
			dataBlock2.length  = 0;
			dataBlock2.data	   = (char*) (&lastBlockBuffer[0]);
	
			fillCount = lastDataLength.fill_count;

			if (excess)
			{
				dataBlock1.length -= excess;

				dataBlock2.length  = excess;

				memcpy(&lastBlockBuffer[0],(datap->data + dataBlock1.length),excess);
			}

			if (dataBlock1.length != 0)
			{
				result = sm_put_replay_data(&dataBlock1);
			}

			while ((result == 0) && ((fillCount > 0) || (dataBlock2.length != 0)))
			{
				padding = fillCount;

				if (padding > (int)(sizeof(lastBlockBuffer)-dataBlock2.length))
				{
					padding = (sizeof(lastBlockBuffer)-dataBlock2.length);
				}

				memset(&lastBlockBuffer[dataBlock2.length],lastDataLength.fill_octet,padding);

				dataBlock2.length += padding;

				result = sm_put_replay_data(&dataBlock2);

				dataBlock2.length = 0;
				fillCount -= padding;
			}
		}
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_REPLAY_STATUS
 *
 * Obtain status of current replay on channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_replay_status( struct sm_replay_status_parms* statusp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSMDevHandle	smStatusDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		smStatusDevice = (tSMDevHandle) ((statusp->channel != kSMNullChannelId) ? ((tSMDevHandle) statusp->channel) : ((tSMDevHandle) smControlDevice));

		result = smd_ioctl_dev_fwapi( 	BESPIO_REPLAY_STATUS, 
                        				(SMIOCTLU *) statusp,
										smStatusDevice,
										sizeof(struct sm_replay_status_parms),
										-1,
										kFWLIBVersion																								);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_REPLAY_DELAY
 *
 * Calculate delay till underrun for channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_replay_delay( struct sm_replay_delay_parms* delayp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_fwapi( 	BESPIO_REPLAY_DELAY, 
			                        	(SMIOCTLU *) delayp,
										(tSMDevHandle) delayp->channel,
										sizeof(struct sm_replay_delay_parms),
										-1,
										kFWLIBVersion							);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_REPLAY_ABORT
 *
 * Abort an on-going replay on given channel.
 */ 
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_replay_abort( struct sm_replay_abort_parms* abortp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_fwapi( 	BESPIO_REPLAY_ABORT, 
							        	(SMIOCTLU *) abortp,
										(tSMDevHandle) abortp->channel,
										sizeof(struct sm_replay_abort_parms),
										-1,
										kFWLIBVersion							);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_REPLAY_ADJUST
 *
 * Adjust parameters to current replay.
 */ 
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_replay_adjust( struct sm_replay_adjust_parms* adjustp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSM_INT			channelIx;
	tSMChannelId	background;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		channelIx = -1;

		if ((background = adjustp->background) != kSMNullChannelId)
		{
			channelIx = sm_get_channel_ix(background);
		}

		adjustp->background = (tSMChannelId) channelIx;

		result = smd_ioctl_dev_fwapi( 	BESPIO_REPLAY_ADJUST1, 
			                        	(SMIOCTLU *) adjustp,
										(tSMDevHandle) adjustp->channel,
										sizeof(struct sm_replay_adjust_parms),
										-1,
										kFWLIBVersion							);

		adjustp->background = background;
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_PLAY_TONE
 *
 * Output tone on output channel.
 */ 
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_play_tone( struct sm_play_tone_parms* tonep )
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_fwapi( 	BESPIO_PLAY_TONE, 
			                        	(SMIOCTLU *) tonep,
										(tSMDevHandle) tonep->channel,
										sizeof(struct sm_play_tone_parms),
										-1,
										kFWLIBVersion							);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_PLAY_TONE_ABORT
 *
 * Abort playing of tone.
 */ 
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_play_tone_abort( tSMChannelId channel )
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

		result = smd_ioctl_dev_fwapi( 	BESPIO_PLAY_TONE_ABORT, 
					                    (SMIOCTLU *) &localChannel,
										(tSMDevHandle) channel,
										sizeof(localChannel),
										-1,
										kFWLIBVersion								);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_PLAY_TONE_STATUS
 *
 * Obtain status of current tone generation on channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_play_tone_status( struct sm_play_tone_status_parms* statusp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSMDevHandle	smStatusDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		smStatusDevice = (tSMDevHandle) ((statusp->channel != kSMNullChannelId) ? ((tSMDevHandle) statusp->channel) : ((tSMDevHandle) smControlDevice));

		result = smd_ioctl_dev_fwapi( 	BESPIO_PLAY_TONE_STATUS, 
			                        	(SMIOCTLU *) statusp,
										smStatusDevice,
										sizeof(struct sm_play_tone_status_parms),
										-1,
										kFWLIBVersion								);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_PLAY_CPTONE
 *
 * Output call progress tone on output channel.
 */ 
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_play_cptone( struct sm_play_cptone_parms* cptonep )
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_fwapi( 	BESPIO_PLAY_CPTONE, 
			                        	(SMIOCTLU *) cptonep,
										(tSMDevHandle) cptonep->channel,
										sizeof(struct sm_play_cptone_parms),
										-1,
										kFWLIBVersion							);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_PLAY_TONE_ABORT
 *
 * Abort playing of tone.
 */ 
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_play_cptone_abort( tSMChannelId channel )
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

		result = smd_ioctl_dev_fwapi( 	BESPIO_PLAY_CPTONE_ABORT, 
					                    (SMIOCTLU *) &localChannel,
										(tSMDevHandle) channel,
										sizeof(localChannel),
										-1,
										kFWLIBVersion								);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_PLAY_CPTONE_STATUS
 *
 * Obtain status of current call progress tone generation on channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_play_cptone_status( struct sm_play_cptone_status_parms* statusp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSMDevHandle	smStatusDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		smStatusDevice = (tSMDevHandle) ((statusp->channel != kSMNullChannelId) ? ((tSMDevHandle) statusp->channel) : ((tSMDevHandle) smControlDevice));

		result = smd_ioctl_dev_fwapi( 	BESPIO_PLAY_CPTONE_STATUS, 
			                        	(SMIOCTLU *) statusp,
										smStatusDevice,
										sizeof(struct sm_play_cptone_status_parms),
										-1,
										kFWLIBVersion								);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_PLAY_DIGITS
 *
 * Output DTMF digits on output channel.
 */ 
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_play_digits( struct sm_play_digits_parms* digitsp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_fwapi( 	BESPIO_PLAY_DIGITS, 
			                        	(SMIOCTLU *) digitsp,
										(tSMDevHandle) digitsp->channel,
										sizeof(struct sm_play_digits_parms),
										-1,
										kFWLIBVersion							);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_PLAY_CPTONE_STATUS
 *
 * Obtain status of current call progress tone generation on channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_play_digits_status( struct sm_play_digits_status_parms* statusp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSMDevHandle	smStatusDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		smStatusDevice = (tSMDevHandle) ((statusp->channel != kSMNullChannelId) ? ((tSMDevHandle) statusp->channel) : ((tSMDevHandle) smControlDevice));

		result = smd_ioctl_dev_fwapi( 	BESPIO_PLAY_DIGITS_STATUS, 
			                        	(SMIOCTLU *) statusp,
										smStatusDevice,
										sizeof(struct sm_play_digits_status_parms),
										-1,
										kFWLIBVersion								);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_RECORD_START
 *
 * Initiate recording on channel.
 */ 
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_record_start( struct sm_record_parms* recordp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSM_INT			channelIx;
	tSMChannelId	alt_data_source;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		channelIx = -1;

		if ((alt_data_source = recordp->alt_data_source) != kSMNullChannelId)
		{
			channelIx = sm_get_channel_ix(alt_data_source);
		}

		recordp->alt_data_source = (tSMChannelId) channelIx;

		/*
		 * Record start2 indicates paramter block has volume/AGC.
		 */
		result = smd_ioctl_dev_fwapi( 	
#ifdef _SM_BESP_PRE1P4_
										BESPIO_RECORD_START1, 
#else
										BESPIO_RECORD_START2, 
#endif
										(SMIOCTLU *) recordp,
										(tSMDevHandle) recordp->channel,
										sizeof(struct sm_record_parms),
										-1,
										kFWLIBVersion							);

		recordp->alt_data_source = alt_data_source;
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_GET_RECORDED_DATA
 *
 * Obtain data recorded on channel.
 */ 
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_get_recorded_data( struct sm_ts_data_parms* datap)
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSM_INT 		readOctets;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		readOctets = kSMMaxRecordDataBufferSize;

		if (datap->channel != kSMNullChannelId)
		{
			result = smd_read_dev((tSMDevHandle) datap->channel,datap->data,&readOctets);
		}
		else
		{
			/*
			 * Most urgent channel read.
			 */
			result = smd_read_dev((tSMDevHandle) smControlDevice,datap->data,&readOctets);
		}

		if (result == 0)
		{
			datap->length = readOctets;
		}
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_RECORD_STATUS
 *
 * Get status of recording on channel.
 */ 
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_record_status( struct sm_record_status_parms* statusp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSMDevHandle 	smStatusDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		smStatusDevice = (tSMDevHandle) ((statusp->channel != kSMNullChannelId) ? ((tSMDevHandle) statusp->channel) : ((tSMDevHandle) smControlDevice));

		result = smd_ioctl_dev_fwapi( 	BESPIO_RECORD_STATUS, 
                        				(SMIOCTLU *) statusp,
										smStatusDevice,
										sizeof(struct sm_record_status_parms),
										-1,
										kFWLIBVersion							);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_RECORD_ABORT
 *
 * Abort recording on given channel.
 */ 
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_record_abort( struct sm_record_abort_parms* abortp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_fwapi( 	BESPIO_RECORD_ABORT, 
										(SMIOCTLU *) abortp,
										(tSMDevHandle) abortp->channel,
										sizeof(struct sm_record_abort_parms),
										-1,
										kFWLIBVersion							);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_RECORD_HOW_TERMINATED
 *
 * Determine reason for which last record on channel terminated.
 */ 
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_record_how_terminated( struct sm_record_how_terminated_parms* howp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_fwapi( 	BESPIO_RECORD_HOW_TERMINATED, 
										(SMIOCTLU *) howp,
										(tSMDevHandle) howp->channel,
										sizeof(struct sm_record_how_terminated_parms),
										-1,
										kFWLIBVersion							);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_LISTEN_FOR
 *
 * Condition channel to recognise items (tones, digits etc.)
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_listen_for( struct sm_listen_for_parms* listenp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_fwapi( 	BESPIO_LISTEN_FOR, 
			                        	(SMIOCTLU *) listenp,
										(tSMDevHandle) listenp->channel,
										sizeof(struct sm_listen_for_parms),
										-1,
										kFWLIBVersion							);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CONF_PRIM_START
 *
 * Initialise a conferenced output channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_conf_prim_start( struct sm_conf_prim_start_parms* confp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_fwapi( 	BESPIO_CONF_PRIM_START, 
			                        	(SMIOCTLU *) confp,
										(tSMDevHandle) confp->channel,
										sizeof(struct sm_conf_prim_start_parms),
										-1,
										kFWLIBVersion								);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CONF_PRIM_CLONE
 *
 * Initialise a conferenced output channel based on existing one.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_conf_prim_clone( struct sm_conf_prim_clone_parms* clonep )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSM_INT			channelIx;
	tSMChannelId	model;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		channelIx = -1;

		if ((model = clonep->model) != kSMNullChannelId)
		{
			channelIx = sm_get_channel_ix(model);
		}

		clonep->model = (tSMChannelId) channelIx;

		result = smd_ioctl_dev_fwapi( 	BESPIO_CONF_PRIM_CLONE1, 
			                        	(SMIOCTLU *) clonep,
										(tSMDevHandle) clonep->channel,
										sizeof(struct sm_conf_prim_clone_parms),
										-1,
										kFWLIBVersion								);

		clonep->model = model;
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CONF_PRIM_ADD
 *
 * Add input channel to set of input channels combined
 * to generate conference summed output.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_conf_prim_add ( struct sm_conf_prim_add_parms* confp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSM_INT			channelIx;
	tSMChannelId	participant;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		channelIx = -1;

		if ((participant = confp->participant) != kSMNullChannelId)
		{
			channelIx = sm_get_channel_ix(participant);
		}

		confp->participant = (tSMChannelId) channelIx;

		result = smd_ioctl_dev_fwapi( 	BESPIO_CONF_PRIM_ADD1, 
			                        	(SMIOCTLU *) confp,
										(tSMDevHandle) confp->channel,
										sizeof(struct sm_conf_prim_add_parms),
										-1,
										kFWLIBVersion								);

		confp->participant = participant;
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CONF_PRIM_LEAVE
 *
 * Remove input channel from set of input channels combined
 * to generate conference summed output.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_conf_prim_leave( struct sm_conf_prim_leave_parms* confp )  
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_fwapi( 	BESPIO_CONF_PRIM_LEAVE, 
			                        	(SMIOCTLU *) confp,
										(tSMDevHandle) confp->channel,
										sizeof(struct sm_conf_prim_leave_parms),
										-1,
										kFWLIBVersion								);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CONF_PRIM_INFO
 *
 * Retrieve information about currently active input
 * channels among set combined to generate a conferenced 
 * output channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_conf_prim_info( struct sm_conf_prim_info_parms* confp ) 
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_fwapi( 	BESPIO_CONF_PRIM_INFO, 
			                        	(SMIOCTLU *) confp,
										(tSMDevHandle) confp->channel,
										sizeof(struct sm_conf_prim_info_parms),
										-1,
										kFWLIBVersion								);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CONF_PRIM_ADJ_INPUT
 *
 * Adjust the parameters of an input channel that
 * is participating in one or more conferenced outputs.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_conf_prim_adj_input( struct sm_conf_prim_adj_input_parms* confp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_fwapi( 	BESPIO_CONF_PRIM_ADJ_INPUT, 
			                        	(SMIOCTLU *) confp,
										(tSMDevHandle) confp->channel,
										sizeof(struct sm_conf_prim_adj_input_parms),
										-1,
										kFWLIBVersion								);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CONF_PRIM_ADJ_OUTPUT
 *
 * Adjust output level of conferenced output channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_conf_prim_adj_output( struct sm_conf_prim_adj_output_parms* confp ) 
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_fwapi( 	BESPIO_CONF_PRIM_ADJ_OUTPUT, 
			                        	(SMIOCTLU *) confp,
										(tSMDevHandle) confp->channel,
										sizeof(struct sm_conf_prim_adj_output_parms),
										-1,
										kFWLIBVersion								);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CONF_PRIM_ABORT
 *
 * Abort conferenced output channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_conf_prim_abort( tSMChannelId channel ) 
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

		result = smd_ioctl_dev_fwapi( 	BESPIO_CONF_PRIM_ABORT, 
					                    (SMIOCTLU *) &localChannel,
										(tSMDevHandle) channel,
										sizeof(localChannel),
										-1,
										kFWLIBVersion								);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CONF_PRIMIX_START
 *
 * Initialise a conferenced output channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_conf_primix_start( struct sm_conf_primix_start_parms* confp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSM_INT			module;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		module = sm_get_channel_ix_module_ix(confp->channel_ix);

		if (module >= 0)
		{
			result = smd_ioctl_dev_fwapi( 	BESPIO_CONF_PRIMIX_START, 
				                        	(SMIOCTLU *) confp,
											(tSMDevHandle) smControlDevice,
											sizeof(struct sm_conf_primix_start_parms),
											module,
											kFWLIBVersion								);
		}
		else
		{
			result = module;
		}
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CONF_PRIMIX_CLONE
 *
 * Initialise a conferenced output channel based on existing one.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_conf_primix_clone( struct sm_conf_primix_clone_parms* clonep )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSM_INT			module;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		module = sm_get_channel_ix_module_ix(clonep->channel_ix);

		if (module >= 0)
		{
			result = smd_ioctl_dev_fwapi( 	BESPIO_CONF_PRIMIX_CLONE, 
				                        	(SMIOCTLU *) clonep,
											(tSMDevHandle) smControlDevice,
											sizeof(struct sm_conf_primix_clone_parms),
											module,
											kFWLIBVersion								);
		}
		else
		{
			result = module;
		}
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CONF_PRIMIX_ADD
 *
 * Add input channel to set of input channels combined
 * to generate conference summed output.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_conf_primix_add ( struct sm_conf_primix_add_parms* confp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSM_INT			module;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		module = sm_get_channel_ix_module_ix(confp->channel_ix);

		if (module >= 0)
		{
			result = smd_ioctl_dev_fwapi( 	BESPIO_CONF_PRIMIX_ADD, 
				                        	(SMIOCTLU *) confp,
											(tSMDevHandle) smControlDevice,
											sizeof(struct sm_conf_primix_add_parms),
											module,
											kFWLIBVersion								);
		}
		else
		{
			result = module;
		}
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CONF_PRIMIX_LEAVE
 *
 * Remove input channel from set of input channels combined
 * to generate conference summed output.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_conf_primix_leave( struct sm_conf_primix_leave_parms* confp )  
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSM_INT			module;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		module = sm_get_channel_ix_module_ix(confp->channel_ix);

		if (module >= 0)
		{
			result = smd_ioctl_dev_fwapi( 	BESPIO_CONF_PRIMIX_LEAVE, 
				                        	(SMIOCTLU *) confp,
											(tSMDevHandle) smControlDevice,
											sizeof(struct sm_conf_primix_leave_parms),
											module,
											kFWLIBVersion								);
		}
		else
		{
			result = module;
		}
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CONF_PRIMIX_INFO
 *
 * Retrieve information about currently active input
 * channels among set combined to generate a conferenced 
 * output channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_conf_primix_info( struct sm_conf_primix_info_parms* confp ) 
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSM_INT			module;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		module = sm_get_channel_ix_module_ix(confp->channel_ix);

		if (module >= 0)
		{
			result = smd_ioctl_dev_fwapi( 	BESPIO_CONF_PRIMIX_INFO, 
				                        	(SMIOCTLU *) confp,
											(tSMDevHandle) smControlDevice,
											sizeof(struct sm_conf_primix_info_parms),
											module,
											kFWLIBVersion								);
		}
		else
		{
			result = module;
		}
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CONF_PRIMIX_ADJ_INPUT
 *
 * Adjust the parameters of an input channel that
 * is participating in one or more conferenced outputs.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_conf_primix_adj_input( struct sm_conf_primix_adj_input_parms* confp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSM_INT			module;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		module = sm_get_channel_ix_module_ix(confp->channel_ix);

		if (module >= 0)
		{
			result = smd_ioctl_dev_fwapi( 	BESPIO_CONF_PRIMIX_ADJ_INPUT, 
				                        	(SMIOCTLU *) confp,
											(tSMDevHandle) smControlDevice,
											sizeof(struct sm_conf_primix_adj_input_parms),
											module,
											kFWLIBVersion								);
		}
		else
		{
			result = module;
		}
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CONF_PRIMIX_ADJ_OUTPUT
 *
 * Adjust output level of conferenced output channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_conf_primix_adj_output( struct sm_conf_primix_adj_output_parms* confp ) 
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSM_INT			module;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		module = sm_get_channel_ix_module_ix(confp->channel_ix);

		if (module >= 0)
		{
			result = smd_ioctl_dev_fwapi( 	BESPIO_CONF_PRIMIX_ADJ_OUTPUT, 
				                        	(SMIOCTLU *) confp,
											(tSMDevHandle) smControlDevice,
											sizeof(struct sm_conf_primix_adj_output_parms),
											module,
											kFWLIBVersion									);
		}
		else
		{
			result = module;
		}
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CONF_PRIMIX_ABORT
 *
 * Abort conferenced output channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_conf_primix_abort( int channelIx ) 
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSM_INT 		localChannelIx;
	tSM_INT			module;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		module = sm_get_channel_ix_module_ix(channelIx);

		if (module >= 0)
		{
			/*
			 *   For maximum portability do not
			 *   take address of value stack parameters.
			 */
			localChannelIx = channelIx;

			result = smd_ioctl_dev_fwapi( 	BESPIO_CONF_PRIMIX_ABORT, 
						                    (SMIOCTLU *) &localChannelIx,
											(tSMDevHandle) smControlDevice,
											sizeof(localChannelIx),
											module,
											kFWLIBVersion								);
		}
		else
		{
			result = module;
		}
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_SET_SIDETONE_CHANNEL
 *
 * Set up echo suppression on input channel with respect to 
 * nominated output channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_set_sidetone_channel( struct sm_set_sidetone_channel_parms* sidetp ) 
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSM_INT			channelIx;
	tSMChannelId	output;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		channelIx = -1;

		if ((output = sidetp->output) != kSMNullChannelId)
		{
			channelIx = sm_get_channel_ix(output);
		}

		sidetp->output = (tSMChannelId) channelIx;

		result = smd_ioctl_dev_fwapi( 	BESPIO_SET_SIDETONE_CHANNEL1, 
			                        	(SMIOCTLU *) sidetp,
										(tSMDevHandle) sidetp->channel,
										sizeof(struct sm_set_sidetone_channel_parms),
										-1,
										kFWLIBVersion								);

		sidetp->output = (tSMChannelId) output;
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_RESET_INPUT_VOCABS 
 * 
 * Reset modules repertoire of ASR vocabulary.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_reset_input_vocabs( struct sm_reset_input_vocabs_parms* resetp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_fwapi( 	BESPIO_RESET_INPUT_VOCABS,
					                    (SMIOCTLU *) resetp,
										smControlDevice,
										sizeof(struct sm_reset_input_vocabs_parms),
										resetp->module,
										kFWLIBVersion									);
	}
	else
	{
		result = ERR_SM_DEVERR;
	}

	return result;
}


/*
 * SM_ADD_INPUT_VOCAB 
 * 
 * Add a new input vocabulary to module repertoire.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_add_input_vocab( struct sm_input_vocab_parms* vocabp )
{
	int  								result;
	tSMFileHandle  						readh;
	int  								size;
	struct sm_vocab_init_parms	 		init_parms;
	struct sm_vocab_buffer_parms 		buffer_parms;
	struct sm_vocab_complete_parms 		complete_parms;
	int									bufferSize;
	int									complete_result;
	tSMDevHandle 						smControlDevice;
	int									i;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = 0;

		readh = smd_file_open(vocabp->filename);

		if (readh > 0)
		{
			init_parms.module = vocabp->module;

			bufferSize = sizeof(buffer_parms.vcbuffer);

			size = smd_file_read(readh,&buffer_parms.vcbuffer[0],bufferSize);

			if (size < 8)
			{
				result = ERR_SM_FILE_FORMAT;
			}
			else
			{
				init_parms.length    = 8 + ((unsigned char) buffer_parms.vcbuffer[4]) + (((unsigned char)buffer_parms.vcbuffer[5]) << 8);

				result = smd_ioctl_dev_fwapi( 	BESPIO_VOCAB_INIT,
									            (SMIOCTLU *) (&init_parms),
												smControlDevice,
												sizeof(struct sm_vocab_init_parms),
												vocabp->module,
												kFWLIBVersion									);
			}

			if (result == 0)
			{
				buffer_parms.module  = vocabp->module;
				buffer_parms.length  = size;

				result = smd_ioctl_dev_fwapi( 	BESPIO_VOCAB_BUFFER,
									            (SMIOCTLU *) (&buffer_parms),
												smControlDevice,
												sizeof(struct sm_vocab_buffer_parms),
												vocabp->module,
												kFWLIBVersion									);
			}

			if (result == ERR_SM_NO_ACTION)
			{
				/*
				 * This vocabulary item is already loaded.
				 */
				complete_parms.module = vocabp->module;
				complete_parms.abort  = 1;

				result = smd_ioctl_dev_fwapi( 	BESPIO_VOCAB_COMPLETE,
									            (SMIOCTLU *) (&complete_parms),
												smControlDevice,
												sizeof(struct sm_vocab_complete_parms),
												vocabp->module,
												kFWLIBVersion									);
				if (result == 0)
				{
					vocabp->item_id = complete_parms.item_id;
				}
			}
			else if (result == 0)
			{
				i = 0;

				bufferSize = sizeof(buffer_parms.vcbuffer);

				size = smd_file_read(readh,&buffer_parms.vcbuffer[0],bufferSize);

				if (size > 0)
				{
					while (result == 0)
					{
						if (size != 0)
						{
							buffer_parms.module  = vocabp->module;
							buffer_parms.length  = size;

							result = smd_ioctl_dev_fwapi( 	BESPIO_VOCAB_BUFFER,
												            (SMIOCTLU *) (&buffer_parms),
															smControlDevice,
															sizeof(struct sm_vocab_buffer_parms),
															vocabp->module,
															kFWLIBVersion									);

							if ((size == bufferSize) && (result == 0))
							{
								size = smd_file_read(readh,&buffer_parms.vcbuffer[0],bufferSize);
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
					complete_parms.module = vocabp->module;
					complete_parms.abort  = (result != 0);

					complete_result = smd_ioctl_dev_fwapi( 	BESPIO_VOCAB_COMPLETE,
												            (SMIOCTLU *) (&complete_parms),
															smControlDevice,
															sizeof(struct sm_vocab_complete_parms),
															vocabp->module,
															kFWLIBVersion									);
					if (result == 0)
					{
						result = complete_result;

						if (result == 0)
						{
							vocabp->item_id = complete_parms.item_id;
						}
					}
				}
				else
				{
					result = ERR_SM_FILE_FORMAT;
				}
			}

			smd_file_close(readh);
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


static void sm_port_characteristics( 	SM_ASR_CHARACTERISTICS* 	asrCharacteristics, 
										SM_ASR_DRV_CHARACTERISTICS* asrDrvCharacteristics )
{
	asrDrvCharacteristics->vfr_max_frames = 
		asrCharacteristics->vfr_max_frames;

	*((tSMIEEE32Bit754854Float*) (&asrDrvCharacteristics->vfr_diff_threshold[0])) = 
		(tSMIEEE32Bit754854Float) asrCharacteristics->vfr_diff_threshold;

	asrDrvCharacteristics->pse_max_frames = 
		asrCharacteristics->pse_max_frames;

	asrDrvCharacteristics->pse_min_frames = 
		asrCharacteristics->pse_min_frames;

	*((tSMIEEE32Bit754854Float*) (&asrDrvCharacteristics->pse_score_difference[0])) = 
		(tSMIEEE32Bit754854Float) asrCharacteristics->pse_score_difference;

	*((tSMIEEE32Bit754854Float*) (&asrDrvCharacteristics->pse_snr[0])) = 
		(tSMIEEE32Bit754854Float) asrCharacteristics->pse_snr;

	*((tSMIEEE32Bit754854Float*) (&asrDrvCharacteristics->pse_noise_score[0])) = 
		(tSMIEEE32Bit754854Float) asrCharacteristics->pse_noise_score;

	*((tSMIEEE32Bit754854Float*) (&asrDrvCharacteristics->pse_latency[0])) = 
		(tSMIEEE32Bit754854Float) asrCharacteristics->pse_latency;

	*((tSMIEEE32Bit754854Float*) (&asrDrvCharacteristics->vit_soft_threshold[0])) = 
		(tSMIEEE32Bit754854Float) asrCharacteristics->vit_soft_threshold;

	*((tSMIEEE32Bit754854Float*) (&asrDrvCharacteristics->vit_hard_threshold[0])) = 
		(tSMIEEE32Bit754854Float) asrCharacteristics->vit_hard_threshold;
}


/*
 * SM_ASR_LISTEN_FOR
 *
 * Condition channel to recognise items (tones, digits etc.)
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_asr_listen_for( struct sm_asr_listen_for_parms* listenp )
{
	int 					result;
	tSMDevHandle 			smControlDevice;
	SM_ASR_LF_DRV1_PARMS	asrListenDrvParms;
	SM_ASR_LF_DRV2_PARMS	channelASRCharacteristics;
	int						j,n,ix;

	result = 0;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		if (listenp->specific_parameters)
		{
			channelASRCharacteristics.channel = listenp->channel;

			sm_port_characteristics( 	listenp->specific_parameters,
										&channelASRCharacteristics.specific_parameters	);

			result = smd_ioctl_dev_fwapi( 	BESPIO_ASR_LF_DRV2,
						                    (SMIOCTLU *) &channelASRCharacteristics,
											(tSMDevHandle) listenp->channel,
											sizeof(struct sm_asr_lf_drv2_parms),
											-1,
											kFWLIBVersion									);
		}

		if (result == 0)
		{
			asrListenDrvParms.channel = listenp->channel;

			n  = listenp->vocab_item_count;
			ix = 0;

			while ((result == 0) && ((n > 0) || (listenp->vocab_item_count == 0)))
			{
				asrListenDrvParms.is_new_item_list 	= (n == listenp->vocab_item_count);
				asrListenDrvParms.is_list_complete 	= (n <= (sizeof(asrListenDrvParms.vocab_item_ids)/sizeof(tSM_UT32)));
				asrListenDrvParms.mode				= (asrListenDrvParms.is_list_complete) ? listenp->asr_mode : kSMASRModeDisabled;

				if (asrListenDrvParms.is_list_complete)
				{
					asrListenDrvParms.vocab_item_count = n;
				}
				else
				{
					asrListenDrvParms.vocab_item_count = (sizeof(asrListenDrvParms.vocab_item_ids)/sizeof(tSM_UT32));
				}

				for (j = 0; (j < asrListenDrvParms.vocab_item_count); j++)
				{
					asrListenDrvParms.vocab_item_ids[j]  = *(listenp->vocab_item_ids+ix);
					asrListenDrvParms.vocab_recog_ids[j] = *(listenp->vocab_recog_ids+ix);

					ix += 1;
				}

				result = smd_ioctl_dev_fwapi( 	BESPIO_ASR_LF_DRV1,
							                    (SMIOCTLU *) &asrListenDrvParms,
												(tSMDevHandle) listenp->channel,
												sizeof(struct sm_asr_lf_drv1_parms),
												-1,
												kFWLIBVersion							);

				if (asrListenDrvParms.vocab_item_count)
				{
					n -= asrListenDrvParms.vocab_item_count;
				}
				else
				{
					break;
				}
			}
		}
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_ASR_MODULE_PARAMETERS 
 * 
 * Alter speech recognition paramters for module.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_asr_module_parameters( struct sm_asr_module_parameters_parms* asrp )
{
	int 					result;
	tSMDevHandle 			smControlDevice;
	SM_ASR_MP_DRV_PARMS		drvParms;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		drvParms.module = asrp->module;

		sm_port_characteristics( 	&asrp->characteristics,
									&drvParms.characteristics	);

		result = smd_ioctl_dev_fwapi( 	BESPIO_ASR_MODULE_PARAMETERS,
					                    (SMIOCTLU *) &drvParms,
										smControlDevice,
										sizeof(struct sm_asr_mp_drv_parms),
										asrp->module,
										kFWLIBVersion						);
	}
	else
	{
		result = ERR_SM_DEVERR;
	}

	return result;
}


/*
 * SM_DISCARD_RECOGNISED 
 * 
 * Discard any uncollected recognised items.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_discard_recognised( tSMChannelId channel )
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

		result = smd_ioctl_dev_fwapi( 	BESPIO_DISCARD_RECOGNISED,
				                        (SMIOCTLU *) &localChannel,
										(tSMDevHandle) channel,
										sizeof(localChannel),
										-1,
										kFWLIBVersion								);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CONDITION_INPUT
 *
 * Apply input conditioning algoritm such as echo cancellation to channel.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_condition_input( struct sm_condition_input_parms* condp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSM_INT			referenceChannelIx;
	tSMChannelId	referenceChannel;
	tSM_INT			altDataChannelIx;
	tSMChannelId	altDataChannel;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		referenceChannelIx 	= -1;
		referenceChannel 	= condp->reference;

		if (	(condp->reference_type != kSMInputCondRefNone) 
			&&	(referenceChannel != kSMNullChannelId) 			)
		{
			referenceChannelIx = sm_get_channel_ix(referenceChannel);
		}

		altDataChannelIx 	= -1;
		altDataChannel 		= condp->alt_data_dest;

		if (	(condp->alt_dest_type != kSMInputCondAltDestNone) 
			&&	(altDataChannel != kSMNullChannelId) 				)
		{
			altDataChannelIx = sm_get_channel_ix(altDataChannel);
		}

		condp->reference 	 = (tSMChannelId) referenceChannelIx;
		condp->alt_data_dest = (tSMChannelId) altDataChannelIx;

		result = smd_ioctl_dev_fwapi( 	BESPIO_COND_INPUT, 
			                        	(SMIOCTLU *) condp,
										(tSMDevHandle) condp->channel,
										sizeof(struct sm_condition_input_parms),
										-1,
										kFWLIBVersion								);

		condp->reference 	 = referenceChannel;
		condp->alt_data_dest = altDataChannel;
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_CONDITION_REINIT 
 * 
 * Re-initialise input conditioning algorithm.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_condition_reinit( tSMChannelId channel )
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

		result = smd_ioctl_dev_fwapi( 	BESPIO_COND_REINIT,
				                        (SMIOCTLU *) &localChannel,
										(tSMDevHandle) channel,
										sizeof(localChannel),
										-1,
										kFWLIBVersion								);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_GET_RECOGNISED
 *
 * Poll channel for recognised item (digit, tone, etc. ).
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_get_recognised( struct sm_recognised_parms* recogp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSMDevHandle 	smRecogDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		smRecogDevice = (tSMDevHandle) ((recogp->channel != kSMNullChannelId) ? ((tSMDevHandle) recogp->channel) : ((tSMDevHandle) smControlDevice));

		result = smd_ioctl_dev_fwapi( 	BESPIO_GET_RECOGNISED, 
                        				(SMIOCTLU *) recogp,
										smRecogDevice,
										sizeof(struct sm_recognised_parms),
										-1,
										kFWLIBVersion																								);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_GET_RECOGNISED_IX
 *
 * Poll channel for recognised item (digit, tone, etc. ).
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_get_recognised_ix( struct sm_recognised_ix_parms* recogp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSMDevHandle 	smRecogDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		recogp->channel_ix = -1;

		smRecogDevice = (tSMDevHandle) (tSMDevHandle) smControlDevice;

		result = smd_ioctl_dev_fwapi( 	BESPIO_GET_RECOGNISED_IX, 
                        				(SMIOCTLU *) recogp,
										smRecogDevice,
										sizeof(struct sm_recognised_ix_parms),
										-1,
										kFWLIBVersion																								);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_BESP_READ_STATUS
 *
 * Obtain status following read event.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_besp_read_status( struct sm_besp_status_parms* statusp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSMDevHandle	smStatusDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		statusp->channel = kSMNullChannelId;

		smStatusDevice = (tSMDevHandle) ((statusp->channel != kSMNullChannelId) ? ((tSMDevHandle) statusp->channel) : ((tSMDevHandle) smControlDevice));

		result = smd_ioctl_dev_fwapi( 	BESPIO_READ_STATUS, 
                        				(SMIOCTLU *) statusp,
										smStatusDevice,
										sizeof(struct sm_besp_status_parms),
										-1,
										kFWLIBVersion																								);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_BESP_WRITE_STATUS
 *
 * Obtain status following write event.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_besp_write_status( struct sm_besp_status_parms* statusp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSMDevHandle	smStatusDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		statusp->channel = kSMNullChannelId;

		smStatusDevice = (tSMDevHandle) ((statusp->channel != kSMNullChannelId) ? ((tSMDevHandle) statusp->channel) : ((tSMDevHandle) smControlDevice));

		result = smd_ioctl_dev_fwapi( 	BESPIO_WRITE_STATUS, 
                        				(SMIOCTLU *) statusp,
										smStatusDevice,
										sizeof(struct sm_besp_status_parms),
										-1,
										kFWLIBVersion																								);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_BESP_READ_STATUS_IX
 *
 * Obtain status following read event (channel index version).
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_besp_read_status_ix( struct sm_besp_status_ix_parms* statusp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSMDevHandle	smStatusDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		statusp->channel_ix = -1;

		smStatusDevice = (tSMDevHandle) (tSMDevHandle) smControlDevice;

		result = smd_ioctl_dev_fwapi( 	BESPIO_READ_STATUS_IX, 
                        				(SMIOCTLU *) statusp,
										smStatusDevice,
										sizeof(struct sm_besp_status_ix_parms),
										-1,
										kFWLIBVersion																								);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


/*
 * SM_BESP_WRITE_STATUS_IX
 *
 * Obtain status following write event (channel index variant).
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int sm_besp_write_status_ix( struct sm_besp_status_ix_parms* statusp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;
	tSMDevHandle	smStatusDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		statusp->channel_ix = -1;

		smStatusDevice = (tSMDevHandle) (tSMDevHandle) smControlDevice;

		result = smd_ioctl_dev_fwapi( 	BESPIO_WRITE_STATUS_IX, 
                        				(SMIOCTLU *) statusp,
										smStatusDevice,
										sizeof(struct sm_besp_status_ix_parms),
										-1,
										kFWLIBVersion																								);
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
int sm_adjust_catsig_module_params( struct sm_adjust_catsig_module_parms* alg_lf_p )
{
	int											result;
	tSMDevHandle 								smControlDevice;
	struct sm_adjust_catsig_module_drv_parms	drv_parms;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		drv_parms.module        = alg_lf_p->module;
		drv_parms.catsig_alg_id	= alg_lf_p->catsig_alg_id;
		drv_parms.parameter_id  = alg_lf_p->parameter_id;

		*((tSMIEEE32Bit754854Float*) (&drv_parms.fp_value[0])) = (tSMIEEE32Bit754854Float) alg_lf_p->parameter_value.fp_value;

		drv_parms.int_value = alg_lf_p->parameter_value.int_value;

		result = smd_ioctl_dev_fwapi( 	BESPIO_ADJUST_CATSIG_PARAMS, 
			                        	(SMIOCTLU *) &drv_parms,
										smControlDevice,
										sizeof(struct sm_adjust_catsig_module_drv_parms),
										drv_parms.module,
										kFWLIBVersion										);
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
int sm_catsig_listen_for( struct sm_catsig_listen_for_parms* listenp )
{
	int 			result;
	tSMDevHandle 	smControlDevice;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		result = smd_ioctl_dev_fwapi( 	BESPIO_CATSIG_LISTEN_FOR, 
			                        	(SMIOCTLU *) listenp,
										(tSMDevHandle) listenp->channel,
										sizeof(struct sm_catsig_listen_for_parms),
										-1,
										kFWLIBVersion								);
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
int sm_adjust_agc_module_params( struct sm_adjust_agc_module_parms* agc_parms )
{
	int											result;
	tSMDevHandle 								smControlDevice;
	struct sm_adjust_agc_module_drv_parms		drv_parms;
	double										blockMult;
	double										red;
	double										inc;
	double										decay;
	double										adjRed;
	double										adjInc;
	double										adjDecay;

	adjInc 		= agc_parms->fp_params[kAdjustAGCFPParamIxMaxIncreseRate];
	adjRed 		= agc_parms->fp_params[kAdjustAGCFPParamIxMaxDecreaseRate];
	adjDecay 	= agc_parms->fp_params[kAdjustAGCFPParamIxSensitivity];

	if ((adjInc < 0.0) || (adjInc > 1.0))
	{
		result = ERR_SM_BAD_PARAMETER;
	}
	else if ((adjRed < 0.0) || (adjRed > 1.0))
	{
		result = ERR_SM_BAD_PARAMETER;
	}
	else if ((adjDecay < 0.0) || (adjDecay > 1.0))
	{
		result = ERR_SM_BAD_PARAMETER;
	}
 	else
	{
		smControlDevice = smd_open_ctl_dev( );

		if (smControlDevice != kSMNullDevHandle)
		{
			drv_parms.module        = agc_parms->module;
			drv_parms.is_put		= 0;
			drv_parms.agc_type		= agc_parms->agc_type;

			result = smd_ioctl_dev_fwapi( 	BESPIO_ADJUST_AGC_PARAMS, 
				                        	(SMIOCTLU *) &drv_parms,
											smControlDevice,
											sizeof(struct sm_adjust_agc_module_drv_parms),
											drv_parms.module,
											kFWLIBVersion										);

			if (result == 0)
			{
				blockMult = (double)(*((tSMIEEE32Bit754854Float*) (&drv_parms.fp_values[kAdjustAGCDrvFPGetParamIxBlockMult][0])));

				inc 	= 1.0 + adjInc * blockMult;
				red 	= 1.0 - adjRed * blockMult;
				decay 	= 1.0 - adjDecay * blockMult;

				*((tSMIEEE32Bit754854Float*) (&drv_parms.fp_values[kAdjustAGCDrvFPPutParamIxLMaxDecay][0])) = 
					(tSMIEEE32Bit754854Float)decay;
				*((tSMIEEE32Bit754854Float*) (&drv_parms.fp_values[kAdjustAGCDrvFPPutParamIxGainInc][0])) = 
					(tSMIEEE32Bit754854Float)(inc);
				*((tSMIEEE32Bit754854Float*) (&drv_parms.fp_values[kAdjustAGCDrvFPPutParamIxSqrGainInc][0])) = 
					(tSMIEEE32Bit754854Float)(inc*inc);
				*((tSMIEEE32Bit754854Float*) (&drv_parms.fp_values[kAdjustAGCDrvFPPutParamIxRecipGainInc][0])) = 
					(tSMIEEE32Bit754854Float)(1.0/inc);
				*((tSMIEEE32Bit754854Float*) (&drv_parms.fp_values[kAdjustAGCDrvFPPutParamIxGainRed][0])) = 
					(tSMIEEE32Bit754854Float)(red);
				*((tSMIEEE32Bit754854Float*) (&drv_parms.fp_values[kAdjustAGCDrvFPPutParamIxSqrRedOverInc][0])) = 
					(tSMIEEE32Bit754854Float)((red*red)/(inc*inc));

				drv_parms.is_put = 1;

				result = smd_ioctl_dev_fwapi( 	BESPIO_ADJUST_AGC_PARAMS, 
					                        	(SMIOCTLU *) &drv_parms,
												smControlDevice,
												sizeof(struct sm_adjust_agc_module_drv_parms),
												drv_parms.module,
												kFWLIBVersion										);
			}
		}
		else
	    {
			result = ERR_SM_DEVERR;
	    }
	}

	return result;
}


#ifdef _PROSDLL
ACUDLL 
#endif
int sm_adjust_cond_module_params( struct sm_adjust_cond_module_parms* adjcondp )
{
	int										result;
	tSMDevHandle 							smControlDevice;
	struct sm_adjust_cond_module_drv_parms	drv_parms;

	smControlDevice = smd_open_ctl_dev( );

	if (smControlDevice != kSMNullDevHandle)
	{
		drv_parms.module			= adjcondp->module;
		drv_parms.conditioning_type	= adjcondp->conditioning_type;
		drv_parms.parameter_id		= adjcondp->parameter_id;

		*((tSMIEEE32Bit754854Float*) (&drv_parms.fp_value[0])) = (tSMIEEE32Bit754854Float) adjcondp->parameter_value.fp_value;

		drv_parms.int_value = adjcondp->parameter_value.int_value;

		result = smd_ioctl_dev_fwapi( 	BESPIO_ADJUST_COND_PARAMS, 
			                        	(SMIOCTLU *) &drv_parms,
										smControlDevice,
										sizeof(struct sm_adjust_cond_module_drv_parms),
										drv_parms.module,
										kFWLIBVersion										);
	}
	else
    {
		result = ERR_SM_DEVERR;
    }

	return result;
}


#ifdef SM_CRACK_API

int sm_crack_besp_ioctl( int ioctl, char* buffer )
{
	int		result;
	char* 	p;

	result = 0;

	p = buffer;

	switch(ioctl)
	{
		case BESPIO_ADD_OUTPUT_FREQ:
        	sprintf(buffer,"ADD_OUTPUT_FREQ");
			break;

		case BESPIO_ADD_OUTPUT_TONE:
        	sprintf(buffer,"ADD_OUTPUT_TONE");
			break;

		case BESPIO_ADD_INPUT_FREQ_COEFFS:
        	sprintf(buffer,"ADD_INPUT_FREQ_COEFFS");
			break;

		case BESPIO_ADD_INPUT_TONE_SET:
        	sprintf(buffer,"ADD_INPUT_TONE_SET");
			break;

		case BESPIO_RESET_INPUT_CPTONES:
        	sprintf(buffer,"RESET_INPUT_CPTONES");
			break;

		case BESPIO_ADD_INPUT_CPTONE:
        	sprintf(buffer,"ADD_INPUT_CPTONE");
			break;

		case BESPIO_RESET_INPUT_VOCABS:
        	sprintf(buffer,"RESET_INPUT_VOCABS");
			break;

		case BESPIO_VOCAB_INIT:
        	sprintf(buffer,"VOCAB_INIT");
			break;

		case BESPIO_VOCAB_BUFFER:
        	sprintf(buffer,"VOCAB_BUFFER");
			break;

		case BESPIO_VOCAB_COMPLETE:
        	sprintf(buffer,"VOCAB_COMPLETE");
			break;

		case BESPIO_CONF_PRIMIX_START:
        	sprintf(buffer,"CONF_PRIMIX_START");
			break;

		case BESPIO_CONF_PRIMIX_ADD:
        	sprintf(buffer,"CONF_PRIMIX_ADD");
			break;

		case BESPIO_CONF_PRIMIX_LEAVE:
        	sprintf(buffer,"CONF_PRIMIX_LEAVE");
			break;

		case BESPIO_CONF_PRIMIX_INFO:
        	sprintf(buffer,"CONF_PRIMIX_INFO");
			break;

		case BESPIO_CONF_PRIMIX_ADJ_INPUT:
        	sprintf(buffer,"CONF_PRIMIX_ADJ_INPUT");
			break;

		case BESPIO_CONF_PRIMIX_ADJ_OUTPUT:
        	sprintf(buffer,"CONF_PRIMIX_ADJ_OUTPUT");
			break;

		case BESPIO_CONF_PRIMIX_ABORT:
        	sprintf(buffer,"CONF_PRIMIX_ABORT");
			break;

		case BESPIO_CONF_PRIMIX_CLONE:
        	sprintf(buffer,"CONF_PRIMIX_CLONE");
			break;

		case BESPIO_LISTEN_FOR:
        	sprintf(buffer,"LISTEN_FOR");
			break;

		case BESPIO_REPLAY_START:
        	sprintf(buffer, "REPLAY_START");
			break;

		case BESPIO_REPLAY_STATUS:
        	sprintf(buffer, "REPLAY_STATUS");
			break;

		case BESPIO_REPLAY_DELAY:
        	sprintf(buffer, "REPLAY_DELAY");
			break;

		case BESPIO_REPLAY_ABORT:
        	sprintf(buffer, "REPLAY_ABORT");
			break;

		case BESPIO_REPLAY_ADJUST:
       		sprintf(buffer, "REPLAY_ADJUST");
			break;

		case BESPIO_PLAY_TONE:
       		sprintf(buffer, "PLAY_TONE");
			break;

		case BESPIO_PLAY_CPTONE:
       		sprintf(buffer, "PLAY_CPTONE");
			break;

		case BESPIO_PLAY_DIGITS:
        	sprintf(buffer, "PLAY_DIGITS");
			break;

		case BESPIO_RECORD_START:
			sprintf(buffer, "RECORD_START");
			break;

		case BESPIO_RECORD_STATUS:
        	sprintf(buffer, "RECORD_STATUS");
			break;

		case BESPIO_RECORD_ABORT:
        	sprintf(buffer, "RECORD_ABORT");
			break;

		case BESPIO_ASR_LF_DRV1:
        	sprintf(buffer, "ASR_LF_DRV1");
			break;

		case BESPIO_ASR_LF_DRV2:
        	sprintf(buffer, "ASR_LF_DRV2");
			break;

		case BESPIO_CONF_PRIM_START:
       		sprintf(buffer, "CONF_PRIM_START");
			break;

		case BESPIO_CONF_PRIM_ADD:
       		sprintf(buffer, "CONF_PRIM_ADD");
			break;

		case BESPIO_CONF_PRIM_LEAVE:
       		sprintf(buffer, "CONF_PRIM_LEAVE");
			break;

		case BESPIO_CONF_PRIM_INFO:
       		sprintf(buffer, "CONF_PRIM_INFO");
			break;

		case BESPIO_CONF_PRIM_ADJ_INPUT:
       		sprintf(buffer, "CONF_PRIM_ADJ_INPUT");
			break;

		case BESPIO_CONF_PRIM_ADJ_OUTPUT:
       		sprintf(buffer, "CONF_PRIM_ADJ_OUTPUT");
			break;

		case BESPIO_CONF_PRIM_ABORT:
       		sprintf(buffer, "CONF_PRIM_ABORT");
			break;

		case BESPIO_SET_SIDETONE_CHANNEL:
       		sprintf(buffer, "SET_SIDETONE_CHANNEL");
			break;

		case BESPIO_ASR_MODULE_PARAMETERS:
       		sprintf(buffer, "ASR_MODULE_PARAMETERS");
			break;

		case BESPIO_CONF_PRIM_CLONE:
       		sprintf(buffer, "CONF_PRIM_CLONE");
			break;

		case BESPIO_PLAY_TONE_STATUS:
       		sprintf(buffer, "PLAY_TONE_STATUS");
			break;

		case BESPIO_PLAY_CPTONE_STATUS:
       		sprintf(buffer, "PLAY_CPTONE_STATUS");
			break;

		case BESPIO_PLAY_DIGITS_STATUS:
       		sprintf(buffer, "PLAY_DIGITS_STATUS");
			break;

		case BESPIO_DISCARD_RECOGNISED:
       		sprintf(buffer, "DISCARD_RECOGNISED");
			break;

		case BESPIO_CONF_PRIM_ADD1:
       		sprintf(buffer, "CONF_PRIM_ADD1");
			break;

		case BESPIO_CONF_PRIM_CLONE1:
       		sprintf(buffer, "CONF_PRIM_CLONE1");
			break;

		case BESPIO_REPLAY_START1:
       		sprintf(buffer, "REPLAY_START1");
			break;

		case BESPIO_REPLAY_ADJUST1:
       		sprintf(buffer, "REPLAY_ADJUST1");
			break;

		case BESPIO_RECORD_START1:
       		sprintf(buffer, "RECORD_START1");
			break;

		case BESPIO_RECORD_START2:
       		sprintf(buffer, "RECORD_START2");
			break;

		case BESPIO_SET_SIDETONE_CHANNEL1:
       		sprintf(buffer, "SET_SIDETONE_CHANNEL1");
			break;

		case BESPIO_COND_INPUT:
       		sprintf(buffer, "COND_INPUT");
			break;

		case BESPIO_COND_REINIT:
       		sprintf(buffer, "COND_REINIT");
			break;

		case BESPIO_GET_RECOGNISED:
       		sprintf(buffer, "GET_RECOGNISED");
			break;

		case BESPIO_GET_RECOGNISED_IX:
       		sprintf(buffer, "GET_RECOGNISED_IX");
			break;

		case BESPIO_READ_STATUS:
       		sprintf(buffer, "READ_STATUS");
			break;

		case BESPIO_WRITE_STATUS:
       		sprintf(buffer, "WRITE_STATUS");
			break;

		case BESPIO_READ_STATUS_IX:
       		sprintf(buffer, "READ_STATUS_IX");
			break;

		case BESPIO_WRITE_STATUS_IX:
       		sprintf(buffer, "WRITE_STATUS_IX");
			break;

		case BESPIO_PUT_LAST_REPLAY_DATA:
       		sprintf(buffer, "PUT_LAST_REPLAY_DATA");
			break;

		case BESPIO_CATSIG_LISTEN_FOR:
       		sprintf(buffer, "CATSIG_LISTEN_FOR");
			break;

		case BESPIO_ADJUST_CATSIG_PARAMS:
       		sprintf(buffer, "ADJUST_CATSIG_MODULE_PARAMS");
			break;

		case BESPIO_ADJUST_AGC_PARAMS:
       		sprintf(buffer, "ADJUST_AGC_PARAMS");
			break;

		case BESPIO_ADJUST_COND_PARAMS:
       		sprintf(buffer, "ADJUST_COND_PARAMS");
			break;

		default:
			result = -1;
			break;
	}

	return result;
}

#endif


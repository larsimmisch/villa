/*------------------------------------------------------------*/
/* Copyright ACULAB plc. (c) 1996-2000                        */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : smbesp.h 	                      	  */
/*                                                            */
/*           Purpose : SHARC Module driver header file        */
/*                     specific to Basic and Enhanced Speech  */
/*                     processing.                            */
/*                                                            */
/*            Author : Peter Dain                             */
/*                                                            */
/*       Create Date : 21st February 1997                     */
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

#ifndef __SMBESP__
#define __SMBESP__

/*
 * Used to align API lib with driver.
 */
#define kBESPVersion  			"1.00"
#define kBESPVersionMaj  		1
#define kBESPVersionMin  		0
#define kBESPFWSpecificAPIId	8

#ifndef __SMDRVR__
#include "smdrvr.h"
#endif

#pragma pack ( 1 )


/*-------- BESP IOCTL Functions ---------*/
/*---------------------------------------*/

#define BESPIO_ADD_OUTPUT_FREQ			(SMIO_FW_MODULE_CONTROL_BASE+0)
#define BESPIO_ADD_OUTPUT_TONE			(SMIO_FW_MODULE_CONTROL_BASE+1)
#define BESPIO_ADD_INPUT_FREQ_COEFFS	(SMIO_FW_MODULE_CONTROL_BASE+2)
#define BESPIO_ADD_INPUT_TONE_SET		(SMIO_FW_MODULE_CONTROL_BASE+3)
#define BESPIO_RESET_INPUT_CPTONES		(SMIO_FW_MODULE_CONTROL_BASE+4)
#define BESPIO_ADD_INPUT_CPTONE			(SMIO_FW_MODULE_CONTROL_BASE+5)

#define BESPIO_RESET_INPUT_VOCABS		(SMIO_FW_MODULE_CONTROL_BASE+6)
#define BESPIO_VOCAB_INIT				(SMIO_FW_MODULE_CONTROL_BASE+7)
#define BESPIO_VOCAB_BUFFER				(SMIO_FW_MODULE_CONTROL_BASE+8)
#define BESPIO_VOCAB_COMPLETE			(SMIO_FW_MODULE_CONTROL_BASE+9)
#define BESPIO_ASR_MODULE_PARAMETERS	(SMIO_FW_MODULE_CONTROL_BASE+10)

/* Conferencing primitives where subject channel handle are replaced by channel ixs */
#define BESPIO_CONF_PRIMIX_START		(SMIO_FW_MODULE_CONTROL_BASE+11)
#define BESPIO_CONF_PRIMIX_ADD			(SMIO_FW_MODULE_CONTROL_BASE+12)
#define BESPIO_CONF_PRIMIX_LEAVE		(SMIO_FW_MODULE_CONTROL_BASE+13)
#define BESPIO_CONF_PRIMIX_INFO			(SMIO_FW_MODULE_CONTROL_BASE+14)
#define BESPIO_CONF_PRIMIX_ADJ_INPUT	(SMIO_FW_MODULE_CONTROL_BASE+15)
#define BESPIO_CONF_PRIMIX_ADJ_OUTPUT	(SMIO_FW_MODULE_CONTROL_BASE+16)
#define BESPIO_CONF_PRIMIX_ABORT		(SMIO_FW_MODULE_CONTROL_BASE+17)
#define BESPIO_CONF_PRIMIX_CLONE		(SMIO_FW_MODULE_CONTROL_BASE+18)

#define BESPIO_ADJUST_INPUT_TONE_SET	(SMIO_FW_MODULE_CONTROL_BASE+19)
#define BESPIO_ADJUST_CATSIG_PARAMS		(SMIO_FW_MODULE_CONTROL_BASE+20)
#define BESPIO_ADJUST_AGC_PARAMS		(SMIO_FW_MODULE_CONTROL_BASE+21)
#define BESPIO_ADJUST_COND_PARAMS		(SMIO_FW_MODULE_CONTROL_BASE+22)

#define BESPIO_REPLAY_START				(SMIO_FW_CHANNEL_CONTROL_BASE+0)
#define BESPIO_PUT_REPLAY_DATA			(SMIO_FW_CHANNEL_CONTROL_BASE+1)
#define BESPIO_REPLAY_STATUS			(SMIO_FW_CHANNEL_CONTROL_BASE+2)
#define BESPIO_REPLAY_ABORT				(SMIO_FW_CHANNEL_CONTROL_BASE+3)
#define BESPIO_REPLAY_ADJUST			(SMIO_FW_CHANNEL_CONTROL_BASE+4)
#define BESPIO_PLAY_TONE				(SMIO_FW_CHANNEL_CONTROL_BASE+5)
#define BESPIO_PLAY_CPTONE				(SMIO_FW_CHANNEL_CONTROL_BASE+6)
#define BESPIO_PLAY_DIGITS				(SMIO_FW_CHANNEL_CONTROL_BASE+7)
#define BESPIO_RECORD_START				(SMIO_FW_CHANNEL_CONTROL_BASE+8)
#define BESPIO_GET_RECORDED_DATA		(SMIO_FW_CHANNEL_CONTROL_BASE+9)
#define BESPIO_RECORD_STATUS			(SMIO_FW_CHANNEL_CONTROL_BASE+10)
#define BESPIO_RECORD_ABORT				(SMIO_FW_CHANNEL_CONTROL_BASE+11)
#define BESPIO_LISTEN_FOR				(SMIO_FW_CHANNEL_CONTROL_BASE+12)
#define BESPIO_CONF_PRIM_START			(SMIO_FW_CHANNEL_CONTROL_BASE+13)
#define BESPIO_CONF_PRIM_ADD			(SMIO_FW_CHANNEL_CONTROL_BASE+14)
#define BESPIO_CONF_PRIM_LEAVE			(SMIO_FW_CHANNEL_CONTROL_BASE+15)
#define BESPIO_CONF_PRIM_INFO			(SMIO_FW_CHANNEL_CONTROL_BASE+16)
#define BESPIO_CONF_PRIM_ADJ_INPUT		(SMIO_FW_CHANNEL_CONTROL_BASE+17)
#define BESPIO_CONF_PRIM_ADJ_OUTPUT		(SMIO_FW_CHANNEL_CONTROL_BASE+18)
#define BESPIO_CONF_PRIM_ABORT			(SMIO_FW_CHANNEL_CONTROL_BASE+19)
#define BESPIO_SET_SIDETONE_CHANNEL		(SMIO_FW_CHANNEL_CONTROL_BASE+20)
#define BESPIO_ASR_LF_DRV1				(SMIO_FW_CHANNEL_CONTROL_BASE+21)
#define BESPIO_ASR_LF_DRV2				(SMIO_FW_CHANNEL_CONTROL_BASE+22)	
/* Not used	(SMIO_FW_CHANNEL_CONTROL_BASE+23) */
#define BESPIO_RECORD_HOW_TERMINATED	(SMIO_FW_CHANNEL_CONTROL_BASE+24)
#define BESPIO_CONF_PRIM_CLONE			(SMIO_FW_CHANNEL_CONTROL_BASE+25)
#define BESPIO_PLAY_TONE_STATUS			(SMIO_FW_CHANNEL_CONTROL_BASE+26)
#define BESPIO_PLAY_CPTONE_STATUS		(SMIO_FW_CHANNEL_CONTROL_BASE+27)
#define BESPIO_PLAY_DIGITS_STATUS		(SMIO_FW_CHANNEL_CONTROL_BASE+28)
#define BESPIO_DISCARD_RECOGNISED		(SMIO_FW_CHANNEL_CONTROL_BASE+29)
#define BESPIO_REPLAY_DELAY				(SMIO_FW_CHANNEL_CONTROL_BASE+30)
/* Alternate i/o controls for 1.3 built application (unix multi-process apps fix) */
#define BESPIO_CONF_PRIM_ADD1			(SMIO_FW_CHANNEL_CONTROL_BASE+31)
#define BESPIO_CONF_PRIM_CLONE1			(SMIO_FW_CHANNEL_CONTROL_BASE+32)
#define BESPIO_REPLAY_START1			(SMIO_FW_CHANNEL_CONTROL_BASE+33)
#define BESPIO_REPLAY_ADJUST1			(SMIO_FW_CHANNEL_CONTROL_BASE+34)
#define BESPIO_RECORD_START1			(SMIO_FW_CHANNEL_CONTROL_BASE+35)
#define BESPIO_SET_SIDETONE_CHANNEL1	(SMIO_FW_CHANNEL_CONTROL_BASE+36)
/* more functions. */
#define BESPIO_COND_INPUT				(SMIO_FW_CHANNEL_CONTROL_BASE+37)
#define BESPIO_COND_REINIT				(SMIO_FW_CHANNEL_CONTROL_BASE+38)
#define BESPIO_PLAY_TONE_ABORT			(SMIO_FW_CHANNEL_CONTROL_BASE+39)
#define BESPIO_PLAY_CPTONE_ABORT		(SMIO_FW_CHANNEL_CONTROL_BASE+40)
#define BESPIO_GET_RECOGNISED			(SMIO_FW_CHANNEL_CONTROL_BASE+41)
#define BESPIO_READ_STATUS				(SMIO_FW_CHANNEL_CONTROL_BASE+42)
#define BESPIO_WRITE_STATUS				(SMIO_FW_CHANNEL_CONTROL_BASE+43)
#define BESPIO_PUT_LAST_REPLAY_DATA		(SMIO_FW_CHANNEL_CONTROL_BASE+44)
#define BESPIO_CATSIG_LISTEN_FOR		(SMIO_FW_CHANNEL_CONTROL_BASE+45)
#define BESPIO_READ_STATUS_IX			(SMIO_FW_CHANNEL_CONTROL_BASE+46)
#define BESPIO_WRITE_STATUS_IX			(SMIO_FW_CHANNEL_CONTROL_BASE+47)
#define BESPIO_RECORD_START2			(SMIO_FW_CHANNEL_CONTROL_BASE+48)
#define BESPIO_GET_RECOGNISED_IX		(SMIO_FW_CHANNEL_CONTROL_BASE+49)


/*
 * Note every structure must be less or equal in size to kMaxIOCTLSpaceParmsSize octets.
 */
typedef struct sm_besp_status_parms {
	tSMChannelId	channel;
	tSM_INT			status;
} SM_BESP_STATUS_PARMS;

typedef struct sm_besp_status_ix_parms {
	tSM_INT			channel_ix;
	tSM_INT			status;
} SM_BESP_STATUS_IX_PARMS;

typedef struct sm_output_freq_parms {
	tSM_INT		module;
	double		freq;
	double		amplitude;	
	tSM_INT		id;	
} SM_OUTPUT_FREQ_PARMS;


/*
 * Same as above with doubles converted to IEEE floats 
 * for portability - internal driver use only.
 */
typedef struct sm_output_freq_drv_parms {
	tSM_INT		module;
	tSM_UT8		freq[4];		/* 32 bit IEEE float format. */
	tSM_UT8		amplitude[4];	/* 32 bit IEEE float format. */
	tSM_INT		id;	
} SM_OUTPUT_FREQ_DRV_PARMS;


typedef struct sm_output_tone_parms {
	tSM_INT		module;
	tSM_INT		component1_id;
	tSM_INT		component2_id;
	tSM_INT		id;	
} SM_OUTPUT_TONE_PARMS;

typedef struct sm_input_freq_coeffs_parms {
	tSM_INT		module;
	double		upper_limit;
	double		lower_limit;
	tSM_INT		id;	
} SM_INPUT_FREQ_COEFFS_PARMS;

/*
 * Same as above with doubles converted to IEEE floats 
 * for portability - internal driver use only.
 */
typedef struct sm_input_freq_coeffs_drv_parms {
	tSM_INT		module;
	tSM_UT8		upper_limit[4];	/* 32 bit IEEE float format. */
	tSM_UT8		lower_limit[4];	/* 32 bit IEEE float format. */
	tSM_INT		id;	
} SM_INPUT_FREQ_COEFFS_DRV_PARMS;


typedef struct sm_input_tone_set_parms {
	tSM_INT		module;
	tSM_INT		band1_first_freq_coeffs_id;
	tSM_INT		band1_freq_count;
	tSM_INT		band2_first_freq_coeffs_id;
	tSM_INT		band2_freq_count;
	double		req_third_peak;
	double		req_signal_to_noise_ratio;
	double		req_minimum_power;
	double		req_twist_for_dual_tone;
	tSM_INT		id;
} SM_INPUT_TONE_SET_PARMS;


/*
 * Same as above with doubles converted to IEEE floats 
 * for portability - internal driver use only.
 */
typedef struct sm_input_tone_set_drv_parms {
	tSM_INT		module;
	tSM_INT		band1_first_freq_coeffs_id;
	tSM_INT		band1_freq_count;
	tSM_INT		band2_first_freq_coeffs_id;
	tSM_INT		band2_freq_count;
	tSM_UT8		req_third_peak[4];
	tSM_UT8		req_signal_to_noise_ratio[4];
	tSM_UT8		req_minimum_power[4];
	tSM_UT8		req_twist_for_dual_tone[4];
	tSM_INT		id;
} SM_INPUT_TONE_SET_DRV_PARMS;

#define kAdjustToneSetFPParamId3rdPeak 		1
#define kAdjustToneSetFPParamIdSNRatio 		2
#define kAdjustToneSetFPParamIdMinPower 	3
#define kAdjustToneSetFPParamIdTwist 		4
#define kAdjustToneSetIntParamIdMinOnTime 	5
#define kAdjustToneSetIntParamIdMinOffTime	6
#define kAdjustToneSetFPParamIdStartFreq 	7
#define kAdjustToneSetFPParamIdStopFreq		8

typedef struct sm_adjust_tone_set_drv_parms {
	tSM_INT		module;
	tSM_INT		tone_set_id;
	tSM_INT		parameter_id;
	tSM_UT8		fp_value[4];
	tSM_INT		int_value;
} SM_ADJUST_TONE_SET_DRV_PARMS;

typedef struct sm_adjust_tone_set_parms {
	tSM_INT		module;
	tSM_INT		tone_set_id;
	tSM_INT		parameter_id;
	union 
	{
		double		fp_value;
		tSM_INT		int_value;
	} parameter_value;
} SM_ADJUST_TONE_SET_PARMS;


typedef struct sm_reset_input_cptones_parms {
	tSM_INT module;
	tSM_INT	tone_set_id;
} SM_RESET_INPUT_CPTONES_PARMS;


#define kSMMaxCPToneStates 4
typedef struct sm_cptone_state {
	tSM_INT		freq_id;
	tSM_INT		maximum_cadence;
	tSM_INT		minimum_cadence;
} SM_CPTONE_STATE;

typedef struct sm_input_cptone_parms {
	tSM_INT 				module;
	tSM_INT					state_count;
	SM_CPTONE_STATE			states[kSMMaxCPToneStates];
	tSM_INT					id;
} SM_INPUT_CPTONE_PARMS;

typedef struct sm_replay_parms {
	tSMChannelId		channel;
	tSMChannelId		background;
	tSM_INT				volume; 			/* value in dBs, may range from +8 to -24 dB */	
	tSM_INT				agc;				/* Non-zero if AGC enabled. */
	tSM_INT				speed;				/* +/- percent deviation. (-31 to +31) */
	tSM_INT				type;
	tSM_UT32			data_length;
} SM_REPLAY_PARMS;

	
#define kSMDataFormat8KHzPCM		1
#define kSMDataFormat8KHzOKIADPCM	2
#define kSMDataFormat8KHzACUBLKPCM	3
#define kSMDataFormat6KHzPCM		4
#define kSMDataFormat6KHzOKIADPCM	5
#define kSMDataFormat6KHzACUBLKPCM	6

#define kSMDataFormat8KHzULawPCM	7
#define kSMDataFormat8KHzALawPCM	8

#define kSMDataFormatCELP8KBPS		9

#define kSMDataFormatG726_48KBPS	10
#define kSMDataFormatG726_32KBPS	11
#define kSMDataFormatG726_24KBPS	12
#define kSMDataFormatG726_16KBPS	13

#define kSMDataFormatG721 kSMDataFormatG726_32KBPS

#define kSMDataFormat8KHz16bitMono	14
#define kSMDataFormat8KHz8bitMono	15

#define kSMDataFormatIMAADPCM   	17

typedef struct sm_ts_data_parms {
	tSMChannelId	channel;
	char* 			data;
	tSM_INT 		length;
} SM_TS_DATA_PARMS;


typedef struct sm_last_ts_data_length_parms {
	tSMChannelId	channel;
	tSM_INT 		length;
	tSM_UT8			fill_octet;
	tSM_INT 		fill_count;
} SM_LAST_TS_DATA_LENGTH_PARMS;


#define kSMMaxReplayDataBufferSize 2048
#define kSMMaxRecordDataBufferSize 2048

typedef struct sm_replay_status_parms {
	tSMChannelId	channel;
	tSM_INT			status;
} SM_REPLAY_STATUS_PARMS;

#define kSMReplayStatusComplete			1
#define kSMReplayStatusCompleteData		2
#define kSMReplayStatusUnderrun			3
#define kSMReplayStatusHasCapacity		4
#define kSMReplayStatusNoCapacity		5


typedef struct sm_replay_abort_parms {
	tSMChannelId	channel;
	tSM_UT32		offset;
} SM_REPLAY_ABORT_PARMS;


typedef struct sm_replay_adjust_parms {
	tSMChannelId	channel;
	tSMChannelId	background;
	tSM_INT			volume; 			/* value in dBs, may range from +8 to -24 dB */	
	tSM_INT			agc;				/* Non-zero if AGC enabled. */
	tSM_INT			speed;				/* +/- percent deviation. (-31 to +31) */
} SM_REPLAY_ADJUST_PARMS;

typedef struct sm_replay_delay_parms {
	tSMChannelId	channel;
	tSM_UT32		underrun_delay;
} SM_REPLAY_DELAY_PARMS;

typedef struct sm_play_tone_parms {
	tSMChannelId	channel;
	tSM_UT32		duration;
	tSM_INT	 		wait_for_completion;
	tSM_INT			tone_id;
} SM_PLAY_TONE_PARMS;

#define kSMPlayToneStatusComplete		1
#define kSMPlayToneStatusOngoing		2

typedef struct sm_play_tone_status_parms {
	tSMChannelId	channel;
	tSM_INT			status;
} SM_PLAY_TONE_STATUS_PARMS;

#define kSMMaxPlayCPToneCadences 	8
#define kSMPlayCPToneTypeOneShot	1
#define kSMPlayCPToneTypeRepeat		2
#define kSMPlayCPToneTypeContinuous	3

typedef struct sm_cadence {
	tSM_INT	    tone_id;
	tSM_INT	 	on_cadence;
	tSM_INT		off_cadence;
} SM_CADENCE;

typedef struct sm_play_cptone_parms {
	tSMChannelId	channel;
	tSM_UT32		duration;
	tSM_INT	 		wait_for_completion;
	tSM_INT			type;
	tSM_INT			tone_count;
    SM_CADENCE  	cadences[kSMMaxPlayCPToneCadences];
} SM_PLAY_CPTONE_PARMS;

#define kSMPlayCPToneStatusComplete		1
#define kSMPlayCPToneStatusOngoing		2

typedef struct sm_play_cptone_status_parms {
	tSMChannelId	channel;
	tSM_INT			status;
} SM_PLAY_CPTONE_STATUS_PARMS;


#define kSMMaxDigits 32

typedef struct sm_digits {
	tSM_INT		type;
	tSM_INT		qualifier;
	char		digit_string[1+kSMMaxDigits];
    tSM_INT     inter_digit_delay;
    tSM_INT     digit_duration;
} SM_DIGITS;

#define kSMDTMFDigits 	1
#define kSMPulseDigits 	2	/* Note just for recognition, not for generation. */

typedef struct sm_play_digits_parms {
	tSMChannelId 	channel;
	tSM_INT	 		wait_for_completion;
	SM_DIGITS		digits;
} SM_PLAY_DIGITS_PARMS;

#define kSMPlayDigitsStatusComplete		1
#define kSMPlayDigitsStatusOngoing		2

typedef struct sm_play_digits_status_parms {
	tSMChannelId	channel;
	tSM_INT			status;
} SM_PLAY_DIGITS_STATUS_PARMS;

#define kSMChannelCapsRecAltDataSourceOnly 	(1<<14)

#define kSMDRecordNoElimination 		0
#define kSMDRecordSilenceElimination 	1
#define kSMDRecordToneElimination 		2
#define kSMDRecordVADInitElimination 	3
#define kSMDRecordVADNextElimination 	4


/*
 * Values for alt_data_source_type in sm_record_parms.
 */
#define kSMRecordAltSourceDefault		0
#define kSMRecordAltSourceInput			1
#define kSMRecordAltSourceOutput		2

typedef struct sm_record_parms {
	tSMChannelId	channel;
	tSMChannelId	alt_data_source;
	tSM_INT			type;
#ifdef BESP_PRE_316_NAME
	tSM_INT			silence_elimination;
#else
	tSM_INT			elimination;
#endif
	tSM_UT32		max_octets;
	tSM_UT32		max_elapsed_time;
	tSM_UT32  		max_silence;	
#ifndef _SM_BESP_PRE1P4_
	tSM_INT			agc;				/* Non-zero if AGC enabled. */
	tSM_INT			volume;	
	tSM_INT			alt_data_source_type;
#endif
} SM_RECORD_PARMS;


typedef struct sm_record_status_parms {
	tSMChannelId	channel;
	tSM_INT			status;
} SM_RECORD_STATUS_PARMS;


#define kSMRecordStatusComplete		1
#define kSMRecordStatusCompleteData	2
#define kSMRecordStatusOverrun		3
#define kSMRecordStatusData			4
#define kSMRecordStatusNoData		5


typedef struct sm_record_abort_parms {
	tSMChannelId	channel;
	tSM_INT			discard;
} SM_RECORD_ABORT_PARMS;


#define kSMRecordHowTerminatedNotYet	0
#define kSMRecordHowTerminatedLength	1
#define kSMRecordHowTerminatedMaxTime	2
#define kSMRecordHowTerminatedSilence	3
#define kSMRecordHowTerminatedAborted	4
#define kSMRecordHowTerminatedError		5


typedef struct sm_record_how_terminated_parms {
	tSMChannelId   	channel;
	tSM_INT			termination_reason;
	tSM_UT32		termination_octets;
} SM_RECORD_HOW_TERMINATED_PARMS;


/*
 * Mappings from tones to digits for field map_tones_to_digits 
 * in SM_LISTEN_FOR_PARMS.
 */ 
#define kSMNoDigitMapping			0
#define kSMDTMFToneSetDigitMapping 	1

/*
 * Tone detection modes for field tone_detection_mode
 * in SM_LISTEN_FOR_PARMS.
 */ 
#define kSMToneDetectionNone				0
#define kSMToneDetectionNoMinDuration		1
#define kSMToneDetectionMinDuration64		2
#define kSMToneDetectionMinDuration40		3
#define kSMToneEndDetectionNoMinDuration	4
#define kSMToneEndDetectionMinDuration64	5
#define kSMToneEndDetectionMinDuration40	6
#define kSMToneLenDetectionNoMinDuration	7
#define kSMToneLenDetectionMinDuration64	8
#define kSMToneLenDetectionMinDuration40	9


typedef struct sm_listen_for_parms {
	tSMChannelId   	channel;
	tSM_INT			enable_pulse_digit_recognition;
	tSM_INT			tone_detection_mode;
	tSM_INT			active_tone_set_id;
	tSM_INT			map_tones_to_digits;
	tSM_INT			enable_cptone_recognition;
	tSM_INT			enable_grunt_detection;
	tSM_INT			grunt_latency;
} SM_LISTEN_FOR_PARMS;


/*
 * F/W specific ids for generic get recognised.
 */
#define kSMRecognisedTrainingDigit	1
#define kSMRecognisedDigit			2
#define kSMRecognisedTone			3
#define kSMRecognisedCPTone			4
#define kSMRecognisedGruntStart		5
#define kSMRecognisedGruntEnd		6
#define kSMRecognisedASRResult		7
#define kSMRecognisedASRUncertain	8
#define kSMRecognisedASRRejected	9
#define kSMRecognisedASRTimeout		10
#define kSMRecognisedCatSig			11

typedef struct sm_conf_prim_start_parms {
	tSMChannelId	channel;
	tSM_INT			volume;
    tSM_INT   		agc;
} SM_CONF_PRIM_START_PARMS;


typedef struct sm_conf_prim_clone_parms {
	tSMChannelId	channel;
	tSMChannelId	model;
} SM_CONF_PRIM_CLONE_PARMS;


typedef struct sm_conf_prim_add_parms {
	tSMChannelId	channel;
	tSMChannelId	participant;
	tSM_INT			id;
} SM_CONF_PRIM_ADD_PARMS;


typedef struct sm_conf_prim_leave_parms {
	tSMChannelId	channel;
	tSM_INT			id;
} SM_CONF_PRIM_LEAVE_PARMS;


typedef struct sm_conf_prim_info_parms {
	tSMChannelId	channel;
	tSM_INT			participant_count;
	char			speakers[8];
} SM_CONF_PRIM_INFO_PARMS;

#define kSMConfAdjInputVolumeMute -23	

typedef struct sm_conf_prim_adj_input_parms {
	tSMChannelId	channel;
    tSM_INT   		volume;
    tSM_INT   		agc;
} SM_CONF_PRIM_ADJ_INPUT_PARMS;


typedef struct sm_conf_prim_adj_output_parms {
	tSMChannelId	channel;
	tSM_INT			volume;
    tSM_INT   		agc;
} SM_CONF_PRIM_ADJ_OUTPUT_PARMS;


typedef struct sm_conf_primix_start_parms {
	tSM_INT			channel_ix;
	tSM_INT			volume;
    tSM_INT   		agc;
} SM_CONF_PRIMIX_START_PARMS;


typedef struct sm_conf_primix_clone_parms {
	tSM_INT			channel_ix;
	tSM_INT			model_ix;
} SM_CONF_PRIMIX_CLONE_PARMS;


typedef struct sm_conf_primix_add_parms {
	tSM_INT			channel_ix;
	tSM_INT			participant_ix;
	tSM_INT			id;
} SM_CONF_PRIMIX_ADD_PARMS;


typedef struct sm_conf_primix_leave_parms {
	tSM_INT			channel_ix;
	tSM_INT			id;
} SM_CONF_PRIMIX_LEAVE_PARMS;


typedef struct sm_conf_primix_info_parms {
	tSM_INT			channel_ix;
	tSM_INT			participant_count;
	char			speakers[8];
} SM_CONF_PRIMIX_INFO_PARMS;


typedef struct sm_conf_primix_adj_input_parms {
	tSM_INT			channel_ix;
    tSM_INT   		volume;
    tSM_INT   		agc;
} SM_CONF_PRIMIX_ADJ_INPUT_PARMS;


typedef struct sm_conf_primix_adj_output_parms {
	tSM_INT			channel_ix;
	tSM_INT			volume;
    tSM_INT   		agc;
} SM_CONF_PRIMIX_ADJ_OUTPUT_PARMS;


typedef struct sm_set_sidetone_channel_parms {
	tSMChannelId	channel;
	tSMChannelId	output;
} SM_SET_SIDETONE_CHANNEL_PARMS;


typedef struct sm_reset_input_vocabs_parms {
	tSM_INT					module;
} SM_RESET_INPUT_VOCABS_PARMS;


typedef struct sm_input_vocab_parms {
	tSM_INT					module;
	char*					filename;
	tSM_UT32				item_id;
} SM_INPUT_VOCAB_PARMS;


typedef struct sm_vocab_init_parms {
	tSM_INT		module;
	tSM_INT		length;
} SM_VOCAB_INIT_PARMS;


typedef struct sm_vocab_buffer_parms {
	tSM_INT		module;
	tSM_INT		length;
	char 		vcbuffer[kMaxIOCTLSpaceParmsSize - (2*sizeof(tSM_INT))];	
} SM_VOCAB_BUFFER_PARMS;


typedef struct sm_vocab_complete_parms {
	tSM_INT		module;
	tSM_INT		abort;
	tSM_UT32	item_id;
} SM_VOCAB_COMPLETE_PARMS;


typedef struct sm_asr_characteristics {
	tSM_INT			vfr_max_frames;
	double			vfr_diff_threshold;
	tSM_INT			pse_max_frames;
	tSM_INT			pse_min_frames;
	double			pse_score_difference;
	double			pse_snr;
	double			pse_noise_score;
	double			pse_latency;
	double			vit_soft_threshold;
	double			vit_hard_threshold;
} SM_ASR_CHARACTERISTICS;


/*
 * Same as above with doubles converted to IEEE floats 
 * for portability - internal driver use only.
 */
typedef struct sm_asr_drv_characteristics {
	tSM_INT			vfr_max_frames;
	tSM_UT8			vfr_diff_threshold[4];
	tSM_INT			pse_max_frames;
	tSM_INT			pse_min_frames;
	tSM_UT8			pse_score_difference[4];
	tSM_UT8			pse_snr[4];
	tSM_UT8			pse_noise_score[4];
	tSM_UT8			pse_latency[4];
	tSM_UT8			vit_soft_threshold[4];
	tSM_UT8			vit_hard_threshold[4];
} SM_ASR_DRV_CHARACTERISTICS;


/*
 * Capability bit for ASR channels.
 */
#define kSMChannelCapsASR (1<<10)

/*
 * ASR modes for field mode in SM_ASR_LISTEN_PARMS.
 */
#define kSMASRModeDisabled 		0
#define kSMASRModeOneShot		1
#define kSMASRModeContinuous	2

typedef struct sm_asr_listen_for_parms {
	tSMChannelId			channel;
	tSM_INT					vocab_item_count;
	tSM_UT32*				vocab_item_ids;
	tSM_INT*				vocab_recog_ids;
	tSM_INT					asr_mode;
	SM_ASR_CHARACTERISTICS*	specific_parameters;
} SM_ASR_LISTEN_FOR_PARMS;

typedef struct sm_asr_lf_drv1_parms {
	tSMChannelId			channel;
	tSM_INT					mode;
	tSM_INT					is_new_item_list;
	tSM_INT					is_list_complete;
	tSM_INT					vocab_item_count;
	tSM_UT32				vocab_item_ids[8];
	tSM_INT					vocab_recog_ids[8];
} SM_ASR_LF_DRV1_PARMS;

typedef struct sm_asr_lf_drv2_parms {
	tSMChannelId				channel;
	SM_ASR_DRV_CHARACTERISTICS	specific_parameters;
} SM_ASR_LF_DRV2_PARMS;

typedef struct sm_asr_module_parameters_parms {
	tSM_INT					module;
	SM_ASR_CHARACTERISTICS	characteristics;
} SM_ASR_MODULE_PARAMETERS_PARMS;

/*
 * Same as above with doubles converted to IEEE floats 
 * for portability - internal driver use only.
 */
typedef struct sm_asr_mp_drv_parms {
	tSM_INT						module;
	SM_ASR_DRV_CHARACTERISTICS	characteristics;
} SM_ASR_MP_DRV_PARMS;


#define kSMInputCondRefNone			0
#define kSMInputCondRefUseInput		1
#define kSMInputCondRefUseOutput	2

#define kSMInputCondNone				0
#define kSMInputCondEchoCancelation		1

#define kSMInputCondAltDestNone			0
#define kSMInputCondAltDestInput		1
#define kSMInputCondAltDestOutput		2

/*
 * Capability bit for input conditioning reference channels.
 */
#define kSMChannelCapsCondRefInput 		(1<<11)

/*
 * Capability bit to allow maximum no. of input conditioning enabled channels 
 * (may be used when input conditioning not used in combination with other algorithms
 *  such as conferencing).
 */
#define kSMChannelCapsCondStandAlone 	(1<<12)
#define kSMChannelCapsCondHalfLoad	 	(1<<13)

typedef struct sm_condition_input_parms {
	tSMChannelId	channel;
	tSMChannelId	reference;
	tSM_INT			reference_type;
	tSM_INT   		conditioning_type;
	tSM_INT   		conditioning_param;
	tSMChannelId	alt_data_dest;
	tSM_INT			alt_dest_type;
} SM_CONDITION_INPUT_PARMS;


#define kBESPCatSigAlgLiveSpeaker 1

typedef struct sm_catsig_listen_for_parms {
	tSMChannelId			channel;
	tSM_INT					catsig_alg_id;
	tSM_INT					abort_catsig_alg;
} SM_CATSIG_LISTEN_FOR_PARMS;


typedef struct sm_adjust_catsig_module_drv_parms {
	tSM_INT		module;
	tSM_INT		catsig_alg_id;
	tSM_INT		parameter_id;
	tSM_UT8		fp_value[4];
	tSM_INT		int_value;
} SM_ADJUST_CATSIG_MODULE_DRV_PARMS;

typedef struct sm_adjust_catsig_module_parms {
	tSM_INT		module;
	tSM_INT		catsig_alg_id;
	tSM_INT		parameter_id;
	union 
	{
		double		fp_value;
		tSM_INT		int_value;
	} parameter_value;
} SM_ADJUST_CATSIG_MODULE_PARMS;

#define kSMAGCTypeRecord	0
#define kSMAGCTypeReplay	1

#define kAdjustAGCFPParamIxMaxIncreseRate 	0
#define kAdjustAGCFPParamIxMaxDecreaseRate	1
#define kAdjustAGCFPParamIxSensitivity	 	2

#define kAdjustAGCDrvFPGetParamIxBlockMult	0
#define kAdjustAGCDrvFPGetParamIxMuLevel	1
#define kAdjustAGCDrvFPGetParamIxMuEnergy	2

#define kAdjustAGCDrvFPPutParamIxLMaxDecay		0
#define kAdjustAGCDrvFPPutParamIxGainInc		1
#define kAdjustAGCDrvFPPutParamIxSqrGainInc		2
#define kAdjustAGCDrvFPPutParamIxRecipGainInc	3
#define kAdjustAGCDrvFPPutParamIxGainRed		4
#define kAdjustAGCDrvFPPutParamIxSqrRedOverInc	5

typedef struct sm_adjust_agc_module_drv_parms {
	tSM_INT		module;
	tSM_INT		is_put;
	tSM_INT		agc_type;
	tSM_UT8		fp_values[8][4];
	tSM_INT		int_values[4];
} SM_ADJUST_AGC_MODULE_DRV_PARMS;

typedef struct sm_adjust_agc_module_parms {
	tSM_INT		module;
	tSM_INT		agc_type;
	double		fp_params[4];
} SM_ADJUST_AGC_MODULE_PARMS;

typedef struct sm_adjust_cond_module_drv_parms {
	tSM_INT		module;
	tSM_INT		conditioning_type;
	tSM_INT		parameter_id;
	tSM_UT8		fp_value[4];
	tSM_INT		int_value;
} SM_ADJUST_COND_MODULE_DRV_PARMS;

typedef struct sm_adjust_cond_module_parms {
	tSM_INT		module;
	tSM_INT		conditioning_type;
	tSM_INT		parameter_id;
	union 
	{
		double		fp_value;
		tSM_INT		int_value;
	} parameter_value;
} SM_ADJUST_COND_MODULE_PARMS;


#ifdef __cplusplus
extern "C" {
#endif

ACUDLL int sm_add_output_freq( 
	struct sm_output_freq_parms*
);

ACUDLL int sm_add_output_tone( 
	struct sm_output_tone_parms*
);

ACUDLL int sm_add_input_freq_coeffs( 
	struct sm_input_freq_coeffs_parms*
);

ACUDLL int sm_add_input_tone_set( 
	struct sm_input_tone_set_parms*
);

ACUDLL int sm_adjust_input_tone_set( 
	struct sm_adjust_tone_set_parms*
);

ACUDLL int sm_reset_input_cptones( 
	struct sm_reset_input_cptones_parms*
);

ACUDLL int sm_add_input_cptone( 
	struct sm_input_cptone_parms*
);

ACUDLL int sm_replay_start( 
	struct sm_replay_parms*
);

ACUDLL int sm_put_replay_data( 
	struct sm_ts_data_parms*
);

ACUDLL int sm_put_last_replay_data( 
	struct sm_ts_data_parms*
);

ACUDLL int sm_replay_status( 
	struct sm_replay_status_parms*
);

ACUDLL int sm_replay_abort( 
	struct sm_replay_abort_parms*
);

ACUDLL int sm_replay_adjust( 
	struct sm_replay_adjust_parms*
);

ACUDLL int sm_replay_delay( 
	struct sm_replay_delay_parms*
);

ACUDLL int sm_play_tone( 
	struct sm_play_tone_parms*
);

ACUDLL int sm_play_tone_abort( 
	tSMChannelId 
);

ACUDLL int sm_play_tone_status( 
	struct sm_play_tone_status_parms*
);

ACUDLL int sm_play_cptone( 
	struct sm_play_cptone_parms*
);

ACUDLL int sm_play_cptone_abort( 
	tSMChannelId 
);

ACUDLL int sm_play_cptone_status( 
	struct sm_play_cptone_status_parms*
);

ACUDLL int sm_play_digits( 
	struct sm_play_digits_parms*
);

ACUDLL int sm_play_digits_status( 
	struct sm_play_digits_status_parms*
);

ACUDLL int sm_record_start( 
	struct sm_record_parms*
);

ACUDLL int sm_get_recorded_data( 
	struct sm_ts_data_parms*
);

ACUDLL int sm_record_status( 
	struct sm_record_status_parms*
);

ACUDLL int sm_record_abort( 
	struct sm_record_abort_parms*
);

ACUDLL int sm_record_how_terminated(
	struct sm_record_how_terminated_parms*
);

ACUDLL int sm_besp_read_status(
	struct sm_besp_status_parms*
);

ACUDLL int sm_besp_write_status(
	struct sm_besp_status_parms*
);

ACUDLL int sm_besp_read_status_ix(
	struct sm_besp_status_ix_parms*
);

ACUDLL int sm_besp_write_status_ix(
	struct sm_besp_status_ix_parms*
);

ACUDLL int sm_listen_for( 
	struct sm_listen_for_parms*
);

ACUDLL int sm_conf_prim_start( 
	struct sm_conf_prim_start_parms* 
);

ACUDLL int sm_conf_prim_clone( 
	struct sm_conf_prim_clone_parms* 
);

ACUDLL int sm_conf_prim_add( 
	struct sm_conf_prim_add_parms* 
);

ACUDLL int sm_conf_prim_leave( 
	struct sm_conf_prim_leave_parms* 
);

ACUDLL int sm_conf_prim_info( 
	struct sm_conf_prim_info_parms* 
);

ACUDLL int sm_conf_prim_adj_input( 
	struct sm_conf_prim_adj_input_parms*
);

ACUDLL int sm_conf_prim_adj_output( 
	struct sm_conf_prim_adj_output_parms* 
);

ACUDLL int sm_conf_prim_abort( 
	tSMChannelId 
);

ACUDLL int sm_conf_primix_start( 
	struct sm_conf_primix_start_parms* 
);

ACUDLL int sm_conf_primix_clone( 
	struct sm_conf_primix_clone_parms* 
);

ACUDLL int sm_conf_primix_add( 
	struct sm_conf_primix_add_parms* 
);

ACUDLL int sm_conf_primix_leave( 
	struct sm_conf_primix_leave_parms* 
);

ACUDLL int sm_conf_primix_info( 
	struct sm_conf_primix_info_parms* 
);

ACUDLL int sm_conf_primix_adj_input( 
	struct sm_conf_primix_adj_input_parms*
);

ACUDLL int sm_conf_primix_adj_output( 
	struct sm_conf_primix_adj_output_parms* 
);

ACUDLL int sm_conf_primix_abort( 
	tSM_INT channelIndex
);

ACUDLL int sm_set_sidetone_channel( 
	 struct sm_set_sidetone_channel_parms* 
);

ACUDLL int sm_reset_input_vocabs( 
	struct sm_reset_input_vocabs_parms*
);

ACUDLL int sm_add_input_vocab( 
	struct sm_input_vocab_parms*
);

ACUDLL int sm_asr_listen_for(
	struct sm_asr_listen_for_parms*
);

ACUDLL int sm_asr_module_parameters(
	struct sm_asr_module_parameters_parms*
);

ACUDLL int sm_get_recognised( 
	struct sm_recognised_parms* 
);

ACUDLL int sm_get_recognised_ix( 
	struct sm_recognised_ix_parms* 
);

ACUDLL int sm_discard_recognised(
	tSMChannelId
);

ACUDLL int sm_condition_input( 
	struct sm_condition_input_parms* 
);

ACUDLL int sm_condition_reinit( 
	tSMChannelId	channel
);

ACUDLL int sm_adjust_catsig_module_params(
	struct sm_adjust_catsig_module_parms*
);

ACUDLL int sm_catsig_listen_for(
	struct sm_catsig_listen_for_parms*
);

ACUDLL int sm_adjust_agc_module_params(
	struct sm_adjust_agc_module_parms*
);

ACUDLL int sm_adjust_cond_module_params(
	struct sm_adjust_cond_module_parms*
);

#ifdef SM_CRACK_API

int sm_crack_besp_ioctl( int, char*  );

#endif

#ifdef __cplusplus
}
#endif


#pragma pack ( )

#endif


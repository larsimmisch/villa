/*------------------------------------------------------------*/
/* Copyright ACULAB plc. (c) 1996-1997                        */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : smdrvr.h 	                      	  */
/*                                                            */
/*           Purpose : SHARC Module driver header file        */
/*                                                            */
/*            Author : Peter Dain                             */
/*                                                            */
/*       Create Date : 21st February 1997                     */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/

#ifndef __SMDRVR__
#define __SMDRVR__

#define kSMDVersion  	"1.00"
#define kSMDVersionMaj  1
#define kSMDVersionMin  0

#ifndef __DRIVERVERSIONONLY__

/*
 * Includes definitions for portability across different operating systems.
 */
#ifdef ACU_DIGITAL
#include <sys/ioctl.h>
#endif

#ifndef __SMPORT__
#include "smport.h"
#endif

#pragma pack ( 1 )


/*
 * Errors possibly returned by low level API.
 */
#define ERR_SM_DEVERR					-1
#define ERR_SM_PENDING					-2
#define ERR_SM_MORE						-3
#define ERR_SM_NO_ACTION				-4
#define ERR_SM_NOT_IMPLEMENTED         	-100
#define ERR_SM_BAD_PARAMETER           	-101
#define ERR_SM_NO_SUCH_MODULE          	-102
#define ERR_SM_MODULE_ACCESS			-103
#define ERR_SM_FILE_ACCESS             	-104
#define ERR_SM_FILE_FORMAT             	-105
#define ERR_SM_DOWNLOAD                	-106
#define ERR_SM_NO_RESOURCES            	-107
#define ERR_SM_FIRMWARE_NOT_RUNNING    	-108
#define ERR_SM_MODULE_ALREADY_RUNNING  	-109
#define ERR_SM_NO_SUCH_CHANNEL         	-110
#define ERR_SM_NO_DATA_AVAILABLE       	-111
#define ERR_SM_NO_RECORD_IN_PROGRESS   	-112
#define ERR_SM_NO_CAPACITY             	-113
#define ERR_SM_NO_REPLAY_IN_PROGRESS   	-114
#define ERR_SM_BAD_DATA_LENGTH         	-115
#define ERR_SM_WRONG_CHANNEL_STATE     	-116
#define ERR_SM_WRONG_CHANNEL_TYPE      	-117
#define ERR_SM_NO_SUCH_GROUP		   	-118
#define ERR_SM_NO_SUCH_FIRMWARE		   	-119
#define ERR_SM_NO_SUCH_DIGIT		   	-120
#define ERR_SM_CHANNEL_ALLOCATED	   	-121
#define ERR_SM_NO_DIGIT				   	-122
#define ERR_SM_NOT_SAME_MODULE		   	-123
#define ERR_SM_INCOMPATIBLE_DRIVER		-124
#define ERR_SM_INCOMPATIBLE_APP			-125
#define ERR_SM_WRONG_MODULE_TYPE		-126
#define ERR_SM_NOT_SAME_GROUP			-127
#define ERR_SM_NO_LICENSE				-128
#define ERR_SM_NO_LICENCE				-128 /* alternative spelling */
#define ERR_SM_WRONG_FIRMWARE_TYPE      -129
#define ERR_SM_FIRMWARE_PROBLEM		    -130
#define ERR_SM_OS_RESOURCE_PROBLEM	    -131
#define ERR_SM_WRONG_MODULE_STATE     	-132
#define ERR_SM_NO_ASSOCIATED_SWITCH    	-133
#define ERR_SM_OS_OTHER_PROBLEM		    -134
#define ERR_SM_OS_INTERRUPTED_WAIT	    -135


/*------------- IOCTL Functions ---------*/
/*---------------------------------------*/

/*
 * Note: function codes must be as follows:
 *   0  - 31  : generic i/o control (applies to all f/w)
 *  32  - 47  : diagnostic i/o control (used for driver development)
 *  48  - 127 : f/w specific module control  (no associated channel)
 *  128 - 255 : f/w specific channel control (always an associated channel)
 */

#ifndef ACU_DIGITAL

#define SMIO_GENERIC_BASE				0
#define SMIO_DIAG_BASE					32
#define SMIO_SUP_GENERIC_BASE			44
#define	SMIO_FW_MODULE_CONTROL_BASE		48
#define SMIO_FW_CHANNEL_CONTROL_BASE	128

#define SMIO_TRACE_CONTROL				(SMIO_GENERIC_BASE+0)
#define SMIO_GET_MODULES				(SMIO_GENERIC_BASE+1)
#define SMIO_RESET_MODULE				(SMIO_GENERIC_BASE+2)
#define SMIO_DOWNLOAD_INIT				(SMIO_GENERIC_BASE+3)
#define SMIO_DOWNLOAD_BUFFER			(SMIO_GENERIC_BASE+4)
#define SMIO_DOWNLOAD_COMPLETE			(SMIO_GENERIC_BASE+5)
#define SMIO_FIRMWARE_CAPS_BUFFER		(SMIO_GENERIC_BASE+6)
#define SMIO_GET_MODULE_INFO			(SMIO_GENERIC_BASE+7)
#define SMIO_CONFIG_MODULE_SW			(SMIO_GENERIC_BASE+8)
#define SMIO_GET_CHANNEL_IX_MODULE_IX	(SMIO_GENERIC_BASE+9)
#define SMIO_GET_EV_MECH				(SMIO_GENERIC_BASE+10)
#define SMIO_CHANNEL_ALLOC				(SMIO_GENERIC_BASE+11)
#define SMIO_STORE_APP_CHANNEL_ID		(SMIO_GENERIC_BASE+12)
#define SMIO_CHANNEL_SET_EVENT			(SMIO_GENERIC_BASE+13)
#define SMIO_CHANNEL_RELEASE			(SMIO_GENERIC_BASE+14)
#define SMIO_CHANNEL_INFO				(SMIO_GENERIC_BASE+15)
#define SMIO_RESET_CHANNEL				(SMIO_GENERIC_BASE+16)
#define SMIO_GET_CHANNEL_IX				(SMIO_GENERIC_BASE+17)
#define SMIO_SWITCH_CHANNEL_OUTPUT		(SMIO_GENERIC_BASE+18)
#define SMIO_SWITCH_CHANNEL_INPUT		(SMIO_GENERIC_BASE+19)
#define SMIO_GET_RECOGNISED_GN			(SMIO_GENERIC_BASE+20)	/* Backwards compat with pre 1.3.5 */
#define SMIO_GET_CARDS					(SMIO_GENERIC_BASE+21)
#define SMIO_GET_CARD_INFO				(SMIO_GENERIC_BASE+22)
#define SMIO_GET_CHANNEL_MODULE_IX		(SMIO_GENERIC_BASE+23)
#define SMIO_GET_MODULE_CARD_IX			(SMIO_GENERIC_BASE+24)
#define SMIO_GET_DRIVER_INFO			(SMIO_GENERIC_BASE+25)
#define SMIO_CHANNEL_VALIDATE_ID		(SMIO_GENERIC_BASE+26)
#define SMIO_GET_CARD_REV				(SMIO_GENERIC_BASE+27)
#define SMIO_GET_CHANNEL_TYPE			(SMIO_GENERIC_BASE+28)
#define SMIO_CHANNEL_ALLOC_PLACED		(SMIO_GENERIC_BASE+29)
#define SMIO_GET_CARD_SWITCH_IX			(SMIO_GENERIC_BASE+30)
#define SMIO_ENABLE_LICENCE				(SMIO_GENERIC_BASE+31)
#define SMIO_FIND_CHANNEL				(SMIO_SUP_GENERIC_BASE+0)
#define SMIO_CHANNEL_IX_INFO			(SMIO_SUP_GENERIC_BASE+1)
#define SMIO_SWITCH_CHANNEL_IX			(SMIO_SUP_GENERIC_BASE+2)
#define SMIO_CHANNEL_IX_DUMP			(SMIO_SUP_GENERIC_BASE+3)

#else

#define SMIO_GENERIC_BASE				(int)_IOWR('m', 0, SMMSGBLK)
#define SMIO_DIAG_BASE					(int)_IOWR('m', 32, SMMSGBLK)
#define SMIO_SUP_GENERIC_BASE			(int)_IOWR('m', 44, SMMSGBLK)
#define	SMIO_FW_MODULE_CONTROL_BASE		(int)_IOWR('m', 48, SMMSGBLK)
#define SMIO_FW_CHANNEL_CONTROL_BASE	(int)_IOWR('m', 128, SMMSGBLK)

#define SMIO_TRACE_CONTROL				(int)(SMIO_GENERIC_BASE+0)
#define SMIO_GET_MODULES				(int)(SMIO_GENERIC_BASE+1)
#define SMIO_RESET_MODULE				(int)(SMIO_GENERIC_BASE+2)
#define SMIO_DOWNLOAD_INIT				(int)(SMIO_GENERIC_BASE+3)
#define SMIO_DOWNLOAD_BUFFER			(int)(SMIO_GENERIC_BASE+4)
#define SMIO_DOWNLOAD_COMPLETE			(int)(SMIO_GENERIC_BASE+5)
#define SMIO_FIRMWARE_CAPS_BUFFER		(int)(SMIO_GENERIC_BASE+6)
#define SMIO_GET_MODULE_INFO			(int)(SMIO_GENERIC_BASE+7)
#define SMIO_CONFIG_MODULE_SW			(int)(SMIO_GENERIC_BASE+8)
#define SMIO_GET_CHANNEL_IX_MODULE_IX	(int)(SMIO_GENERIC_BASE+9)
#define SMIO_GET_EV_MECH				(int)(SMIO_GENERIC_BASE+10)
#define SMIO_CHANNEL_ALLOC				(int)(SMIO_GENERIC_BASE+11)
#define SMIO_STORE_APP_CHANNEL_ID		(int)(SMIO_GENERIC_BASE+12)
#define SMIO_CHANNEL_SET_EVENT			(int)(SMIO_GENERIC_BASE+13)
#define SMIO_CHANNEL_RELEASE			(int)(SMIO_GENERIC_BASE+14)
#define SMIO_CHANNEL_INFO				(int)(SMIO_GENERIC_BASE+15)
#define SMIO_RESET_CHANNEL				(int)(SMIO_GENERIC_BASE+16)
#define SMIO_GET_CHANNEL_IX				(int)(SMIO_GENERIC_BASE+17)
#define SMIO_SWITCH_CHANNEL_OUTPUT		(int)(SMIO_GENERIC_BASE+18)
#define SMIO_SWITCH_CHANNEL_INPUT		(int)(SMIO_GENERIC_BASE+19)
#define SMIO_GET_RECOGNISED_GN			(int)(SMIO_GENERIC_BASE+20)	/* Backwards compat with pre 1.3.5 */
#define SMIO_GET_CARDS					(int)(SMIO_GENERIC_BASE+21)
#define SMIO_GET_CARD_INFO				(int)(SMIO_GENERIC_BASE+22)
#define SMIO_GET_CHANNEL_MODULE_IX		(int)(SMIO_GENERIC_BASE+23)
#define SMIO_GET_MODULE_CARD_IX			(int)(SMIO_GENERIC_BASE+24)
#define SMIO_GET_DRIVER_INFO			(int)(SMIO_GENERIC_BASE+25)
#define SMIO_CHANNEL_VALIDATE_ID		(int)(SMIO_GENERIC_BASE+26)
#define SMIO_GET_CARD_REV				(int)(SMIO_GENERIC_BASE+27)
#define SMIO_GET_CHANNEL_TYPE			(int)(SMIO_GENERIC_BASE+28)
#define SMIO_CHANNEL_ALLOC_PLACED		(int)(SMIO_GENERIC_BASE+29)
#define SMIO_GET_CARD_SWITCH_IX			(int)(SMIO_GENERIC_BASE+30)
#define SMIO_ENABLE_LICENCE				(int)(SMIO_GENERIC_BASE+31)
#define SMIO_FIND_CHANNEL				(int)(SMIO_SUP_GENERIC_BASE+0)
#define SMIO_CHANNEL_IX_INFO			(int)(SMIO_SUP_GENERIC_BASE+1)
#define SMIO_SWITCH_CHANNEL_IX			(int)(SMIO_SUP_GENERIC_BASE+2)
#define SMIO_CHANNEL_IX_DUMP			(int)(SMIO_SUP_GENERIC_BASE+3)

#endif


/*
 * Maximum size of block copied to driver i/o space in i/o call.
 */
#ifdef __OS2__
#define kMaxIOCTLSpaceParmsSize (128-10)
#else
#define kMaxIOCTLSpaceParmsSize 128
#endif

typedef struct sm_driver_info_parms {
	tSM_INT		major;
	tSM_INT		minor;
	tSM_INT		step;	
	tSM_INT		custom;	
	tSM_INT		quality;	
	tSM_INT		buildno0;	
	tSM_INT		buildno1;	
} SM_DRIVER_INFO_PARMS;


#define kSMMaxSerialNoText 12

#define kSMCarrierCardTypeYetUnknown	0
#define kSMCarrierCardTypeISAS2 		1
#define kSMCarrierCardTypeBR4 			2
#define kSMCarrierCardTypeBR8 			3
#define kSMCarrierCardTypePCIP1			4
#define kSMCarrierCardTypePCIT1			5
#define kSMCarrierCardTypePCIC1			6

typedef struct sm_card_info_parms {
	tSM_INT		card;
	tSM_INT		card_type;
	tSM_INT		card_detected;	
	tSM_INT		module_count;	
	tSM_UT32	physical_address;	
	tSM_UT32	io_address;
	tSM_UT32	physical_irq;
	char		serial_no[kSMMaxSerialNoText];
} SM_CARD_INFO_PARMS;

#define kSMMaxRevAddText 64
typedef struct sm_card_rev_parms {
	tSM_INT		card;
	tSM_INT		rev[8];
	char		rev_additional[kSMMaxRevAddText];	
} SM_CARD_REV_PARMS;

typedef struct sm_download_parms {
	tSM_INT		module;
	tSM_INT		id;	
	char* 		filename;	
} SM_DOWNLOAD_PARMS;

typedef struct sm_download_buffer_parms {
	tSM_INT		module;
	tSM_INT		length;
	char 		fwbuffer[kMaxIOCTLSpaceParmsSize - (2*sizeof(tSM_INT))];	
} SM_DOWNLOAD_BUFFER_PARMS;

typedef struct sm_download_complete_parms {
	tSM_INT		module;
	tSM_INT		id;
	tSM_INT		abort;
} SM_DOWNLOAD_COMPLETE_PARMS;


/*
 * Maximum length of f/w capabilities header (not including initial 4 octet signature).
 */
#define kSMMaxFWCapsLen 1024

/*
 * Note this structure not passed through to Kernel so can be big.
 */
typedef struct sm_fwcaps_parms {
	tSM_INT 	module;
	tSM_INT		caps_length;
	char 		caps[kSMMaxFWCapsLen];
} SM_FWCAPS_PARMS;

typedef struct sm_fwcaps_buffer_parms {
	tSM_INT 	module;
	tSM_INT		length;
	tSM_INT		offset;
	char 		buffer[kMaxIOCTLSpaceParmsSize - (3*sizeof(tSM_INT))];
} SM_FWCAPS_BUFFER_PARMS;


typedef struct sm_config_module_sw_parms {
	tSM_INT module;
	tSM_INT auto_assign_out_ts;
	tSM_INT auto_assign_in_ts;
	tSM_INT out_st0;
	tSM_INT out_st1;
	tSM_INT out_base_ts;
	tSM_INT in_st0;
	tSM_INT in_st1;
	tSM_INT in_base_ts;
} SM_CONFIG_MODULE_SW_PARMS;


typedef struct sm_module_info_parms {
	tSM_INT		module;
	tSM_INT		firmware_running;	
	tSM_INT		firmware_id;	
	tSM_INT		average_loading;	
	tSM_INT		max_loading;	
} SM_MODULE_INFO_PARMS;

/*
 * Following enumeration defines type of channel.
 */
#define kSMChannelTypeInput 		0
#define kSMChannelTypeOutput 		1
#define kSMChannelTypeHalfDuplex 	2
#define kSMChannelTypeFullDuplex 	3


/*
 * Following bits may be set in channel capabilities bit mask - caps_mask.
 * For default capabilities, set caps_mask to zero.
 */
#define kSMChannelCapsWriteDataAbility	(1<<0)
#define kSMChannelCapsReadDataAbility	(1<<1)
#define kSMChannelCapsRecogAbility		(1<<2)
#define kSMChannelCapsALaw				(1<<3)
#define kSMChannelCapsULaw				(1<<4)
#define kSMChannelCapsNoCompanding		(1<<5)
#define kSMChannelCapsNoSwitching		(1<<6)


typedef struct sm_channel_alloc_parms {
	tSMChannelId	channel;
	tSM_INT			type;
	tSM_INT			firmware_id;
	tSM_INT			group;	
	tSM_INT			caps_mask;
} SM_CHANNEL_ALLOC_PARMS;

typedef struct sm_channel_alloc_placed_parms {
	tSMChannelId	channel;
	tSM_INT			type;
	tSM_INT			module;
	tSM_INT			caps_mask;
} SM_CHANNEL_ALLOC_PLACED_PARMS;

typedef struct sm_switch_channel_parms {
	tSMChannelId	channel;
	tSM_INT			st;
	tSM_INT			ts;
} SM_SWITCH_CHANNEL_PARMS;

typedef struct sm_switch_channel_ix_parms {
	tSM_INT			channel_ix;
	tSM_INT			st;
	tSM_INT			ts;
	tSM_INT			switch_output;
} SM_SWITCH_CHANNEL_IX_PARMS;

typedef struct sm_find_channel_parms {
	tSM_INT			is_output;
	tSM_INT			st;
	tSM_INT			ts;
	tSM_INT			card;
	tSM_INT			from_ix;
	tSMChannelId	channel;
	tSM_INT			channel_ix;
} SM_FIND_CHANNEL_PARMS;


/*
 * Following event types are defined.
 */
#define kSMEventTypeWriteData	1
#define kSMEventTypeReadData	2
#define kSMEventTypeRecog		3


/*
 * Following may be set in issue_events.
 */
#define kSMChannelNoEvent 		0
#define kSMChannelSpecificEvent 1
#define kSMAnyChannelEvent 		2

typedef struct sm_channel_set_event_parms {
	tSMChannelId	channel;
	tSM_INT			event_type;
	tSM_INT			issue_events;
	tSMEventId		event;
} SM_CHANNEL_SET_EVENT_PARMS;

typedef struct sm_channel_info_parms {
	tSMChannelId	channel;
	tSM_INT			card;	
	tSM_INT			ist;
	tSM_INT			its;
	tSM_INT			ost;
	tSM_INT			ots;
	tSM_INT			group;
	tSM_INT			caps_mask;
} SM_CHANNEL_INFO_PARMS;

typedef struct sm_channel_ix_info_parms {
	tSM_INT			channel_ix;
	tSM_INT			switch_ix;	
	tSM_INT			ist;
	tSM_INT			its;
	tSM_INT			ost;
	tSM_INT			ots;
	tSM_INT			group;
	tSM_INT			caps_mask;
	tSM_INT			type;
	tSM_INT			module;
} SM_CHANNEL_IX_INFO_PARMS;

#define kSMChIxDumpCmdReadRecogWaiting 		0
#define kSMChIxDumpCmdReadGenMemLength 		1
#define kSMChIxDumpCmdReadGenMem 			2
#define kSMChIxDumpCmdReadFWMemLength 		3
#define kSMChIxDumpCmdReadFWMem 			4
#define kSMChIxDumpCmdReadOSInfo 			5

typedef struct sm_channel_ix_dump_parms {
	tSM_INT			channel_ix;
	tSM_INT			word_count;		/* Count of 32 bit words to retrive. */
	tSM_INT			cmd;
	tSM_INT			use_weak_lock;
	tSM_UT32		addr;
	tSM_UT32		data[(kMaxIOCTLSpaceParmsSize - ((4*sizeof(tSM_INT)) + (2*sizeof(tSM_UT32))))/sizeof(tSM_UT32)];
} SM_CHANNEL_IX_DUMP_PARMS;


/*
 * Generic codes - others specific to f/w.
 */
#define kSMRecognisedNothing		0
#define kSMRecognisedOverrun		255

typedef struct sm_recognised_parms {
	tSMChannelId	channel;
	tSM_INT			type;
    tSM_INT			param0;
	tSM_INT			param1;
} SM_RECOGNISED_PARMS;

typedef struct sm_recognised_ix_parms {
	tSM_INT			channel_ix;
	tSM_INT			type;
    tSM_INT			param0;
	tSM_INT			param1;
} SM_RECOGNISED_IX_PARMS;

/*
 * SerialNumber/DongleNumber/AlgorithmID/NumLicenses/Checksum
 */
#define kMaxLicenceStringLength (kSMMaxSerialNoText+1+(2*16)+1+5+1+5+1+(2*16)+1)

typedef struct sm_enable_licence_parms {
	char	licence_string[kMaxLicenceStringLength];
} SM_ENABLE_LICENCE_PARMS;

typedef union {
    tSM_INT                        	card_id;
    tSM_INT                        	module_id;
    tSM_INT                        	channel_ix;
    tSMChannelId                  	channel_id;
    SM_DRIVER_INFO_PARMS			driver_info_parms;  
    SM_CARD_INFO_PARMS				card_info_parms;
	SM_CARD_REV_PARMS				card_rev_parms;  
    SM_DOWNLOAD_BUFFER_PARMS		download_buffer_parms;  
    SM_DOWNLOAD_COMPLETE_PARMS		download_complete_parms;  
	SM_FWCAPS_BUFFER_PARMS			fwcaps_parms;
	SM_CONFIG_MODULE_SW_PARMS		config_module_sw_parms;
    SM_MODULE_INFO_PARMS			module_info_parms;  
	SM_CHANNEL_ALLOC_PARMS			channel_alloc_parms;
	SM_CHANNEL_ALLOC_PLACED_PARMS	channel_alloc_placed_parms;
	SM_SWITCH_CHANNEL_PARMS			switch_channel_parms;
	SM_SWITCH_CHANNEL_IX_PARMS		switch_channel_ix_parms;
	SM_FIND_CHANNEL_PARMS			find_channel_parms;
	SM_CHANNEL_SET_EVENT_PARMS		channel_set_event_parms;
	SM_CHANNEL_INFO_PARMS			channel_info_parms;
	SM_CHANNEL_IX_INFO_PARMS		channel_ix_info_parms;
	SM_CHANNEL_IX_DUMP_PARMS		channel_ix_dump_parms;
	SM_RECOGNISED_PARMS				recognised_parms;
	SM_RECOGNISED_IX_PARMS			recognised_ix_parms;
	SM_ENABLE_LICENCE_PARMS			enable_licence_parms;
	tSM_INT							trace_level;
	void*							usr_space_parms;
	char							ioctl_space_parms[kMaxIOCTLSpaceParmsSize];
} SMIOCTLU;


#ifdef __NT__
	#define kSMDNTDevControlName 			"MVIP$SMC"
	#define kSMDNTDevControlWideName 		L"MVIP$SMC"
	#define kSMDNTDevBasisName 	 			"MVIP$SM"
	#define kSMDNTDevBasisWideName 	 		L"MVIP$SM"
	#define kSMDNTCtlDataEvBasisName		"MVIP$SMCSignalDEvt"
	#define kSMDNTCtlDataEvBasisWideName	L"MVIP$SMCSignalDEvt"
	#define kSMDNTCtlWrDataEvBasisName		"MVIP$SMCSigWrDtEvt"
	#define kSMDNTCtlWrDataEvBasisWideName	L"MVIP$SMCSigWrDtEvt"
	#define kSMDNTCtlRdDataEvBasisName		"MVIP$SMCSigRdDtEvt"
	#define kSMDNTCtlRdDataEvBasisWideName	L"MVIP$SMCSigRdDtEvt"
	#define kSMDNTCtlRecogEvBasisName		"MVIP$SMCSignalREvt"
	#define kSMDNTCtlRecogEvBasisWideName	L"MVIP$SMCSignalREvt"
	#define kSMDNTDataEvBasisName			"MVIP$SMSignalDEvt"
	#define kSMDNTDataEvBasisWideName		L"MVIP$SMSignalDEvt"
	#define kSMDNTWrDataEvBasisName			"MVIP$SMSigWrDtEvt"
	#define kSMDNTWrDataEvBasisWideName		L"MVIP$SMSigWrDtEvt"
	#define kSMDNTRdDataEvBasisName			"MVIP$SMSigRdDtEvt"
	#define kSMDNTRdDataEvBasisWideName		L"MVIP$SMSigRdDtEvt"
	#define kSMDNTRecogEvBasisName			"MVIP$SMSignalREvt"
	#define kSMDNTRecogEvBasisWideName		L"MVIP$SMSignalREvt"

	typedef struct skntioctl {
	   tSM_INT      command;
	   tSM_INT      error;
	   tSM_INT      apiLibVersion;
	   tSM_INT      fwLibVersion;
	   tSM_INT		module;
	   SMIOCTLU  	ioctlu;
	} SKNTIOCTL, *PSKNTIOCTL;

	#define GPD_TYPE 40000

	#define SM_IOCTL \
		CTL_CODE ( GPD_TYPE, 0x904, METHOD_BUFFERED, FILE_ANY_ACCESS )
#endif

#ifdef UNIX_SYSTEM
    #define kSMDrvrCtlCmdIOCTL			0
    #define kSMDrvrCtlCmdREAD			1
    #define kSMDrvrCtlCmdWRITE			2
    #define kSMDrvrCtlCmdBindToCCB		3
	#define kSMDrvrCtlCmdGetBindId		4
	#define kSMDrvrCtlCmdBindToEvent	5
	#define kSMDrvrCtlCmdAbortEventWait	6
	#define kSMDrvrCtlCmdIOCTLEventWait	7
	#define kSMDrvrCtlCmdEventWaitStat	8
	#define kSMDrvrCtlCmdIOCTLEnterKDB	9

	typedef struct smmsgblk {
	   tSM_INT		ctlcmd;
	   tSM_INT		length;
	   tSM_INT      command;
	   tSM_INT      error;
	   tSM_INT      apiLibVersion;
	   tSM_INT      fwLibVersion;
	   tSM_INT		module;
	   tSM_INT		channel;
	   SMIOCTLU*  	ioctlup;
	} SMMSGBLK;
#endif

#ifdef __OS2__
   #define kSMDOS2DevControlName            "MVIP$SMC"
   #define kSMDOS2DevBasisName              "MVIP$SM"
   #define kSMDOS2CtlDataEvBasisName        "\\SEM32\\MVIP$SMCSignalDEvt"
   #define kSMDOS2CtlRecogEvBasisName       "\\SEM32\\MVIP$SMCSignalREvt"
   #define kSMDOS2DataEvBasisName           "\\SEM32\\MVIP$SMSignalDEvt"
   #define kSMDOS2RecogEvBasisName          "\\SEM32\\MVIP$SMSignalREvt"

   typedef struct skos2ioctl {
      tSM_INT      command;
      tSM_INT      error;
      tSM_INT      apiLibVersion;
      tSM_INT      fwLibVersion;
      tSM_INT      module;
      SMIOCTLU     ioctlu;
   } SKOS2IOCTL, *PSKOS2IOCTL;

   #define SM_IOCAT 0x91
   #define SM_IOCTL 0x01
   #define SM_IOCTL_OPEN 0x02
#endif

#ifdef __cplusplus
extern "C" {
#endif

ACUDLL int sm_get_driver_info(
	struct sm_driver_info_parms* drvinfop 
);

ACUDLL int sm_get_cards(
	void 
);

ACUDLL int sm_get_card_info(
	struct sm_card_info_parms* cardinfop 
);

ACUDLL int sm_get_card_rev(
	struct sm_card_rev_parms* cardrevp 
);

ACUDLL int sm_get_modules(
	void 
);

ACUDLL int sm_reset_module( 
	int module 
);
 
ACUDLL int sm_download_fmw( 
	struct sm_download_parms* downloadp 
);

ACUDLL int sm_get_firmware_caps( 
	struct sm_fwcaps_parms* fwcapsp 
);

ACUDLL int sm_config_module_switching(
	struct sm_config_module_sw_parms* configmoduleswp 
);

ACUDLL int sm_get_module_info(
	struct sm_module_info_parms* moduleinfop 
);

ACUDLL int sm_get_ev_mech( 
	void
);

ACUDLL int sm_channel_alloc( 
	struct sm_channel_alloc_parms*
);

ACUDLL int sm_channel_alloc_placed( 
	struct sm_channel_alloc_placed_parms*
);

ACUDLL int sm_switch_channel_output( 
	struct sm_switch_channel_parms*
);

ACUDLL int sm_switch_channel_input( 
	struct sm_switch_channel_parms*
);

ACUDLL int sm_switch_channel_ix( 
	struct sm_switch_channel_ix_parms*
);

ACUDLL int sm_find_channel( 
	struct sm_find_channel_parms*
);

ACUDLL int sm_channel_set_event( 
	struct sm_channel_set_event_parms*
);

ACUDLL int sm_channel_release( 
	tSMChannelId
);

ACUDLL int sm_channel_validate_id( 
	tSMChannelId
);

ACUDLL int sm_get_channel_ix( 
	tSMChannelId
);

ACUDLL int sm_get_channel_type( 
	tSMChannelId
);

ACUDLL int sm_get_channel_module_ix( 
	tSMChannelId
);

ACUDLL int sm_get_channel_ix_module_ix( 
	tSM_INT
);

ACUDLL int sm_get_module_card_ix( 
	tSM_INT
);

ACUDLL int sm_get_card_switch_ix( 
	tSM_INT
);

ACUDLL int sm_channel_info( 
	struct sm_channel_info_parms*
);

ACUDLL int sm_channel_ix_info( 
	struct sm_channel_ix_info_parms*
);

ACUDLL int sm_channel_ix_dump( 
	struct sm_channel_ix_dump_parms*
);

ACUDLL int sm_reset_channel( 
	tSMChannelId
);

ACUDLL int sm_enable_licence(
	struct sm_enable_licence_parms*
);

#ifdef SM_CRACK_API

void sm_crack_result( int, char* );
int sm_crack_generic_ioctl( int, char*  );

#endif

#ifdef __cplusplus
}
#endif

#pragma pack ( )

#endif
#endif


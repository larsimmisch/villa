/*------------------------------------------------------------*/
/* Copyright ACULAB plc. (c) 1996-2000                        */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : mvswdrvr.h                             */
/*                                                            */
/*           Purpose : Switch driver header file              */
/*                                                            */
/*            Author : Peter Dain                             */
/*                                                            */
/*       Create Date : 26th April 1995                        */
/*                                                            */
/*                                                            */
/*                                                            */
/*                                                            */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Change History                                             */
/*                                                            */
/* cur:  3.30   22/03/99    pgd   Align with 3.1.4 Driver     */
/* rev:  1.00   15/09/92    agr   File created                */
/* rev:  1.01   16/03/93    agr   Tristate switch added       */
/* rev:  1.02   05/10/95    pgd   Changes for SCbus           */
/* rev:  2.10   14/02/96    pgd   First SCbus switch release  */
/* rev:  2.20   18/06/96    pgd   BR net streams>=32 release  */
/* rev:  2.30   17/10/96    pgd   Migrate to V4 generic etc.  */
/* rev:  3.00   20/02/98    pgd   Integrate mvswerr, ACU_INT. */
/* rev:  3.01   31/03/98    pgd   Integrate mvswerr, ACU_INT. */
/* rev:  3.03   16/06/98    pgd   Eliminate __NEWC__ etc      */
/* rev:  3.04   19/08/98    mcb   Added Support for NT DLL    */
/* rev:  3.10   26/11/98    pgd   H.100 Enhancements          */
/* rev:  3.20   07/01/99    pgd   MC3 Enhancements            */
/* rev:  3.30   22/03/99    pgd   Align with 3.1.4 Driver     */
/*                                                            */
/*------------------------------------------------------------*/


#ifndef _MVSWDRVR_
#define _MVSWDRVR_

#ifndef ACUDLL
#define ACUDLL
#endif


/* For Visual Basic Applications define ACU_VB_LIBRARY in the */
/* 'Preprocessor definitions'. This allows the documented API functions to be   */
/* exported correctly when building a DLL*/
#ifndef	ACU_WINAPI
#ifdef	ACU_VB_LIBRARY
#define ACU_WINAPI WINAPI
#else
#define ACU_WINAPI
#endif
#endif


#ifdef WIN32
/*
 * Get definition for HANDLE.
 */
#ifndef COMPILE_DRIVER
#ifndef _WINDOWS_
/*
 * Get definition for HANDLE.
 */
#include <windows.h>
#endif
#endif

#ifndef __NT__
#define __NT__
#endif

#else
	#ifdef unix
		#ifndef UNIX_SYSTEM
			#define UNIX_SYSTEM
		#endif
	#endif
#endif

#ifdef ACU_DIGITAL
#include <sys/ioctl.h>
#endif

#ifndef ACU_SOLARIS_SPARC
#pragma pack ( 1 )
#endif


/*
 * Following ACU_INT definition may need to be changed according to compiler used.
 *
 * Use 16 bit value (integer or short) for DSO and OS/2.
 * Use 32 bit value (integer or long)  for NT and UNIX.
 */ 
#ifdef _ACU_OS2_32BIT_
 #define ACU_OS2_32
#endif

#ifndef COMPILE_DRIVER
#ifdef ACU_OS2_32
#define _ACU_INT_
typedef short ACU_INT;
#endif
#ifndef _ACU_INT_
#define _ACU_INT_
typedef int ACU_INT;
#endif
#ifndef _ACU_ULONG_
#define _ACU_ULONG_
#ifndef ACU_SOLARIS_SPARC
typedef unsigned long ACU_ULONG;
#else
typedef unsigned int ACU_ULONG;
#endif
#endif
#endif

#ifdef WIN32
	typedef HANDLE 	tSWEventId;
#define kSWNTEvBasisName		"MVIP$SWSignalEvt"
#define kSWNTEvBasisWideName	L"MVIP$SWSignalEvt"
#endif

#ifdef UNIX_SYSTEM

#ifdef SW_POLL_UNIX
	typedef struct tSWEventId { ACU_INT fd; ACU_INT mode; } tSWEventId;
#else

#ifdef SW_SEL_UNIX
	typedef struct tSWEventId { ACU_INT fd; ACU_INT mode; } tSWEventId;
#else
	typedef int tSWEventId;
#endif

#endif

#endif

#ifdef __OS2__
   typedef unsigned long  tSWEventId; 
#endif


/*------------- IOCTL Functions ---------*/
/*---------------------------------------*/
/* MVIP Switching Functions */

/*
 * Avoid polluting pre-processor name space unless compiling lib or driver.
 */
#ifdef SW_IOCTL_CODES

#ifdef ACU_DIGITAL

#define RESET_SWITCH                    _IOWR('m', 0x00, SWMSGBLK)
#define QUERY_SWITCH_CAPS               _IOWR('m', 0x01, SWMSGBLK)
#define REINIT_SWITCH                   _IOWR('m', 0x02, SWMSGBLK)
#define SWCARD_INFO 	                _IOWR('m', 0x03, SWMSGBLK)

#define H100_CONFIG_BOARD_CLOCK         _IOWR('m', 0x08, SWMSGBLK)
#define H100_CONFIG_NETREF_CLOCK        _IOWR('m', 0x09, SWMSGBLK)
#define H100_QUERY_BOARD_CLOCK          _IOWR('m', 0x0A, SWMSGBLK)
#define H100_QUERY_NETREF_CLOCK         _IOWR('m', 0x0B, SWMSGBLK)

#define SWMODE_SWITCH                   _IOWR('m', 0x0F, SWMSGBLK)

#define SET_OUTPUT                      _IOWR('m', 0x10, SWMSGBLK)
#define QUERY_OUTPUT                    _IOWR('m', 0x11, SWMSGBLK)
#define SAMPLE_INPUT                    _IOWR('m', 0x12, SWMSGBLK)
#define SET_VERIFY                      _IOWR('m', 0x13, SWMSGBLK)
#define CONFIG_CLOCK                    _IOWR('m', 0x14, SWMSGBLK)
#define SAMPLE_INPUT0                   _IOWR('m', 0x15, SWMSGBLK)
#define QUERY_SCM                       _IOWR('m', 0x17, SWMSGBLK)
#define QUERY_CLOCK_MODE                _IOWR('m', 0x18, SWMSGBLK)
#define REGISTER_EVENT                  _IOWR('m', 0x19, SWMSGBLK)

#define SWVER_SWITCH                    _IOWR('m', 0x1A, SWMSGBLK)
#define TRACK_SWITCH                    _IOWR('m', 0x1B, SWMSGBLK)
#define DUMP_SWITCH                     _IOWR('m', 0x1C, SWMSGBLK)
#define SET_TRACE                       _IOWR('m', 0x1D, SWMSGBLK)
#define TRISTATE_SWITCH                 _IOWR('m', 0x1E, SWMSGBLK)
#define SWITCH_OVERRIDE_MODE            _IOWR('m', 0x1F, SWMSGBLK)

#define DIAGTRACE                       _IOWR('m', 0x99, SWMSGBLK)

#define MC3_BERT_STATUS                 _IOWR('m', 0x20, SWMSGBLK)
#define MC3_CONFIG_BERT                 _IOWR('m', 0x21, SWMSGBLK)
#define MC3_BYPASS_FIBRE                _IOWR('m', 0x22, SWMSGBLK)
#define MC3_RING_STATUS                 _IOWR('m', 0x23, SWMSGBLK)
#define MC3_EVENT_CRITERIA              _IOWR('m', 0x24, SWMSGBLK)
#define MC3_RESYNC_RING                 _IOWR('m', 0x25, SWMSGBLK)
#define MC3_RING_LATCHED_STATUS         _IOWR('m', 0x26, SWMSGBLK)

#else

#define RESET_SWITCH       			0x00   /* reset the switch block */
#define QUERY_SWITCH_CAPS  			0x01   /* query switch capabilities */
#define REINIT_SWITCH 	   			0x02   /* Re-initialise switch. */
#define SWCARD_INFO 	            0x03

#define H100_CONFIG_BOARD_CLOCK				0x08
#define H100_CONFIG_NETREF_CLOCK			0x09
#define H100_QUERY_BOARD_CLOCK				0x0A
#define H100_QUERY_NETREF_CLOCK				0x0B

#define SWMODE_SWITCH		0x0F

#define SET_OUTPUT         0x10   /* makes/breaks a switch connection */
#define QUERY_OUTPUT       0x11   /* returns the state of the switch output */
#define SAMPLE_INPUT       0x12   /* returns data of a switch input */
#define SET_VERIFY         0x13   /* enables make/break verification */
#define CONFIG_CLOCK 	   0x14   /* configure the clock output */
#define SAMPLE_INPUT0      0x15   /* returns data of a switch input - zero delay */
#define QUERY_SCM	  	   0x17   /* determine who is driving SC bus clock */
#define QUERY_CLOCK_MODE   0x18   /* determine card clock mode */
#define REGISTER_EVENT     0x19	  /* set up switch driver event. */

/* MVIP Diagnostic Functions */
#define SWVER_SWITCH     		0x1a     /* Driver version */
#define TRACK_SWITCH     		0x1b     /* Track API calls */
#define DUMP_SWITCH      		0x1c     /* Returns the contents of a specific switch */
#define SET_TRACE       		0x1d     /* Enables Printing ofg diagnostic info */
#define TRISTATE_SWITCH  		0x1e     /* Globally enables/disables a switch block */
#define SWITCH_OVERRIDE_MODE 	0x1f     /* control MUX override mode. */

/* Not used but to distinguish diagnostic trace from other API calls in tracking */
#define DIAGTRACE				0x99

#define	MC3_BERT_STATUS 		0x20
#define MC3_CONFIG_BERT			0x21
#define MC3_BYPASS_FIBRE		0x22
#define MC3_RING_STATUS			0x23
#define MC3_EVENT_CRITERIA		0x24
#define MC3_RESYNC_RING			0x25
#define MC3_RING_LATCHED_STATUS 0x26

#endif /* ACU_DIGITAL */

#endif

/*
 * Some stream no. definitions.
 */
#define kSW_LI_OUT_TO_DSI	0
#define kSW_LI_OUT_TO_DSO	8
#define kSW_LI_IN_FROM_DSO	0
#define kSW_LI_IN_FROM_DSI	8
#define kSW_LI_SCBUS 		24
#define kSW_LI_E1_PEB1		20
#define kSW_LI_E1_PEB2		22
#define kSW_LI_E1_NET1		16
#define kSW_LI_E1_NET2		18
#define kSW_LI_BR_PEB1		19
#define kSW_LI_BR_NET1		32


/*
 * PCI Prosody (P1) stream numering.
 */ 
#define kSW_LI_P1_NET1		32
#define kSW_LI_P1_NET2		33
#define kSW_LI_P1_NET3		34
#define kSW_LI_P1_NET4		35

#define kSW_LI_P1_DSP1		40
#define kSW_LI_P1_DSP2		41
#define kSW_LI_P1_DSP3		42
#define kSW_LI_P1_DSP4		43

/*
 * Flagship card streams for expansion module.
 */
#define kSW_LI_P1_DSP5		44
#define kSW_LI_P1_DSP6		45
#define kSW_LI_P1_DSP7		46
#define kSW_LI_P1_DSP8		47


#define kSW_LI_P1_SHARC1_0		48
#define kSW_LI_P1_SHARC1_1		49
#define kSW_LI_P1_SHARC2_0		50
#define kSW_LI_P1_SHARC2_1		51
#define kSW_LI_P1_SHARC3_0		52
#define kSW_LI_P1_SHARC3_1		53
#define kSW_LI_P1_SHARC4_0		54
#define kSW_LI_P1_SHARC4_1		55


/*
 * cPCI Prosody (C1) stream numering.
 */ 
#define kSW_LI_C1_NET1		32
#define kSW_LI_C1_NET2		33
#define kSW_LI_C1_NET3		34
#define kSW_LI_C1_NET4		35

#define kSW_LI_C1_DSP1		40
#define kSW_LI_C1_DSP2		41
#define kSW_LI_C1_DSP3		42
#define kSW_LI_C1_DSP4		43


#define kSW_LI_C1_SHARC1_0		48
#define kSW_LI_C1_SHARC1_1		49
#define kSW_LI_C1_SHARC2_0		50
#define kSW_LI_C1_SHARC2_1		51
#define kSW_LI_C1_SHARC3_0		52
#define kSW_LI_C1_SHARC3_1		53
#define kSW_LI_C1_SHARC4_0		54
#define kSW_LI_C1_SHARC4_1		55


/*
 * E1/T1 cPCI (C2) stream numering.
 */ 
#define kSW_LI_C2_NET1		32
#define kSW_LI_C2_NET2		33
#define kSW_LI_C2_NET3		34
#define kSW_LI_C2_NET4		35
#define kSW_LI_C2_NET5		36
#define kSW_LI_C2_NET6		37
#define kSW_LI_C2_NET7		38
#define kSW_LI_C2_NET8		39

/* Per port CAS DSP streams */
#define kSW_LI_C2_DSP1_A	40
#define kSW_LI_C2_DSP2_A	41
#define kSW_LI_C2_DSP3_A	42
#define kSW_LI_C2_DSP4_A	43
#define kSW_LI_C2_DSP5_A	44
#define kSW_LI_C2_DSP6_A	45
#define kSW_LI_C2_DSP7_A	46
#define kSW_LI_C2_DSP8_A	47

/* Per port other func (eg. A-mu) streams */
#define kSW_LI_C2_DSP1_B	56
#define kSW_LI_C2_DSP2_B	57
#define kSW_LI_C2_DSP3_B	58
#define kSW_LI_C2_DSP4_B	59
#define kSW_LI_C2_DSP5_B	60
#define kSW_LI_C2_DSP6_B	61
#define kSW_LI_C2_DSP7_B	62
#define kSW_LI_C2_DSP8_B	63

/* Aculab internal production test use only */
#define kSW_LI_C2_TEST_LINK_1	64
#define kSW_LI_C2_TEST_LINK_8	71

/*
 * T1 PCI (T1) stream numering.
 */ 
#define kSW_LI_T1_NET1		32
#define kSW_LI_T1_NET2		33

#define kSW_LI_T1_HDLC1		46
#define kSW_LI_T1_HDLC2		47

#define kSW_LI_T1_SHARC1_0		48
#define kSW_LI_T1_SHARC1_1		49


/*
 * ISA (F1) MC3 stream numering.
 */ 
#define kSW_MC3_F1_MVIPA_BASE			0
#define kSW_MC3_F1_MVIPA_OUT_TO_DSI		0
#define kSW_MC3_F1_MVIPA_OUT_TO_DSO		8
#define kSW_MC3_F1_MVIPA_IN_FROM_DSO	0
#define kSW_MC3_F1_MVIPA_IN_FROM_DSI	8
#define kSW_MC3_F1_MVIPB_BASE			16
#define kSW_MC3_F1_MVIPB_OUT_TO_DSI		16
#define kSW_MC3_F1_MVIPB_OUT_TO_DSO		24
#define kSW_MC3_F1_MVIPB_IN_FROM_DSO	16
#define kSW_MC3_F1_MVIPB_IN_FROM_DSI	24
#define kSW_MC3_F1_RING1				32
#define kSW_MC3_F1_RING2				33
#define kSW_MC3_F1_BERT					48
#define kSW_MC3_F1_TIMESLOTS_IN_RING	2423


/*
 * VoIP PCI H.323 Gateway card (V1) stream numbering.
 */ 
#define kSW_LI_V1_NET1		32
#define kSW_LI_V1_NET2		33
#define kSW_LI_V1_NET3		34
#define kSW_LI_V1_NET4		35

#define kSW_LI_V1_DSP1		40
#define kSW_LI_V1_DSP2		41
#define kSW_LI_V1_DSP3		42
#define kSW_LI_V1_DSP4		43

#define kSW_VOIP_V1_MODULE_0 48
#define kSW_VOIP_V1_MODULE_1 49
#define kSW_VOIP_V1_MODULE_2 50
#define kSW_VOIP_V1_MODULE_3 51
#define kSW_VOIP_V1_MODULE_4 52
#define kSW_VOIP_V1_MODULE_5 53
#define kSW_VOIP_V1_MODULE_6 54
#define kSW_VOIP_V1_MODULE_7 55


#define ERR_SW_INVALID_COMMAND     (-200)  		/* Command code is not supported       	*/
#define ERR_SW_DEVICE_ERROR        (-202)  		/* An error was returned from a device 	*/
                                       	 		/* driver called by this driver.       	*/

#define ERR_SW_NO_RESOURCES        (-204)  		/* An internal driver resource has     	*/
                                       	 		/* been exhausted.                     	*/

#define ERR_SW_INVALID_SWITCH      (-209)  		/* Out of range switch driver index  	*/

#define ERR_SW_INVALID_STREAM      (-210)  		/* Stream number in parameter list is  	*/
                                       	 		/* out of range.                       	*/
#define ERR_SW_INVALID_TIMESLOT    (-211)  		/* Time slot in parameter list is out  	*/
                                       	 		/* of range.                           	*/
#define ERR_SW_INVALID_CLOCK_PARM  (-213)  		/* Invalid clock configuration         	*/
                                       	 		/* parameter                           	*/
#define ERR_SW_INVALID_MODE        (-216)  		/* Incorrect SET_OUTPUT or             	*/
                                       	 		/* QUERY_OUTPUT mode.                  	*/
#define ERR_SW_INVALID_MINOR_SWITCH (-217) 		/* Minor (internal) switch error.      	*/
#define ERR_SW_INVALID_PARAMETER    (-218)	 	/* General invalid parameter error.    	*/
#define ERR_SW_NO_PATH             	(-220)  	/* Connection cannot be made due to switch limitation */
#define ERR_SW_NO_SCBUS_CLOCK      	(-224)  	/* No card driving SCbus clock. 		*/
#define ERR_SW_OTHER_SCBUS_CLOCK   	(-225)  	/* Another non-MVSWDEV controlled card is driving scbus clock */
#define ERR_SW_PATH_BLOCKED		 	(-226)	 	/* eg. because of MVIP output MUX 		*/
#define ERR_SW_OS_INTERRUPTED		(-227)	 	/* eg. Unix event wait interrupted through signal */



/*
 * For compatibility with old error names.
 */
#define MVIP_INVALID_COMMAND		ERR_SW_INVALID_COMMAND
#define MVIP_DEVICE_ERROR 			ERR_SW_DEVICE_ERROR
#define MVIP_NO_RESOURCES 			ERR_SW_NO_RESOURCES
#define MVIP_INVALID_STREAM 		ERR_SW_INVALID_STREAM
#define MVIP_INVALID_TIMESLOT 		ERR_SW_INVALID_TIMESLOT
#define MVIP_INVALID_CLOCK_PARM 	ERR_SW_INVALID_CLOCK_PARM
#define MVIP_INVALID_MODE 			ERR_SW_INVALID_MODE
#define MVIP_INVALID_MINOR_SWITCH 	ERR_SW_INVALID_MINOR_SWITCH
#define MVIP_INVALID_PARAMETER 		ERR_SW_INVALID_PARAMETER
#define MVIP_NO_PATH 				ERR_SW_NO_PATH
#define MVIP_NO_SCBUS_CLOCK 		ERR_SW_NO_SCBUS_CLOCK
#define MVIP_OTHER_SCBUS_CLOCK 		ERR_SW_OTHER_SCBUS_CLOCK
#define MVIP_PATH_BLOCKED 			ERR_SW_PATH_BLOCKED

#ifdef MVSW_EXTENDED_NSWITCH
#define NSWITCH  16        /* maximum switch drivers supported */
#else
#define NSWITCH  10        /* maximum switch drivers supported */
#endif

#define TPS_FOR_DOS   18       /* ticks per second for dos  */
#define TPS_FOR_OS2   32       /* ticks per second for OS/2 */
#define TPS_FOR_UNIX 100       /* ticks per second for Unix */
#define TPS_FOR_NT    20       /* ticks per second for NT   */


#define DISABLE_MODE     		0
#define PATTERN_MODE     		1
#define CONNECT_MODE     		2
#define FRAMED_CONNECT_MODE		3


#define CLOCK_REF_MVIP      0x0000
#define CLOCK_REF_SEC8K     0x0001
#define CLOCK_REF_LOCAL     0x0002
#define CLOCK_REF_NET1      0x0003
#define CLOCK_REF_NET2      0x0004
#define CLOCK_REF_SCBUS     0x0006
#define CLOCK_REF_H100      0x0007
#define CLOCK_REF_RING1     0x0008
#define CLOCK_REF_RING2     0x0009

#define CLOCK_REF_NET3     	0x000C
#define CLOCK_REF_NET4     	0x000D

#define CLOCK_REF_NET5     	0x0005
#define CLOCK_REF_NET6     	0x000A
#define CLOCK_REF_NET7     	0x000B
#define CLOCK_REF_NET8     	0x000E


#define CLOCK_PRIVATE       0x0010
#define MC3_MVIP_B			0x0020
#define DRIVE_SCBUS         0x0040
#define HI_MVIPCLK          0x0080

#define CLOCK_NO_MVIP90_MAPPING 0x000F

#define SEC8K_NOT_DRIVEN      				0x0000
#define SEC8K_DRIVEN_BY_LOCAL				0x0200
#define SEC8K_DRIVEN_BY_NET1  				0x0300
#define SEC8K_DRIVEN_BY_NET2  				0x0400
#define SEC8K_DRIVEN_BY_NET3  				0x0500
#define SEC8K_DRIVEN_BY_NET4  				0x0600
#define SEC8K_DRIVEN_BY_OTHER_BUS_SEC8K  	0x0700
#define SEC8K_DRIVEN_BY_RING1  				0x0800
#define SEC8K_DRIVEN_BY_RING2  				0x0900
#define SEC8K_DRIVEN_BY_NET5  				0x0A00
#define SEC8K_DRIVEN_BY_NET6  				0x0B00
#define SEC8K_DRIVEN_BY_NET7  				0x0C00
#define SEC8K_DRIVEN_BY_NET8  				0x0D00

#define IDLE   0x54           /* ccitt Idle Pattern */

typedef struct swver_parms {
	ACU_INT		major;
	ACU_INT		minor;
	ACU_INT		step;	
	ACU_INT		custom;	
	ACU_INT		quality;	
	ACU_INT		buildno0;	
	ACU_INT		buildno1;	
} SWVER_PARMS;

#define SW_ISADAC_REV4_CARD				1
#define SW_ISADAC_REV5_CARD				2
#define SW_ISADAC_BR4_CARD				3
#define SW_ISADAC_BR8_CARD   			4
#define SW_ISAMC3_CARD					5
#define SW_PROSODY_PCI_CARD				0x10
#define SW_T1_PCI_HALF_CARD				0x11
#define SW_E1_T1_PCI_TRUNK_CARD			0x12
#define SW_E1_T1_PCI_CARD				0x12 /* Alternative name */
#define SW_PCIMC3_CARD					0x13
#define SW_VOIP_PCI_H323_GATEWAY_CARD	0x14
#define SW_PROSODY_CPCI_CARD			0x20
#define SW_E1_T1_CPCI_CARD				0x21
#define SW_VOIP_CPCI_H323_GATEWAY_CARD	0x22

#define kSWMaxSerialNoText 12

typedef struct swcard_info_parms {
	ACU_INT		card_type;
	ACU_INT		card_present;	
	ACU_INT		max_capacity;	
	ACU_INT		additional_data[4];	
	ACU_ULONG	physical_address;	
	ACU_ULONG	io_address_or_pcidev;
	ACU_ULONG	physical_irq;
	char		serial_no[kSWMaxSerialNoText];
} SWCARD_INFO_PARMS;


#define SWMODE_CTBUS_MVIP	0
#define SWMODE_CTBUS_SCBUS	1
#define SWMODE_CTBUS_H100	2
#define SWMODE_CTBUS_PEB	3
#define SWMODE_CTBUS_MC3	4

typedef struct swmode_parms {
	ACU_INT		ct_buses;	/* Bit mask of enabled CT-buses */
} SWMODE_PARMS;

typedef struct capabilities_parms 
{
	ACU_INT   size;
	ACU_INT   revision;
	ACU_INT   domain;
	ACU_INT   routing;
	ACU_INT   blocking;
	ACU_INT   networks;
	ACU_INT   channels[8];
} CAP_PARMS;


typedef struct output_parms 
{
	ACU_INT ost;
	ACU_INT ots;
	ACU_INT mode;
	ACU_INT ist;
	ACU_INT its;
	ACU_INT pattern;
} OUTPUT_PARMS;


typedef struct sample_parms 
{
	ACU_INT  ist;
	ACU_INT  its;
	char sample;
} SAMPLE_PARMS;

/*
 * Aculab use only.
 *
 * On T810X type boards, normally
 *  Stream is AMR reg
 *  Slot   is LAR reg
 *  data is read value
 * unless stream is set to 1 of following values where upon 
 *  slot is CAM location
 *  data = its + (ist<<8)
 *  cmhi = controlBits + ((valid) ? (1<<8) : 0)
 *  cmlo = tagOrPattern
 */
#define kDumpSwitchStreamForLocalCAM		256		
#define kDumpSwitchStreamForEvenCAM			257
#define kDumpSwitchStreamForOddCAM			258
#define kDumpSwitchStreamForTagTableDump	259

typedef struct dump_parms 
{
	ACU_INT size;
	ACU_INT minor;
	ACU_INT stream;
	ACU_INT slot;
	ACU_INT cmhi;
	ACU_INT cmlo;
	ACU_INT data;
} DUMP_PARMS;


typedef struct qclock_parms 
{
	ACU_INT scclk_driven;
	ACU_INT driving_scclk;
} QCLOCK_PARMS;

typedef struct query_clkmode_parms 
{
	ACU_INT last_clock_mode;
	ACU_INT sysinit_clock_mode;
} QUERY_CLKMODE_PARMS;

typedef struct trace_parms 
{
	ACU_INT code;
} TRACE_PARMS;

#define kSWTrackTypeAppAPICall		1

#define kSWDiagTrackCmdTrackingOff				0
#define kSWDiagTrackCmdTrackAPI					2
#define kSWDiagTrackCmdTrackAPIWithTimestamp	3
#define kSWDiagTrackCmdTrackAPIAndDiag			4
#define kSWDiagTrackCmdResetTimestamp			16

typedef struct track_parms 
{
	ACU_INT				tracking_on;
	ACU_INT				is_tracked_data;
	ACU_INT				swdrvr;
	ACU_INT				tracked_type;
	ACU_INT				index;
	ACU_INT				words[8];
	ACU_ULONG			timestamp;
} TRACK_PARMS;


/*
 * Backwards compatibility.
 */
typedef struct track_313_parms 
{
	ACU_INT				tracking_on;
	ACU_INT				is_tracked_data;
	ACU_INT				swdrvr;
	ACU_INT				tracked_type;
	ACU_INT				index;
	ACU_INT				words[8];
	ACU_ULONG			timestamp;
} TRACK_313_PARMS;


#define H100_SOURCE_INTERNAL	1
#define H100_SOURCE_NETWORK		3
#define H100_SOURCE_H100_A		8
#define H100_SOURCE_H100_B		9
#define H100_SOURCE_NETREF		0x0A
#define H100_SOURCE_NETREF_1	0x0A
#define H100_SOURCE_NETREF_2	0x0B
#define H100_SOURCE_RING_1      0x11
#define H100_SOURCE_RING_2      0x12

#define H100_SLAVE				0
#define H100_MASTER_A			1
#define H100_MASTER_B			2

#define H100_FALLBACK_DISABLED		0
#define H100_FALLBACK_ENABLED		1
#define H100_AUTO_RETURN			0x10
#define H100_CHANGEOVER_TO_NETWORK	0x20
#define H100_CHANGEOVER_TO_NETREF	0x40
#define H100_CHANGEOVER_TO_NETREF_2	0x80
#define H100_CHANGEOVER_TO_RING_1         0x100
#define H100_CHANGEOVER_TO_RING_2         0x200
#define H100_CHANGEOVER_TO_ALTERNATE_RING 0x400

#define H100_DISABLE_NETREF		0
#define H100_ENABLE_NETREF		1
#define H100_ENABLE_NETREF_2	2

#define H100_NETREF_8KHZ		0
#define H100_NETREF_1544MHZ		1
#define H100_NETREF_2048MHZ		2


typedef struct h100_config_board_clock_parms
{
	ACU_INT clock_source; 	 		/* internal, net, H100_A, H100_B, H100_NETREF */ 
	ACU_INT network;		 		/* 1..4 - MVIP-95 uses base of 1 not 0 */
	ACU_INT h100_clock_mode;	 	/* slave, A-master, B-Master */
	ACU_INT auto_fall_back;	 		/* enable/disable */
	ACU_INT netref_clock_speed; 	/* 8KHz, 1544MHz, 2048MHz */	
} H100_CONFIG_BOARD_CLOCK_PARMS;

typedef struct h100_netref_clock_parms
{
	ACU_INT network;		 			/* 1..4 - MVIP-95 uses base of 1 not 0 */
	ACU_INT netref_clock_mode;	 		/* none, generate_netref  */
	ACU_INT netref_clock_speed; 		/* 8KHz, 1544MHz, 2048MHz */
} H100_NETREF_CLOCK_PARMS;

#define H100_CLOCK_STATUS_GOOD		0
#define H100_CLOCK_STATUS_BAD		1
#define H100_CLOCK_STATUS_UNKNOWN	2

typedef struct h100_query_board_clock_parms
{
	ACU_INT clock_source;	 			/* internal, network, H100_A, H100_B, NETREF */
	ACU_INT network;					/* 1..4 */
	ACU_INT h100_clock_mode;			/* slave, A-master, B-master */
	ACU_INT auto_fall_back;			/* enabled/disabled */
	ACU_INT fall_back_occurred;		/* yes/no */
	ACU_INT h100_a_clock_status; 		/* good/bad/unknown */
	ACU_INT h100_b_clock_status; 		/* good/bad/unknown */
	ACU_INT netref_a_clock_status; 	/* good/bad/unknown */
	ACU_INT netref_b_clock_status; 	/* good/bad/unknown - for H.110 */
} H100_QUERY_BOARD_CLOCK_PARMS;


#define	MC3_BERT_DISABLE				0
#define	MC3_BERT_ENABLE_ON_SINGLE_TS	1
#define	MC3_BERT_ENABLE_ON_ALL_TS		2

#define	MC3_BERT_PATTERN_ALL_ZEROS		0
#define	MC3_BERT_PATTERN_ALL_ONES		1
#define	MC3_BERT_PATTERN_ALT_10			2
#define	MC3_BERT_PATTERN_ALT_1100		3
#define	MC3_BERT_PATTERN_223_O151		4
#define	MC3_BERT_PATTERN_215_O151		5

#define MC3_BERT_ERROR_INSERT_NONE		0x000
#define MC3_BERT_ERROR_INSERT_1EMINUS_1	0x100
#define MC3_BERT_ERROR_INSERT_1EMINUS_2	0x200
#define MC3_BERT_ERROR_INSERT_1EMINUS_3	0x300
#define MC3_BERT_ERROR_INSERT_1EMINUS_4	0x400
#define MC3_BERT_ERROR_INSERT_1EMINUS_5	0x500
#define MC3_BERT_ERROR_INSERT_1EMINUS_6	0x600
#define MC3_BERT_ERROR_INSERT_1EMINUS_7	0x700

typedef struct mc3_config_bert_parms
{
	ACU_INT mode;
	ACU_INT pattern_type;
	ACU_INT	error_insertion_rate;
} MC3_CONFIG_BERT_PARMS;

#define MC3_BERT_STATUS_RX_ALL_ONES			6
#define MC3_BERT_STATUS_RX_ALL_ZEROS		5
#define MC3_BERT_STATUS_RX_SYNC_LOSS		4
#define MC3_BERT_STATUS_RX_ERRORS			3
#define MC3_BERT_STATUS_RX_CNT_OFLW			2
#define MC3_BERT_STATUS_RX_ERR_CNT_OFLW		1
#define MC3_BERT_STATUS_RX_IS_SYNCED		0

typedef struct mc3_bert_status_parms
{
	ACU_INT		status_bits;
	ACU_ULONG	last_rx_bits;
	ACU_ULONG 	rx_bit_count;
	ACU_ULONG 	rx_bit_error_count;
} MC3_BERT_STATUS_PARMS;

#define MC3_RING_STATUS_LINK_FAULT			0
#define MC3_RING_STATUS_RECEIVE_ALARM		1
#define MC3_RING_STATUS_B1_PARITY_ERROR		2
#define MC3_RING_STATUS_RX_FRAME_ERROR		3
#define MC3_RING_STATUS_OUT_OF_FRAME		4
#define MC3_RING_STATUS_LOSS_OF_FRAME		5
#define MC3_RING_STATUS_LOSS_OF_SIGNAL		6
#define MC3_RING_STATUS_RECEIVE_ALARM_FSA	7
#define MC3_RING_STATUS_RECEIVE_ALARM_TXPAA	8
#define MC3_RING_STATUS_RECEIVE_ALARM_PPCE	9
#define MC3_RING_STATUS_CLOCK_HOLDOVER	   15

typedef struct mc3_ring_status_parms
{
	ACU_INT	ring1_status_bits;
	ACU_INT	ring2_status_bits;
	ACU_INT	is_in_bypass_mode;
} MC3_RING_STATUS_PARMS;

typedef struct register_event_parms {
	ACU_INT		enable_notification;
	tSWEventId	event_id;
} REGISTER_EVENT_PARMS;

typedef struct mc3_event_criteria_parms
{
	ACU_INT	ring_status_bits;
} MC3_EVENT_CRITERIA_PARMS;

#define MC3_RESYNC_RING_BOTH		0
#define MC3_RESYNC_RING_PRIMARY		1
#define MC3_RESYNC_RING_SECONDARY	2


typedef union pioctl 
{
	SWVER_PARMS						swver_parms;
	SWCARD_INFO_PARMS				swcard_info_parms;
	SWMODE_PARMS					swmode_parms;
	ACU_INT          				traceswitch;
    CAP_PARMS    					cap_parms;
	TRACK_PARMS						track_parms;	
    OUTPUT_PARMS 					output_parms;
    SAMPLE_PARMS 					sample_parms;
    DUMP_PARMS   					dump_parms;
	QCLOCK_PARMS					qclock_parms;
    TRACE_PARMS				  		trace_parms;
	QUERY_CLKMODE_PARMS				qclkmode_parms;
    ACU_INT   				       	clock_mode;
    ACU_INT       				   	tristate;
    ACU_INT          				override_mode;
	REGISTER_EVENT_PARMS			register_event_parms;
	H100_CONFIG_BOARD_CLOCK_PARMS	h100_config_board_clock_parms;
	H100_NETREF_CLOCK_PARMS			h100_config_netref_clock_parms;
	H100_QUERY_BOARD_CLOCK_PARMS	h100_query_board_clock_parms;
	H100_NETREF_CLOCK_PARMS			h100_query_netref_clock_parms;
	MC3_CONFIG_BERT_PARMS			mc3_config_bert_parms;
	MC3_BERT_STATUS_PARMS			mc3_bert_status_parms;
    ACU_INT       				   	mc3_bypass_fibre_mode;
	MC3_RING_STATUS_PARMS			mc3_ring_status_parms;
	MC3_EVENT_CRITERIA_PARMS		mc3_event_criteria_parms;
    ACU_INT       				   	mc3_resync_ring_mode;
} SWIOCTLU;


#define kSWDrvrCtlCmdAbortEventWait	1
#define kSWDrvrCtlCmdIOCTLEventWait	2

typedef struct swmsgblk
{
    int  		status;
    SWIOCTLU* 	swioctlup;
} SWMSGBLK;

#define kMVSWDevBasisName 	 		"MVIP$SW"
#define kMVSWDevBasisWideName 	 	L"MVIP$SW"

typedef struct sxntioctl {
   int       command;
   SWIOCTLU  ioctlu;
   int       error;
} SXNTIOCTL, *PSXNTIOCTL;

#define GPD_TYPE 40000

#define SX_IOCTL \
	CTL_CODE ( GPD_TYPE, 0x904, METHOD_BUFFERED, FILE_ANY_ACCESS )

#ifndef COMPILE_DRIVER

#ifdef __cplusplus
extern "C" {
#endif


/*--------- Multiple Driver Functions ---------*/
/*---------------------------------------------*/

ACUDLL int ACU_WINAPI sw_ver_switch( 
	int, struct swver_parms*
);

ACUDLL int sw_card_info( 
	int, struct swcard_info_parms*
);

ACUDLL int ACU_WINAPI sw_reset_switch( 
	int
);

ACUDLL int ACU_WINAPI sw_reinit_switch( 
	int 
);

ACUDLL int ACU_WINAPI sw_query_switch_caps( 
	int, struct capabilities_parms * 
);

ACUDLL int ACU_WINAPI sw_set_output( 
	int, struct output_parms * 
);

ACUDLL int ACU_WINAPI sw_query_output(
	int, struct output_parms * 
);

ACUDLL int ACU_WINAPI sw_sample_input(
	int, struct sample_parms * 
);

ACUDLL int ACU_WINAPI sw_sample_input0(
	int, struct sample_parms * 
);

ACUDLL int ACU_WINAPI sw_clock_control(
	int, int 
);

ACUDLL int ACU_WINAPI sw_query_clock_control(
	int, struct query_clkmode_parms *
);

ACUDLL int ACU_WINAPI sw_dump_switch(
	int, struct dump_parms *  
);

ACUDLL int sw_set_trace(
	int, struct trace_parms * 
);

ACUDLL int sw_track_switch(
	int, struct track_parms * 
);

ACUDLL int sw_set_verify(
	int, int 
);

ACUDLL int ACU_WINAPI sw_tristate_switch( 
	int, int 
);

ACUDLL int ACU_WINAPI sw_get_drvrs(
	void 
);

ACUDLL int ACU_WINAPI sw_clock_scbus_master(
	void 
);

ACUDLL int ACU_WINAPI sw_switch_override_mode( 
	int, int 
);

ACUDLL int ACU_WINAPI sw_mode_switch(
	int, struct swmode_parms * 
);


/*--------- H.100 switch functions ------------*/
/*---------------------------------------------*/

ACUDLL int ACU_WINAPI sw_h100_config_board_clock( 
	int, struct h100_config_board_clock_parms*    	
);

ACUDLL int ACU_WINAPI sw_h100_config_netref_clock( 
	int, struct h100_netref_clock_parms*   
);

ACUDLL int ACU_WINAPI sw_h100_query_board_clock( 
	int, struct h100_query_board_clock_parms*  
);

ACUDLL int ACU_WINAPI sw_h100_query_netref_clock( 
	int, struct h100_netref_clock_parms*   
);


/*----------- MC3 switch functions ------------*/
/*---------------------------------------------*/

ACUDLL int sw_mc3_config_bert( 
	int, struct mc3_config_bert_parms*    	
);

ACUDLL int sw_mc3_bert_status( 
	int, struct mc3_bert_status_parms*    	
);

ACUDLL int sw_mc3_bypass_fibre( 
	int, int 
);

ACUDLL int sw_mc3_resync_ring( 
	int, int 
);

ACUDLL int sw_mc3_ring_latched_status( 
	int, struct mc3_ring_status_parms*
);

ACUDLL int sw_mc3_ring_status( 
	int, struct mc3_ring_status_parms*    	
);

ACUDLL int sw_mc3_event_criteria( 
	int, struct mc3_event_criteria_parms*    	
);

ACUDLL int sw_register_event( 
	int, struct register_event_parms*    	
);


ACUDLL int sw_ev_create( int, tSWEventId* );
ACUDLL int sw_ev_wait( int, tSWEventId );
ACUDLL int sw_ev_free( int, tSWEventId );


/*--------- Old Single Driver Functions -------*/
/*---------------------------------------------*/

ACUDLL int ver_switch( 
	struct swver_parms*
);

ACUDLL int reset_switch( 
	void 
);

ACUDLL int reinit_switch( 
	void 
);

ACUDLL int query_switch_caps( 
	struct capabilities_parms * 
);

ACUDLL int set_output( 
	struct output_parms * 
);

ACUDLL int query_output( 
	struct output_parms * 
);

ACUDLL int sample_input( 
	struct sample_parms * 
);

ACUDLL int sample_input0( 
	struct sample_parms * 
);

ACUDLL int clock_control( 
	int 
);

ACUDLL int query_clock_control(
	struct query_clkmode_parms *
);

ACUDLL int dump_switch( 
	struct dump_parms *
);

ACUDLL int set_trace( 
	struct trace_parms * 
);

ACUDLL int set_verify( 
	int 
);

ACUDLL int tristate_switch( 
	int 
);

ACUDLL int switch_override_mode( 
	int 
);

#ifdef SW_TRACK_API

ACUDLL void sw_crack_result(
	int, char*
);

ACUDLL void sw_track_api_calls(
	int
);

#endif

/*---------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif

#ifndef ACU_SOLARIS_SPARC
#pragma pack ( )
#endif

#endif


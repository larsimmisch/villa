/*------------------------------------------------------------*/
/* ACULAB plc                                                 */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : mvcldrvr.h                             */
/*                                                            */
/*           Purpose : Header file for call subsystem         */
/*                                                            */
/*       Create Date : 15th September 1992                    */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Change History                                             */
/*                                                            */
/* rev: 5.8.0   11/10/2001 Updated for 5.8.0 Release          */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/


#ifndef _MVCLDRVR_
#define _MVCLDRVR_

/* define NT_WOS to enable generation of Win32 events for call events and changes in */
/* level 1 and 2 status */
#ifdef _WIN32
#define NT_WOS
#endif

#ifdef NT_WOS
#ifndef DRIVER

#include <windows.h>

#endif
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


#ifndef ACUDLL
#define ACUDLL
#endif

#ifdef ACU_DIGITAL
#include <sys/ioctl.h>
typedef struct{
	int		status;
	union ioctlu*	ioctlup;
	}MSGBLK;
#endif

#ifndef LINUX
#ifndef ACU_SOLARIS_SPARC
# pragma pack ( 1 )
# define ACU_PACK_DIRECTIVE
#else
# define ACU_PACK_DIRECTIVE
#endif /* ACU_SOLARIS_SPARC */
#else
# define ACU_PACK_DIRECTIVE __attribute__ ((packed))
#endif /* LINUX */



/*------------ data type definitions ---------------*/
/*                                                  */
/* The device driver makes the following assumptions*/
/* about the data types char, int and long          */
/*                                                  */
/*             char:  8 bit, signed                 */
/*             int:  16 bit, signed                 */
/*             long: 32 bit, signed                 */
/*                                                  */
/* The libraries have been modified to use the      */
/* typedef's provided below. If you wish to change  */
/* your environment, then modify the typedef's to   */
/* suit.                                            */
/*                                                  */


/*-------------------------------------------------------*/
/*                                                       */
/* for Solaris Sparc 32/64 bit environment               */
/* compilers use the following typedefs:                 */
/*                                                       */
/*-------------------------------------------------------*/

#ifdef ACU_SOLARIS_SPARC

#include <inttypes.h>

#ifndef _BUFP_
    typedef char  * BUFP;                   /* obsolete (pre-V5) */
#define _BUFP_
#endif

#ifndef _ACU_CHAR_
    typedef int8_t  ACU_CHAR;               /* signed 8 bit    */
#define _ACU_CHAR_
#endif

#ifndef _ACU_UCHAR_
    typedef uint8_t ACU_UCHAR;              /* unsigned 8 bit  */
#define _ACU_UCHAR_
#endif

#ifndef _ACU_INT_
    typedef int32_t   ACU_INT;              /* signed 16 bit   */
#define _ACU_INT_
#endif

#ifndef _ACU_UINT_
    typedef uint32_t ACU_UINT;              /* unsigned 16 bit */
#define _ACU_UINT_
#endif

#ifndef _ACU_LONG_
    typedef int32_t  ACU_LONG;              /* signed 32 bit   */
#define _ACU_LONG_
#endif

#ifndef _ACU_ULONG_
    typedef uint32_t  ACU_ULONG;            /* unsigned 32 bit */
#define _ACU_ULONG_
#endif

#ifndef _ACU_ACT_
    typedef uint64_t  ACU_ACT;              /* unsigned 64 bit */
#define _ACU_ACT_
#endif

#endif



/*-------------------------------------------------------*/
/*                                                       */
/* for Windows NT, Unix, Linux                           */
/* bit compilers use the following typedefs:             */
/*                                                       */
/*-------------------------------------------------------*/

#ifndef _BUFP_
    typedef char  * BUFP;                 /* obsolete (pre-V5) */
#define _BUFP_
#endif

#ifndef _ACU_CHAR_
    typedef char  ACU_CHAR;               /* signed 8 bit    */
#define _ACU_CHAR_
#endif

#ifndef _ACU_UCHAR_
    typedef unsigned char ACU_UCHAR;      /* unsigned 8 bit  */
#define _ACU_UCHAR_
#endif

#ifndef _ACU_SHORT_
    typedef short  ACU_SHORT;             /* unsigned 16 bit */
#define _ACU_SHORT_
#endif

#ifndef _ACU_USHORT_
    typedef unsigned short  ACU_USHORT;   /* unsigned 16 bit */
#define _ACU_USHORT_
#endif


#ifndef _ACU_INT_
    typedef int   ACU_INT;                /* signed 32 bit   */
#define _ACU_INT_
#endif

#ifndef _ACU_UINT_
    typedef unsigned int ACU_UINT;        /* unsigned 32 bit */
#define _ACU_UINT_
#endif

#ifndef _ACU_ACT_
    typedef unsigned long ACU_ACT;        /* unsigned 32 bit */
#define _ACU_ACT_
#endif

#ifndef ACU_DIGITAL

/*-------------------------------------------------------*/
/*                                                       */
/* for Digital Unix                                      */
/* bit compilers use the following typedefs:             */
/*                                                       */
/*-------------------------------------------------------*/

#ifndef _ACU_LONG_
    typedef long  ACU_LONG;               /* signed 32 bit   */
#define _ACU_LONG_
#endif

#ifndef _ACU_ULONG_
    typedef unsigned long  ACU_ULONG;     /* unsigned 32 bit */
#define _ACU_ULONG_
#endif

#else

#ifndef _ACU_LONG_
    typedef int  ACU_LONG;                /* signed 32 bit   */
#define _ACU_LONG_
#endif

#ifndef _ACU_ULONG_
    typedef unsigned int  ACU_ULONG;      /* unsigned 32 bit */
#define _ACU_ULONG_
#endif

#ifndef _ACU_ACT_
    typedef unsigned long ACU_ACT;        /* unsigned 32 bit */
#define _ACU_ACT_
#endif

#endif



/*-------------------------------------------------------*/
/*                                                       */
/* Some NT specifics for event signalling                */
/*                                                       */
/*-------------------------------------------------------*/
#ifdef NT_WOS
#define kMVNTL1EvBaseName "MVIP$MVL1SignalEvt"
#define kMVNTL1EvBaseWideName L"MVIP$MVL1SignalEvt"
#define kMVNTL2EvBaseName "MVIP$MVLayer2SignalEvt"
#define kMVNTL2EvBaseWideName L"MVIP$MVLayer2SignalEvt"
#define kMVNTEvBaseName "MVIP$MVSignalEvt"
#define kMVNTEvBaseWideName L"MVIP$MVSignalEvt"

#ifndef DRIVER
typedef HANDLE tMVEventId;

#ifdef __cplusplus
extern "C"{
#endif
int mvcl_ev_create ( tMVEventId *eventId );
int mvcl_l1_ev_create ( tMVEventId *eventId );
int mvcl_l2_ev_create ( tMVEventId *eventId );
int mvcl_ev_free ( tMVEventId *eventId );
int mvcl_ev_wait ( tMVEventId *eventId );
#ifdef __cplusplus
};
#endif

#endif
#endif


/*-------------------------------------------------------------------*/

#define CONFSTRSIZE 128      /* maximum size of configuration string */
#define XFERSIZE    0x2000   /* download data transfer size          */
#define PM4ZAPSIZE  0x1ff6   /* pm4 zap code data transfer size      */

/*---- API library version information -----*/
/*- do not change these values -*/
#define SIGNATUREA      0x04087026
#define ACU_API_VERSION 0x0000000b
#define SIGNATUREB      0x20099303
/*- do not change these values -*/



/*---- system configuration information ----*/
/*------------------------------------------*/

#define MAXCNTRL   20                  /* maximum number of controllers        */
#define MAXPPC      8                  /* maximum number of ports per card     */
#define MAXNPP      8                  /* Max associated nets per signalling port */
#define MAXIO       3                  /* maximum types of handle              */
#define MAXPORT    (MAXCNTRL * MAXPPC) /* maximum number of ports in system    */
#define MAXNUM     32                  /* maximum number of subscriber digits  */
#define MAXSIGSYS   8                  /* maximum number of signalling type    */
#define MAXBEARER  16                  /* maximum length of bearer code        */
#define MAXCLC     10                  /* maximum length of calling category   */
#define MAXSPID    32                  /* maximum length of spid               */
#define MAXHILAYER 16                  /* maximum length of high layer         */
#define MAXLOLAYER 16                  /* maximum length of low layer          */
#define MAXPROGRESS 4                  /* maximum length of progress indicator */
#define MAXNOTIFY   4                  /* maximum length of notify indicator   */
#define MAXDISPLAY 34                  /* maximum length of display            */
#define MAXMESSWAIT 16                 /* maximum length of message waiting    */
#define MAXKEYPAD   34                 /* maximum length of keypad information */

#define MAXUUI_INFO 128                /* maximum length of User To User information */
#define MAXFACILITY_INFO 128           /* maximum length of Facility information */
#define MAXRAWDATA 260                 /* maximum length of raw data information */

#define MAX_FEAT_MSG       5           /* dpnss enhanced only: Max Msg for DPNSS feature Info          */
#define MAXNSI             50          /* dpnss enhanced only: maximum number of DPNSS NSI characters  */
#define MAXTXT             30          /* dpnss enhanced only: maximum number of DPNSS TXT characters  */
#define MAXTID             15          /* dpnss enhanced only: maximum number of Trunk ID characters   */
#define TRANSIT_MSG_LENGTH 50          /* dpnss enhanced only: DPNSS transit message length            */

#define MAXL3LENGTH       260          /* The Max Packet size for a layer 3 packet to be transported accross layer 2 */

#define MAXSERIALNO        12          /* Maximum length of the serial number of the PCI cards */
#define MAXHWVERSION       20          /* Maximum length of the hardware version number */

#define INCOMING  1       /* incoming call */
#define OUTGOING  2       /* outgoing call */

#define XFERSIZE 0x2000   /* download data transfer size */


/*------ call charging ------*/
/* call charging information */
/*---------------------------*/

#define CHARGE_NONE   0     /* no valid information */
#define CHARGE_INFO   1     /* call charge valid    */
#define CHARGE_METER  2     /* meter pulse valid    */
#define CHARGEMAX    34     /* maximum number of call charging digits */


/*---------- service octet defintions ---------*/
/* and additional information octet definitions*/
/*---------------------------------------------*/

#define  UNKNOWN_SERVICE 0xff  /* undetermined service */

#define  TELEPHONY    1     /* service octet - telephony */
#define     ISDN_3K1  1     /* 3.1khz telephony */
#define     ANALOGUE  2     /* analogue         */
#define     ISDN_7K   3     /* 7Khz telephony   */

#define  ABSERVICE    2     /* service octet - a/b services */
#define     FAXGP2    1     /* group fax 2 */
#define     FAXGP3    2     /* group fax 3 */

#define  X21SERVICE   3     /* service octet - X.21 Services     */
#define     UC4       4     /* UC 4  */
#define     UC5       5     /* UC 5  */
#define     UC6       6     /* UC 6  */
#define     UC19     12     /* UC 19 */

#define  FAXGP4       4     /* service octet - Fax Group 4       */
#define  VIDEO64K     5     /* service octet - 64Kbits Videotext */
#define  DATA64K      7     /* service octet - 64Kbits data      */

#define  X25SERVICE   8     /* service octet - X.25 Services     */
#define     UC8       1     /* UC 8  */
#define     UC9       2     /* UC 9  */
#define     UC10      3     /* UC 10 */
#define     UC11      4     /* UC 11 */
#define     UC13      5     /* UC 13 */
#define     UC19K2    6     /* 19.2k */

#define  TELTEXT64    9     /* service octet - Teletext 64       */
#define  MIXEDMODE   10     /* service octet - Mixed Mode        */
#define  TELEACTION  13     /* service octet - Teleaction        */
#define  GRAPHIC     14     /* service octet - Graphic Telephone */
#define  VIDEOTEXT   15     /* service octet - Videotext         */

#define  VIDEOPHONE  16     /* service octet - Videophone        */
#define     SOUND_3K1 1     /* 3.1khz sound     */
#define     SOUND_7K  2     /* 7Khz sound       */
#define     IMAGE     3     /* image            */

/*---------------------------------------------*/


/*-------------- VoIP specific information --------------*/
/*-------------------------------------------------------*/

/* maximum number of codecs in system */
#define MAXCODECS   3

/* VoIP administration channel message types */
#define  GRQ        1
#define  GCF        2
#define  GRJ        3

#define  RRQ        4
#define  RCF        5
#define  RRJ        6

#define  URQ        7
#define  UCF        8
#define  URJ        9

#define  LRQ       10
#define  LCF       11
#define  LRJ       12

/* Do not use 0 for codec type! */
#define G711  1
#define G723  2
#define G729A 3

#define VAD_DEFAULT 0
#define VAD_OFF     1
#define VAD_ON      2

#define TDM_ALAW    1
#define TDM_ULAW    2


/*---------------------------------------*/
/*  Signalling Protocol Identifiers      */
/*---------------------------------------*/

#define  S_UNKNOWN       0

#define  S_1TR6          1     /* user end definitions */
#define  S_DASS          2
#define  S_DPNSS         3
#define  S_CAS           4
#define  S_AUSTEL        5
#define  S_ETS300        6
#define  S_VN3           7
#define  S_ATT           8
#define  S_CAS_TONE      9
#define  S_TNA_NZ       10
#define  S_FETEX_150    11
#define  S_SWETS300     12
#define  S_IDAP         13
#define  S_T1CAS        14
#define  S_T1CAS_TONE   15
#define  S_NI2          16
#define  S_DPNSS_EN     17
#define  S_ATT_T1       18
#define  S_QSIG         19

#define  S_1TR6NET      20     /* network end definitions */
#define  S_VN3NET       21
#define  S_ETSNET       22
#define  S_AUSTNET      23
#define  S_ATTNET       24
#define  S_DASSNET      25
#define  S_TNANET       26
#define  S_FETEXNET     27
#define  S_SWETSNET     28
#define  S_IDAPNET      29
#define  S_NI2NET       30
#define  S_ATTNET_T1    31
#define  S_DPNSS_T1     32
#define  S_FETEX_150_T1 33
#define  S_FETEXNET_T1  34

#define  S_INS_T1       35
#define  S_INSNET_T1    36

#define  S_INS          37
#define  S_INSNET       38

#define  S_ISUP         39

#define  S_GLOBAND      40
#define  S_GLOBNET      41

#define  S_MON          42
#define  S_MON_T1       43

#define  S_QSIG_T1      44
#define  S_DPNSS_EN_T1  45

#define  S_ETS300_T1    46
#define  S_ETSNET_T1    47

#define  S_H323         48


/*---------------------------------------*/
/*  Basic Rate Identifiers               */
/*---------------------------------------*/

#define  BR_ETS300    50
#define  BR_NI1       51
#define  BR_ATT       52
#define  BR_INS       53

#define  BR_ETSNET    70
#define  BR_NI1NET    71
#define  BR_ATTNET    72
#define  BR_INSNET    73

#define  S_SS5_TONE   90

#define  S_BASE       99


/*---------------------------------------*/
/*  Line Interface Types                 */
/*---------------------------------------*/

#define  L_E1           1
#define  L_T1_CAS       2
#define  L_T1_ISDN      3
#define  L_BASIC_RATE   4
#define  L_PSN          5


/*---------------------------------------*/
/*  Card Types                           */
/*---------------------------------------*/

#define C_REV4          1
#define C_REV5          2
#define C_BR4           3
#define C_BR8           4
#define C_PM4           5
#define C_PCI_T1        6
#define C_VOIP          7

/*---------------------------------------*/
/*  PM module types                      */
/*---------------------------------------*/

#define PM4_NOTFITTED               0
#define PM4_ONE_PORT_E1             1
#define PM4_TWO_PORT_E1             2
#define PM4_FOUR_PORT_E1            3
#define PM4_ONE_PORT_T1             5
#define PM4_TWO_PORT_T1             6
#define PM4_FOUR_PORT_T1            7
#define PM4_TWO_PORT_E1T1           8
#define PM4_FOUR_PORT_E1T1          9
#define PM4_TWO_PORT_E1_MONITOR     10  /* high impedance, passive monitor */
#define PM4_FOUR_PORT_E1_MONITOR    11  /* high impedance, passive monitor */
#define PM4_TWO_PORT_T1_MONITOR     12  /* high impedance, passive monitor */
#define PM4_FOUR_PORT_T1_MONITOR    13  /* high impedance, passive monitor */


/*---------------------------------------*/
/*  Board types                          */
/*---------------------------------------*/

#define B_ISA		     1
#define B_P1_PCI	     2
#define B_C1_PCI	     3
#define B_V1_PCI	     4
#define B_C2_PCI	     5
#define B_P2_PCI	     6

/*------------- IOCTL Functions ---------*/
/* MVIP Switching Functions */
/*---------------------------------------*/
#ifdef ACU_DIGITAL

#define CALL_INIT                   _IOWR( 'm', 0, MSGBLK )
#define CALL_SIGNAL_INFO            _IOWR( 'm', 1, MSGBLK )
#define CALL_OPENOUT                _IOWR( 'm', 2, MSGBLK )
#define CALL_OPENIN                 _IOWR( 'm', 3, MSGBLK )
#define CALL_STATE                  _IOWR( 'm', 4, MSGBLK )
#define CALL_DETAILS                _IOWR( 'm', 5, MSGBLK )
#define CALL_ACCEPT                 _IOWR( 'm', 6, MSGBLK )
#define CALL_RELEASE                _IOWR( 'm', 7, MSGBLK )
#define CALL_DISCONNECT             _IOWR( 'm', 8, MSGBLK )
#define CALL_GETCAUSE               _IOWR( 'm', 9, MSGBLK )
#define CALL_TCMD                   _IOWR( 'm', 10, MSGBLK )
#define CALL_V4PBLOCK               _IOWR( 'm', 11, MSGBLK )
#define CALL_SFMW                   _IOWR( 'm', 12, MSGBLK )
#define CALL_LEQ                    _IOWR( 'm', 13, MSGBLK )
#define CALL_LEQ_S                  _IOWR( 'm', 14, MSGBLK )
#define CALL_GET_ORIGINATING_ADDR   _IOWR( 'm', 15, MSGBLK )
#define CALL_INCOMING_RINGING       _IOWR( 'm', 16, MSGBLK )
#define CALL_GET_CHARGE             _IOWR( 'm', 17, MSGBLK )
#define CALL_PUT_CHARGE             _IOWR( 'm', 18, MSGBLK )
#define CALL_SEND_OVERLAP           _IOWR( 'm', 19, MSGBLK )
#define CALL_SYSTEM_INFO            _IOWR( 'm', 20, MSGBLK )
#define CALL_ANSWERCODE             _IOWR( 'm', 21, MSGBLK )
#define CALL_SEND_ALARM             _IOWR( 'm', 22, MSGBLK )
#define CALL_ENDPOINT_INITIALISE    _IOWR( 'm', 23, MSGBLK )

#define CALL_EXPANSION              _IOWR( 'm', 24, MSGBLK )

#define CALL_LINK                   _IOWR( 'm', 25, MSGBLK )
#define CALL_PROTOCOL               _IOWR( 'm', 26, MSGBLK )
#define CALL_DCBA                   _IOWR( 'm', 27, MSGBLK )
#define CALL_FEATURE                _IOWR( 'm', 28, MSGBLK )
#define CALL_L2_STATE               _IOWR( 'm', 29, MSGBLK )
#define CALL_L1_STATS               _IOWR( 'm', 30, MSGBLK )
#define CALL_TRACE                  _IOWR( 'm', 31, MSGBLK )

#define EXPANSION_FUNCTION          _IOWR( 'm', 32, MSGBLK )

#define CALL_SEND_ENDPOINT_ID       _IOWR( 'm', 32, MSGBLK )
#define CALL_GET_ENDPOINT_STATUS    _IOWR( 'm', 33, MSGBLK )
#define CALL_GET_SPID               _IOWR( 'm', 34, MSGBLK )

#define CALL_BR_L1_STATS            _IOWR( 'm', 35, MSGBLK )
#define CALL_BR_L2_STATE            _IOWR( 'm', 36, MSGBLK )

#define CALL_PUT_FACILITY           _IOWR( 'm', 40, MSGBLK )
#define CALL_GET_FACILITY           _IOWR( 'm', 41, MSGBLK )

#define CALL_FEATURE_ACTIVATE       _IOWR( 'm', 45, MSGBLK )
#define CALL_SEND_KEYPAD_INFO       _IOWR( 'm', 46, MSGBLK )
#define CALL_GET_KEYPAD_INFO        _IOWR( 'm', 47, MSGBLK )
#define CALL_GET_BUTTON_STATUS      _IOWR( 'm', 48, MSGBLK )

#define CALL_HOLD                   _IOWR( 'm', 50, MSGBLK )
#define CALL_ENQUIRY                _IOWR( 'm', 51, MSGBLK )
#define CALL_TRANSFER               _IOWR( 'm', 52, MSGBLK )
#define CALL_SHUTTLE                _IOWR( 'm', 53, MSGBLK )
#define CALL_PROGRESS               _IOWR( 'm', 54, MSGBLK )
#define CALL_PROCEEDING             _IOWR( 'm', 55, MSGBLK )
#define CALL_NOTIFY                 _IOWR( 'm', 56, MSGBLK )
#define CALL_SETUP_ACK              _IOWR( 'm', 57, MSGBLK )

#define CALL_WATCHDOG               _IOWR( 'm', 58, MSGBLK )

#define CALL_DSP_CONFIG             _IOWR( 'm', 59, MSGBLK )
#define CALL_RECONNECT              _IOWR( 'm', 60, MSGBLK )
#define SEND_DPR_COMMAND            _IOWR( 'm', 61, MSGBLK )
#define RECEIVE_DPR_EVENT           _IOWR( 'm', 62, MSGBLK )

#define CALL_SEND_Q921              _IOWR( 'm', 63, MSGBLK )
#define CALL_GET_Q921               _IOWR( 'm', 64, MSGBLK )

#define CALL_BRDSPBLOCK             _IOWR( 'm', 70, MSGBLK )
#define CALL_V5PBLOCK               _IOWR( 'm', 71, MSGBLK )
#define CALL_TSINFO                 _IOWR( 'm', 72, MSGBLK )

#define CALL_FEATURE_OPENOUT        _IOWR( 'm', 73, MSGBLK )
#define CALL_FEATURE_ENQUIRY        _IOWR( 'm', 74, MSGBLK )
#define CALL_FEATURE_DETAILS        _IOWR( 'm', 75, MSGBLK )
#define CALL_FEATURE_SEND           _IOWR( 'm', 76, MSGBLK )

#define CALL_ASSOC_NET              _IOWR( 'm', 77, MSGBLK )
#define CALL_HANDLE_2_PORT          _IOWR( 'm', 78, MSGBLK )


#define CALL_SEND_CONNECTIONLESS    _IOWR( 'm', 79, MSGBLK )
#define CALL_GET_CONNECTIONLESS     _IOWR( 'm', 80, MSGBLK )
#define CALL_MAINTENANCE            _IOWR( 'm', 81, MSGBLK )

#ifdef ACUC_CLONED
#define CALL_EVENT_IF               _IOWR( 'm', 82, MSGBLK )
#endif


#define CALL_SEND_VOIP_ADMIN_MSG    _IOWR( 'm', 83, MSGBLK )
#define CALL_GET_VOIP_ADMIN_MSG     _IOWR( 'm', 84, MSGBLK )
#define CALL_OPEN_VOIP_ADMIN_CHAN   _IOWR( 'm', 85, MSGBLK )
#define CALL_CLOSE_VOIP_ADMIN_CHAN  _IOWR( 'm', 86, MSGBLK )
#define GENERIC_TLS_INIT_COMPLETE   _IOWR( 'm', 87, MSGBLK )
#define GENERIC_SEND_MSG_ACK        _IOWR( 'm', 88, MSGBLK )
#define CALL_VOIP_STARTUP           _IOWR( 'm', 89, MSGBLK )
#define CALL_VOIP_READ_BOARD_VERS   _IOWR( 'm', 90, MSGBLK )
#define CALL_VOIP_READ_BOARD_STATUS _IOWR( 'm', 91, MSGBLK )
#define CALL_GET_CONFIGURATION      _IOWR( 'm', 92, MSGBLK )
#define CALL_SET_CONFIGURATION      _IOWR( 'm', 93, MSGBLK )
#define CALL_GET_STATS              _IOWR( 'm', 94, MSGBLK )
#define CALL_SET_DEBUG              _IOWR( 'm', 95, MSGBLK )
#define CALL_SET_SYSTEM_INFO        _IOWR( 'm', 96, MSGBLK )

#define CALL_SIGNAL_APIEVENT        _IOWR( 'm', 97, MSGBLK )
#define CALL_CODEC_CONFIG           _IOWR( 'm', 98, MSGBLK )

/*--------------------------*/
/* Enhanced DPNSS functions */
/*--------------------------*/

#define FIRST_ENHANCED_FEATURE      _IOWR( 'm', 100, MSGBLK )

#define DPNS_INCOMING_RINGING       _IOWR( 'm', 101, MSGBLK )
#define DPNS_OPENOUT                _IOWR( 'm', 102, MSGBLK )
#define DPNS_SET_TRANSIT            _IOWR( 'm', 103, MSGBLK )
#define DPNS_SEND_TRANSIT           _IOWR( 'm', 104, MSGBLK )
#define DPNS_TRANSIT_DETAILS        _IOWR( 'm', 105, MSGBLK )
#define DPNS_SET_L2_CH              _IOWR( 'm', 106, MSGBLK )
#define DPNS_L2_STATE               _IOWR( 'm', 107, MSGBLK )
#define DPNS_SEND_FEAT_INFO         _IOWR( 'm', 108, MSGBLK )
#define DPNS_CALL_ACCEPT            _IOWR( 'm', 109, MSGBLK )
#define DPNS_CALL_DETAILS           _IOWR( 'm', 110, MSGBLK )
#define DPNS_GETCAUSE               _IOWR( 'm', 111, MSGBLK )
#define DPNS_DISCONNECT             _IOWR( 'm', 112, MSGBLK )
#define DPNS_RELEASE                _IOWR( 'm', 113, MSGBLK )
#define DPNS_SEND_OVERLAP           _IOWR( 'm', 114, MSGBLK )
#define DPNS_WATCHDOG               _IOWR( 'm', 115, MSGBLK )

/*--------------------------*/
/* Groomer Features         */
/*--------------------------*/

#define FIRST_GROOMER_FEATURE       _IOWR( 'm', 200, MSGBLK )
#define GROOMER_IO                  _IOWR( 'm', 201, MSGBLK )
#define GROOMER_WDENABLE            _IOWR( 'm', 202, MSGBLK )
#define GROOMER_WDINIT              _IOWR( 'm', 203, MSGBLK )
#define GROOMER_WDREFRESH           _IOWR( 'm', 204, MSGBLK )



#else

#define CALL_INIT                    0
#define CALL_SIGNAL_INFO             1
#define CALL_OPENOUT                 2
#define CALL_OPENIN                  3
#define CALL_STATE                   4
#define CALL_DETAILS                 5
#define CALL_ACCEPT                  6
#define CALL_RELEASE                 7
#define CALL_DISCONNECT              8
#define CALL_GETCAUSE                9
#define CALL_TCMD                   10
#define CALL_V4PBLOCK               11
#define CALL_SFMW                   12
#define CALL_LEQ                    13
#define CALL_LEQ_S                  14
#define CALL_GET_ORIGINATING_ADDR   15
#define CALL_INCOMING_RINGING       16
#define CALL_GET_CHARGE             17
#define CALL_PUT_CHARGE             18
#define CALL_SEND_OVERLAP           19
#define CALL_SYSTEM_INFO            20
#define CALL_ANSWERCODE             21
#define CALL_SEND_ALARM             22
#define CALL_ENDPOINT_INITIALISE    23

#define CALL_EXPANSION              24

#define CALL_LINK                   25
#define CALL_PROTOCOL               26
#define CALL_DCBA                   27
#define CALL_FEATURE                28
#define CALL_L2_STATE               29
#define CALL_L1_STATS               30
#define CALL_TRACE                  31

#define EXPANSION_FUNCTION          32

#define CALL_SEND_ENDPOINT_ID       32
#define CALL_GET_ENDPOINT_STATUS    33
#define CALL_GET_SPID               34

#define CALL_BR_L1_STATS            35
#define CALL_BR_L2_STATE            36

#define CALL_PUT_FACILITY           40
#define CALL_GET_FACILITY           41

#define CALL_FEATURE_ACTIVATE       45
#define CALL_SEND_KEYPAD_INFO       46
#define CALL_GET_KEYPAD_INFO        47
#define CALL_GET_BUTTON_STATUS      48

#define CALL_HOLD                   50
#define CALL_ENQUIRY                51
#define CALL_TRANSFER               52
#define CALL_SHUTTLE                53
#define CALL_PROGRESS               54
#define CALL_PROCEEDING             55
#define CALL_NOTIFY                 56
#define CALL_SETUP_ACK              57

#define CALL_WATCHDOG               58

#define CALL_DSP_CONFIG             59
#define CALL_RECONNECT              60
#define SEND_DPR_COMMAND            61
#define RECEIVE_DPR_EVENT           62

#define CALL_SEND_Q921              63
#define CALL_GET_Q921               64


#define CALL_BRDSPBLOCK             70
#define CALL_V5PBLOCK               71
#define CALL_TSINFO                 72

#define CALL_FEATURE_OPENOUT        73
#define CALL_FEATURE_ENQUIRY        74
#define CALL_FEATURE_DETAILS        75
#define CALL_FEATURE_SEND           76

#define CALL_ASSOC_NET              77
#define CALL_HANDLE_2_PORT          78

#define CALL_SEND_CONNECTIONLESS    79
#define CALL_GET_CONNECTIONLESS     80
#define CALL_MAINTENANCE            81

#ifdef ACUC_CLONED
#define CALL_EVENT_IF               82
#endif

#define CALL_SEND_VOIP_ADMIN_MSG    83
#define CALL_GET_VOIP_ADMIN_MSG     84
#define CALL_OPEN_VOIP_ADMIN_CHAN   85
#define CALL_CLOSE_VOIP_ADMIN_CHAN  86
#define GENERIC_TLS_INIT_COMPLETE   87
#define GENERIC_SEND_MSG_ACK        88
#define CALL_VOIP_STARTUP           89
#define CALL_VOIP_READ_BOARD_VERS   90
#define CALL_VOIP_READ_BOARD_STATUS 91
#define CALL_GET_CONFIGURATION      92
#define CALL_SET_CONFIGURATION      93
#define CALL_GET_STATS              94
#define CALL_SET_DEBUG              95
#define CALL_SET_SYSTEM_INFO        96

#define CALL_SIGNAL_APIEVENT        97
#define CALL_CODEC_CONFIG           98

/*--------------------------*/
/* Enhanced DPNSS functions */
/*--------------------------*/

#define FIRST_ENHANCED_FEATURE      100

#define DPNS_INCOMING_RINGING       101
#define DPNS_OPENOUT                102
#define DPNS_SET_TRANSIT            103
#define DPNS_SEND_TRANSIT           104
#define DPNS_TRANSIT_DETAILS        105
#define DPNS_SET_L2_CH              106
#define DPNS_L2_STATE               107
#define DPNS_SEND_FEAT_INFO         108
#define DPNS_CALL_ACCEPT            109
#define DPNS_CALL_DETAILS           110
#define DPNS_GETCAUSE               111
#define DPNS_DISCONNECT             112
#define DPNS_RELEASE                113
#define DPNS_SEND_OVERLAP           114
#define DPNS_WATCHDOG               115

/*--------------------------*/
/* Groomer Features         */
/*--------------------------*/

#define FIRST_GROOMER_FEATURE       200
#define GROOMER_IO                  201
#define GROOMER_WDENABLE            202
#define GROOMER_WDINIT              203
#define GROOMER_WDREFRESH           204

#endif


/*-------------------------------------*/
/* Sub-commands for SFMW               */
/*-------------------------------------*/

#define SFMWMODE_RESTART  0x0100
#define SFMWMODE_SSTYPE   0x0200
#define SFMWMODE_REGISTER 0x0300
#define SFMWMODE_ZAPDNLD  0x0400

#define REGISTER_XAPI     0x0001

/*-------------------------------------*/
/* call time configuration switches    */
/*-------------------------------------*/

#define CNF_REM_DISC      0x8000       /* remote disconnect  */
#define CNF_CALL_CHARGE   0x4000       /* call charging      */
#define CNF_TSPREFER      0x2000       /* preferred timeslot */
#define CNF_ENABLE_V5API  0x1000       /* enable V5 api      */
#define CNF_TSVIRTUAL     0x0800       /* virtual tineslot   */
#define CNF_NET_ANY       0x0200       /* Accept calls on associated nets */
#define CNF_COMPLETE      0x0100       /* ISRMI/ISRMC        */

#define CNF_NET_MASK      0x001f       /* for aculab use only*/


/*-------------------------------------*/
/* VoIP Signal Command/Events          */
/*-------------------------------------*/

#define ACU_SIGEVNT_INIT                  0x1
#define ACU_SIGEVNT_STATE                 0x2
#define ACU_SIGEVNT_DEBUG                 0x3
#define ACU_SIGEVNT_WAITFOREXIT           0x4
#define ACU_SIGEVNT_CLEAREXIT             0x5

#define ACU_SIGEVNT_EXITFROMCLOSE         0x2001
#define ACU_SIGEVNT_EXITFROMCLEAR         0x2002

#define ACU_VOIP_INACTIVE                 0
#define ACU_VOIP_ACTIVE                   1

/*-------------------------------------*/
/* Q.931 Message types                 */
/*-------------------------------------*/

#define Q931_NO_MSG             0x00
#define Q931_ALERTING           0x01
#define Q931_CALL_PROCEEDING    0x02
#define Q931_PROGRESS           0x03
#define Q931_SETUP              0x05
#define Q931_CONNECT            0x07
#define Q931_SETUP_ACK          0x0D
#define Q931_CONNECT_ACK        0x0F
#define Q931_USER_INFO          0x20
#define Q931_SUSPEND_REJ        0x21
#define Q931_RESUME_REJ         0x22
#define Q931_HOLD               0x24
#define Q931_SUSPEND            0x25
#define Q931_RESUME             0x26
#define Q931_HOLD_ACK           0x28
#define Q931_SUSPEND_ACK        0x2D
#define Q931_RESUME_ACK         0x2E
#define Q931_HOLD_REJECT        0x30
#define Q931_RETRIEVE           0x31
#define Q931_RETRIEVE_ACK       0x33
#define Q931_RETRIEVE_REJECT    0x37
#define Q931_DISCONNECT         0x45
#define Q931_RESTART            0x46
#define Q931_RELEASE            0x4D
#define Q931_RESTART_ACK        0x4E
#define Q931_RELEASE_CMPL       0x5A
#define Q931_SEGMENT            0x60
#define Q931_FACILITY           0x62
#define Q931_NOTIFY             0x6E
#define Q931_STATUS_ENQ         0x75
#define Q931_CONGESTION_CNTRL   0x79
#define Q931_INFORMATION        0x7B
#define Q931_STATUS             0x7D


/*------------------------------*/
/* DPNSS Feature MSG Types      */
/*------------------------------*/

#define  NO_MSG                 0 		/* DPNSS Enhanced Only */
#define  ACKNOWLEDGE            40		/* DPNSS Enhanced Only */
#define  REJECT                 41		/* DPNSS Enhanced Only */
#define  STATE_OF_DEST_FREE     42		/* DPNSS Enhanced Only */
#define  STATE_OF_DEST_BUSY     43		/* DPNSS Enhanced Only */
#define  STATE_OF_DEST_REQ      44		/* DPNSS Enhanced Only */

/*--- DPNSS Call types : Virtual/Real ---*/

#define REAL                    0		/* DPNSS Enhanced Only */
#define VIRTUAL                 1		/* DPNSS Enhanced Only */


/*-------------------------------------*/
/* Call Diversion messages             */
/*-------------------------------------*/

#define  DIVERT_IMMEDIATE       1 		/* DPNSS Enhanced Only */
#define  DIVERT_BUSY            2 		/* DPNSS Enhanced Only */
#define  DIVERT_NO_REPLY        3 		/* DPNSS Enhanced Only */
#define  DIVERTING_IMM          4 		/* DPNSS Enhanced Only */
#define  DIVERTING_BSY          5 		/* Generic */
#define  DIVERTING_RNR          6 		/* Generic */
#define  DIVERTED_IMM           7 		/* DPNSS Enhanced Only */
#define  DIVERTED_BSY           8 		/* DPNSS Enhanced Only */
#define  DIVERTED_RNR           9 		/* DPNSS Enhanced Only */
#define  DIV_VALIDATION        10		/* DPNSS Enhanced Only */

#define  DIV_BYPASS            11       /* DPNSS Enhanced Only */
#define  DIVERTING_UNKNOWN     12  		/* Generic */
#define  DIVERTING_UNCONDITIONAL    13  /* Generic */
#define  DIVERTING_CD          14       /* Generic */
#define  DEFLECTION_RINGING    15       /* Generic */
#define  DEFLECTION_IMM        16       /* Generic */


/*-------------------------------------*/
/* Call Hold messages                  */
/*-------------------------------------*/

#define  HOLD_CALL             20		/* DPNSS Enhanced Only */
#define  HOLD_ACK              21		/* DPNSS Enhanced Only */
#define  HOLD_REJECT           22		/* DPNSS Enhanced Only */
#define  HOLD_NOT_SUPPORTED    23               /* DPNSS Enhanced Only */
#define  RECONNECT_CALL        24 		/* DPNSS Enhanced Only */


/*-------------------------------------*/
/* DPNSS Enquiry Call messages         */
/*-------------------------------------*/

#define ENQUIRY                30 		/* DPNSS Enhanced Only */

/*-------------------------------------*/
/* DPNSS Call Transfer messages        */
/*-------------------------------------*/

#define TRANSFER_O             31		/* DPNSS Enhanced Only */
#define TRANSFER_T             32		/* DPNSS Enhanced Only */
#define TRANSFERRED            33		/* DPNSS Enhanced Only */
#define TRANSFERRED_INFO       34		/* DPNSS Enhanced Only */

/*-------------------------------------*/
/* DPNSS Mitel Call Charging           */
/*-------------------------------------*/

#define CHARGE_REQUEST         50  		/* DPNSS Enhanced Only */
#define CHARGE_UNITS_USED      51  		/* DPNSS Enhanced Only */
#define CHARGE_ACTIVATE        52  		/* DPNSS Enhanced Only */

#define CHARGE_ACCOUNT_REQUEST 53		/* DPNSS Enhanced Only */
#define CHARGE_ACCOUNT_CODE    54		/* DPNSS Enhanced Only */


/*-------------------------------------*/
/* DPNSS Conference                    */
/*-------------------------------------*/

#define ADD_ON_VALIDATION      60 		/* DPNSS Enhanced Only */
#define ADD_ON_ACK             61 		/* DPNSS Enhanced Only */
#define ADDED_ON               62		/* DPNSS Enhanced Only */
#define ADD_ON_REJECT          63		/* DPNSS Enhanced Only */
#define ADD_ON_NOT_SUPPORTED   64		/* DPNSS Enhanced Only */
#define ADD_ON_CLEARDOWN       65		/* DPNSS Enhanced Only */
#define TWO_PARTY_O            66		/* DPNSS Enhanced Only */
#define TWO_PARTY_T            67		/* DPNSS Enhanced Only */


/*-------------------------------------*/
/* Executive Intrusion                 */
/*-------------------------------------*/

#define INTRUSION_REQUEST      70		/* DPNSS Enhanced Only */
#define PV_INTRUSION           71		/* DPNSS Enhanced Only */
#define INTRUSION_ACK          72		/* DPNSS Enhanced Only */
#define INTRUDING              73		/* DPNSS Enhanced Only */
#define IPL_REQUEST            74		/* DPNSS Enhanced Only */
#define IPL_RESPONSE           75		/* DPNSS Enhanced Only */
#define INTRUSION_WITHDRAW     76		/* DPNSS Enhanced Only */
#define WITHDRAW_ACK           77		/* DPNSS Enhanced Only */
#define WITHDRAW_NOT_SUPPORTED 78		/* DPNSS Enhanced Only */


/*-------------------------------------*/
/* DPNSS Call Back Messaging           */
/*-------------------------------------*/

#define CALL_BACK_MESSAGE_REQ  80		/* DPNSS Enhanced Only */
#define CALL_BACK_MESSAGE_CAN  81		/* DPNSS Enhanced Only */
#define DPNSS_RAW              82		/* DPNSS Enhanced Only */


/*---------------------------------*/
/* DPNSS layer 2 states / commands */
/*---------------------------------*/

#define  DPNS_L2_ENABLE        1        /* DPNSS Enhanced Only */
#define  DPNS_L2_DISABLE       2        /* DPNSS Enhanced Only */

/*--------------------------------------*/
/* dpnss Calling/Called Line Categories */
/*--------------------------------------*/

#define NO_CLC                 0        /* DPNSS Enhanced Only */
#define ORDINARY               1        /* DPNSS Enhanced Only */
#define DECADIC                2        /* DPNSS Enhanced Only */
#define DASS2                  3        /* DPNSS Enhanced Only */
#define PSTN                   4        /* DPNSS Enhanced Only */
#define MF5                    5        /* DPNSS Enhanced Only */
#define OPERATOR               6        /* DPNSS Enhanced Only */
#define NETWORK                7        /* DPNSS Enhanced Only */
#define CONFERENCE             8        /* DPNSS Enhanced Only */

/*---------------------------------------*/
/* CLI presentation indicators for Q.931 */
/*---------------------------------------*/

#define PR_ALLOWED             0
#define PR_RESTRICTED          1
#define PR_NOTAVAILABLE        2

/*---------------------------------------*/
/* CLI Screening indicators for Q.931    */
/*---------------------------------------*/

#define SC_NOTSCREENED         0
#define SC_VERIFYPASS          1
#define SC_VERIFYFAIL          2
#define SC_NETWORKPROVIDED     3

/*---------------------------------------*/
/* Keypad locations                      */
/*---------------------------------------*/

#define KEYPAD_GLOBAL          0
#define KEYPAD_CALL_REF        1
#define KEYPAD_CONNECT         2

/*-------------------------------------*/
/* state values                        */
/*-------------------------------------*/
#define CS_IDLE                0x00000000
#define CS_WAIT_FOR_INCOMING   0x00000001
#define CS_INCOMING_CALL_DET   0x00000002
#define CS_CALL_CONNECTED      0x00000004
#define CS_WAIT_FOR_OUTGOING   0x00000008
#define CS_OUTGOING_RINGING    0x00000010
#define EV_INCOMING_DETAILS    0x00000020
#define EV_CALL_CHARGE         0x00000021
/*
spare                          0x00000040
 */
#define CS_EMERGENCY_CONNECT   0x00000080
#define CS_TEST_CONNECT        0x00000100
/*
spare                          0x00000200
 */
#define CS_REMOTE_DISCONNECT   0x00000400
#define CS_WAIT_FOR_ACCEPT     0x00000800
#define CS_PROGRESS            0x00001000
#define CS_OUTGOING_PROCEEDING 0x00002000
#define EV_NOTIFY              0x00004000
#define CS_INFO                0x00008000
#define CS_HOLD                0x00010000
#define CS_HOLD_REJECT         0x00020000
#define CS_TRANSFER_REJECT     0x00040000
#define CS_RECONNECT_REJECT    0x00080000
#define EV_CHARGE_INT          0x00100000
/*-----------------------------------------------------*/
/* Extension to indicate new events in extended_state  */
/*-----------------------------------------------------*/
#define CS_EXTENDED            0x00200000
/*
spare                          0x00400000
spare                          0x00800000
 */
#define CS_DPNS_TRANSIT        0x01000000		 /* DPNSS Enhanced Only */
#define CS_DPNS_IN_TRANSIT     0x02000000		 /* DPNSS Enhanced Only */
#define CS_DPNS_HOLDING        0x04000000		 /* DPNSS Enhanced Only */
#define CS_DPNS_HELD           0x08000000		 /* DPNSS Enhanced Only */
#define CS_DPNS_CONFERENCE     0x10000000		 /* DPNSS Enhanced Only */
#define CS_DPNS_INTRUDING      0x20000000		 /* DPNSS Enhanced Only */
#define EV_DPNS_IN_TRANSIT     0x40000000        /* DPNSS Enhanced Only */




/*-------------------------------------*/
/* event values                        */
/*-------------------------------------*/

#define EV_IDLE                CS_IDLE
#define EV_WAIT_FOR_INCOMING   CS_WAIT_FOR_INCOMING
#define EV_INCOMING_CALL_DET   CS_INCOMING_CALL_DET
#define EV_CALL_CONNECTED      CS_CALL_CONNECTED
#define EV_EMERGENCY_CONNECT   CS_EMERGENCY_CONNECT
#define EV_TEST_CONNECT        CS_TEST_CONNECT
#define EV_WAIT_FOR_OUTGOING   CS_WAIT_FOR_OUTGOING
#define EV_OUTGOING_RINGING    CS_OUTGOING_RINGING
#define EV_REMOTE_DISCONNECT   CS_REMOTE_DISCONNECT
#define EV_WAIT_FOR_ACCEPT     CS_WAIT_FOR_ACCEPT
#define EV_HOLD                CS_HOLD
#define EV_HOLD_REJECT         CS_HOLD_REJECT
#define EV_TRANSFER_REJECT     CS_TRANSFER_REJECT
#define EV_RECONNECT_REJECT    CS_RECONNECT_REJECT
#define EV_PROGRESS            CS_PROGRESS
#define EV_OUTGOING_PROCEEDING CS_OUTGOING_PROCEEDING
#define EV_DETAILS             EV_INCOMING_DETAILS
#define EV_EXTENDED            CS_EXTENDED

/*-------------------------------------*/
/* DPNSS Enhanced Events               */
/*-------------------------------------*/

#define EV_DPNS_TRANSIT        CS_DPNS_TRANSIT	        /* DPNSS Enhanced Only */
#define EV_DPNS_HOLDING        CS_DPNS_HOLDING          /* DPNSS Enhanced Only */
#define EV_DPNS_HELD           CS_DPNS_HELD             /* DPNSS Enhanced Only */
#define EV_DPNS_CONFERENCE     CS_DPNS_CONFERENCE       /* DPNSS Enhanced Only */
#define EV_DPNS_INTRUDING      CS_DPNS_INTRUDING        /* DPNSS Enhanced Only */

/*-----------------------------------------------*/
/* Extended Events - look in - extended_state    */
/*-----------------------------------------------*/
#define EV_EXT_FACILITY             0x00000001
#define EV_EXT_UUI_PENDING          0x00000002
#define EV_EXT_UUI_CONGESTED        0x00000004
#define EV_EXT_UUI_UNCONGESTED      0x00000008
#define EV_EXT_UUS_SERVICE_REQUEST  0x00000010
#define EV_EXT_HOLD_REQUEST         0x00000020
#define EV_EXT_RECONNECT_REQUEST    0x00000040
#define EV_EXT_TRANSFER_INFORMATION 0x00000080


/*-------------------------------------*/
/* Feature Indications                 */
/*-------------------------------------*/

#define FEATURE_FACILITY        0x00000001
#define FEATURE_USER_USER       0x00000002
#define FEATURE_DIVERSION       0x00000004
#define FEATURE_MESSAGE_WAITING 0x00000008
#define FEATURE_REGISTER        0x00000010
#define FEATURE_KEYPAD          0x00000020
#define FEATURE_DISPLAY         0x00000040
#define FEATURE_VIRTUAL         0x00000080
#define FEATURE_HOLD_RECONNECT  0x00000100
#define FEATURE_TRANSFER        0x00000200
/* Some firmwares give access to information elements not directly
 * supported through the API. Seeing this flag set indicates that
 * this information is present.
 */
#define FEATURE_RAW_DATA        0x00000400

/*-------------------------------------*/
/* DPNSS layer 2 states                */
/*-------------------------------------*/

#define DPNS_L2_NO_STATE       0x00                        /* DPNSS Enhanced Only */
#define DPNS_L2_ENABLED        0x01                        /* DPNSS Enhanced Only */
#define DPNS_L2_DISABLED       0x02                        /* DPNSS Enhanced Only */

/*------------------------------*/
/*  Layer 3 to Data Link Layer  */
/*------------------------------*/

#define     DL_ESTABLISH_REQ            1                /* Q921 Driver Only */
#define     DL_RELEASE_REQ              2                /* Q921 Driver Only */
#define     DL_DATA_REQ                 3                /* Q921 Driver Only */
#define     DL_UNIT_DATA_REQ            4                /* Q921 Driver Only */


/*------------------------------*/
/*  Data Link Layer to Layer 3  */
/*------------------------------*/

#define     DL_ESTABLISH_IND            5                /* Q921 Driver Only */
#define     DL_RELEASE_IND              6                /* Q921 Driver Only */
#define     DL_UNIT_DATA_IND            7                /* Q921 Driver Only */
#define     DL_DATA_IND                 8                /* Q921 Driver Only */
#define     DL_ESTABLISH_CON            9                /* Q921 Driver Only */
#define     DL_RELEASE_CON              10               /* Q921 Driver Only */


/*-------------------------------------*/




/*-------------------------------------*/
/*-------------------------------------*/


/*-------------------------*/
/* numbering type          */
/*-------------------------*/

#define NT_UNKNOWN            0
#define NT_INTERNATIONAL      1
#define NT_NATIONAL           2
#define NT_NETWORK_SPECIFIC   3
#define NT_SUBSCRIBER_NUMBER  4
#define NT_ABBREVIATED_NUMBER 6

/*-------------------------*/
/* numbering plan          */
/*-------------------------*/

#define NP_UNKNOWN            0
#define NP_ISDN               1
#define NP_DATA               3
#define NP_TELEX              4
#define NP_NATIONAL_STANDARD  8
#define NP_PRIVATE            9

/*-------------------------*/
/* Nature of address (ISUP)*/
/*-------------------------*/

#define NOA_SUBSCRIBER_NUMBER 1
#define NOA_NATIONAL_RESERVED 2
#define NOA_NATIONAL          3
#define NOA_INTERNATIONAL     4

/*------------------------------*/
/* Calling Party Category (ISUP)*/
/*------------------------------*/

#define CPC_FRENCH_OPERATOR   1
#define CPC_ENGLISH_OPERATOR  2
#define CPC_GERMAN_OPERATOR   3
#define CPC_RUSSIAN_OPERATOR  4
#define CPC_SPANISH_OPERATOR  5
#define CPC_ORDINARY_SUBSCRIBER 10
#define CPC_PRIORITY_SUBSCRIBER 11
#define CPC_DATA              12
#define CPC_TEST              13
#define CPC_PAYPHONE          15

/*-----------------------------------*/
/* Continuity Check Indicator (ISUP) */
/*-----------------------------------*/

#define CCI_NOT_REQUIRED      0
#define CCI_REQUIRED          1
#define CCI_PREVIOUS          2

/*------------------------------*/
/* ISUP Preference Indicators   */
/*------------------------------*/

#define PI_ISUP_PREFERRED     0
#define PI_ISUP_NOT_REQUIRED  1
#define PI_ISUP_REQUIRED      2

/*--------------------------------*/
/* Port blocking reason codes,    */
/* Meanings are protocol-specific */
/* ISUP Blocking Reasons          */
/*--------------------------------*/

#define ISUP_MAINTENANCE_BLOCK  0x0
#define ISUP_HW_BLOCK           0x1

/*-------------------------*/
/* error codes             */
/*-------------------------*/

#define ERR_HANDLE      ( -2  )  /* Illegal handle, or out of handles      */
#define ERR_COMMAND     ( -3  )  /* illegal command                        */
#define ERR_NET         ( -4  )  /* illegal network number                 */
#define ERR_PARM        ( -5  )  /* illegal parameter                      */
#define ERR_RESPONSE    ( -6  )  /*                                        */
#define ERR_NOCALLIP    ( -7  )  /* no call in progress                    */
#define ERR_TSBAR       ( -8  )  /* timeslot barred                        */
#define ERR_TSBUSY      ( -9  )  /* timeslot busy                          */
#define ERR_CFAIL       ( -10 )  /* command failed to execute              */
#define ERR_SERVICE     ( -11 )  /* invalid service code                   */
#define ERR_BUFF_FAIL   ( -12 )  /* out of buffer resources                */

/* download errors defined */

#define ERR_DNLD_ZAP     ( -13 )  /* debug monitor running                  */
#define ERR_DNLD_NOCMD   ( -14 )  /* firmware not loaded - command denied   */
#define ERR_DNLD_NODNLD  ( -15 )  /* firmware installed - download denied   */
#define ERR_DNLD_GEN     ( -16 )  /* general failure during download        */
#define ERR_DNLD_NOSIG   ( -17 )  /* downloaded firmware failed to start    */
#define ERR_DNLD_NOEXEC  ( -18 )  /* downloaded firmware is not executing   */
#define ERR_DNLD_NOCARD  ( -19 )  /* downloaded firmware is not executing   */
#define ERR_DNLD_SYSTAT  ( -20 )  /* downloaded firmware detected an error  */
#define ERR_DNLD_BADTLS  ( -21 )  /* driver does not support the firmware   */
#define ERR_DNLD_POST    ( -22 )  /* board failed power on self test        */
#define ERR_DNLD_SW      ( -23 )  /* switch setup error                     */

#define ERR_DNLD_MEM     ( -24 )  /* could not allocate memory for download */
#define ERR_DNLD_FILE    ( -25 )  /* could not find the file to download    */
#define ERR_DNLD_TYPE    ( -26 )  /* the file is not downloadable           */

/* systat errors defined */

#define ERR_LIB_INCOMPAT ( -27 )  /* incompatible libary used               */
#define ERR_DRV_INCOMPAT ( -28 )  /* incompatible driver used (pre v4.0)    */
#define ERR_DRV_CALLINIT ( -29 )  /* another process attempted to call 'call_init' while other processing are accessing the driver */
#define ERR_TS_BLOCKED   ( -30 )  /* timeslot blocked */
#define ERR_NO_SYS_RES   ( -31 )  /* Used in NT_WOS code                    */

/* more download errors for VoIP */
#define ERR_INVALID_ADDR   ( -40 )  /* tried to access invalid address                */
#define ERR_INVALID_PORT   ( -41 )  /* tried to access invalid port                   */
#define ERR_MANAGEMENT_RPC ( -42 )  /* failed to startup Management RPC session       */
#define ERR_SESSION_RPC    ( -43 )  /* failed to startup Session RPC session          */
#define ERR_NO_SERVICE     ( -44 )  /* The service did not respond                    */
#define ERR_NO_BOARD       ( -45 )  /* The board did not respond                      */
#define ERR_BOARD_UNLOADED ( -46 )  /* The board has not been downloaded              */
#define ERR_BOARD_VERSION  ( -47 )  /* Board software incompatible with host software */

/*--------------------------------*/
/* clearing cause FROM the driver */
/*--------------------------------*/

#define LC_NORMAL                  0
#define LC_NUMBER_BUSY             1
#define LC_NO_ANSWER               2
#define LC_NUMBER_UNOBTAINABLE     3
#define LC_NUMBER_CHANGED          4
#define LC_OUT_OF_ORDER            5
#define LC_INCOMING_CALLS_BARRED   6
#define LC_CALL_REJECTED           7
#define LC_CALL_FAILED             8
#define LC_CHANNEL_BUSY            9
#define LC_NO_CHANNELS            10
#define LC_CONGESTION             11

/*--------------------------------*/
/* clearing cause FROM the driver */
/*--------------------------------*/

#define AC_NORMAL             0  /* default acceptance code      */
#define AC_CHARGE           100  /* answer call with charging    */
#define AC_NOCHARGE         101  /* answer call without charging */
#define AC_LAST_RELEASE     102  /* last party release           */
#define AC_SPARE1           103  /* spare                        */
#define AC_SPARE2           104  /* spare                        */

/*-------------------------------------*/
/* layer 1 stat values                 */
/*-------------------------------------*/

#define LSTAT_NOS    0x8000
#define LSTAT_LOS    0x4000
#define LSTAT_AIS    0x2000
#define LSTAT_FEC    0x1000
#define LSTAT_RRA    0x0800
#define LSTAT_SLP    0x0400
#define LSTAT_CVC    0x0200
#define LSTAT_CRC    0x0100
#define LSTAT_FFA    0x0080
#define LSTAT_CML    0x0040

/*-------------------------------------*/
/* layer 1 alarm values                */
/*-------------------------------------*/


#define ALARM_NONE   0x0000
#define ALARM_AIS    LSTAT_AIS
#define ALARM_RRA    LSTAT_RRA
#define ALARM_CML    LSTAT_CML

/*-------------------------------------*/
/* DSP Configurations                  */
/*-------------------------------------*/

#define DSPA 0x01    /* Rev4/5 Lucent DSP fitted */
#define DSPB 0x02

#define DSP1A 0x01   /* BR4/4e/8 Lucent DSP fitted */
#define DSP1B 0x02
#define DSP2A 0x04
#define DSP2B 0x08

/*-------------------------------------*/
/* DPR Memory Access Macros            */
/*-------------------------------------*/

/*-------------------------*/
/* UUS services/protocols  */
/*-------------------------*/
/* UU Service Request Types */
#define UUS_1_IMPLICITLY_PREFERRED      (1<<0)
#define UUS_1_REQUIRED                  (1<<1)
#define UUS_1_PREFERRED                 (1<<2)
#define UUS_2_REQUIRED                  (1<<3)
#define UUS_2_PREFERRED                 (1<<4)
#define UUS_3_REQUIRED                  (1<<5)
#define UUS_3_PREFERRED                 (1<<6)

/* UU Service Response Types */
#define UUS_1_ACCEPTED                  (1<<1)
#define UUS_1_REFUSED                   (1<<2)
#define UUS_1_REFUSED_BY_PEER           (1<<3)
#define UUS_1_REFUSED_BY_NET            (1<<4)
#define UUS_2_ACCEPTED                  (1<<5)
#define UUS_2_REFUSED                   (1<<6)
#define UUS_2_REFUSED_BY_PEER           (1<<7)
#define UUS_2_REFUSED_BY_NET            (1<<8)
#define UUS_3_ACCEPTED                  (1<<9)
#define UUS_3_REFUSED                   (1<<10)
#define UUS_3_REFUSED_BY_PEER           (1<<11)
#define UUS_3_REFUSED_BY_NET            (1<<12)

/* UU Protocol Types */
#define UUI_PROTOCOL_USER_SPECIFIC      0
#define UUI_PROTOCOL_OSI_HIGHER_LAYER   1
#define UUI_PROTOCOL_CCITT_X244         2
#define UUI_PROTOCOL_SYSMNG_CONV        3
#define UUI_PROTOCOL_IA5                4
#define UUI_PROTOCOL_CCITT_V120         7
#define UUI_PROTOCOL_CCITT_Q931         8


/* UU Flow Control  */
#define UUI_FC_STOP_SENDING             (1<<0)
#define UUI_FC_DATA_DISCARDED           (1<<1)

/* UU Commands  */
#define UU_SERVICE_CMD                  1
#define UU_DATA_CMD                     2
#define UU_GET_PENDING_DATA_CMD         3

#define UUI_CONTROL_CCM_DATA            (1<<0)

/* Settings for 'control' fields */
/* Info is sent immediately */
#define CONTROL_DEFAULT                 0
/* send in next ALERT, CONNECT, DISCONNECT, RELEASE message */
#define CONTROL_NEXT_CC_MESSAGE         1
/* More information will be sent in this message - don't transmit yet */
#define CONTROL_DEFERRED                2
#define CONTROL_DEFERRED_SETUP          3
/* extra information (after CONTROL_DEFERRED) - don't transmit yet */
#define CONTROL_EXTRA_INFO              4
#define CONTROL_EXTRA_INFO_SETUP        5
/* extra information (after CONTROL_DEFERRED/CONTROL_EXTRA_INFO)- send now */
#define CONTROL_LAST_INFO               6
#define CONTROL_LAST_INFO_SETUP         7

#define UUI_CONTROL_CCM_DATA            (1<<0)
/* Facility Commands For 'call_send_connectionless()'  */
#define FAC_CLESS_DL_DATA_CMD         1
#define FAC_CLESS_DL_UNIT_DATA_CMD    2
/* Facility Commands For proprietary transfer mechanism with Hicom switch
 * for use on EuroISDN with a Hicom switch (of course) - This is the
 * second stage of a two part operation
 * Part 1 - send keypad 'R' for Hold
 * Part 2 call_feature_send - facility - command= FAC_HICOM_TRANSFER -
 *  destination_addr = 'transfered to' number
 */
#define FAC_HICOM_TRANSFER            3

/* Commands for use with FEATURE_HOLD_XPARMS */
#define HOLD_ACKNOWLEDGE_CMD              1
#define HOLD_REJECT_CMD                   2
#define RECONNECT_ACKNOWLEDGE_CMD         3
#define RECONNECT_REJECT_CMD              4

/* Operation Types - See FEATURE_TRANSFER_STRUCT */
/* EuroISDN                                      */
#define OP_EXPLICIT_ECT_EXECUTE           1
#define OP_ECT_LINK_ID_REQUEST            4
#define OP_ECT_EXECUTE                    7

/* Operation Values */
#define INVOKE                            1
#define RETURN_RESULT                     2
#define RETURN_ERROR                      3

/*-------------------------------------*/
/* unpublished private structures      */
/*-------------------------------------*/

typedef struct api_register {
    ACU_LONG   signaturea;
    ACU_LONG   libversion;
    ACU_LONG   structsize;
    ACU_LONG   signatureb;
} ACU_PACK_DIRECTIVE API_REGISTER;

/* Command modes for the associate net interface */
#define ACUC_ASSOC_SET          0x11  /* Set database entry */
#define ACUC_ASSOC_LOCATE       0x12  /* Locate signalling net from user net */
#define ACUC_ASSOC_CONF         0x100 /* Add/modify NFAS-style net */
#define ACUC_ASSOC_CLEAR        0x101 /* Clear/remove NFAS-style net */
#define ACUC_ASSOC_QUERY        0x102 /* Query NFAS net configuration */

/* Flag definitions for associate net interface */
#define ACUC_ASSOC_LOCAL	(1<<0)
#define ACUC_RANDOM_SELECT	(1<<1)
#define ACUC_ASSOC_OFF_CARD	(1<<2)

typedef struct handle_2_port_xparms {
  API_REGISTER api_reg;
  ACU_INT      handle;           /* Handle, supplied by caller */
  int          port;             /* Port, returned by driver */
} ACU_PACK_DIRECTIVE HANDLE_2_PORT_XPARMS;


#ifdef ACUC_CLONED

/* Event interface command types... */
#define ACUC_EVENT_ALLOC_EMID          0x100
#define ACUC_EVENT_SET_EMID            0x101
#define ACUC_EVENT_POLL_GLOBAL         0x102
#define ACUC_EVENT_POLL_SPECIFIC       0x103
#define ACUC_EVENT_ATTACH_EMID         0x201
#define ACUC_EVENT_DETACH_EMID         0x202
#define ACUC_EVENT_SIG0                0x301
#define ACUC_EVENT_CLR0                0x302

typedef struct acuc_event_if_xparms {
    API_REGISTER  api_reg;
    ACU_INT       cmd;
    ACU_INT       emid;
    ACU_INT       cnum;
    ACU_INT       handle;
    ACU_INT       timeout;
    ACU_INT       state;
    ACU_INT       extended_state ;
} ACU_PACK_DIRECTIVE ACUC_EVENT_IF_XPARMS;
#endif

typedef struct acuc_assoc_net_xparms {
    API_REGISTER  api_reg;
    ACU_INT       net;            /* Signalling network port */
    ACU_INT       mode;           /* Command - add/remove/query etc. */
    ACU_INT       unet;           /* User-view of bearer network port */
    ACU_INT       flags;
    ACU_LONG      umask;          /* Mask of valid bearer timeslots */
    ACU_LONG      smask;          /* Mask of signalling timeslots */
    ACU_INT       bnet;           /* Driver-view of bearer network port */

    union
        {
        struct
           {
           ACU_INT    slc;        /* Link code for signalling link (if any) */
           ACU_ULONG  opc;        /* Own point code */
           ACU_ULONG  dpc;        /* Destination point code */
           ACU_ULONG  base_cic;   /* Base Circuit ID code */
           ACU_INT    selection_method;
           ACU_INT    ni;         /* Network indicator */
           } ACU_PACK_DIRECTIVE sig_isup ;
      } unique_xparms ;

}ACU_PACK_DIRECTIVE ACUC_ASSOC_NET_XPARMS ;

typedef struct acuc_maintenance_xparms {
  /* This structure is used internally by the library, not directly by applications */
  API_REGISTER api_reg ;
  ACU_INT      net;       /* Net for signalling protocol */
  ACU_INT      unet;      /* Net for affected channels */
  ACU_INT      ts;        /* Timeslot for ts-oriented calls */
  ACU_INT      cmd;       /* Command type */
  ACU_INT      flags;
  ACU_INT      reserved ;
  ACU_INT      type;      /* Protocol-specific qualifier to cmd */
  ACU_ULONG    ts_mask;   /* Mask for group-oriented calls */
} ACU_PACK_DIRECTIVE ACUC_MAINTENANCE_XPARMS ;


/* Generic Commands valid for acuc_maintenance_xparms */
#define ACUC_MAINT_TS_BLOCK        0x0001
#define ACUC_MAINT_TS_UNBLOCK      0x0002
#define ACUC_MAINT_PORT_BLOCK      0x0101
#define ACUC_MAINT_PORT_UNBLOCK    0x0102
#define ACUC_MAINT_PORT_RESET      0x0202

/* Generic flag settings valid for acuc_maintenance_xparms */
#define ACUC_MAINT_SYNC            0x80  /* Command is to execute synchronously */

/*
 * VoIP specific data structures for Aculab use only
 */

struct acu_vcc_thread_id {
  ACU_UINT  id;
  ACU_UINT  handle;
};
typedef struct acu_vcc_thread_id ACU_VCC_THREAD_ID;


typedef struct tsinfo_xparms {
               API_REGISTER api_reg ;
               ACU_INT      net ;
               ACU_INT      modify ;
               ACU_LONG     validvector;
               ACU_LONG     signalvector;
} ACU_PACK_DIRECTIVE TSINFO_XPARMS;


typedef struct {
               API_REGISTER api_reg;
               ACU_LONG     timeout;
               ACU_INT      valid;
               ACU_INT      card;
               ACU_INT      net;
               ACU_INT      ch;
               ACU_INT      io;
               union
                  {
                  struct
                     {
                     ACU_VCC_THREAD_ID calling_thread;
                     } sig_h323;
                  } ACU_PACK_DIRECTIVE unique_xparms;
               } ACU_PACK_DIRECTIVE LEQ;


typedef struct dc {
                  ACU_INT  result;
                  ACU_INT  card;
                  ACU_INT  handle;
} ACU_PACK_DIRECTIVE  DC;


typedef union control {
                      struct alarm_xparms * alarm_xparms;
                      struct q921_xparms  * q921_xparms;
                      struct l1_xstats    * l1_xstate;
                      struct l2_xstate    * l2_xstate;
                      struct br_l1_xstats * br_l1_xstate;
                      struct br_l2_xstate * br_l2_xstate;
} ACU_PACK_DIRECTIVE  CONTROL;


typedef struct sfmw_xparms {
                           API_REGISTER api_reg;
                           ACU_INT      net;
                           BUFP         bufp_confstr;               /* V4 version */
                           ACU_INT      ss_type;
                           ACU_INT      regapp;
                           ACU_INT      ss_line;
                           ACU_INT      ss_vers;
                           char         confstr[CONFSTRSIZE];       /* V5 version */
                           union
                              {
                              struct
                                 {
                                 ACU_VCC_THREAD_ID calling_thread;
                                 } sig_h323;
                           } ACU_PACK_DIRECTIVE unique_xparms;
} ACU_PACK_DIRECTIVE  SFMW_XPARMS;


typedef struct bearer {
                      ACU_UCHAR ie[MAXBEARER];
                      ACU_UCHAR last_msg;
} ACU_PACK_DIRECTIVE  BEARER;


typedef struct hilayer {
                       ACU_UCHAR ie[MAXHILAYER];
                       ACU_UCHAR last_msg;
} ACU_PACK_DIRECTIVE  HILAYER;

typedef struct lolayer {
                       ACU_UCHAR ie[MAXLOLAYER];
                       ACU_UCHAR last_msg;
} ACU_PACK_DIRECTIVE  LOLAYER;

typedef struct progress_indicator {
                                  ACU_UCHAR ie[MAXPROGRESS];
                                  ACU_UCHAR last_msg;
} ACU_PACK_DIRECTIVE  PROGRESS_INDICATOR;


typedef struct notify_indicator {
                                ACU_UCHAR ie[MAXNOTIFY];
                                ACU_UCHAR last_msg;
} ACU_PACK_DIRECTIVE  NOTIFY_INDICATOR;

typedef struct keypad {
                      ACU_UCHAR  ie[MAXNUM];
                      ACU_UCHAR last_msg;
} ACU_PACK_DIRECTIVE  KEYPAD;

typedef struct display {
                       ACU_UCHAR ie[MAXDISPLAY];
                       ACU_UCHAR last_msg;
} ACU_PACK_DIRECTIVE  DISPLAY;

typedef struct ep {               /* End point addressing */
                  ACU_UCHAR  usid;
                  ACU_UCHAR  tid;
                  ACU_UCHAR  interpreter;
} ACU_PACK_DIRECTIVE  EP;



/*-------------------------------------*/
/* API structure definitions           */
/*-------------------------------------*/


typedef struct uniquex_q931 {
                            ACU_UCHAR  service_octet;
                            ACU_UCHAR  add_info_octet;
                            ACU_UCHAR  dest_numbering_type;
                            ACU_UCHAR  dest_numbering_plan;
                            BEARER     bearer;              /* V4 ends here */

                            ACU_UCHAR  orig_numbering_type;
                            ACU_UCHAR  orig_numbering_plan;
                            ACU_UCHAR  orig_numbering_presentation;
                            ACU_UCHAR  orig_numbering_screening;
                            ACU_UCHAR  conn_numbering_type;
                            ACU_UCHAR  conn_numbering_plan;
                            ACU_UCHAR  conn_numbering_presentation;
                            ACU_UCHAR  conn_numbering_screening;

                            ACU_UCHAR  dest_subaddr[MAXNUM];
                            ACU_UCHAR  orig_subaddr[MAXNUM];

                            HILAYER            hilayer;
                            LOLAYER            lolayer;
                            PROGRESS_INDICATOR progress_indicator;
                            NOTIFY_INDICATOR   notify_indicator;
                            KEYPAD             keypad;
                            DISPLAY            display;

                            ACU_LONG           slotmap;

                            EP                 endpoint_id;

} ACU_PACK_DIRECTIVE  UNIQUEX_Q931;


typedef struct uniquex_1tr6 {
                            ACU_UCHAR  service_octet;
                            ACU_UCHAR  add_info_octet;
                            ACU_UCHAR  numbering_type;
                            ACU_UCHAR  numbering_plan;
} ACU_PACK_DIRECTIVE  UNIQUEX_1TR6;


typedef struct uniquex_dass {
                            ACU_UCHAR  sic1;
                            ACU_UCHAR  sic2;
} ACU_PACK_DIRECTIVE  UNIQUEX_DASS;

typedef struct uniquex_dpnss {
                             ACU_UCHAR  sic1;
                             ACU_UCHAR  sic2;
                             ACU_UCHAR  clc[MAXCLC];
} ACU_PACK_DIRECTIVE  UNIQUEX_DPNSS;

typedef struct uniquex_cas {
                           ACU_UCHAR  category;
} ACU_PACK_DIRECTIVE  UNIQUEX_CAS;


/*--------------------------------------------------------*/
/* SS7 ISUP Rate Unique Structure                         */
/*--------------------------------------------------------*/
typedef struct uniquex_isup   {
                            ACU_UCHAR          service_octet;
                            ACU_UCHAR          add_info_octet;
                            ACU_UCHAR          dest_natureof_addr;
                            ACU_UCHAR          dest_numbering_plan;
                            BEARER             bearer;
                            ACU_UCHAR          orig_natureof_addr;
                            ACU_UCHAR          orig_numbering_plan;
                            ACU_UCHAR          orig_numbering_presentation;
                            ACU_UCHAR          orig_numbering_screening;
                            ACU_UCHAR          conn_natureof_addr;
                            ACU_UCHAR          conn_numbering_plan;
                            ACU_UCHAR          conn_numbering_presentation;
                            ACU_UCHAR          conn_numbering_screening;
                            ACU_UCHAR          conn_number_req ;
                            ACU_UCHAR          orig_category;
                            ACU_UCHAR          orig_number_incomplete;

                            ACU_UCHAR          dest_subaddr[MAXNUM];
                            ACU_UCHAR          orig_subaddr[MAXNUM];

                            HILAYER            hilayer;
                            LOLAYER            lolayer;
                            PROGRESS_INDICATOR progress_indicator;

                            ACU_UCHAR          in_band ;

                            ACU_UCHAR          nat_inter_call_ind;
                            ACU_UCHAR          interworking_ind;
                            ACU_UCHAR          isdn_userpart_ind;
                            ACU_UCHAR          isdn_userpart_pref_ind;
                            ACU_UCHAR          isdn_access_ind;
                            ACU_UCHAR          dest_int_nw_ind;

                            ACU_UCHAR          continuity_check_ind ;
                            ACU_UCHAR          satellite_ind ;
                            ACU_UCHAR          charge_ind ;
} ACU_PACK_DIRECTIVE  UNIQUEX_ISUP;


/*--------------------------------------------------------*/
/* VoIP Unique Structure                                  */
/*--------------------------------------------------------*/

typedef struct uniquex_h323 {
                            ACU_VCC_THREAD_ID  calling_thread;
                            ACU_ULONG          destination_addr; /* only used for IP address not E164 */
                            ACU_ULONG          originating_addr; /* as above                          */
                            ACU_INT            request_admission;
                            ACU_INT            tdm_encoding;
                            ACU_INT            encode_gain;
                            ACU_INT            decode_gain;
                            ACU_INT            voice_activity_det;
                            ACU_INT            codecs[MAXCODECS];
                            ACU_CHAR           display[MAXDISPLAY];
                            ACU_ULONG          gk_addr;
                            ACU_INT            endpoint_identifier_length;
                            ACU_USHORT         endpoint_identifier[128];
} ACU_PACK_DIRECTIVE UNIQUEX_H323;


/*--------------------------------------------------------*/
/* Union of Unique structures passed to the driver        */
/*--------------------------------------------------------*/

typedef union uniquex {
       UNIQUEX_1TR6      sig_1tr6;
       UNIQUEX_DASS      sig_dass;
       UNIQUEX_DPNSS     sig_dpnss;
       UNIQUEX_CAS       sig_cas;
       UNIQUEX_Q931      sig_q931;
       UNIQUEX_ISUP      sig_isup;
       UNIQUEX_H323      sig_h323;
} ACU_PACK_DIRECTIVE  UNIQUEXU;

/* open for outgoing structure */

typedef struct out_xparms {
                          API_REGISTER  api_reg;
                          ACU_INT       handle;
                          ACU_INT       net;
                          ACU_INT       ts;
                          ACU_INT       cnf;
                          ACU_INT       sending_complete;
                          char          destination_addr[MAXNUM];
                          char          originating_addr[MAXNUM];
                          union         uniquex unique_xparms;
                          ACU_ACT       app_context_token;
} ACU_PACK_DIRECTIVE  OUT_XPARMS;


/* call details structure */


typedef struct detail_xparms {
                             API_REGISTER  api_reg;
                             ACU_INT       handle;
                             ACU_LONG      timeout;
                             ACU_INT       valid;
                             ACU_INT       stream;
                             ACU_INT       ts;
                             ACU_INT       calltype;
                             ACU_INT       sending_complete;
                             char          destination_addr[MAXNUM];
                             char          originating_addr[MAXNUM];
                             char          connected_addr[MAXNUM];
                             ACU_ULONG     feature_information;                     /* This has been changed into an ACU_LONG from an ACU_UCHAR - we need to use some of the following spare bytes */
                             ACU_UCHAR     spare[(MAXNUM-1)-(sizeof(ACU_LONG)-1)];  /* 32-1=31 - the number of bytes needed to turn an ACU_UCHAR into an ACU_LONG (OS and compiler dependent) */
                             union uniquex unique_xparms;
                             ACU_ACT       app_context_token;
} ACU_PACK_DIRECTIVE  DETAIL_XPARMS;

typedef struct init_xparms {
                           API_REGISTER api_reg;
                           ACU_INT      nnets;
                           char         ournum[MAXNUM];
                           ACU_LONG     responsetime;
                           char         board_ip_address[32];
                           union
                              {
                              struct
                                 {
                                 ACU_VCC_THREAD_ID calling_thread;
                                 } sig_h323;
                           } ACU_PACK_DIRECTIVE unique_xparms;
} ACU_PACK_DIRECTIVE  INIT_XPARMS;


/* signalling information parameters */

typedef struct siginfo_xparms {
                              API_REGISTER api_reg;
                              ACU_INT      nnets;
                              ACU_LONG     validvector;
                              char         sigsys[MAXSIGSYS];
                              union
                                 {
                                 struct
                                    {
                                    ACU_VCC_THREAD_ID calling_thread;
                                    } sig_h323;
                                 } ACU_PACK_DIRECTIVE unique_xparms;
} ACU_PACK_DIRECTIVE  SIGINFO_XPARMS;


/* system information parameters */

typedef struct sysinfo_xparms {
                              API_REGISTER api_reg;
                              ACU_INT      net;
                              ACU_INT      port_address;
                              ACU_INT      intr_vect;
                              ACU_INT      mem_address;
                              ACU_INT      nphys;
                              ACU_INT      phys_port;
                              ACU_INT      irqtick;
                              ACU_INT      clocktick;
                              ACU_INT      cardtype;
                              ACU_INT      limtype;
                              ACU_CHAR     serialnumber[MAXSERIALNO];
                              ACU_CHAR     hw_version[MAXHWVERSION];
                              ACU_INT      boardtype;
                              union
                                 {
                                 struct
                                    {
                                    ACU_VCC_THREAD_ID calling_thread;
                                    } sig_h323;
                              } ACU_PACK_DIRECTIVE unique_xparms;
} ACU_PACK_DIRECTIVE  SYSINFO_XPARMS;


/* set system information parameters */

typedef struct set_sysinfo_xparms {
                              API_REGISTER api_reg;
                              ACU_INT      net;
                              char board_ip_address[16];
                              int board_number;
                              union
                                 {
                                 struct
                                    {
                                    ACU_VCC_THREAD_ID calling_thread;
                                    } sig_h323;
                                 } ACU_PACK_DIRECTIVE unique_xparms;
} ACU_PACK_DIRECTIVE SET_SYSINFO_XPARMS;


/* overlap structure */

typedef struct overlap_xparms {
                              API_REGISTER api_reg;
                              ACU_INT      handle;
                              ACU_INT      sending_complete;
                              char         destination_addr[MAXNUM];
} ACU_PACK_DIRECTIVE  OVERLAP_XPARMS;

/* open for incoming structure */

typedef struct in_xparms {
                         API_REGISTER api_reg;
                         ACU_INT      handle;
                         ACU_INT      net;
                         ACU_INT      ts;
                         ACU_INT      cnf;
                         union        uniquex unique_xparms;
                         ACU_ACT      app_context_token;
} ACU_PACK_DIRECTIVE  IN_XPARMS;


/* call state structure */

typedef struct state_xparms {
                            API_REGISTER api_reg;
                            ACU_INT      handle;
                            ACU_LONG     state;
                            ACU_LONG     timeout;
                            ACU_LONG     extended_state;
                            union
                               {
                               struct
                                  {
                                  ACU_VCC_THREAD_ID calling_thread;
                                  } sig_h323;
                            } ACU_PACK_DIRECTIVE unique_xparms;
                            ACU_ACT      app_context_token;
} ACU_PACK_DIRECTIVE  STATE_XPARMS;


/* call charge structure */

typedef struct get_charge_xparms {
                                 API_REGISTER api_reg;
                                 ACU_INT      handle;
                                 ACU_INT      type;
                                 char         charge[CHARGEMAX];
                                 ACU_UINT     meter;
}  ACU_PACK_DIRECTIVE GET_CHARGE_XPARMS;

typedef struct put_charge_xparms {
                                 API_REGISTER api_reg;
                                 ACU_INT      handle;
                                 char         charge[CHARGEMAX];
} ACU_PACK_DIRECTIVE PUT_CHARGE_XPARMS;

/* disconnect cause structure */

typedef struct cause_xparms {
                            API_REGISTER api_reg;
                            ACU_INT      handle;
                            ACU_INT      cause;
                            ACU_INT      raw;
                            } ACU_PACK_DIRECTIVE CAUSE_XPARMS;

/* call transfer structure */

typedef struct transfer_xparms {
                               API_REGISTER api_reg;
                               ACU_INT      handlea;
                               ACU_INT      handlec;
                               union
                                  {
                                  struct
                                     {
                                     ACU_VCC_THREAD_ID calling_thread;
                                     } sig_h323;
                                  } ACU_PACK_DIRECTIVE unique_xparms;
} ACU_PACK_DIRECTIVE TRANSFER_XPARMS;

/* command string to the MVIP */

typedef struct tcmd_xparms {
                           API_REGISTER api_reg;
                           ACU_INT      net;
                           char    str[48];
                           } ACU_PACK_DIRECTIVE TCMD_XPARMS;


/* V4 driver version of sending a program block to the MVIP */

typedef struct v4_pblock_xparms {
    API_REGISTER api_reg;
    ACU_INT      net;
    ACU_INT      len;
    BUFP         datap;
} ACU_PACK_DIRECTIVE V4_PBLOCK_XPARMS;



/* send a program block to the MVIP */

typedef struct v5_pblock_xparms {
                            API_REGISTER api_reg;
                            ACU_INT      command;
                            ACU_INT      error;
                            ACU_INT      spare;
                            ACU_INT      net;
                            ACU_INT      len;
                            char         datap[XFERSIZE];
                            } ACU_PACK_DIRECTIVE V5_PBLOCK_XPARMS;

/* progress structure */


typedef struct progress_xparms {
                               API_REGISTER api_reg;
                               ACU_INT      handle;
                               union
                                   {
                                   struct
                                      {
                                      PROGRESS_INDICATOR    progress_indicator;
                                      DISPLAY               display;
                                      } sig_q931;
                                   struct
                                      {
                                      PROGRESS_INDICATOR    progress_indicator;
                                      ACU_UCHAR             in_band;
                                      ACU_UCHAR             charge_ind ;
                                      } sig_isup;
                                   struct
                                      {
                                      ACU_VCC_THREAD_ID     calling_thread;
                                      DISPLAY               display;
                                      } sig_h323;
                                   }  ACU_PACK_DIRECTIVE unique_xparms;
                               } ACU_PACK_DIRECTIVE PROGRESS_XPARMS;


/* proceeding structure */

typedef struct proceeding_xparms {
                               API_REGISTER                api_reg;
                               ACU_INT                     handle;
                               union
                                  {
                                  struct
                                     {
                                     PROGRESS_INDICATOR    progress_indicator;
                                     DISPLAY               display;
                                     NOTIFY_INDICATOR      notify_indicator;
                                     } sig_q931;
                                  struct
                                     {
                                     PROGRESS_INDICATOR    progress_indicator;
                                     ACU_UCHAR             in_band;
                                     ACU_UCHAR             charge_ind ;
                                  } sig_isup;
                                  UNIQUEX_H323             sig_h323;
                                  } ACU_PACK_DIRECTIVE unique_xparms;
                               } ACU_PACK_DIRECTIVE PROCEEDING_XPARMS;

/* setup_ack structure */

typedef struct setup_ack_xparms {
                                API_REGISTER          api_reg;
                                ACU_INT               handle;
                                union
                                   {
                                   struct
                                      {
                                      PROGRESS_INDICATOR    progress_indicator;
                                      DISPLAY               display;
                                      } sig_q931;
                                   struct
                                      {
                                      ACU_VCC_THREAD_ID calling_thread;
                                      } sig_h323;
                                   } ACU_PACK_DIRECTIVE unique_xparms;
                               } ACU_PACK_DIRECTIVE SETUP_ACK_XPARMS;

/* notify structure */

typedef struct notify_xparms {
                             API_REGISTER            api_reg;
                             ACU_INT                 handle;
                             union {
                                 struct
                                    {
                                    NOTIFY_INDICATOR        notify_indicator;
                                    } sig_q931;
                                 } ACU_PACK_DIRECTIVE unique_xparms;
                             } ACU_PACK_DIRECTIVE NOTIFY_XPARMS;

/* keypad structure */

typedef struct keypad_xparms {
                             API_REGISTER            api_reg;
                             ACU_INT                 handle;
                             union {
                                 struct
                                    {
                                    ACU_INT          net;
                                    ACU_INT          device;
                                    KEYPAD           keypad;
                                    ACU_INT          location;
                                    DISPLAY          display;
                                    } sig_q931;
                                 struct
                                    {
                                    ACU_VCC_THREAD_ID calling_thread;
                                    KEYPAD            keypad;
                                    } sig_h323;
                                 } ACU_PACK_DIRECTIVE unique_xparms;
                             } ACU_PACK_DIRECTIVE KEYPAD_XPARMS;

/* disconnect structure */

typedef struct disconnect_xparms {
                                 API_REGISTER        api_reg;
                                 ACU_INT             handle;
                                 ACU_INT             cause;
                                 union
                                    {
                                    struct
                                       {
                                       ACU_INT             raw;
                                       PROGRESS_INDICATOR  progress_indicator;
                                       DISPLAY             display;
                                       NOTIFY_INDICATOR	   notify_indicator;
                                       } sig_q931;
                                    struct
                                       {
                                       ACU_INT             raw;
                                       PROGRESS_INDICATOR  progress_indicator;
                                       ACU_INT             location;
                                       ACU_INT             reattempt;
                                       } sig_isup;
                                    struct
                                       {
                                       ACU_INT             raw;
                                       } sig_1tr6;
                                    struct
                                       {
                                       ACU_INT             raw;
                                       } sig_dass;
                                    struct
                                       {
                                       ACU_INT             raw;
                                       } sig_dpnss;
                                    struct
                                       {
                                       ACU_INT             raw;
                                       } sig_cas;
                                    struct
                                       {
                                       ACU_INT             raw;
                                       ACU_VCC_THREAD_ID   calling_thread;
                                       } sig_h323;
                                    } ACU_PACK_DIRECTIVE unique_xparms;
                                 } ACU_PACK_DIRECTIVE DISCONNECT_XPARMS;

/* accept structure */

typedef struct accept_xparms {
                             API_REGISTER            api_reg;
                             ACU_INT                 handle;
                             union
                                {
                                struct
                                   {
                                   PROGRESS_INDICATOR      progress_indicator;
                                   LOLAYER                 lolayer;
                                   DISPLAY                 display;
                                   char                    connected_addr[MAXNUM];
                                   ACU_UCHAR               conn_numbering_type;
                                   ACU_UCHAR               conn_numbering_plan;
                                   ACU_UCHAR               conn_numbering_presentation;
                                   ACU_UCHAR               conn_numbering_screening;
                                   } sig_q931;
                                struct
                                   {
                                   PROGRESS_INDICATOR      progress_indicator;
                                   LOLAYER                 lolayer;
                                   char                    connected_addr[MAXNUM];
                                   ACU_UCHAR               conn_natureof_addr;
                                   ACU_UCHAR               conn_numbering_plan;
                                   ACU_UCHAR               conn_numbering_presentation;
                                   ACU_UCHAR               conn_numbering_screening;
                                   ACU_UCHAR               charge_ind;
                                   } sig_isup;
                                   UNIQUEX_H323            sig_h323;
                                } ACU_PACK_DIRECTIVE unique_xparms;
                             } ACU_PACK_DIRECTIVE ACCEPT_XPARMS;


/* incoming_ringing structure */

typedef struct incoming_ringing_xparms {
                                       API_REGISTER        api_reg;
                                       ACU_INT             handle;
                                       union
                                          {
                                          struct
                                             {
                                             PROGRESS_INDICATOR    progress_indicator;
                                             DISPLAY               display;
                                             } sig_q931;
                                          struct
                                             {
                                             PROGRESS_INDICATOR    progress_indicator;
                                             ACU_UCHAR             charge_ind ;
                                             ACU_UCHAR             in_band;
                                             } sig_isup;
                                          UNIQUEX_H323             sig_h323;
                                          } ACU_PACK_DIRECTIVE unique_xparms;
                                       } ACU_PACK_DIRECTIVE INCOMING_RINGING_XPARMS;

/* hold structure */

typedef struct hold_xparms {
                           API_REGISTER api_reg;
                           ACU_INT      handle;
                           union
                               {
                               struct
                                   {
                                   ACU_VCC_THREAD_ID calling_thread;
                                   } sig_h323;
                               } ACU_PACK_DIRECTIVE unique_xparms;
                           } ACU_PACK_DIRECTIVE HOLD_XPARMS;

typedef struct voip_files_xparms {
                           API_REGISTER api_reg;
                           ACU_INT      handle;
                           char         bootfile[80];
                           char         codefile[80];
                           } VOIP_FILES_XPARMS;

/* for voip startup */
typedef struct voip_stats_xparms {
                           API_REGISTER api_reg;
                           ACU_INT      handle;
                           } VOIP_STATS_XPARMS;

/* for voip startup */
typedef struct voip_vers_xparms {
                           API_REGISTER api_reg;
                           ACU_INT      handle;
                           } VOIP_VERS_XPARMS;

/* get_originating_addr structure */

typedef struct get_originating_addr_xparms {
                           API_REGISTER api_reg;
                           ACU_INT      handle;
                           } ACU_PACK_DIRECTIVE GET_ORIGINATING_ADDR_XPARMS;


/* trace structure */

typedef struct trace_xparms {
                           API_REGISTER api_reg;
                           ACU_INT      handle;
                           ACU_INT      card;
                           ACU_INT      unet;
                           ACU_INT      traceflag;
                           } ACU_PACK_DIRECTIVE TRACE_XPARMS;


/* Port (Group) reset */

typedef struct port_reset_xparms {
                           API_REGISTER api_reg ;
                           ACU_INT      net ;
                           ACU_INT      flags ;
                           ACU_INT      reserved ;
                           union
                              {
                              ACU_ULONG      ts_mask ;
                              } ACU_PACK_DIRECTIVE unique_xparms;
} ACU_PACK_DIRECTIVE PORT_RESET_XPARMS ;


/* Port (Group) blocking/unblocking */

typedef struct port_blocking_xparms {
                           API_REGISTER api_reg ;
                           ACU_INT      net ;
                           ACU_INT      flags ;
                           ACU_INT      type ;
                           ACU_INT      reserved ;
                           union
                              {
                              ACU_ULONG      ts_mask ;
                              } ACU_PACK_DIRECTIVE unique_xparms;
} ACU_PACK_DIRECTIVE PORT_BLOCKING_XPARMS ;


/* Timeslot blocking/unblocking */

typedef struct ts_blocking_xparms {
                           API_REGISTER api_reg ;
                           ACU_INT      net ;
                           ACU_INT      ts ;
                           ACU_INT      flags ;
} ACU_PACK_DIRECTIVE TS_BLOCKING_XPARMS ;



/*----------------------------------------------*/
/* primary rate layer 1 and layer 2 information */
/*----------------------------------------------*/

/* these can be read and reset */

typedef struct getset {
                      ACU_INT   linestat;
                      ACU_INT   bipvios;
                      ACU_INT   faserrs;
                      ACU_INT   sliperrs;
                      }ACU_PACK_DIRECTIVE GETSET;

/* these can be read only */

typedef struct get {
                   ACU_UCHAR    nos;
                   ACU_UCHAR    ais;
                   ACU_UCHAR    los;
                   ACU_UCHAR    rra;
                   ACU_UCHAR    tra;
                   ACU_UCHAR    rma;
                   ACU_UCHAR    tma;
                   ACU_UCHAR    usr;
                   ACU_UCHAR    majorrev;
                   ACU_UCHAR    minorrev;
                   ACU_LONG     clock;
                   char         buildstr[16];
                   char         manstr[16];
                   char         sigstr[16];
                   } ACU_PACK_DIRECTIVE GET;

typedef struct l1_xstats {
                         API_REGISTER api_reg;
                         ACU_INT      net;
                         GETSET       getset;
                         GET          get;
                         } ACU_PACK_DIRECTIVE L1_XSTATS;

typedef struct l2_xstate {
                         API_REGISTER api_reg;
                         ACU_INT      net;
                         ACU_LONG     state;
                         } ACU_PACK_DIRECTIVE L2_XSTATE;


typedef struct alarm_xparms {
                            API_REGISTER api_reg;
                            ACU_INT      net;
                            ACU_INT      alarm;
                            } ACU_PACK_DIRECTIVE ALARM_XPARMS;

typedef struct q921_xparms {
                            API_REGISTER api_reg;
                            ACU_INT      net;
                            ACU_INT      primitive;
                            ACU_LONG     timeout;
                            ACU_INT      layer3_length;
                            ACU_INT      layer3[MAXL3LENGTH];
                            } ACU_PACK_DIRECTIVE Q921_XPARMS;

typedef struct watchdog_xparms {
                              API_REGISTER api_reg;
                              ACU_INT      net;
                              ACU_INT      alarm;
                              ACU_LONG     timeout;
                              } ACU_PACK_DIRECTIVE WATCHDOG_XPARMS;

typedef struct dcba_xparms {
                           API_REGISTER   api_reg;
                           ACU_INT        net;
                           ACU_UCHAR      tdcba[16];
                           ACU_UCHAR      rdcba[16];
                           } ACU_PACK_DIRECTIVE DCBA_XPARMS;

typedef struct log {
                   ACU_LONG      TimeStamp;
                   ACU_UCHAR     RxTx;
                   ACU_UCHAR     Data_Packet [75];
                   } ACU_PACK_DIRECTIVE LOG;

typedef struct log_xparms {
                          API_REGISTER api_reg;
                          ACU_INT     net;
                          LOG          log;
                          } ACU_PACK_DIRECTIVE LOG_XPARMS;


/*-----------------------------------*/
/* DPNSS Enhanced Feature Structures */
/*-----------------------------------*/

typedef struct feature_xparms {
                              ACU_INT    msg[MAX_FEAT_MSG];     /* Feature information message */
                              ACU_UCHAR  call_type;             /* Call type - real or virtual */
                              char       digits[MAXNUM];        /* Feature digits              */
                              char       cli [MAXNUM];          /* Called Line Identity        */
                              char       nsi[MAXNSI];           /* Non Specified Information   */
                              char       txt[MAXTXT];           /* Text                        */
                              char       tid[MAXTID];           /* Trunk ID                    */
                              ACU_UCHAR  clc;                   /* Call/Called Line category   */
                              ACU_UCHAR  held_clc;              /* Held Calling Line Category  */
                              ACU_UCHAR  ipl;                   /* Intrusion protection level  */
                              ACU_UCHAR  icl;                   /* Intrusion capability level  */
                              } ACU_PACK_DIRECTIVE FEATURE_XPARMS;



typedef struct dpns_out_xparms {
                              API_REGISTER    api_reg;
                              ACU_INT         handle;
                              ACU_INT         net;
                              ACU_INT         ts;
                              ACU_INT         cnf;
                              ACU_INT         sending_complete;
                              char            destination_addr[MAXNUM];
                              char            originating_addr[MAXNUM];
                              union  uniquex  unique_xparms;
                              FEATURE_XPARMS  feature_info;
                              } ACU_PACK_DIRECTIVE DPNS_OUT_XPARMS;


typedef struct dpns_overlap_xparms {
                             API_REGISTER     api_reg;
                             ACU_INT          handle;
                             ACU_INT          sending_complete;
                             char             destination_addr[MAXNUM];
                             FEATURE_XPARMS   feature_info;
                             } ACU_PACK_DIRECTIVE DPNS_OVERLAP_XPARMS;




typedef struct dpns_feature_xparms {
                                   API_REGISTER    api_reg;
                                   ACU_INT         handle;
                                   FEATURE_XPARMS  feature_info;
                                   } ACU_PACK_DIRECTIVE DPNS_FEATURE_XPARMS;


typedef struct dpns_incoming_ring_xparms {
                                         API_REGISTER    api_reg;
                                         ACU_INT         handle;
                                         FEATURE_XPARMS  feature_info;
                                         } ACU_PACK_DIRECTIVE DPNS_INCOMING_RING_XPARMS;


typedef struct dpns_call_accept_xparms {
                                       API_REGISTER      api_reg;
                                       ACU_INT           handle;
                                       FEATURE_XPARMS    feature_info;
                                       } ACU_PACK_DIRECTIVE DPNS_CALL_ACCEPT_XPARMS;


typedef struct dpns_detail_xparms {
                           API_REGISTER   api_reg;
                           ACU_INT        handle;
                           ACU_LONG       timeout;
                           ACU_INT        valid;
                           ACU_INT        stream;
                           ACU_INT        ts;
                           ACU_INT        calltype;
                           ACU_INT        sending_complete;
                           char           destination_addr[MAXNUM];
                           char           originating_addr[MAXNUM];
                           char           connected_addr[MAXNUM];
                           char           redirected_addr[MAXNUM];
                           union  uniquex unique_xparms;
                           FEATURE_XPARMS feature_info;
                           } ACU_PACK_DIRECTIVE DPNS_DETAIL_XPARMS;



typedef struct dpns_transit_xparms {
                                   API_REGISTER api_reg;
                                   ACU_INT      handle;
                                   ACU_LONG     timeout;
                                   ACU_INT      valid;
                                   char         trans_msg[TRANSIT_MSG_LENGTH];
                                   } ACU_PACK_DIRECTIVE DPNS_TRANSIT_XPARMS;


typedef struct dpns_set_transit_xparms {
                                       API_REGISTER api_reg;
                                       ACU_INT      handle;
                                       } ACU_PACK_DIRECTIVE DPNS_SET_TRANSIT_XPARMS;


typedef struct dpns_cause_xparms {
                                 API_REGISTER     api_reg;
                                 ACU_INT          handle;
                                 ACU_INT          cause;
                                 ACU_INT          raw;
                                 FEATURE_XPARMS   feature_info;
                                 } ACU_PACK_DIRECTIVE DPNS_CAUSE_XPARMS;

typedef struct dpns_l2_xparms {
                              API_REGISTER  api_reg;
                              ACU_INT       net;
                              ACU_INT       channel;
                              ACU_UCHAR     state;
                              ACU_LONG      timeout;
                              } ACU_PACK_DIRECTIVE DPNS_L2_XPARMS;

typedef struct dpns_wd_xparms {
                              ACU_INT   net;
                              ACU_LONG  timeout;
                              } ACU_PACK_DIRECTIVE DPNS_WD_XPARMS;

/*-----------------------------------*/

/*-------------------------------*/
/* Enhanced Feature Structures   */
/*-------------------------------*/

typedef struct uui_xparms {
                  ACU_INT        command;
                  ACU_UINT       request;
                  ACU_INT        tx_response;
                  ACU_INT        rx_response;
                  char           control;
                  char           flow_control;
                  ACU_UCHAR      protocol;
                  char           more;
                  ACU_UCHAR      length;
                  ACU_UCHAR      data[MAXUUI_INFO];
              } ACU_PACK_DIRECTIVE UUI_XPARMS;

typedef struct facility_xparms {
                  ACU_INT        command;
                  ACU_UCHAR      control;
                  ACU_UCHAR      length;
                  ACU_UCHAR      data[MAXFACILITY_INFO];
                  /* a connectionless message may contain the following information */
                  char           destination_addr[MAXNUM];
                  char           originating_addr[MAXNUM];
                  ACU_UCHAR      dest_subaddr[MAXNUM];
                  ACU_UCHAR      dest_numbering_type;
                  ACU_UCHAR      dest_numbering_plan;
                  ACU_UCHAR      orig_numbering_type;
                  ACU_UCHAR      orig_numbering_plan;
                  ACU_UCHAR      orig_numbering_presentation;
                  ACU_UCHAR      orig_numbering_screening;
                  /* when multiple Facility information elements are present this
                   * indicates how many more are present
                   */
                  ACU_UCHAR      more;
              } ACU_PACK_DIRECTIVE FACILITY_XPARMS;

typedef struct diversion_xparms {
                  ACU_UCHAR      diverting_reason;
                  ACU_UCHAR      diverting_counter;
                  char           diverting_to_addr[MAXNUM];
                  char           diverting_from_addr[MAXNUM];
                  char           original_called_addr[MAXNUM];
                  ACU_UCHAR      diverting_from_type;
                  ACU_UCHAR      diverting_from_plan;
                  ACU_UCHAR      diverting_from_presentation;
                  ACU_UCHAR      diverting_from_screening;
                  ACU_UCHAR      diverting_indicator ;           /* supported only by ISUP drivers */
                  ACU_UCHAR      original_diverting_reason ;     /* supported only by ISUP drivers */
                  ACU_UCHAR      diverting_to_type;              /* supported only by ISUP drivers */
                  ACU_UCHAR      diverting_to_plan;              /* supported only by ISUP drivers */
                  ACU_UCHAR      diverting_to_int_nw_indicator;  /* supported only by ISUP drivers */
                  ACU_UCHAR      original_called_type;           /* supported only by ISUP drivers */
                  ACU_UCHAR      original_called_plan;           /* supported only by ISUP drivers */
                  ACU_UCHAR      original_called_presentation;   /* supported only by ISUP drivers */

              } ACU_PACK_DIRECTIVE DIVERSION_XPARMS;

typedef struct feature_hold_xparms {
                  ACU_INT        command;
                  ACU_INT        cause;
                  union
                  {
                      struct
                      {
                          /* RETRIEVE in ETS300 196 uses timeslot information*/
                          ACU_INT        ts;
                          ACU_INT        raw;
                          DISPLAY        display;
                      } sig_q931;
                  } ACU_PACK_DIRECTIVE unique_xparms;
              }ACU_PACK_DIRECTIVE  FEATURE_HOLD_XPARMS;

typedef struct feature_transfer_xparms {
                  char           control;
                  union
                  {
                      struct
                      {
                          ACU_INT        operation;
                          ACU_INT        operation_type;
                          ACU_INT        error;
                          union
                          {
                              struct
                              {
                                  ACU_INT        LinkID;
                              } ets;
                          } specific;
                      } sig_q931;
                  }ACU_PACK_DIRECTIVE  unique_xparms;
              } ACU_PACK_DIRECTIVE FEATURE_TRANSFER_XPARMS;

typedef struct raw_data_struct {
                  ACU_INT        length;
                  ACU_UCHAR      data[MAXRAWDATA];
                  ACU_UCHAR      more;
           } ACU_PACK_DIRECTIVE RAW_DATA_STRUCT;


typedef union feature_union {
                  UUI_XPARMS              uui;
                  FACILITY_XPARMS         facility;
                  DIVERSION_XPARMS        diversion;
                  FEATURE_HOLD_XPARMS     hold;
                  FEATURE_TRANSFER_XPARMS transfer;
                  RAW_DATA_STRUCT         raw_data;
              } ACU_PACK_DIRECTIVE FEATURE_UNION;


typedef struct feature_detail_xparms {
                  API_REGISTER           api_reg;
                  ACU_INT                handle;
                  ACU_INT                net;
                  ACU_ULONG              feature_type;
                  FEATURE_UNION          feature;
                  ACU_INT                message_control;
              } ACU_PACK_DIRECTIVE FEATURE_DETAIL_XPARMS;

/* open for outgoing structure with feature information */
typedef struct feature_out_xparms {
                    API_REGISTER  api_reg;
                    ACU_INT       handle;
                    ACU_INT       net;
                    ACU_INT       ts;
                    ACU_INT       cnf;
                    ACU_INT       sending_complete;
                    char          destination_addr[MAXNUM];
                    char          originating_addr[MAXNUM];
                    ACU_ULONG     feature_information;
                    union         uniquex unique_xparms;
                    FEATURE_UNION feature;
                    ACU_INT       message_control;
            } ACU_PACK_DIRECTIVE FEATURE_OUT_XPARMS;




/*-----------------------------------*/
/* basic rate specific functions     */
/*-----------------------------------*/

typedef struct br_getset {
                         ACU_INT   linestat;
                         ACU_INT   framerrs;
                         ACU_INT   sliperrs;
             }ACU_PACK_DIRECTIVE BR_GETSET;

/* these can be read only */

typedef struct br_get {
                      ACU_UCHAR    nos;
                      ACU_UCHAR    los;
                      ACU_UCHAR    crc;
                      ACU_UCHAR    phantom;
                      char         state[3];
                      ACU_UCHAR    majorrev;
                      ACU_UCHAR    minorrev;
                      ACU_LONG     clock;
              }ACU_PACK_DIRECTIVE BR_GET;

typedef struct br_l1_xstats {
                API_REGISTER api_reg;
                            ACU_INT      net;
                            BR_GETSET    getset;
                            BR_GET       get;
                }ACU_PACK_DIRECTIVE  BR_L1_XSTATS;

typedef struct br_l2_xstate {
                API_REGISTER api_reg;
                            ACU_INT      net;
                            ACU_LONG     state;
                } ACU_PACK_DIRECTIVE BR_L2_XSTATE;


/* Used by the user side to send SPID */

typedef struct send_spid_xparms {
                API_REGISTER api_reg;
                                ACU_INT      net;
                                ACU_INT      device;
                                ACU_INT      status;
                                char         spid[MAXSPID];
                                char         originating_addr[MAXNUM];
                                ACU_INT      originating_addr_index;
                }ACU_PACK_DIRECTIVE  SEND_SPID_XPARMS;

typedef struct get_spid_xparms {
                   API_REGISTER api_reg;
                               ACU_INT      net;
                               ACU_UCHAR    ces;
                               char         spid[MAXSPID];
                               char         originating_addr[MAXNUM];
                               ACU_INT      originating_addr_index;
                   } ACU_PACK_DIRECTIVE GET_SPID_XPARMS;

/* used by the network side to send Endpoint Address */

typedef struct send_endpoint_id_xparms {
                       API_REGISTER api_reg;
                                       ACU_INT      net;
                                       ACU_UCHAR    ces;
                                       ACU_UCHAR    usid;
                                       ACU_UCHAR    tid;
                                       ACU_UCHAR    flag;
                       }ACU_PACK_DIRECTIVE  SEND_ENDPOINT_ID_XPARMS;

/* device / timeslot encoding decoding */

typedef struct endec_xparms {
                API_REGISTER api_reg;
                            ACU_INT      net;
                            ACU_INT      device;
                            ACU_INT      ts;
                            ACU_INT      endec;
                } ACU_PACK_DIRECTIVE ENDEC_XPARMS;

/* interrogate dsp fitted per port */

typedef struct dsp_xparms {
                          API_REGISTER api_reg;
                          ACU_INT      net;
                          ACU_INT      config;
                          union
                              {
                              struct
                                  {
                                  ACU_VCC_THREAD_ID calling_thread;
                                  } sig_h323;
                              } ACU_PACK_DIRECTIVE unique_xparms;
                          } ACU_PACK_DIRECTIVE DSP_XPARMS;

/* VoIP codec configuration info */

typedef struct codec_xparms {
                            API_REGISTER api_reg;
                            ACU_INT      net;
                            union
                               {
                               struct
                                  {
                                  ACU_VCC_THREAD_ID calling_thread;
                                  ACU_INT codecs[MAXCODECS];
                                  } sig_h323;
                               } ACU_PACK_DIRECTIVE unique_xparms;
                            } CODEC_XPARMS;

typedef struct set_configuration_xparms {
                                        API_REGISTER    api_reg;
                                        ACU_INT         net;
                                        char           *board_ip_address ;           /* stores the ip address for the board */
                                        ACU_ULONG       ip_address1;
                                        ACU_ULONG       ip_address2;
                                        ACU_ULONG       ip_address3;
                                        ACU_INT         def_encode_gain;
                                        ACU_INT         def_decode_gain;
                                        ACU_UCHAR       def_tos;
                                        ACU_UINT        def_jitter;
                                        ACU_UINT        max_jitter;
                                        ACU_UINT        max_jitter_buffer;
                                        union
                                           {
                                           struct
                                              {
                                              ACU_VCC_THREAD_ID calling_thread;
                                              } sig_h323;
                                           } ACU_PACK_DIRECTIVE unique_xparms;
                                        } SET_CONFIGURATION_XPARMS;

  typedef struct get_configuration_xparms{
                                         API_REGISTER   api_reg;
                                         ACU_INT        net;
                                         char          *board_ip_address;           /* stores the ip address for the board */
                                         ACU_INT        itf;
                                         ACU_ULONG      ip_address1;
                                         ACU_ULONG      ip_address2;
                                         ACU_ULONG      ip_address3;
                                         ACU_INT        version_major;
                                         ACU_INT        version_minor;
                                         ACU_INT        hardware_type;
                                         ACU_INT        devices_fitted;
                                         ACU_INT        psos_bf;
                                         ACU_INT        VoIP;
                                         ACU_INT        SS7;
                                         ACU_INT        PSOS_ef;
                                         ACU_INT        allowed_G711;
                                         ACU_INT        allowed_G723_1;
                                         ACU_INT        allowed_G729A;
                                         ACU_UCHAR      dsp_serial_number[17];
                                         ACU_UCHAR      AC_number[17];
                                         ACU_UCHAR      def_tos;
                                         ACU_UINT       def_jitter;
                                         ACU_UINT       max_jitter;
                                         ACU_UINT       max_jitter_buffer;
                                         union
                                            {
                                            struct
                                               {
                                               ACU_VCC_THREAD_ID calling_thread;
                                               } sig_h323;
                                            } ACU_PACK_DIRECTIVE unique_xparms;
                                         } GET_CONFIGURATION_XPARMS;

typedef struct voip_get_stats_xparms {
                                     API_REGISTER       api_reg;
                                     ACU_INT            net;
                                     char              *board_ip_address ;  /* stores the ip address for the board */
                                     ACU_LONG           lastchange;
                                     ACU_LONG           unknownprotos;
                                     ACU_LONG           outdiscards;
                                     ACU_LONG           outerrors;
                                     ACU_INT            ipformaterrors;
                                     ACU_INT            ipaddrerrors;
                                     ACU_INT            ipoutnoroutes;
                                     union
                                         {
                                         struct
                                            {
                                            ACU_VCC_THREAD_ID calling_thread;
                                            } sig_h323;
                                         } ACU_PACK_DIRECTIVE unique_xparms;
                                     } VOIP_GET_STATS_XPARMS;

typedef struct channel_stats_xparms {
                                    API_REGISTER api_reg;
                                    ACU_INT      net;
                                    char        *board_ip_address ;    /* stores the ip address for the board */
                                    ACU_INT      timeslot;
                                    ACU_INT      stream;
                                    union
                                       {
                                       struct
                                          {
                                          ACU_VCC_THREAD_ID calling_thread;
                                          } sig_h323;
                                       } ACU_PACK_DIRECTIVE unique_xparms;
                                    } CHANNEL_STATS_XPARMS;

typedef struct set_debug_xparms {
                                API_REGISTER api_reg;
                                ACU_INT      net;
                                char        *board_ip_address ;           /* stores the ip address for the board */
                                ACU_INT      level;
                                ACU_INT      output;
                                union
                                   {
                                   struct
                                      {
                                      ACU_VCC_THREAD_ID calling_thread;
                                      } sig_h323;
                                   } ACU_PACK_DIRECTIVE unique_xparms;
                                } SET_DEBUG_XPARMS;

/*-----------------------------------------*/
/* VoIP administration channel information */
/*-----------------------------------------*/

typedef struct  {
               ACU_INT               msg_type;
               ACU_INT               sequence_number;
               ACU_INT               endpoint_alias_count;
               struct alias_address  *endpoint_alias;
               ACU_ULONG             transport_address;
               ACU_ULONG             endpoint_address;
               ACU_INT               time_to_live;
               ACU_INT               endpoint_identifier_length;
               ACU_USHORT            endpoint_identifier[128];
               ACU_INT               reason;
               ACU_INT               keep_alive;
               ACU_INT               gatekeeper_identifier_length;
               ACU_USHORT            gatekeeper_identifier[128];
               ACU_INT               prefix_count;
               struct alias_address  *prefixes;
               } voip_admin_msg;

typedef struct voip_admin_in_xparms {
                                    API_REGISTER        api_reg;
                                    ACU_VCC_THREAD_ID   calling_thread;
                                    voip_admin_msg     *admin_msg;
                                    ACU_INT             valid;
                                    } VOIP_ADMIN_IN_XPARMS;

typedef struct voip_admin_out_xparms {
                                     API_REGISTER       api_reg;
                                     ACU_VCC_THREAD_ID  calling_thread;
                                     voip_admin_msg    *admin_msg;
                                     } VOIP_ADMIN_OUT_XPARMS;

typedef struct default_ras_config {
                                  ACU_INT    endpoint_identifier_length;
                                  ACU_USHORT endpoint_identifier[128];  /* only used by the gatekeeper */
                                  ACU_ULONG gk_addr;   /* unsigned long int of the gatekeeper IP address */
                                  ACU_INT request_admission;
                                  } DEFAULT_RAS_CONFIG;


/*-----------------------------*/
/* groomer specific structures */
/*-----------------------------*/

typedef struct io_xparms {
                         API_REGISTER api_reg;
                         ACU_INT      net;
                         ACU_INT      io;
                         ACU_INT      port;
                         ACU_UCHAR    data;
             } ACU_PACK_DIRECTIVE IO_XPARMS;


typedef struct init_wd_xparms {
                              API_REGISTER api_reg;
                              ACU_INT      net;
                              ACU_LONG     time;
                              ACU_UCHAR    alarm;
                  }ACU_PACK_DIRECTIVE  INIT_WD_XPARMS;


typedef struct enable_wd_xparms {
                API_REGISTER api_reg;
                                ACU_INT      net;
                                ACU_UCHAR    enable;
                } ACU_PACK_DIRECTIVE ENABLE_WD_XPARMS;


typedef struct refresh_wd_xparms {
                 API_REGISTER api_reg;
                                 ACU_INT      net;
                                 ACU_UCHAR    refresh;
                 } ACU_PACK_DIRECTIVE REFRESH_WD_XPARMS;

#ifdef ACU_TEST
typedef struct dpr_xparms {
                                 API_REGISTER api_reg;
                                 ACU_UCHAR    net;
                                 ACU_UCHAR    channel;
                                 ACU_UCHAR    prim;
                                 ACU_UCHAR    data[500];
                 } ACU_PACK_DIRECTIVE DPR_XPARMS;
#endif


typedef struct signal_apievent_xparms {
                          API_REGISTER api_reg;
                          ACU_INT      command;
                          ACU_INT      event;
                          ACU_INT      net;
                          ACU_INT      handle;
                          ACU_LONG     state;
                          ACU_LONG     timeout;
                          ACU_LONG     extended_state;
                          } ACU_PACK_DIRECTIVE SIGNAL_APIEVENT_XPARMS;

/*-----------------------------*/
/* union past to ioctl         */
/*-----------------------------*/


typedef union V5_pblock_ioctlu {
                     API_REGISTER                api_reg;
                     V5_PBLOCK_XPARMS            pblock_xparms;
                     } V5_PBLOCK_IOCTLU;


typedef union ioctlu {
             API_REGISTER                api_reg;
             INIT_XPARMS                 init_xparms;
             OUT_XPARMS                  out_xparms;
             SIGINFO_XPARMS              siginfo;
             SYSINFO_XPARMS              sysinfo_xparms;
             IN_XPARMS                   in_xparms;
             STATE_XPARMS                state_xparms;
             DETAIL_XPARMS               detail_xparms;
             GET_CHARGE_XPARMS           get_charge_xparms;
             PUT_CHARGE_XPARMS           put_charge_xparms;
             OVERLAP_XPARMS              overlap_xparms;
             CAUSE_XPARMS                cause_xparms;
             TRANSFER_XPARMS             transfer_xparms;
             LEQ                         leq;
             SFMW_XPARMS                 sfmw_xparms;
             L1_XSTATS                   l1_stats;
             L2_XSTATE                   l2_state;
             WATCHDOG_XPARMS             watchdog_xparms;
             DCBA_XPARMS                 dcba_xparms;
             LOG_XPARMS                  log_xparms;
             TCMD_XPARMS                 tcmd_xparms;
             V4_PBLOCK_XPARMS            v4_pblock_xparms;
             ALARM_XPARMS                alarm_xparms;
             Q921_XPARMS                 q921_xparms;
             ACU_INT                     handle;
             ACU_INT                     portnum;
             SEND_SPID_XPARMS            send_spid_xparms;
             GET_SPID_XPARMS             get_spid_xparms ;
             SEND_ENDPOINT_ID_XPARMS     send_endpoint_id_xparms ;
             ENDEC_XPARMS                endec_xparms;
             BR_L1_XSTATS                br_l1_stats;
             BR_L2_XSTATE                br_l2_state;
             PROGRESS_XPARMS             progress_xparms;
             PROCEEDING_XPARMS           proceeding_xparms;
             SETUP_ACK_XPARMS            setup_ack_xparms;
             NOTIFY_XPARMS               notify_xparms;
             INCOMING_RINGING_XPARMS     incoming_ringing_xparms;
             ACCEPT_XPARMS               accept_xparms;
             DISCONNECT_XPARMS           disconnect_xparms;
             HOLD_XPARMS                 hold_xparms;
             GET_ORIGINATING_ADDR_XPARMS get_originating_addr_xparms;
             TRACE_XPARMS                trace_xparms;
             DSP_XPARMS                  dsp_xparms;
             DPNS_INCOMING_RING_XPARMS   dpns_incoming_ring_xparms;
             DPNS_CALL_ACCEPT_XPARMS     dpns_call_accept_xparms;
             DPNS_OUT_XPARMS             dpns_out_xparms;
             DPNS_OVERLAP_XPARMS         dpns_overlap_xparms;
             DPNS_TRANSIT_XPARMS         dpns_transit_xparms;
             DPNS_SET_TRANSIT_XPARMS     dpns_set_transit_xparms;
             DPNS_L2_XPARMS              dpns_l2_xparms;
             DPNS_FEATURE_XPARMS         dpns_feature_xparms;
             DPNS_DETAIL_XPARMS          dpns_detail_xparms;
             DPNS_CAUSE_XPARMS           dpns_cause_xparms;
             KEYPAD_XPARMS               keypad_xparms;
             TSINFO_XPARMS               tsinfo_xparms;
             FEATURE_OUT_XPARMS          feature_out_xparms;
             FEATURE_DETAIL_XPARMS       feature_detail_xparms;
#ifdef ACU_TEST
             DPR_XPARMS                  dpr_xparms;
#endif
             HANDLE_2_PORT_XPARMS        handle_2_port_xparms ;
             ACUC_ASSOC_NET_XPARMS       assoc_net_xparms ;
             ACUC_MAINTENANCE_XPARMS     maintenance_xparms ;
#ifdef ACUC_CLONED
             ACUC_EVENT_IF_XPARMS        event_if_xparms ;
#endif
             SIGNAL_APIEVENT_XPARMS      signal_apievent_xparms;
             CODEC_XPARMS                codec_xparms;
             VOIP_ADMIN_IN_XPARMS        voip_admin_in_xparms;
             VOIP_ADMIN_OUT_XPARMS       voip_admin_out_xparms;
             GET_CONFIGURATION_XPARMS    get_configuration_xparms;
             SET_CONFIGURATION_XPARMS    set_configuration_xparms;
             VOIP_GET_STATS_XPARMS       voip_get_stats_xparms;
             CHANNEL_STATS_XPARMS        channel_stats_xparms;
             SET_DEBUG_XPARMS            set_debug_xparms;
             ACU_INT                     command_error;
             SET_SYSINFO_XPARMS          set_sysinfo_xparms;
             } IOCTLU;

/*-----------------------------*/
/* Library Specific Structures */
/*-----------------------------*/

typedef struct download_xparms {
                   API_REGISTER api_reg;
                               ACU_INT      net;
                               char       * filenamep;
                   } ACU_PACK_DIRECTIVE DOWNLOAD_XPARMS;


typedef struct restart_xparms  {
                   API_REGISTER api_reg;
                               ACU_INT      net;
                               char       * filenamep;
                               char       * config_stringp;
                   } ACU_PACK_DIRECTIVE RESTART_XPARMS;


/*---------------------------------------*/
/* library internal management structure */
/*---------------------------------------*/

#define NCARDS    MAXCNTRL  /* maximum number of cards */

#ifdef ACU_OS2
#define NCHAN     30        /* number of channels      */
#else
#define NCHAN     255        /* number of channels      */
#endif

#define ENQH      0xC000    /* Enquiry  call mask */
#define INCH      0x8000    /* Incoming call mask */
#define OUCH      0x4000    /* Outgoing call mask */

typedef struct card {
                    int      clh;                             /* device handle                       */
                    ACU_INT  nnets;                           /* number of network ports on the card */
                    ACU_INT  version;                         /* driver version                      */
                    ACU_INT  types[MAXPPC];                   /* Protocol Type                       */
                    ACU_INT  lines[MAXPPC];                   /* line types                          */
                    char     board_ip_address[32];            /* stores the ip address for the board */
                    ACU_INT  v1bmi_card_num;                  /* driver number and switch number     */
                    ACU_INT  voipservice;
                    } ACU_PACK_DIRECTIVE CARD;

/*----------- Function Prototypes ---------------*/
/* functions in the call library file            */

#ifdef __cplusplus
extern "C"{
#endif
/*-----------------------------------------------*/

ACUDLL int            call_free_admin_msg       ( voip_admin_msg * );
ACUDLL int ACU_WINAPI call_set_default_gk_config( struct default_ras_config * );
ACUDLL int ACU_WINAPI call_init                 ( struct init_xparms * );
ACUDLL int ACU_WINAPI call_signal_info          ( struct siginfo_xparms * );
ACUDLL int ACU_WINAPI call_system_info          ( struct sysinfo_xparms * );
ACUDLL int ACU_WINAPI call_openout              ( struct out_xparms * );
ACUDLL int ACU_WINAPI call_openin               ( struct in_xparms *  );
ACUDLL int ACU_WINAPI call_state                ( struct state_xparms * );
ACUDLL int ACU_WINAPI call_event                ( struct state_xparms * );
ACUDLL int ACU_WINAPI call_details              ( struct detail_xparms * );
ACUDLL int ACU_WINAPI call_get_charge           ( struct get_charge_xparms * );
ACUDLL int ACU_WINAPI call_put_charge           ( struct put_charge_xparms * );
ACUDLL int ACU_WINAPI call_send_overlap         ( struct overlap_xparms * );
ACUDLL int ACU_WINAPI call_incoming_ringing     ( int );
ACUDLL int ACU_WINAPI call_accept               ( int );
ACUDLL int ACU_WINAPI call_hold                 ( int );
ACUDLL int ACU_WINAPI call_reconnect            ( int );
ACUDLL int ACU_WINAPI call_enquiry              ( struct out_xparms * );
ACUDLL int ACU_WINAPI call_transfer             ( struct transfer_xparms * );
ACUDLL int ACU_WINAPI call_answercode           ( struct cause_xparms * );
ACUDLL int ACU_WINAPI call_get_originating_addr ( int );
ACUDLL int ACU_WINAPI call_getcause             ( struct cause_xparms * );
ACUDLL int ACU_WINAPI call_disconnect           ( struct cause_xparms * );
ACUDLL int ACU_WINAPI call_release              ( struct cause_xparms * );
ACUDLL int ACU_WINAPI call_nports               ( void );
ACUDLL void	          call_set_net0_swnum      ( int  );
ACUDLL int ACU_WINAPI call_port_2_swdrvr        ( int );
ACUDLL int ACU_WINAPI call_port_2_stream        ( int );
ACUDLL int ACU_WINAPI call_handle_2_port        ( int );
ACUDLL int ACU_WINAPI call_handle_2_chan        ( int );
ACUDLL int ACU_WINAPI call_handle_2_io          ( int );
ACUDLL int            call_pblock               ( struct v5_pblock_xparms * );
ACUDLL int            call_tcmd                 ( struct tcmd_xparms * );
ACUDLL int            call_sfmw                 ( int, int, char * );
ACUDLL int ACU_WINAPI call_restart_fmw          ( struct restart_xparms * );
ACUDLL int ACU_WINAPI call_send_alarm           ( struct alarm_xparms * );
ACUDLL int            call_send_q921            ( struct q921_xparms * );
ACUDLL int            call_get_q921             ( struct q921_xparms * );
ACUDLL int ACU_WINAPI call_watchdog             ( struct watchdog_xparms * );
ACUDLL int ACU_WINAPI call_l1_stats             ( struct l1_xstats * );
ACUDLL int ACU_WINAPI call_l2_state             ( struct l2_xstate * );
ACUDLL int            call_br_l1_stats          ( struct br_l1_xstats * );
ACUDLL int            call_br_l2_state          ( struct br_l2_xstate * );
ACUDLL int ACU_WINAPI call_type                 ( int );
ACUDLL int ACU_WINAPI call_line                 ( int );
ACUDLL int ACU_WINAPI call_is_download          ( int );
ACUDLL int ACU_WINAPI call_version              ( int );
ACUDLL int ACU_WINAPI call_download_fmw         ( struct download_xparms * );
ACUDLL int ACU_WINAPI call_download_dsp         ( struct download_xparms * );
ACUDLL int            call_download_brdsp       ( struct download_xparms * );
ACUDLL int ACU_WINAPI call_trace                ( int, int );
ACUDLL int ACU_WINAPI call_dcba                 ( struct dcba_xparms * );
ACUDLL int ACU_WINAPI call_protocol_trace       ( struct log_xparms * );
ACUDLL int ACU_WINAPI call_progress             ( struct progress_xparms * );
ACUDLL int ACU_WINAPI call_proceeding           ( struct proceeding_xparms * );
ACUDLL int ACU_WINAPI call_setup_ack            ( struct setup_ack_xparms * );
ACUDLL int ACU_WINAPI call_notify               ( struct notify_xparms * );
ACUDLL int ACU_WINAPI call_dsp_config           ( struct dsp_xparms * );
ACUDLL int ACU_WINAPI call_send_voip_admin_msg  ( struct voip_admin_out_xparms *);
ACUDLL int ACU_WINAPI call_get_voip_admin_msg   ( struct voip_admin_in_xparms *);
ACUDLL int ACU_WINAPI call_open_voip_admin_chan ( void );
ACUDLL int ACU_WINAPI call_close_voip_admin_chan( void );
ACUDLL int ACU_WINAPI call_get_configuration	( struct get_configuration_xparms * pdsp );
ACUDLL int ACU_WINAPI call_set_configuration	( struct set_configuration_xparms * pdsp );
ACUDLL int ACU_WINAPI call_get_stats            ( struct voip_get_stats_xparms * pdsp );
ACUDLL int ACU_WINAPI call_set_debug            ( struct set_debug_xparms * pdsp );
ACUDLL int ACU_WINAPI call_set_system_info      ( struct set_sysinfo_xparms * set_sysinfop, struct sysinfo_xparms * sysinfop);
ACUDLL int ACU_WINAPI xcall_incoming_ringing    ( struct incoming_ringing_xparms * );
ACUDLL int ACU_WINAPI xcall_accept              ( struct accept_xparms * );
ACUDLL int ACU_WINAPI xcall_getcause            ( struct disconnect_xparms * );
ACUDLL int ACU_WINAPI xcall_disconnect          ( struct disconnect_xparms * );
ACUDLL int ACU_WINAPI xcall_release             ( struct disconnect_xparms * );
ACUDLL int ACU_WINAPI xcall_hold                ( struct hold_xparms * );
ACUDLL int ACU_WINAPI xcall_reconnect           ( struct hold_xparms * );
ACUDLL int ACU_WINAPI xcall_get_originating_addr( struct get_originating_addr_xparms * );
ACUDLL int ACU_WINAPI xcall_trace               ( struct trace_xparms * );
ACUDLL int ACU_WINAPI call_codec_config         ( struct codec_xparms * );

ACUDLL int            call_endpoint_initialise  ( struct send_spid_xparms * );
ACUDLL int            call_get_spid             ( struct get_spid_xparms * );
ACUDLL int            call_send_endpoint_id     ( struct send_endpoint_id_xparms * );
ACUDLL int            call_get_endpoint_status  ( struct send_spid_xparms * );
ACUDLL int ACU_WINAPI call_send_keypad_info     ( struct keypad_xparms * );
ACUDLL int            call_encode_devts         ( struct endec_xparms * );
ACUDLL int            call_decode_devts         ( struct endec_xparms * );
ACUDLL int            call_ncards               ( void );
ACUDLL int            call_expose_fd            ( int );

ACUDLL void init_api_reg             ( struct api_register *, ACU_LONG );



/*-----------------------------------*/
/* DPNSS Feature Function Prototypes */
/*-----------------------------------*/

ACUDLL int ACU_WINAPI dpns_incoming_ringing ( struct dpns_incoming_ring_xparms * );
ACUDLL int ACU_WINAPI dpns_call_details     ( struct dpns_detail_xparms * );
ACUDLL int ACU_WINAPI dpns_call_accept      ( struct dpns_call_accept_xparms * );
ACUDLL int ACU_WINAPI dpns_openout          ( struct dpns_out_xparms * );
ACUDLL int ACU_WINAPI dpns_send_overlap     ( struct dpns_overlap_xparms * );
ACUDLL int ACU_WINAPI dpns_set_transit      ( int );
ACUDLL int            xdpns_set_transit     ( struct dpns_set_transit_xparms * );
ACUDLL int ACU_WINAPI dpns_send_transit     ( struct dpns_transit_xparms * );
ACUDLL int ACU_WINAPI dpns_transit_details  ( struct dpns_transit_xparms * );
ACUDLL int ACU_WINAPI dpns_set_l2_ch        ( struct dpns_l2_xparms * );
ACUDLL int ACU_WINAPI dpns_l2_state         ( struct dpns_l2_xparms * );
ACUDLL int ACU_WINAPI dpns_send_feat_info   ( struct dpns_feature_xparms * );
ACUDLL int ACU_WINAPI dpns_call_details     ( struct dpns_detail_xparms * );
ACUDLL int ACU_WINAPI dpns_getcause         ( struct dpns_cause_xparms * );
ACUDLL int ACU_WINAPI dpns_disconnect       ( struct dpns_cause_xparms * );
ACUDLL int ACU_WINAPI dpns_release          ( struct dpns_cause_xparms * );
ACUDLL int ACU_WINAPI dpns_watchdog         ( struct dpns_wd_xparms * );

/*--------------------------------------*/
/* Enhanced Feature Function Prototypes */
/*--------------------------------------*/
ACUDLL int ACU_WINAPI call_feature_openout  ( struct feature_out_xparms * );
ACUDLL int ACU_WINAPI call_feature_enquiry  ( struct feature_out_xparms * );
ACUDLL int ACU_WINAPI call_feature_details  ( struct feature_detail_xparms * );
ACUDLL int ACU_WINAPI call_feature_send     ( struct feature_detail_xparms * );
ACUDLL int ACU_WINAPI call_send_connectionless ( struct  feature_detail_xparms * );
ACUDLL int ACU_WINAPI call_get_connectionless ( struct  feature_detail_xparms * );

/*--------------------------------------*/
/* Maintenance-oriented prototypes      */
/* - only available for ISUP at present.*/
/*--------------------------------------*/
ACUDLL int ACU_WINAPI call_maint_ts_block  ( struct ts_blocking_xparms * );
ACUDLL int ACU_WINAPI call_maint_ts_unblock  ( struct ts_blocking_xparms * );
ACUDLL int ACU_WINAPI call_maint_port_block  ( struct port_blocking_xparms * );
ACUDLL int ACU_WINAPI call_maint_port_unblock  ( struct port_blocking_xparms * );
ACUDLL int ACU_WINAPI call_maint_port_reset  ( struct port_reset_xparms * );

/*-----------------------------------*/
/* Aculab Internal Testing Functions */
/*-----------------------------------*/
#ifdef ACU_TEST
ACUDLL int send_dpr_command      ( struct dpr_xparms *);
ACUDLL int receive_dpr_event     ( struct dpr_xparms *);
#endif

ACUDLL int call_tsinfo                  ( struct tsinfo_xparms * );
ACUDLL int call_assoc_net               ( struct acuc_assoc_net_xparms * );
ACUDLL int call_signal_apievent ( struct  signal_apievent_xparms * );


/*----------- Function Prototypes ---------------*/
/* functions in common.c                         */

ACUDLL int  ACU_WINAPI system_init ( void );       /* initialise switch and call driver system */
ACUDLL void ACU_WINAPI swap_clock  ( int  );       /* change clock sources between cards       */
ACUDLL int             chknet_port ( int );        /* returns a signalling system dependent value */
ACUDLL int             chknet      ( char * );     /* returns a signalling system dependent value */
ACUDLL void            init_api_reg ( struct api_register *, ACU_LONG );

ACUDLL char * ACU_WINAPI port_2_filename  ( int ); /* select firmware file name to be downloaded */
ACUDLL char * ACU_WINAPI sigtype_2_string ( int ); /* convert a signalling system type to a printable string */

ACUDLL char * ACU_WINAPI error_2_string(int);

/* some useful switch functions */

ACUDLL int  ACU_WINAPI nailup        ( int, int );
ACUDLL void ACU_WINAPI handle_switch ( struct detail_xparms *, int, int );
ACUDLL void ACU_WINAPI idle_net_ts   ( int, int );
ACUDLL int             verify_ddi    ( char * );
ACUDLL int             is_tone       ( int );

/*-----------------------------------------------*/

#ifdef __cplusplus
}
#endif

/*-----------------------------------------------*/

/*++

IOCTL definitions for WINDOWS NT

--*/

#if 1

#define GPD_TYPE 40000

#define CALL_IOCTL \
	CTL_CODE ( GPD_TYPE, 0x0900, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define REGISTER_NCNTRLS \
	CTL_CODE ( GPD_TYPE, 0x0901, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define CLEAR_EVENT \
	CTL_CODE ( GPD_TYPE, 0x0902, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define SIGNAL_EVENT \
	CTL_CODE ( GPD_TYPE, 0x0903, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define CHECK4REV5 \
	CTL_CODE ( GPD_TYPE, 0x0904, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define CHECK_EVENT \
	CTL_CODE ( GPD_TYPE, 0x0905, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define DRIVER_VERSION \
	CTL_CODE ( GPD_TYPE, 0x0906, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define CALL_PBLOCK_IOCTL \
        CTL_CODE ( GPD_TYPE, 0x0907, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define GET_TNETS \
        CTL_CODE ( GPD_TYPE, 0x0908, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define REGISTER_UNET \
        CTL_CODE ( GPD_TYPE, 0x0909, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define DRV0_EVENT_IF \
	       CTL_CODE ( GPD_TYPE, 0x090a, METHOD_BUFFERED, FILE_ANY_ACCESS )


typedef struct ntioctl {
   int    command;
   int    error;
   int    spare;
   union  ioctlu ioctlu;
}ACU_PACK_DIRECTIVE NTIOCTL, *PNTIOCTL;

typedef struct report_event_parms {
   int cnum;
   int handle;
   int state;
}ACU_PACK_DIRECTIVE REPORT_EVENT_PARMS, *PREPORT_EVENT_PARMS;


typedef struct report_checkevent_parms {
   int cnum;
   int handle;
   int state;
   int ret;
}ACU_PACK_DIRECTIVE REPORT_CHECKEVENT_PARMS, *PREPORT_CHECKEVENT_PARMS;


typedef struct report_driverversion_parms {
   int cnum;
   int majorversion;
   int minorversion;
   int ret;
} ACU_PACK_DIRECTIVE REPORT_DRIVERVERSION_PARMS, *PREPORT_DRIVERVERSION_PARMS;


#endif

/*----------------- end of file -----------------*/
#ifndef LINUX
#ifndef ACU_SOLARIS_SPARC
#pragma pack ( )
#endif /* ACU_SOLARIS_SPARC */
#endif /* LINUX */

#endif

/*------------------------------------------------------------*/
/* Copyright ACULAB plc. (c) 1996-1998                        */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : smport.h 	                      	  */
/*                                                            */
/*           Purpose : Handle portability across multiple O/S */
/*                                                            */
/*            Author : Peter Dain                             */
/*                                                            */
/*       Create Date : 21st February 1997                     */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/

#ifndef __SMPORT__
#define __SMPORT__

/*
 * The Pre-processer definition UNIX_SYSTEM should be defined for Unix Prosody applications.
 * The following Unix variants are provided for through conditional compilation:
 *      SM_POLL_UNIX  - Unix supporting chpoll driver entry point and Prosody events through poll system call
 *					    for example Unixware 2
 *      SM_SEL_UNIX   - Unix supporting driver "select" primitives and Prosody events through select system call
 *      SM_CLONE_UNIX - Unix supporting clone node channels and Prosody events through async i/o on clone channels
 *
 * In addition if multi-threaded applications are being written, SM_THREAD_UNIX, should also be defined. 
 */

#ifndef ACUDLL
	#define ACUDLL
#endif

#ifdef WIN32
	/*
	 * Get definition for HANDLE.
	 */
	#include <windows.h>

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

#ifdef __OS2__
   /*
    * Get definition for LHANDLE.
    */
   #include <os2.h>
#endif

#pragma pack ( 1 )


/*
 * Note if compiling driver, we take data type definitions from O/S specific 
 * smdport.h while if compiling application we use this one.
 *
 * Why? - because in OS/2 sizeof(int) in driver is 16 bits while for application
 *        and many compilers it is 32 bits.
 */
/*
 * This is defined for Windows NT applications.
 */
#ifdef WIN32
#ifndef UNIX_SYSTEM
	#define far

	#define tSMCriticalSection CRITICAL_SECTION

	#define tSMDevHandle 	HANDLE
	#define tSMFileHandle	int

	typedef int				tSM_INT;
	typedef unsigned long	tSM_UT32;
	typedef unsigned char	tSM_UT8;

	typedef HANDLE 	tSMChannelId;
	typedef HANDLE 	tSMEventId;
    typedef void* tSMExternEventRef;

	#define kSMNullChannelId 0
	#define kSMNullDevHandle 0

	typedef float			tSMIEEE32Bit754854Float;
#endif
#endif

#ifdef UNIX_SYSTEM
	#define far

	typedef void* 	tSMCriticalSection;
	typedef int 	tSMDevHandle;
	typedef int 	tSMFileHandle;

	typedef int 			tSM_INT;
	typedef unsigned long  	tSM_UT32;
	typedef unsigned char  	tSM_UT8;     

	typedef int 			tSMChannelId;

	#define kSMNullChannelId 0
	#define kSMNullDevHandle 0

	typedef float			tSMIEEE32Bit754854Float;

#ifdef SM_CLONE_UNIX
	typedef int tSMEventId;
#endif

#ifdef SM_POLL_UNIX
	typedef struct tSMEventId { tSM_INT fd; tSM_INT mode; } tSMEventId;
#endif

#ifdef SM_SEL_UNIX
	typedef struct tSMEventId { tSM_INT fd; tSM_INT mode; } tSMEventId;
#endif

#endif

/*
 * This is defined for OS/2 applications.
 */
#ifdef __OS2__
   #define far

   #define tSMDevHandle    HFILE
   #define tSMFileHandle   int

   typedef void* 		   tSMCriticalSection;
   typedef short           tSM_INT;
   typedef unsigned long   tSM_UT32;
   typedef unsigned short  tSM_UT16BITS;
   typedef unsigned char   tSM_UT8;
   typedef char            tSM_Boolean;

   #define kSM_TRUE        1
   #define kSM_FALSE       0

   typedef long  tSMChannelId;
   typedef unsigned long  tSMEventId; 
   #define INVALID_HANDLE_VALUE 0xFFFFFFFFL

	#define kSMNullChannelId 0
	#define kSMNullDevHandle 0

   typedef float           tSMIEEE32Bit754854Float;

   typedef void* tSMExternEventRef;
#endif

#pragma pack ( )

#endif


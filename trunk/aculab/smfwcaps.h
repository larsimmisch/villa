/*------------------------------------------------------------*/
/* Copyright ACULAB plc. (c) 1996-1997                        */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : smfwcaps.h 	                      	  */
/*                                                            */
/*           Purpose : SHARC Module driver header file        */
/*                     for functions to parse f/w capability  */
/*                     data.                                  */
/*                                                            */
/*            Author : Peter Dain                             */
/*                                                            */
/*       Create Date : 21st February 1997                     */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/

#ifndef __SMFWCAPS__
#define __SMFWCAPS__

#ifndef __SMDRVR__
#include "smdrvr.h"
#endif

#pragma pack ( 1 )

#define kSMFFileId				0xAC534D46L 
#define kSMFMajVersion			1 
#define kSMFMinVersion			0 
#define kSMFMaxFWLibName		32
#define kSMFACULABReservedLen	16

#ifdef __cplusplus
extern "C" {
#endif

ACUDLL  char* smdPSSMFFindLibName( SM_FWCAPS_PARMS* fwcaps );

ACUDLL  char* smdPSSMFFindComment( SM_FWCAPS_PARMS* fwcaps );

ACUDLL  int   smdPSSMFGetFWVersion( SM_FWCAPS_PARMS* fwcaps );

ACUDLL  char* smdPSSMFFindSection( 	SM_FWCAPS_PARMS* 	fwcaps, 
							int 				sectionNo, 
							char* 				sectionName, 
							int*			 	sectionVersion, 
							int*			 	sectionLength 		);

ACUDLL  void smdPSSMFVersionString( SM_FWCAPS_PARMS* fwcaps, char* string );

#ifdef __cplusplus
}
#endif

#pragma pack ( )

#endif


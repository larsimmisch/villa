/*------------------------------------------------------------*/
/* Copyright ACULAB plc. (c) 1996-1997                        */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : smfwcaps.c                             */
/*                                                            */
/*           Purpose : SHARC Module driver library file       */
/*                     for functions to parse f/w capability  */
/*                     data.                                  */
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

#include "smfwcaps.h"
#include <stdio.h>
#include <string.h>


/*
 *******************************************************************
 * Entry points for F/W Capabilities Parsing Functions             *
 *******************************************************************
 */

static unsigned short getle16Bits( char* p )
{
	return (unsigned short) ((*((unsigned char*)p)) + ((*(((unsigned char*)p)+1))<<8));
}


/*
 * smdPSSMFFindLibName
 * 
 * Locate zero terminated f/w interface library name - indicates
 * type of interface for library, currently the
 * driver supports:
 *
 *  "SMDBESP" - Speech processing
 *  "SMDDVLP" - Custom F/W Development
 *  "SMDDC"   - Data Comms
 */
#ifdef _PROSDLL
ACUDLL 
#endif
char* smdPSSMFFindLibName( SM_FWCAPS_PARMS* fwcaps )
{
	return &(fwcaps->caps[0]);
}


/*
 * smdPSSMFFindComment
 * 
 * Locate zero terminated f/w desription comment
 */
#ifdef _PROSDLL
ACUDLL 
#endif
char* smdPSSMFFindComment( SM_FWCAPS_PARMS* fwcaps )
{
	char*		result;
	int 		n;
	int			i;
	char*		p;
	int			capsLen;

	result = 0;

	i = 0;

	p 		= &(fwcaps->caps[0]);
	capsLen = fwcaps->caps_length;

	while (i < capsLen)
	{
		if ((*p) == 0)
		{
			/* Found end of lib name. */
	
			n = ((i % 2) == 0) ? 2+kSMFACULABReservedLen+2 : 1+kSMFACULABReservedLen+2;

			i += n;

			result = ((capsLen - i) > 0) ? (p+n) : 0;
			break;
		}
		else
		{
			i += 1;
			p += 1;
		}
	}

	return result;
}


/*
 * smdPSSMFGetFWVersion
 * 
 * Extract f/w version no.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
int smdPSSMFGetFWVersion( SM_FWCAPS_PARMS* fwcaps )
{
	int				result;
	int 			n;
	int				i;
	char*			p;
	int				capsLen;

	result = 0;

	i = 0;

	p 		= &(fwcaps->caps[0]);
	capsLen = fwcaps->caps_length;

	while (i < capsLen)
	{
		if ((*p) == 0)
		{
			/* Found end of lib name. */
	
			n = ((i % 2) == 0) ? 2+kSMFACULABReservedLen : 1+kSMFACULABReservedLen;

			i += n;
			p += n;

			result = ((capsLen - i) >= 2) ? getle16Bits(p) : 0;
			break;
		}
		else
		{
			i += 1;
			p += 1;
		}
	}

	return result;
}


/*
 * smdPSSMFFindSection
 * 
 * Locate specific section in f/w capabilities data.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
char* smdPSSMFFindSection( 	SM_FWCAPS_PARMS* 	fwcaps, 
							int 				sectionNumber, 
							char* 				sectionName, 
							int* 				sectionVersion, 
							int*			 	sectionLength 		)
{
	char*		result;
	int 		n;
	int			i,j,k;
	char*		p;
	int			pastLibName;
	int			pastComment;
	int			sectionCount;
	int			isThisSection;
	int			capsLen;

	result = 0;

	i 			= 0;
	pastLibName = 0;
	pastComment = 0;

	p		= &(fwcaps->caps[0]);
	capsLen = fwcaps->caps_length;

	while ((i < capsLen) && (!pastComment))
	{
		if ((*p) == 0)
		{
			if (!pastLibName)
			{
				/* Found end of lib name. */
		
				n = ((i % 2) == 0) ? (2 + kSMFACULABReservedLen + 2) : (1 + kSMFACULABReservedLen + 2);

				pastLibName = 1;
			}
			else
			{
				n = ((i % 2) == 0) ? 2 : 1;

				pastComment = 1;
			}

			p += n;
			i += n;
		}
		else
		{
			i += 1;
			p += 1;
		}
	}

	if (pastComment && ((capsLen - i) > 2))
	{
		sectionCount = getle16Bits(p);

		p += 2;
		i += 2;

		for (j = 0; ((result == 0) && (j < sectionCount) && ((capsLen - i) > (4+2+2))); j++)
		{
			if ((capsLen - i) > (4+2+2))
			{
				*sectionLength = getle16Bits(p+4+2);

				isThisSection = 0;

				if (sectionNumber == -1)
				{
					isThisSection = 1;

					for (k = 0; (isThisSection) && (k < 4); k++)
					{
						if (*(p+k) != *(sectionName+k))
						{
							isThisSection = 0;
						}
					}
				}
				else if (j == sectionNumber)
				{
					isThisSection = 1;
					
					for (k = 0; (k < 4); k++)
					{
						*(sectionName+k) = (*(p+k));
					}

					*(sectionName+4) = 0;
				}

				if (isThisSection)
				{
					*sectionVersion = getle16Bits(p+4);

					result = (p + 4 + 2 + 2);
				}
				else
				{
					n = ((4 + 2 + 2) + *sectionLength);

					p += n;
					i += n;
				}
			}
		}
	}

	return result;
}


/*
 * smdPSSMFVersionString
 * 
 * Create printable version of f/w version in string.
 */
#ifdef _PROSDLL
ACUDLL 
#endif
void smdPSSMFVersionString( SM_FWCAPS_PARMS* fwcaps, char* string )
{
	int								maj;
	int								min;
	int								version;
	int								vers;
	int								len;
	tSM_UT32*						up;
	int								step;
	char							qualityCode;
	char*							p;

	p = string;

	version = smdPSSMFGetFWVersion(fwcaps);

	maj = (version >> 8);
	min = (version & 0x0ff);

	sprintf(p,"%d.%d",maj,min);

	p += strlen(p);

	if (up = (tSM_UT32*) smdPSSMFFindSection(fwcaps,-1,"VERS",&vers,&len))
	{
		step = ((*up) >> 16) & 0xff;
		
		qualityCode = (char) (((*up) >> 24) & 0xff);

		switch(qualityCode)
		{
			case 'I':
				sprintf(p,".%d Released (%ld)",step,*(up+1));
				break;

			case 'P':
				sprintf(p,".%d Released with patch (%ld)",step,*(up+1));
				break;

			case 'T':
				sprintf(p,".%d Test Only (%ld)",step,*(up+1));
				break;

			case 'F':
				sprintf(p,".%d Field Trial Only (%ld)",step,*(up+1));
				break;

			case 'S':
				sprintf(p,".%d Special Build (%ld)",step,*(up+1));
				break;
		
			case 'B':
				sprintf(p,".%d Beta (%ld)",step,*(up+1));
				break;

			case 'D':
				sprintf(p,".%d Under-Development (%ld)",step,*(up+1));
				break;

			default:
				sprintf(p,".%d Unknown-Quality (%c%ld)",step,qualityCode,*(up+1));
				break;
		}
	}

	p += strlen(p);

	if (up = (tSM_UT32*) smdPSSMFFindSection(fwcaps,-1,"CUST",&vers,&len))
	{
		sprintf(p," Cust %ld",*up);
	}
}


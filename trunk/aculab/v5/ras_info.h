/*------------------------------------------------------------*/
/* ACULAB plc                                                 */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : ras_info.h                             */
/*                                                            */
/*           Purpose : alias_address defines                  */
/*                                                            */
/*           Create Date : 17th April 2002                    */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Change History                                             */
/*                                                            */
/* rev: v5.10.0   07/03/2002 for v5.10.0 Release.             */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/

#ifndef RAS_INFO_H
#define RAS_INFO_H

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#endif

#include "mvcldrvr.h"

/*
 * General RAS info
 */
#define ASN1_GLOBALLY_UNIQUE_ID_LEN 16

#define TLS_Q931_MAX_DIGITS 32
#define TLS_Q931_CALL_ID_LENGTH ASN1_GLOBALLY_UNIQUE_ID_LEN
#define TLS_Q931_CONF_ID_LENGTH ASN1_GLOBALLY_UNIQUE_ID_LEN
#define TLS_H450_CALL_ID_LENGTH ASN1_GLOBALLY_UNIQUE_ID_LEN
#define TLS_ENDPOINT_ID_LENGTH  128
#define TLS_GATEKEEPER_ID_LENGTH 128

/*
 * alias address max lengths
 */
#define ALIAS_ADDR_MAX_E164       128
#define ALIAS_ADDR_MAX_H323_ID    256
#define ALIAS_ADDR_MAX_URL_ID     512
#define ALIAS_ADDR_MAX_EMAIL_ADDR 512

/*
 * alias address types
 */
#define ALIAS_ADDR_E164       1
#define ALIAS_ADDR_H323_ID    2
#define ALIAS_ADDR_URL_ID     3
#define ALIAS_ADDR_SOCK       4
#define ALIAS_ADDR_EMAIL      5

typedef struct alias_address {

  int type;
  int length;

  union {    
    char e164_addr [ALIAS_ADDR_MAX_E164];
    ACU_USHORT h323_id [ALIAS_ADDR_MAX_H323_ID];
    char url_id [ALIAS_ADDR_MAX_URL_ID];
    struct sockaddr transport_addr;
    char email_addr [ALIAS_ADDR_MAX_EMAIL_ADDR];
  } address;

}alias_address;

#endif

/* 
	v3.h 

	Global definitions for Voice3 
	
	(Lars' 3rd version of a 100% vegetable call control/media abstraction)

	$Id$ 
*/

#ifndef V3_DEFS_H__
#define V3_DEFS_H__

/* events */
#define V3_EVENT 100

/* completion codes */
#define V3_OK 200
#define V3_SILENCE 201
#define V3_TIMEOUT 202
#define V3_ABORTED 203
#define V3_DISCONNECTED 204

/* warnings */
#define V3_WARNING_EMPTY 400

/* errors */
#define V3_ERROR_FAILED 500
#define V3_ERROR_INVALID_STATE 501
#define V3_ERROR_PROTOCOL_VIOLATION 502
#define V3_ERROR_BUSY 503
#define V3_ERROR_NO_RESOURCE 504
#define V3_ERROR_NOT_FOUND 505
#define V3_ERROR_NOT_IMPLEMENTED 506
#define V3_ERROR_INVALID_ARGUMENT 507
#define V3_ERROR_TIMEOUT 508
#define V3_ERROR_NUMBER_CHANGED 509
#define V3_ERROR_UNREACHABLE 510
#define V3_ERROR_REJECTED 511

/* fatal errors */
#define V3_FATAL_SYNTAX 600

/* Conference definitions */
#define V3_CONF_LISTEN 0x01
#define V3_CONF_SPEAK 0x02
#define V3_CONF_DUPLEX 0x03

/* Molecule modes */
#define V3_MODE_DISCARD 0x01
#define V3_MODE_PAUSE 0x02
#define V3_MODE_MUTE 0x04
#define V3_MODE_RESTART 0x08
#define V3_MODE_DONT_INTERRUPT 0x10
#define V3_MODE_LOOP 0x20

#endif /* V3_DEFS_H__ */

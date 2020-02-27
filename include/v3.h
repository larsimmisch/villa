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
#define V3_DONE_OFFSET				200
#define V3_OK						(V3_DONE_OFFSET)
#define V3_ENDSILENCE				(V3_DONE_OFFSET + 1) /* operation ended with silence, but produced data */
#define V3_STOPPED					(V3_DONE_OFFSET + 2) /* operation was aborted */
#define V3_STOPPED_DISCONNECT		(V3_DONE_OFFSET + 3) /* operation was stopped by disconnect */
#define V3_STOPPED_DTMF				(V3_DONE_OFFSET + 4) /* operation was stopped by DTMF */

/* warnings */
#define V3_WARNING_OFFSET			400
#define V3_WARNING_SILENCE			(V3_WARNING_OFFSET) /* operation got only silence */
#define V3_WARNING_TIMEOUT			(V3_WARNING_OFFSET + 1)

/* errors */
#define V3_ERROR_OFFSET				500
#define V3_ERROR_FAILED				(V3_ERROR_OFFSET)
#define V3_ERROR_INVALID_STATE		(V3_ERROR_OFFSET + 1)
#define V3_ERROR_PROTOCOL_VIOLATION	(V3_ERROR_OFFSET + 2)
#define V3_ERROR_BUSY				(V3_ERROR_OFFSET + 3)
#define V3_ERROR_NO_RESOURCE		(V3_ERROR_OFFSET + 4)
#define V3_ERROR_NOT_FOUND			(V3_ERROR_OFFSET + 5)
#define V3_ERROR_NOT_IMPLEMENTED	(V3_ERROR_OFFSET + 6)
#define V3_ERROR_INVALID_ARGUMENT	(V3_ERROR_OFFSET + 7)
#define V3_ERROR_TIMEOUT			(V3_ERROR_OFFSET + 8)
#define V3_ERROR_NUMBER_CHANGED		(V3_ERROR_OFFSET + 9)
#define V3_ERROR_UNREACHABLE		(V3_ERROR_OFFSET + 10)
#define V3_ERROR_REJECTED			(V3_ERROR_OFFSET + 11)

/* fatal errors */
#define V3_FATAL_OFFSET				600
#define V3_FATAL_SYNTAX				(V3_FATAL_OFFSET)

/* Conference definitions */
#define V3_CONF_LISTEN 0x01
#define V3_CONF_SPEAK 0x02
#define V3_CONF_DUPLEX 0x03

/* Molecule modes */
#define V3_MODE_DISCARD 0x01
#define V3_MODE_PAUSE 0x02
#define V3_MODE_MUTE 0x04
#define V3_MODE_RESTART 0x08
/* Don't interrupt molecule for moelcules with higher priority. V3_MODE_DTMF_STOP is unaffected */
#define V3_MODE_DONT_INTERRUPT 0x10 
#define V3_MODE_LOOP 0x20
#define V3_MODE_DTMF_STOP 0x40

#endif /* V3_DEFS_H__ */

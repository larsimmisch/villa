#include "v3.h"

const char* result_name(int r)
{
	switch (r)
	{
	case V3_EVENT:
		return "event";
	case V3_OK:
		return "ok";
	case V3_STOPPED:
		return "stopped";
	case V3_STOPPED_DISCONNECT:
		return "stopped by disconnect";
	case V3_STOPPED_DTMF:
		return "stopped by DTMF";
	case V3_ENDSILENCE:
		return "end silence";
	case V3_WARNING_TIMEOUT:
		return "timeout (warning)";
	case V3_WARNING_SILENCE:
		return "silence (warning)";
	case V3_ERROR_FAILED:
		return "failed";
	case V3_ERROR_INVALID_STATE: 
		return "invalid state";
	case V3_ERROR_PROTOCOL_VIOLATION: 
		return "protocol violation";
	case V3_ERROR_BUSY:
		return "busy";
	case V3_ERROR_NO_RESOURCE:
		return "no resource";
	case V3_ERROR_NOT_FOUND: 
		return "not found";
	case V3_ERROR_NOT_IMPLEMENTED:
		return "not implemented";
	case V3_ERROR_INVALID_ARGUMENT:
		return "invalid argument";
	case V3_ERROR_TIMEOUT:
		return "timeout";
	case V3_ERROR_NUMBER_CHANGED:
		return "number changed";
	case V3_ERROR_UNREACHABLE:
		return "unreachable";
	case V3_ERROR_REJECTED:
		return "rejected";
	case V3_FATAL_SYNTAX:
		return "syntax error";
	default:
		return "unknown";
	}
};

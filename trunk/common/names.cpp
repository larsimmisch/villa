#include "phone.h"

const char* result_name(int r)
{
	switch (r)
	{
	case PHONE_EVENT:
		return "event";
	case PHONE_OK:
		return "ok";
	case PHONE_WARNING_EMPTY:
		return "empty";
	case PHONE_ERROR_FAILED:
		return "failed";
	case PHONE_ERROR_INVALID_STATE: 
		return "invalid state";
	case PHONE_ERROR_PROTOCOL_VIOLATION: 
		return "protocol violation";
	case PHONE_ERROR_BUSY:
		return "busy";
	case PHONE_ERROR_NO_RESOURCE:
		return "no resource";
	case PHONE_ERROR_NOT_FOUND: 
		return "not found";
	case PHONE_ERROR_ABORTED:
		return "aborted";
	case PHONE_ERROR_NOT_IMPLEMENTED:
		return "not implemented";
	case PHONE_ERROR_INVALID_ARGUMENT:
		return "invalid argument";
	case PHONE_ERROR_TIMEOUT:
		return "timeout";
	case PHONE_ERROR_NUMBER_CHANGED:
		return "number changed";
	case PHONE_ERROR_UNREACHABLE:
		return "unreachable";
	case PHONE_ERROR_REJECTED:
		return "rejected";
	case PHONE_FATAL_SYNTAX:
		return "syntax error";
	default:
		return "unknown";
	}
};

#include "phone.h"

const char* result_name(int r)
{
	switch (r)
	{
	case r_ok:
		return "r_ok";
	case r_timeout:
		return "r_timeout";
	case r_aborted: 
		return "r_aborted";
	case r_rejected: 
		return "r_rejected";
	case r_disconnected:
		return "r_disconnected";
	case r_failed:
		return "r_failed";
	case r_invalid: 
		return "r_invalid";
	case r_busy:
		return "r_busy";
	case r_not_available:
		return "r_not_available";
	case r_no_dialtone:
		return "r_no_dialtone";
	case r_empty:
		return "r_empty";
	case r_bad_state:
		return "r_bad_state";
	case r_number_changed:
		return "r_number_changed";
	default:
		return "unknown";
	}
};

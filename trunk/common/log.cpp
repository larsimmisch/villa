#include "log.h"

static nullbuf nb;
std::ostream nullstream(&nb);

Log* log_instance = NULL;

std::ostream& Log::log(int loglevel, const char *logclass, const char *name)
{ 
	lock();

	if (log_os && log_level >= loglevel)
	{
		time_t now;
		time(&now);
		struct tm *time = localtime(&now);

		char fill = log_os.fill('0');

		log_os << std::setw(2) << time->tm_year % 100 << '-' 
			<< std::setw(2) << time->tm_mon + 1 << '-' 
			<< std::setw(2) << time->tm_mday << ' ' 
			<< std::setw(2) << time->tm_hour << ':' 
			<< std::setw(2) << time->tm_min << ':' 
			<< std::setw(2) << time->tm_sec;

		log_os.fill(fill);

		switch (loglevel)
		{
		case log_error:
			log_os << " error ";
			break;
		case log_info:
			log_os << " info ";
			break;
		case log_warning:
			log_os << " warning ";
			break;
		default:
			log_os << " d" << loglevel - log_debug << ' ';
			break;
		}

		log_os << logclass;

		if (name && strlen(name) > 0)
			log_os << ' ' << name;

		log_os << ": ";

		return log_os;
	}
	else return nullstream;
}

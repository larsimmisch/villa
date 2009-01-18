#include "log.h"

static nullbuf nb;
std::ostream nullstream(&nb);

Log* log_instance = NULL;

#ifdef _WIN32

#include <time.h>
     
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif
     
struct timezone 
{
    int  tz_minuteswest; /* minutes W of Greenwich */
    int  tz_dsttime;     /* type of dst correction */
};
     
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    FILETIME ft;
    unsigned __int64 tmpres = 0;
    static int tzflag;
     
    if (NULL != tv)
    {
        GetSystemTimeAsFileTime(&ft);
     
        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;
     
        /*converting file time to unix epoch*/
        tmpres /= 10;  /*convert into microseconds*/
        tmpres -= DELTA_EPOCH_IN_MICROSECS; 
        tv->tv_sec = (long)(tmpres / 1000000UL);
        tv->tv_usec = (long)(tmpres % 1000000UL);
    }
     
    if (NULL != tz)
    {
        if (!tzflag)
        {
            _tzset();
            tzflag++;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }
     
    return 0;
}

#endif

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

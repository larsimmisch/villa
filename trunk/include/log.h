
#ifndef LOG_H__
#define LOG_H__

#pragma warning (disable : 4786)

#include <time.h>
#include <streambuf>
#include <ostream>
#include <iomanip>
#include <omnithread.h>

// this is a definition of a custom streambuf that outputs nothing
// this is needed by Loggable
template<class _char_t, class _traits = std::char_traits<_char_t> > 
class basic_nullbuf: public std::basic_streambuf<_char_t, _traits> 
{
protected:

	virtual int_type overflow(int_type c)
	{
		return c;
	}
	virtual std::streamsize xsputn(const char* s, std::streamsize num)
	{
		return num;
	}
};

typedef basic_nullbuf<char> nullbuf;
typedef basic_nullbuf<wchar_t> wnullbuf;

enum { log_error, log_warning, log_debug };

// base class, override if necessary
class Log
{

public:

	Log(std::ostream &o) : log_os(o) {}
	virtual ~Log() {}

	virtual std::ostream& log(int loglevel, const char *logclass, const char *name = 0);

	virtual void lock() { log_mutex.lock(); }
	virtual void unlock() { log_mutex.unlock(); }

	int log_level;
	std::ostream &log_os;
	omni_mutex log_mutex;
};

extern Log *log_instance;
extern std::ostream nullstream;

inline std::ostream& log(int loglevel, const char *logclass, const char *name = 0)
{
	if (!log_instance)
		return nullstream;

	return log_instance->log(loglevel, logclass, name);
}

inline void set_log_instance(Log* l)
{
	log_instance = l;
}

inline void set_log_level(int l)
{
	if (log_instance)
		log_instance->log_level = l;
}

struct logmanip
{
	logmanip(std::ostream& (*f)(std::ostream&)) 
		: function(f) {}

	std::ostream& (*function)(std::ostream&);
};

inline std::ostream& operator<<(std::ostream& os, logmanip& l)
{
	return l.function(os);
}

// helper
inline std::ostream& log_end(std::ostream& s)
{
	s << std::endl;

	if (log_instance)
		log_instance->unlock();

	return s;
}

inline logmanip logend()
{
	return logmanip(log_end);
}

#endif
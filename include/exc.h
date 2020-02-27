//  os\exc.h

#ifndef INCL_OS_EXC_H
#define INCL_OS_EXC_H

#include <ostream>

class Exception
{
public:

    Exception(
        const char* fileName,
        int lineNumber,
        const char* function,
        const char* exceptionName,
        const Exception* previous = 0) 
		: m_file(fileName), m_line(lineNumber), m_func(function), m_name(exceptionName), m_prev(previous) {}

    virtual ~Exception() {}

    virtual void printOn(std::ostream& out) const
	{
		out	<< m_file << ", line "
			<< m_line << ": "
			<< m_func << " -> "
			<< m_name;
	}

    const char*   m_file;
    int           m_line;
    const char*   m_func;
    const char*   m_name;
    const Exception*  m_prev;
};

static std::ostream& operator<<(std::ostream &out, const Exception &e)
{
    e.printOn(out);

    return out;
}

#endif

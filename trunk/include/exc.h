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
		: file(fileName), line(lineNumber), func(function), name(exceptionName), prev(previous) {}

    virtual ~Exception() {}

    virtual void printOn(std::ostream& out) const
	{
		out	<< file << ", line "
			<< line << ": "
			<< func << " -> "
			<< name;
	}

    const char*   file;
    int           line;
    const char*   func;
    const char*   name;
    const Exception*  prev;
};

static std::ostream& operator<<(std::ostream &out, const Exception &e)
{
    e.printOn(out);

    out << std::endl;

    return out;
}

#endif

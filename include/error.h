//	os\error.h

#ifndef INCL_OS_ERROR_H
#define INCL_OS_ERROR_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "exc.h"

class OutOfMemory : public Exception
{
	public:

	OutOfMemory(
		const char* fileName,
		int lineNumber,
		const char* function,
		const char* errorName = "out of memory",
		Exception* prev = 0)
	: Exception(fileName,lineNumber,function,errorName,prev) {}

	virtual ~OutOfMemory() {}
};

class OSError : public Exception
{
	public:

	OSError(
		const char* fileName,
		int lineNumber,
		const char* function,
		const char* errorName = "OS Error",
		Exception* prev = 0)
	: Exception(fileName,lineNumber,function,errorName,prev) {}

	virtual ~OSError() {}
};

class APIError : public OSError
{
public:

	APIError(
		const char* fileName,
		int lineNumber,
		const char* function,
		unsigned long error,
		Exception* prev = 0) 
	: OSError(fileName,lineNumber,function,name(error),prev), errorCode(error) {}

	virtual ~APIError()
	{
		LocalFree((void*)Exception::m_name);
	}

	const char* name(int error)
	{
		char* aName;
		unsigned size;

		size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
							NULL,
							error,
							MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
							(LPTSTR) &aName, 0, (char**)NULL);

		if (size) return aName;

		aName = (char*)LocalAlloc(LMEM_FIXED, 32);

		sprintf(aName, "unknown error: %d", error);

		return aName;
	}

protected:

	unsigned long errorCode;
};

#endif

/*
	buffers.h

	$Id: storage.h,v 1.1 2000/10/30 11:38:57 lars Exp $

	Copyright 2000 ibp (uk) Ltd.

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _BUFFERS_H_
#define _BUFFERS_H_ 

#include <stdio.h>
#include <iostream>
#include <string>
#include "exc.h"

class FileDoesNotExist : public Exception
{
public:

    FileDoesNotExist(const char *file, Exception* prev = 0)
		: Exception(0, 0, 0, "FileDoesNotExist", prev), failed(file) {}
    virtual ~FileDoesNotExist() {}

    virtual void printOn(std::ostream& aStream)
	{
		aStream << "file does not exist: " << failed << std::endl;
	}

protected:

	std::string failed;
};

class FileFormatError : public Exception
{
public:

    FileFormatError(const char *file, int lineNumber, const char* function, const char* desc, Exception* prev = 0)
		: Exception(file, line, function, "FileFormatError", prev), description(desc) {}

    virtual ~FileFormatError() {}

    virtual void printOn(std::ostream& aStream)
	{
		Exception::printOn(aStream);

		aStream << ": " << description;
	}

protected:

	const char* description;
};

class ReadError : public Exception
{
public:
	ReadError(const char *file, int lineNumber, const char* function, int e, Exception* prev = 0)
		: Exception(file, lineNumber, function, "ReadError", prev), error(e) {}

	virtual ~ReadError() {}

    virtual void printOn(std::ostream& aStream)
	{
		Exception::printOn(aStream);

		aStream << ": " << _sys_errlist[error];
	}

protected:

	int error;
};

class WriteError : public Exception
{
public:
	WriteError(const char *file, int lineNumber, const char* function, int e, Exception* prev = 0)
		: Exception(file, lineNumber, function, "WriteError", prev), error(e) {}

	virtual ~WriteError() {}

    virtual void printOn(std::ostream& aStream)
	{
		Exception::printOn(aStream);

		aStream << ": " << _sys_errlist[error];
	}

protected:

	int error;
};

class Storage
{
public:

	Storage() {}
	virtual ~Storage() {}

	virtual unsigned read(void* data, unsigned length)  = 0;
	virtual unsigned write(void* data, unsigned length) = 0;

	// setPos operates with byte offset
	virtual int setPos(unsigned pos) = 0;
	virtual unsigned getLength()	 = 0;

	unsigned bytesPerSecond;
	unsigned encoding;
};

class RawFileStorage : public Storage
{
public:

	RawFileStorage(const char* name, bool write = false)
	{
		file = fopen(name, write ? "wb" : "rb");
		if (!file)
		{
			throw FileDoesNotExist(name);
		}
	}

	virtual ~RawFileStorage()
	{
		fclose(file);
	}

	virtual unsigned read(void* data, unsigned length)
	{
		unsigned bytes = fread(data, sizeof(char), length, file);

		if (bytes == 0 && ferror(file))
		{
			throw ReadError(__FILE__, __LINE__, "RawFileStorage::read", errno);
		}

		return bytes;
	}

	virtual unsigned write(void* data, unsigned length)
	{
		unsigned bytes = fwrite(data, sizeof(char), length, file);

		if (bytes == 0 && ferror(file))
		{
			throw WriteError(__FILE__, __LINE__, "RawFileStorage::write", errno);
		}
		
		return bytes;
	}

	// setPos operates with byte offsets
	virtual int setPos(unsigned pos)
	{
		return fseek(file, pos, SEEK_SET) == 0;
	}

	virtual unsigned getLength()
	{
		// we've got no header 

		unsigned pos = ftell(file);

		fseek(file, 0, SEEK_END);

		unsigned bytes = ftell(file);

		fseek(file, pos, SEEK_SET);

		return bytes;
	}

protected:

	FILE* file;
};

#endif
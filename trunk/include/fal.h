/*
	fal.h

	$Id: fal.h,v 1.1 2001/06/16 22:31:33 lars Exp $

	Copyright 1995-2001 Lars Immisch

	Author: Lars Immisch <lars@ibp.de>

	This file was named storage.h until I discovered no file dependent
	on storage.h was rebuilt when I changed it.

	Now there is a storage.h coming with MSVC which is being obsoleted,
	so I guess someone is playing tricks with that special filename.

*/

#ifndef _STORAGE_H_
#define _STORAGE_H_ 

#include <stdio.h>
#include <iostream>
#include <string>
#include "exc.h"

class FileDoesNotExist : public Exception
{
public:

    FileDoesNotExist(const char *file, int line, const char* function, 
		const char *notfound, Exception* prev = 0)
		: Exception(file, line, function, "FileDoesNotExist", prev), failed(notfound) {}
    virtual ~FileDoesNotExist() {}

    virtual void printOn(std::ostream& aStream) const
	{
		Exception::printOn(aStream);

		aStream << ": " << failed;
	}

protected:

	std::string failed;
};

class FileFormatError : public Exception
{
public:

    FileFormatError(const char *file, int line, const char* function, const char* desc, Exception* prev = 0)
		: Exception(file, line, function, "FileFormatError", prev), description(desc) {}

    virtual ~FileFormatError() {}

    virtual void printOn(std::ostream& aStream) const
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
	ReadError(const char *file, int line, const char* function, int e, Exception* prev = 0)
		: Exception(file, line, function, "ReadError", prev), error(e) {}

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
	WriteError(const char *file, int line, const char* function, int e, Exception* prev = 0)
		: Exception(file, line, function, "WriteError", prev), error(e) {}

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
			int x = GetLastError();

			throw FileDoesNotExist(__FILE__, __LINE__, 
				"RawFileStorage::RawFileStorage", name);
		}

		// compute the length - we've got no header 

		fseek(file, 0, SEEK_END);

		length = ftell(file);

		fseek(file, 0, SEEK_SET);
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

	virtual unsigned getLength() { return length; }

protected:

	unsigned length;
	FILE* file;
};

#endif
/*
	buffers.h

	$Id: buffers.h,v 1.1 2000/10/18 16:58:43 lars Exp $

	Copyright 2000 ibp (uk) Ltd.

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _BUFFERS_H_
#define _BUFFERS_H_ 

#include <stdio.h>
#include <iostream>
#include <string>
#include "exc.h"

class Telephone;

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

class BufferStorage
{
public:

	BufferStorage() {}
	virtual ~BufferStorage() {}

	virtual unsigned read(void* data, unsigned length)  = 0;
	virtual unsigned write(void* data, unsigned length) = 0;

	// setPos operates with byte offset
	virtual int setPos(unsigned pos) = 0;
	virtual unsigned getLength()	 = 0;
};

class Buffers
{
public:
	
	Buffers(BufferStorage* aStorage, unsigned numBuffers, unsigned size) 
		: store(aStorage), encoding(0), current(0), flushed(0), bufferSize(size), 
		nBuffers(numBuffers), buffer(0) , status(0 /* r_ok */), bytesPerSecond(8000) 
	{
		setBufferSize(size);
	}

	virtual ~Buffers()	{ delete buffer; delete store; }

	Buffers& operator++()	{ current++; current %= nBuffers; return *this; }

	unsigned read()
	{
		unsigned bytes = store->read(bufferAt(flushed), bufferSize);

		*sizeAt(flushed) = bytes;

		flushed++;
		flushed %= nBuffers;

		return bytes;
	}

	unsigned write()
	{
		size_t bytes = store->write(bufferAt(flushed), *sizeAt(flushed));

		flushed++;
		flushed %= nBuffers;

		return bytes;
	}

	// setPos operates with millisecond offsets
	virtual int setPos(unsigned pos)	{ current = flushed = 0; return store->setPos(pos * bytesPerSecond / 1000); }

	// returns size in ms
	virtual unsigned getLength()		{ return store->getLength() * 1000 / bytesPerSecond; }
	virtual unsigned getSize()			{ return store->getLength(); }

	virtual unsigned getStatus()		{ return status; }

	int isLast() { return *sizeAt(current) < bufferSize; }

	int getNumBuffers() { return nBuffers; }

	void setBufferSize(unsigned size)
	{
		bufferSize = size;
		
		delete buffer;

		buffer = new char[(bufferSize + sizeof(unsigned) + 16) * nBuffers];
	}

	void setEncoding(unsigned anEncoding) { encoding = anEncoding; }
	unsigned getEncoding()		{ return encoding; }

	void setCurrentSize(unsigned size)
	{
		*(unsigned*)&buffer[current * bufferSize] = size;
	}

	unsigned getCurrentSize()	{ return *sizeAt(current); }
	void* getCurrent()	{ return bufferAt(current); }

	void setBytesPerSecond(unsigned bytes)	{ bytesPerSecond = bytes; }
	unsigned getBytesPerSecond()	{ return bytesPerSecond; }

protected:
	
	void* bufferAt(unsigned index)		{ return &buffer[index * (bufferSize + sizeof(unsigned)) + sizeof(unsigned)]; }
	unsigned* sizeAt(unsigned index)	{ return (unsigned*)&buffer[index * (bufferSize + sizeof(unsigned))]; }

	unsigned bytesPerSecond;
	unsigned encoding;
	unsigned current;
	unsigned flushed;
	unsigned bufferSize;
	unsigned nBuffers;
	unsigned status;
	char* buffer;
	BufferStorage* store;
};

class RawFileStorage : public BufferStorage
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

	friend class Buffers;

	FILE* file;
};

#endif
/*
	fal.h

	$Id: fal.h,v 1.3 2003/12/17 23:27:21 lars Exp $

	Copyright 1995-2001 Lars Immisch

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _STORAGE_H_
#define _STORAGE_H_ 

#include <stdio.h>
#include <iostream>
#include <string>
#include <mmsystem.h>
#include <mmreg.h>
#include "exc.h"
#include "smbesp.h"

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


class WavFileStorage : public Storage
{
public:

	WavFileStorage(const char* file, bool write) : position(0), dataOffset(0)
	{
		if (write) 
		{
			openForWriting(file);
		}
		else
		{
			openForReading(file);
		}
	}

	virtual ~WavFileStorage()
	{
		// ascend out of the data chunk
		mmioAscend(hmmio, &subchunk, 0);

		// ascend out of the WAVE chunk
		mmioAscend(hmmio, &parent, 0);

		mmioClose(hmmio, 0);
	}

	virtual unsigned read(void* data, unsigned length)
	{
		// I assume it is possible to read after a completed chunk
		// if there is another chunk, so I make sure this does not happen 
		// by obeying the size of the chunk

		unsigned bytes = min(dataSize - position, length);

		if (bytes)
		{
			bytes = mmioRead(hmmio, (char*)data, bytes);
			if (bytes == -1) 
				throw Exception(__FILE__, __LINE__, "mmioRead()",  "Win32Exception");

			position += bytes;
		}
 
		return bytes;
	}

	virtual unsigned write(void* data, unsigned length)
	{
		unsigned bytes = mmioWrite(hmmio, (const char*)data, length);
		if (bytes == -1) 
		{
			throw Exception(__FILE__, __LINE__, "mmioWrite()", "Win32Exception");
		}
		position += bytes;

		return bytes;
	}

	// setPos operates with byte offset
	virtual int setPos(unsigned pos)
	{
		position = pos;

		return mmioSeek(hmmio, pos, SEEK_SET) != -1;
	}

	virtual unsigned getLength()	{ return dataSize; }

	unsigned getEncoding()	{ return encoding; }

	unsigned findEncoding(WAVEFORMATEX& format)
	{
		if (format.nChannels == 1
			&& format.nSamplesPerSec == 8000
			&& format.wBitsPerSample == 8)
		{
			if (format.wFormatTag == WAVE_FORMAT_ALAW)
				return kSMDataFormat8KHzALawPCM;
			else if (format.wFormatTag == WAVE_FORMAT_MULAW)
				return kSMDataFormat8KHzULawPCM;
		}
		
		throw FileFormatError(__FILE__, __LINE__, "WaveBuffers::findEncoding", "cannot read this Wave Format");

		return 0;
	}

	void openForWriting(const char* file)
	{
		WAVEFORMATEX format;     // possibly partial "FMT" chunk 
 
		// Open the file for writing
		hmmio = mmioOpen((char*)file, NULL, MMIO_WRITE | MMIO_CREATE | MMIO_DENYWRITE);
		if(!hmmio) 
		{ 
			throw FileDoesNotExist(__FILE__, __LINE__, "WaveBuffers::openForWriting", file);
		} 
 
		// Create a "RIFF" chunk with a "WAVE" form type 
		parent.fccType = mmioFOURCC('W', 'A', 'V', 'E'); 
		parent.cksize = 0;
		if (mmioCreateChunk(hmmio, &parent, MMIO_CREATERIFF)) 
		{ 
			mmioClose(hmmio, 0); 
			throw FileFormatError(__FILE__, __LINE__, "WaveBuffers::openForWriting", "cannot create RIFF chunk");
		} 

		// Create a"FMT" chunk (form type "FMT"); it must be 
		// a subchunk of the "RIFF" chunk. 
		subchunk.ckid = mmioFOURCC('f', 'm', 't', ' '); 
		if (mmioCreateChunk(hmmio, &subchunk, 0)) 
		{ 
			mmioClose(hmmio, 0); 
			throw FileFormatError(__FILE__, __LINE__, "WaveBuffers::openForWriting", "cannot create FMT chunk.");
		} 

		format.wFormatTag = WAVE_FORMAT_PCM;
		format.nChannels = 1;
		format.nSamplesPerSec =  11025; 
		format.nAvgBytesPerSec = 11025;
		format.nBlockAlign = 2;
		format.wBitsPerSample = 16;
		format.cbSize = 0; 

		// Write the "FMT" chunk. 
		if (mmioWrite(hmmio, (HPSTR)&format, sizeof(format)) != sizeof(format))
		{ 
			mmioClose(hmmio, 0); 
			throw FileFormatError(__FILE__, __LINE__, "WaveBuffers::openForWriting", "Failed to write format chunk.");
		} 
    
		// Ascend out of the "FMT" subchunk. 
		mmioAscend(hmmio, &subchunk, 0); 
 
		// Create the data subchunk. 
		subchunk.ckid = mmioFOURCC('d', 'a', 't', 'a'); 
		if (mmioCreateChunk(hmmio, &subchunk, 0)) 
		{ 
			mmioClose(hmmio, 0); 
			throw FileFormatError(__FILE__, __LINE__, "WaveBuffers::openForWriting", "cannot create data chunk.");
		}
	}

	void openForReading(const char* file)
	{
		WAVEFORMATEX	format;     // possibly partial "FMT" chunk 
 
		// Open the file for reading with
		hmmio = mmioOpen((char*)file, NULL, MMIO_READ | MMIO_DENYWRITE);
		if(!hmmio) 
		{ 
			throw FileDoesNotExist(__FILE__, __LINE__, "WaveBuffers::openForReading", file);
		} 
 
		// Locate a "RIFF" chunk within a "WAVE" form type to make 
		// sure the file is a waveform-audio file. 
		parent.fccType = mmioFOURCC('W', 'A', 'V', 'E'); 
		if (mmioDescend(hmmio, &parent, NULL, MMIO_FINDRIFF)) 
		{ 
			mmioClose(hmmio, 0); 
			throw FileFormatError(__FILE__, __LINE__, "WaveBuffers::openForReading", "This is not a waveform-audio file.");
		} 

		// Find the "FMT" chunk (form type "FMT"); it must be 
		// a subchunk of the "RIFF" chunk. 
		subchunk.ckid = mmioFOURCC('f', 'm', 't', ' '); 
		if (mmioDescend(hmmio, &subchunk, &parent, MMIO_FINDCHUNK)) 
		{ 
			mmioClose(hmmio, 0); 
			throw FileFormatError(__FILE__, __LINE__, "WaveBuffers::openForReading", "Waveform-audio file has no FMT chunk."); 
		} 
 
		// Read the "FMT" chunk. 
		if (mmioRead(hmmio, (HPSTR)&format, sizeof(format)) != sizeof(format))
		{ 
			mmioClose(hmmio, 0); 
			throw FileFormatError(__FILE__, __LINE__, "WaveBuffers::openForReading", "Failed to read format chunk.");
		} 
    
		// Ascend out of the "FMT" subchunk. 
		mmioAscend(hmmio, &subchunk, 0); 
 
		// Find the data subchunk. The current file position should be at 
		// the beginning of the data chunk; however, you should not make 
		// this assumption. Use mmioDescend to locate the data chunk. 
		subchunk.ckid = mmioFOURCC('d', 'a', 't', 'a'); 
		if (mmioDescend(hmmio, &subchunk, &parent, MMIO_FINDCHUNK)) 
		{ 
			mmioClose(hmmio, 0);
			throw FileFormatError(__FILE__, __LINE__, "WaveBuffers::openForReading", "Waveform-audio file has no data chunk.");
		} 
 
		// Get the size of the data subchunk. 
		dataSize = subchunk.cksize;
		dataOffset = subchunk.dwDataOffset; 

	}

protected:

    HMMIO	 hmmio;
	MMCKINFO subchunk;
	MMCKINFO parent;
	unsigned dataSize;
	unsigned dataOffset;
	unsigned position;
	unsigned encoding;
};

#endif
/*
	fal.h

	$Id$

	Copyright 1995-2004 Lars Immisch

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _STORAGE_H_
#define _STORAGE_H_ 

#include <stdio.h>
#include <iostream>
#include <string>
#include <direct.h>
#include <mmsystem.h>
#include <mmreg.h>
#include "exc.h"
#include "smbesp.h"

class FileDoesNotExist : public Exception
{
public:

    FileDoesNotExist(const char *file, int line, const char* function, 
		const char *notfound, Exception* prev = 0)
		: Exception(file, line, function, "FileDoesNotExist", prev), m_failed(notfound) {}
    virtual ~FileDoesNotExist() {}

    virtual void printOn(std::ostream& aStream) const
	{
		Exception::printOn(aStream);

		aStream << ": " << m_failed;
	}

protected:

	std::string m_failed;
};

class FileFormatError : public Exception
{
public:

    FileFormatError(const char *file, int line, const char* function, const char* desc, Exception* prev = 0)
		: Exception(file, line, function, "FileFormatError", prev), m_description(desc) {}

    virtual ~FileFormatError() {}

    virtual void printOn(std::ostream& aStream) const
	{
		Exception::printOn(aStream);

		aStream << ": " << m_description;
	}

protected:

	const char* m_description;
};

class ReadError : public Exception
{
public:
	ReadError(const char *file, int line, const char* function, int e, Exception* prev = 0)
		: Exception(file, line, function, "ReadError", prev), m_error(e) {}

	virtual ~ReadError() {}

    virtual void printOn(std::ostream& aStream)
	{
		Exception::printOn(aStream);

		aStream << ": " << _sys_errlist[m_error];
	}

protected:

	int m_error;
};

class WriteError : public Exception
{
public:
	WriteError(const char *file, int line, const char* function, int e, Exception* prev = 0)
		: Exception(file, line, function, "WriteError", prev), m_error(e) {}

	virtual ~WriteError() {}

    virtual void printOn(std::ostream& aStream)
	{
		Exception::printOn(aStream);

		aStream << ": " << _sys_errlist[m_error];
	}

protected:

	int m_error;
};

/* filename is a fully qualified file name */
inline int createPath(const char *filename)
{
	char path[_MAX_PATH];
	char *s;
	int rc;

	strcpy(path, filename);

	/* normalize to backslashes - this is Windows */
	for (s = path; *s; ++s)
	{
		if (*s == '/')
			*s = '\\';
	}

	/* don't deal with non-normalized paths (overzealous) */
	if (strstr(path, ".\\") || strstr(path, "..\\"))
		return EINVAL;

	/* Handle drive letter */
	if (strncmp(path + 1, ":\\", 2) == 0)
		s = path + 3;
	else
		s = path;

	for (s = strchr(s, '\\'); s; s = strchr(s + 1, '\\'))
	{
		*s = '\0';
		rc = _mkdir(path);
		if (rc < 0 && errno != EEXIST)
			return errno;

		*s = '\\';
	}
	

	return 0;
}

class Storage
{
public:

	// alaw/mulaw hardcoded via bytes/second 
	Storage(int encoding) : m_encoding(encoding), m_bytesPerSecond(8000) {}
	virtual ~Storage() {}

	virtual unsigned read(void* data, unsigned length)  = 0;
	virtual unsigned write(void* data, unsigned length) = 0;

	// setPos operates with byte offset
	virtual int setPos(unsigned pos) = 0;
	virtual unsigned getLength()	 = 0;

	unsigned m_bytesPerSecond;
	unsigned m_encoding;
};

class RawFileStorage : public Storage
{
public:

	RawFileStorage(const char* name, int encoding, bool write = false) : Storage(encoding),
		m_length(0), m_file(NULL)
	{
		m_file = fopen(name, write ? "wb" : "rb");
		if (!m_file && write)
		{
			createPath(name);
			m_file = fopen(name, "wb");
		}

		if (!m_file)
		{
			throw FileDoesNotExist(__FILE__, __LINE__, 
				"RawFileStorage::RawFileStorage", name);
		}

		// compute the length - we've got no header 

		fseek(m_file, 0, SEEK_END);

		m_length = ftell(m_file);

		fseek(m_file, 0, SEEK_SET);
	}

	virtual ~RawFileStorage()
	{
		fclose(m_file);
	}

	virtual unsigned read(void* data, unsigned length)
	{
		unsigned bytes = fread(data, sizeof(char), length, m_file);

		if (bytes == 0 && ferror(m_file))
		{
			throw ReadError(__FILE__, __LINE__, "RawFileStorage::read", errno);
		}

		return bytes;
	}

	virtual unsigned write(void* data, unsigned length)
	{
		unsigned bytes = fwrite(data, sizeof(char), length, m_file);

		if (bytes == 0 && ferror(m_file))
		{
			throw WriteError(__FILE__, __LINE__, "RawFileStorage::write", errno);
		}
		
		return bytes;
	}

	// setPos operates with byte offsets
	virtual int setPos(unsigned pos)
	{
		return fseek(m_file, pos, SEEK_SET) == 0;
	}

	virtual unsigned getLength() { return m_length; }

protected:

	unsigned m_length;
	FILE* m_file;
};


class WavFileStorage : public Storage
{
public:

	WavFileStorage(const char* file, int encoding, bool write) 
		: Storage(encoding), m_position(0), m_dataOffset(0), m_dataSize(0)
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
		mmioAscend(m_hmmio, &m_subchunk, 0);

		// ascend out of the WAVE chunk
		mmioAscend(m_hmmio, &m_parent, 0);

		mmioClose(m_hmmio, 0);
	}

	virtual unsigned read(void* data, unsigned length)
	{
		// I assume it is possible to read after a completed chunk
		// if there is another chunk, so I make sure this does not happen 
		// by obeying the size of the chunk

		unsigned bytes = min(m_dataSize - m_position, length);

		if (bytes)
		{
			bytes = mmioRead(m_hmmio, (char*)data, bytes);
			if (bytes == -1) 
				throw Exception(__FILE__, __LINE__, "mmioRead()",  "Win32Exception");

			m_position += bytes;
		}
 
		return bytes;
	}

	virtual unsigned write(void* data, unsigned length)
	{
		unsigned bytes = mmioWrite(m_hmmio, (const char*)data, length);
		if (bytes == -1) 
		{
			throw Exception(__FILE__, __LINE__, "mmioWrite()", "Win32Exception");
		}
		m_position += bytes;

		return bytes;
	}

	// setPos operates with byte offset
	virtual int setPos(unsigned pos)
	{
		m_position = pos;

		return mmioSeek(m_hmmio, pos, SEEK_SET) != -1;
	}

	virtual unsigned getLength()	{ return m_dataSize; }

	unsigned getEncoding()	{ return m_encoding; }

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
		m_hmmio = mmioOpen((char*)file, NULL, MMIO_WRITE | MMIO_CREATE | MMIO_DENYWRITE);
		if(!m_hmmio) 
		{
			createPath(file);
			m_hmmio = mmioOpen((char*)file, NULL, MMIO_WRITE | MMIO_CREATE | MMIO_DENYWRITE);
		}
		
		if(!m_hmmio) 
		{ 
			throw FileDoesNotExist(__FILE__, __LINE__, "WaveBuffers::openForWriting", file);
		} 
 
		// Create a "RIFF" chunk with a "WAVE" form type 
		m_parent.fccType = mmioFOURCC('W', 'A', 'V', 'E'); 
		m_parent.cksize = 0;
		if (mmioCreateChunk(m_hmmio, &m_parent, MMIO_CREATERIFF)) 
		{ 
			mmioClose(m_hmmio, 0); 
			throw FileFormatError(__FILE__, __LINE__, "WaveBuffers::openForWriting", "cannot create RIFF chunk");
		} 

		// Create a"FMT" chunk (form type "FMT"); it must be 
		// a subchunk of the "RIFF" chunk. 
		m_subchunk.ckid = mmioFOURCC('f', 'm', 't', ' '); 
		if (mmioCreateChunk(m_hmmio, &m_subchunk, 0)) 
		{ 
			mmioClose(m_hmmio, 0); 
			throw FileFormatError(__FILE__, __LINE__, "WaveBuffers::openForWriting", "cannot create FMT chunk.");
		} 

		switch(m_encoding)
		{
		case kSMDataFormat8KHzALawPCM:
			format.wFormatTag = WAVE_FORMAT_ALAW;
			break;
		case kSMDataFormat8KHzULawPCM:
			format.wFormatTag = WAVE_FORMAT_MULAW;
			break;
		default:
			throw FileFormatError(__FILE__, __LINE__, "WaveBuffers::openForWriting", 
				"unknown format");
			break;
		}
		format.nChannels = 1;
		format.nSamplesPerSec =  8000; 
		format.nAvgBytesPerSec = 8000;
		format.nBlockAlign = 1;
		format.wBitsPerSample = 8;
		format.cbSize = 0; 

		// Write the "FMT" chunk. 
		if (mmioWrite(m_hmmio, (HPSTR)&format, sizeof(format)) != sizeof(format))
		{ 
			mmioClose(m_hmmio, 0); 
			throw FileFormatError(__FILE__, __LINE__, "WaveBuffers::openForWriting", "Failed to write format chunk.");
		} 
    
		// Ascend out of the "FMT" subchunk. 
		mmioAscend(m_hmmio, &m_subchunk, 0); 
 
		// Create the data subchunk. 
		m_subchunk.ckid = mmioFOURCC('d', 'a', 't', 'a'); 
		if (mmioCreateChunk(m_hmmio, &m_subchunk, 0)) 
		{ 
			mmioClose(m_hmmio, 0); 
			throw FileFormatError(__FILE__, __LINE__, "WaveBuffers::openForWriting", "cannot create data chunk.");
		}
	}

	void openForReading(const char* file)
	{
		WAVEFORMATEX	format;     // possibly partial "FMT" chunk 
 
		// Open the file for reading with
		m_hmmio = mmioOpen((char*)file, NULL, MMIO_READ | MMIO_DENYWRITE);
		if(!m_hmmio) 
		{ 
			throw FileDoesNotExist(__FILE__, __LINE__, "WaveBuffers::openForReading", file);
		} 
 
		// Locate a "RIFF" chunk within a "WAVE" form type to make 
		// sure the file is a waveform-audio file. 
		m_parent.fccType = mmioFOURCC('W', 'A', 'V', 'E'); 
		if (mmioDescend(m_hmmio, &m_parent, NULL, MMIO_FINDRIFF)) 
		{ 
			mmioClose(m_hmmio, 0); 
			throw FileFormatError(__FILE__, __LINE__, "WaveBuffers::openForReading", "This is not a waveform-audio file.");
		} 

		// Find the "FMT" chunk (form type "FMT"); it must be 
		// a subchunk of the "RIFF" chunk. 
		m_subchunk.ckid = mmioFOURCC('f', 'm', 't', ' '); 
		if (mmioDescend(m_hmmio, &m_subchunk, &m_parent, MMIO_FINDCHUNK)) 
		{ 
			mmioClose(m_hmmio, 0); 
			throw FileFormatError(__FILE__, __LINE__, "WaveBuffers::openForReading", "Waveform-audio file has no FMT chunk."); 
		} 
 
		// Read the "FMT" chunk. 
		if (mmioRead(m_hmmio, (HPSTR)&format, sizeof(format)) != sizeof(format))
		{ 
			mmioClose(m_hmmio, 0); 
			throw FileFormatError(__FILE__, __LINE__, "WaveBuffers::openForReading", "Failed to read format chunk.");
		} 
    
		// Ascend out of the "FMT" subchunk. 
		mmioAscend(m_hmmio, &m_subchunk, 0); 
 
		// Find the data subchunk. The current file position should be at 
		// the beginning of the data chunk; however, you should not make 
		// this assumption. Use mmioDescend to locate the data chunk. 
		m_subchunk.ckid = mmioFOURCC('d', 'a', 't', 'a'); 
		if (mmioDescend(m_hmmio, &m_subchunk, &m_parent, MMIO_FINDCHUNK)) 
		{ 
			mmioClose(m_hmmio, 0);
			throw FileFormatError(__FILE__, __LINE__, "WaveBuffers::openForReading", "Waveform-audio file has no data chunk.");
		} 
 
		// Get the size of the data subchunk. 
		m_dataSize = m_subchunk.cksize;
		m_dataOffset = m_subchunk.dwDataOffset; 

	}

protected:

    HMMIO	 m_hmmio;
	MMCKINFO m_subchunk;
	MMCKINFO m_parent;
	unsigned m_dataSize;
	unsigned m_dataOffset;
	unsigned m_position;
};

#endif
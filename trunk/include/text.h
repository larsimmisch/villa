/*
	TCP.h
*/

#ifndef _TEXT_H_
#define _TEXT_H_

#include <iostream>
#include <vector>
#include <assert.h>
#include "socket.h"
#include "omnithread.h"
#include "client.h"

// The streambuf implementation uses its own buffers instead of the buffers
// in basic_streambuf - maybe this could be done more elegantly

class SocketStream : public std::basic_iostream<char>,
					 public std::basic_streambuf<char>,
					 public Socket
{
public:

	typedef std::basic_streambuf<char>::int_type int_type;

    enum { indefinite = -1 };
	
	SocketStream(int protocol = PF_INET, int s = -1);
	SocketStream(const Socket &socket);
	virtual ~SocketStream();
		
	void lock() { m_mutex.lock(); }
	void unlock() { m_mutex.unlock(); }

	// lock the mutex. unlocking is done via the io manipulator end()
	SocketStream &begin()
	{
		lock();

		clear();
	
		return *this;
	}

	// streambuf protocol

	virtual int_type overflow(int_type c)
	{
		m_pbuf += c;
		
		return c;
	}

	virtual std::streamsize xsputn(const char *s, std::streamsize l)
	{
		m_pbuf += s;

		return l;
	}

	virtual int_type underflow()
	{
		if (m_gpos >= m_gsize)
			return std::char_traits<char>::eof();

		return m_gbuf[m_gpos];
	}

	virtual int_type uflow()
	{
		if (m_gpos >= m_gsize)
			return std::char_traits<char>::eof();

		return m_gbuf[m_gpos++];
	}

	virtual std::streamsize xsgetn(char *s, std::streamsize l)
	{
		if (m_gpos >= m_gsize)
			return 0;

		int x = m_gsize - m_gpos;
		int m = l > x ? x : l;

		memmove(s, m_gbuf + m_gpos, m);

		m_gpos += m;

		return m;
	}

	virtual unsigned send();
	virtual unsigned receive();	

protected:
	
	friend std::ostream& text_end(std::ostream& s);

	// helper methods
	void grow_rbuf();

	// fills the streambuf get area from the receive buffer
	unsigned fillGBuf();
			
	// streambuf put area
	std::string m_pbuf;
	// streambuf get area
	char *m_gbuf;
	int m_gpos;
	int m_gsize;
	int m_gmax;
	// receive buffer
	char *m_rbuf;
	int m_rsize;
	int m_rmax;
	omni_mutex m_mutex;
};

struct textmanip
{
	textmanip(std::ostream& (*f)(std::ostream&)) 
		: function(f) {}

	std::ostream& (*function)(std::ostream&);
};

inline std::ostream& operator<<(std::ostream& os, textmanip& l)
{
	return l.function(os);
}

// helper
inline std::ostream& text_end(std::ostream& s)
{
	SocketStream &t = dynamic_cast<SocketStream&>(s);

	t << "\r\n";

	t.send();

	t.unlock();

	return s;
}

inline textmanip end()
{
	return textmanip(text_end);
}

#endif /* _TEXT_H_ */

/*
	Text.cpp
*/

#include <iostream>
#include <stdio.h>
#include <new.h>
#include <assert.h>
#include "log.h"
#include "text.h"

static char get_lost[] = "finger weg!\r\n";

#define RMIN 256
#define RSIZE 1024
#define GSIZE 1024

SocketStream::SocketStream(const Socket &socket) : 
	Socket(socket.protocol(), socket.fd()),
	m_gbuf(0), m_gpos(0), m_gsize(0), m_gmax(GSIZE),
	m_rbuf(0), m_rpos(0), m_rsize(0), m_rmax(RSIZE),
	std::basic_iostream<char>(this)
{
	m_gbuf = new char[m_gmax];
	m_rbuf = new char[m_rmax];
	m_rbuf[0] = '\0';
	m_pbuf.reserve(512);

	setbuf(0, 0);
}

SocketStream::SocketStream(int protocol, int s) : 
	Socket(protocol, s),
	m_gbuf(0), m_gpos(0), m_gsize(0), m_gmax(GSIZE),
	m_rbuf(0), m_rpos(0), m_rsize(0), m_rmax(RSIZE),
	std::basic_iostream<char>(this)
{
	m_gbuf = new char[m_gmax];
	m_rbuf = new char[m_rmax];
	m_rbuf[0] = '\0';
	m_pbuf.reserve(512);

	setbuf(0, 0);
}


SocketStream::~SocketStream()
{
}

unsigned SocketStream::send()
{
	assert(m_pbuf.size());

	std::ostream &o = log(log_info, "text") << m_remote << " sent: ";
	o.write(m_pbuf.c_str(), m_pbuf.size() - 2);
	o << logend();

	int rc = Socket::send((void*)m_pbuf.c_str(), m_pbuf.size());
	if (rc == 0)
	{	
		return 0;
	}

	m_pbuf.erase();

    return rc;
}

unsigned SocketStream::fillGBuf()
{
	if (strlen(m_rbuf + m_rpos) == 0)
		return 0;

	char *e = strstr(m_rbuf + m_rpos, "\r\n");

	if (!e)
		return 0;

	int p = e - m_rbuf - m_rpos;

	// grow the buffer if necessary - take trailing "\n\0" into account
    if (p + 2 >= m_gmax)
    {
		m_gmax *= 2;

		log(log_info, "text") << m_remote << " growing get buffer to " << m_gmax << logend();

		char *newbuf = new char[m_gmax];

		assert(newbuf);

		delete m_gbuf;
		m_gbuf = newbuf;
    }

	memcpy(m_gbuf, m_rbuf + m_rpos, p);
	m_gbuf[p] = '\n';
	m_gbuf[p+1] = '\0';

	m_gpos = 0;
	m_gsize = p + 1;
	m_rpos += p + 2;

	std::ostream &o = log(log_info, "text") << m_remote << " received: ";
	o.write(m_gbuf, p);
	o << logend();

	return m_gsize;
}

void SocketStream::grow_rbuf()
{
	char *newbuf = new char[m_rmax*2];
	memcpy(newbuf, m_rbuf, m_rsize);
	m_rmax *= 2;
	delete m_rbuf;
	m_rbuf = newbuf;

	log(log_info, "text") << m_remote << " growing read buffer to " << m_rmax << logend();
 }

unsigned SocketStream::receive()
{
	unsigned len = fillGBuf();

	if (len)
		return len;

	m_rsize = 0;
	m_rpos = 0;
	m_rbuf[0] = '\0';

	for(;;)
	{	
		unsigned rcvd = Socket::receive(m_rbuf + m_rsize, m_rmax - m_rsize - 1);
		if (rcvd <= 0)
		{
			log(log_warning, "text") << m_remote << " receive returned: " << GetLastError()
				<< logend();
			return 0;
		}
		
		m_rsize += rcvd;
		m_rbuf[m_rsize] = '\0';

		len = fillGBuf();
		if (len)
			return len;

		// grow the buffer if the remaining buffer size is below RMIN
        if (m_rmax - m_rsize < RMIN)
        {
			grow_rbuf();
	    }
	}

    // unreached statement
	return m_rsize;
}

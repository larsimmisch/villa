/*
	Text.cpp
*/

#include <iostream>
#include <stdio.h>
#include <new.h>
#include "Text.h"

static char get_lost[] = "finger weg!\r\n";

TextTransport::TextTransport(void* aPrivateData) : m_socket(), 
	m_privateData(aPrivateData), m_state(idle), m_gbuf(0), m_gpos(0), 
	m_gsize(0), m_gmax(256),
	std::basic_iostream<char>(this)
{
	m_gbuf = new char[256];
	m_pbuf.reserve(256);

	setbuf(0, 0);
}

TextTransport::~TextTransport()
{
	if (m_state == connected)	
		abort();
	
	m_socket.close();
}

void TextTransport::assertState(states aState)
{	
	if (aState != getState())
	{
		throw "invalid state";
	} 
}

int TextTransport::listen(SAP& aLocalSAP, unsigned aTimeout, int single)
{	
	assertState(idle);
	
	m_local = aLocalSAP;
	m_timeout = aTimeout;
	m_socket.bind(aLocalSAP, single);

	setState(listening);
	
	return doListen();
}

int TextTransport::connect(SAP& aRemoteSAP, unsigned aTimeout)
{
	assertState(idle);
	
	m_remote = aRemoteSAP;
	m_timeout = aTimeout;
	
	setState(calling);
	
	return doConnect();
}

void TextTransport::accept()
{
	assertState(connecting);
	
	setState(connected);
}

void TextTransport::reject()
{
	assertState(connecting);

	setState(idle);

	m_socket.close();	
}

void TextTransport::disconnect(unsigned aTimeout)
{	
	m_timeout = aTimeout;

	switch (m_state)
    {
    case idle:
        return;
    case listening:
    case calling:
    case connected:
        m_socket.close();
        setState(idle);
        break;
    case disconnecting:
        disconnectAccept();
        break;
    default:
        throw "disconnect in invalid state";
    }
	
}

int TextTransport::disconnectAndWait(unsigned aTimeout)
{
    unsigned received;

    disconnect(aTimeout);

    while (m_state != idle)
    {
        received = receiveRaw(m_timeout);
        if (received == 0)  return 0;
    }

    return 1;
}

void TextTransport::disconnectAccept()
{
	assertState(disconnecting);
	
	setState(idle);
	
	m_socket.close();	
}

void TextTransport::abort()
{
	assertState(connected);
	
	setState(idle);

	m_socket.close();
}

unsigned TextTransport::sendRaw(const char *data, unsigned size,
								unsigned aTimeout, int expedited)
{
	int rc;
	unsigned sent = 0;

	while (sent < size)
	{
		m_socket.waitForSend(aTimeout);
		rc = m_socket.send((void*)&data[sent], size - sent, expedited);
		if (rc == 0)
		{	
			if (m_state == connected)
				aborted();

			return 0;
		}
		
		sent += rc;
	}

    return sent;
}

unsigned TextTransport::receiveRaw(unsigned aTimeout)
{
	unsigned rcvd;

	for (;m_gpos < m_gsize-1; ++m_gpos)
	{
		if (m_gbuf[m_gpos] == '\r' && m_gbuf[m_gpos+1] == '\n')
		{
			m_gpos += 2;
			break;
        }
	}

	if (m_gpos < m_gsize - 2)
		return m_gsize - m_gpos;

	m_gsize = 0;
	m_gpos = 0;

	while (true)
	{
		if ((m_timeout = m_socket.waitForData(aTimeout)) == 0)	
		{
			return 0;
		}
	
		rcvd = m_socket.receive(&m_gbuf[m_gsize], m_gmax - m_gsize);
		if (rcvd == 0)
		{
			if (m_state == connected)
				aborted();

			return 0;
		}
		
		m_gsize += rcvd;
		
        if (m_gsize >= 2) for (unsigned i = 0; i < m_gsize-1; ++i)
        {
            if (m_gbuf[i] == '\r' && m_gbuf[i+1] == '\n')
            {
                return i;
            }
        }

		// grow the buffer if necessary
        if (m_gsize == m_gmax)
        {
			char *newbuf = new char[m_gmax*2];
			memcpy(newbuf, m_gbuf, m_gmax);
			m_gmax *= 2;
			delete m_gbuf;
			m_gbuf = newbuf;
        }
	}

    // unreached statement
	return m_gsize;
}

void TextTransport::aborted()
{
	setState(idle);
	
	m_socket.close();
}

void TextTransport::fatal(char* error)
{
	throw "TextTransport::fatal";
}

int TextTransport::doListen()
{
    while(1)
    {
   		m_timeout = m_socket.listen(m_remote, m_timeout);
    	if (m_timeout == 0)
    	{
    		setState(idle);
    		m_socket.close();
    		return 0;
    	}
        break;
	}
	m_socket.setLingerTimeout(2);
	m_socket.setNoDelay(1);
	m_socket.setKeepAlive(1);

	setState(connecting);

	return 1;
}

int TextTransport::doConnect()
{
	m_timeout = m_socket.connect(m_remote, m_timeout);
	if (m_timeout == 0)
	{
		setState(idle);
		m_socket.close();
		return 0;
	}

	m_socket.setLingerTimeout(2);
	m_socket.setNoDelay(1);
	m_socket.setKeepAlive(1);

	setState(connected);
    return 1;
}


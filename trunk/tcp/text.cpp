/*
	Text.cpp
*/

#include <iostream>
#include <stdio.h>
#include <new.h>
#include "Text.h"

static char get_lost[] = "finger weg!\r\n";

TextTransport::TextTransport(void* aPrivateData) : socket(), 
	privateData(aPrivateData), state(idle), m_gbuf(0), m_gpos(0), m_gmax(256),
	std::basic_iostream<char>(this)
{
	m_gbuf = new char[256];
	m_pbuf.reserve(256);

	setbuf(0, 0);
}

TextTransport::~TextTransport()
{
	if (state == connected)	abort();
	
	socket.close();
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
	
	local = aLocalSAP;
	timeout = aTimeout;
	socket.bind(aLocalSAP, single);

	setState(listening);
	
	return doListen();
}

int TextTransport::connect(SAP& aRemoteSAP, unsigned aTimeout)
{
	assertState(idle);
	
	remote = aRemoteSAP;
	timeout = aTimeout;
	
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

	socket.close();	
}

void TextTransport::disconnect(unsigned aTimeout)
{	
	timeout = aTimeout;

	switch (state)
    {
    case idle:
        return;
    case listening:
    case calling:
    case connected:
        socket.close();
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

    while (state != idle)
    {
        received = receiveRaw(timeout);
        if (received == 0)  return 0;
    }

    return 1;
}

void TextTransport::disconnectAccept()
{
	assertState(disconnecting);
	
	setState(idle);
	
	socket.close();	
}

void TextTransport::abort()
{
	assertState(connected);
	
	setState(idle);

	socket.close();
}

unsigned TextTransport::sendRaw(const char *data, unsigned size,
								unsigned aTimeout, int expedited)
{
	int rc;
	unsigned sent = 0;

	while (sent < size)
	{
		socket.waitForSend(aTimeout);
		rc = socket.send((void*)&data[sent], size - sent, expedited);
		if (rc == 0)
		{	
			if (state == connected)  
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

	m_gsize = 0;
	m_gpos = 0;

	while(1)
	{
		if ((timeout = socket.waitForData(aTimeout)) == 0)	
		{
			return 0;
		}
	
		rcvd = socket.receive(&m_gbuf[m_gsize], m_gmax - m_gsize);
		if (rcvd == 0)
		{
			if (state == connected)
				aborted();

			return 0;
		}
		
		m_gsize += rcvd;
		
        if (m_gsize >= 2) for (unsigned i = 0; i < m_gsize-1; ++i)
        {
            if (m_gbuf[i] == '\r' && m_gbuf[i+1] == '\n')
            {
                return m_gsize;
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
	
	socket.close();
}

void TextTransport::fatal(char* error)
{
	throw "TextTransport::fatal";
}

int TextTransport::doListen()
{
    while(1)
    {
   		timeout = socket.listen(remote, timeout);
    	if (timeout == 0)
    	{
    		setState(idle);
    		socket.close();
    		return 0;
    	}
        break;
	}
	socket.setLingerTimeout(2);
	socket.setNoDelay(1);
	socket.setKeepAlive(1);

	setState(connecting);

	return 1;
}

int TextTransport::doConnect()
{
	timeout = socket.connect(remote, timeout);
	if (timeout == 0)
	{
		setState(idle);
		socket.close();
		return 0;
	}

	socket.setLingerTimeout(2);
	socket.setNoDelay(1);
	socket.setKeepAlive(1);

	setState(connected);
    return 1;
}


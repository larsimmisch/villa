/*
	Text.cpp
*/

#include <iostream>
#include <stdio.h>
#include <new.h>
#include <assert.h>
#include "Text.h"

static char get_lost[] = "finger weg!\r\n";

TextTransport::TextTransport(TextTransportClient& client, void* userData) : 
	m_socket(), m_client(client), m_userData(userData), m_state(idle), 
	m_gbuf(0), m_gpos(0), 
	m_gsize(0), m_gmax(256),
	std::basic_iostream<char>(this),
	m_event(&m_mutex)
{
	m_gbuf = new char[256];
	m_pbuf.reserve(256);

	setbuf(0, 0);
}

TextTransport::~TextTransport()
{
	setState(dying);

	m_event.signal();

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

int TextTransport::listen(SAP& aLocalSAP, int single)
{	
	assertState(idle);
	
	m_local = aLocalSAP;
	m_socket.bind(aLocalSAP, single);

	setState(listening);
	
	m_event.signal();

	return 0;
}

int TextTransport::connect(SAP& aRemoteSAP)
{
	assertState(idle);
	
	m_remote = aRemoteSAP;
	
	setState(calling);
	
	m_event.signal();

	return 0;
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

void TextTransport::disconnect()
{	
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

unsigned TextTransport::send()
{
	//assert(m_pbuf.size());

	if (!m_pbuf.size())
		return 0;

	int rc = m_socket.send((void*)m_pbuf.c_str(), m_pbuf.size());
	if (rc == 0)
	{	
		if (m_state == connected)
			aborted();

		return 0;
	}

	m_pbuf.erase();

    return rc;
}

unsigned TextTransport::receive()
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

	getClient().abort(this);
}

int TextTransport::doListen()
{
	m_socket.listen(m_remote);

	m_socket.setLingerTimeout(2);
	m_socket.setNoDelay(1);
	m_socket.setKeepAlive(1);

	setState(connecting);

	return 1;
}

int TextTransport::doConnect()
{
	m_socket.connect(m_remote);

	m_socket.setLingerTimeout(2);
	m_socket.setNoDelay(1);
	m_socket.setKeepAlive(1);

	setState(connected);
    return 1;
}

void TextTransport::setState(states aState)
{
	if (m_state != aState)
	{
		m_mutex.lock();
		m_state = aState;
		m_mutex.unlock();
	}
}

void TextTransport::run()
{
    try
    {
        int result;

    	while(1)
    	{
    		switch (getState())
    		{
    		case idle:
    			m_event.wait();		
    			break;
    		case listening:
    			result = TextTransport::doListen();
    			if (!result) getClient().connectRequestTimeout(this);
    			else getClient().connectRequest(this, m_remote);
    			break;
    		case calling:
    			result = TextTransport::doConnect();
    			if (result == 0)
    			{
					getClient().connectReject(this);
    			}
    			else getClient().connectConfirm(this);
    			break;
    		case disconnecting:
    		case connected:
    			result = receive();
                if (result == 0)    
					break;

            	getClient().data(this);
    			break;
    		case dying:
    			return;
    		default:
    			break;
    		}
    	}	
    }
    catch (const char* e)
    {
        getClient().fatal(e);
    }
    catch(...)
    {
        getClient().fatal("unknown exception in thread AsyncText");
    }
}

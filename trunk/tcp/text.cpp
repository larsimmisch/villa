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

TextTransport::TextTransport(TextTransportClient& client, void* userData) : 
	m_socket(), m_client(client), m_userData(userData), m_state(idle), 
	m_gbuf(0), m_gpos(0), m_gsize(0), m_gmax(512),
	m_rbuf(0), m_rpos(0), m_rsize(0), m_rmax(512),
	std::basic_iostream<char>(this),
	m_event(&m_mutex)
{
	m_gbuf = new char[m_gmax];
	m_rbuf = new char[m_rmax];
	m_rbuf[0] = '\0';
	m_pbuf.reserve(512);

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
	assert(m_pbuf.size());

	std::ostream &o = log(log_debug, "text") << "sent: ";
	o.write(m_pbuf.c_str(), m_pbuf.size() - 2);
	o << logend();

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

unsigned TextTransport::fillGBuf()
{
	if (strlen(m_rbuf) == 0)
		return 0;

	char *e = strstr(m_rbuf + m_rpos, "\r\n");

	if (!e)
		return 0;

	int p = e - m_rbuf - m_rpos;

	// grow the buffer if necessary - take trailing "\n\0" into account
    if (p + 2 >= m_gmax)
    {
		m_gmax *= 2;
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

	return m_gsize;
}

unsigned TextTransport::receive()
{
	unsigned len = fillGBuf();

	if (len)
		return len;

	m_rsize = 0;
	m_rpos = 0;
	m_rbuf[0] = '\0';

	for(;;)
	{	
		unsigned rcvd = m_socket.receive(&m_rbuf[m_rsize], m_rmax - m_rsize - 1);
		if (rcvd == 0)
		{
			if (m_state == connected)
				aborted();

			return 0;
		}
		
		m_rsize += rcvd;
		m_rbuf[m_rsize] = '\0';

		len = fillGBuf();

		if (len)
			return len;

		// grow the buffer if necessary
        if (m_rsize >= m_rmax)
        {
			char *newbuf = new char[m_rmax*2];
			memcpy(newbuf, m_rbuf, m_rmax);
			m_rmax *= 2;
			delete m_rbuf;
			m_rbuf = newbuf;
        }
	}

    // unreached statement
	return m_rsize;
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
				{
    			result = receive();
                if (result == 0)    
					break;

				// don't print trailing newline
				std::ostream o = log(log_debug, "text") << "received: ";
				o.write(m_gbuf, m_gsize - 1) ;
				o << logend();

				clear();

            	getClient().data(this);
				}
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

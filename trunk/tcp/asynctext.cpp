/*
	AsyncText.cpp
*/

#include <stdlib.h>
#include "exc.h"
#include "asynctext.h"


AsyncText::AsyncText(TextTransportClient& aClient, void* aPrivateData)
	: TextTransport(aPrivateData), m_client(aClient), m_event(&m_mutex)
{
}

AsyncText::~AsyncText()
{
	setState(dying);

	m_socket.close();

	m_event.signal();
}

void AsyncText::aborted()
{
    TextTransport::aborted();

	getClient().abort(this);
}

void AsyncText::setState(states aState)
{
	if (TextTransport::m_state != aState)
	{
		m_mutex.lock();
		TextTransport::setState(aState);
		m_mutex.unlock();
	}
}

void AsyncText::fatal(char* error)
{
	throw "AsyncText::fatal";
}

void AsyncText::run()
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
    				if (m_timeout == 0)	
						getClient().connectTimeout(this);
    				else
						getClient().connectReject(this);
    			}
    			else getClient().connectConfirm(this);
    			break;
    		case disconnecting:
    		case connected:
    			result = receiveRaw();
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


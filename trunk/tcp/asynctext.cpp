/*
	AsyncText.cpp
*/

#include <stdlib.h>
#include "exc.h"
#include "asynctext.h"


AsyncText::AsyncText(TextTransportClient& aClient, void* aPrivateData)
	: TextTransport(aPrivateData), client(aClient), incoming(), event(&mutex)
{
	start_undetached();
}

AsyncText::~AsyncText()
{
	setState(dying);

	socket.close();

	event.signal();
}

void AsyncText::send(const char* aPacket, int expedited)
{
	mutex.lock();
	if (TextTransport::state != connected)	
	{
		mutex.unlock();
		return;	
	}
	
	TextTransport::send(aPacket, expedited);
	mutex.unlock();
}

char* AsyncText::receive()
{
	char* aPacket = (char*)incoming.dequeue();
	
	return aPacket;
}

void AsyncText::aborted()
{
    TextTransport::aborted();

	getClient().abort(this);
}

void AsyncText::setState(states aState)
{
	if (TextTransport::state != aState)
	{
		mutex.lock();
		TextTransport::setState(aState);
		mutex.unlock();
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
        char* packet;

    	while(1)
    	{
    		switch (getState())
    		{
    		case idle:
    			event.wait();		
    			break;
    		case listening:
    			result = TextTransport::doListen();
    			if (!result) getClient().connectRequestTimeout(this);
    			else getClient().connectRequest(this, remote);
    			break;
    		case calling:
    			result = TextTransport::doConnect();
    			if (result == 0)
    			{
    				if (timeout == 0)	getClient().connectTimeout(this);
    				else	getClient().connectReject(this);
    			}
    			else getClient().connectConfirm(this);
    			break;
    		case disconnecting:
    		case connected:
    			packet = receiveRaw();
                if (packet == 0)    break;
            	if(getClient().asynchronous(this))
                {
                	incoming.enqueue(packet);
                    getClient().dataReady(this);
                }
            	else getClient().data(this, packet);
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


/*
	AsyncTCP.cpp
*/

#include <stdlib.h>
#include "exc.h"
#include "asynctcp.h"

AsyncTCPNoThread::AsyncTCPNoThread(TransportClient& aClient, void* aPrivateData)
	: TCP(aPrivateData), client(aClient), incoming(), event()
{
}

AsyncTCPNoThread::~AsyncTCPNoThread()
{
	setState(dying);

	socket.close();

	event.post();
}

void AsyncTCPNoThread::send(Packet& aPacket, int expedited)
{
	mutex.lock();
	if (state != connected)	
	{
		mutex.unlock();
		return;	
	}
	
	TCP::send(aPacket, expedited);
	mutex.unlock();
}

Packet* AsyncTCPNoThread::receive()
{
	Packet* aPacket = (Packet*)incoming.dequeue();
	
	return aPacket;
}

void AsyncTCPNoThread::checkForControlPacket(Packet* aPacket)
{
	if(!aPacket->hasControlData())
	{
		return;
	}
	
	switch((Transport::packets)aPacket->getControlContent())
	{
	case t_disconnect_request:
		// t_disconnect_request_packets from both sides might have met in the middle of their way...
		if (state == disconnecting)
		{
			disconnectAccept();
			socket.close();
			getClient().disconnectConfirm(this, aPacket->getNumArgs() == 0 ? 0 : aPacket);
			break;
		}
		setState(disconnecting);
		getClient().disconnectRequest(this, aPacket->getNumArgs() == 0 ? 0 : aPacket);
		break;
	case t_disconnect_reject:
		setState(connected);
		getClient().disconnectReject(this, aPacket->getNumArgs() == 0 ? 0 : aPacket);
		break;
	case t_disconnect_confirm:
		setState(idle);
		getClient().disconnectConfirm(this, aPacket->getNumArgs() == 0 ? 0 : aPacket);
		socket.close();
		break;
	case t_abort:
		packet = aPacket;
		aborted();
		break;
	default:
		return;
	}
}

void AsyncTCPNoThread::aborted()
{
    TCP::aborted();

	getClient().abort(this, packet);
}

void AsyncTCPNoThread::setState(states aState)
{
	if (state != aState)
	{
		mutex.lock();
		TCP::setState(aState);
		mutex.unlock();
	}
}

Transport::states AsyncTCPNoThread::getState()
{
	states aState;
	
	mutex.lock();
	aState = TCP::getState();
	mutex.unlock();

	return aState;

}

void AsyncTCPNoThread::fatal(char* error)
{
	throw "AsyncTCPNoThread::fatal";
}

void AsyncTCPNoThread::run()
{
    try
    {
    	while(1)
    	{
    		switch (getState())
    		{
    		case idle:
    			event.wait();
    			break;
    		case listening:
    			packet = TCP::doListen();

				if (getState() == dying)	return;

    			if (packet == 0) getClient().connectRequestTimeout(this);
    			else
				{
					getClient().connectRequest(this, remote, packet);
					delete packet;
				}
    			break;
    		case calling:
    			packet = TCP::doConnect();

				if (getState() == dying)	return;
    			
				if (packet == 0)
    			{
    				if (timeout == 0)	getClient().connectTimeout(this);
    				else	getClient().connectReject(this, 0);
    			}
    			else
    			{
    				if (packet->getControlContent() == t_connect_confirm)
    						getClient().connectConfirm(this, packet);
    				else	getClient().connectReject(this, packet);

					delete packet;
    			}
    			break;
    		case disconnecting:
    			packet = receiveRaw();
				if (packet)
				{
					if (packet->hasControlData())
					{
						if (packet->getControlContent() == t_disconnect_reject)
						{
							getClient().disconnectReject(this, packet->getNumArgs() ? packet : 0);
							setState(connected);
						}
						if (packet->getControlContent() == t_disconnect_confirm)
						{
							getClient().disconnectConfirm(this, packet->getNumArgs() ? packet : 0);
							setState(connected);
						}
					}
					else
					{
            			if(getClient().asynchronous(this))
						{
                			incoming.enqueue(packet);
							getClient().dataReady(this);
						}
            			else getClient().data(this, packet);
					}
					delete packet;
				}
				else
				{
	                getClient().disconnectConfirm(this);
					setState(idle);
				}
				break;
    		case connected:
    			packet = receiveRaw();
                if (packet == 0) break;
            	if(getClient().asynchronous(this))
                {
                	incoming.enqueue(packet);
                    getClient().dataReady(this);
                }
            	else getClient().data(this, packet);
				delete packet;
    			break;
    		case dying:
    			return;
    		default:
    			break;
    		}
    	}	
    }
    catch (Exception& e)
    {
        getClient().fatal(e);
    }
    catch (const char* e)
    {
        getClient().fatal(e);
    }
}

AsyncTCP::AsyncTCP(TransportClient& aClient, void* aPrivateData)
	: AsyncTCPNoThread(aClient, aPrivateData)
{
    resume();
}

AsyncTCP::~AsyncTCP()
{
	setState(dying);

	socket.close();

	event.post();

	// if I'm not the signalling thread, wait for it to exit
	if (!isSelf()) Thread::wait();
}

void AsyncTCP::run()
{ 
	AsyncTCPNoThread::run(); 
}
/*
	TCP.cpp
*/

#include <iostream.h>
#include <new.h>
#include "tcp.h"

static char get_lost[] = "finger weg!\n";

TCP::TCP(void* aPrivateData) : Transport(aPrivateData), socket(), packet(0) 
{
}

TCP::~TCP()
{	
	socket.close();
}

void TCP::assertState(states aState)
{	
	if (aState != getState())
	{
		throw "invalid state";
	} 
}

Packet* TCP::listen(SAP& aLocalSAP, unsigned aTimeout, int single)
{	
	assertState(idle);
	
	timeout = aTimeout;

	socket.bind(aLocalSAP, single);
	local = aLocalSAP;

	setState(listening);
	
	Packet* reply = doListen();

	return reply;
}

Packet* TCP::connect(SAP& aRemoteSAP, unsigned aTimeout, Packet* initialPacket)
{
	assertState(idle);
	
	remote = aRemoteSAP;
	timeout = aTimeout;
	packet = initialPacket;
	
	setState(calling);

	return doConnect();
}

void TCP::accept(Packet* aPacket)
{
	assertState(connecting);
	
	setState(connected);

	sendControl(aPacket, t_connect_confirm);
}

void TCP::reject(Packet* aPacket)
{
	assertState(connecting);

	setState(idle);
	
	sendControl(aPacket, t_connect_reject);

	socket.close();	
}

void TCP::disconnect(unsigned aTimeout, Packet* aPacket)
{	
	timeout = aTimeout;

	switch (state)
    {
    case idle:
        return;
    case listening:
    case connecting:
        socket.close();
        setState(idle);
		local.clear();
		remote.clear();
        break;
    case connected:
    	setState(disconnecting);
    	sendControl(aPacket, t_disconnect_request);
        break;
    case disconnecting:
        disconnectAccept();
        break;
    default:
        throw "disconnect in invalid state";
    }
	
}

int TCP::disconnectAndWait(unsigned aTimeout, Packet* aPacket)
{
    Packet* received;

    disconnect(aTimeout, aPacket);

    while (state != idle)
    {
        received = receiveRaw(timeout);
        if (received == 0)  return 0;
        delete received;
    }

    return 1;
}

void TCP::disconnectAccept(Packet* aPacket)
{
	assertState(disconnecting);
	
	setState(idle);
	
	sendControl(aPacket, t_disconnect_confirm);

	socket.close();	

	local.clear();
	remote.clear();
}

void TCP::disconnectReject(Packet* aPacket)
{
	assertState(disconnecting);
	
	setState(connected);
	
	sendControl(aPacket, t_disconnect_reject);
}

void TCP::abort(Packet* aPacket)
{
	if (getState() != connected) return;
	
	if (state != dying) setState(idle);

	sendControl(aPacket, t_abort);
	
	socket.close();

	local.clear();
	remote.clear();
}

void TCP::stopListening()
{
	if (getState() != listening)	return;

	setState(idle);
	socket.close();
	local.clear();
}

void TCP::stopConnecting()
{
	if (getState() != calling)	return;

	setState(idle);
	socket.close();
	local.clear();
	remote.clear();
}

int TCP::send(Packet& aPacket, unsigned aTimeout, int expedited)
{
	if (state != connected)	
	{
		return 0;	
	}
	
	return sendRaw(aPacket, aTimeout, expedited);
}

int TCP::sendRaw(Packet& aPacket, unsigned aTimeout, int expedited)
{
	int rc;
	unsigned sent = 0;
    unsigned size = aPacket.getSize();
	
	aPacket.finalize();

    aPacket.swapByteOrder(1);

	while (sent < size)
	{
		socket.waitForSend(aTimeout);
		rc = socket.send(aPacket[sent], size - sent, expedited);
		if (rc == 0)
		{	
			if (state == connected)  aborted();
			return 0;
		}
		
		sent += rc;
	}

    return sent;
}

int TCP::sendControl(Packet* aPacket, int aControlContent)
{
	char buffer[sizeof(Packet)];

	if (aPacket == 0)	aPacket = new(buffer) Packet(0, sizeof(Packet));
	
	aPacket->addControlPart(0);
	aPacket->setControlContent(aControlContent);
	
	return sendRaw(*aPacket);
}

Packet* TCP::receive(unsigned aTimeout)
{
	return receiveRaw(aTimeout);
}

/* 
	this is a pain in the ass. As little as possible is copied, but socket.waitForData is called at
	least twice, which should not be necessary in most cases.
	
	To not copy the packet and read in one bulk wherever possible, we need a special PacketBuffer class
	that implements its own memory management (a heap with a fragment list). 
*/
Packet* TCP::receiveRaw(unsigned aTimeout)
{
	Packet* newPacket;
	char buffer[3 * sizeof(unsigned)];
	unsigned len = 0;
	unsigned rcvd, size;
	
	while(len < sizeof(buffer))
	{
		if ((timeout = socket.waitForData(aTimeout)) == 0)	return 0;
	
		rcvd = socket.receive(&buffer[len], sizeof(buffer) - len);
		if (rcvd == 0)
		{
			if (state == connected)	aborted();
			return 0;
		}
		
		len += rcvd;
				
		if (!verifyMagic((Packet*)buffer, len))
		{
			if (state == connected)	aborted();
			return 0;
		}	
	}
	size = ntohl(((Packet*)buffer)->getSize());
	newPacket = (Packet*)new char[size];
	memcpy(newPacket, buffer,  len);

	while(len < size)
	{
		if ((timeout = socket.waitForData(aTimeout)) == 0)	return 0;
	
		rcvd = socket.receive((void*)((unsigned)newPacket + len), size - len);
		if (rcvd == 0)
		{
			if (state == connected)	aborted();
			return 0;
		}
		
		len += rcvd;
	}

	newPacket->initialize();

    newPacket->swapByteOrder(0);

	if (state == connected || state == disconnecting) checkForControlPacket(newPacket);

	return newPacket;
}

void TCP::checkForControlPacket(Packet* aPacket)
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
			break;
		}
		setState(disconnecting);
		break;
	case t_disconnect_confirm:
		setState(idle);
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

int TCP::verifyMagic(Packet* incomplete, int size)
{
	// if we got a wrong packet, we reply get_lost
	if (!incomplete->verifyMagic(size, 1))
	{
		socket.send(get_lost, strlen(get_lost));
		return 0;
	}
	
	return 1;
}

void TCP::aborted()
{
	setState(idle);
	
	socket.close();

	local.clear();
	remote.clear();

}

void TCP::setState(states aState)
{
	state = aState;
}

void TCP::fatal(char* error)
{
	throw "TCP::fatal";
}

Packet* TCP::doListen()
{
    while(1)
    {
   		timeout = socket.listen(remote, timeout);
    	if (timeout == 0)
    	{
    		if (state != dying) setState(idle);
    		socket.close();
    		return 0;
    	}

		socket.getPeerName(remote);
		socket.getName(local);
		socket.setLingerTimeout(2000);
		socket.setNoDelay(1);
		// socket.setKeepAlive(1);
    
		packet = receiveRaw(timeout);
    	if (packet == 0 && timeout == 0)
    	{
    		setState(idle);
    		socket.close();
    		return 0;
    	}
    	// if we didn't get a proper packet, close connection and try again
    	if (packet == 0 || !packet->hasControlData() || packet->getControlContent() != t_connect_request)
    	{
    		socket.close();
			socket.bind(local);

            continue;
        }  
        else break;
	}
	setState(connecting);

	return packet;
}

Packet* TCP::doConnect()
{

	if ((timeout = socket.connect(remote, timeout)) == 0)
	{
		if (state != dying) setState(idle);
		socket.close();
		return 0;
	}

	socket.getName(local);
	socket.setLingerTimeout(2000);
	socket.setNoDelay(1);
	// socket.setKeepAlive(1);

	if (!sendControl(packet, t_connect_request))    
    {
        setState(idle);
        socket.close();
        return 0;
    }
    packet = receiveRaw(timeout);
	if (packet == 0 || timeout == 0 || !packet->hasControlData())
	{
		setState(idle);
		socket.close();
		return 0;
	}
	else if (packet->getControlContent() == t_connect_confirm)
	{
		setState(connected);
		return packet;
	}
	else if (packet->getControlContent() == t_connect_reject)
	{
		setState(idle);
		socket.close();
		return packet;
	}
	else fatal("unexpected control packet");

	return 0;
}

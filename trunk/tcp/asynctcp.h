/*
	AsyncTCP.h
*/

#ifndef _ASYNCTCP_H_
#define _ASYNCTCP_H_

#include "omnithread.h"
#include "tcp.h"
#include "packet.h"
#include "socket.h"
#include "queue.h"
#include "client.h"

class AsyncTCPNoThread : public TCP
{
public:
		
	AsyncTCPNoThread(TransportClient& aClient, void* aPrivateData = 0);
	virtual ~AsyncTCPNoThread();
	
	// Data transfer
	virtual void send(Packet& aPacket, int expedited = 0);
	virtual Packet* receive();

	virtual void run();
	
	virtual void fatal(char* error);

	virtual states getState();

protected:
	
	// helper methods
	virtual void checkForControlPacket(Packet* aPacket);
	virtual void aborted();
	virtual void setState(states aState);
	
	virtual Packet* doListen()  { event.post(); return 0; }
	virtual Packet* doConnect() { event.post(); return 0; }
	
	TransportClient& getClient()	{ return client; }
	
	TransportClient& client; 
	PacketQueue incoming;
	omni_mutex mutex;
	omni_condition event;
};

class AsyncTCP : public AsyncTCPNoThread, public Thread
{
public:
		
	AsyncTCP(TransportClient& aClient, void* aPrivateData = 0);
	virtual ~AsyncTCP();

	virtual void run();
};

#endif /* _ASYNCTCP_H_ */

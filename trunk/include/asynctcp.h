/*
	AsyncTCP.h
*/

#ifndef _ASYNCTCP_H_
#define _ASYNCTCP_H_

#include "omnithread.h"
#include "tcp.h"
#include "packet.h"
#include "socket.h"
#include "client.h"
#include "list.h"

class PacketQueue : public List
{
	public:

	struct Item : public List::Link
	{
		Item(void* aData) : data(aData) {}
		
		void* data;
	};
	
	PacketQueue() : List() {}
	virtual ~PacketQueue() { for( LinkIter i(head); !i.isDone(); i.next() ) freeLink( i.current() );}
	
	void enqueue(void* data)	{ addLast(new Item(data)); }
	void* dequeue()
	{
		Item* i = (Item*)removeFirst();
		void* d = i->data;
		delete i;
		return d;
	}
	virtual void freeLink(List::Link* item)	{ delete (Item*)item; }
};

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
	
	virtual Packet* doListen()  { event.signal(); return 0; }
	virtual Packet* doConnect() { event.signal(); return 0; }
	
	TransportClient& getClient()	{ return client; }
	
	TransportClient& client; 
	PacketQueue incoming;
	omni_mutex mutex;
	omni_condition event;
};

class AsyncTCP : public AsyncTCPNoThread, public omni_thread
{
public:
		
	AsyncTCP(TransportClient& aClient, void* aPrivateData = 0);
	virtual ~AsyncTCP();

	virtual void *run_undetached(void *arg);
};

#endif /* _ASYNCTCP_H_ */

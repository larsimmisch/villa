/*
    Socket.h 
	
	Copyright (c) Immisch, Becker & Partner 1994, 1995
	
	
	timeouts are always 1/1000 seconds

	known bugs:

	if a socket is bound, but no subsequent listen follows,
	the waiting member does not get destroyed.

	The fix is to count if someone is listening, and destroy if not
*/

#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <winsock.h>
#include "omnithread.h"
#include "set.h"
#include "list.h"
#include "error.h"
#include "sap.h"

class Socket;
class Listener;

class ListenerQueue : public DList
{
public:

	class Item : public DList::DLink
	{
	public:

		Item(Listener* aListener) 
			: result(0), listener(aListener), event(&mutex) {}

		omni_mutex mutex;
		omni_condition event;
		SAP sap;
		int hsocket;
		Listener* listener;
		unsigned result;
	};
	
	ListenerQueue();
	virtual ~ListenerQueue();
	
	Item* enqueue(Listener* aListener);
	Item* dequeue();
	
	void cancel(Item* item);
	
protected:

	virtual void freeLink(List::Link* aLink);
	
	ListenerQueue(ListenerQueue&);
};

class Services : private Set
{
public:

	class Key : public List::Link
	{
		public:

		Key(int aProtocol, int aService) : service(aService), protocol(aProtocol) {}
		virtual ~Key() {}

		int service;
		int protocol;
	};

	Services(int size) : mutex(), Set(size) { }
	virtual ~Services() { empty(); }

	void lock() 	{ mutex.lock(); }
	void unlock()	{ mutex.unlock(); }

	ListenerQueue::Item* add(int protocol, SAP& aService);
	void remove(ListenerQueue::Item* anItem);
	void remove(Listener* aListener);

	protected:

	friend class ListenerQueue; // for mutex

	Listener* contains(int aProtocol, int aService);

	virtual void empty();
	virtual int hasKey(List::Link* anItem, const char* key) { return 0; }
	virtual int isEqual(List::Link* anItem, List::Link* anotherItem) 
		{ return ((Key*)anItem)->service == ((Key*)anotherItem)->service && ((Key*)anItem)->protocol == ((Key*)anotherItem)->protocol; }
	virtual int hasIndex(List::Link* anItem, int anIndex)	{ return 0; }
	virtual unsigned hashAssoc(List::Link* anItem);
		
	omni_mutex mutex;
};				

class Listener : public Services::Key, public omni_thread
{
public:

	Listener(int aProtocol, SAP& aService);
	virtual ~Listener();

	virtual void *run_undetached(void *arg);
	
	ListenerQueue queue;
	int hsocket;
};

class InvalidAddress : public Exception
{
public:

	InvalidAddress(
        const char* fileName,
        int lineNumber,
        const char* function,
        SAP& address,
        const Exception* previous = 0);

	virtual ~InvalidAddress() {}

    virtual void printOn(std::ostream& aStream);

	SAP address;
};

class SocketError : public OSError
{
public:

	SocketError(
        const char* fileName,
        int lineNumber,
        const char* function,
		unsigned long error,
        Exception* previous = 0);

	virtual ~SocketError();

    const char* name(unsigned long error);

protected:

	unsigned error;
};

class Socket
{
public:
 
    enum { infinite = -1 };
		
	Socket(int aProtocol = PF_INET);
	~Socket();

	void bind(SAP& local, int single = 0);
	int listen(SAP& remote, unsigned aTimeout = infinite);	// returns what is left of timeout
	int connect(SAP& remote, unsigned aTimeout = infinite);				// returns what is left of timeout

	int send(void* data, unsigned dataLength, int expedited = 0);	// returns no of bytes sent. 0 means queue is full
	int receive(void* data, unsigned dataLength);	// returns no of bytes received
	
	int waitForData(unsigned aTimeout = infinite);	// returns what is left of timeout 
	int waitForSend(unsigned aTimeout = infinite);	// returns what is left of timeout

	// bytesPending returns the number of bytes pending for atomic read
	unsigned bytesPending();

	void setReceiveQueueLength(unsigned aLength);
	void setSendQueueLength(unsigned aLength);
	void setNoDelay(int on);
	void setLingerTimeout(unsigned aTimeout);
	void setKeepAlive(int on);
    void setNonblocking(int on);
    void setReuseAddress(int on);

	void getName(SAP& name);
	void getPeerName(SAP& name);

	// use close to abort asynchronous pending listen or connects
	void close();

	int rejected();

	static void fillSocketAddress(SAP& aSAP, void* socketAddress);
	static void fillSAP(void* socketAddress, SAP& aSAP);

protected:

	friend class ListenerQueue;
	friend class Listener;
#pragma warning(disable: 4251)
	static Services services;
#pragma warning(default: 4251)
	
	int hsocket;
    int nonblocking;
	int protocol;
	ListenerQueue::Item* waiting;
	Listener* listener;
};

#endif

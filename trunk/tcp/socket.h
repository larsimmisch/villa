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
#include "error.h"
#include "sap.h"
#include "services.h"

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

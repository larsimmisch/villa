/*
    Socket.h 
	
	Copyright (c) Immisch, Becker & Partner 1994, 1995
	
	
	timeouts are always 1/1000 seconds

	known bugs:

	If a socket is bound, but no subsequent listen follows,
	the waiting member does not get destroyed.

	The fix is to count if someone is listening, and destroy if not
*/

#ifndef _SOCKET_H_
#define _SOCKET_H_

#include "omnithread.h"
#include "set.h"
#include "list.h"
#include "error.h"
#include "sap.h"
#include <winsock2.h>

class Socket;

class InvalidAddress : public Exception
{
public:

	InvalidAddress(
        const char* fileName,
        int lineNumber,
        const char* function,
        const SAP& address,
        const Exception* previous = 0);

	virtual ~InvalidAddress() {}

    virtual void printOn(std::ostream& aStream) const;

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
		
	Socket(int protocol = PF_INET, int s = -1);
	virtual ~Socket();

	void bind(SAP& local);
	void listen(int backlog = 5);
	void connect(const SAP& remote);
	int accept(SAP &remote);
	void close();

	int send(void* data, unsigned dataLength);	// returns no of bytes sent. 0 means queue is full

    int receive(void* data, unsigned dataLength);	// returns no of bytes received
	
	// bytesPending returns the number of bytes pending for atomic read
	unsigned bytesPending();

	void setReceiveQueueLength(unsigned aLength);
	void setSendQueueLength(unsigned aLength);
	void setNoDelay(int on);
	void setLingerTimeout(unsigned aTimeout);
	void setKeepAlive(int on);
    void setNonblocking(int on);
    void setReuseAddress(int on);

	int fd() const { return m_socket; } 
	int protocol() const { return m_protocol; }

	void getName(SAP& name);
	void getPeerName(SAP& name);

	int rejected();

	static void fillSocketAddress(const SAP& aSAP, void* socketAddress);
	static void fillSAP(void* socketAddress, SAP& aSAP);

	static void init();

	SAP m_local;
	SAP m_remote;

protected:
	
	int m_socket;
    int m_nonblocking;
	int m_protocol;
};

#endif

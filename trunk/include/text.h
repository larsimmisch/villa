/*
	TCP.h
*/

#ifndef _TEXT_H_
#define _TEXT_H_

#include "socket.h"

class TextTransport
{
public:

	enum states  { idle, calling, listening, connecting, connected, disconnecting, dying };

    enum { indefinite = -1 };
		
	TextTransport(void* aPrivateData = 0);
	virtual ~TextTransport();
	
	// Connection establishment 
	virtual int listen(SAP& aLocalSAP, unsigned aTimeout = indefinite);
	virtual int connect(SAP& aRemoteSAP, unsigned aTimeout = indefinite);
	
	// must be called by client after  a connectIndication
	virtual void accept();
	virtual void reject();
	
	// Dissolve a connection
	virtual void disconnect(unsigned aTimeout = indefinite);
	
    // convenience disconnect method. returns if disconnected or timeout
    virtual int disconnectAndWait(unsigned aTimeout = indefinite);

	// must be called by client after a disconnectRequest
	virtual void disconnectAccept();
	
	// Dissolve a connection rapidly
	virtual void abort();

	// Data transfer. send null terminated strings.
	virtual int send(const char* data, unsigned aTimeout = indefinite, int expedited = 0);

	virtual char* receive(unsigned aTimeout = indefinite);
	
    TextTransport& operator<<(void (*fn)(TextTransport&)) { fn(*this); return *this; }
    TextTransport& operator<<(int i);
    TextTransport& operator<<(unsigned i)     { *this << (int)i; return *this; }
    TextTransport& operator<<(char c);
    TextTransport& operator<<(char* s)        { this->send(s); return *this; }
    TextTransport& operator<<(const char* s)  { this->send(s); return *this; }
 
	virtual void fatal(char* error);

	states getState()	{ return state; }
	int isConnected()	{ return state == connected; }
	int isDisconnecting()	{ return state == disconnecting; }
	int isIdle()		{ return state == idle; }
	void* getPrivateData()			{ return privateData; }

    SAP& getLocalSAP()  { return local; }
    SAP& getRemoteSAP() { return remote; }

protected:
	
	// helper methods
	virtual void assertState(states aState);
	virtual char* receiveRaw(unsigned aTimeout = indefinite);
	virtual int sendRaw(const char* aPacket, unsigned aTimeout = indefinite, int expedited = 0);
	virtual void aborted();
	
	virtual int doListen();
	virtual int doConnect();
	
	virtual void setState(states aState) { state = aState; }

	volatile states state;
	SAP local;
	SAP remote;
	volatile unsigned timeout;
	void* privateData;
	Socket socket;
};

void endl(TextTransport& t);

#endif /* _TEXT_H_ */

/*
	TCP.h
*/

#ifndef _TCP_H_
#define _TCP_H_

#include "transport.h"
#include "packet.h"
#include "socket.h"

class TCP : public Transport
{
	public:

    enum { infinite = -1 };
		
	TCP(void* aPrivateData = 0);
	virtual ~TCP();
	
	// Connection establishment 
	virtual Packet* listen(SAP& aLocalSAP, unsigned aTimeout = infinite, int single = 0);
	virtual Packet* connect(SAP& aRemoteSAP, unsigned aTimeout = infinite, Packet* initialPacket = 0);
	
	// must be called by client after  a connectIndication
	virtual void accept(Packet* aPacket = 0);
	virtual void reject(Packet* aPacket = 0);
	
	// Dissolve a connection
	virtual void disconnect(unsigned aTimeout = infinite, Packet* aPacket = 0);
	
    // convenience disconnect method. returns if disconnected or timeout
    virtual int disconnectAndWait(unsigned aTimeout = infinite, Packet* aPacket = 0);

	// must be called by client after a disconnectRequest
	virtual void disconnectAccept(Packet* aPacket = 0);
	virtual void disconnectReject(Packet* aPacket = 0);
	
	// Dissolve a connection rapidly
	virtual void abort(Packet* aPacket = 0);

	// stop listening
	void stopListening();

	// stop connecting
	void stopConnecting();

	// Data transfer
	virtual int send(Packet& aPacket, unsigned aTimeout = infinite, int expedited = 0);
	virtual Packet* receive(unsigned aTimeout = infinite);
	
	virtual void fatal(char* error);

	virtual Packet* receiveRaw(unsigned aTimeout = infinite);
	
	protected:
	
	// helper methods
	virtual void assertState(states aState);
	virtual int sendRaw(Packet& aPacket, unsigned aTimeout = infinite, int expedited = 0);
	virtual int sendControl(Packet* aPacket, int aControlContent);
	virtual int verifyMagic(Packet* incomplete, int size);
	virtual void checkForControlPacket(Packet* aPacket);
	virtual void aborted();
	virtual void setState(states aState);
	
	virtual Packet* doListen();
	virtual Packet* doConnect();
				
	Socket socket;
	Packet* packet;
};

#endif /* _TCP_H_ */

/* Client.h */

#ifndef _TRANSPORT_CLIENT_H_
#define _TRANSPORT_CLIENT_H_

#include <iostream>
#include "exc.h"
#include "sap.h"

class Packet;
class Transport;

class TransportClient
{
public:

	TransportClient() {}
	virtual ~TransportClient() {}
	
	// must call server.accept or server.reject
	virtual void connectRequest(Transport* server, SAP& remote, Packet* initialPacket = 0) = 0;
	virtual void connectRequestTimeout(Transport* server) = 0;
	
	// replies to server.connect from far end
	virtual void connectConfirm(Transport* server, Packet* initialReply = 0) = 0;
	virtual void connectReject(Transport* server, Packet* initialReply = 0) = 0;
	virtual void connectTimeout(Transport* server) = 0;
	
	// must call server.disconnectAccept or server.disconnectReject 
	virtual void disconnectRequest(Transport* server, Packet* finalPacket = 0) = 0;
	
	// replies to server.disconnect from far end
	virtual void disconnectConfirm(Transport* server, Packet* finalReply = 0) = 0;
	virtual void disconnectReject(Transport* server, Packet* aPacket = 0) = 0;
	virtual void disconnectTimeout(Transport* server) = 0;
	
	// sent whenever packet is received
	virtual void dataReady(Transport* server) = 0;
 
	// flow control
	virtual void stopSending(Transport* server) = 0;
	virtual void resumeSending(Transport* server) = 0;
	
	// miscellaneous
	
	virtual void abort(Transport* sender, Packet* lastPacket) = 0;
	
	// you don't have to use the dataReady/dataNotReady protocol if you absolutely don't want to
	// overwrite asynchronous and have your data delivered by the data call
	virtual int asynchronous(Transport* server)	{ return 1; }
	virtual void data(Transport* server, Packet* aPacket) {}

    virtual void fatal(const char* e) { std::cerr << e << endl; }
    virtual void fatal(Exception& e)  { std::cerr << e; }
};
#endif

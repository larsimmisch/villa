/*
	Copyright 1995 Immisch, Becker & Partner, Hamburg

	created: Mon Nov 18 12:05:02 GMT+0100 1996

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#include <Collect/List.h>
#include <Transport/Packet.h>
#include <TCP/AsyncTCP.h>
#include "Configuration.h"

class InterfaceConnection : public AsyncTCPNoThread, public List::Link
{
public:

	InterfaceConnection(TransportClient& client, SAP& local);
	virtual ~InterfaceConnection() {}

	void sendConnectDone(unsigned syncMajor, unsigned syncMinor, unsigned result);

	Mutex& getMutex()	{ return mutex; }

    Packet* staticPacket(unsigned numArgs) { packet->clear(numArgs); return packet; }

protected:

	Mutex mutex;
    Packet* packet;
    char buffer[1024];
};

class InterfaceConnectionThread : public InterfaceConnection, public Thread
{
	public:

	InterfaceConnectionThread(TransportClient& client, SAP& local) : InterfaceConnection(client, local) { Thread::resume(); }
	virtual ~InterfaceConnectionThread()	{}

	virtual void run()	{ AsyncTCPNoThread::run(); }
};

class InterfaceConnections : public List
{
	public:

	InterfaceConnections() {}
	virtual ~InterfaceConnections() { empty(); }

    virtual void  freeLink(Link* aLink) { delete (InterfaceConnection*)aLink; }
};

class Interface : public TransportClient
{
	public:

	Interface(SAP& local);
	virtual ~Interface() {}

	virtual void run();

	void cleanup(Transport* server);

	// protocol of TransportClient
	virtual void connectRequest(Transport* server, SAP& remote, Packet* initialPacket = 0);
	virtual void connectRequestTimeout(Transport* server);
	
	// replies to server.connect from far end
	virtual void connectConfirm(Transport* server, Packet* initialReply = 0);
	virtual void connectReject(Transport* server, Packet* initialReply = 0);
	virtual void connectTimeout(Transport* server);
	
	// must call server.disconnectAccept or server.disconnectReject 
	virtual void disconnectRequest(Transport* server, Packet* finalPacket = 0);
	
	// replies to server.disconnect from far end
	virtual void disconnectConfirm(Transport* server, Packet* finalReply = 0);
	virtual void disconnectReject(Transport* server, Packet* aPacket = 0);
	virtual void disconnectTimeout(Transport* server);

    virtual void abort(Transport* server, Packet* final);

	// sent whenever packet is received
	virtual void dataReady(Transport* server) {}
 
	// flow control
	virtual void stopSending(Transport* server) {}
	virtual void resumeSending(Transport* server) {}
	
    virtual int asynchronous(Transport* server)  { return 0; }
    virtual void data(Transport* server, Packet* aPacket);

	protected:

	unsigned unused;
	Mutex mutex;
	InterfaceConnections connections;
	SAP local;
};

// really obscure helper class...
// contains all information necessary to complete the if_connect
// because if_connect is completed only when the tcp connection 
// is either established or failed

class ConnectCompletion
{
public:

	ConnectCompletion(InterfaceConnection& aFace, unsigned aSyncMajor, unsigned aSyncMinor, SAP& aClient)
		: iface(aFace), syncMajor(aSyncMajor), syncMinor(aSyncMinor), client(aClient) {}
	~ConnectCompletion() {}

	void done(unsigned result)	{ iface.sendConnectDone(syncMajor, syncMinor, result); }

	SAP& getClient()	{ return client; }

protected:

	InterfaceConnection& iface;
	unsigned syncMajor; 
	unsigned syncMinor;
	SAP client;
};

#endif
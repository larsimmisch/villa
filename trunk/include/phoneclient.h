/*
	phoneclient.h

	$Id: phoneclient.h,v 1.5 2001/05/20 20:02:44 lars Exp $

	Copyright 1995-2001 Lars Immisch

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _PHONECLIENT_H_
#define _PHONECLIENT_H_ 


#include "sap.h"

class Trunk;
class Telephone;
class Sample;

class TrunkClient
{
public:

	TrunkClient() {}
	virtual ~TrunkClient() {}
	
	// must call server.accept or server.reject
	virtual void connectRequest(Trunk* server, const SAP& local, const SAP& remote) = 0;
	
	// replies to server.connect from far end
	virtual void connectDone(Trunk* server, int result) = 0;
	
	// results from transfer
	virtual void transferDone(Trunk* server, int result) {}

	virtual void disconnectRequest(Trunk* server, int cause) = 0;
	
	// disconnect completion
	virtual void disconnectDone(Trunk* server, unsigned result) = 0;

	// accept completion
	virtual void acceptDone(Trunk* server, unsigned result) = 0;

	// reject completion
	virtual void rejectDone(Trunk* server, unsigned result) = 0;

    // called whenever additional dialling information comes in (caller finishes dialling)
    virtual void details(Trunk* server, const SAP& local, const SAP& remote)   {}

	// called when remote end ringing is detected on an outgoing line
	virtual void remoteRinging(Trunk* server) {}
};

class TelephoneClient : public TrunkClient 
{
public:

	TelephoneClient() {}
	virtual ~TelephoneClient() {}

	virtual void disconnected(Telephone *server) = 0;
	virtual void connected(Telephone *server) = 0;

	// sent whenever a touchtone is received
	virtual void touchtone(Telephone* server, char tt) = 0;

    // sent whenever a Sample is started
    virtual void started(Telephone* server, Sample* sample) = 0;

    // sent whenever a Sample is successfully sent
    virtual void completed(Telephone* server, Sample* sample, unsigned msecs) = 0;
	
};
#endif

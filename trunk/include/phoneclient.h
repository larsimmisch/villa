/*
	phoneclient.h

	$Id: phoneclient.h,v 1.7 2001/09/26 22:41:57 lars Exp $

	Copyright 1995-2001 Lars Immisch

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _PHONECLIENT_H_
#define _PHONECLIENT_H_ 


#include "sap.h"

class Trunk;
class Media;
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

class MediaClient
{
public:

	MediaClient() {}
	virtual ~MediaClient() {}

	// sent whenever a touchtone is received
	virtual void touchtone(Media* server, char tt) = 0;

    // sent whenever a Sample is started
    virtual void started(Media* server, Sample* sample) = 0;

    // sent whenever a Sample is successfully sent
    virtual void completed(Media* server, Sample* sample, unsigned msecs) = 0;	
};
#endif
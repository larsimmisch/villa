/*
	phoneclient.h

	$Id: phoneclient.h,v 1.10 2004/01/08 21:22:20 lars Exp $

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
	
	// callref may be INVALID_CALLREF
	virtual void idleDone(Trunk *server, unsigned callref) = 0;

	// must call server.accept or server.reject
	virtual void connectRequest(Trunk* server, unsigned callref,
		const SAP& local, const SAP& remote) = 0;
	
	// replies to server.connect from far end
	virtual void connectDone(Trunk* server, unsigned callref, int result) = 0;
	
	// results from transfer
	virtual void transferDone(Trunk* server, unsigned callref, int result) {}

	virtual void disconnectRequest(Trunk* server, unsigned callref, int cause) = 0;
	
	// disconnect completion
	virtual void disconnectDone(Trunk* server, unsigned callref, int result) = 0;

	// accept completion
	virtual void acceptDone(Trunk* server, unsigned callref, int result) = 0;

    // called whenever additional dialling information comes in (caller finishes dialling)
    virtual void details(Trunk* server, unsigned callref, const SAP& local, const SAP& remote)   {}

	// called when remote end ringing is detected on an outgoing line
	virtual void remoteRinging(Trunk* server, unsigned callref) {}
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
/*
	trunktest.cpp

	$Id: trunktest.cpp,v 1.5 2001/05/20 20:02:44 lars Exp $

	Copyright 1995-2001 Lars Immisch

	Author: Lars Immisch <lars@ibp.de>
*/

#include <iostream>
#include "aculab/acutrunk.h"
#include "mvip.h"
#include "log.h"

using namespace std;

Log cout_log(cout);

class Application : public TrunkClient
{
	// must call server.accept or server.reject
	virtual void connectRequest(Trunk* server, const SAP& local, const SAP& remote)
	{
		log(log_debug, "app") << "incoming call - local: " << local << " remote: " << remote << logend();
		server->accept();
	}
	
	// replies to server.connect from far end
	virtual void connectDone(Trunk* server, int result)
	{
		if (result == r_ok)
		{
			log(log_debug, "app") << "outgoing call connected" << logend();
		}
		else
		{
			log(log_debug, "app") << "outgoing call failed" << logend();
		}
	}
	
	// results from transfer
	virtual void transferDone(Trunk* server, int result) {}

	virtual void disconnectRequest(Trunk* server, int cause)
	{
		log(log_debug, "app") << "remote disconnect" << logend();

		server->disconnectAccept();
	}
	
	// disconnect completion
	virtual void disconnectDone(Trunk* server, unsigned result)
	{
		log(log_debug, "app") << "disconnected" << logend();

		server->listen();
	}

	// accept completion
	virtual void acceptDone(Trunk* server, unsigned result)
	{
		if (result == r_ok)
		{
			log(log_debug, "app") << "incoming call connected" << logend();
		}
		else
		{
			log(log_debug, "app") << "incoming call failed" << logend();

			server->listen();
		}
	}

	// reject completion
	virtual void rejectDone(Trunk* server, unsigned result)
	{
		log(log_debug, "app") << "rejected" << logend();

		server->listen();
	}

    // called whenever additional dialling information comes in (caller finishes dialling)
    virtual void details(Trunk* server, const SAP& local, const SAP& remote)
	{
		log(log_debug, "app") << "details - local: " << local << " remote: " << remote << logend();
	}

	// called when remote end ringing is detected on an outgoing line
	virtual void remoteRinging(Trunk* server)
	{
		log(log_debug, "app") << "remote ringing" << logend();
	}
};

int main(int argc, char* argv[])
{
	set_log_instance(&cout_log);
	set_log_level(4);

	Application app;

	AculabTrunk trunk(&app, 0);

	AculabTrunk::start();

	SAP local, remote;

	remote.setAddress("3172547");

	trunk.connect(local, remote);
	
	// trunk.listen();

	while(true)
		omni_thread::sleep(-1);

	return 0;
}

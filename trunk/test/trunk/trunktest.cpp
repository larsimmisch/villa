/*
	trunktest.cpp

	$Id: trunktest.cpp,v 1.2 2000/10/18 16:58:43 lars Exp $

	Copyright 2000 ibp (uk) Ltd.

	Author: Lars Immisch <lars@ibp.de>
*/

#include <iostream>
#include "aculab/acutrunk.h"
#include "mvip.h"

using namespace std;

class Application : public TrunkClient
{
	// must call server.accept or server.reject
	virtual void connectRequest(Trunk* server, const SAP& local, const SAP& remote)
	{
		cout << "incoming call - local: " << local << " remote: " << remote << endl;
		server->accept();
	}
	
	// replies to server.connect from far end
	virtual void connectDone(Trunk* server, int result)
	{
		if (result == r_ok)
		{
			cout << "outgoing call connected" << endl;
		}
		else
		{
			cout << "outgoing call failed" << endl;
		}
	}
	
	// results from transfer
	virtual void transferDone(Trunk* server, int result) {}

	virtual void disconnectRequest(Trunk* server, int cause)
	{
		cout << "remote disconnect" << endl;

		server->disconnectAccept();
	}
	
	// disconnect completion
	virtual void disconnectDone(Trunk* server, unsigned result)
	{
		cout << "disconnected" << endl;

		server->listen();
	}

	// accept completion
	virtual void acceptDone(Trunk* server, unsigned result)
	{
		if (result == r_ok)
		{
			cout << "incoming call connected" << endl;
		}
		else
		{
			cout << "incoming call failed" << endl;

			server->listen();
		}
	}

	// reject completion
	virtual void rejectDone(Trunk* server, unsigned result)
	{
		cout << "rejected" << endl;

		server->listen();
	}

    // called whenever additional dialling information comes in (caller finishes dialling)
    virtual void details(Trunk* server, const SAP& local, const SAP& remote)
	{
		cout << "details - local: " << local << " remote: " << remote << endl;
	}

	// called when remote end ringing is detected on an outgoing line
	virtual void remoteRinging(Trunk* server)
	{
		cout << "remote ringing" << endl;
	}
};

int main(int argc, char* argv[])
{
	Application app;

	AculabTrunk trunk(&app, 0);

	AculabTrunk::start();

	trunk.listen();

	while(true)
		omni_thread::sleep(-1);

	return 0;
}

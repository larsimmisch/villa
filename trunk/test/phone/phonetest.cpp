/*
	phonetest.cpp

	$Id: phonetest.cpp,v 1.8 2001/06/08 10:09:33 lars Exp $

	Copyright 1995-2001 Lars Immisch

	Author: Lars Immisch <lars@ibp.de>
*/

#pragma warning (disable : 4786)

#include <iostream>
#include "getopt.h"
#include "log.h"
#include "aculab/acuphone.h"
#include "aculab/mvswdrvr.h"
#include "ctbus.h"

using namespace std;

Log cout_log(cout);

class Application : public TelephoneClient
{
public:

	// must call server.accept or server.reject
	virtual void connectRequest(Trunk *server, const SAP& local, const SAP& remote)
	{
		log(log_debug, "app") << "incoming call - local: " << local << " remote: " << remote << logend();
		server->accept();
	}
	
	// replies to server.connect from far end
	virtual void connectDone(Trunk *server, int result)
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
	virtual void transferDone(Trunk *server, int result) {}

	virtual void disconnectRequest(Trunk* server, int cause)
	{
		log(log_debug, "app") << "remote disconnect" << logend();

		server->disconnectAccept();
	}
	
	// disconnect completion
	virtual void disconnectDone(Trunk *server, unsigned result)
	{
		log(log_debug, "app") << "disconnected" << logend();

		server->listen();
	}

	// accept completion
	virtual void acceptDone(Trunk *server, unsigned result)
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
	virtual void rejectDone(Trunk *server, unsigned result)
	{
		log(log_debug, "app") << "rejected" << logend();

		server->listen();
	}

    // called whenever additional dialling information comes in (caller finishes dialling)
    virtual void details(Trunk *server, const SAP &local, const SAP &remote)
	{
		log(log_debug, "app") << "details - local: " << local << " remote: " << remote << logend();
	}

	// called when remote end ringing is detected on an outgoing line
	virtual void remoteRinging(Trunk *server)
	{
		log(log_debug, "app") << "remote ringing" << logend();
	}

	virtual void touchtone(Telephone* server, char tt)
	{
		log(log_debug, "app") << "received touchtone: " << tt << logend();

		char s[2];
		s[0] = tt;
		s[1] = '\0';

		// Sample *touchtones = server->createTouchtones(s);

		// touchtones->start(server);
	}

	virtual void started(Telephone *server, Sample *sample)
	{
	}

	virtual void connected(Telephone *server)
	{
		try
		{
			Sample* sample = server->createFileSample("startrek.al");

			sample->start(server);
		}
		catch(const Exception &e)
		{
			log(log_error, "app") << "caught exception starting sample: " 
				<< e << logend();

			server->disconnect();
		}
		
	}

	virtual void disconnected(Telephone *server)
	{
	}

	virtual void completed(Telephone *server, Sample *sample, unsigned msecs)
	{
		server->disconnect();
	}
};

void usage()
{
	cout << "usage: phonetest -c <count> -p <port> -s <switch> -l <loglevel>" << endl;
}

int main(int argc, char* argv[])
{
	extern int  opterr;            /* error => print message */
	extern int  optind;            /* next argv[] index */
	extern char *optarg;       /* option parameter if any */

	set_log_instance(&cout_log);
	set_log_level(4);

	CTbus *bus;
	Application app;
	int count = 1;
	int port = 0;
	int sw = 0;
	
	int c;
	while( (c = getopt(argc, argv, "c:p:s:l:")) != EOF) 
	{
		switch(c) 
		{
		case 'c':
			count = atoi(optarg);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 's':
			sw = atoi(optarg);
			break;
		case 'l':
			set_log_level(atoi(optarg));
			break;
		case '?':
			usage();
		default:
			usage();
		}
	}

	struct swmode_parms swmode;

	int rc = sw_mode_switch(sw, &swmode);

	if (rc)
	{
		log(log_error, "app") << "sw_mode_switch("<< sw << ") failed: " << rc
			<< logend();

		return rc;
	}

	if (swmode.ct_buses & 1 << SWMODE_CTBUS_H100)
	{
		bus = new H100;

		log(log_debug, "app") << "using H.100 for switching" << logend();
	}
	else if (swmode.ct_buses & 1 << SWMODE_CTBUS_SCBUS)
	{
		bus = new SCbus;

		log(log_debug, "app") << "using SCbus for switching" << logend();
	}

	vector<AculabTrunk*> trunks;
	vector<AculabPhone*> phones;

	trunks.reserve(count);
	phones.reserve(count);

	for (int i = 0; i < count; ++i)
	{
		Timeslot receive = bus->allocate();
		Timeslot transmit = bus->allocate();

		trunks[i] = new AculabTrunk(&app, port);
		phones[i] = new AculabPhone(&app, trunks[i], sw, receive, transmit);
	}

	AculabTrunk::start();
	AculabPhone::start();

	for (i = 0; i < count; ++i)
	{
		phones[i]->listen();
	}

	while(true)
		omni_thread::sleep(-1);

	return 0;
}

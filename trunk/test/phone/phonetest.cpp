/*
	phonetest.cpp

	$Id: phonetest.cpp,v 1.11 2001/06/19 15:02:51 lars Exp $

	Copyright 1995-2001 Lars Immisch

	Author: Lars Immisch <lars@ibp.de>
*/

#pragma warning (disable : 4786)

#include <iostream>
#include "getopt.h"
#include "log.h"
#include "omnithread.h"
#include "phone.h"
#include "aculab/acuphone.h"
#include "aculab/mvswdrvr.h"
#include "ctbus.h"

using namespace std;

Log cout_log(cout);

class Application : public TelephoneClient
{
public:

	Application() : m_active(false) {}

	// must call server.accept or server.reject
	virtual void connectRequest(Trunk *server, const SAP& local, const SAP& remote)
	{
		log(log_debug, "app", server->getName()) << "incoming call - local: " << local << " remote: " << remote << logend();
		server->accept();
	}
	
	// replies to server.connect from far end
	virtual void connectDone(Trunk *server, int result)
	{
		if (result == r_ok)
		{
			log(log_debug, "app", server->getName()) << "outgoing call connected" << logend();
		}
		else
		{
			log(log_debug, "app", server->getName()) << "outgoing call failed" << logend();
		}
	}
	
	// results from transfer
	virtual void transferDone(Trunk *server, int result) {}

	virtual void disconnectRequest(Trunk* server, int cause)
	{
		log(log_debug, "app", server->getName()) << "remote disconnect" << logend();

		m_mutex.lock();

		if (m_active)
		{
			m_mutex.unlock();
			server->getTelephone()->abortSending();
		}
		else
		{
			m_mutex.unlock();
			server->disconnectAccept();
		}
	}
	
	// disconnect completion
	virtual void disconnectDone(Trunk *server, unsigned result)
	{
		log(log_debug, "app", server->getName()) << "disconnected" << logend();

		server->listen();
	}

	// accept completion
	virtual void acceptDone(Trunk *server, unsigned result)
	{
		if (result == r_ok)
		{
			log(log_debug, "app", server->getName()) << "incoming call connected" << logend();
		}
		else
		{
			log(log_debug, "app", server->getName()) << "incoming call failed" << logend();

			server->listen();
		}
	}

	// reject completion
	virtual void rejectDone(Trunk *server, unsigned result)
	{
		log(log_debug, "app", server->getName()) << "rejected" << logend();

		server->listen();
	}

    // called whenever additional dialling information comes in (caller finishes dialling)
    virtual void details(Trunk *server, const SAP &local, const SAP &remote)
	{
		log(log_debug, "app", server->getName()) << "details - local: " << local << " remote: " << remote << logend();
	}

	// called when remote end ringing is detected on an outgoing line
	virtual void remoteRinging(Trunk *server)
	{
		log(log_debug, "app", server->getName()) << "remote ringing" << logend();
	}

	virtual void touchtone(Telephone* server, char tt)
	{
		log(log_debug, "app", server->getName()) << "received touchtone: " << tt << logend();

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

			m_mutex.lock();

			m_active = true;

			m_mutex.unlock();
		}
		catch(const Exception &e)
		{
			log(log_error, "app", server->getName()) << "caught exception starting sample: " 
				<< e << logend();

			server->disconnect();
		}		
	}

	virtual void disconnected(Telephone *server)
	{
	}

	virtual void completed(Telephone *server, Sample *sample, unsigned msecs)
	{
		delete sample;
		
		m_mutex.lock();
		m_active = false;
		m_mutex.unlock();

		server->disconnect();
	}

private:

	omni_mutex m_mutex; 
	bool m_active;

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

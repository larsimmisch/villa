/*
	phonetest.cpp

	$Id: phonetest.cpp,v 1.19 2001/09/26 22:41:57 lars Exp $

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

class Application : public MediaClient, public TrunkClient
{
public:

	Application() : m_active(0), m_media(0), m_trunk(0) {}

	void init(Trunk *trunk, AculabMedia *media)
	{
		m_media = media;
		m_trunk = trunk;
	}

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
		m_mutex.lock();

		if (m_active)
		{
			m_active->stop(m_media);
			m_mutex.unlock();
			log(log_debug, "app", server->getName()) << "remote disconnect - aborting DSP activity" << logend();
		}
		else
		{
			m_mutex.unlock();
			log(log_debug, "app", server->getName()) << "remote disconnect - accepting" << logend();
			server->disconnectAccept();
		}
	}
	
	// disconnect completion
	virtual void disconnectDone(Trunk *server, unsigned result)
	{
		log(log_debug, "app", server->getName()) << "disconnected" << logend();

		m_media->disconnected(server);
	}

	// accept completion
	virtual void acceptDone(Trunk *server, unsigned result)
	{
		if (result == r_ok)
		{
			log(log_debug, "app", server->getName()) << "incoming call connected" << logend();

			m_media->connected(server);

			try
			{
				Sample* sample = m_media->createFileSample("sitrtoot.al");

				m_mutex.lock();
				sample->start(m_media);
				m_active = sample;
				m_mutex.unlock();

			}
			catch(const Exception &e)
			{
				log(log_error, "app", server->getName()) << "caught exception starting sample: " 
					<< e << logend();

				server->disconnect();
			}		

		}
		else
		{
			log(log_debug, "app", server->getName()) << "incoming call failed" << logend();
		}
	}

	// reject completion
	virtual void rejectDone(Trunk *server, unsigned result)
	{
		log(log_debug, "app", server->getName()) << "rejected" << logend();
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

	virtual void touchtone(Media *server, char tt)
	{
		log(log_debug, "app", server->getName()) << "received touchtone: " << tt << logend();

		char s[2];
		s[0] = tt;
		s[1] = '\0';

		// Sample *touchtones = server->createTouchtones(s);

		// touchtones->start(server);
	}

	virtual void started(Media *server, Sample *sample)
	{
	}

	virtual void completed(Media *server, Sample *sample, unsigned msecs)
	{		
		m_mutex.lock();
		m_active = 0;
		m_mutex.unlock();

		delete sample;

		log(log_debug, "app", server->getName()) << "sample completed" 
			<< logend();

		m_trunk->disconnect();
	}

private:

	omni_mutex m_mutex; 
	Sample *m_active;
	AculabMedia *m_media;
	Trunk *m_trunk;
};

void usage()
{
	cout << "usage: phonetest -p <port>[:<count>] -s <switch> -l <loglevel>" << endl;
	cout << "\t -p may be repeated" << std::endl;

	exit(2);
}

int main(int argc, char* argv[])
{
	extern int  opterr;            /* error => print message */
	extern int  optind;            /* next argv[] index */
	extern char *optarg;       /* option parameter if any */

	set_log_instance(&cout_log);
	set_log_level(4);

	CTbus *bus;
	std::map<int,int> port_counts;
	int sw = 0;
	
	int c;
	while( (c = getopt(argc, argv, "p:s:l:")) != EOF) 
	{
		switch(c) 
		{
		case 'p':
			{
				// syntax is "-p <port>:<count>
				std::string s(optarg);
				int colon = s.find(':');
				if (colon != std::string::npos)
				{
					std::string s2(s);
					s.erase(colon);
					s2.erase(0, colon+1);

					port_counts[atoi(s.c_str())] = atoi(s2.c_str());
				}
				else
					port_counts[atoi(s.c_str())] = 1;
			}
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

	// initialize defaults
	if (port_counts.size() == 0)
		port_counts[0] = 1;

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
	vector<AculabMedia*> media;

	int count = 0;
	for (std::map<int,int>::iterator i = port_counts.begin(); i != port_counts.end(); ++i)
	{
		log(log_debug, "app") << "starting " << i->second 
			<< " channels on port " << i->first <<	logend();

		for (int j = 0; j < i->second; ++j)
		{
			Timeslot receive = bus->allocate();
			Timeslot transmit = bus->allocate();

			Application *app = new Application;

			// this will leak memory - we don't care
			AculabTrunk *t = new AculabTrunk(app, i->first);
			AculabMedia *m = new AculabMedia(app, sw, 
				receive, transmit);

			app->init(t, m);

			trunks.push_back(t);
			media.push_back(m);
			++count;
		}
	}

	AculabTrunk::start();
	AculabMedia::start();

	for (int j = 0; j < count; ++j)
	{
		trunks[j]->listen();
	}

	while(true)
		omni_thread::sleep(-1);

	return 0;
}

/*
	Copyright 1995-2003 Lars Immisch

	created: Tue Jul 25 16:05:02 GMT+0100 1995

	Author: Lars Immisch <lars@ibp.de>
*/
#pragma warning (disable: 4786)

#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <new.h>
#include <assert.h>

#include "phoneclient.h"
#include "acuphone.h"
#include "rphone.h"
#include "ctbus.h"
#include "activ.h"
#include "sequence.h"
#include "interface.h"
#include "getopt.h"
#include "mvswdrvr.h"

Log cout_log(std::cout);

Conferences gConferences;

ConfiguredTrunks gConfiguration;
ClientQueue gClientQueue;

CTbus *gBus;

Timer Sequencer::timer;

// I use 'this' in the base member initializer list on purpose
#pragma warning(disable : 4355)

Sequencer::Sequencer(TrunkConfiguration* aConfiguration) 
  :	m_activity(this), m_configuration(aConfiguration), m_connectComplete(0),
	m_clientSpec(0), m_disconnecting(false), m_interface(0),
	m_callref(INVALID_CALLREF)
{
	Timeslot receive = gBus->allocate();
	Timeslot transmit = gBus->allocate();

	m_media = new AculabMedia(this, aConfiguration->getSwitch(), 
		receive, transmit);

	m_trunk = aConfiguration->getTrunk(this);
	
	m_trunk->listen();
}

void Sequencer::lost_connection()
{
	lock();
	m_interface = 0;
	unlock();

	disconnect(m_callref);
}

#pragma warning(default : 4355)

int Sequencer::addMolecule(InterfaceConnection *server, const std::string &id)
{
	unsigned mode;
	unsigned priority;

	(*server) >> mode;
	(*server) >> priority;

	if (!server->good() || server->eof())
	{
		server->clear();

		server->syntax_error(id) << "expecting jobid, mode and priority" << end(); 

		return PHONE_FATAL_SYNTAX;
	}

	Molecule* molecule = new Molecule(mode, priority, id);

	std::string type;
	
	(*server) >> type;

	while (server->good() && !server->eof())
	{
		Atom *atom;

		try
		{
			if (type == "play")
			{
				std::string file;
			
				(*server) >> file;

				atom = new PlayAtom(this, file.c_str());
			}
			else if (type == "record")
			{
				std::string file;
				unsigned max;

				(*server) >> file;
				(*server) >> max;

				atom = new RecordAtom(this, file.c_str(), max);
			}
			else if (type == "dtmf")
			{
				std::string tt;

				(*server) >> tt;

				atom = new TouchtoneAtom(this, tt.c_str());
			}
			else if (type == "beep")
			{
				unsigned count;

				(*server) >> count;

				atom = new BeepAtom(this, count);

			}
			else if (type == "silence")
			{
				int len;

				(*server) >> len;

				atom = new SilenceAtom(len);
			}
			else if (type == "conference")
			{
				std::string conf;

				(*server) >> conf;

				unsigned handle(0);

				sscanf(conf.c_str(), "Conf[%d]", &handle);

				atom = new ConferenceAtom(handle, 
					Conference::listen | Conference::speak);
			}

			std::string notifications;

			(*server) >> notifications;

			if (notifications == "start")
				atom->setNotifications(notify_start);
			else if (notifications == "stop")
				atom->setNotifications(notify_stop);
			else if (notifications == "both")
				atom->setNotifications(notify_start | notify_stop);
			// todo: syntax error if not 'none'
		}
		catch (const char* e)
		{
			log(log_error, "sequencer") << "Exception in addMolecule: " << e << logend();
			atom = 0;

			if (m_interface)
			{
				m_interface->begin() << PHONE_ERROR_FAILED << ' ' << id
					<< " MLCA " << m_trunk->getName() << " 0 " 
					<< molecule->getLength() << end();
			}

			return PHONE_ERROR_FAILED;
		}
		catch (const Exception& e)
		{
			log(log_error, "sequencer") << e << logend();
			atom = 0;

			if (m_interface)
			{
				m_interface->begin() << PHONE_ERROR_FAILED << ' ' << id  
					<< " MLCA " << m_trunk->getName() << " 0 " << molecule->getLength() << end();
			}

			return PHONE_ERROR_FAILED;
		}

		molecule->add(*atom);

		(*server) >> type;
	}

	if (molecule->getSize() == 0)
	{
		log(log_warning, "sequencer") << "Molecule is empty. will not be added." 
			<< logend();

		return PHONE_WARNING_EMPTY;
	}

	lock();

	m_activity.add(*molecule);
	checkCompleted();

	unlock();

	return PHONE_OK;
}


int Sequencer::discardMolecule(InterfaceConnection *server, const std::string &id)
{

	std::string mid;

	(*server) >> mid;

	if (!server->good())
	{
		server->syntax_error(id) << "expecting <molecule id>" << end();
		return PHONE_FATAL_SYNTAX;
	}

	Molecule* molecule = m_activity.find(mid);

	if (!molecule) 
	{
		log(log_error, "sequencer") << "discard molecule: (" << mid.c_str() 
			<< ") not found" << logend();

		server->begin() << PHONE_ERROR_NOT_FOUND << ' ' << id.c_str() << " invalid molecule "
			<< mid.c_str() << end();

		return PHONE_ERROR_NOT_FOUND;
	}

	// if active, stop and send ack when stopped, else remove and send ack immediately
	if (molecule->isActive())		  
	{
		molecule->setMode(Molecule::discard);
		molecule->stop(this);

		checkCompleted();
	}
	else
	{
		m_activity.remove(molecule);

		server->begin() << PHONE_OK << ' ' << id.c_str() 
			<< " MLCD " << m_id.c_str() << " removed" << end();
	}

	return PHONE_OK;
}

int Sequencer::discardByPriority(InterfaceConnection *server, const std::string &id)
{
	int discarded = 0;
	int fromPriority;
	int toPriority;
	int immediately;

	(*server) >> fromPriority;
	(*server) >> toPriority;
	(*server) >> immediately;

	if (!server->good())
	{
		server->syntax_error(id) << "expecting <fromPriority> <toPriority> <immediately>" << end();
		return PHONE_FATAL_SYNTAX;
	}

	log(log_debug, "sequencer") << "discarding molecules with " << fromPriority 
		<< " <= priority <= " << toPriority << logend();

	omni_mutex_lock lock(m_mutex);

	for (ActivityIter i(m_activity); !i.isDone(); i.next())
	{
		Molecule *molecule = i.current();

		if (molecule->getMode() & Molecule::dont_interrupt)  
			continue;

		if (molecule->getPriority() >= fromPriority && molecule->getPriority() <= toPriority)
		// if active, stop and send ack when stopped, else remove and send ack immediately
		if (molecule->isActive() && immediately)
		{
			molecule->setMode(Molecule::discard);
			molecule->stop(this);
			
			log(log_debug, "sequencer") << "stopped molecule " 
				<< molecule->getId() << logend();

			checkCompleted();
		}
		else
		{
			discarded++;
			m_activity.remove(molecule);
			
			log(log_debug, "sequencer") << "removed molecule "  
				<< molecule->getId() << logend();
		}
	}

	if (discarded)
	{
		server->begin() << PHONE_OK << ' ' << id.c_str()
			<< " MLDP removed molecules with priorities from " 
			<< fromPriority << " to " << toPriority << end();
	}

	return PHONE_OK;
}

void Sequencer::sendAtomDone(const char *id, unsigned nAtom, 
							 unsigned status, unsigned msecs)
{
	if (m_interface)
	{
		m_interface->begin() <<  PHONE_EVENT << " ATOM " 
			<< m_trunk->getName() << ' ' << id << ' '
			<< nAtom << ' ' << status << ' ' << msecs << end();
	}
}

void Sequencer::sendMoleculeDone(const char *id, unsigned status, 
								 unsigned pos, unsigned length)
{
	if (m_interface)
	{
		m_interface->begin() << status << ' ' << id << " MLCA " << m_trunk->getName()
			<< ' ' << pos << ' ' << length << end();
	}

	log(log_debug, "sequencer", m_trunk->getName())
		<< "sent molecule done for: " << id << " status: " << status << " pos: " 
		<< pos << " length: " << length << logend();
}

int Sequencer::connect(ConnectCompletion* complete)
{
	omni_mutex_lock lock(m_mutex);

	if (m_connectComplete)
	{
		log(log_debug, "sequencer", m_trunk->getName())
			<< "connect failed - busy" << logend();

		return PHONE_ERROR_BUSY;
	}

	m_connectComplete = complete;

	return PHONE_OK;
}

int Sequencer::transfer(InterfaceConnection *server, const std::string &id)
{
/* Todo

	SAP remote;
	unsigned timeout = indefinite;

	if (aPacket->typeAt(0) != Packet::type_string)
	{
		lock();

		packet->clear(1);
		packet->setSync(aPacket->getSyncMajor(), aPacket->getSyncMinor());
		packet->setContent(phone_transfer_done);
		packet->setUnsignedAt(0, _invalid);

		tcp.send(*packet);

		unlock();

		return _invalid;
	}

	remote.setAddress(aPacket->getStringAt(0));
	if (aPacket->typeAt(1) == Packet::type_string) remote.setService(aPacket->getStringAt(1));
	if (aPacket->typeAt(2) == Packet::type_string) remote.setSelector(aPacket->getStringAt(2));
	if (aPacket->typeAt(3) == Packet::type_unsigned) timeout = aPacket->getUnsignedAt(3);

	log(log_debug, "sequencer") << "transferring to: " << remote << logend();

	lock();

#ifdef __RECOGNIZER__
	if (recognizer) recognizer->stop();
#endif
	m_activity.stop();

	unlock();

	m_trunk->transfer(remote, timeout);
*/
	return PHONE_OK;
}

int Sequencer::disconnect(InterfaceConnection *server, const std::string &id)
{
	int cause(0);

	(*server) >> cause;

	// cause is optional
	if (server->eof())
	{
		server->clear();
	}

	lock();

	if (m_id.size())
	{
		unlock();
 		server->begin() << PHONE_ERROR_PROTOCOL_VIOLATION << ' ' << id.c_str() 
			<< " DISC protocol violation" << end();

		return PHONE_ERROR_PROTOCOL_VIOLATION;
	}

	m_disconnecting = true;
	m_id = id;
	unlock();

	int rc = disconnect(cause);

	if (rc != PHONE_OK) 
	{
 		server->begin() << rc << ' ' << id.c_str() 
			<< " DISC" << end();
	}

	return rc;
}

int Sequencer::disconnect(int cause)
{
	int rc = PHONE_OK;

	omni_mutex_lock l(m_mutex);

	if (m_activity.getState() == Activity::active)
	{
		log(log_debug, "sequencer", m_trunk->getName()) 
			<< "disconnect - stopping activity" << logend();

		m_activity.stop();
	}

	checkCompleted();

	if (m_activity.getState() == Activity::idle)
	{
		log(log_debug, "sequencer", m_trunk->getName()) << "disconnect - activity idle" << logend();

		m_media->disconnected(m_trunk);
		rc = m_trunk->disconnect(m_callref, cause);
	}
	else
		log(log_debug, "sequencer", m_trunk->getName()) << "disconnect - stopping activity" << logend();


	return rc;
}

void Sequencer::onIncoming(Trunk* server, unsigned callref, const SAP& local, const SAP& remote)
{
	int contained; 

	omni_mutex_lock lock(m_mutex);

	m_callref = callref;

	// do we have an exact match?
	m_clientSpec = m_configuration->dequeue(local);
	if (m_clientSpec)
	{
		log(log_debug, "sequencer", m_trunk->getName()) 
			<< "id: " << m_clientSpec->m_id
			<< " found client matching: " << local << logend();
	}
	else
	{
		// no exact match. 
		// is the destination known so far part of a client spec?
		contained = m_configuration->isContained(local);
		
		if (!contained)
		{
			// ok. look in the global queue
			m_clientSpec = gClientQueue.dequeue();
			if (m_clientSpec)
			{
				log(log_debug, "sequencer", m_trunk->getName()) 
					<< "id: " << m_clientSpec->m_id
					<< " found client in global queue" << logend();
			}
		}
	}
	if (m_clientSpec)
	{
		m_interface = m_clientSpec->m_interface;
		m_interface->add(m_trunk->getName(), this);

		m_local = local;
		m_remote = remote;

		if (m_interface)
		{
			m_interface->begin() << PHONE_OK << ' ' << m_clientSpec->m_id.c_str() 
				<< " LSTN " << m_trunk->getName()
				<< " \"" << m_remote.getAddress() << "\" \""
				<< m_configuration->getNumber() << "\" \""
				<< m_local.getAddress() << "\" "
				<< server->getTimeslot().ts << end();
		}
	}
	else 
	{
		if (!contained)
		{
			log(log_warning, "sequencer", m_trunk->getName()) 
				<< "no client found. rejecting call." << logend(); 

			m_trunk->disconnect(m_callref);
		}
		else
		{
			log(log_debug, "sequencer", m_trunk->getName()) 
				<< "received partial local address: " << local 
				<< logend();
		}
	}
}

// Protocol of TrunkClient. Basically forwards it's information to the remote client

void Sequencer::connectRequest(Trunk* server, unsigned callref,
							   const SAP &local, const SAP &remote)
{
	if (m_connectComplete)
	{
		m_trunk->disconnect(callref);

		log(log_debug, "sequencer", m_trunk->getName()) 
			<< "rejecting request because of outstanding outgoing call" << logend();
	}
	else
	{
		onIncoming(server, callref, local, remote);

		if (!m_connectComplete)
		{
			log(log_debug, "sequencer", m_trunk->getName()) 
				<< "connect request from: " 
				<< remote << " , " << local	<< " [" << server->getTimeslot() << ']' 
				<< logend();
		}
	}
}

void Sequencer::connectDone(Trunk* server, unsigned callref, int result)
{
	log(log_debug, "sequencer", m_trunk->getName()) 
		<< "connect done: " << result << logend();

	if (result == PHONE_OK)
	{
		// good. we got through

		m_media->connected(server);

		if (m_connectComplete)
		{
			m_connectComplete->m_interface->begin() 
				<< m_connectComplete->m_id.c_str() << ' ' << result 
				<< (result == PHONE_OK ? " connected" : " connect failed") << end();

			delete m_connectComplete;

			m_connectComplete = 0;
		}
		else
		{
			// Todo log error
		}
	}
	else
	{
		lock();
		m_callref = INVALID_CALLREF;
		if (m_interface)
		{
			m_interface->remove(server->getName());
		}
		unlock();
	}
}

void Sequencer::transferDone(Trunk *server, unsigned callref, int result)
{
	log(log_debug, "sequencer", m_trunk->getName()) 
		<< "transfer succeeded" << logend();

	m_activity.stop();

	checkCompleted();

/* Todo

	lock();
	packet->clear(1);

	packet->setContent(phone_transfer_done);
	packet->setUnsignedAt(0, _ok);
 
	tcp.send(*packet);
	unlock();

*/
}

void Sequencer::disconnectRequest(Trunk *server, unsigned callref, int cause)
{
	log(log_debug, "sequencer", m_trunk->getName()) 
		<< "disconnect request" << logend();
	
	omni_mutex_lock lock(m_mutex);

	// if we are active, stop
	if (m_activity.getState() == Activity::active)
	{
		log(log_debug, "sequencer", m_trunk->getName()) 
			<< "disconnect request - stopping activity" << logend();

		m_activity.stop();
	}

	checkCompleted();

	// notify client unless already disconnecting or still stopping
	if (m_interface && !m_disconnecting && m_activity.getState() == Activity::idle)
	{
		m_interface->begin() << PHONE_EVENT << " RDIS " << m_trunk->getName() 
			<< end();
	}
}

void Sequencer::disconnectDone(Trunk *server, unsigned callref, int result)
{
	log(log_debug, "sequencer", server->getName()) 
		<< "call disconnected" << logend();

	omni_mutex_lock lock(m_mutex);

	if (m_interface)
	{
		m_interface->remove(server->getName());

		assert(m_id.size());

		m_interface->begin() << result << ' ' << m_id.c_str() << " DISC "
			<< m_trunk->getName() << end();
	}

	m_id.erase();

	m_disconnecting = false;
	m_callref = INVALID_CALLREF;
}

int Sequencer::accept(InterfaceConnection *server, const std::string &id)
{
	omni_mutex_lock l(m_mutex);

	if (m_id.size())
	{
		if (server)
		{
 			server->begin() << PHONE_ERROR_PROTOCOL_VIOLATION << ' ' << id.c_str() 
				<< " protocol violation" << end();
		}

		return PHONE_ERROR_PROTOCOL_VIOLATION;
	}

	m_id = id;

	int rc = m_trunk->accept(m_callref);

	if (rc != PHONE_OK) 
	{
 		server->begin() << rc << ' ' << id.c_str() << " ACPT"
			<< end();
	}

	return rc;
}

void Sequencer::acceptDone(Trunk *server, unsigned callref, int result)
{
	if (result == PHONE_OK)
	{
		log(log_debug, "sequencer", server->getName()) << "call accepted" << logend();

		m_media->connected(server);

		if (m_interface)
		{
			m_interface->begin() << PHONE_OK << ' ' << m_id.c_str() << " ACPT " 
				<< m_trunk->getName() << end();
		}

		lock();
		delete m_clientSpec;
		m_clientSpec = 0;
		m_id.erase();
		unlock();
	}
	else
	{
		lock();
		m_callref = INVALID_CALLREF;
		if (m_interface)
		{
			m_interface->remove(server->getName());
		}
		unlock();

		log(log_error, "sequencer", server->getName()) << "call accept failed: " 
			<< result << logend();
	}	
}

void Sequencer::details(Trunk *server, unsigned callref, const SAP& local, const SAP& remote)
{
	log(log_debug, "sequencer", server->getName()) 
		<< "details: " << local << " " << remote << logend();

	onIncoming(server, callref, local, remote);
}

void Sequencer::remoteRinging(Trunk *server, unsigned callref)
{
	log(log_debug, "sequencer", server->getName()) 
		<< "remote end ringing" << logend();

/* Todo

	lock();
	packet->clear(1);

	packet->setContent(phone_connect_remote_ringing);
	packet->setStringAt(0, "not implemented"); //server->getLocalSAP().getSelector());
 
	tcp.send(*packet);
	unlock();
*/
}

void Sequencer::started(Media *server, Sample *aSample)
{
	Molecule* m = (Molecule*)aSample->getUserData();

	if (m->notifyStart())
	{
		log(log_debug, "sequencer", server->getName()) << " sent ABEG for " 
			<< m->getId() << ", " 
			<< m->currentAtom() << logend();

		if (m_interface)
		{
			m_interface->begin() << PHONE_EVENT << m->getId() << " ABEG "
				<< m->currentAtom() << end();
		}
	}
}

void Sequencer::completed(Media *server, Sample *aSample, unsigned msecs)
{
	completed(server, (Molecule*)(aSample->getUserData()), msecs, aSample->getStatus());
}

void Sequencer::completed(Media* server, Molecule* molecule, unsigned msecs, unsigned status)
{
	int done, atEnd, notifyStop;
	unsigned pos, length, nAtom;
	std::string id;
	std::string jobid;

	omni_mutex_lock lock(m_mutex);

	assert(molecule);

	// the molecule will be changed after the done. Grab all necesssary information before done.

	nAtom = molecule->currentAtom();
	atEnd = molecule->atEnd();
	notifyStop = molecule->notifyStop();
	id = molecule->getId();
	done = molecule->done(this, msecs, status);
	pos = molecule->getPos();
	length = molecule->getLength();
	atEnd = atEnd && done;

	if (notifyStop)
	{
		log(log_debug, "sequencer", server->getName()) 
			<< "sent atom-done for " 
			<< ", " << id.c_str() << ", " << nAtom << std::endl 
			<< *molecule << logend();

		sendAtomDone(id.c_str(), nAtom, status, msecs);
	}

	if (m_activity.getState() == Activity::stopping || done)
	{
		log(log_debug+2, "sequencer", server->getName()) 
			<< "removing " << *molecule << logend();

		m_activity.remove(molecule);
	}
	else
	{
		log(log_debug+2, "sequencer", server->getName()) 
			<< "done " << *molecule << logend();
	}

	// handle the various disconnect conditions
	if (m_disconnecting)
	{
		sendMoleculeDone(id.c_str(), PHONE_ERROR_DISCONNECTED, pos, length);

		log(log_debug, "sequencer", m_trunk->getName()) << "disconnect - activity idle" << logend();

		m_media->disconnected(m_trunk);
		m_trunk->disconnect(m_callref);

		return;
	}

	if (m_trunk->remoteDisconnect())
	{
		sendMoleculeDone(id.c_str(), PHONE_ERROR_DISCONNECTED, pos, length);
		if (m_interface)
		{
			m_interface->begin() << PHONE_EVENT << " RDIS " << m_trunk->getName() 
				<< end();
		}

		return;
	}

	// start next molecule before sending reply to minimise delay
	m_activity.start();

	if (atEnd)
	{
		sendMoleculeDone(id.c_str(), status, pos, length);
	}
}

void Sequencer::touchtone(Media* server, char tt)
{
	log(log_debug, "sequencer", server->getName())
		<< "DTMF: " << tt << logend();

	if (m_interface)
	{
		m_interface->begin() << PHONE_EVENT << " DTMF " << server->getName() 
			<< ' ' << tt << end();
	}
}

void Sequencer::fatal(Media* server, const char* e)
{
	log(log_error, "sequencer", server->getName()) 
		<< "fatal Media exception: " << e << std::endl
		<< "terminating" << logend();

	exit(5);
}

void Sequencer::fatal(Media* server, Exception& e)
{
	log(log_error, "sequencer", server->getName()) 
		<< "fatal Media exception: " << e << std::endl
		<< "terminating" << logend();

	exit(5);
}

void Sequencer::addCompleted(Media* server, Molecule* molecule, unsigned msecs, unsigned status)
{
	m_delayedCompletions.enqueue(server, molecule, msecs, status);
}

void Sequencer::checkCompleted()
{
	CompletedQueue::Item* i = m_delayedCompletions.dequeue();

	if (i)
	{
		completed(i->server, i->molecule, i->msecs, i->status);
		delete i;
	}
}

bool Sequencer::data(InterfaceConnection* server, const std::string &command,
					 const std::string &id)
{
	// the main packet inspection method...

	if (command == "ACPT")
		accept(server, id);
	else if (command == "MLCA")
		addMolecule(server, id);
	else if (command == "MLCD")
		discardMolecule(server, id);
	else if (command == "MLDP")
		discardByPriority(server, id);
	else if (command == "DISC")
		disconnect(server, id);
	else if (command == "TRSF")
		transfer(server, id);
	else
	{
		server->begin() << PHONE_FATAL_SYNTAX << ' ' << id.c_str()  
			<< " syntax error - unknown command " << command.c_str()
			<< end();

		return false;
	}

	server->clear();

	return true;
}

void usage()
{
	std::cerr << "usage: " << std::endl << "sequence -[dl]" << std::endl;

	exit(1);
}

int main(int argc, char* argv[])
{
	int c;
	int analog = 0;
	int sw = 0;
	char szKey[256];

	ULONG rc;
	WSADATA wsa;

	set_log_instance(&cout_log);
	set_log_level(2);

	rc = WSAStartup(MAKEWORD(2,0), &wsa);

	/*
	 * commandline parsing
	 */
	while( (c = getopt(argc, argv, "d:l:")) != EOF) {
		switch(c) 
		{
		case 'd':
			set_log_level(atoi(optarg));
			std::cout << "debug level " << atoi(optarg) << std::endl;
			break;
		case 'l':
			std::cout << "logging to: " << optarg << std::endl;
			// todo
			break;
		case '?':
			usage();
		default:
			usage();
		}
	}

	try
	{
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
			gBus = new H100;

			log(log_debug, "app") << "using H.100 for switching" << logend();
		}
		else if (swmode.ct_buses & 1 << SWMODE_CTBUS_SCBUS)
		{
			gBus = new SCbus;

			log(log_debug, "app") << "using SCbus for switching" << logend();
		}

		for (unsigned index = 0; 1; index++)
		{
			sprintf(szKey, "SOFTWARE\\ibp\\voice3\\Aculab PRI\\Trunk%d", index);

			RegistryKey key(HKEY_LOCAL_MACHINE, szKey);
			if (!key.exists())	break;

			TrunkConfiguration* trunk = new AculabPRITrunkConfiguration();

			if (!trunk->readFromKey(key))
			{
				log(log_error, "sequencer") 
					<< "error reading trunk description: " << szKey << logend();
				return 2;
			}

			gConfiguration.add(trunk);

			trunk->start();
		}

		if (gConfiguration.numElements() == 0)
		{
			int card = 0;
			int nports = call_nports();
			if (nports)
			{
				for (int m = 0; m < nports;)
				{
					struct sysinfo_xparms sysinfo;
					memset(&sysinfo, 0, sizeof(sysinfo));
					sysinfo.net = m;
					int rc = call_system_info(&sysinfo);
					if (rc)
					{
						log(log_error, "sequencer") 
							<< "call_system_info() returned: " << rc << logend();
					}

					int lines = 0;

					switch(sysinfo.cardtype)
					{
					case C_REV4:
					case C_REV5:
					case C_PM4:
						lines = 30;
						break;
					case C_BR4:
					case C_BR8:
						lines = 2;
						break;
					default:
						log(log_error, "sequencer") 
							<< "unsupported card type " << sysinfo.cardtype << logend();
						return 0;
					}

					for (int n = 0; n < sysinfo.nphys; ++n)
					{
						AculabPRITrunkConfiguration* trunk = new AculabPRITrunkConfiguration();

						trunk->init(m + n, card, lines);

						gConfiguration.add(trunk);

						trunk->start();
					}

					m += sysinfo.nphys;
					++card;
				}
			}
		}

		if (gConfiguration.numElements() == 0)
		{
			log(log_error, "sequencer") 
				 << "no trunks configured - nothing to do" << logend();

			return 0;
		}

		// start interface here

		AculabTrunk::start();
		AculabMedia::start();
		Sequencer::getTimer().start();

		SAP local;
		
		local.setService(interface_port);

		Interface iface(local);

		iface.run();

	}
	catch (const char* e)
	{
		log(log_error, "sequencer") << "exception: " << e << ". terminating." << logend();

		return 2;
	}
	catch (Exception& e)
	{
		log(log_error, "sequencer") << "exception: " << e << ". terminating." << logend();

		return 2;
	}
	catch(...)
	{
		log(log_error, "sequencer") << "unknown exception. terminating." << logend();

		return 2;
	}

	return 0;
}

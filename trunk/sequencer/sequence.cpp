/*
	Copyright 1995-2001 Lars Immisch

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
  :	activity(this), configuration(aConfiguration), connectComplete(0),
	clientSpec(0), outOfService(0), m_interface(0)
{
	Timeslot receive = gBus->allocate();
	Timeslot transmit = gBus->allocate();

	m_media = new AculabMedia(this, aConfiguration->getSwitch(), 
		receive, transmit);

	m_trunk = aConfiguration->getTrunk(this);
	
	m_trunk->listen();
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

		server->syntax_error(id) << "expecting mode and priority\r\n"; 

		return _syntax_error;
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

			return _failed;
		}
		catch (const Exception& e)
		{
			log(log_error, "sequencer") << e << logend();
			atom = 0;

			return _failed;
		}

		molecule->add(*atom);

		(*server) >> type;
	}

	if (molecule->getSize() == 0)
	{
		log(log_warning, "sequencer") << "Molecule is empty. will not be added." 
			<< logend();

		return _empty;
	}

	activity.add(*molecule);
	checkCompleted();

	return _ok;
}


int Sequencer::discardMolecule(InterfaceConnection *server, const std::string &id)
{

	std::string mid;

	(*server) >> mid;

	if (!server->good())
	{
		server->syntax_error(id) << "expecting <molecule id>\r\n";
		return _syntax_error;
	}

	Molecule* molecule = activity.find(mid);

	if (!molecule) 
	{
		log(log_error, "sequencer") << "discard molecule: (" << mid.c_str() 
			<< ") not found" << logend();

		(*server) << id.c_str() << ' ' << _invalid << " invalid molecule "
			<< mid.c_str() << "\r\n";

		return _invalid;
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
		activity.remove(molecule);

		(*server) << id.c_str() << ' ' << _ok << " molecule " << m_id.c_str()
			<< " removed\r\n";
	}

	return _ok;
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
		server->syntax_error(id) << "expecting <fromPriority> <toPriority> <immediately>\r\n";
		return _syntax_error;
	}

	log(log_debug, "sequencer") << "discarding molecules with " << fromPriority 
		<< " <= priority <= " << toPriority << logend();

	omni_mutex_lock lock(mutex);

	for (ActivityIter i(activity); !i.isDone(); i.next())
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
			activity.remove(molecule);
			
			log(log_debug, "sequencer") << "removed molecule "  
				<< molecule->getId() << logend();
		}
	}

	if (discarded)
	{
		(*server) << id.c_str() << ' ' << _ok << " removed molecules with priorities from " 
			<< fromPriority << " to " << toPriority << "\r\n";
	}

	return _ok;
}

void Sequencer::sendAtomDone(const char *id, unsigned nAtom, unsigned status, unsigned msecs)
{
	(*m_interface) << id << ' ' << _event << " atom-done " 
		<< nAtom << ' ' << status << ' ' << msecs << "\r\n";
}

void Sequencer::sendMoleculeDone(const char *id, unsigned status, unsigned pos, unsigned length)
{
	log(log_debug + 2, "sequencer", m_trunk->getName())
		<< "send molecule done for: " << id << " status: " << status << " pos: " 
		<< pos << " length: " << length << logend();


	(*m_interface) << id << ' ' << status << ' ' << m_trunk->getName()
		<< " molecule-done " << pos << ' ' << length << "\r\n";
}

int Sequencer::connect(ConnectCompletion* complete)
{
	lock();

	if (connectComplete	|| m_trunk->getState() != Trunk::listening)
	{
		log(log_debug, "sequencer", m_trunk->getName())
			<< "connect failed - invalid state: " 
			<< m_trunk->getState() << logend();

		unlock();
		return _busy;
	}
	else if (outOfService)
	{
		unlock();
		return _out_of_service;
	}

	connectComplete = complete;

	unlock();

	return _ok;
}

int Sequencer::accept(InterfaceConnection *server, const std::string &id)
{
	if (m_id.size())
	{
 		(*server) << _protocol_violation << ' ' << id.c_str() 
			<< " protocol violation\r\n";

		return _protocol_violation;
	}

	// completion in acceptDone
	m_id = id;

	m_trunk->accept();

	return _ok;
}

int Sequencer::reject(InterfaceConnection *server, const std::string &id)
{
	if (m_id.size())
	{
 		(*server) << _protocol_violation << ' ' << id.c_str() 
			<< " protocol violation\r\n";

		return _protocol_violation;
	}

	// completion in acceptDone
	m_id = id;

	m_trunk->reject();

	return _ok;
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
	activity.stop();

	unlock();

	m_trunk->transfer(remote, timeout);
*/
	return _ok;
}

int Sequencer::disconnect(InterfaceConnection *server, const std::string &id)
{
	int c = 0;
	std::string cause;

	(*server) >> cause;

	// cause is optional
	if (!server->good())
		server->clear();
	else
		c = atoi(cause.c_str());

	if (m_id.size())
	{
 		(*server) << _protocol_violation << ' ' << id.c_str() 
			<< " protocol violation\r\n";

		return _protocol_violation;
	}

	m_id = id;

	log(log_debug, "sequencer", m_trunk->getName()) << "disconnecting" << logend();

	m_trunk->disconnect(c);
	m_media->disconnected(m_trunk);

	checkCompleted();

	return _ok;
}

void Sequencer::onIncoming(Trunk* server, const SAP& local, const SAP& remote)
{
	int contained; 

	// do we have an exact match?
	clientSpec = configuration->dequeue(local);
	if (clientSpec)
	{
		log(log_debug, "sequencer", m_trunk->getName()) 
			<< "found client matching: " << local 
			<< " id: " << clientSpec->m_id << logend();
	}
	else
	{
		// no exact match. 
		// is the destination known so far part of a client spec?
		contained = configuration->isContained(local);
		
		if (!contained)
		{
			// ok. look in the global queue
			clientSpec = gClientQueue.dequeue();
			if (clientSpec)
			{
				log(log_debug, "sequencer", m_trunk->getName()) 
					<< "found client in global queue, remote: " 
					<< clientSpec->m_id << logend();
			}
		}
	}
	if (clientSpec)
	{
		lock();

		m_interface = clientSpec->m_interface;
		m_interface->add(m_trunk->getName(), this);

		(*m_interface) << clientSpec->m_id.c_str() << ' ' << _ok 
			<< ' ' << m_trunk->getName()
			<< " listen-done "
			<< " \"" << remote.getAddress() << "\" \""
			<< configuration->getNumber() << "\" \""
			<< local.getService() << "\" "
			<< server->getTimeslot().ts << "\r\n";

		unlock();
	}
	else 
	{
		if (!contained)
		{
			log(log_debug, "sequencer", m_trunk->getName()) 
				<< "no client found. rejecting call." << logend(); 

			m_trunk->reject(0);
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

void Sequencer::connectRequest(Trunk* server, const SAP &local, const SAP &remote)
{
	if (connectComplete)
	{
		m_trunk->reject();

		log(log_debug, "sequencer", m_trunk->getName()) 
			<< "rejecting request because of outstanding outgoing call" << logend();
	}
	else
	{
		onIncoming(server, local, remote);

		if (!connectComplete)
		{
			log(log_debug, "sequencer", m_trunk->getName()) 
				<< "connect request from: " 
				<< remote << " , " << local	<< " [" << server->getTimeslot() << ']' 
				<< logend();
		}
	}
}

void Sequencer::connectRequestFailed(Trunk* server, int cause)
{
	if (cause == _aborted && connectComplete)
	{
		log(log_debug, "sequencer", m_trunk->getName()) << "connecting to " 
			<< connectComplete->m_id.c_str() 
			<< " for dialout" << logend();

		// todo better info
		(*connectComplete->m_interface) << connectComplete->m_id.c_str() 
			<< ' ' << _failed << "\r\n";

	}
	else
	{
		log(log_debug, "sequencer", m_trunk->getName()) 
			<< "connect request failed with " << cause 
			<< logend();

		m_trunk->listen();
	}
}

void Sequencer::connectDone(Trunk* server, int result)
{
	log(log_debug, "sequencer", m_trunk->getName()) 
		<< "connect done: " << result << logend();

	// good. we got through

	m_media->connected(server);

	if (connectComplete)
	{
		(*connectComplete->m_interface) << connectComplete->m_id.c_str()
			<< ' ' << result 
			<< (result == _ok ? " connected\r\n" : " connect failed\r\n");

		delete connectComplete;

		connectComplete = 0;
	}
	else
	{
		// Todo log error
	}
}

void Sequencer::transferDone(Trunk *server)
{
	log(log_debug, "sequencer", m_trunk->getName()) 
		<< "transfer succeeded" << logend();

	activity.stop();

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

void Sequencer::transferFailed(Trunk *server, int cause)
{
	log(log_warning, "sequencer", m_trunk->getName()) 
		<< "transfer failed: " << cause << logend();

/* Todo

	lock();
	packet->clear(1);

	packet->setContent(phone_transfer_done);
	packet->setUnsignedAt(0, cause);
 
	tcp.send(*packet);
	unlock();
*/
}

void Sequencer::disconnectRequest(Trunk *server, int cause)
{
	log(log_debug, "sequencer", m_trunk->getName()) 
		<< "disconnect request" << logend();

	m_media->disconnected(server);
	
	lock();

	if (activity.getState() == Activity::active)
	{
		activity.stop();

		checkCompleted();

		if (activity.getState() == Activity::idle)
		{
			(*m_interface) << _event << ' ' << m_trunk->getName() 
				<< " disconnect\r\n";
		}
	}

	unlock();
}

void Sequencer::disconnectDone(Trunk *server, unsigned result)
{
	log(log_debug, "sequencer", server->getName()) 
		<< "call disconnected" << logend();

	if (m_id.size())
	{
		(*m_interface) << m_id.c_str() << ' ' << result << ' '
			<< m_trunk->getName() << " disconnect-done\r\n";
	}
	else if (m_interface)
	{
		(*m_interface) << "-1 " << _event << ' '
			<< m_trunk->getName() << " disconnect\r\n";
	}

	m_id.erase();

	server->listen();
}

void Sequencer::acceptDone(Trunk *server, unsigned result)
{
	if (result == r_ok)
		log(log_debug, "sequencer", server->getName()) << "call accepted" << logend();
	else
		log(log_debug, "sequencer", server->getName()) << "call accept failed: " 
		<< result << logend();

	m_media->connected(server);

	(*m_interface) << m_id.c_str() << ' ' << result
		<< " " << m_trunk->getName() << " accept-done\r\n";

	m_id.erase();
}

void Sequencer::rejectDone(Trunk *server, unsigned result)
{
	// result is always _ok

	lock();

	if (connectComplete)
	{
		// internal reject. an outgoing call is outstanding

		m_interface = connectComplete->m_interface;

		unlock();

		m_trunk->connect(connectComplete->m_local, connectComplete->m_remote, 
			connectComplete->m_timeout);

		log(log_debug, "sequencer", server->getName()) 
			<< "connecting to: " << connectComplete->m_remote 
			<< " timeout: " << connectComplete->m_timeout 
			<< " as: " << connectComplete->m_local << logend();
	}
	else
	{
		delete clientSpec;
		clientSpec = 0;

		log(log_debug, "sequencer", server->getName()) 
			<< "call rejected" << logend();

		(*m_interface) << m_id.c_str() << ' ' << result << server->getName() 
			<< " reject-done\r\n";

		m_id.erase();

		unlock();
		m_trunk->listen();
	}
}

void Sequencer::details(Trunk *server, const SAP& local, const SAP& remote)
{
	log(log_debug, "sequencer", server->getName()) 
		<< "details: " << local << " " << remote << logend();

	onIncoming(server, local, remote);
}

void Sequencer::remoteRinging(Trunk *server)
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
		log(log_debug, "sequencer", server->getName()) << "sent atom-started for " 
			<< m->getId() << ", " 
			<< m->currentAtom() << logend();

		
		(*m_interface) << m->getId() << ' ' << _event << " atom-started "
			<< m->currentAtom() << "\r\n";
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

	omni_mutex_lock lock(mutex);

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

	if (activity.getState() == Activity::stopping || done)
	{
		log(log_debug+2, "sequencer", server->getName()) 
			<< "removing " << *molecule << logend();

		activity.remove(molecule);
	}
	else
	{
		log(log_debug+2, "sequencer", server->getName()) 
			<< "done " << *molecule << logend();
	}

	if (m_trunk->getState() == Trunk::disconnecting)
	{
		sendMoleculeDone(id.c_str(), status, pos, length);

		m_trunk->disconnect();

		return;
	}

	// start mext molecule before sending reply to minimise delay
	activity.start();

	if (atEnd)
	{
		sendMoleculeDone(id.c_str(), status, pos, length);
	}
}

void Sequencer::touchtone(Media* server, char tt)
{
	(*m_interface) << "-1 0 " <<
	server->getName() << " touchtone " << tt 
	<< "\r\n";
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
	delayedCompletions.enqueue(server, molecule, msecs, status);
}

void Sequencer::checkCompleted()
{
	CompletedQueue::Item* i = delayedCompletions.dequeue();

	if (i)
	{
		completed(i->server, i->molecule, i->msecs, i->status);
		delete i;
	}
}

void Sequencer::data(InterfaceConnection* server, const std::string &id)
{
	// the main packet inspection method...

	std::string command;

	(*server) >> command;

	if (command == "add")
		addMolecule(server, id);
	else if (command == "discard")
		discardMolecule(server, id);
	else if (command == "discard-by-priority")
		discardByPriority(server, id);
	else if (command == "accept")
		accept(server, id);
	else if (command == "reject")
		reject(server, id);
	else if (command == "disconnect")
		disconnect(server, id);
	else if (command == "transfer")
		transfer(server, id);
	else
	{
		(*server) << id.c_str() << ' ' << _failed 
			<< " syntax error - unknown command " << command.c_str()
			<< "\r\n";
	}

	server->clear();
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
	set_log_level(4);

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
			sprintf(szKey, "SOFTWARE\\Immisch, Becker & Partner\\Voice Server\\Aculab PRI\\Trunk%d", index);

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

		// start interface here

		if (gConfiguration.numElements() == 0)
		{
			log(log_error, "sequencer") 
				 << "no trunks configured - nothing to do" << logend();

			return 0;
		}

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

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

#include "fal.h"
#include "phoneclient.h"
#include "acuphone.h"
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
MediaPool gMediaPool;

CTbus *gBus;

Timer Sequencer::timer;

// I use 'this' in the base member initializer list on purpose
#pragma warning(disable : 4355)

Sequencer::Sequencer(TrunkConfiguration* aConfiguration) 
  :	m_configuration(aConfiguration), m_connectComplete(0),
	m_clientSpec(0), m_disconnecting(INVALID_CALLREF), 	m_sent_rdis(INVALID_CALLREF),
	m_interface(0),	m_callref(INVALID_CALLREF), 
	m_trunk(0), m_media(0), m_closing(false)

{
	for (int i = 0; i < MAXCHANNELS; ++i)
	{
		m_activity[i].m_sequencer = this;
	}

	m_receive = gBus->allocate();
	m_transmit = gBus->allocate();

	m_trunk = aConfiguration->getTrunk(this);
		
	m_trunk->listen();
}

Sequencer::Sequencer(InterfaceConnection *server) 
  :	m_configuration(0), m_connectComplete(0),
	m_clientSpec(0), m_disconnecting(INVALID_CALLREF), 	m_sent_rdis(INVALID_CALLREF),
	m_interface(server), m_callref(INVALID_CALLREF), 
	m_trunk(0), m_media(0), m_closing(false)
{
	m_receive = gBus->allocate();
	m_transmit = gBus->allocate();

	for (int i = 0; i < MAXCHANNELS; ++i)
	{
		m_activity[i].m_sequencer = this;
	}

	m_media = gMediaPool.allocate(this);
	if (!m_media)
	{
		throw Exception(__FILE__, __LINE__, "Sequencer::Sequencer()", "no media resources");
	}
}

void Sequencer::lost_connection()
{
	lock();
	m_interface = 0;
	unlock();

	if (m_callref != INVALID_CALLREF)
	{
		disconnect(m_callref);
	}
	else if (!m_trunk)
	{
		close();
	}
}

void Sequencer::release()
{
	gMediaPool.release(m_media);
	m_media = 0;
	m_id.erase();

	m_disconnecting = INVALID_CALLREF;
	m_callref = INVALID_CALLREF;
	m_sent_rdis = INVALID_CALLREF;

	for (int i = 0; i < MAXCHANNELS; ++i)
	{
		m_activity[i].empty();
	}
}

#pragma warning(default : 4355)

unsigned Sequencer::MLCA(InterfaceConnection *server, const std::string &id)
{
	unsigned channel;
	unsigned mode;
	unsigned priority;

	(*server) >> channel;
	(*server) >> mode;
	(*server) >> priority;

	if (!server->good() || server->eof())
	{
		server->clear();

		server->syntax_error(id) << "expecting channel, mode and priority" << end(); 

		return V3_FATAL_SYNTAX;
	}

	if (channel >= MAXCHANNELS)
	{
		server->clear();

		server->syntax_error(id) << "invalid channel" << end(); 

		return V3_FATAL_SYNTAX;
	}

	Molecule* molecule = new Molecule(channel, mode, priority, id);

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

				atom = new PlayAtom(channel, this, file.c_str());
			}
			else if (type == "rec")
			{
				std::string file;
				unsigned max;

				(*server) >> file;
				(*server) >> max;

				atom = new RecordAtom(channel, this, file.c_str(), max);
			}
			else if (type == "dtmf")
			{
				std::string tt;

				(*server) >> tt;

				atom = new TouchtoneAtom(channel, this, tt.c_str());
			}
			else if (type == "beep")
			{
				unsigned count;

				(*server) >> count;

				atom = new BeepAtom(channel, this, count);

			}
			else if (type == "slnc")
			{
				int len;

				(*server) >> len;

				atom = new SilenceAtom(channel, len);
			}
			else if (type == "conf")
			{
				std::string conf;

				(*server) >> conf;

				unsigned handle(0);

				if (sscanf(conf.c_str(), "conf[%d]", &handle) != 1)
				{
					server->clear();

					server->syntax_error(id) << "invalid conference descriptor" 
						<< end(); 

					return V3_FATAL_SYNTAX;
				}

				std::string s;
				Conference::mode mode(Conference::duplex);

				(*server) >> s;

				if (s == "listen")
					mode = Conference::listen;
				else if (s == "speak")
					mode = Conference::speak;
				else if (s == "duplex")
					mode = Conference::duplex;

				atom = new ConferenceAtom(channel, handle, mode);
			}

			std::string notifications;

			(*server) >> notifications;

			if (notifications == "start")
				atom->setNotifications(NOTIFY_START);
			else if (notifications == "stop")
				atom->setNotifications(NOTIFY_STOP);
			else if (notifications == "both")
				atom->setNotifications(NOTIFY_START | NOTIFY_STOP);
			// todo: syntax error if not 'none'
		}
		catch (const char *e)
		{
			log(log_error, "sequencer") << "Exception in addMolecule: " << e << logend();
			atom = 0;

			return sendMLCA(id, V3_ERROR_FAILED, molecule->getPos(), molecule->getLength());
		}
		catch (const FileDoesNotExist &e)
		{
			log(log_error, "sequencer") << e << logend();
			atom = 0;

			return sendMLCA(id, V3_ERROR_NOT_FOUND, molecule->getPos(), molecule->getLength());

		}
		catch (const Exception &e)
		{
			log(log_error, "sequencer") << e << logend();
			atom = 0;

			return sendMLCA(id.c_str(), V3_ERROR_FAILED, molecule->getPos(), molecule->getLength());
		}

		molecule->add(*atom);

		(*server) >> type;
	}

	if (molecule->getSize() == 0)
	{
		log(log_warning, "sequencer") << "Molecule is empty. will not be added." 
			<< logend();

		return V3_WARNING_EMPTY;
	}

	lock();

	m_activity[channel].add(*molecule);
	checkCompleted();

	unlock();

	return V3_OK;
}


unsigned Sequencer::MLCD(InterfaceConnection *server, const std::string &id)
{
	std::string mid;

	(*server) >> mid;

	if (!server->good())
	{
		server->syntax_error(id) << "expecting <molecule id>" << end();
		return V3_FATAL_SYNTAX;
	}

	Molecule* molecule = 0;
	for (int i = 0; i < MAXCHANNELS && !molecule; ++i)
	{
		molecule = m_activity[i].find(mid);
	}

	if (!molecule) 
	{
		log(log_error, "sequencer") << "discard molecule: (" << mid.c_str() 
			<< ") not found" << logend();

		server->begin() << V3_ERROR_NOT_FOUND << ' ' << id.c_str() << " invalid molecule "
			<< mid.c_str() << end();

		return V3_ERROR_NOT_FOUND;
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
		m_activity[molecule->getChannel()].remove(molecule);

		server->begin() << V3_OK << ' ' << id.c_str() 
			<< " MLCD " << m_id.c_str() << " removed" << end();
	}

	return V3_OK;
}

unsigned Sequencer::MLDP(InterfaceConnection *server, const std::string &id)
{
	bool done = true;
	std::string channels;
	int ic = -1;
	int fromPriority;
	int toPriority;
	int immediately;

	(*server) >> channels;
	(*server) >> fromPriority;
	(*server) >> toPriority;
	(*server) >> immediately;

	if (!server->good())
	{
		server->syntax_error(id) << "expecting <channels> <fromPriority> <toPriority> <immediately>" << end();
		return V3_FATAL_SYNTAX;
	}

	log(log_debug, "sequencer") << "discarding molecules for channel(s) " 
		<< channels << " with " << fromPriority 
		<< " <= priority <= " << toPriority << logend();

	if (channels != "all")
	{
		ic = atoi(channels.c_str());
	}

	omni_mutex_lock lock(m_mutex);

	for (int i = 0; i < MAXCHANNELS; ++i)
	{
		if (ic == i || ic == -1)
		{
			for (ActivityIter ai(m_activity[i]); !ai.isDone(); ai.next())
			{
				Molecule *molecule = ai.current();

				if (molecule->getMode() & Molecule::dont_interrupt)  
				{
					done = false;
					continue;
				}

				if (molecule->getPriority() >= fromPriority && molecule->getPriority() <= toPriority)
				// if active, stop and send ack when stopped, else remove and send ack immediately
				if (molecule->isActive() && immediately)
				{
					done = false;	

					molecule->setMode(Molecule::discard);
					molecule->stop(this);
					
					log(log_debug, "sequencer") << "stopped molecule " 
						<< molecule->getId() << logend();

					checkCompleted();
				}
				else
				{
					log(log_debug, "sequencer") << "removing molecule "  
						<< molecule->getId() << logend();

					m_activity[i].remove(molecule);
				}
			}
		}
	}

	// Todo: also send MLDP when stop was asynchronous
	if (done)
	{
		server->begin() << V3_OK << ' ' << id.c_str()
			<< " MLDP " << getName() << end();
	}

	return V3_OK;
}

unsigned Sequencer::sendATOM(const std::string &id, unsigned nAtom, 
						 unsigned status, unsigned msecs)
{
	if (m_interface)
	{
		m_interface->begin() << V3_EVENT << " ATOM " 
			<< getName() << ' ' << id << ' '
			<< nAtom << ' ' << status << ' ' << msecs << end();
	}

	return status;
}

unsigned Sequencer::sendMLCA(const std::string &id, unsigned status, 
						 unsigned pos, unsigned length)
{
	if (m_interface)
	{
		m_interface->begin() << status << ' ' << id << " MLCA " << getName()
			<< ' ' << pos << ' ' << length << end();
	}

	log(log_debug, "sequencer", getName())
		<< "sent molecule done for: " << id << " status: " << status << " pos: " 
		<< pos << " length: " << length << logend();

	return status;
}

void Sequencer::sendRDIS()
{
	lock();

	if (m_sent_rdis == m_callref)
	{
		unlock();
	}
	else
	{
		m_sent_rdis = m_callref;
		unlock();

		if (m_interface)
		{
			m_interface->begin() << V3_EVENT << " RDIS " << getName() 
				<< end();
		}
	}
}

unsigned Sequencer::connect(ConnectCompletion* complete)
{
	omni_mutex_lock lock(m_mutex);

	if (m_connectComplete)
	{
		log(log_debug, "sequencer", getName())
			<< "connect failed - busy" << logend();

		return V3_ERROR_BUSY;
	}

	m_media = gMediaPool.allocate(this);
	if (!m_media)
	{
		log(log_warning, "sequencer", getName()) 
			<< "connect failed - could no allocate media channel." << logend(); 

		return V3_ERROR_NO_RESOURCE;
	}

	m_connectComplete = complete;

	return V3_OK;
}

unsigned Sequencer::TRSF(InterfaceConnection *server, const std::string &id)
{
/* Todo

	SAP remote;
	unsigned timeout = indefinite;

	if (aPacket->typeAt(0) != Packet::type_string)
	{
		lock();

		packet->clear(1);
		packet->setSync(aPacket->getSyncMajor(), aPacket->getSyncMinor());
		packet->setContent(V3_transfer_done);
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
	return V3_OK;
}

unsigned Sequencer::DISC(InterfaceConnection *server, const std::string &id)
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
 		server->begin() << V3_ERROR_PROTOCOL_VIOLATION << ' ' << id.c_str() 
			<< " DISC protocol violation" << end();

		return V3_ERROR_PROTOCOL_VIOLATION;
	}

	m_id = id;
	unlock();

	int rc = disconnect(cause);

	if (rc != V3_OK) 
	{
 		server->begin() << rc << ' ' << id.c_str() 
			<< " DISC" << end();
	}

	return rc;
}

bool Sequencer::channelsIdle()
{
	bool idle(true);

	for (int i = 0; i < MAXCHANNELS; ++i)
	{
		idle = (m_activity[i].getState() == Activity::idle) && idle;
	}

	return idle;
}

unsigned Sequencer::disconnect(int cause)
{
	int rc = V3_OK;

	omni_mutex_lock l(m_mutex);
	m_disconnecting = m_callref;

	for (int i = 0; i < MAXCHANNELS; ++i)
	{
		if (m_activity[i].getState() == Activity::active)
		{
			log(log_debug, "sequencer", getName()) 
				<< "disconnect - stopping channel " << i << logend();

			m_activity[i].stop();
		}
	}

	checkCompleted();

	if (channelsIdle())
	{
		log(log_debug, "sequencer", getName()) << "disconnect - all channels idle" << logend();

		m_media->disconnected(m_trunk);
		rc = m_trunk->disconnect(m_callref, cause);
	}

	return rc;
}

unsigned Sequencer::BGRC(const std::string &id)
{
	bool idle(false);

	lock();

	if (m_id.size())
	{
		unlock();
		if (m_interface)
		{
 			m_interface->begin() << V3_ERROR_PROTOCOL_VIOLATION << ' ' << id.c_str() 
				<< " BGRC " << getName() << end();
		}

		return V3_ERROR_PROTOCOL_VIOLATION;
	}

	unlock();

	if (close(id.c_str()))
	{
		if (m_interface)
		{
			m_interface->begin() << V3_OK << ' ' << id.c_str() << " BGRC " << getName() 
				<< end();
		}
		delete this;
	}

	return V3_OK;
}

bool Sequencer::close(const char *id)
{
	bool idle(false);

	lock();

	m_closing = true;

	for (int i = 0; i < MAXCHANNELS; ++i)
	{
		if (m_activity[i].getState() == Activity::active)
		{
			log(log_debug, "sequencer", getName()) 
				<< "close - stopping channel " << i << logend();

			m_activity[i].abort();
		}
	}

	checkCompleted();

	if (channelsIdle())
	{
		log(log_debug, "sequencer", getName()) << "close - all channels idle" << logend();

		idle = true;
		gMediaPool.release(m_media);
		m_media = 0;
	}

	if (!idle && id)
		m_id = id;

	unlock();

	return idle;
}

void Sequencer::onIncoming(Trunk* server, unsigned callref, const SAP& local, const SAP& remote)
{
	int contained; 

	omni_mutex_lock lock(m_mutex);

	m_media = gMediaPool.allocate(this);

	if (!m_media)
	{
		log(log_warning, "sequencer", getName()) 
			<< "could no allocate media channel. rejecting call." << logend(); 

		m_trunk->disconnect(m_callref);
		return;
	}

	m_callref = callref;

	// do we have an exact match?
	m_clientSpec = m_configuration->dequeue(local);
	if (m_clientSpec)
	{
		log(log_debug, "sequencer", getName()) 
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
				log(log_debug, "sequencer", getName()) 
					<< "id: " << m_clientSpec->m_id
					<< " found client in global queue" << logend();
			}
		}
	}
	if (m_clientSpec)
	{
		m_interface = m_clientSpec->m_interface;
		m_interface->add(getName(), this);

		m_local = local;
		m_remote = remote;

		if (m_interface)
		{
			m_interface->begin() << V3_OK << ' ' << m_clientSpec->m_id.c_str() 
				<< " LSTN " << getName()
				<< " '" << m_remote.getAddress() << "' '"
				<< m_configuration->getNumber() << "' '"
				<< m_local.getAddress() << "' "
				<< server->getTimeslot().ts << end();
		}
	}
	else 
	{
		if (!contained)
		{
			log(log_warning, "sequencer", getName()) 
				<< "no client found. rejecting call." << logend(); 

			m_trunk->disconnect(m_callref);
		}
		else
		{
			log(log_debug, "sequencer", getName()) 
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

		log(log_debug, "sequencer", getName()) 
			<< "rejecting request because of outstanding outgoing call" << logend();
	}
	else
	{
		onIncoming(server, callref, local, remote);

		if (!m_connectComplete)
		{
			log(log_debug, "sequencer", getName()) 
				<< "connect request from: " 
				<< remote << " , " << local	<< " [" << server->getTimeslot() << ']' 
				<< logend();
		}
	}
}

void Sequencer::connectDone(Trunk* server, unsigned callref, int result)
{
	log(log_debug, "sequencer", getName()) 
		<< "connect done: " << result << logend();

	if (result == V3_OK)
	{
		// good. we got through

		m_media->connected(server);

		if (m_connectComplete)
		{
			m_connectComplete->m_interface->begin() 
				<< m_connectComplete->m_id.c_str() << ' ' << result 
				<< (result == V3_OK ? " connected" : " connect failed") << end();

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
	log(log_debug, "sequencer", getName()) 
		<< "transfer succeeded" << logend();

/* Todo

	lock();
	packet->clear(1);

	packet->setContent(V3_transfer_done);
	packet->setUnsignedAt(0, _ok);
 
	tcp.send(*packet);
	unlock();

*/
}

void Sequencer::disconnectRequest(Trunk *server, unsigned callref, int cause)
{

	omni_mutex_lock lock(m_mutex);

	// if we are active, stop
	for (int i = 0; i < MAXCHANNELS; ++i)
	{
		if (m_activity[i].getState() == Activity::active)
		{
			log(log_debug, "sequencer", getName()) 
				<< "remote disconnect - stopping channel " << i << logend();

			m_activity[i].abort(V3_DISCONNECTED);
		}
	}

	checkCompleted();

	// notify client unless already disconnecting or still stopping
	if (channelsIdle() && m_disconnecting == INVALID_CALLREF)
	{
		sendRDIS();
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
			<< getName() << end();
	}

	release();
}

unsigned Sequencer::ACPT(InterfaceConnection *server, const std::string &id)
{
	omni_mutex_lock l(m_mutex);

	if (m_id.size())
	{
		if (server)
		{
 			server->begin() << V3_ERROR_PROTOCOL_VIOLATION << ' ' << id.c_str() 
				<< " protocol violation" << end();
		}

		return V3_ERROR_PROTOCOL_VIOLATION;
	}

	m_id = id;

	int rc = m_trunk->accept(m_callref);

	if (rc != V3_OK) 
	{
 		server->begin() << rc << ' ' << id.c_str() << " ACPT"
			<< end();
	}

	return rc;
}

void Sequencer::acceptDone(Trunk *server, unsigned callref, int result)
{
	if (result == V3_OK)
	{
		log(log_debug, "sequencer", server->getName()) << "call accepted" << logend();

		m_media->connected(server);

		if (m_interface)
		{
			m_interface->begin() << V3_OK << ' ' << m_id.c_str() << " ACPT " 
				<< getName() << end();
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
		gMediaPool.release(m_media);
		m_media = 0;

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
			m_interface->begin() << V3_EVENT << m->getId() << " ABEG "
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
	bool done(false), send_atom_done(false), send_molecule_done(false), start(true);
	unsigned pos, length, atom, channel;
	std::string id, jobid;

	lock();

	assert(molecule);

	// the molecule will be changed after the done. Grab all necesssary information before done.

	channel = molecule->getChannel();
	atom = molecule->currentAtom();
	send_molecule_done = molecule->atEnd();
	send_atom_done = molecule->notifyStop();
	id = molecule->getId();
	done = molecule->done(this, msecs, status);
	pos = molecule->getPos();
	length = molecule->getLength();
	send_molecule_done = send_molecule_done && done;

	if (send_atom_done)
	{
		log(log_debug, "sequencer", server->getName()) 
			<< "sent ATOM done for " 
			<< ", " << id.c_str() << ", " << atom << std::endl 
			<< *molecule << logend();

		sendATOM(id.c_str(), atom, status, msecs);
	}

	if (m_activity[channel].getState() == Activity::stopping || done)
	{
		log(log_debug+2, "sequencer", server->getName()) 
			<< "removing " << *molecule << logend();

		m_activity[channel].remove(molecule);
	}
	else
	{
		log(log_debug+2, "sequencer", server->getName()) 
			<< "done " << *molecule << logend();
	}

	// handle the various disconnect/close conditions

	if ((m_trunk && (m_disconnecting != INVALID_CALLREF || m_trunk->remoteDisconnect()))
		|| m_closing)
	{
		start = false;
		send_molecule_done = true;
	}

	if (start)
	{
		// start next molecule before sending reply to minimise delay
		m_activity[channel].start();
	}
	else
	{
		// current channel is idle
		m_activity[channel].setState(Activity::idle);
	}

	if (send_molecule_done)
	{
		sendMLCA(id.c_str(), status, pos, length);
	}

	unlock();

	if (!channelsIdle())
	{
		return;
	}

	// various disconnect/close conditions continued

	if (m_trunk)
	{
		// active DISC
		if (m_disconnecting != INVALID_CALLREF)
		{
			log(log_debug, "sequencer", getName()) << "disconnect - all channels idle" 
				<< logend();

			m_media->disconnected(m_trunk);
			m_trunk->disconnect(m_disconnecting);
		}
		// remote disconnect
		else if (m_trunk->remoteDisconnect())
		{
			log(log_debug, "sequencer", getName()) << "remote disconnect - all channels idle" 
				<< logend();

			sendRDIS();
		}
	}

	if (m_closing)
	{
		log(log_debug, "sequencer", getName()) << "close - all channels idle" << logend();

		if (m_interface)
		{
			m_interface->begin() << V3_OK << ' ' << m_id.c_str() << " BGRC " 
				<< getName() << end();
		}

		release();

		delete this;
	}
}

void Sequencer::touchtone(Media* server, char tt)
{
	log(log_debug, "sequencer", server->getName())
		<< "DTMF: " << tt << logend();

	if (m_interface)
	{
		m_interface->begin() << V3_EVENT << " DTMF " << server->getName() 
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
		ACPT(server, id);
	else if (command == "MLCA")
		MLCA(server, id);
	else if (command == "MLCD")
		MLCD(server, id);
	else if (command == "MLDP")
		MLDP(server, id);
	else if (command == "DISC")
		DISC(server, id);
	else if (command == "TRSF")
		TRSF(server, id);
	else
	{
		server->begin() << V3_FATAL_SYNTAX << ' ' << id.c_str()  
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
	int nmodules = 0;
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

		/* If nothing was configured, start everything with default values */
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

		nmodules = sm_get_modules();
		log(log_debug, "sequencer") 
			 << nmodules << " prosody modules found" << logend();

		gMediaPool.add(nmodules * 30);

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
		
		local.setService(SEQUENCER_PORT);

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

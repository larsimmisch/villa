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

#ifdef __AG__
Conferences gConferences;
#endif

ConfiguredTrunks gConfiguration;
ClientQueue gClientQueue;

CTbus *gBus;

Timer Sequencer::timer;

// I use 'this' in the base member initializer list on purpose
#pragma warning(disable : 4355)

Sequencer::Sequencer(TrunkConfiguration* aConfiguration) 
  :	activity(this), configuration(aConfiguration), connectComplete(0),
	clientSpec(0), outOfService(0)
{
	Timeslot receive = gBus->allocate();
	Timeslot transmit = gBus->allocate();

	phone = new AculabPhone(this, aConfiguration->getTrunk(this), 0, 
		receive, transmit);

	phone->listen();
}

#pragma warning(default : 4355)

int Sequencer::addMolecule(InterfaceConnection *server, const std::string &id)
{
/* Todo:

	int result;
	unsigned pos = 0;
	unsigned syncMinor = aPacket->getSyncMinor();
	
	omni_mutex_lock lock(mutex);

	while(pos < aPacket->getNumArgs())
	{
		result = addMolecule(aPacket, &pos);
		if (result != _ok)
		{
			log(log_error, "sequencer") << "add molecule failed" << logend();
			
			packet->clear(1);

			packet->setSync(aPacket->getSyncMajor(), syncMinor);
			packet->setContent(phone_add_molecule_done);

			packet->setUnsignedAt(0, result);
			tcp.send(*packet);

			return _invalid;
		}
	}
	packet->clear(1);

	int index;
	unsigned pos = *pPos;
	unsigned pos2;
	int numAtoms;
	unsigned notifications;
	Atom* atom;

	// mode, priority, numAtoms

	if (aPacket->typeAt(pos) != Packet::type_unsigned ||
		aPacket->typeAt(pos+1) != Packet::type_unsigned ||
		aPacket->typeAt(pos+2) != Packet::type_unsigned)	return _invalid;

	// Molecule's ctor parms are mode, priority, syncMinor
	Molecule* aMolecule = new Molecule(aPacket->getUnsignedAt(pos), aPacket->getUnsignedAt(pos+1), aPacket->getSyncMinor());

	pos += 2;

	numAtoms = aPacket->getUnsignedAt(pos++);
	for (index = 0; index < numAtoms; index++)
	{
		notifications = aPacket->getUnsignedAt(pos++);
		
		pos2 = pos + 1;
		try
		{
			switch(aPacket->getUnsignedAt(pos++))
			{
			case atom_play_file_sample:
				// check validity
				if (aPacket->typeAt(pos) != Packet::type_string)
				{
					log(log_error, "sequencer") 
						<< "invalid packet contents for addMolecule(atom_play_sample)." 
						<< std::endl << *aPacket << logend();
					
					return _invalid;
				}
				// we still get the message number, but ignore it
				pos += 2;

				atom = new PlayAtom(this, aPacket->getStringAt(pos2));

				break;
			case atom_record_file_sample:
				// check validity
				if (aPacket->typeAt(pos) != Packet::type_string 
					|| aPacket->typeAt(pos+1) != Packet::type_unsigned)
				{
					log(log_error, "sequencer") 
						<< "invalid packet contents for addMolecule(atom_record_sample)." 
						<< std::endl << *aPacket << logend();
					
					return _invalid;
				}

				// we still get the message number, but ignore it
				pos += 3;

				atom = new RecordAtom(this, aPacket->getStringAt(pos2), 
					aPacket->getUnsignedAt(pos2+1));

				break;
			case atom_beep:
				// check validity
				if (aPacket->typeAt(pos) != Packet::type_unsigned)
				{
					log(log_error, "sequencer")
						<< "invalid packet contents for addMolecule(atom_beep)." 
						<< std::endl << *aPacket << logend();

					return _invalid;
				}
				pos += 1;

				atom = new BeepAtom(this, aPacket->getUnsignedAt(pos2));

				break;
			case atom_conference:
				// check validity
				if (aPacket->typeAt(pos) != Packet::type_unsigned || aPacket->typeAt(pos+1) != Packet::type_unsigned)
				{
					log(log_error, "sequencer") 
						<< "invalid packet contents for addMolecule(atom_conference)." 
						<< std::endl << *aPacket << logend();

					return _invalid;
				}
				pos += 2;

				atom = new ConferenceAtom(aPacket->getUnsignedAt(pos2), aPacket->getUnsignedAt(pos2+1));

				break;
			case atom_touchtones:
				// check validity
				if (aPacket->typeAt(pos) != Packet::type_string)
				{
					log(log_error, "sequencer") 
						<< "invalid packet contents for addMolecule(atom_touchtone)." 
						<< std::endl << *aPacket << logend();

					return _invalid;
				}
				pos += 1;

				atom = new TouchtoneAtom(this, aPacket->getStringAt(pos2));

				break;
			case atom_silence:
				// check validity
				if (aPacket->typeAt(pos) != Packet::type_unsigned)
				{
					log(log_error, "sequencer")
						<< "invalid packet contents for addMolecule(atom_silence)." 
						<< std::endl << *aPacket << logend();

					return _invalid;
				}
				pos += 1;

				atom = new SilenceAtom(aPacket->getUnsignedAt(pos2));

				break;
			}
		}
		catch (char* e)
		{
			log(log_error, "sequencer") << "Exception in addMolecule: " << e << logend();
			atom = 0;
		}
		catch (Exception& e)
		{
			log(log_error, "sequencer") << e << logend();
			atom = 0;
		}
		if (atom)
		{
			atom->setNotifications(notifications);
			aMolecule->add(*atom);
		}
		else if (notifications & notify_done)
		{
			sendAtomDone(aPacket->getSyncMinor(), index, _failed, 0);
		}
	}

	if (aMolecule->getSize() == 0)
	{
		log(log_warning, "sequencer") << "Molecule is empty. will not be added." 
			<< std::endl << *aPacket << logend();

		return _empty;
	}

	activity.add(*aMolecule);
	checkCompleted();

	packet->setSync(aPacket->getSyncMajor(), syncMinor);
	packet->setContent(phone_add_molecule_done);

	packet->setUnsignedAt(0, _ok);
	tcp.send(*packet);

*/
	return _ok;
}


int Sequencer::discardMolecule(InterfaceConnection *server, const std::string &id)
{
/* Todo

	omni_mutex_lock lock(mutex);

	unsigned syncMajor = aPacket->getSyncMajor();
	unsigned syncMinor = aPacket->getSyncMinor();

	Molecule* molecule = activity.find(syncMinor);

	if (!molecule) 
	{
		log(log_error, "sequencer") << "discard molecule: (" << syncMajor << ", "
			<< syncMinor << ") not found" << logend();

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
		packet->clear(1);
		packet->setSync(syncMajor, molecule->getSyncMinor());
		packet->setContent(phone_discard_molecule_done);
		packet->setUnsignedAt(0, _ok);

		activity.remove(molecule);

		tcp.send(*packet);
	}

*/
	return _ok;
}

int Sequencer::discardByPriority(InterfaceConnection *server, const std::string &id)
{
/* Todo

	unsigned syncMajor = aPacket->getSyncMajor();

	Molecule* molecule;
	int discarded = 0;

	unsigned fromPriority = aPacket->getUnsignedAt(0);
	unsigned toPriority = aPacket->getUnsignedAt(1);
	int immediately = aPacket->getUnsignedAt(2);

	log(log_debug, "sequencer") << "discarding molecules with " << fromPriority 
		<< " <= priority <= " << toPriority << logend();

	omni_mutex_lock lock(mutex);

	for (ActivityIter i(activity); !i.isDone(); i.next())
	{
		molecule = i.current();

		if (molecule->getMode() & Molecule::dont_interrupt)  continue;

		if (molecule->getPriority() >= fromPriority && molecule->getPriority() <= toPriority)
		// if active, stop and send ack when stopped, else remove and send ack immediately
		if (molecule->isActive() && immediately)
		{
			molecule->setMode(Molecule::discard);
			molecule->stop(this);
			
			log(log_debug, "sequencer") << "stopped molecule (" << syncMajor 
				<< ", " << molecule->getSyncMinor() << ')' << logend();

			checkCompleted();
		}
		else
		{
			discarded++;
			activity.remove(molecule);
			
			log(log_debug, "sequencer") << "removed molecule (" << syncMajor 
				<< ", " << molecule->getSyncMinor() << ')' << logend();
		}
	}

	if (discarded)
	{
		packet->clear(2);
		packet->setSync(syncMajor, 0);
		packet->setContent(phone_discard_molecule_priority_done);
		packet->setUnsignedAt(0, _ok);
		packet->setUnsignedAt(discarded, _ok);

		tcp.send(*packet);
	}

*/
	return _ok;
}

void Sequencer::sendAtomDone(unsigned syncMinor, unsigned nAtom, unsigned status, unsigned msecs)
{
/* Todo

	lock();
	packet->clear(3);
	packet->setSync(0, syncMinor);
	packet->setContent(phone_atom_done);
	packet->setUnsignedAt(0, nAtom);
	packet->setUnsignedAt(1, status);
	packet->setUnsignedAt(2, msecs);
 
	tcp.send(*packet);
	unlock();

*/
}

void Sequencer::sendMoleculeDone(unsigned syncMinor, unsigned status, unsigned pos, unsigned length)
{
	log(log_debug + 2, "sequencer", phone->getName())
		<< "send molecule done for: 0." << syncMinor << " status: " << status << " pos: " 
		<< pos << " length: " << length << logend();

/* Todo

	lock();
	packet->clear(3);
	packet->setSync(0, syncMinor);
	packet->setContent(phone_molecule_done);
	packet->setUnsignedAt(0, status);
	packet->setUnsignedAt(1, pos);
	packet->setUnsignedAt(2, length);
 
	tcp.send(*packet);
	unlock();

*/
}

int Sequencer::connect(ConnectCompletion* complete)
{
	lock();

	if (connectComplete	|| phone->getState() != Trunk::listening)
	{
		log(log_debug, "sequencer", phone->getName())
			<< "connect failed - invalid state: " 
			<< phone->getState() << logend();

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

	// Todo
	phone->abort();

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

	phone->accept();

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

	phone->reject();

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

	phone->transfer(remote, timeout);
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

	log(log_debug, "sequencer", phone->getName()) << "disconnecting" << logend();

	phone->disconnect(c);

	checkCompleted();

	return _ok;
}

int Sequencer::abort(InterfaceConnection *server, const std::string &id)
{
/* Todo

	lock();

#ifdef __RECOGNIZER__
	if (recognizer) delete recognizer;
	recognizer = 0;
#endif

	unlock();

	Timer::sleep(200);
	
	log(log_debug, "sequencer") << "aborting..." << logend();

	checkCompleted();

	phone->abort();
*/
	return _ok;
}

void Sequencer::onIncoming(Trunk* server, const SAP& local, const SAP& remote)
{
	int contained; 

	// do we have an exact match?
	clientSpec = configuration->dequeue(local);
	if (clientSpec)
	{
		log(log_debug, "sequencer", phone->getName()) 
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
				log(log_debug, "sequencer", phone->getName()) 
					<< "found client in global queue, remote: " 
					<< clientSpec->m_id << logend();
			}
		}
	}
	if (clientSpec)
	{
		lock();

		m_interface = clientSpec->m_interface;
		m_interface->add(phone->getName(), this);

		(*m_interface) << clientSpec->m_id.c_str() << ' ' << _ok 
			<< " alerting "
			<< phone->getName() << " \"" << remote.getAddress() << "\" \""
			<< configuration->getNumber() << "\" \""
			<< local.getService() << "\" "
			<< server->getTimeslot().ts << "\r\n";

		unlock();
	}
	else 
	{
		if (!contained)
		{
			log(log_debug, "sequencer", phone->getName()) 
				<< "no client found. rejecting call." << logend(); 

			phone->reject(0);
		}
		else
		{
			log(log_debug, "sequencer", phone->getName()) 
				<< "received partial local address: " << local 
				<< logend();
		}
	}
}

// Protocol of Telephone Client. Basically forwards it's information to the remote client

void Sequencer::connectRequest(Trunk* server, const SAP &local, const SAP &remote)
{
	if (connectComplete)
	{
		phone->reject();

		log(log_debug, "sequencer", phone->getName()) 
			<< "rejecting request because of outstanding outgoing call" << logend();
	}
	else
	{
		onIncoming(server, local, remote);

		if (!connectComplete)
		{
			log(log_debug, "sequencer", phone->getName()) 
				<< "telephone connect request from: " 
				<< remote << " , " << local	<< " [" << server->getTimeslot() << ']' 
				<< logend();
		}
	}
}

void Sequencer::connectRequestFailed(Trunk* server, int cause)
{
	if (cause == _aborted && connectComplete)
	{
		log(log_debug, "sequencer", phone->getName()) << "connecting to " 
			<< connectComplete->m_id.c_str() 
			<< " for dialout" << logend();

		// todo better info
		(*connectComplete->m_interface) << connectComplete->m_id.c_str() 
			<< ' ' << _failed << "\r\n";

	}
	else
	{
		log(log_debug, "sequencer", phone->getName()) 
			<< "connect request failed with " << cause 
			<< logend();

		phone->listen();
	}
}

void Sequencer::connectDone(Trunk* server, int result)
{
	log(log_debug, "sequencer", phone->getName()) 
		<< "telephone connect done: " << result << logend();

	// good. we got through

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
	log(log_debug, "sequencer", phone->getName()) 
		<< "telephone transfer succeeded" << logend();

	activity.setASAP();
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
	log(log_warning, "sequencer", phone->getName()) 
		<< "telephone transfer failed: " << cause << logend();

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
	log(log_debug, "sequencer", phone->getName()) 
		<< "telephone disconnect request" << logend();

	server->disconnectAccept();

	lock();

	activity.setASAP();
	activity.stop();

	checkCompleted();

/* Todo
	packet->clear(0);
	packet->setContent(phone_disconnect);
 
	tcp.disconnect(indefinite, packet);
*/

	unlock();
}

void Sequencer::disconnectDone(Trunk *server, unsigned result)
{
	log(log_debug, "sequencer", server->getName()) 
		<< "call disconnected" << logend();

	(*m_interface) << m_id.c_str() << ' ' << result << ' '
		<< phone->getName() << " disconnect-done\r\n";

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

	(*m_interface) << m_id.c_str() << ' ' << result << " accept-done\r\n";

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

		phone->connect(connectComplete->m_local, connectComplete->m_remote, 
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
		phone->listen();
	}
}

void Sequencer::details(Trunk *server, const SAP& local, const SAP& remote)
{
	log(log_debug, "sequencer", server->getName()) 
		<< "telephone details: " << local << " " << remote << logend();

	onIncoming(server, local, remote);
}

void Sequencer::remoteRinging(Trunk *server)
{
	log(log_debug, "sequencer", server->getName()) 
		<< "telephone remote end ringing" << logend();

/* Todo

	lock();
	packet->clear(1);

	packet->setContent(phone_connect_remote_ringing);
	packet->setStringAt(0, "not implemented"); //server->getLocalSAP().getSelector());
 
	tcp.send(*packet);
	unlock();
*/
}

void Sequencer::started(Telephone *server, Sample *aSample)
{
	Molecule* m = (Molecule*)aSample->getUserData();

	if (m->notifyStart())
	{
		log(log_debug, "sequencer", server->getName()) << "sent atom_started for 0, " 
			<< m->getSyncMinor() << ", " 
			<< m->currentAtom() << logend();
 
/* Todo

		lock();
		packet->clear(1);
		packet->setSync(0, m->getSyncMinor());
		packet->setContent(phone_atom_started);
		packet->setUnsignedAt(0, m->currentAtom());
 
		tcp.send(*packet);
		unlock();

*/
	}
}

void Sequencer::completed(Telephone *server, Sample *aSample, unsigned msecs)
{
	completed(server, (Molecule*)(aSample->getUserData()), msecs, aSample->getStatus());
}

void Sequencer::completed(Telephone* server, Molecule* aMolecule, unsigned msecs, unsigned status)
{
	int done, atEnd, notifyStop, started;
	unsigned pos, length, nAtom, syncMinor;

	omni_mutex_lock lock(mutex);

	if (aMolecule)
	{
		// the molecule will be changed after the done. Grab all necesssary information before done.

		nAtom = aMolecule->currentAtom();
		atEnd = aMolecule->atEnd();
		notifyStop = aMolecule->notifyStop();
		syncMinor = aMolecule->getSyncMinor();
		done = aMolecule->done(this, msecs, status);
		pos = aMolecule->getPos();
		length = aMolecule->getLength();
		atEnd = atEnd && done;

		if (notifyStop)
		{
			log(log_debug, "sequencer", server->getName()) 
				<< "sent atom_done for 0, " 
				<< ", " << aMolecule->getSyncMinor() 
				<< ", " << nAtom << std::endl << *aMolecule << logend();

			sendAtomDone(aMolecule->getSyncMinor(), nAtom, status, msecs);
		}

		if (done)
		{
			if (done) 
			{
				log(log_debug+2, "sequencer", server->getName()) 
					<< "removing " << *aMolecule << logend();
			}

			activity.remove(aMolecule);
		}
		else
		{
			log(log_debug+2, "sequencer", server->getName()) 
				<< "done " << *aMolecule << logend();
		}
 
		// start the new activity before sending packets to minimize delay

		if (activity.isDisabled() || activity.isDiscarded())	
		{
			if (!atEnd && !activity.isASAP())
			{
				started = activity.start();
			}
		}
		else 
		{
			started = activity.start();
		}

		if (atEnd)
		{
			sendMoleculeDone(syncMinor, status, pos, length);
		}
	}
}

void Sequencer::touchtone(Telephone* server, char tt)
{
/* Todo

	char s[2];

	s[0] = tt;
	s[1] = '\0';

	lock();

	packet->clear(1);
	packet->setContent(phone_touchtones);
	packet->setStringAt(0, s);

	tcp.send(*packet);
	unlock();

*/
}


/* Todo

void Sequencer::disconnectRequest(Transport* server, Packet* finalPacket)
{
	// blast it. reset the phone->
	lock();

	activity.setASAP();
	activity.stop();

	unlock();

	if (!phone->isIdle())
		phone->abort();

	checkCompleted();

	tcp.disconnectAccept();
}

*/

void Sequencer::fatal(Telephone* server, const char* e)
{
	log(log_error, "sequencer", server->getName()) 
		<< "fatal Telephone exception: " << e << std::endl
		<< "terminating" << logend();

	exit(5);
}

void Sequencer::fatal(Telephone* server, Exception& e)
{
	log(log_error, "sequencer", server->getName()) 
		<< "fatal Telephone exception: " << e << std::endl
		<< "terminating" << logend();

	exit(5);
}

void Sequencer::addCompleted(Telephone* server, Molecule* molecule, unsigned msecs, unsigned status)
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
	else if (command == "abort")
		abort(server, id);
	else if (command == "transfer")
		transfer(server, id);
	else
	{
		(*server) << id.c_str() << ' ' << _failed 
			<< " syntax error - unknown command " << command.c_str()
			<< "\r\n";
	}
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
		AculabPhone::start();
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

/*
	Copyright 1995-2001 Lars Immisch

	created: Tue Jul 25 16:05:02 GMT+0100 1995

	Author: Lars Immisch <lars@ibp.de>
*/

#ifdef _WIN32
#include <windows.h>
#else
#define INCL_BASE
#include <os2.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <new.h>

#include "phoneclient.h"
#include "acuphone.h"
#include "rphone.h"
#include "bitset.h"
#include "activ.h"
#include "sequence.h"
#include "interface.h"
#include "getopt.h"

int debug = 0;

bitset<1024> SCBus;

Log cout_log(std::cout);

#ifdef __AG__
Conferences gConferences;
#endif

ConfiguredTrunks gConfiguration;
ClientQueue gClientQueue;

Timer Sequencer::timer;

// I use 'this' in the base member initializer list on purpose
#pragma warning(disable : 4355)

Sequencer::Sequencer(TrunkConfiguration* aConfiguration) 
  :	tcp(*this), activities(), activity(0), nextActivity(0), 
	mutex(), configuration(aConfiguration), connectComplete(0),
	clientSpec(0), outOfService(0)
{
	packet = new(buffer) Packet(0, sizeof(buffer));

	Timeslot receive(24, SCBus.lowest_bit());
	SCBus.set_bit(receive.ts, false);

	Timeslot transmit(24, SCBus.lowest_bit());
	SCBus.set_bit(transmit.ts, false);

	phone = new AculabPhone(this, aConfiguration->getTrunk(this), 0, 
		receive, transmit);

	phone->listen();
}

#pragma warning(default : 4355)

int Sequencer::addActivity(Packet* aPacket)
{	
	unsigned syncMajor = aPacket->getSyncMajor();
	if (syncMajor == 0) syncMajor = activities.getSize() +1;

	Activity* activ = new Activity(syncMajor, this);

	// reply success or failures

	omni_mutex_lock lock(mutex);

	packet->clear(1);
	packet->setSync(syncMajor, 0);

	packet->setContent(phone_add_activity_done);

	if (findActivity(syncMajor))
	{
		log(log_debug, "sequencer") << "add activity " << syncMajor 
			<< " duplicate."<< logend();

		packet->setUnsignedAt(0, _duplicate);
		
		tcp.send(*packet);

		return _duplicate;
	}

	if (activ)
	{
		log(log_debug, "sequencer") << "added activity " << syncMajor << logend();
			
		activities.add(*activ);
		packet->setUnsignedAt(0, _ok);
	}
	else packet->setUnsignedAt(0, _failed);

	tcp.send(*packet);

	return _ok;
}

int Sequencer::addMolecule(Packet* aPacket)
{
	int result;
	unsigned pos = 0;
	unsigned syncMajor = aPacket->getSyncMajor();
	unsigned syncMinor = aPacket->getSyncMinor();
	
	omni_mutex_lock lock(mutex);

	Activity* anActivity = findActivity(syncMajor);

	if (anActivity == 0)
	{
		log(log_warning, "sequencer") << "add molecule to unknown activity " 
			<< syncMajor << logend();

		packet->clear(1);

		packet->setSync(syncMajor, syncMinor);
		packet->setContent(phone_add_molecule_done);
 
		packet->setUnsignedAt(0, _invalid);
		tcp.send(*packet);

		return _invalid;
	}

	while(pos < aPacket->getNumArgs())
	{
		result = addMolecule(aPacket, *anActivity, &pos);
		if (result != _ok)
		{
			log(log_error, "sequencer") << "add molecule failed" << logend();
			
			packet->clear(1);

			packet->setSync(syncMajor, syncMinor);
			packet->setContent(phone_add_molecule_done);

			packet->setUnsignedAt(0, result);
			tcp.send(*packet);

			return _invalid;
		}
	}
	packet->clear(1);

	packet->setSync(syncMajor, syncMinor);
	packet->setContent(phone_add_molecule_done);

	packet->setUnsignedAt(0, _ok);
	tcp.send(*packet);

	return _ok;
}

int Sequencer::addMolecule(Packet* aPacket, Activity& anActivity, unsigned* pPos)
{
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
						<< endl << *aPacket << logend();
					
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
						<< endl << *aPacket << logend();
					
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
						<< endl << *aPacket << logend();

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
						<< endl << *aPacket << logend();

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
						<< endl << *aPacket << logend();

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
						<< endl << *aPacket << logend();

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
			<< endl << *aPacket << logend();

		return _empty;
	}

	anActivity.add(*aMolecule);
	checkCompleted();

	*pPos = pos;

	return _ok;
}

int Sequencer::discardActivity(Packet* aPacket)
{
	unsigned syncMajor = aPacket->getSyncMajor();
	unsigned immediately = aPacket->getUnsignedAt(0);

	omni_mutex_lock lock(mutex);

	Activity* activ = findActivity(syncMajor);

	if (!activ || activ->isDiscarded()) 
	{
		if (!activ) 
			log(log_error, "sequencer") << "discard unknown activity " << syncMajor 
			<< logend();
		else
			log(log_warning, "sequencer") << "discard activity " << syncMajor 
			<< " is already discarded" << logend();
 
		packet->clear(1);
		packet->setSync(syncMajor, 0);
		packet->setContent(phone_discard_activity_done);
		packet->setUnsignedAt(0, _invalid);

		tcp.send(*packet);

		return _invalid;
	}

	// if active, set discarded and send ack when stopped, else remove and send ack immediately
	if (activ->isActive() && !activ->isIdle())
	{
		activ->setDiscarded();
		if (immediately)	
		{
			activ->setASAP();
			log(log_debug, "sequencer") << "discard active activity " << syncMajor 
				<< " as soon as possible" << logend();
			if (activ->stop() == -1) 
			{
				log(log_debug, "sequencer") << "activity " << syncMajor 
					<< " was not started yet" << logend();
			}
		}
		else
		{
			log(log_debug, "sequencer") << "discard active activity " << syncMajor 
				<< " when finished" << logend();
		}

		checkCompleted();
	}
	else
	{
		log(log_debug, "sequencer") << "discarded activity " << syncMajor << logend();

		if (activ == activity)
			activity = 0;

		activities.remove(activ);

		packet->clear(1);
		packet->setSync(syncMajor, 0);
		packet->setContent(phone_discard_activity_done);
		packet->setUnsignedAt(0, _ok);

		tcp.send(*packet);
	}

	return _ok;
}

int Sequencer::discardMolecule(Packet* aPacket)
{
	omni_mutex_lock lock(mutex);

	unsigned syncMajor = aPacket->getSyncMajor();
	unsigned syncMinor = aPacket->getSyncMinor();

	Activity* activ = findActivity(syncMajor);

	if (activ)
	{
		log(log_error, "sequencer") << "discard molecule: activity " << syncMajor
			<< " not found" << logend();

		return _invalid;
	}

	Molecule* molecule = activ->find(syncMinor);

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
		packet->setSync(activ->getSyncMajor(), molecule->getSyncMinor());
		packet->setContent(phone_discard_molecule_done);
		packet->setUnsignedAt(0, _ok);

		activ->remove(molecule);

		tcp.send(*packet);
	}

	return _ok;
}

int Sequencer::discardByPriority(Packet* aPacket)
{
	unsigned syncMajor = aPacket->getSyncMajor();

	Activity* activ = findActivity(syncMajor);
	Molecule* molecule;
	int discarded = 0;

	if (!activ) return _invalid;

	unsigned fromPriority = aPacket->getUnsignedAt(0);
	unsigned toPriority = aPacket->getUnsignedAt(1);
	int immediately = aPacket->getUnsignedAt(2);

	log(log_debug, "sequencer") << "discarding molecules with " << fromPriority 
		<< " <= priority <= " << toPriority << logend();

	omni_mutex_lock lock(mutex);

	for (ActivityIter i(*activ); !i.isDone(); i.next())
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
			activ->remove(molecule);
			
			log(log_debug, "sequencer") << "removed molecule (" << syncMajor 
				<< ", " << molecule->getSyncMinor() << ')' << logend();
		}
	}

	if (discarded)
	{
		packet->clear(2);
		packet->setSync(activ->getSyncMajor(), 0);
		packet->setContent(phone_discard_molecule_priority_done);
		packet->setUnsignedAt(0, _ok);
		packet->setUnsignedAt(discarded, _ok);

		tcp.send(*packet);
	}

	return _ok;
}

int Sequencer::switchTo(Packet* aPacket)
{
	unsigned syncMajor = aPacket->getSyncMajor();
	unsigned immediately = aPacket->getUnsignedAt(0);

	omni_mutex_lock lock(mutex);

	Activity* activ = findActivity(syncMajor);

	if (activ)
	{
		activ->unsetDisabled();

		// if active, set  and send ack when stopped, else switch and send ack immediately
		if (activity && activity != activ && activity->isActive())
		{
			activity->setDisabled();
			if (immediately)
			{
				activity->setASAP();

				log(log_debug, "sequencer") << "disable active activity " << syncMajor 
					<< " as soon as possible" << logend();

				nextActivity = activ;

				if (activity->stop() == -1) 
				{
					log(log_debug, "sequencer") << "activity " << syncMajor 
						<< " was not started yet" << logend();
				}
			}
			else
			{
				log(log_debug, "sequencer") << "disable active activity " << syncMajor 
					<< " when finished" << logend();
			}

			checkCompleted();
		}
		else
		{
			log(log_debug, "sequencer") << "switch to activity " << syncMajor 
				<< logend();

			activity = activ;
			activity->start();

			packet->clear(1);
			packet->setSync(syncMajor, 0);
			packet->setContent(phone_switch_activity_done);
 
			tcp.send(*packet);
		}

	} 
	else 
	{
		log(log_error, "sequencer") << "switch to unknown activity " << syncMajor 
			<< logend();

		return _invalid;
	}

	return _ok;
}

void Sequencer::sendAtomDone(unsigned syncMinor, unsigned nAtom, unsigned status, unsigned msecs)
{
	lock();
	packet->clear(3);
	packet->setSync(activity->getSyncMajor(), syncMinor);
	packet->setContent(phone_atom_done);
	packet->setUnsignedAt(0, nAtom);
	packet->setUnsignedAt(1, status);
	packet->setUnsignedAt(2, msecs);
 
	tcp.send(*packet);
	unlock();
}

void Sequencer::sendMoleculeDone(unsigned syncMajor, unsigned syncMinor, unsigned status, unsigned pos, unsigned length)
{
	if (debug > 3) cout << "send molecule done for: " << syncMajor << "." << syncMinor << " status: " << status << " pos: " << pos << " length: " << length << endl;

	lock();
	packet->clear(3);
	packet->setSync(syncMajor, syncMinor);
	packet->setContent(phone_molecule_done);
	packet->setUnsignedAt(0, status);
	packet->setUnsignedAt(1, pos);
	packet->setUnsignedAt(2, length);
 
	tcp.send(*packet);
	unlock();
}

int Sequencer::listen(Packet* aPacket)
{
	log(log_debug, "sequencer") << "listening" << logend();

	phone->listen();

	return _ok;
}

int Sequencer::accept(Packet* aPacket)
{
	// completion and error handling in acceptDone

	phone->accept();

	return _ok;
}

int Sequencer::reject(Packet* aPacket)
{
	// completion and error handling in rejectDone

	phone->reject();

	return _ok;
}

int Sequencer::connect(Packet* aPacket)
{
	SAP remote;
	unsigned timeout;

	if (aPacket->typeAt(0) != Packet::type_string
	||	aPacket->typeAt(1) != Packet::type_string
	||	aPacket->typeAt(2) != Packet::type_string
	||	aPacket->typeAt(3) != Packet::type_unsigned)
	{
		lock();
		packet->clear(1);
		packet->setSync(aPacket->getSyncMajor(), aPacket->getSyncMinor());
		packet->setContent(phone_connect_done);
		packet->setUnsignedAt(0, _invalid);

		tcp.send(*packet);
		unlock();

		return _invalid;
	}

	remote.setAddress(aPacket->getStringAt(0));
	remote.setService(aPacket->getStringAt(1));
	remote.setSelector(aPacket->getStringAt(2));	
	timeout = aPacket->getUnsignedAt(3);

	// todo: check usage
	SAP local;

	phone->connect(local, remote, timeout);

	log(log_debug, "sequencer") << "connecting to: " << remote << " timeout: " 
		<< timeout << logend();

	return _ok;
}

int Sequencer::transfer(Packet* aPacket)
{
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
	if (activity)	activity->stop();

	unlock();

	phone->transfer(remote, timeout);

	return _ok;
}

int Sequencer::disconnect(Packet* aPacket)
{
	log(log_debug, "sequencer") << "disconnecting" << logend();

	if (!phone->isConnected() || aPacket->typeAt(0) != Packet::type_unsigned)
	{
		lock();
		packet->clear(1);
		packet->setSync(aPacket->getSyncMajor(), aPacket->getSyncMinor());
		packet->setContent(phone_disconnect_done);

		packet->setUnsignedAt(0, _invalid);

		tcp.send(*packet);
		unlock();

		return _invalid;
	}

	lock();

#ifdef __RECOGNIZER__
	if (recognizer) delete recognizer;
	recognizer = 0;
#endif

	if (activities.empty()) 
	{
		activity = 0;
	}
	
	unlock();

	Timer::sleep(200);

	phone->disconnect(aPacket->getUnsignedAt(0));

	checkCompleted();

	tcp.disconnect();

	return _ok;
}

int Sequencer::abort(Packet* aPacket)
{
	lock();

#ifdef __RECOGNIZER__
	if (recognizer) delete recognizer;
	recognizer = 0;
#endif
	if (activities.empty()) activity = 0;

	unlock();

	Timer::sleep(200);
	
	log(log_debug, "sequencer") << "aborting..." << logend();

	checkCompleted();

	phone->abort();

	return _ok;
}

int Sequencer::startRecognition(Packet* aPacket)
{
#ifdef __RECOGNIZER__
	if (recognizer)
	{
		int type = aPacket->getUnsignedAt(0);
		int vocabulary = aPacket->getUnsignedAt(1);
		recognizer->setVocabulary(vocabulary);
		type == recognizer_isolated ? recognizer->startIsolated() : recognizer->startContiguous();

		log(log_debug, "sequencer") << "recognition started. mask " << vocabulary << logend();
	}
	else
	{
		log(log_warning, "sequencer") << "tried to start nonexistent recognizer" 
			<< logend();
	}

	return _ok;

#else

	log(log_warning, "sequencer") << "tried to start nonexistent recognizer" 
		<< logend();

	return _failed;

#endif
}

int Sequencer::stopRecognition(Packet* aPacket)
{
#ifdef __RECOGNIZER__
	if (recognizer)
	{
		recognizer->stop();

		log(log_debug, "sequencer") << "recognition stopped." << logend();
	}
	else 
	{
		log(log_warning, "sequencer") << "tried to stop nonexistent recognizer" 
			<< logend();
	}

	return _ok;
#else

	log(log_warning, "sequencer") << "tried to stop nonexistent recognizer" 
		<< logend();

	return _failed;

#endif
}

int Sequencer::connect(ConnectCompletion* complete)
{
	lock();

	if (connectComplete || tcp.getState() != Transport::idle
	 || phone->getState() != Trunk::listening)
	{
		log(log_warning, "sequencer") << "connect failed due to invalid state (phone: " 
			<< (int)phone->getState() << " tcp: " << (int)tcp.getState() << logend();

		unlock();

		return _busy;
	}
	else if (outOfService)
	{
		log(log_warning, "sequencer") << "connect failed: out of service" << logend();

		unlock();

		return _out_of_service;
	}

	connectComplete = complete;

	unlock();

	// tcp connect happens in connectRequestFailed of TelephoneClient

	phone->abort();

	return _ok;
}

#ifdef __RECOGNIZER__

// Protocol of RecognizerClient

void Sequencer::speechStarted(Recognizer* server)
{	
	log(log_debug, "sequencer") << "speech started." << logend();
}

void Sequencer::recognized(Recognizer* server, Recognizer::Result& result)
{
	for (int i = 0; i < result.getWordCount(); i++)
	{
		log(log_debug, "sequencer") << "recognized: " << result[i] << endl;
	}
	log(log_debug, "sequencer") << "probability: " << result.probability() << logend();

	lock();

	packet->clear(result.getWordCount());
	packet->setContent(phone_recognition);

	for (int i = 0; i < result.getWordCount(); ++i) 
		packet->setStringAt(i, result[i]);

	tcp.send(*packet);
	unlock();
}
#endif

void Sequencer::onIncoming(Trunk* server, const SAP& local, const SAP& remote)
{
	int contained; 

	// do we have an exact match?
	clientSpec = configuration->dequeue(local);
	if (clientSpec)
	{
		log(log_debug, "sequencer") << "found client matching: " << local 
			<< " remote: " << clientSpec->client << logend();
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
				log(log_debug, "sequencer") << "found client in global queue, remote: " 
					<< clientSpec->client << logend();
			}
		}
	}
	if (clientSpec)
	{
		lock();
		packet->clear(6);

		packet->setContent(phone_connect_request);
		packet->setUnsignedAt(0, _ok);
		packet->setStringAt(1, remote.getAddress());
		packet->setStringAt(2, configuration->getNumber());  
		packet->setStringAt(3, local.getService());  
		packet->setStringAt(4, local.getSelector());
		packet->setUnsignedAt(5, server->getTimeslot().ts);
 
		tcp.connect(clientSpec->client, 2000, packet);
		unlock();
	}
	else 
	{
		if (!contained)
		{
			log(log_debug, "sequencer") << "no client found. rejecting call." << logend(); 

			phone->reject(0);
		}
		else
		{
			log(log_debug, "sequencer") << "received partial local address: " << local 
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

		log(log_debug, "sequencer") 
			<< "rejecting request because of outstanding outgoing call" << logend();
	}
	else
	{
		onIncoming(server, local, remote);

		if (!connectComplete)
		{
			log(log_debug, "sequencer") << "telephone connect request from: " 
				<< remote << " , " << local	<< " [" << server->getTimeslot() << ']' 
				<< logend();
		}
	}
}

void Sequencer::connectRequestFailed(Trunk* server, int cause)
{
	if (cause == _aborted && connectComplete)
	{
		log(log_debug, "sequencer") << "connecting to " << connectComplete->getClient() 
			<< " for dialout" << logend();

		tcp.connect(connectComplete->getClient(), 10000);
	}
	else
	{
		log(log_debug, "sequencer") << "connect request failed with " << cause 
			<< logend();

		phone->listen();
	}
}

void Sequencer::connectDone(Trunk* server, int result)
{
	log(log_debug, "sequencer") << "telephone connect done: " << result << logend();

	lock();
	packet->clear(2);

	packet->setContent(phone_connect_done);
	packet->setUnsignedAt(0, result);
 
	tcp.disconnect(indefinite, packet);
	unlock();

	if (result == r_no_dialtone)
	{
		outOfService = 1;
	}
}

void Sequencer::transferDone(Trunk *server)
{
	log(log_debug, "sequencer") << "telephone transfer succeeded" << logend();

#ifdef __RECOGNIZER__
	if (recognizer) 
		delete recognizer;
	recognizer = 0;
#endif

	if (activities.empty()) 
		activity = 0;

	checkCompleted();

	lock();
	packet->clear(1);

	packet->setContent(phone_transfer_done);
	packet->setUnsignedAt(0, _ok);
 
	tcp.send(*packet);
	unlock();
}

void Sequencer::transferFailed(Trunk *server, int cause)
{
	log(log_warning, "sequencer") << "telephone transfer failed: " << cause << logend();

	lock();
	packet->clear(1);

#ifdef __RECOGNIZER__
	if (recognizer) recognizer->startContiguous();
	if (activity)	activity->start();
#endif

	packet->setContent(phone_transfer_done);
	packet->setUnsignedAt(0, cause);
 
	tcp.send(*packet);
	unlock();
}

void Sequencer::disconnectRequest(Trunk *server, int cause)
{
	log(log_debug, "sequencer") << "telephone disconnect request" << logend();

	lock();

#ifdef __RECOGNIZER__
	if (recognizer) 
		delete recognizer;

	recognizer = 0;
#endif

	if (activities.empty()) 
		activity = 0;

	checkCompleted();

	packet->clear(0);
	packet->setContent(phone_disconnect);
 
	tcp.disconnect(indefinite, packet);

	unlock();
}

void Sequencer::disconnectDone(Trunk *server, unsigned result)
{
	// result is always _ok

	lock();
	packet->clear(1);
	packet->setSync(0, 0);
	packet->setContent(phone_disconnect_done);
	packet->setUnsignedAt(0, _ok);
	
	tcp.disconnect(indefinite, packet);

	unlock();

	phone->listen();
}

void Sequencer::acceptDone(Trunk *server, unsigned result)
{
	lock();
	packet->clear(1);

	delete clientSpec;
	clientSpec = 0;

	packet->setContent(phone_accept_done);

	if(result == r_ok) 
		packet->setUnsignedAt(0, _ok);
	else 
		packet->setUnsignedAt(0, _failed);

	tcp.send(*packet);
	unlock();

	if (result == r_ok)
	{
		log(log_debug, "sequencer") << "call accepted" << logend();

#ifdef __RECOGNIZER__
		try
		{
			recognizer = new AsyncRecognizer(*this, phone->getSlot(), ~phone->getSlot(), 1);
		}	   
		catch(Exception& e)
		{
			log(log_warning, "sequencer") << "no recognizer due to:" << e << logend();
		}
#endif
	}
	else
	{
		tcp.disconnect();

		log(log_debug, "sequencer") << "call accept failed" << logend();
	}
}

void Sequencer::rejectDone(Trunk *server, unsigned result)
{
	// result is always _ok

	lock();

	if (connectComplete)
	{
		// internal reject. an outgoing call is outstanding

		log(log_debug, "sequencer") << "connecting to " 
			<< connectComplete->getClient() << " for dialout" << logend();

		unlock();

		tcp.connect(connectComplete->getClient(), 10000);
	}
	else
	{
		packet->clear(1);

		delete clientSpec;
		clientSpec = 0;

		packet->setContent(phone_reject_done);

		packet->setUnsignedAt(0, _ok);

		tcp.send(*packet);
		unlock();

		if (tcp.isConnected())
			tcp.disconnect();

		phone->listen();
	}
}

void Sequencer::details(Trunk *server, const SAP& local, const SAP& remote)
{
	log(log_debug, "sequencer") << "telephone details: " << local << " " << remote 
		<< logend();

	onIncoming(server, local, remote);
}

void Sequencer::remoteRinging(Trunk *server)
{
	log(log_debug, "sequencer") << "telephone remote end ringing" << logend();

	lock();
	packet->clear(1);

	packet->setContent(phone_connect_remote_ringing);
	packet->setStringAt(0, "not implemented"); //server->getLocalSAP().getSelector());
 
	tcp.send(*packet);
	unlock();
}

void Sequencer::started(Telephone *server, Sample *aSample)
{
#ifdef __RECOGNIZER__
	if (isOutgoing && recognizer)	
	{
		log(log_debug, "sequencer") << "ec started" << logend();

		recognizer->playbackStarted();
	}
#endif

	Molecule* m = (Molecule*)aSample->getUserData();

	if (m->notifyStart())
	{
		log(log_debug, "sequencer") << "sent atom_started for " 
			<< activity->getSyncMajor() << ", " << m->getSyncMinor() << ", " 
			<< m->currentAtom() << logend();
 
		lock();
		packet->clear(1);
		packet->setSync(activity->getSyncMajor(), m->getSyncMinor());
		packet->setContent(phone_atom_started);
		packet->setUnsignedAt(0, m->currentAtom());
 
		tcp.send(*packet);
		unlock();
	}
}

void Sequencer::completed(Telephone *server, Sample *aSample, unsigned msecs)
{
#ifdef __RECOGNIZER__
	if (aSample->isOutgoing() && recognizer)
	{
		log(log_debug, "sequencer") << "ec stopped" << logend();

		recognizer->playbackStopped();
	}
#endif

	completed(server, (Molecule*)(aSample->getUserData()), msecs, aSample->getStatus());
}

void Sequencer::completed(Telephone* server, Molecule* aMolecule, unsigned msecs, unsigned status)
{
	int done, atEnd, notifyStop, activityStopped, started;
	unsigned pos, length, nAtom, syncMinor;

	omni_mutex_lock lock(mutex);

	if (activity && aMolecule)
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
			log(log_debug, "sequencer") << "sent atom_done for " 
				<< activity->getSyncMajor() << ", " << aMolecule->getSyncMinor() 
				<< ", " << nAtom << endl << *aMolecule << logend();

			sendAtomDone(aMolecule->getSyncMinor(), nAtom, status, msecs);
		}

		if (done)
		{
			if (done) 
			{
				log(log_debug+2, "sequencer") << "removing " << *aMolecule << logend();
			}

			activity->remove(aMolecule);
		}
		else
		{
			log(log_debug+2, "sequencer") << "done " << *aMolecule << logend();
		}
 
		// start the new activity before sending packets to minimize delay

		activityStopped = 0;
		if (activity->isDisabled() || activity->isDiscarded())	
		{
			if (!atEnd && !activity->isASAP())
			{
				started = activity->start();
			}
			else if (activity->isDisabled() && nextActivity)
			{
				// need to switch context. set activity later (we need to unlock it)...

				started = nextActivity->start();
			}
			else activityStopped = 1;
		}
		else 
		{
			started = activity->start();
		}

		if (atEnd)
		{
			sendMoleculeDone(activity->getSyncMajor(), syncMinor, status, pos, length);
		}

		if (activityStopped)
		{
			if (activity->isDiscarded())
			{
				unsigned syncMajor = activity->getSyncMajor();

				// the activity is possibly locked twice when the molecule is synchronously stoppable
				// force the unlock

				activities.remove(activity);
  
				activity = 0;

				log(log_debug, "sequencer") << "discarded activity " << syncMajor 
					<< logend();

				packet->clear(0);
				packet->setSync(syncMajor, 0);
				packet->setContent(phone_discard_activity_done);
				tcp.send(*packet);
			}
			else if (nextActivity && activity->isDisabled())
			{
				// send switch_activity_done only if nextActivity was set

				unsigned syncMajor = activity->getSyncMajor();

				log(log_debug, "sequencer") << "switched to activity " << syncMajor 
					<< logend();

				activity->unsetASAP();

				activity = nextActivity;
				nextActivity = 0;
 
				packet->clear(0);
				packet->setSync(syncMajor, 0);
				packet->setContent(phone_switch_activity_done);
				tcp.send(*packet);
			}
		}
	}
}

void Sequencer::touchtone(Telephone* server, char tt)
{
	char s[2];

	s[0] = tt;
	s[1] = '\0';

	lock();

	packet->clear(1);
	packet->setContent(phone_touchtones);
	packet->setStringAt(0, s);

	tcp.send(*packet);
	unlock();
}

// protocol of Transport Client

void Sequencer::connectRequest(Transport* server, SAP& remote, Packet* initialPacket)
{
	// future extension. We won't listen right now
}

void Sequencer::connectRequestTimeout(Transport* server)
{
	// future extension. We won't listen right now
}

void Sequencer::connectConfirm(Transport* server, Packet* initialReply)
{
	// good. we got through

	if (connectComplete)
	{
		connectComplete->done(_ok);
		delete connectComplete;

		connectComplete = 0;
	}
}

void Sequencer::connectFailed(Transport* server)
{
	if (connectComplete)
	{
		// our client wanted us to dial out, we must let him know
		connectComplete->done(_failed);
		delete connectComplete;

		connectComplete = 0;
	}
	else if (clientSpec)
	{
		// re-queue the listen, the other possibility would be to let the 
		// other side know we did not get through (similar to the connect case)

		log(log_error, "sequencer") << "connect to " << clientSpec->client 
			<< " failed" << logend();

		configuration->enqueue(clientSpec->details, clientSpec->client, clientSpec->tag);
			
		delete clientSpec;
		clientSpec = 0;
	}
	else
	{
		log(log_error, "sequencer") 
			<< "neither client spec nor connect completion - something is _fishy_" 
			<< logend();
	}

	phone->abort();
}

void Sequencer::connectReject(Transport* server, Packet* initialReply)
{
	// nasty. Didn't get through to client

	log(log_error, "sequencer") << "connect to client " << server->getRemoteSAP() 
		<< " rejected" << logend();

	connectFailed(server);
}

void Sequencer::connectTimeout(Transport* server)
{
	// nasty. Didn't get through to client

	log(log_error, "sequencer") << "connect to client " << server->getRemoteSAP() 
		<< " timed out" << logend();

	connectFailed(server);
}

void Sequencer::disconnectRequest(Transport* server, Packet* finalPacket)
{
	// blast it. reset the phone->
	lock();

	if (activities.empty()) activity = 0;

	unlock();

	if (!phone->isIdle())
		phone->abort();

	checkCompleted();

	tcp.disconnectAccept();
}

void Sequencer::abort(Transport* server, Packet* finalPacket)
{
	lock();

	if (activities.empty())
		activity = 0;

	unlock();

	if (!phone->isIdle())
		phone->abort();

	checkCompleted();

	log(log_warning, "sequencer") << "TCP connection aborted." << logend();
	
	phone->listen();
}

void Sequencer::fatal(Transport* server, const char* e)
{
	log(log_error, "sequencer") << "fatal TCP exception: " << e << endl 
		<< "terminating" << logend();

	exit(4);
}

void Sequencer::fatal(Transport* server, Exception& e)
{
	log(log_error, "sequencer") << "fatal Telephone exception: " << e << endl
		<< "terminating" << logend();

	exit(4);
}

void Sequencer::fatal(Telephone* server, const char* e)
{
	log(log_error, "sequencer") << "fatal Telephone exception: " << e << endl
		<< "terminating" << logend();

	exit(5);
}

void Sequencer::fatal(Telephone* server, Exception& e)
{
	log(log_error, "sequencer") << "fatal Telephone exception: " << e << endl
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

void Sequencer::disconnectConfirm(Transport* server, Packet* finalReply)
{
	phone->listen();
}

void Sequencer::disconnectReject(Transport* server, Packet* aPacket)
{
	// will not happen
}

void Sequencer::disconnectTimeout(Transport* server)
{
	// will not happen
}

void Sequencer::data(Transport* server, Packet* aPacket)
{
	// that's the main packet inspection method...

	switch (aPacket->getContent())
	{
	case phone_add_activity:
		addActivity(aPacket);
		break;
	case phone_add_molecule:
		addMolecule(aPacket);
		break;
	case phone_switch_activity:
		switchTo(aPacket);
		break;
	case phone_discard_activity:
		discardActivity(aPacket);
		break;
	case phone_discard_molecule:
		discardMolecule(aPacket);
		break;
	case phone_discard_molecule_priority:
		discardByPriority(aPacket);
		break;
	case phone_listen:
		listen(aPacket);
		break;
	case phone_accept:
		accept(aPacket);
		break;
	case phone_reject:
		reject(aPacket);
		break;
	case phone_connect:
		connect(aPacket);
		break;
	case phone_disconnect:
		disconnect(aPacket);
		break;
	case phone_abort:
		abort(aPacket);
		break;
	case phone_transfer:
		transfer(aPacket);
		break;
	case phone_stop_listening:
		break;
	case phone_start_recognition:
		startRecognition(aPacket);
		break;
	case phone_stop_recognition:
		stopRecognition(aPacket);
		break;
	}
}

void usage()
{
	cerr << "usage: " << endl << "sequence -[dl]" << endl;

	exit(1);
}

int main(int argc, char* argv[])
{
	int c;
	int analog = 0;
	char* switchModule = 0;
	char szKey[256];

	ULONG rc;
	WSADATA wsa;

	// free timeslots are marked true
	SCBus.set_bits(true);

	set_log_instance(&cout_log);
	set_log_level(4);

	rc = WSAStartup(MAKEWORD(2,0), &wsa);

	cout << "sequence starting." << endl;

	/*
	 * commandline parsing
	 */
	while( (c = getopt(argc, argv, "d:l:")) != EOF) {
		switch(c) 
		{
		case 'd':
			debug = atoi(optarg);
			cout << "debug level " << debug << endl;
			break;
		case 'l':
			cout << "logging to: " << optarg << endl;
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
		unsigned index;

		for (index = 0; 1; index++)
		{
			sprintf(szKey, "SOFTWARE\\Immisch, Becker & Partner\\Voice Server\\Aculab PRI\\Trunk%d", index);

			RegistryKey key(HKEY_LOCAL_MACHINE, szKey);
			if (!key.exists())	break;

			TrunkConfiguration* trunk = new AculabPRITrunkConfiguration();

			if (!trunk->readFromKey(key))
			{
				cout << "error reading trunk description: " << szKey << endl;
				return 2;
			}

			gConfiguration.add(trunk);

			trunk->start();
		}

		// start interface here

		if (gConfiguration.numElements() == 0)
		{
			cout << "no trunks configured - nothing to do" << endl;

			return 0;
		}

		AculabTrunk::start();
		AculabPhone::start();
		Sequencer::getTimer().start();

		SAP local;
		
		local.setService(interface_port);

		Interface iface(local);

		iface.run();

		cout << "normal shutdown" << endl;

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

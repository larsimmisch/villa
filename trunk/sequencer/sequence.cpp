/*
	Copyright 1995, 1996 Immisch, Becker & Partner, Hamburg

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

#include "phoneclient.h
#include "acuphone.h"
#include "rphone.h"
#include "activ.h"
#include "sequence.h"
#include "interface.h"
#include "getopt.h"

int debug = 0;
Log* logger;

MVIP gMVIP;

#ifdef __AG__
Conferences gConferences;
#endif

ConfiguredTrunks gConfiguration;
ClientQueue gClientQueue;

AsyncTimer Sequencer::timer;

// I use 'this' in the base member initializer list on purpose
#pragma warning(disable : 4355)

Sequencer::Sequencer(TrunkConfiguration* aConfiguration) 
  : phone(*this, aConfiguration->getTrunk(), aConfiguration->preferredSlot()),
	tcp(*this), activities(), activity(0), nextActivity(0), 
	mutex(), configuration(aConfiguration), connectComplete(0),
	clientSpec(0), outOfService(0)
{
    packet = new(buffer) Packet(0, sizeof(buffer));

	SAP local;

	phone.listen(local);
}

#pragma warning(default : 4355)

int Sequencer::addActivity(Packet* aPacket)
{   
    unsigned syncMajor = aPacket->getSyncMajor();
    if (syncMajor == 0) syncMajor = activities.getSize() +1;

    Activity* activ = new Activity(syncMajor, this);

    // reply success or failures

    Lock lock(mutex);

    packet->clear(1);
    packet->setSync(syncMajor, 0);

    packet->setContent(phone_add_activity_done);

    if (findActivity(syncMajor))
    {
        if (debug)
		{ 
			logger->start() << "add activity " << syncMajor << " duplicate."<< endl;
			logger->end();
		}

        packet->setUnsignedAt(0, _duplicate);
		
	    tcp.send(*packet);

		return _duplicate;
    }

    if (activ)
    {
		if (debug)
		{
			logger->start() << "added activity " << syncMajor << endl;
			logger->end();
		}
            
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
	
	Lock lock(mutex);

    Activity* anActivity = findActivity(syncMajor);

    if (anActivity == 0)
    {
        if (debug)
		{
			logger->start() << "add molecule to unknown activity " << syncMajor << endl;
			logger->end();
		}

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
        aPacket->typeAt(pos+2) != Packet::type_unsigned)    return _invalid;

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
                if (aPacket->typeAt(pos) != Packet::type_string || aPacket->typeAt(pos+1) != Packet::type_unsigned)
                {
                    logger->start() << "invalid packet contents for addMolecule(atom_play_sample)." << endl << *aPacket;
					logger->end();
                    return _invalid;
                }
                pos += 2;

                atom = new PlayAtom(this, aPacket->getStringAt(pos2), aPacket->getUnsignedAt(pos2+1));

                break;
            case atom_record_file_sample:
                // check validity
                if (aPacket->typeAt(pos) != Packet::type_string || aPacket->typeAt(pos+1) != Packet::type_unsigned || aPacket->typeAt(pos+2) != Packet::type_unsigned)
                {
                    logger->start() << "invalid packet contents for addMolecule(atom_record_sample)." << endl << *aPacket;
					logger->end();
                    return _invalid;
                }
                pos += 3;

                atom = new RecordAtom(this, aPacket->getStringAt(pos2), aPacket->getUnsignedAt(pos2+2), aPacket->getUnsignedAt(pos2+1));

                break;
            case atom_beep:
                // check validity
                if (aPacket->typeAt(pos) != Packet::type_unsigned)
                {
                    logger->start() << "invalid packet contents for addMolecule(atom_beep)." << endl;
					logger->end();
                    return _invalid;
                }
                pos += 1;

                atom = new BeepAtom(aPacket->getUnsignedAt(pos2));

                break;
            case atom_conference:
                // check validity
                if (aPacket->typeAt(pos) != Packet::type_unsigned || aPacket->typeAt(pos+1) != Packet::type_unsigned)
                {
                    logger->start() << "invalid packet contents for addMolecule(atom_conference)." << endl;
					logger->end();
                    return _invalid;
                }
                pos += 2;

                atom = new ConferenceAtom(aPacket->getUnsignedAt(pos2), aPacket->getUnsignedAt(pos2+1));

                break;
            case atom_touchtones:
                // check validity
                if (aPacket->typeAt(pos) != Packet::type_string)
                {
                    logger->start() << "invalid packet contents for addMolecule(atom_touchtone)." << endl;
					logger->end();
                    return _invalid;
                }
                pos += 1;

                atom = new TouchtoneAtom(aPacket->getStringAt(pos2));

                break;
            case atom_silence:
                // check validity
                if (aPacket->typeAt(pos) != Packet::type_unsigned)
                {
                    logger->start() << "invalid packet contents for addMolecule(atom_silence)." << endl;
					logger->end();
                    return _invalid;
                }
                pos += 1;

                atom = new SilenceAtom(aPacket->getUnsignedAt(pos2));

                break;
            case atom_energy_detector:
                // check validity
                if (aPacket->typeAt(pos) != Packet::type_unsigned || aPacket->typeAt(pos+1) != Packet::type_unsigned)
                {
                    logger->start() << "invalid packet contents for addMolecule(atom_energy_detector)." << endl << *aPacket;
					logger->end();
                    return _invalid;
                }
                pos += 2;

                atom = new EnergyDetectorAtom(aPacket->getUnsignedAt(pos2), aPacket->getUnsignedAt(pos2+1));

                break;
            }
        }
        catch (char* e)
        {
            logger->start() << "Exception in addMolecule: " << e << endl;
			logger->end();
            atom = 0;
        }
        catch (Exception& e)
        {
            logger->start() << e << endl;
			logger->end();
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
        logger->start() << "Molecule is empty. will not be added." << endl;
        logger->stream << *aPacket;
		logger->end();

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

	Lock lock(mutex);

    Activity* activ = findActivity(syncMajor);

    if (!activ || activ->isDiscarded()) 
    {
		logger->start();
        if (!activ) logger->stream << "discard unknown activity " << syncMajor << endl;
		else 		logger->stream << "discard activity " << syncMajor << " is already discarded" << endl;
		logger->end();
 
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
            if (debug)
			{
				logger->start() << "discard active activity " << syncMajor << " as soon as possible" << endl;
				logger->end();
			}
            if (activ->stop() == -1) 
            {
                if (debug) 
				{
					logger->start() << "activity was not started yet" << endl;
					logger->end();
				}
            }
        }
        else if (debug) 
		{
			logger->start() << "discard active activity " << syncMajor << " when finished" << endl;
			logger->end();
		}

		checkCompleted();
    }
    else
    {
        if (debug)
		{
			logger->start() << "discarded activity " << syncMajor << endl;
			logger->end();
		}

		if (activ == activity)	activity = 0;

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
	Lock lock(mutex);

    Activity* activ = findActivity(aPacket->getSyncMajor());

    if (!activ) return _invalid;

    Molecule* molecule = activ->find(aPacket->getSyncMinor());

    if (!molecule) return _invalid;

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
    Activity* activ = findActivity(aPacket->getSyncMajor());
    Molecule* molecule;
    int discarded = 0;

    if (!activ) return _invalid;

    unsigned fromPriority = aPacket->getUnsignedAt(0);
    unsigned toPriority = aPacket->getUnsignedAt(1);
    int immediately = aPacket->getUnsignedAt(2);

    if (debug > 2)
	{
		logger->start() << "discarding molecules with " << fromPriority << " <= priority <= " << toPriority << endl;
		logger->end();
	}

	Lock lock(mutex);

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
			
			if (debug > 3)
			{
      	    	logger->start() << "stopped molecule" << endl;
				logger->end();
			}

			checkCompleted();
        }
        else
        {
            discarded++;
            activ->remove(molecule);
			
			if (debug > 3)
			{
    	        logger->start() << "removed molecule" << endl;
				logger->end();
			}
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

	Lock lock(mutex);

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
                if (debug)
				{
					logger->start() << "disable active activity " << syncMajor << " as soon as possible" << endl;
					logger->end();
				}
                nextActivity = activ;
                if (activity->stop() == -1) 
                {
                    if (debug)
					{
						logger->start() << "activity was not started yet" << endl;
						logger->end();
					}
                }
            }
            else if (debug)
			{
				logger->start() << "disable active activity " << syncMajor << " when finished" << endl;
				logger->end();
			}

			checkCompleted();
        }
        else
        {
            if (debug)
			{
				logger->start() << "switch to activity " << syncMajor << endl;
				logger->end();
			}

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
		logger->start() << "switch to unknown activity " << syncMajor << endl;
		logger->end();
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
    SAP local;

	local.setAddress(aPacket->getStringAt(0));	
	local.setService(aPacket->getStringAt(1));	
	local.setSelector(aPacket->getStringAt(2));	

	if (debug)
	{
		logger->start() << "listening from: " << local << endl;
		logger->end();
	}

    phone.listen(local);

    return _ok;
}

int Sequencer::accept(Packet* aPacket)
{
	// completion and error handling in acceptDone

	phone.accept();

	return _ok;
}

int Sequencer::reject(Packet* aPacket)
{
	// completion and error handling in rejectDone

	phone.reject();

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

    phone.connect(remote, timeout);

	if (debug)
	{
		logger->start() << "connecting to: " << remote << " timeout: " << timeout << endl;
		logger->end();
	}

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

	if (debug)
	{
		logger->start() << "transferring to: " << remote << endl;
		logger->end();
	}

	lock();

#ifdef __RECOGNIZER__
	if (recognizer) recognizer->stop();
#endif
	if (activity)	activity->stop();

	unlock();

    phone.transfer(remote, timeout);

    return _ok;
}

int Sequencer::disconnect(Packet* aPacket)
{
	if (debug)
	{
		logger->start() << "disconnecting" << endl;
		logger->end();
	}

	if (!phone.isConnected() || aPacket->typeAt(0) != Packet::type_unsigned)
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

    phone.disconnect(aPacket->getUnsignedAt(0));

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
	
	if (debug)
	{
		logger->start() << "aborting..." << endl;
		logger->end();
	}

	checkCompleted();

    phone.abort();

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

		if (debug)
		{
			logger->start() << "recognition started. mask " << vocabulary << endl;
			logger->end();
		}
	}
	else if (debug)	
	{
		logger->start() << "tried to start nonexistent recognizer" << endl;
		logger->end();
	}

	return _ok;

#else

	logger->start() << "tried to start nonexistent recognizer" << endl;
	logger->end();

	return _failed;

#endif
}

int Sequencer::stopRecognition(Packet* aPacket)
{
#ifdef __RECOGNIZER__
	if (recognizer)
	{
		recognizer->stop();

		if (debug)
		{
			logger->start() << "recognition stopped." << endl;
			logger->end();
		}
	}
	else if (debug)	
	{
		logger->start() << "tried to stop nonexistent recognizer" << endl;
		logger->end();
	}

	return _ok;
#else

	logger->start() << "tried to stop nonexistent recognizer" << endl;
	logger->end();

	return _failed;

#endif
}

int Sequencer::connect(ConnectCompletion* complete)
{
	lock();

	if (connectComplete	|| tcp.getState() != Transport::idle
	 || phone.getState() != Telephone::listening)
	{
		if (debug)
		{
			logger->start() << "connect failed due to invalid state (phone: " 
				<< (int)phone.getState() << " tcp: " << (int)tcp.getState() << endl;
			logger->end();
		}

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

	// tcp connect happens in connectRequestFailed of TelephoneClient

	phone.abort();

	return _ok;
}

#ifdef __RECOGNIZER__

// Protocol of RecognizerClient

void Sequencer::speechStarted(Recognizer* server)
{	
	if (debug)
	{
		logger->start() << "speech started." << endl;
		logger->end();
	}
}

void Sequencer::recognized(Recognizer* server, Recognizer::Result& result)
{
	if (debug)
	{
		logger->start() << endl;
		for (int i = 0; i < result.getWordCount(); i++)
		{
			logger->stream << "recognized: " << result[i] << endl;
		}
		logger->stream << "probability: " << result.probability() << endl;
		logger->end();
	}

	lock();

	packet->clear(result.getWordCount());
	packet->setContent(phone_recognition);

	for (int i = 0; i < result.getWordCount(); i++)	packet->setStringAt(i, result[i]);

	tcp.send(*packet);
	unlock();
}
#endif

void Sequencer::onIncoming(Telephone* server, SAP& local, SAP& remote)
{
	int contained; 

	// do we have an exact match?
	clientSpec = configuration->dequeue(local);
	if (clientSpec && debug)
	{
		logger->start() << "found client matching: " << local << " remote: " << clientSpec->client << endl;
		logger->end();
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
			if (clientSpec && debug)
			{
				logger->start() << "found client in global queue, remote: " << clientSpec->client << endl;
				logger->end();
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
		packet->setUnsignedAt(5, server->getTrunkSlot().ts);
 
		tcp.connect(clientSpec->client, 2000, packet);
		unlock();
	}
	else 
	{
		if (!contained)
		{
			if (debug) 
			{
				logger->start() << "no client found. rejecting call." << endl; 
				logger->end();
			}

			phone.reject(0);
		}
		else if (debug)
		{
			logger->start() << "received partial local address: " << local << endl; 
			logger->end();

			phone.details(10000);
		}
	}
}

// Protocol of Telephone Client. Basically forwards it's information to the remote client

void Sequencer::connectRequest(Telephone* server, SAP& remote)
{
	if (connectComplete)
	{
		phone.reject();

		if (debug)
		{
			logger->start() << "rejecting request because of outstanding outgoing call" << endl;
			logger->end();
		}
	}
	else
	{
		onIncoming(server, server->getLocalSAP(), remote);

		if (debug && !connectComplete)
		{
			logger->start() << "telephone connect request from: " 
			<< remote << " , " << server->getLocalSAP() 
			<< " [" << server->getSlot() << ' ' << server->getTrunkSlot() << ']' << endl;
			logger->end();
		}
	}
}

void Sequencer::connectRequestFailed(Telephone* server, int cause)
{
	SAP local;

	if (cause == _aborted && connectComplete)
	{
		if (debug)
		{
			logger->start() << "connecting to " << connectComplete->getClient() << " for dialout" << endl;
			logger->end();
		}

		tcp.connect(connectComplete->getClient(), 10000);
	}
	else
	{
		if (debug)
		{
			logger->start() << "connect request failed with " << cause << endl;
			logger->end();
		}

		phone.listen(local);
	}
}

void Sequencer::connectConfirm(Telephone* server)
{
    lock();
    packet->clear(2);

    packet->setContent(phone_connect_done);
    packet->setUnsignedAt(0, _ok);
    packet->setUnsignedAt(1, server->getTrunkSlot().ts);
 
    tcp.send(*packet);
    unlock();
}

void Sequencer::connectFailed(Telephone* server, int cause)
{
    if (debug) 
	{
		logger->start() << "telephone connect failed: " << cause << endl;
		logger->end();
	}

    lock();
    packet->clear(2);

    packet->setContent(phone_connect_done);
    packet->setUnsignedAt(0, cause);
 
    tcp.disconnect(indefinite, packet);
    unlock();

	if (cause == r_no_dialtone)
	{
		outOfService = 1;
	}
}

void Sequencer::transferDone(Telephone* server)
{
    if (debug) 
	{
		logger->start() << "telephone transfer succeeded" << endl;
		logger->end();
	}

#ifdef __RECOGNIZER__
	if (recognizer) delete recognizer;
	recognizer = 0;
#endif

    if (activities.empty()) activity = 0;

	checkCompleted();

    lock();
    packet->clear(1);

    packet->setContent(phone_transfer_done);
    packet->setUnsignedAt(0, _ok);
 
    tcp.send(*packet);
    unlock();
}

void Sequencer::transferFailed(Telephone* server, int cause)
{
    if (debug) 
	{
		logger->start() << "telephone transfer failed: " << cause << endl;
		logger->end();
	}

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

void Sequencer::disconnectRequest(Telephone* server)
{
    if (debug) 
	{
		logger->start() << "telephone disconnect request" << endl;
		logger->end();
	}

    lock();

#ifdef __RECOGNIZER__
	if (recognizer) delete recognizer;
	recognizer = 0;
#endif

    if (activities.empty()) activity = 0;

	checkCompleted();

    packet->clear(0);
    packet->setContent(phone_disconnect);
 
	tcp.disconnect(indefinite, packet);

    unlock();
}

void Sequencer::disconnectDone(Telephone* server, unsigned result)
{
	// result is always _ok

    lock();
    packet->clear(1);
    packet->setSync(0, 0);
    packet->setContent(phone_disconnect_done);
	packet->setUnsignedAt(0, _ok);
	
	tcp.disconnect(indefinite, packet);

    unlock();

	SAP local;

	phone.listen(local);
}

void Sequencer::acceptDone(Telephone* server, unsigned result)
{
    lock();
    packet->clear(1);

	delete clientSpec;
	clientSpec = 0;

    packet->setContent(phone_accept_done);

    if(result == r_ok) packet->setUnsignedAt(0, _ok);
    else packet->setUnsignedAt(0, _failed);

    tcp.send(*packet);
    unlock();

	if (result == r_ok)
	{
		if (debug)
		{
			logger->start() << "call accepted" << endl;
			logger->end();
		}

#ifdef __RECOGNIZER__
		try
		{
			recognizer = new AsyncRecognizer(*this, phone.getSlot(), ~phone.getSlot(), 1);
		}      
		catch(Exception& e)
		{
			logger->start();
			if (debug) logger->stream << "no recognizer due to:" << e << endl;
			else logger->stream << "no recognizer" << endl;
			logger->end();
		}
#endif
	}
	else
	{
		tcp.disconnect();

		if (debug)
		{
			logger->start() << "call accept failed" << endl;
			logger->end();
		}
	}
}

void Sequencer::rejectDone(Telephone* server, unsigned result)
{
	// result is always _ok

    lock();

	if (connectComplete)
	{
		// internal reject. an outgoing call is outstanding

		if (debug)
		{
			logger->start() << "connecting to " << connectComplete->getClient() << " for dialout" << endl;
			logger->end();
		}

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

		if (tcp.isConnected()) tcp.disconnect();

		SAP local;

		phone.listen(local);
	}
}

void Sequencer::details(Telephone* server, SAP& local, SAP& remote)
{
    if (debug) 
	{
		logger->start() << "telephone details: " << local << " " << remote << endl;
		logger->end();
	}

	onIncoming(server, local, remote);
}

void Sequencer::remoteRinging(Telephone* server)
{
    if (debug) 
	{
		logger->start() << "telephone remote end ringing" << endl;
		logger->end();
	}

    lock();
    packet->clear(1);

    packet->setContent(phone_connect_remote_ringing);
    packet->setStringAt(0, server->getLocalSAP().getSelector());
 
    tcp.send(*packet);
    unlock();
}

void Sequencer::started(Telephone* server, Telephone::Sample* aSample)
{
#ifdef __RECOGNIZER__
	if (isOutgoing && recognizer)	
	{
		if (debug > 2)
		{
			logger->start() << "ec started" << endl;
			logger->end();
		}
		recognizer->playbackStarted();
	}
#endif

    Molecule* m = (Molecule*)aSample->getUserData();

    if (m->notifyStart())
    {
        if (debug > 2)
		{
			logger->start() << "sent atom_started for " << activity->getSyncMajor() << ", " << m->getSyncMinor() << ", " << m->currentAtom() << endl;
			logger->end();
		}
 
        lock();
        packet->clear(1);
        packet->setSync(activity->getSyncMajor(), m->getSyncMinor());
        packet->setContent(phone_atom_started);
        packet->setUnsignedAt(0, m->currentAtom());
 
        tcp.send(*packet);
        unlock();
    }
}

void Sequencer::completed(Telephone* server, Telephone::Sample* aSample, unsigned msecs)
{
#ifdef __RECOGNIZER__
	if (aSample->isOutgoing() && recognizer)
	{
		if (debug > 2)	
		{
			logger->start() << "ec stopped" << endl;
			logger->end();
		}
		recognizer->playbackStopped();
	}
#endif

    completed(server, (Molecule*)(aSample->getUserData()), msecs, aSample->getStatus());
}

void Sequencer::completed(Telephone* server, Molecule* aMolecule, unsigned msecs, unsigned status)
{
    int done, atEnd, notifyStop, activityStopped, started;
    unsigned pos, length, nAtom, syncMinor;

	Lock lock(mutex);

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
            if (debug > 2)
			{
				logger->start() << "sent atom_done for " << activity->getSyncMajor() << ", " << aMolecule->getSyncMinor() << ", " << nAtom << endl << *aMolecule;
				logger->end();
			}

            sendAtomDone(aMolecule->getSyncMinor(), nAtom, status, msecs);
        }

        if (done)
        {
            if (done && debug > 3) 
			{
				logger->start() << "removing " << *aMolecule;
				logger->end();
			}

            activity->remove(aMolecule);
        }
        else if (debug > 3)
		{
			logger->start() << "done " << *aMolecule;
			logger->end();
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

                if (debug)
				{
					logger->start() << "discarded activity " << syncMajor << endl;
					logger->end();
				}

                packet->clear(0);
                packet->setSync(syncMajor, 0);
                packet->setContent(phone_discard_activity_done);
                tcp.send(*packet);
            }
            else if (nextActivity && activity->isDisabled())
            {
                // send switch_activity_done only if nextActivity was set

                unsigned syncMajor = activity->getSyncMajor();

                if (debug)
				{
					logger->start() << "switched to activity " << syncMajor << endl;
					logger->end();
				}

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

void Sequencer::dataReady(Telephone* server)
{ 
    char tt[32]; 
    phone.receive(tt); 

    lock();

    packet->clear(1);
    packet->setContent(phone_touchtones);
    packet->setStringAt(0, tt);

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

	    logger->start() << "connect to " << clientSpec->client << " failed" << endl;
		logger->end();

		configuration->enqueue(clientSpec->details, clientSpec->client, clientSpec->tag);
			
		delete clientSpec;
		clientSpec = 0;
	}
	else
	{
	    logger->start() << "have neither client spec nor connect completion" 
				<< endl << "something is _fishy_" << endl;
		logger->end();
	}

	phone.abort();
}

void Sequencer::connectReject(Transport* server, Packet* initialReply)
{
    // nasty. Didn't get through to client

    logger->start() << "connect to client " << server->getRemoteSAP() << " rejected" << endl;
	logger->end();

	connectFailed(server);
}

void Sequencer::connectTimeout(Transport* server)
{
    // nasty. Didn't get through to client

    logger->start() << "connect to client " << server->getRemoteSAP() << " timed out" << endl;
	logger->end();

	connectFailed(server);
}

void Sequencer::disconnectRequest(Transport* server, Packet* finalPacket)
{
    // blast it. reset the phone.
	lock();

    if (activities.empty()) activity = 0;

	unlock();

    if (!phone.isIdle())    phone.abort();

	checkCompleted();

    tcp.disconnectAccept();
}

void Sequencer::abort(Transport* server, Packet* finalPacket)
{
    SAP local;

	lock();

    if (activities.empty())	activity = 0;

	unlock();

    if (!phone.isIdle())    phone.abort();

	checkCompleted();

    logger->start() << "TCP connection aborted." << endl;
	logger->end();
	
    phone.listen(local);
}

void Sequencer::fatal(Transport* server, const char* e)
{
    logger->start() << "fatal TCP exception: " << e << endl;
	logger->stream << "terminating" << endl;
	logger->end();

	exit(4);
}

void Sequencer::fatal(Transport* server, Exception& e)
{
    logger->start() << "fatal Telephone exception: " << e << endl;
	logger->stream << "terminating" << endl;
	logger->end();

	exit(4);
}

void Sequencer::fatal(Telephone* server, const char* e)
{
    logger->start() << "fatal Telephone exception: " << e << endl;
	logger->stream << "terminating" << endl;
	logger->end();

	exit(5);
}

void Sequencer::fatal(Telephone* server, Exception& e)
{
    logger->start() << "fatal Telephone exception: " << e << endl;
	logger->stream << "terminating" << endl;
	logger->end();

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
    SAP local;

	phone.listen(local);
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

	logger = 0;

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
			logger = new Log(optarg);
			break;
        case '?':
            usage();
        default:
            usage();
        }
    }

	if (!logger) logger = new Log(2);

    try
    {
		ConferenceSwitch* mixing = 0;

		sprintf(szKey, "SOFTWARE\\Immisch, Becker & Partner\\Voice Server\\Amtelco XDS\\Switch0");

		{
			RegistryKey key(HKEY_LOCAL_MACHINE, szKey);
			if (key.exists())
			{
				try 
				{
					mixing = new XDSModule(key.getStringAt("Name"), 1, key.getUnsignedAt("Device"));

					if (mixing)	Conferences::setMixing(mixing);
				}
				catch(...)
				{
					delete mixing;

					mixing = 0;
				}

				if (mixing)
				{
					logger->start() << "Amtelco XDS Conference Switch loaded"
						<< mixing->availableConferences() << " conferences" << endl;
					logger->end();
				}
				else
				{
					logger->start() << "Amtelco XDS Conference Switch failed to load" << endl;
					logger->end();
				}
			}
		}

		if (mixing == 0)
		{
			sprintf(szKey, "SOFTWARE\\Immisch, Becker & Partner\\Voice Server\\NMS Conference\\Switch0");

			RegistryKey key(HKEY_LOCAL_MACHINE, szKey);
			if (key.exists())
			{
				try 
				{
					mixing = new AGSwitchModule(key.getStringAt("Name"), 1, key.getUnsignedAt("Device"));

					if (mixing)	Conferences::setMixing(mixing);
				}
				catch(...)
				{
					delete mixing;

					mixing = 0;
				}

				if (mixing)
				{
					logger->start() << "NMS Conference Switch loaded "
						<< mixing->availableConferences() << " conferences" << endl;
					logger->end();
				}
				else
				{
					logger->start() << "NMS Conference Switch failed to load" << endl;
					logger->end();
				}
			}
		}

		if (mixing == 0)
		{
			sprintf(szKey, "SOFTWARE\\Immisch, Becker & Partner\\Voice Server\\Aculab DConf\\Switch0");

			RegistryKey key(HKEY_LOCAL_MACHINE, szKey);
			if (key.exists())
			{
				try 
				{
					mixing = new AculabSwitchModule(key.getStringAt("Name"), 1, key.getUnsignedAt("Device"));

					if (mixing)	Conferences::setMixing(mixing);
				}
				catch(...)
				{
					delete mixing;

					mixing = 0;
				}

				if (mixing)
				{
					logger->start() << "Aculab DConf loaded with " 
						<< mixing->availableConferences() << " conferences" << endl;
					logger->end();
				}
				else
				{
					logger->start() << "Aculab DConf failed to load" << endl;
					logger->end();
				}
			}
		}

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

		{
			// load ag parameter file
			strcpy(szKey, "SOFTWARE\\Immisch, Becker & Partner\\Voice Server\\NMS WTI-8");
			RegistryKey key(HKEY_LOCAL_MACHINE, szKey);
			if (key.exists())
			{
				char* parameterFile = key.getStringAt("Parameters");

				if (parameterFile)
					NMSPhone::getConfiguration().loadParameters(parameterFile);
			}
		}

		for (index = 0; 1; index++)
		{
			sprintf(szKey, "SOFTWARE\\Immisch, Becker & Partner\\Voice Server\\NMS WTI-8\\Trunk%d", index);

			RegistryKey key(HKEY_LOCAL_MACHINE, szKey);
			if (!key.exists())	break;

			TrunkConfiguration* trunk = new NmsAnalogTrunkConfiguration();

			if (!trunk->readFromKey(key))
			{
				cout << "error reading trunk description: " << szKey << endl;
				return 2;
			}

			gConfiguration.add(trunk);

			trunk->start();

			Sleep(200);
		} 

		NMSPhone::getConfiguration().reportResources(gMVIP);
		Conferences::setMVIP(gMVIP);

		// start interface here

		if (gConfiguration.numElements() == 0)
		{
			cout << "no trunks configured - nothing to do" << endl;

			return 0;
		}

		SAP local;
		
		local.setService(interface_port);

		Interface iface(local);

		iface.run();

		cout << "normal shutdown" << endl;

    }
    catch (const char* e)
    {
        logger->start() << "exception: " << e << ". terminating." << endl;
		logger->end();

        return 2;
    }
    catch (Exception& e)
    {
        logger->start() << "exception: " << e << ". terminating." << endl;
		logger->end();

        return 2;
    }
    catch(...)
    {
        logger->start() << "unknown exception. terminating." << endl;
		logger->end();

        return 2;
    }

    return 0;
}

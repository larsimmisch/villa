/*
	Copyright 1995 Immisch, Becker & Partner, Hamburg

	created: Tue Jul 25 16:05:02 GMT+0100 1995

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _SEQUENCE_H_
#define _SEQUENCE_H_

#include <fstream.h>
#include "phone.h"
#include "phoneclient.h"
// #include <TCP/AsyncTCP.h>
// #include <TCP/Client.h>
#include "activ.h"
#include "queue.h"
// #include "Configuration.h"

class Sequencer : public TelephoneClient, public TransportClient
#ifdef __RECOGNIZER__
, public RecognizerClient
#endif
{
public:

	enum { indefinite = -1 };

	Sequencer(TrunkConfiguration* configuration);
	virtual ~Sequencer() {}

	// external protocol - called by remote client by sending appropriate packets

	int addActivity(Packet* aPacket);
	int addMolecule(Packet* aPacket);

	int discardActivity(Packet* aPacket);
	int discardMolecule(Packet* aPacket);
	int discardByPriority(Packet* aPacket);
 
	int startActivity(Packet* aPacket);
	int stopActivity(Packet* aPacket);
 
	int switchTo(Packet* aPacket);

	int startRecognition(Packet* aPacket);
	int stopRecognition(Packet* aPacket);

	int listen(Packet* aPacket);
	int connect(Packet* aPacket);
	int transfer(Packet* aPacket);
	int disconnect(Packet* aPacket);
	int abort(Packet* aPacket);

	int accept(Packet* aPacket);
	int reject(Packet* aPacket);

	int stopListening(Packet* aPacket);
	int stopConnecting(Packet* aPacket);
	int stopTransferring(Packet* aPacket);

	int connect(ConnectCompletion* complete);

	void lock()   { mutex.lock(); }
	void unlock() { mutex.unlock(); }

	// helpers for sending packets
	void sendAtomDone(unsigned syncMinor, unsigned nAtom, unsigned status, unsigned msecs);
	void sendMoleculeDone(unsigned syncMajor, unsigned syncMinor, unsigned status, unsigned pos, unsigned msecs);

#ifdef __RECOGNIZER__
	// protocol of RecognizerClient
	virtual void speechStarted(Recognizer* server);
	virtual void recognized(Recognizer* server, Recognizer::Result& result);
#endif

	// TelephoneClients connectRequest & details are treated here
	void onIncoming(Telephone* server, SAP& local, SAP& remote); 

	// protocol of TelephoneClient
	// must call server.accept or server.reject
	virtual void connectRequest(Telephone* server, SAP& remote);
	virtual void connectRequestFailed(Telephone* server, int cause);
	
	// replies to call from far end
	virtual void connectConfirm(Telephone* server);
	virtual void connectFailed(Telephone* server, int cause);
	
	// must call server.disconnectAccept
	virtual void disconnectRequest(Telephone* server);
	
	// disconnect completion
	virtual void disconnectDone(Telephone* server, unsigned result);

	// accept completion
	virtual void acceptDone(Telephone* server, unsigned result);

	// reject completion
	virtual void rejectDone(Telephone* server, unsigned result);

	virtual void details(Telephone* server, SAP& local, SAP& remote);
	virtual void remoteRinging(Telephone* server);
	
	// flow control
	virtual void stopSending(Telephone* server) {}
	virtual void resumeSending(Telephone* server) {}

	// sent whenever a packet is delivered
	virtual void dataReady(Telephone* server); 

	// sent whenever a sample is started
	virtual void started(Telephone* server, Sample* aSample);

	// sent whenever a Sample is sent
	virtual void completed(Telephone* server, Sample* aSample, unsigned msecs);
	virtual void completed(Telephone* server, Molecule* aMolecule, unsigned msecs, unsigned reason);
	
	virtual void aborted(Telephone* sender, int cause) {}

	virtual void fatal(Telephone* server, const char* e);
	virtual void fatal(Telephone* server, Exception& e);

	// protocol of TransportClient
	virtual void connectRequest(Transport* server, SAP& remote, Packet* initialPacket = 0);
	virtual void connectRequestTimeout(Transport* server);
	
	// replies to server.connect from far end
	virtual void connectConfirm(Transport* server, Packet* initialReply = 0);
	virtual void connectReject(Transport* server, Packet* initialReply = 0);
	virtual void connectTimeout(Transport* server);
	
	// helper for connectTimeout & connectReject
	void connectFailed(Transport* server);
	
	// results from transfer
	virtual void transferDone(Telephone* server);
	virtual void transferFailed(Telephone* server, int cause);

	// must call server.disconnectAccept or server.disconnectReject 
	virtual void disconnectRequest(Transport* server, Packet* finalPacket = 0);
	
	// replies to server.disconnect from far end
	virtual void disconnectConfirm(Transport* server, Packet* finalReply = 0);
	virtual void disconnectReject(Transport* server, Packet* aPacket = 0);
	virtual void disconnectTimeout(Transport* server);
	
	// sent whenever packet is received
	virtual void dataReady(Transport* server) {}
 
	// we want to get new data with the data() call
	virtual int asynchronous(Transport* server)  { return 0; }
	virtual void data(Transport* server, Packet* aPacket);

	// flow control
	virtual void stopSending(Transport* server) {}
	virtual void resumeSending(Transport* server) {}

	// miscellaneous
	virtual void abort(Transport* server, Packet* lastPacket);
	virtual void fatal(Transport* server, const char* e);
	virtual void fatal(Transport* server, Exception& e);

	void addCompleted(Telephone* server, Molecule* molecule, unsigned msecs, unsigned reason);
	void checkCompleted();

	virtual void waitForCompletion()	{ done.wait(); }

	void loadSwitch(const char* aName, int is32bit = 1, int aDevice = 0)	{ phone.loadSwitch(aName, aDevice); }

	AsyncTimer& getTimer()				{ return timer; }
	Telephone*	getPhone()				{ return &phone; }

protected:

	int addMolecule(Packet* aPacket, Activity& anActivity, unsigned* posInPacket);
 
	Activity* findActivity(unsigned anActivity) { return activities.find(anActivity); }
	Packet* newPacket(unsigned args, unsigned size = 1024);

	TrunkConfiguration* configuration;
	NMSAsyncPhone phone;
	AsyncTCP tcp;
#ifdef __RECOGNIZER__
	AsyncRecognizer* recognizer;
#endif
	ClientQueue::Item* clientSpec;
	Event done;
	Activity* activity;
	Activity* nextActivity;
	ActivityCollection activities;
	CompletedQueue delayedCompletions;
	Packet* packet;
	char buffer[1024];
	omni_mutex mutex;
	ConnectCompletion* connectComplete;
	int outOfService;

	static AsyncTimer timer;
};

#endif

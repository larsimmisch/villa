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
#include "asynctcp.h"
#include "client.h"
#include "activ.h"
#include "queue.h"
#include "configuration.h"

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

	int addMolecule(Packet* aPacket);

	int discardMolecule(Packet* aPacket);
	int discardByPriority(Packet* aPacket);
 
	int startActivity(Packet* aPacket);
	int stopActivity(Packet* aPacket);
 
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
	void sendMoleculeDone(unsigned syncMinor, unsigned status, unsigned pos, unsigned msecs);

#ifdef __RECOGNIZER__
	// protocol of RecognizerClient
	virtual void speechStarted(Recognizer* server);
	virtual void recognized(Recognizer* server, Recognizer::Result& result);
#endif

	// TrunkClients connectRequest & details are treated here
	void onIncoming(Trunk *server, const SAP &local, const SAP &remote); 

	// TrunkClient protocol

	// must call server.accept or server.reject
	virtual void connectRequest(Trunk *server, const SAP &local, const SAP &remote);
	virtual void connectRequestFailed(Trunk *server, int cause);
	
	// replies to call from far end
	virtual void connectDone(Trunk *server, int result);
	
	// must call server.disconnectAccept
	virtual void disconnectRequest(Trunk *server, int cause);
	
	// disconnect completion
	virtual void disconnectDone(Trunk *server, unsigned result);

	// accept completion
	virtual void acceptDone(Trunk *server, unsigned result);

	// reject completion
	virtual void rejectDone(Trunk *server, unsigned result);

	virtual void details(Trunk *server, const SAP& local, const SAP& remote);
	virtual void remoteRinging(Trunk *server);

	// results from transfer
	virtual void transferDone(Trunk *server);
	virtual void transferFailed(Trunk *server, int cause);

	// TelephoneClient protocol

	// unused
	virtual void disconnected(Telephone *server) {};
	virtual void connected(Telephone *server) {};

	// sent whenever a touchtone is received
	virtual void touchtone(Telephone *server, char tt);

	// sent whenever a sample is started
	virtual void started(Telephone *server, Sample *aSample);

	// sent whenever a Sample is sent
	virtual void completed(Telephone *server, Sample *aSample, unsigned msecs);
	virtual void completed(Telephone *server, Molecule *aMolecule, unsigned msecs, unsigned reason);
	
	virtual void aborted(Telephone *sender, int cause) {}

	virtual void fatal(Telephone *server, const char *e);
	virtual void fatal(Telephone *server, Exception& e);

	// protocol of TransportClient
	virtual void connectRequest(Transport* server, SAP& remote, Packet* initialPacket = 0);
	virtual void connectRequestTimeout(Transport* server);
	
	// replies to server.connect from far end
	virtual void connectConfirm(Transport* server, Packet* initialReply = 0);
	virtual void connectReject(Transport* server, Packet* initialReply = 0);
	virtual void connectTimeout(Transport* server);
	
	// helper for connectTimeout & connectReject
	void connectFailed(Transport* server);
	
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

	// todo: needed?
	// virtual void waitForCompletion()	{ done.wait(); }

	// void loadSwitch(const char* aName, int is32bit = 1, int aDevice = 0)	{ phone.loadSwitch(aName, aDevice); }

	static Timer& getTimer()				{ return timer; }
	Telephone*	getPhone()				{ return phone; }

protected:

	int addMolecule(Packet* aPacket, unsigned* posInPacket);
 
	Packet* newPacket(unsigned args, unsigned size = 1024);

	TrunkConfiguration *configuration;
	AsyncTCP tcp;
	Telephone *phone;
#ifdef __RECOGNIZER__
	AsyncRecognizer *recognizer;
#endif
	ClientQueue::Item *clientSpec;
	Activity activity;
	CompletedQueue delayedCompletions;
	Packet *packet;
	char buffer[1024];
	omni_mutex mutex;
	ConnectCompletion *connectComplete;
	int outOfService;

	static Timer timer;
};

#endif

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
#include "text.h"
#include "client.h"
#include "activ.h"
#include "queue.h"
#include "configuration.h"

#define MAXCHANNELS 2
#define SEQUENCER_PORT 2104

class Sequencer : public TrunkClient, public MediaClient
{
public:

	enum { indefinite = -1 };

	// Sequencer with trunk
	Sequencer(TrunkConfiguration* configuration);
	// background server
	Sequencer(InterfaceConnection *server);
	virtual ~Sequencer() {}

	/* Add molecule */
	int MLCA(InterfaceConnection *server, const std::string &id);
	/* Delete molecule */
	int MLCD(InterfaceConnection *server, const std::string &id);
	/* delete molecules by priority */
	int MLDP(InterfaceConnection *server, const std::string &id);

	/* accept */
	int ACPT(InterfaceConnection *server, const std::string &id);
	/* transfer */
	int TRSF(InterfaceConnection *server, const std::string &id);
	/* disconnect */
	int DISC(InterfaceConnection *server, const std::string &id);
	/* close background channel */
	int BGRC(const std::string &id);

	int disconnect(int cause);

	int reject(InterfaceConnection *server, const std::string &id);

	int stopListening(InterfaceConnection *server, const std::string &id);
	int stopConnecting(InterfaceConnection *server, const std::string &id);
	int stopTransferring(InterfaceConnection *server, const std::string &id);

	void lock()   { m_mutex.lock(); }
	void unlock() { m_mutex.unlock(); }

	// helpers for sending packets
	void sendAtomDone(const char *id, unsigned nAtom, unsigned status, unsigned msecs);
	void sendMoleculeDone(const char *id, unsigned status, unsigned pos, unsigned msecs);

	int connect(ConnectCompletion* complete);

	// TrunkClients connectRequest & details are treated here
	void onIncoming(Trunk *server, unsigned callref, const SAP &local, const SAP &remote); 

	// TrunkClient protocol

	virtual void idleDone(Trunk *server, unsigned callref) {}

	// must call server.accept or server.reject
	virtual void connectRequest(Trunk *server, unsigned callref, 
		const SAP &local, const SAP &remote);
	
	// replies to call from far end
	virtual void connectDone(Trunk *server, unsigned callref, int result);
	
	// must call server.disconnectAccept
	virtual void disconnectRequest(Trunk *server, unsigned callref, int cause);
	
	// disconnect completion
	virtual void disconnectDone(Trunk *server, unsigned callref, int result);

	// accept completion
	virtual void acceptDone(Trunk *server, unsigned callref, int result);

	virtual void details(Trunk *server, unsigned callref, const SAP& local, const SAP& remote);
	virtual void remoteRinging(Trunk *server, unsigned callref);

	// results from transfer
	virtual void transferDone(Trunk *server, unsigned callref, int result);

	// MediaClient protocol

	// sent whenever a touchtone is received
	virtual void touchtone(Media *server, char tt);

	// sent whenever a sample is started
	virtual void started(Media *server, Sample *aSample);

	// sent whenever a Sample is sent
	virtual void completed(Media *server, Sample *aSample, 
		unsigned msecs);

	virtual void completed(Media *server, Molecule *molecule, 
		unsigned msecs, unsigned reason);
	
	virtual void aborted(Media *sender, int cause) {}

	virtual void fatal(Media *server, const char *e);
	virtual void fatal(Media *server, Exception& e);

	bool data(InterfaceConnection* server, const std::string &data, const std::string &id);

	void addCompleted(Media* server, Molecule* molecule, unsigned msecs, unsigned reason);
	void checkCompleted();

	bool channelsIdle();

	// called when connection to client is lost
	void lost_connection();

	// todo: needed?
	// virtual void waitForCompletion()	{ done.wait(); }

	// void loadSwitch(const char* aName, int is32bit = 1, int aDevice = 0)	{ phone.loadSwitch(aName, aDevice); }

	static Timer& getTimer()		{ return timer; }
	AculabMedia* getMedia()			{ return m_media; }

	const char *getName() { return m_trunk ? m_trunk->getName() : m_media->getName(); }

	Timeslot m_receive;
	Timeslot m_transmit;

protected:

	void release();

	TrunkConfiguration *m_configuration;
	Trunk *m_trunk;
	unsigned m_callref;
	AculabMedia *m_media;
	ClientQueue::Item *m_clientSpec;
	Activity m_activity[MAXCHANNELS];
	CompletedQueue m_delayedCompletions;
	omni_mutex m_mutex;
	ConnectCompletion *m_connectComplete;
	unsigned m_disconnecting;
	bool m_closing;
	SAP m_local;
	SAP m_remote;
	InterfaceConnection *m_interface;

	// the transaction id for trunk operations
	// at any given time, only one may be active
	std::string m_id;

	static Timer timer;
};

#endif

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
	unsigned MLCA(InterfaceConnection *server, const std::string &id);
	/* Delete molecule */
	unsigned MLCD(InterfaceConnection *server, const std::string &id);
	/* delete molecules by priority */
	unsigned MLDP(InterfaceConnection *server, const std::string &id);

	/* accept */
	unsigned ACPT(InterfaceConnection *server, const std::string &id);
	/* transfer */
	unsigned TRSF(InterfaceConnection *server, const std::string &id);
	/* disconnect */
	unsigned DISC(InterfaceConnection *server, const std::string &id);
	/* close background channel */
	unsigned BGRC(const std::string &id);

	unsigned disconnect(int cause);
	bool close(const char *id = 0);

	unsigned reject(InterfaceConnection *server, const std::string &id);

	unsigned stopListening(InterfaceConnection *server, const std::string &id);
	unsigned stopConnecting(InterfaceConnection *server, const std::string &id);
	unsigned stopTransferring(InterfaceConnection *server, const std::string &id);

	void lock()   { m_mutex.lock(); }
	void unlock() { m_mutex.unlock(); }

	// helpers for sending packets
	unsigned sendATOM(const std::string &id, unsigned nAtom, unsigned status, unsigned msecs);
	unsigned sendMLCA(const std::string &id, unsigned status, unsigned pos, unsigned msecs);
	void sendRDIS();

	unsigned connect(ConnectCompletion* complete);

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

	static Timer& getTimer()		{ return timer; }
	AculabMedia* getMedia()			{ return m_media; }

	const char *getName();

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
	unsigned m_disconnecting; 	/* contains the call reference of the DISC request */
	unsigned m_sent_rdis; /* contains call reference for RDIS sent */
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

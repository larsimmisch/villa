/*
	acuphone.h

	$Id: acutrunk.h,v 1.1 2000/10/02 15:52:14 lars Exp $

	Copyright 2000 ibp (uk) Ltd.

	Author: Lars Immisch <lars@ibp.de>
*/

#pragma warning (disable: 4786)

#include <map>
#include "omnithread.h"
#include "phone.h"
#include "phoneclient.h"
#include "mvcldrvr.h"

class AculabTrunk;

class CallEventDispatcher : public omni_thread
{
public:

	CallEventDispatcher() { start_undetached(); }
	~CallEventDispatcher()	{}

	void lock()		{ mutex.lock(); }
	void unlock()	{ mutex.unlock(); }

	void add(ACU_INT handle, AculabTrunk* trunk);
	void remove(ACU_INT handle);

    virtual void* run_undetached(void* arg);

protected:

	std::map<ACU_INT, AculabTrunk*> handle_map;
	omni_mutex mutex;
};

class AculabTrunk : public Trunk
{
public:

	AculabTrunk(int aPort, TrunkClient* aClient, Telephone* aTelephone = 0) 
		: Trunk(aClient, aTelephone), handle(-1), port(aPort), stopped(false) {}
    virtual ~AculabTrunk() {}

	// Connection establishment 
	virtual int listen();
	virtual int connect(SAP& aLocalSAP, SAP& aRemoteSAP, unsigned aTimeout = indefinite);
	
	// must be called by client after a t_connect_request
	virtual int accept();
	virtual int reject(int cause = 0);
	
	// Dissolve a connection
	virtual int disconnect(int cause = 0);
	
	// must be called by client after a disconnectRequest
	virtual int disconnectAccept();
	
	// forces the state to idle
    virtual void abort();
	
    virtual bool hasDetails()		{ return true; }
    virtual bool needDSPSupport()	{ return false; }

	void lock() { mutex.lock(); }
	void unlock() { mutex.unlock(); }

protected:

	static const char* eventName(ACU_LONG event);
	static const char* stateName(states state);

	void release();

	int getCause();

	void onCallEvent(ACU_LONG event);

	void onIdle();
	void onWaitForIncoming();
	void onIncomingCallDetected();
	void onCallConnected();
	void onOutgoingRinging();
	void onRemoteDisconnect();
	void onWaitForAccept();
	void onWaitForOutgoing();
	void onOutgoingProceeding();
	void onDetails();
	void onCallCharge();
	void onChargeInt();

	friend class CallEventDispatcher;

	omni_mutex mutex;
	ACU_INT handle;
	int port;
	bool stopped;

	static CallEventDispatcher dispatcher;
};
/*
	acutrunk.h

	$Id: acutrunk.h,v 1.2 2000/10/18 16:58:43 lars Exp $

	Copyright 2000 ibp (uk) Ltd.

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _ACUTRUNK_H_
#define _ACUTRUNK_H_

#pragma warning (disable: 4786)

#include <vector>
#include <map>
#include "omnithread.h"
#include "phone.h"
#include "phoneclient.h"
#include "mvcldrvr.h"

class AculabTrunk;

class CallEventDispatcher : public omni_thread
{
public:

	CallEventDispatcher() {}
	~CallEventDispatcher()	{}

	void lock()		{ mutex.lock(); }
	void unlock()	{ mutex.unlock(); }

	void add(ACU_INT handle, AculabTrunk* trunk);
	void remove(ACU_INT handle);

	void start() {  start_undetached(); }

protected:

    virtual void* run_undetached(void* arg);

	std::map<ACU_INT, AculabTrunk*> handle_map;
	omni_mutex mutex;
};

class AculabTrunk : public Trunk
{
public:

	AculabTrunk(TrunkClient* aClient, int aPort, Telephone* aTelephone = 0) 
		: Trunk(aClient, aTelephone), handle(-1), port(aPort), stopped(false) {}
    virtual ~AculabTrunk() {}

	// Connection establishment 
	virtual int listen();
	virtual int connect(const SAP& local, const SAP& remote, unsigned aTimeout = indefinite);
	
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

	static void start() { dispatcher.start(); }

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

#endif
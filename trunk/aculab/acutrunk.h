/*
	acutrunk.h

	$Id: acutrunk.h,v 1.8 2003/11/26 00:09:28 lars Exp $

	Copyright 1995-2001 Lars Immisch

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

	void lock()		{ m_mutex.lock(); }
	void unlock()	{ m_mutex.unlock(); }

	void add(ACU_INT handle, AculabTrunk* trunk);
	void remove(ACU_INT handle);

	void start() {  start_undetached(); }

protected:

    virtual void* run_undetached(void* arg);

	std::map<ACU_INT, AculabTrunk*> m_handle_map;
	omni_mutex m_mutex;
};

class AculabTrunk : public Trunk
{
public:

	AculabTrunk(TrunkClient* aClient, int aPort) 
		: Trunk(aClient), m_handle(-1), m_port(aPort), 
		m_stopped(false) {}
    virtual ~AculabTrunk() {}

	// Connection establishment 
	virtual int listen();
	virtual int connect(const SAP& local, const SAP& remote, unsigned aTimeout = indefinite);
	
	// must be called by client after a t_connect_request
	virtual int accept();
	
	// Dissolve a connection
	virtual int disconnect(int cause = 0);
		
	// forces the state to idle
    virtual void abort();
	
    virtual bool hasDetails()		{ return true; }
    virtual bool needDSPSupport()	{ return false; }

	void lock() { m_mutex.lock(); }
	void unlock() { m_mutex.unlock(); }

	static void start();

protected:

	static const char* eventName(ACU_LONG event);

	void setName(int ts);

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

	omni_mutex m_mutex;
	ACU_INT m_handle;
	int m_port;
	bool m_stopped;

	static siginfo_xparms s_siginfo[MAXPORT];
	static CallEventDispatcher s_dispatcher;
};

#endif
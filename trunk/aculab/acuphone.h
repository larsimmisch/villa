/*
	acuphone.h

	$Id: acuphone.h,v 1.1 2000/10/18 16:58:42 lars Exp $

	Copyright 2000 ibp (uk) Ltd.

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _ACUPHONE_H_
#define _ACUPHONE_H_

#pragma warning (disable: 4786)

#include "acutrunk.h"
#include "smdrvr.h"
#include "smosintf.h"
#include "smbesp.h"
#include "switch.h"

const char* prosody_error(int);

class AculabSwitch : public Switch
{
public:

	AculabSwitch(int device) : Switch(device, "Aculab") {}
	virtual ~AculabSwitch() {}
	
	virtual void listen(const Timeslot &a, const Timeslot &b);
	virtual void listen(const Timeslot &a, char pattern);

	virtual void disable(const Timeslot &a);
	virtual char sample(const Timeslot &a);
	
	virtual Timeslot query(const Timeslot &a);
};

class ProsodyError : public Exception
{
public:

    ProsodyError(const char *file, int line, const char* function, int rc, Exception* prev = 0)
		: Exception(file, line, function, "ProsodyError", prev), error(rc) {}

    virtual ~ProsodyError() {}

    virtual void printOn(std::ostream& out) const
	{
		Exception::printOn(out);
		out << ": " << prosody_error(error);
	}

protected:

	const char* description;
	int error;
};

class ProsodyObserver
{
public:

	virtual void onRead(tSMEventId id) = 0;
	virtual void onWrite(tSMEventId id) = 0;
	virtual void onRecog(tSMEventId id) = 0;
};

// pointer to ProsodyObserver member function. 
// Attention - we are leaving the C++ for Dummies section here.
typedef void (ProsodyObserver::* ProsodyObserverMethod)(tSMEventId);

class ProsodyEventDispatcher
{
public:

	enum { max_observers_per_thread = MAXIMUM_WAIT_OBJECTS };

	ProsodyEventDispatcher() : running(false) {}

	// if add is called after the thread has been started,
	// an exception will be thrown
	// this is for simplicity of this class.
	// if necessary, the restriction can be removed.

	void add(tSMEventId handle, ProsodyObserver* observer, ProsodyObserverMethod method);

	// remove is not implemented for simplicity (we don't currently need it)

	// call start when all observers have been added
	void start();

protected:

	void dispatch(int offset);

	class DispatcherThread : public omni_thread
	{
	public:

		DispatcherThread(ProsodyEventDispatcher& d, int o, int l) : dispatcher(d), offset(o), length(l) {}

		void start() { start_undetached(); }

	protected:

		virtual void* run_undetached(void* arg);

		ProsodyEventDispatcher& dispatcher;
		int offset;
		int length;
	};

	friend class DispatcherThread;

	struct ObserverMethod
	{
		ObserverMethod(ProsodyObserver* o, ProsodyObserverMethod m) : observer(o), method(m) {} 

		ProsodyObserver* observer;
		ProsodyObserverMethod method;
	};

	bool running;
	// events and methods are parallel arrays.
	std::vector<tSMEventId> events;
	std::vector<ObserverMethod> methods;
	std::vector<DispatcherThread*> threads;
};

// this is a full duplex Prosody channel
class ProsodyChannel : public ProsodyObserver
{
public:

	ProsodyChannel();
	virtual ~ProsodyChannel()
	{
		sm_channel_release(channel);
		smd_ev_free(eventRead);
		smd_ev_free(eventWrite);
		smd_ev_free(eventRecog);
	}

	// cast operator
	operator tSMChannelId() { return channel; }

protected:

	tSMEventId set_event(tSM_INT type);

	tSMChannelId channel;
	tSMEventId eventRead;
	tSMEventId eventWrite;
	tSMEventId eventRecog;
	struct sm_listen_for_parms listenFor;
	struct sm_channel_info_parms info;

	static ProsodyEventDispatcher dispatcher;
};

class AculabPhone : public Telephone, public ProsodyChannel
{
public:

	AculabPhone(TelephoneClient *client, Trunk* trunk, int switchNo, Timeslot receive = Timeslot(-1, -1), Timeslot transmit = Timeslot(-1,-1), void* aClientData = 0) 
		: Telephone(client, trunk, receive, transmit, aClientData), sw(switchNo)
	{
		if (trunk)
			trunk->setTelephone(this);
	}
	virtual ~AculabPhone() {}

	virtual void connected(Trunk* aTrunk);
	virtual void disconnected(Trunk *trunk, int cause);

    virtual Switch* getSwitch() { return &sw; }

	static void start() { dispatcher.start(); }

protected:

	virtual unsigned startBeeps(int beeps);
	virtual bool stopBeeps();

	virtual unsigned startTouchtones(const char* tt);
	virtual bool stopTouchtones();

	virtual unsigned startEnergyDetector(unsigned qualTime);
	virtual bool stopEnergyDetector();

	// return msecs queued
	virtual unsigned startPlaying();
	virtual unsigned submitPlaying();

	// returns true if operations was stopped synchronously
	virtual bool stopPlaying();

	virtual unsigned startRecording(unsigned maxTime);
	virtual unsigned submitRecording();

	// returns true if operations was stopped synchronously
	virtual bool stopRecording();

	virtual Buffers* allocateBuffers(const char* aFile, unsigned numBuffers, bool isRecording = 0);

	int checkReplayStatus(Telephone::FileSample* sample);

	virtual void onRead(tSMEventId id);
	virtual void onWrite(tSMEventId id);
	virtual void onRecog(tSMEventId id);

	AculabSwitch sw;
};

#endif
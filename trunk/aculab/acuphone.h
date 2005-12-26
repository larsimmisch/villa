/*
	acuphone.h

	$Id$

	Copyright 1995-2001 Lars Immisch

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
#include "ringbuffer.h"

const char* prosody_error(int);

class Storage;

class AculabSwitch : public Switch
{
public:

	AculabSwitch(int device) : Switch(device, "Aculab") {}
	virtual ~AculabSwitch() {}
	
	virtual void listen(const Timeslot &a, const Timeslot &b, const char *name = 0);
	virtual void listen(const Timeslot &a, char pattern, const char *name = 0);

	virtual void disable(const Timeslot &a, const char *name = 0);
	virtual char sample(const Timeslot &a, const char *name = 0);
	
	virtual Timeslot query(const Timeslot &a, const char *name = 0);
};

class ProsodyError : public Exception
{
public:

    ProsodyError(const char *file, int line, const char* function, int rc, Exception* prev = 0)
		: Exception(file, line, function, "ProsodyError", prev), m_error(rc) {}

    virtual ~ProsodyError() {}

    virtual void printOn(std::ostream& out) const
	{
		Exception::printOn(out);
		out << ": " << prosody_error(m_error);
	}

	const char* m_description;
	int m_error;
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

	ProsodyEventDispatcher() : m_running(false) {}

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

		DispatcherThread(ProsodyEventDispatcher& d, int o, int l) 
			: m_dispatcher(d), m_offset(o), m_length(l), 
			omni_thread(NULL, PRIORITY_HIGH) {}

		void start() { start_undetached(); }

	protected:

		virtual void* run_undetached(void* arg);

		ProsodyEventDispatcher& m_dispatcher;
		int m_offset;
		int m_length;
	};

	friend class DispatcherThread;

	struct ObserverMethod
	{
		ObserverMethod(ProsodyObserver* o, ProsodyObserverMethod m) 
			: m_observer(o), m_method(m) {} 

		ProsodyObserver *m_observer;
		ProsodyObserverMethod m_method;
	};

	bool m_running;
	// events and methods are parallel arrays.
	std::vector<tSMEventId> m_events;
	std::vector<ObserverMethod> m_methods;
	std::vector<DispatcherThread*> m_threads;
};

// this is a full duplex Prosody channel
class ProsodyChannel : public ProsodyObserver
{
public:

	ProsodyChannel();
	virtual ~ProsodyChannel()
	{
		sm_channel_release(m_channel);
		smd_ev_free(m_eventRead);
		smd_ev_free(m_eventWrite);
		smd_ev_free(m_eventRecog);
	}

	// cast operator
	operator tSMChannelId() { return m_channel; }

	virtual void startEnergyDetector(unsigned qualTime);
	virtual void stopEnergyDetector();

	void conferenceStart();
	void conferenceClone(ProsodyChannel *model);
	void conferenceAdd(ProsodyChannel *party);
	void conferenceLeave(ProsodyChannel *party);
	void conferenceAbort();
	void conferenceEC();

protected:

	class FileSample : public Sample
	{
	public:

		FileSample(ProsodyChannel *channel, const char* file, 
			bool isRecordable = false);

		virtual ~FileSample();

        virtual unsigned start(Media *phone);
        virtual bool stop(Media *phone, unsigned status = V3_STOPPED);
		virtual unsigned submit(Media *phone);
		// fills prosody buffers if space available, notifies about completion if done
		virtual int process(Media *phone);

		Storage* allocateStorage(const char *name, bool isRecordable);

		virtual unsigned getLength();

		virtual Storage* getStorage() { return m_storage; }

	protected:
		
		bool m_recordable;
		Storage* m_storage;
		ProsodyChannel *m_prosody;
		std::string m_name;
	};

	class RecordFileSample : public FileSample
	{
	public:
	
		RecordFileSample(ProsodyChannel *channel, const char* name, 
						 unsigned maxTime, unsigned maxSilence)
			: FileSample(channel, name, true), m_maxTime(maxTime), 
			m_maxSilence(maxSilence) {}
		virtual ~RecordFileSample() {}

        virtual unsigned start(Media *phone);
        virtual bool stop(Media *phone, unsigned status = V3_STOPPED);
		virtual unsigned receive(Media *phone);
		// empties prosody buffers if data available, notifies about completion if done
		virtual int process(Media *phone);

		virtual bool isOutgoing()	{ return false; }

	protected:

		unsigned m_maxTime;
		unsigned m_maxSilence;
	};

	class Beep : public Sample
	{
	public:
		
		Beep(ProsodyChannel *channel, int numBeeps) : 
		  m_beeps(numBeeps), m_prosody(channel), m_count(0), m_offset(0) {}
		virtual ~Beep() {}

        virtual unsigned start(Media *phone);
        virtual bool stop(Media *phone, unsigned status = V3_STOPPED);
		virtual unsigned submit(Media *phone);
		// fills prosody buffers if space available, notifies about completion if done
		virtual int process(Media *phone);

		int m_beeps;
		int m_count;
		unsigned m_offset;
		ProsodyChannel *m_prosody;
	};

	class Touchtones : public Sample
	{
	public:
		
		Touchtones(ProsodyChannel *channel, const char* att) 
			: m_tt(att), m_prosody(channel) {}
		virtual ~Touchtones() {}

        virtual unsigned start(Media *phone);
        virtual bool stop(Media *phone, unsigned status = V3_STOPPED);
		virtual int process(Media *phone);

		std::string m_tt;
		ProsodyChannel *m_prosody;
	};

	class UDPStream : public Sample
	{
	public:

		UDPStream(ProsodyChannel *channel, int port);

		virtual ~UDPStream();

        virtual unsigned start(Media *phone);
        virtual bool stop(Media *phone, unsigned status = V3_STOPPED);
		virtual unsigned submit(Media *phone);
		// fills prosody buffers if space available, notifies about completion if done
		virtual int process(Media *phone);
		virtual unsigned getLength();

	protected:

		unsigned startOutput(Media *phone);
		
		ProsodyChannel *m_prosody;
		SOCKET m_socket;
		int m_port;
		ringbuffer<char> m_buffer;
		unsigned m_bytes_played;
	};

	friend class FileSample;
	friend class RecordFileSample;
	friend class Touchtones;
	friend class Beep;
	friend class UDPStream;

	tSMEventId set_event(tSM_INT type);

	tSMChannelId m_channel;
	tSMEventId m_eventRead;
	tSMEventId m_eventWrite;
	tSMEventId m_eventRecog;
	int m_conferenceId;
	struct sm_listen_for_parms m_listenFor;
	struct sm_channel_info_parms m_info;
	omni_mutex m_mutex;
	Sample *m_sending;
	Sample *m_receiving;

	static ProsodyEventDispatcher s_dispatcher;
};

class AculabMedia : public Media, public ProsodyChannel
{
public:

	AculabMedia(MediaClient *client, Timeslot receive = Timeslot(-1, -1), Timeslot transmit = Timeslot(-1,-1), void* aClientData = 0) 
		: Media(client, receive, transmit, aClientData), m_sw(m_info.card), m_trunk(0)
	{
		sprintf(m_name, "Prosody[%x]", m_channel);
	}
	virtual ~AculabMedia() {}

	virtual void connected(Trunk* aTrunk);
	virtual void disconnected(Trunk *trunk);

    virtual Switch* getSwitch() { return &m_sw; }

	virtual Sample* createFileSample(const char *name) 
		{ return new FileSample(this, name); }

	virtual Sample* createRecordFileSample(const char *name, unsigned maxTime,
										   unsigned maxSilence)
		{ return new RecordFileSample(this, name, maxTime, maxSilence); }

	virtual Sample* createTouchtones(const char *tt) 
		{ return new Touchtones(this, tt); }

	virtual Sample* createBeeps(int nBeeps) 
		{ return new Beep(this, nBeeps); }

	virtual Sample* createUDPStream(int port) 
		{ return new UDPStream(this, port); }

	virtual void startEnergyDetector(unsigned qualTime) 
		{ ProsodyChannel::startEnergyDetector(qualTime); }

	virtual void stopEnergyDetector() 
		{ ProsodyChannel::stopEnergyDetector(); }

	/* listen to our own output */
	void loopback();

	virtual const char* getName() { return m_trunk ? m_trunk->getName() : m_name; }

	static void start() { s_dispatcher.start(); }

protected:

	virtual void onRead(tSMEventId id);
	virtual void onWrite(tSMEventId id);
	virtual void onRecog(tSMEventId id);

	AculabSwitch m_sw;
	// only for getName()
	Trunk *m_trunk;
	char m_name[64];
};

#endif

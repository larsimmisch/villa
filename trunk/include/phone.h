/*
	phone.h    

	$Id: phone.h,v 1.3 2000/10/18 16:58:43 lars Exp $

	Copyright 2000 ibp (uk) Ltd.

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _PHONE_H_
#define _PHONE_H_ 

#include "switch.h"
#include "sap.h"
#include "exc.h"
#include "buffers.h"

enum { indefinite = -1 };

enum result  
{
	r_ok, 
	r_timeout, 
	r_aborted, 
	r_rejected, 
	r_disconnected, 
	r_failed, 
	r_invalid, 
	r_busy, 
	r_not_available,
	r_no_dialtone,
	r_empty,
	r_bad_state,
	r_number_changed
};

class Telephone;
class TelephoneClient;
class TrunkClient;

class Trunk
{
public:

    enum states  { idle, listening, connecting, connected, disconnecting, transferring, waiting, collecting_details, accepting, rejecting };

	Trunk(TrunkClient* aClient, Telephone* aTelephone = 0) : client(aClient), phone(aTelephone) {}
    virtual ~Trunk() {}

	// Connection establishment 
	virtual int listen() = 0;
	virtual int connect(const SAP& local, const SAP& remote, unsigned timeout = indefinite) = 0;
	
	// must be called by client after a t_connect_request
	// after acceptDone is called, the call is connected if the result is r_ok or else idle
	virtual int accept() = 0;
	virtual int reject(int cause = 0) = 0;
	
	// transfer
	virtual int transfer(const SAP& remote, unsigned timeout = indefinite) { return r_not_available; }

	// Dissolve a connection
	virtual int disconnect(int cause = 0) = 0;
	
	// must be called by client after a disconnectRequest
	virtual int disconnectAccept() = 0;
	
	// forces the state to idle - synchronous
    virtual void abort() = 0;
	
    virtual bool hasDetails()		{ return false; }
    virtual bool needDSPSupport()	{ return false; }

    virtual void setTelephone(Telephone* aTelephone)    { phone = aTelephone; }
	virtual Telephone* getTelephone() { return phone; }

    virtual Switch* getSwitch() { return 0; }
	virtual Timeslot getTimeslot()		{ return timeslot; }

protected:

	friend class Telephone;

	volatile states state;
	Timeslot timeslot;
	TrunkClient* client;
    Telephone* phone;
};

class Telephone
{
public:

    Telephone(TelephoneClient *aClient, Trunk *aTrunk, Timeslot rcv = Timeslot(-1,-1), Timeslot xmit = Timeslot(-1,-1), void* aClientData = 0) 
		: client(aClient), trunk(aTrunk), receive(rcv), transmit(xmit), clientData(aClientData), current(NULL) {}
    virtual ~Telephone() {}

    // Connection establishment
    virtual int listen() 
	{ 
		if (!trunk) 
			return r_failed; 
		
		return trunk->listen(); 
	}

    virtual int connect(const SAP &local, const SAP& remote, unsigned timeout = indefinite)
	{
		if (!trunk)
			return r_failed;

		return trunk->connect(local, remote, timeout);
	}

	// transfer a caller
	virtual int transfer(const SAP& remote, unsigned timeout = indefinite)
	{
		if (!trunk)
			return r_failed;

		return trunk->transfer(remote, timeout);
	}
    
	// must be called by client after a t_connect_request
    virtual int accept()
	{
		if (!trunk)
			return r_failed;

		return trunk->accept();
	}

    virtual int reject(int cause = 0)
	{
		if (!trunk)
			return r_failed;

		return trunk->reject(cause);
	}

    // Dissolve a connection
    virtual int disconnect(int cause = 0)
	{
		if (!trunk)
			return r_failed;

		return trunk->disconnect(cause);
	}

    // abort whatever is going on trunkwise - synchronous
    virtual void abort()
	{
		if (trunk)
			trunk->abort();
	}
    // called from Trunk
    virtual void disconnected(Trunk* aTrunk, int aCause)
	{
		abortSending();
	}

	virtual void remoteRinging(Trunk* aTrunk) {}

	virtual void connected(Trunk* aTrunk) {}

	// Caution: do _never_ create Sample subclasses on the stack
    class Sample
    {
	public:

        Sample(unsigned aPosition = 0) : position(aPosition), userData(0), active(false), status(r_ok) {}
        virtual ~Sample() {}

        virtual unsigned start(Telephone* aTelephone) = 0;
        virtual bool stop(Telephone* aTelephone) = 0;

        virtual unsigned getLength()	{ return 0; }
		virtual unsigned getStatus()	{ return status; }
        virtual bool isActive()         { return active; }
		virtual bool isOutgoing()		{ return true; }

		// is called with mutex held
		void started()
		{
			active = true;
		}

		// is called with mutex held
		void completed(unsigned msecs)
		{
			position = msecs;
			active = false;
		}

        bool setPos(unsigned aPosition)	{ position = aPosition; return position <= getLength(); }
        unsigned getPos()               { return position; }

        void setUserData(void* data)    { userData = data; }
        void* getUserData()             { return userData; }

        unsigned position;
		unsigned status;

	protected:

        void* userData;
        bool active;
    };

    // Data transfer
    virtual bool start(Sample *aSample)
	{
		omni_mutex_lock l(mutex);

		if (current)
			return false;

		current = aSample;

		aSample->start(this);

		return true;
	}

	virtual Sample* getCurrentSample() { return current; }

    virtual bool abortSending()
	{
		omni_mutex_lock l(mutex);

		if (current) 
			return current->stop(this);

		return false;
	}

    bool isIdle()    	
	{ 
		omni_mutex_lock l(mutex);

		return trunk ? (trunk->state == Trunk::idle) : true; 
	}
    bool isConnected()	
	{ 
		omni_mutex_lock l(mutex);

		return trunk ? (trunk->state == Trunk::connected) : false; 
	}

	Trunk::states getState()
	{
		omni_mutex_lock l(mutex);

		return trunk ? trunk->state : Trunk::idle; 
	}

    SAP& getRemoteSAP() { return remote; }
    SAP& getLocalSAP()  { return local; }

    void setTrunk(Trunk* aTrunk)
	{
		omni_mutex_lock l(mutex);

		trunk = aTrunk; 
		if (trunk) 
			trunk->setTelephone(this);
	}

	// these are uni-directional timeslots
    Timeslot getTransmitTimeslot()		{ return transmit; }
    Timeslot getReceiveTimeslot()		{ return receive; }

    void setTransmitTimeslot(Timeslot aTimeslot) { transmit = aTimeslot; }
    void setReceiveTimeslot(Timeslot aTimeslot)	{ receive = aTimeslot; }

	// the trunk timeslot is bidirectional
	Timeslot getTrunkTimeslot()
	{
		omni_mutex_lock l(mutex);

		return trunk ? trunk->getTimeslot() : Timeslot(-1, -1);
	}

    virtual Switch* getSwitch()	{ return 0; }

	virtual TelephoneClient* getClient() { return client; }

	class FileSample : public Sample
	{
	public:
		
		FileSample(Telephone* aPhone, const char* name, bool isRecordable = false)
			: recordable(isRecordable), buffers(0)
		{
			buffers = aPhone->allocateBuffers(name, 4, isRecordable);
		}

		virtual ~FileSample()
		{
			delete buffers;
		}
		
        virtual unsigned start(Telephone* aTelephone)
		{
			buffers->setPos(position);

			// read as many buffers as possible
			for (int i = 0; i < buffers->getNumBuffers(); ++i)
			{
				buffers->read();
				if (buffers->isLast())
					break;
			}

			position = aTelephone->startPlaying();

			Sample::started();

			return position; 
		}

		virtual unsigned next(Telephone* aTelephone)
		{
			position += aTelephone->submitPlaying();

			if (!buffers->isLast())	
				buffers->read();

			return position;
		}

        virtual bool stop(Telephone* aTelephone)
		{
			return aTelephone->stopPlaying(); 
		}

		virtual unsigned getLength() { return buffers ? buffers->getLength() : 0; }
		virtual unsigned getStatus() {	return buffers ? buffers->getStatus() : r_failed; }

		virtual Buffers* getBuffers() { return buffers; }

	protected:
		
		bool recordable;
		Buffers* buffers;
	};

	class RecordFileSample : public FileSample
	{
	public:
	
		RecordFileSample(Telephone* aPhone, const char* name, unsigned max)
			: Telephone::FileSample(aPhone, name, 1), maxTime(max) {}
		virtual ~RecordFileSample() {}
		
        virtual unsigned start(Telephone* aTelephone)
		{
			position = aTelephone->startRecording(maxTime);

			Sample::started();

			return position; 
		}

		virtual unsigned next(Telephone* aTelephone)
		{
			position += aTelephone->submitRecording();

			// buffers->write(); ??

			return position;
		}

        virtual bool stop(Telephone* aTelephone)
		{
			return aTelephone->stopRecording(); 
		}


		virtual bool isOutgoing()	{ return false; }

	protected:

		unsigned maxTime;
	};

	class Beep : public Sample
	{
	public:
		
		Beep(int numBeeps) : beeps(numBeeps) {}
		virtual ~Beep();
				
        virtual unsigned start(Telephone* aTelephone)
		{
			position = aTelephone->startBeeps(beeps);

			Sample::started(); 

			return position; 
		}

        virtual bool stop(Telephone* aTelephone)	
		{ 
			return aTelephone->stopBeeps(); 
		}

		int beeps;
	};

	class Touchtones : public Sample
	{
	public:
		
		Touchtones(const char* att) : tt(att) {}
		virtual ~Touchtones() {}
		
        virtual unsigned start(Telephone* aTelephone) 
		{ 
			position = aTelephone->startTouchtones(tt.c_str());

			Sample::started(); 

			return position; 
		}

        virtual bool stop(Telephone* aTelephone)
		{ 
			return aTelephone->stopTouchtones(); 
		}

		std::string tt;
	};

protected:
	
    friend class Sample;
    friend class FileSample;
    friend class RecordFileSample;
    friend class Beep;
    friend class Touchtones;
	friend class Buffers;
	
	void lock() { mutex.lock(); }
	void unlock() { mutex.unlock(); }

	// these will be called with mutex held
	virtual unsigned startBeeps(int beeps) = 0;
	virtual bool stopBeeps() = 0;

	virtual unsigned startTouchtones(const char* tt) = 0;
	virtual bool stopTouchtones() = 0;

	virtual unsigned startEnergyDetector(unsigned qualTime) = 0;
	virtual bool stopEnergyDetector() = 0;

	// return msecs queued
	virtual unsigned startPlaying() = 0;
	virtual unsigned submitPlaying() = 0;

	// returns true if operations was stopped synchronously
	virtual bool stopPlaying() = 0;

	virtual unsigned startRecording(unsigned maxTime) = 0;
	virtual unsigned submitRecording() = 0;

	// returns true if operations was stopped synchronously
	virtual bool stopRecording() = 0;

	virtual Buffers* allocateBuffers(const char* aFile, unsigned numBuffers, bool isRecording = 0) = 0;

    virtual void started(Sample* aSample) {}

protected:

	SAP local;
	SAP remote;
	Timeslot transmit;
	Timeslot receive;
    Trunk* trunk;
    void* clientData;
	Sample* current;
	TelephoneClient* client;
	omni_mutex mutex;
};

#endif

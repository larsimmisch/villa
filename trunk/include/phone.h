/*
	phone.h    

	$Id: phone.h,v 1.1 2000/10/02 15:52:14 lars Exp $

	Copyright 2000 ibp (uk) Ltd.

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _PHONE_H_
#define _PHONE_H_ 

#include "switch.h"
#include "sap.h"
#include "exc.h"

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

class Buffers;
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
	virtual int connect(SAP& aLocalSAP, SAP& aRemoteSAP, unsigned aTimeout = indefinite) = 0;
	
	// must be called by client after a t_connect_request
	// after acceptDone is called, the call is connected if the result is r_ok or else idle
	virtual int accept() = 0;
	virtual int reject(int cause = 0) = 0;
	
	// transfer
	virtual int transfer(SAP& aRemote, unsigned aTimeout = indefinite) { return r_not_available; }

	// Dissolve a connection
	virtual int disconnect(int cause = 0) = 0;
	
	// must be called by client after a disconnectRequest
	virtual int disconnectAccept() = 0;
	
	// forces the state to idle
    virtual void abort() = 0;
	
    virtual bool hasDetails()		{ return false; }
    virtual bool needDSPSupport()	{ return false; }

    virtual void setTelephone(Telephone* aTelephone)    { phone = aTelephone; }

    virtual Switch* getSwitch() { return 0; }
	virtual Slot getSlot()		{ return Slot(-1, -1); }

protected:

	friend class Telephone;

	volatile states state;
	Slot slot;
	TrunkClient* client;
    Telephone* phone;
};

class Telephone
{
public:

    Telephone(Trunk* aTrunk, Slot aSlot = Slot(-1, -1), void* aClientData = 0) : trunk(aTrunk) {}
    virtual ~Telephone() {}

    // Connection establishment
    virtual int listen(SAP& aLocalSAP, unsigned aTimeout = indefinite) = 0;
    virtual int connect(SAP& aRemoteSAP, unsigned aTimeout = indefinite) = 0;

	// transfer a caller
	virtual int transfer(SAP& remoteSAP, unsigned aTimeout = indefinite) = 0;
    
	// must be called by client after a t_connect_request
    virtual int accept() = 0;
    virtual int reject(int cause = 0) = 0;

    // Dissolve a connection
    virtual int disconnect(unsigned aTimeout = indefinite, int cause = 0);

    // abort whatever is going on trunkwise
    virtual void abort() = 0;

    // called from Trunk
    virtual void disconnected(Trunk* aTrunk, int aCause);
	virtual void remoteRinging(Trunk* aTrunk) {}

    class Sample
    {
	public:

        Sample(unsigned aPosition = 0) : position(aPosition), userData(0), active(0) {}
        virtual ~Sample() {}

        virtual unsigned play(Telephone* aTelephone);
        virtual unsigned start(Telephone* aTelephone) = 0;
        virtual unsigned process(Telephone* aTelephone) = 0; 
        virtual int stop(Telephone* aTelephone) = 0;

        virtual unsigned getLength()	{ return 0; }
		virtual unsigned getStatus()	{ return r_ok; }
        virtual int isActive()          { return active; }
		virtual int isOutgoing()		{ return 1; }

		void started(Telephone* aPhone);
		void completed(Telephone* aPhone);

        int setPos(unsigned aPosition)	{ position = aPosition; return position <= getLength(); }
        unsigned getPos()               { return position; }

        void setUserData(void* data)    { userData = data; }
        void* getUserData()             { return userData; }

        unsigned position;

	protected:

        void* userData;
        bool active;
    };

    // Data transfer
    virtual int send(Sample& aSample, unsigned aTimeout = indefinite);

    virtual int abortSending();

    bool isIdle()    	{ return trunk ? (trunk->state == Trunk::idle) : true; }
    bool isConnected()	{ return trunk ? (trunk->state == Trunk::connected) : false; }

	Trunk::states getState()	{ return trunk ? trunk->state : Trunk::idle; }

    SAP& getRemoteSAP() { return remote; }
    SAP& getLocalSAP()  { return local; }

    void setTrunk(const Trunk* aTrunk);

    Slot getSlot()		{ return slot; }
	Slot getTrunkSlot();

    void setSlot(Slot aSlot)	{ slot = aSlot; }

    virtual Switch* getSwitch()	{ return 0; }

	class FileSample : public Sample
	{
	public:
		
		FileSample(Telephone* aPhone, const char* name, int isRecordable = 0, unsigned aMessage = 1);
		virtual ~FileSample();
		
        virtual unsigned start(Telephone* aTelephone);
        virtual unsigned process(Telephone* aTelephone);
        virtual int stop(Telephone* aTelephone);

		virtual unsigned getLength();
		virtual unsigned getStatus();

	protected:
		
		int recordable;
		Buffers* buffers;
	};

	class RecordFileSample : public FileSample
	{
	public:
	
		RecordFileSample(Telephone* aPhone, const char* name, unsigned maxTime, unsigned aMessage = 1);
		virtual ~RecordFileSample() {}
		
        virtual unsigned start(Telephone* aTelephone);
        virtual unsigned process(Telephone* aTelephone);
        virtual int stop(Telephone* aTelephone);

		virtual int isOutgoing()	{ return 0; }

	protected:

		unsigned maxTime;
	};

	class Beep : public Sample
	{
	public:
		
		Beep(int numBeeps)	: beeps(numBeeps) {}
		virtual ~Beep();
				
        virtual unsigned start(Telephone* aTelephone);
        virtual unsigned process(Telephone* aTelephone);
        virtual int stop(Telephone* aTelephone)	{ return aTelephone->stopBeeps(); }

		virtual unsigned getStatus()	{ return status; }

		int beeps;
		unsigned status;
	};

	class Touchtones : public Sample
	{
	public:
		
		Touchtones(const char* tt);
		virtual ~Touchtones();
		
        virtual unsigned start(Telephone* aTelephone);
        virtual unsigned process(Telephone* aTelephone);
        virtual int stop(Telephone* aTelephone)	{ return aTelephone->stopTouchtones(); }

		virtual unsigned getStatus()	{ return status; }

		char* tt;
		unsigned status;
	};

	class EnergyDetector : public Sample
	{
	public:
		
		EnergyDetector(unsigned qTime, unsigned mTime) : qualTime(qTime), maxTime(mTime), status(r_ok) {}
		virtual ~EnergyDetector() {}
		
        virtual unsigned start(Telephone* aTelephone);
        virtual unsigned process(Telephone* aTelephone);
        virtual int stop(Telephone* aTelephone)	{ return aTelephone->stopEnergyDetector(); }

		virtual unsigned getStatus()	{ return status; }

		// qualification time in ms
		unsigned qualTime;
		unsigned maxTime;
		unsigned status;
	};

protected:
	
    friend class Sample;
    friend class FileSample;
    friend class RecordFileSample;
    friend class Beep;
    friend class Touchtones;
	friend class Buffers;
	friend class EnergyDetector;
	
	virtual unsigned startBeeps(int beeps) = 0;
	virtual int stopBeeps() = 0;

	virtual unsigned startTouchtones(const char* tt) = 0;
	virtual int stopTouchtones() = 0;

	virtual unsigned startEnergyDetector(unsigned qualTime, unsigned maxTime) = 0;
	virtual int stopEnergyDetector() = 0;

	virtual Buffers* allocateBuffers(const char* aFile, unsigned numBuffers, int isRecording = 0, unsigned aMessage = 1) = 0;

    virtual void started(Sample* aSample) {}

protected:

	SAP local;
	SAP remote;
	Slot slot;
    Trunk* trunk;
    void* clientData;
	Sample* current;
	TelephoneClient* client;
};

#endif

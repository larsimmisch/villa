/*
	phone.h    

	$Id: phone.h,v 1.11 2001/06/23 09:55:20 lars Exp $

	Copyright 1995-2001 Lars Immisch

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _PHONE_H_
#define _PHONE_H_ 

#include <string>
#include "log.h"
#include "phoneclient.h"
#include "switch.h"
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

class Telephone;
class TelephoneClient;
class TrunkClient;

class Trunk
{
public:

    enum states  { idle, listening, connecting, connected, disconnecting, transferring, waiting, collecting_details, accepting, rejecting };

	Trunk(TrunkClient* aClient, Telephone* aTelephone = 0) 
		: client(aClient), phone(aTelephone), state(idle) {}
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

	void setName(const char* s) { name = std::string(s); }
	const char* getName() { return name.c_str(); }

	virtual states getState() { return state; }

protected:

	friend class Telephone;

	volatile states state;
	Timeslot timeslot;
	TrunkClient* client;
    Telephone* phone;
	std::string name;
};

class Sample
{
public:

    Sample(unsigned pos = 0) : position(pos), userData(0), status(r_ok) {}
    virtual ~Sample() {}

    virtual unsigned start(Telephone* aTelephone) = 0;

    virtual bool stop(Telephone* aTelephone) = 0;

    virtual unsigned getLength()	{ return 0; }
	virtual unsigned getStatus()	{ return status; }
	virtual bool isOutgoing()		{ return true; }

    bool setPos(unsigned aPosition)	{ position = aPosition; return position <= getLength(); }
    unsigned getPos()               { return position; }

    void setUserData(void* data)    { userData = data; }
    void* getUserData()             { return userData; }

    unsigned position;
	unsigned status;

protected:

    void* userData;
};

class Telephone
{
public:

    Telephone(TelephoneClient *aClient, Trunk *aTrunk, 
		Timeslot rcv = Timeslot(-1,-1), 
		Timeslot xmit = Timeslot(-1,-1), void* aClientData = 0) 
		: client(aClient), trunk(aTrunk), receive(rcv), transmit(xmit), 
		clientData(aClientData) {}

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

	virtual void connected(Trunk* aTrunk) {}

	virtual void startEnergyDetector(unsigned qualTime) = 0;
	virtual void stopEnergyDetector() = 0;

	virtual Sample* createFileSample(const char *name) = 0;
	virtual Sample* createRecordFileSample(const char *name, unsigned maxTime) = 0;
	virtual Sample* createTouchtones(const char *tt) = 0;
	virtual Sample* createBeeps(int nBeeps) = 0;

	virtual void touchtone(char tt)
	{
		client->touchtone(this, tt);
	}

    bool isIdle()    	
	{ 
		return trunk ? (trunk->getState() == Trunk::idle) : true; 
	}
    bool isConnected()	
	{ 
		return trunk ? (trunk->getState() == Trunk::connected) : false; 
	}

	Trunk::states getState()
	{
		return trunk ? trunk->state : Trunk::idle; 
	}

    SAP& getRemoteSAP() { return remote; }
    SAP& getLocalSAP()  { return local; }

    void setTrunk(Trunk* aTrunk)
	{
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
		return trunk ? trunk->getTimeslot() : Timeslot(-1, -1);
	}

    virtual Switch* getSwitch()	{ return 0; }

	virtual void completed(Sample *sample) 
	{ 
		client->completed(this, sample, sample->position);
	}
	// todo delete?
	virtual TelephoneClient* getClient() { return client; }		

	// debug
	virtual const char *getName() = 0;

protected:
	
	SAP local;
	SAP remote;
	Timeslot transmit;
	Timeslot receive;
    Trunk* trunk;
    void* clientData;
	TelephoneClient* client;
};

#endif

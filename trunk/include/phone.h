/*
	phone.h    

	$Id: phone.h,v 1.12 2001/07/03 23:13:02 lars Exp $

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

    enum states  { 
		idle, 
		listening, 
		connecting, 
		connected, 
		disconnecting, 
		transferring, 
		waiting, 
		collecting_details, 
		accepting, 
		rejecting 
	};

	Trunk(TrunkClient* aClient, Telephone* aTelephone = 0) 
		: m_client(aClient), m_phone(aTelephone), m_state(idle) {}
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

    virtual void setTelephone(Telephone* aTelephone)    { m_phone = aTelephone; }
	virtual Telephone* getTelephone() { return m_phone; }

    virtual Switch* getSwitch() { return 0; }
	virtual Timeslot getTimeslot()		{ return m_timeslot; }

	void setName(const char* s) { m_name = std::string(s); }
	const char* getName() { return m_name.c_str(); }

	virtual states getState() { return m_state; }

protected:

	friend class Telephone;

	volatile states m_state;
	Timeslot m_timeslot;
	TrunkClient* m_client;
    Telephone* m_phone;
	std::string m_name;
};

class Sample
{
public:

	enum state 
	{
		idle,
		active,
		stopping
	};

    Sample(unsigned pos = 0) : m_position(pos), m_userData(0), 
		m_status(r_ok), m_state(idle) {}
    virtual ~Sample() {}

    virtual unsigned start(Telephone* aTelephone) = 0;

    virtual bool stop(Telephone* aTelephone) = 0;

    virtual unsigned getLength()	{ return 0; }
	virtual unsigned getStatus()	{ return m_status; }
	virtual bool isOutgoing()		{ return true; }

    bool setPos(unsigned aPosition)	{ m_position = aPosition; return m_position <= getLength(); }
    unsigned getPos()               { return m_position; }

    void setUserData(void* data)    { m_userData = data; }
    void* getUserData()             { return m_userData; }

	state m_state;
    unsigned m_position;
	unsigned m_status;

protected:

    void* m_userData;
};

class Telephone
{
public:

    Telephone(TelephoneClient *aClient, Trunk *aTrunk, 
		Timeslot rcv = Timeslot(-1,-1), 
		Timeslot xmit = Timeslot(-1,-1), void* aClientData = 0) 
		: m_client(aClient), m_trunk(aTrunk), m_receive(rcv), m_transmit(xmit), 
		m_clientData(aClientData) {}

    virtual ~Telephone() {}

    // Connection establishment
    virtual int listen() 
	{ 
		if (!m_trunk) 
			return r_failed; 
		
		return m_trunk->listen(); 
	}

    virtual int connect(const SAP &local, const SAP& remote, unsigned timeout = indefinite)
	{
		if (!m_trunk)
			return r_failed;

		return m_trunk->connect(local, remote, timeout);
	}

	// transfer a caller
	virtual int transfer(const SAP& remote, unsigned timeout = indefinite)
	{
		if (!m_trunk)
			return r_failed;

		return m_trunk->transfer(remote, timeout);
	}
    
	// must be called by client after a t_connect_request
    virtual int accept()
	{
		if (!m_trunk)
			return r_failed;

		return m_trunk->accept();
	}

    virtual int reject(int cause = 0)
	{
		if (!m_trunk)
			return r_failed;

		return m_trunk->reject(cause);
	}

    // Dissolve a connection
    virtual int disconnect(int cause = 0)
	{
		if (!m_trunk)
			return r_failed;

		return m_trunk->disconnect(cause);
	}

    // abort whatever is going on trunkwise - synchronous
    virtual void abort()
	{
		if (m_trunk)
			m_trunk->abort();
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
		m_client->touchtone(this, tt);
	}

    bool isIdle()    	
	{ 
		return m_trunk ? (m_trunk->getState() == Trunk::idle) : true; 
	}
    bool isConnected()	
	{ 
		return m_trunk ? (m_trunk->getState() == Trunk::connected) : false; 
	}

	Trunk::states getState()
	{
		return m_trunk ? m_trunk->m_state : Trunk::idle; 
	}

    SAP& getRemoteSAP() { return m_remote; }
    SAP& getLocalSAP()  { return m_local; }

    void setTrunk(Trunk* aTrunk)
	{
		m_trunk = aTrunk; 
		if (m_trunk) 
			m_trunk->setTelephone(this);
	}

	// these are uni-directional timeslots
    Timeslot getTransmitTimeslot()		{ return m_transmit; }
    Timeslot getReceiveTimeslot()		{ return m_receive; }

    void setTransmitTimeslot(Timeslot aTimeslot) { m_transmit = aTimeslot; }
    void setReceiveTimeslot(Timeslot aTimeslot)	{ m_receive = aTimeslot; }

	// the trunk timeslot is bidirectional
	Timeslot getTrunkTimeslot()
	{
		return m_trunk ? m_trunk->getTimeslot() : Timeslot(-1, -1);
	}

    virtual Switch* getSwitch()	{ return 0; }

	virtual void completed(Sample *sample) 
	{ 
		m_client->completed(this, sample, sample->m_position);
	}
	// todo delete?
	virtual TelephoneClient* getClient() { return m_client; }		

	// debug
	virtual const char *getName() = 0;

protected:
	
	SAP m_local;
	SAP m_remote;
	Timeslot m_transmit;
	Timeslot m_receive;
    Trunk* m_trunk;
    void* m_clientData;
	TelephoneClient* m_client;
};

#endif

/*
	phone.h    

	$Id: phone.h,v 1.16 2003/11/22 23:44:46 lars Exp $

	Copyright 1995-2001 Lars Immisch

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _PHONE_H_
#define _PHONE_H_ 

#include <string>
#include "log.h"
#include "phoneclient.h"
#include "switch.h"
#include "errors.h"
#include "exc.h"

enum { indefinite = -1 };

class Media;
class MediaClient;
class TrunkClient;

class Trunk
{
public:

    enum states  { 
		idle, 
		listening, 
		connecting, 
		connected, 
		remote_disconnect,
		disconnecting,
		transferring, 
		waiting, 
		collecting_details, 
		accepting, 
		rejecting 
	};

	Trunk(TrunkClient* aClient) 
		: m_client(aClient), m_state(idle) {}
    virtual ~Trunk() {}

	// Connection establishment 
	virtual int listen() = 0;
	virtual int connect(const SAP& local, const SAP& remote, unsigned timeout = indefinite) = 0;
	
	// must be called by client after a t_connect_request
	// after acceptDone is called, the call is connected if the result is r_ok or else idle
	virtual int accept() = 0;
	virtual int reject(int cause = 0) = 0;
	
	// transfer
	virtual int transfer(const SAP& remote, unsigned timeout = indefinite) 
	{ 
		return PHONE_ERROR_NOT_IMPLEMENTED; 
	}

	// Dissolve a connection
	virtual int disconnect(int cause = 0) = 0;
	
	// must be called by client after a disconnectRequest
	virtual int disconnectAccept() = 0;
	
	// forces the state to idle - synchronous
    virtual void abort() = 0;
	
    virtual bool hasDetails()		{ return false; }
    virtual bool needDSPSupport()	{ return false; }

    virtual Switch* getSwitch() { return 0; }
	virtual Timeslot getTimeslot()		{ return m_timeslot; }

	void setName(const char* s) { m_name = std::string(s); }
	const char* getName() { return m_name.c_str(); }

	virtual states getState() { return m_state; }

	static const char* stateName(states state)
	{
		switch (state)
		{
		case idle:
			return "idle";
		case listening:
			return "listening";
		case connecting:
			return "connecting";
		case connected:
			return "connected";
		case disconnecting:
			return "disconnecting";
		case remote_disconnect:
			return "remote_disconnect";
		case transferring:
			return "transferring";
		case waiting:
			return "waiting";
		case collecting_details:
			return "collecting_details";
		case accepting:
			return "accepting";
		case rejecting:
			return "rejecting";
		default:
			return "illegal state";
		}
	}

protected:

	volatile states m_state;
	Timeslot m_timeslot;
	TrunkClient* m_client;
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
		m_status(PHONE_OK), m_state(idle) {}
    virtual ~Sample() {}

    virtual unsigned start(Media* aMedia) = 0;

    virtual bool stop(Media* aMedia) = 0;

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

class Media
{
public:

    Media(MediaClient *aClient, 
		Timeslot rcv = Timeslot(-1,-1), 
		Timeslot xmit = Timeslot(-1,-1), void* aClientData = 0) 
		: m_client(aClient), m_receive(rcv), m_transmit(xmit), 
		m_clientData(aClientData) {}

    virtual ~Media() {}

	virtual void connected(Trunk* aTrunk) = 0;
	virtual void disconnected(Trunk *trunk) = 0;

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

    SAP& getRemoteSAP() { return m_remote; }
    SAP& getLocalSAP()  { return m_local; }

	// these are uni-directional timeslots
    Timeslot getTransmitTimeslot()		{ return m_transmit; }
    Timeslot getReceiveTimeslot()		{ return m_receive; }

    void setTransmitTimeslot(Timeslot aTimeslot) { m_transmit = aTimeslot; }
    void setReceiveTimeslot(Timeslot aTimeslot)	{ m_receive = aTimeslot; }

    virtual Switch* getSwitch()	{ return 0; }

	virtual void completed(Sample *sample) 
	{ 
		m_client->completed(this, sample, sample->m_position);
	}
	// todo delete?
	virtual MediaClient* getClient() { return m_client; }		

	// debug
	virtual const char *getName() = 0;

protected:
	
	SAP m_local;
	SAP m_remote;
	Timeslot m_transmit;
	Timeslot m_receive;
    void* m_clientData;
	MediaClient* m_client;
};

#endif

/*
	phone.h    

	$Id: phone.h,v 1.22 2004/01/15 17:18:58 lars Exp $

	Copyright 1995-2001 Lars Immisch

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _PHONE_H_
#define _PHONE_H_ 

#include <string>
#include "log.h"
#include "phoneclient.h"
#include "switch.h"
#include "v3.h"
#include "exc.h"

enum { INDEFINITE = -1, INVALID_CALLREF = 0 };

class Media;
class MediaClient;
class TrunkClient;

class Trunk
{
public:

    enum TrunkCommand  { 
		t_none, 
		t_idle,
		t_connect, 
		t_disconnect,
		t_transfer, 
		t_accept 
	};

	Trunk(TrunkClient* aClient) 
		: m_client(aClient), m_cmd(t_none), m_remote_disconnect(false) {}
    virtual ~Trunk() {}

	virtual int idle() = 0;

	// Connection establishment 
	virtual int listen() = 0;
	virtual int connect(const SAP& local, const SAP& remote, unsigned timeout = INDEFINITE) = 0;
	
	// must be called by client after a t_connect_request
	// after acceptDone is called, the call is connected if the result is r_ok or else idle
	virtual int accept(unsigned callref) = 0;
	
	// transfer
	virtual int transfer(unsigned callref, const SAP& remote, unsigned timeout = INDEFINITE) 
	{ 
		return V3_ERROR_NOT_IMPLEMENTED; 
	}

	// Dissolve a connection
	virtual int disconnect(unsigned callref, int cause = 0) = 0;
		
    virtual bool hasDetails()		{ return false; }
    virtual bool needDSPSupport()	{ return false; }

    virtual Switch* getSwitch() { return 0; }
	virtual Timeslot getTimeslot()		{ return m_timeslot; }

	void setName(const char* s) { m_name = std::string(s); }
	const char* getName() { return m_name.c_str(); }

	bool remoteDisconnect() { return m_remote_disconnect; }

	virtual TrunkCommand getCommand() { return m_cmd; }

	static const char* commandName(TrunkCommand cmd)
	{
		switch (cmd)
		{
		case t_none:
			return "none";
		case t_connect:
			return "connect";
		case t_disconnect:
			return "disconnect";
		case t_transfer:
			return "transfer";
		case t_accept:
			return "accept";
		default:
			return "unknown command";
		}
	}

protected:

	volatile TrunkCommand m_cmd;
	Timeslot m_timeslot;
	TrunkClient* m_client;
	std::string m_name;
	bool m_remote_disconnect;
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

    Sample(unsigned pos = 0) : m_position(pos), 
		m_status(V3_OK), m_state(idle), m_userData(0) {}
    virtual ~Sample() {}

    virtual unsigned start(Media* aMedia) = 0;

    virtual bool stop(Media* aMedia) = 0;

    virtual unsigned getLength()	{ return 0; }
	virtual unsigned getStatus()	{ return m_status; }
	virtual bool isOutgoing()		{ return true; }

    bool setPos(unsigned aPosition)	{ m_position = aPosition; return m_position <= getLength(); }
    unsigned getPos()               { return m_position; }

    void setUserData(void* data) {	m_userData = data; }
	void* getUserData() { return m_userData; }

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

    Switch* getSwitch()	{ return 0; }

	virtual void completed(Sample *sample) 
	{ 
		m_client->completed(this, sample, sample->m_position);
	}
	MediaClient* getClient() { return m_client; }
	void setClient(MediaClient *client) { m_client = client;; }

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

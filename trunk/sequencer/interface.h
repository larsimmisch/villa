/*
	Copyright 1995 Immisch, Becker & Partner, Hamburg

	created: Mon Nov 18 12:05:02 GMT+0100 1996

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#include <map>

#include "list.h"
#include "packet.h"
#include "asynctext.h"
#include "configuration.h"

class Sequencer;

class InterfaceConnection : public AsyncText, public List::Link
{
public:

	InterfaceConnection(TextTransportClient& client, SAP& local);
	virtual ~InterfaceConnection() {}

	void add(const std::string &name, Sequencer *sequencer)
	{
		omni_mutex_lock l(m_mutex);

		m_calls[name] = sequencer;
	}

	void remove(const std::string &name)
	{
		omni_mutex_lock l(m_mutex);

		m_calls.erase(name);
	}

	Sequencer *find(const std::string &name)
	{
		omni_mutex_lock l(m_mutex);

		std::map<std::string,Sequencer*>::const_iterator i = m_calls.find(name);

		if (i == m_calls.end())
			return 0;

		return i->second;
	}

	omni_mutex& getMutex()	{ return m_mutex; }

protected:

	omni_mutex m_mutex;

	std::map<std::string,Sequencer*> m_calls;
};

class InterfaceConnections : public List
{
public:

	InterfaceConnections() {}
	virtual ~InterfaceConnections() { empty(); }

    virtual void  freeLink(Link* aLink) { delete (InterfaceConnection*)aLink; }
};

class Interface : public TextTransportClient
{
public:

	Interface(SAP& local);
	virtual ~Interface() {}

	virtual void run();

	void cleanup(TextTransport *server);

	// protocol of TransportClient
	virtual void connectRequest(TextTransport *server, SAP& remote);
	virtual void connectRequestTimeout(TextTransport *server);
	
	// replies to server.connect from far end
	virtual void connectConfirm(TextTransport *server);
	virtual void connectReject(TextTransport *server);
	virtual void connectTimeout(TextTransport *server);
	
	// must call server.disconnectAccept or server.disconnectReject 
	virtual void disconnectRequest(TextTransport *server);
	
    virtual void abort(TextTransport *server);

	// sent whenever packet is received
	virtual void dataReady(TextTransport *server) {}
 
	// flow control
	virtual void stopSending(TextTransport *server) {}
	virtual void resumeSending(TextTransport *server) {}
	
    virtual void data(TextTransport *server);

protected:

	unsigned unused;
	omni_mutex mutex;
	InterfaceConnections connections;
	SAP local;
};

// helper class
// contains all information about an outgoing call initiated by the client

class ConnectCompletion
{
public:

	ConnectCompletion(InterfaceConnection *iface, const std::string &id,
		const SAP &local, const SAP &remote, unsigned timeout)
		: m_interface(iface), m_id(id), m_local(local), m_remote(remote),
		m_timeout(timeout) {}

	~ConnectCompletion() {}

	InterfaceConnection *m_interface;
	std::string m_id;
	SAP m_local;
	SAP m_remote;
	unsigned m_timeout;
};

#endif
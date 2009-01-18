/*
	Copyright 1995-2008 ibp (uk) Ltd.

	created: Mon Nov 18 12:05:02 GMT+0100 1996

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#include <list>
#include <map>
#include <sstream>
#include "text.h"
#include "configuration.h"

class Sequencer;

class InterfaceConnection : public SocketStream
{
public:

	InterfaceConnection(int protocol = PF_INET, int s = -1) : SocketStream(protocol, s) {}
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

    int send(std::stringstream &data);
	void lost_connection();

	omni_mutex& getMutex()	{ return m_mutex; }

	typedef std::map<std::string,Sequencer*> t_calls;

	const t_calls& get_calls() { return m_calls; }

protected:

	t_calls m_calls;
    omni_mutex m_mutex;
};

class Interface
{
public:

	Interface(SAP& local);
	virtual ~Interface() {}

	virtual void run();

	// return false if connection should be closed
    bool data(InterfaceConnection *server);

protected:

	Socket m_listener;
	omni_mutex m_mutex;
	std::list<InterfaceConnection*> m_connections;
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
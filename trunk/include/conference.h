/*
	Copyright 1995-2001 Lars Immisch

	created: Thu Nov 16 15:40:24 GMT+0100 1995

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _CONFERENCE_H_
#define _CONFERENCE_H_

#include <set>
#include <map>
#include "omnithread.h"
#include "v3.h"
#include "switch.h"
#include "exc.h"
#include "acuphone.h"
#include "smclib.h"

class Conference
{
public:
	
	enum mode { 
		listen = V3_CONF_LISTEN, 
		speak = V3_CONF_SPEAK, 
		duplex = V3_CONF_DUPLEX 
	};

	virtual ~Conference() {}

	void add(ProsodyChannel *channel, mode m);
	void remove(ProsodyChannel *channel);

	void lock() 	{ m_mutex.lock(); }
	void unlock()	{ m_mutex.unlock(); }

	unsigned size()	{ return m_parties.size(); }

	unsigned getHandle()	{ return m_handle; }

	void* getUserData() 	{ return m_userData; }
	
protected:

	friend class Conferences;

	Conference(unsigned handle, void* aUserData = 0) : m_handle(handle),
		m_userData(0), m_module(0), m_closed(false), m_speakers(0), m_listeners(0) {}

	typedef std::map<ProsodyChannel*,mode> t_party_set;

	omni_mutex m_mutex;
	int m_module;
	bool m_closed;
	unsigned m_handle;
	unsigned m_speakers;
	unsigned m_listeners;
	void*	 m_userData;
	t_party_set m_parties;
};

class Conferences
{
public:

	Conferences() : m_handle(0) {}

	Conference *create(void* aUserData = 0);
	bool close(unsigned handle);

	Conference *operator[](unsigned handle);

	void lock() 	{ m_mutex.lock(); }
	void unlock()	{ m_mutex.unlock(); }

protected:

	typedef std::map<unsigned, Conference*> t_conf_map;
	typedef t_conf_map::iterator iterator;

	omni_mutex m_mutex;
	t_conf_map m_conferences;
	unsigned m_handle;
};

#endif

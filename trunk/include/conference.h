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
#include "switch.h"
#include "exc.h"
#include "acuphone.h"
#include "smclib.h"

class Conference
{
public:
	
	enum mode { listen = 0x01, speak = 0x02, background = 0x04 };

	virtual ~Conference() {}

	void add(ProsodyChannel *channel, int aMode);
	void remove(ProsodyChannel *channel);

	void lock() 	{ m_mutex.lock(); }
	void unlock()	{ m_mutex.unlock(); }

	unsigned getHandle()	{ return m_handle; }

	void* getUserData() 	{ return m_userData; }
	
protected:

	friend class Conferences;

	Conference(unsigned handle, void* aUserData = 0) : m_handle(handle),
		m_userData(0) {}

	typedef std::set<ProsodyChannel*> t_party_set;

	omni_mutex m_mutex;
	int m_module;
	unsigned m_handle;
	unsigned m_speakers;
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

/*
	Copyright 2001 Lars Immisch

	Author: Lars Immisch <lars@ibp.de>
*/

#pragma warning (disable : 4786)

#include "conference.h"

// adjusted from smclib.c

void Conference::add(ProsodyChannel *channel, mode m)
{
	lock();

	if (m_parties.find(channel) != m_parties.end())
	{
		throw Exception(__FILE__, __LINE__,
			"Conference::add", "party already in this conference");
	}

    if (m_parties.size() >= CONF_MAX_PARTY) 
	{
		throw Exception(__FILE__, __LINE__,
			"Conference::add", "conference full");
	}

	if (m & listen)
	{
		if (m_parties.begin() != m_parties.end())
		{
			ProsodyChannel *first = m_parties.begin()->first;
			channel->conferenceClone(first);
			channel->conferenceAdd(first);
		}
		else
			channel->conferenceStart();
	}

	if (m & speak)
	{
		/* 
		 * Add the new party to the other parties' low-level conferences. 
		 */
		for (t_party_set::iterator i = m_parties.begin(); i != m_parties.end(); ++i)
			i->first->conferenceAdd(channel);
 
		m_parties.insert(t_party_set::value_type(channel, m));

		channel->conferenceEC();
	}

	unlock();
}

void Conference::remove(ProsodyChannel *channel)
{
	lock();

	t_party_set::iterator p = m_parties.find(channel);

    if (p == m_parties.end())
	{
		throw Exception(__FILE__, __LINE__,
			"Conference::remove", "party not in this conference");
	}

	mode m = p->second;

	m_parties.erase(p);
    
	if (m & speak)
	{
		/* We now need to do the opposite of joining: delete the
		 * leaving party from the low-level conferences belonging to
		 * each of the remaining parties, and delete all parties from
		 * the low-level conference belonging to the leaving party.
		 * This second action is automatically performed when we abort
		 * the low-level conference, so we just have to do the first
		 * of these.
		 */

		for (t_party_set::iterator i = m_parties.begin(); i != m_parties.end(); ++i)
		{
			i->first->conferenceLeave(channel);
		}
	}

	if (m & listen)
	{
		channel->conferenceAbort();
	}

	unlock();
}

Conference *Conferences::create(void* userData)
{
	omni_mutex_lock l(m_mutex);

	++m_handle;
	if (m_handle == 0)
		m_handle = 1;

	std::pair<iterator, bool> p = 
		m_conferences.insert(std::make_pair(m_handle, (Conference*)0));

	if (p.second == false)
	{
		unsigned i = m_handle; // cache the wraparound value

		while (++m_handle != i) // search until wraparound
		{
			if (++p.first == m_conferences.end()) // iterator wraparound
				p.first = m_conferences.begin();

			if (p.first->first != m_handle) // we've found a hole
				break;
		}

		if (m_handle == i)
			throw Exception(__FILE__, __LINE__, "Conferences::create", "out of handles");
		else
		{
			p = m_conferences.insert(std::make_pair(m_handle, (Conference*)0));
			if (p.second == false)
				throw Exception(__FILE__, __LINE__, "Conferences::create", "conference insertion failed");
		}
	}

	p.first->second = new Conference(m_handle, userData);	

	return p.first->second;
}

bool Conferences::close(unsigned handle)
{
	omni_mutex_lock l(m_mutex);

	iterator h = m_conferences.find(handle);

	if (h == m_conferences.end())
		return false;

	delete(h->second);

	m_conferences.erase(h);

	return true;
}

Conference *Conferences::operator[](unsigned handle)
{
	omni_mutex_lock l(m_mutex);

	iterator h = m_conferences.find(handle);

	if (h == m_conferences.end())
		return 0;

	return h->second;
}

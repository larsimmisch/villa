/*
	Copyright 2001 Lars Immisch

	Author: Lars Immisch <lars@ibp.de>
*/

#pragma warning (disable : 4786)

#include "conference.h"

// adjusted from smclib.c

void Conference::add(ProsodyChannel *channel, mode m)
{
	omni_mutex_lock lock(m_mutex);

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
		if (m_listeners)
		{
			ProsodyChannel *first = m_parties.begin()->first;
			channel->conferenceClone(first);
			channel->conferenceAdd(first);
		}
		else
		{
			channel->conferenceStart();

			/*
			 * First listener in the conference: add all speakers
			 */
			for (t_party_set::iterator i = m_parties.begin(); i != m_parties.end(); ++i)
			{
				if (i->second & speak)
				{
					channel->conferenceAdd(i->first);
				}
			}
		}

		++m_listeners;
	}

	if (m & speak)
	{
		/* 
		 * Add the new party to the other parties' low-level conferences. 
		 */
		
		for (t_party_set::iterator i = m_parties.begin(); i != m_parties.end(); ++i)
		{
			if (i->second & listen)
			{
				i->first->conferenceAdd(channel);
			}
		}

		channel->conferenceEC();

		++m_speakers;
 	}

	m_parties.insert(t_party_set::value_type(channel, m));
}

void Conference::remove(ProsodyChannel *channel)
{
	bool closed = false;
	int parties = 0;

	/* block to restrict lock scope */
	{
		omni_mutex_lock lock(m_mutex);

		closed = m_closed;

		t_party_set::iterator p = m_parties.find(channel);

		if (p == m_parties.end())
		{
			throw Exception(__FILE__, __LINE__,
				"Conference::remove", "party not in this conference");
		}

		mode m = p->second;

		m_parties.erase(p);
    
		parties = m_parties.size();

		if (m & speak)
		{
			--m_speakers;

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
				if (i->second & listen)
				{
					i->first->conferenceLeave(channel);
				}
			}
		}

		if (m & listen)
		{
			--m_listeners;

			channel->conferenceAbort();
		}
	}

	if (closed && parties == 0)
		delete this;
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
	Conference *c = 0;
	int count = 0;

	/* block to restrict lock scope */
	{
		omni_mutex_lock l(m_mutex);

		iterator h = m_conferences.find(handle);

		if (h == m_conferences.end())
			return false;

		c = h->second;

		// remove the handle
		m_conferences.erase(h);
	}

	/* lock the conference after our mutex was unlocked */
	if (c)
	{
		c->lock();
		count = c->size();
		c->m_closed = true;
		c->unlock();

		if (count == 0)
			delete c;
	}

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

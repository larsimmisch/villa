#pragma warning (disable : 4786)

#include <utility>
#include <vector>
#include <assert.h>

#include "timer.h"

Timer::TimerID::TimerID(unsigned long delta, unsigned i,
		TimerClient *client, unsigned user)
	: m_abs_sec(0), m_abs_nsec(0), m_id(i), m_client(client), m_data(user)
{
	omni_thread::get_time(&m_abs_sec, &m_abs_nsec,
			delta / 1000, (delta % 1000) * 1000000);
}

bool Timer::TimerID::is_expired(void) const
{
	unsigned long abs_sec;
	unsigned long abs_nsec;

	omni_thread::get_time(&abs_sec, &abs_nsec);

	if (abs_sec == m_abs_sec)
	{
		return abs_nsec >= m_abs_nsec;
	}
	else
		return abs_sec >= m_abs_sec;
}

void Timer::TimerID::stop(Timer &timer)
{
	if (m_id)
	{
		timer.remove(*this);
		m_id = 0;
		m_abs_sec = 0;
		m_abs_nsec = 0;
	}
}

bool Timer::start(void)
{
	omni_mutex_lock l(m_mutex);

	if (m_active)
		return true;

	m_active = true;

	start_undetached();

	return true;
}

bool Timer::run(void)
{
	{
		omni_mutex_lock l(m_mutex);

		if (m_active)
			return true;

		m_active = true;
	}

	run_undetached(NULL);

	return true;
}

Timer::TimerID Timer::add(unsigned delta,
								   TimerClient *client,
								   unsigned user)
{
	omni_mutex_lock l(m_mutex);

	unsigned i = ++m_id_counter;

	if (!i)
		++i;

	TimerID tid(delta, i, client, user);

	std::pair<std::set<TimerID>::iterator,bool> j = m_timers.insert(tid);

	assert(j.second);

	// we must wake up the timer thread - it needs to readjust it's timedwait
	if (j.first == m_timers.begin())
		m_condition.signal();

	return tid;
}

bool Timer::remove(const TimerID& tid)
{
	if (!tid.is_valid())
		return false;

	omni_mutex_lock l(m_mutex);

	std::set<TimerID>::iterator i = m_timers.find(tid);

	if (i == m_timers.end())
		return false;

	m_timers.erase(i);

	return true;
}

void *Timer::run_undetached(void *arg)
{
	for(;;)
	{
		m_mutex.lock();

		if (!m_active)
		{
			m_mutex.unlock();

			return NULL;
		}

		// wait for the condition
		if (m_timers.size())
		{
			const TimerID& tid = *m_timers.begin();

			m_condition.timedwait(tid.m_abs_sec, tid.m_abs_nsec);
		}
		else
		{
			m_condition.wait();
		}

		if (m_timers.size() == 0)
		{
			m_mutex.unlock();

			continue;
		}

		// we want to signal the timers with the mutex unlocked to
		// avoid deadlocks, which are possible if the clients
		// use mutexes, too (which they should).

		std::vector<TimerID> due_timers;

		// pre-allocating the vector to 5 is a space/time-tradeoff.
		// I expect the average number to be one,
		// so this is classical engineering overdimensioning
		due_timers.reserve(5);

		// send all expired timers
		std::set<TimerID>::iterator i;
		for (i = m_timers.begin(); i != m_timers.end(); ++i)
		{
			if (!i->is_expired())
				break;

			due_timers.push_back(*i);
		}

		m_timers.erase(m_timers.begin(), i);

		m_mutex.unlock();

		std::vector<TimerID>::iterator j;
		for (j = due_timers.begin(); j != due_timers.end(); ++j)
		{
			j->m_client->on_timer(*j);
		}
	}
}

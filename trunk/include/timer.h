// timer.h

#ifndef TIMER_H
#define TIMER_H

#include <set>
#include <omnithread.h>

class TimerClient;

class Timer : public omni_thread
{
public:

	struct TimerID
	{
		TimerID(void) throw() :
			m_abs_sec(0), m_abs_nsec(0), m_id(0), m_client(0), m_data(0)
		{}

		TimerID(unsigned long delta, unsigned i,
			TimerClient *client, void *data);

		bool operator==(const TimerID &a) const throw()
		{
			return (m_abs_sec == a.m_abs_sec)
				&& (m_abs_nsec == a.m_abs_nsec) && (m_id == a.m_id);
		}

		bool operator<(const TimerID &a) const throw()
		{
			if (m_abs_sec == a.m_abs_sec)
			{
				if (m_abs_nsec == a.m_abs_nsec)
					return m_id < a.m_id;
				else
					return m_abs_nsec < a.m_abs_nsec;
			}
			else
				return m_abs_sec < a.m_abs_sec;
		}

		void stop(Timer &timer);

		void invalidate(void) throw() { m_id = 0; }
		bool is_valid(void) const throw() { return m_id != 0; }

		bool is_expired(void) const;

		unsigned long m_abs_sec;
		unsigned long m_abs_nsec;
		unsigned long m_id;
		TimerClient *m_client;
		void *m_data;
	};

	Timer(void) :
		m_condition(&m_mutex), m_active(false), m_id_counter(0)
	{}

	virtual ~Timer() {}

	/// returns a unique TimerID, delta is in ms
	TimerID add(unsigned delta, TimerClient *client, void *data = 0);

	/// returns true if id removed, false if id not found
	bool remove(const TimerID &id);

	/// call start once to start the timer thread
	bool start(void);

	/// call run to execute the timer thread synchronously
	bool run(void);

protected:

	virtual void *run_undetached(void *arg);

private:

	std::set<TimerID> m_timers;

	omni_mutex m_mutex;
	omni_condition m_condition;

	bool m_active;
	unsigned long m_id_counter;

private:

	// no copying or assignment
	Timer(const Timer&);
	Timer &operator=(const Timer&);
};

class TimerClient
{
public:

	virtual void on_timer(const Timer::TimerID &id) = 0;
};

#endif // AWW_TIMER_H

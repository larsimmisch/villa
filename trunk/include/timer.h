// Timer.h

#ifndef TIMER_H_
#define TIMER_H_

#pragma warning (disable : 4786)

#define _WIN32_WINNT 0x0400

#define INCL_DOSDATETIME
#include <windows.h>

#include <set>

class TimerClient;

class Timer : public omni_thread
{
public:

	struct TimerID
	{
		TimerID() : id(0), removed(false) {}
		TimerID(const LARGE_INTEGER& t, unsigned i, TimerClient *client, void *userData) : time(t), id(i), session(s), removed(false) {}

		bool operator==(const TimerID& a) const 
		{ 
			return (time.QuadPart == a.time.QuadPart) && (id == a.id);
		} 
		bool operator<(const TimerID& a) const 
		{ 
			if (time.QuadPart == a.time.QuadPart)
				return id < a.id;
			else
				return time.QuadPart < a.time.QuadPart;
		}

		void invalidate() { id = 0; }
		bool isValid() { return id != 0; }

		LARGE_INTEGER time;
		TimerClient* client;
		void *data;
		unsigned long id;
		bool removed;
	};

	Timer();
	virtual ~Timer();

	// returns a unique Timer ID
	TimerID add(unsigned delta, TimerClient *client, void *userData);

	// returns true if id removed, false if id not found
	bool remove(const TimerID& id);

protected:

	void set(const TimerID& timer);
	void stop();
	void wait();

	std::set<TimerID> m_Timers;

	HANDLE m_Handle;
	bool m_Terminated;	// needed for thread termination workaround
	unsigned long m_IDCounter;
	unsigned m_TID;
	omni_mutex mutex;

private:

	// no copying or assignment
	Timer(const Timer&);
	Timer& operator=(const Timer&);
};

class TimerClient
{
public:

	virtual void onTimer(const Timer::TimerID id, void* userData) = 0;
};

#endif

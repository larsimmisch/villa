// timer.cpp

#include "timer.h"

#include <stdio.h>
#include <assert.h>
#include <process.h>

using namespace std;

// this is yucky. I use the undocumented _assert function because
// the proper "assert" macro is only evaluated if NDEBUG is undefined.

extern "C" _CRTIMP void __cdecl _assert(void *, void *, unsigned);

_CRTIMP void __cdecl _assert(void *, void *, unsigned);

#define assert__(exp) (void)( (exp) || (_assert(#exp, __FILE__, __LINE__), 0) )

class CSLock
{
public:

	CSLock(CRITICAL_SECTION* cs) : m_CS(cs) { EnterCriticalSection(m_CS); }
	~CSLock() { LeaveCriticalSection(m_CS); }

protected:

	CRITICAL_SECTION* m_CS;
};

static unsigned __stdcall threadAction(void* arg)
{
    T_CAsyncTimer* asyncTimer = (T_CAsyncTimer*)arg;

	asyncTimer->Run();

	return 0;
}

T_CAsyncTimer::TimerID::TimerID(CTkvs_ct kvs) : id(0), removed(false)
{
	CTerror error;

	if (CTkvs_GetUInt(kvs, Message_ECTF_EventQualifier, &id, &error) != CT_statusOK)
		return;

	if (CTkvs_GetUInt(kvs, ASR_ECTF_InitialTimeout, &time.LowPart, &error) != CT_statusOK)
		return;

	if (CTkvs_GetInt(kvs, ASR_ECTF_FinalTimeout, &time.HighPart, &error) != CT_statusOK)
		return;
}

CTstatus T_CAsyncTimer::TimerID::WriteEventData(CTkvs_ct kvs, CTerror* error)
{
	CTstatus status = CTkvs_PutUInt(kvs, Message_ECTF_EventQualifier, id, error);
	if (status != CT_statusOK)
		return status;

	status = CTkvs_PutUInt(kvs, ASR_ECTF_InitialTimeout, time.LowPart, error);
	if (status != CT_statusOK)
		return status;

	status = CTkvs_PutInt(kvs, ASR_ECTF_FinalTimeout, time.HighPart, error);

	return status;
}

T_CAsyncTimer::T_CAsyncTimer() : m_Active(false), m_Terminated(false), m_IDCounter(0)
{
	InitializeCriticalSection(&m_CS);
}

T_CAsyncTimer::~T_CAsyncTimer()
{
	if (m_Active)
	{
		m_Active = false;

		// signal the timer as soon as possible
		LARGE_INTEGER t;

		t.QuadPart = -1;

		assert__(SetWaitableTimer(m_Handle, &t, 0, NULL, NULL, FALSE));

		if (m_TID != GetCurrentThreadId())
		{
			// The following line may deadlock (which is bizarre)
			// assert__(WaitForSingleObject(m_Thread, INFINITE) == WAIT_OBJECT_0);

			// so we poll instead
			EnterCriticalSection(&m_CS);

			while(!m_Terminated)
			{
				LeaveCriticalSection(&m_CS);

				Sleep(20);

				EnterCriticalSection(&m_CS);
			}

			LeaveCriticalSection(&m_CS);
		}

		CancelWaitableTimer(m_Handle);

		CloseHandle(m_Handle);
		CloseHandle(m_Thread);

		CTerror error;

		CT_Shutdown(&error);
	}

	DeleteCriticalSection(&m_CS);
}

bool T_CAsyncTimer::Start()
{
	if (m_Active)
		return true;

	CTerror error;

	CT_Initialize(&error);

	if (error != CT_errorOK)
		return false;

	m_Handle = CreateWaitableTimer(NULL, FALSE, NULL);
	if (!m_Handle)
	{
		CT_Shutdown(&error);
		return false;
	}

	m_Active = true;

	m_Thread = (void*)_beginthreadex(NULL, 0, threadAction, this, 0 /* running */, &m_TID);
	if (!m_Thread)
	{
		CT_Shutdown(&error);
		CloseHandle(m_Handle);
		m_Active = false;
	}

	return m_Active;
}

void T_CAsyncTimer::Set(const TimerID &timer)
{
	assert__(SetWaitableTimer(m_Handle, &timer.time, 0, NULL, NULL, FALSE));
}

void T_CAsyncTimer::Stop()
{
    CancelWaitableTimer(m_Handle);
}

void T_CAsyncTimer::Wait()
{
	assert__(WaitForSingleObject(m_Handle, INFINITE) == WAIT_OBJECT_0);
}

T_CAsyncTimer::TimerID T_CAsyncTimer::Add(unsigned delta, CTses_ct session)
{
	SYSTEMTIME s;
	FILETIME f;

	CSLock lock(&m_CS);

	GetSystemTime(&s);

	assert__(SystemTimeToFileTime(&s, &f));

	LARGE_INTEGER t;

	memcpy(&t, &f, sizeof(t));

	ULONGLONG d = delta;

	d *= 10000;

	t.QuadPart += d;

	unsigned id = ++m_IDCounter;

	if (!id)
		++id;

	TimerID tid(t, id, session);

	pair<set<TimerID>::iterator,bool> i = m_Timers.insert(tid);

	assert__(i.second);

	if (i.first == m_Timers.begin())
		Set(tid);

	return tid;
}

bool T_CAsyncTimer::Remove(const TimerID& id)
{
	CSLock lock(&m_CS);

	set<TimerID>::iterator i = m_Timers.find(id);

	if (i == m_Timers.end() || i->removed)
		return false;

	// we cannot safely remove the due timer (it may already be signalled),
	// so we have to mark it as removed

	if (i == m_Timers.begin())
	{
		i->removed = true;
	}
	else
	{
		m_Timers.erase(i);
	}

	return true;
}

void T_CAsyncTimer::Run()
{
	CTerror error;
	CTkvs_ct eventData, event;
	CTtranInfo tranInfo;

	assert__(CTkvs_Create(&eventData, &error) == CT_statusOK);
	assert__(CTkvs_Create(&event, &error) == CT_statusOK);

	CTtran_Initialize(tranInfo, eventData);

	assert__(CTkvs_PutSymbol(event, Message_ECTF_EventID, ISDN_DTB_TimerExpiry, &error) == CT_statusOK);

	for(;;)
	{
		// we lock the CriticalSection before we read m_Active.
		// this is conservative and assumes pthread mutex semantics
		{
			CSLock lock(&m_CS);

			if (!m_Active)
			{
				CTkvs_Destroy(eventData, &error);
				CTkvs_Destroy(event, &error);

				m_Terminated = true;

				return;
			}
		}

		Wait();

		{
			CSLock lock(&m_CS);

			if (m_Timers.size() == 0)
			{
				continue;
			}

			LARGE_INTEGER t;
			
			// in this loop, we send all timers with the same time
			for (set<TimerID>::iterator i = m_Timers.begin(); i != m_Timers.end(); ++i)
			{
				if (i->removed)
					continue;

				// break out if no timer with the same time can be found
				if (i != m_Timers.begin() && i->time.QuadPart > t.QuadPart)
					break;

				t = i->time;

				CTerror error;

				// signal timer to application
				assert__(i->WriteEventData(event, &error) == CT_statusOK);

				assert__(CTses_PutEvent(i->session, event, &tranInfo) == CT_statusOK);
			}

			m_Timers.erase(m_Timers.begin(), i);

			i = m_Timers.begin();
			if (i != m_Timers.end())
				Set(*i);
		}
	}
}

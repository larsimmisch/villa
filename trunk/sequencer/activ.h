/*
	activ.h
*/

#ifndef _ACTIV_H_
#define _ACTIV_H_

#pragma warning (disable : 4786)

#include "omnithread.h"
#include "list.h"
#include "timer.h"
#include "conference.h"
#include "phone.h"
#include "v3.h"

#define NOTIFY_START 0x01
#define NOTIFY_STOP 0x02

// the model here is strictly asynchronous. if an atom/molecule is to be stopped, stop is called,
// but succesful stopping is expected to be signalled by a done with completed 0
// if new Atoms are synchronously stoppable, they must call Sequencer::completed(...)

char* copyString(const char* aString);

class Sequencer;

struct Time
{
	Time() : m_sec(0), m_nsec(0) {}

	void now() { omni_thread::get_time(&m_sec, &m_nsec); }
	
	unsigned long m_sec;
	unsigned long m_nsec;

	unsigned operator-(const Time &b)
	{
		return (m_sec - b.m_sec) * 1000 - (m_nsec - b.m_nsec) / 1000000;
	}
};

class Atom : public DList::DLink
{
public:

	Atom(unsigned channel = 0) : m_sample(0), m_channel(channel), m_notifications(0) {}
	virtual ~Atom() {}

	/* channel is the index of the parallel job */
	virtual bool start(Sequencer* sequencer);
	virtual bool stop(Sequencer* sequencer);

	virtual bool done(Sequencer* sequencer, unsigned msecs, unsigned reason) { return true; }
	virtual bool setPos(unsigned aPosition) { return true; }
	virtual unsigned getLength()	{ return 0; }
	virtual unsigned getStatus()	{ return V3_OK; }

	virtual bool isGrowing() = 0;

	void setNotifications(int notify)	{ m_notifications = notify; }

	unsigned getChannel() { return m_channel; }

	bool notifyStart() const { return (m_notifications & NOTIFY_START) != 0; }
	bool notifyStop() const { return (m_notifications & NOTIFY_STOP) != 0; }

	virtual void printOn(std::ostream& out) = 0;

	// if a subclass does not use m_sample, it must override these methods
	virtual void setUserData(void *data) { m_sample->setUserData(data); }
	virtual void *getUserData() { return m_sample->getUserData(); }

protected:

	Sample *m_sample;
	unsigned m_channel;
	unsigned m_notifications;
};

class Molecule : public Atom, public DList
{
public:

	enum a_mode 
	{ 
		discard = V3_MODE_DISCARD, 
		pause = V3_MODE_PAUSE, 
		mute = V3_MODE_MUTE, 
		restart = V3_MODE_RESTART, 
		dont_interrupt = V3_MODE_DONT_INTERRUPT, 
		loop = V3_MODE_LOOP 
	};

	enum a_flags 
	{ 
		active = 0x01, 
		need_rewind = 0x02, 
		stopped = 0x04 
	};

	Molecule(unsigned channel, unsigned mode, int aPriority, const std::string &id);
	virtual ~Molecule();	

	virtual bool start(Sequencer* sequencer);
	virtual bool stop(Sequencer* sequencer);
	virtual bool done(Sequencer* sequencer, unsigned msecs, unsigned reason);
	virtual bool setPos(unsigned aPosition);
	unsigned getPos() const { return m_pos; }
	virtual unsigned getLength() const { return m_length; }
	virtual bool isGrowing() { return false; }

	// atEnd is slightly different than done. 
	// atEnd returns true at the end of a looped sample, which done doesn't
	bool atEnd() const { return m_current == tail; }

	unsigned getPriority() const { return m_priority; }

	void setMode(unsigned mode) { m_mode = mode; }
	unsigned getMode() const	 { return m_mode;  } 
	unsigned getStatus() const	 { return m_status; } 

	void add(Atom& atom)		 { addLast(&atom); m_length += atom.getLength(); }
	void remove(Atom* atom)	 { m_length -= atom->getLength(); DList::remove(atom); }

	bool isActive() const { return (m_flags & active) != 0; }
	bool needRewind() const { return (m_flags & need_rewind) != 0; }
	bool isStopped() const { return (m_flags & stopped) != 0; }

	const char *getId() { return m_id.c_str(); }

	bool notifyStart() const	 { return m_current ? m_current->notifyStart() : false; }
	bool notifyStop() const { return m_current ? m_current->notifyStop() : false;	}

	unsigned currentAtom() const { return m_nCurrent; }

	virtual void printOn(std::ostream& out);
	
protected:

	friend std::ostream& operator<<(std::ostream& out, Molecule& aMolecule);

	virtual void freeLink(List::Link* aLink);

	unsigned m_mode;
	unsigned m_priority;
	std::string m_id;
	unsigned m_flags;
	Time m_timeStarted;
	Time m_timeStopped;
	unsigned m_pos;
	unsigned m_length;
	unsigned m_status;
	unsigned m_nCurrent;
	Atom* m_current;
};

class PlayAtom : public Atom
{
public:

	PlayAtom(unsigned channel, Sequencer* sequencer, const char* aFile);
	virtual ~PlayAtom() { delete m_file; delete m_sample; }

	virtual bool setPos(unsigned pos) { return m_sample->setPos(pos); }
	virtual unsigned getLength()	{ return m_sample->getLength(); }
	virtual bool isGrowing() { return false; }

	virtual void printOn(std::ostream& out)  { out << "PlayAtom(" << m_file << ")"; }
	
protected:
	
	char* m_file;
};

class RecordAtom : public Atom
{
public:

	RecordAtom(unsigned channel, Sequencer* sequencer, const char* aFile, unsigned aTime);
	virtual ~RecordAtom() { delete m_file; delete m_sample; }

	virtual bool setPos(unsigned pos) { return m_sample->setPos(pos); }
	virtual unsigned getLength()	{ return m_sample->getLength(); }
	virtual unsigned getStatus()	{ return m_sample->getLength() == 0 ? V3_WARNING_EMPTY : V3_OK; }
	virtual bool isGrowing() { return true; }

	virtual void printOn(std::ostream& out)  { out << "RecordAtom(" << m_file << ", " << time << ')'; }

protected:

	char *m_file;
	unsigned m_time;
};

class BeepAtom : public Atom
{
public:

	BeepAtom(unsigned channel, Sequencer* sequencer, unsigned count);
	virtual ~BeepAtom() { delete m_sample; }

	virtual bool setPos(unsigned pos) { return true; }
	virtual unsigned getLength()	{ return m_nBeeps * 250; } // todo fix this
	virtual unsigned getStatus()	{ return V3_OK; }
	virtual bool isGrowing()		{ return false; }

	virtual void printOn(std::ostream& out)	{ out << "BeepAtom(" << m_nBeeps << ')'; }

protected:

	unsigned m_nBeeps;
};

class TouchtoneAtom : public Atom
{
public:

	TouchtoneAtom(unsigned channel, Sequencer* sequencer, const char* att);
	virtual ~TouchtoneAtom() { delete m_sample; delete m_tt; }

	virtual bool setPos(unsigned pos) { return true; }
	virtual unsigned getLength()	{ return strlen(m_tt) * 80; } // this is a rough guess
	virtual unsigned getStatus()	{ return m_sample->getStatus(); }
	virtual bool isGrowing()		{ return false; }

	virtual void printOn(std::ostream& out)	{ out << "TouchtoneAtom(" << m_tt << ')'; }

protected:

	char *m_tt;
};

class SilenceAtom: public Atom, public TimerClient
{
public:

	// time is in milliseconds, as always
	SilenceAtom(unsigned channel, unsigned aLength) : Atom(channel), m_length(aLength) {}
	virtual ~SilenceAtom() {}

	virtual bool start(Sequencer* sequencer);
	virtual bool stop(Sequencer* sequencer);
	virtual bool setPos(unsigned pos);
	virtual unsigned getLength()	{ return m_length; } 
	virtual unsigned getStatus()	{ return V3_OK; }
	virtual bool isGrowing()		{ return false; }

	virtual void printOn(std::ostream& out)	{ out << "SilenceAtom(" << m_length << ')'; }

	// protocol of TimerClient
	virtual void on_timer(const Timer::TimerID &id);

	virtual void setUserData(void *data) { m_timer.m_data = data; }
	virtual void *getUserData() { return m_timer.m_data; }

protected:

	Timer::TimerID m_timer;
	Sequencer *m_seq;
	unsigned m_length;
	unsigned m_pos;
};

class ConferenceAtom : public Atom //, public Termination
{
public:

	ConferenceAtom(unsigned channel, unsigned handle, Conference::mode m);
	virtual ~ConferenceAtom() {}

	virtual bool start(Sequencer* sequencer);
	virtual bool stop(Sequencer* sequencer);
	virtual bool setPos(unsigned pos) { return true; }
	virtual unsigned getLength()	{ return INDEFINITE; }
	virtual unsigned getStatus()	{ return V3_OK; }
	virtual bool isGrowing()		{ return false; }

	virtual void printOn(std::ostream& out)	{ out << "ConferenceAtom()"; }

	virtual void setUserData(void *data) { m_data = data; }
	virtual void *getUserData() { return m_data; }

protected:

	Conference *m_conference;
	void *m_data;
	Time m_started;
	Conference::mode m_mode;
};

class Activity : public DList
{
public:

	enum e_state { idle, active, stopping };

	Activity(Sequencer* sequencer = 0) : m_sequencer(sequencer), m_state(idle) {}
	virtual ~Activity();

	virtual Molecule* add(Molecule& newMolecule);
	virtual void remove(Molecule* aMolecule);

	virtual bool start();
	virtual bool stop();

	virtual bool abort();

	virtual Molecule* find(const std::string &id);

	e_state getState() const { return m_state; }
	void setState(e_state s) { m_state = s; }

	e_state m_state;
	Sequencer* m_sequencer;

protected:

	virtual void freeLink(List::Link* anAtom);

};

class ActivityIter : public ListIter
{
public:

	ActivityIter(Activity& aList) : ListIter(aList) {}

	Molecule* current()   { return (Molecule*)ListIter::current(); }
};

#endif

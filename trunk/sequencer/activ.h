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
#include "rphone.h"

// the model here is strictly asynchronous. if an atom/molecule is to be stopped, stop is called,
// but succesful stopping is expected to be signalled by a done with completed 0
// if new Atoms are synchronously stoppable, they must call Sequencer::completed(...)

char* copyString(const char* aString);

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

class Sequencer;

class Atom : public DList::DLink
{
public:

	Atom() {}
	virtual ~Atom() {}

	virtual int start(Sequencer* sequencer, void* userData = 0);

	virtual int stop(Sequencer* sequencer);

	virtual int done(Sequencer* sequencer, unsigned msecs, unsigned reason) { return 1; }
	virtual int setPos(unsigned aPosition) { return 1; }
	virtual unsigned getLength()	{ return 0; }
	virtual unsigned getStatus()	{ return PHONE_OK; }

	virtual int isGrowing() { return 0; }

	void setNotifications(int notify)	{ m_notifications = notify; }

	int notifyStart() const { return m_notifications & notify_start; }
	int notifyStop() const { return m_notifications & notify_stop; }

	virtual void printOn(std::ostream& out)  { out << "abstract atom"; }

protected:

	Sample *m_sample;

	unsigned m_notifications;
};

class Molecule : public Atom, public DList
{
public:

	enum a_mode 
	{ 
		discard = 0x01, 
		pause = 0x02, 
		mute = 0x04, 
		restart = 0x08, 
		dont_interrupt = 0x10, 
		loop = 0x20 
	};

	enum a_flags 
	{ 
		active = 0x01, 
		need_rewind = 0x02, 
		stopped = 0x04 
	};

	Molecule(unsigned mode, int aPriority, const std::string &id, const std::string& jobid);
	virtual ~Molecule();	

	virtual int start(Sequencer* sequencer, void* userData = 0);
	virtual int stop(Sequencer* sequencer);
	virtual int done(Sequencer* sequencer, unsigned msecs, unsigned reason);
	virtual int setPos(unsigned aPosition);
	unsigned getPos() const { return m_pos; }
	virtual unsigned getLength() const { return m_length; }

	// atEnd is slightly different than done. 
	// atEnd returns true at the end of a looped sample, which done doesn't
	int atEnd() const { return m_current == tail; }

	unsigned getPriority() const { return m_priority; }

	void setMode(unsigned mode) { m_mode = mode; }
	unsigned getMode() const	 { return m_mode;  } 
	unsigned getStatus() const	 { return m_status; } 

	void add(Atom& atom)		 { addLast(&atom); m_length += atom.getLength(); }
	void remove(Atom* atom)	 { m_length -= atom->getLength(); DList::remove(atom); }

	int isActive() const { return m_flags & active; }
	int needRewind() const { return m_flags & need_rewind; }
	int isStopped() const { return m_flags & stopped; }

	const char *getId() { return m_id.c_str(); }
	const char *getJobId() { return m_jobid.c_str(); }

	int notifyStart() const	 { return m_current ? m_current->notifyStart() : 0; }
	int notifyStop() const { return m_current ? m_current->notifyStop() : 0;	}

	unsigned currentAtom() const { return m_nCurrent; }
	
protected:

	friend std::ostream& operator<<(std::ostream& out, Molecule& aMolecule);

	virtual void freeLink(List::Link* aLink);

	unsigned m_mode;
	unsigned m_priority;
	std::string m_id;
	std::string m_jobid;
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

	PlayAtom(Sequencer* sequencer, const char* aFile);
	virtual ~PlayAtom() { delete m_file; delete m_sample; }

	virtual int setPos(unsigned pos) { return m_sample->setPos(pos); }
	virtual unsigned getLength()	{ return m_sample->getLength(); }

	virtual void printOn(std::ostream& out)  { out << "PlayAtom(" << m_file << ")"; }
	
protected:
	
	char* m_file;
};

class RecordAtom : public Atom
{
public:

	RecordAtom(Sequencer* sequencer, const char* aFile, unsigned aTime);
	virtual ~RecordAtom() { delete m_file; delete m_sample; }

	virtual int setPos(unsigned pos) { return m_sample->setPos(pos); }
	virtual unsigned getLength()	{ return m_sample->getLength(); }
	virtual unsigned getStatus()	{ return m_sample->getLength() == 0 ? PHONE_WARNING_EMPTY : PHONE_OK; }

	virtual int isGrowing() { return 1; }

	virtual void printOn(std::ostream& out)  { out << "RecordAtom(" << m_file << ", " << time << ')'; }

protected:

	char *m_file;
	unsigned m_time;
};

class BeepAtom : public Atom
{
public:

	BeepAtom(Sequencer* sequencer, unsigned count);
	virtual ~BeepAtom() { delete m_sample; }

	virtual int setPos(unsigned pos) { return 1; }
	virtual unsigned getLength()	{ return m_nBeeps * 250; } // todo fix this
	virtual unsigned getStatus()	{ return PHONE_OK; }

	virtual void printOn(std::ostream& out)	{ out << "BeepAtom(" << m_nBeeps << ')'; }

protected:

	unsigned m_nBeeps;
};

class TouchtoneAtom : public Atom
{
public:

	TouchtoneAtom(Sequencer* sequencer, const char* att);
	virtual ~TouchtoneAtom() { delete m_sample; delete m_tt; }

	virtual int setPos(unsigned pos) { return 1; }
	virtual unsigned getLength()	{ return strlen(m_tt) * 80; } // this is a rough guess
	virtual unsigned getStatus()	{ return m_sample->getStatus(); }

	virtual void printOn(std::ostream& out)	{ out << "TouchtoneAtom(" << m_tt << ')'; }

protected:

	char *m_tt;
};

class SilenceAtom: public Atom, public TimerClient
{
public:

	// time is in milliseconds, as always
	SilenceAtom(unsigned aLength) : m_length(aLength) {}
	virtual ~SilenceAtom() {}

	virtual int start(Sequencer* sequencer, void* userData = 0);
	virtual int stop(Sequencer* sequencer);
	virtual int setPos(unsigned pos);
	virtual unsigned getLength()	{ return m_length; } 
	virtual unsigned getStatus()	{ return PHONE_OK; }

	virtual void printOn(std::ostream& out)	{ out << "SilenceAtom(" << m_length << ')'; }

	// protocol of TimerClient
	virtual void on_timer(const Timer::TimerID &id);

protected:

	Timer::TimerID m_timer;
	Sequencer *m_seq;
	unsigned m_length;
	unsigned m_pos;
};

class ConferenceAtom : public Atom //, public Termination
{
public:

	ConferenceAtom(unsigned aConf, unsigned aMode);
	virtual ~ConferenceAtom() {}

	virtual int start(Sequencer* sequencer, void* userData = 0);
	virtual int stop(Sequencer* sequencer);
	virtual int setPos(unsigned pos) { return 1; }
	virtual unsigned getLength()	{ return INDEFINITE; }
	virtual unsigned getStatus()	{ return PHONE_OK; }

	virtual void printOn(std::ostream& out)	{ out << "ConferenceAtom()"; }

protected:

	Conference *m_conference;
	void *m_userData;
	Time m_started;
	unsigned m_mode;
};

class Activity : public DList
{
public:

	enum e_state { idle, active, stopping };

	Activity(Sequencer* sequencer) : m_sequencer(sequencer), m_state(idle) {}
	virtual ~Activity();

	virtual Molecule* add(Molecule& newMolecule);
	virtual void remove(Molecule* aMolecule);

	virtual int start();
	virtual int stop();

	virtual Molecule* find(const std::string &id);

	e_state getState() const { return m_state; }
	void setState(e_state s) { m_state = s; }

protected:

	virtual void freeLink(List::Link* anAtom);

	e_state m_state;
	Sequencer* m_sequencer;

};

class ActivityIter : public ListIter
{
public:

	ActivityIter(Activity& aList) : ListIter(aList) {}

	Molecule* current()   { return (Molecule*)ListIter::current(); }
};

#endif

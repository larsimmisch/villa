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
	Time() : sec(0), nsec(0) {}

	void now() { omni_thread::get_time(&sec, &nsec); }
	
	unsigned long sec;
	unsigned long nsec;

	unsigned operator-(const Time &b)
	{
		return (sec - b.sec) * 1000 - (nsec - b.nsec) / 1000000;
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
	virtual unsigned getStatus()	{ return _ok; }

	virtual int isGrowing() { return 0; }

	void setNotifications(int notify)	{ notifications = notify; }

	int notifyStart() const { return notifications & notify_start; }
	int notifyStop() const { return notifications & notify_done; }

	virtual void printOn(std::ostream& out)  { out << "abstract atom"; }

protected:

	Sample *sample;

	unsigned notifications;
};

class Molecule : public Atom, public DList
{
public:

	enum a_mode { discard = 0x01, mute = 0x02, mix = 0x04, dont_interrupt = 0x08, restart = 0x10, pause = 0x20, loop = 0x40 };

	enum a_flags { active = 0x01, need_rewind = 0x02, stopped = 0x04 };

	Molecule(unsigned mode, int aPriority, int aSyncMinor);
	virtual ~Molecule();	

	virtual int start(Sequencer* sequencer, void* userData = 0);
	virtual int stop(Sequencer* sequencer);
	virtual int done(Sequencer* sequencer, unsigned msecs, unsigned reason);
	virtual int setPos(unsigned aPosition);
	unsigned getPos() const { return pos; }
	virtual unsigned getLength() const { return length; }

	// atEnd is slightly different than done. 
	// atEnd returns true at the end of a looped sample, which done doesn't
	int atEnd() const { return current == tail; }

	unsigned getPriority() const { return priority; }

	void setMode(unsigned aMode) { mode = aMode; }
	unsigned getMode() const	 { return mode;  } 
	unsigned getStatus() const	 { return status; } 

	void add(Atom& anAtom)		 { addLast(&anAtom); length += anAtom.getLength(); }
	void remove(Atom* anAtom)	 { length -= anAtom->getLength(); DList::remove(anAtom); }

	int isActive() const { return flags & active; }
	int needRewind() const { return flags & need_rewind; }
	int isStopped() const { return flags & stopped; }

	unsigned getSyncMinor() const  { return syncMinor; }

	int notifyStart() const	 { return current ? current->notifyStart() : 0; }
	int notifyStop() const { return current ? current->notifyStop() : 0;	}

	unsigned currentAtom() const { return nCurrent; }
	
protected:

	friend std::ostream& operator<<(std::ostream& out, Molecule& aMolecule);

	virtual void freeLink(List::Link* aLink);

	unsigned mode;
	unsigned priority;
	unsigned syncMinor;
	unsigned flags;
	Time timeStarted;
	Time timeStopped;
	unsigned pos;
	unsigned length;
	unsigned status;
	unsigned nCurrent;
	Atom* current;
};

class PlayAtom : public Atom
{
public:

	PlayAtom(Sequencer* sequencer, const char* aFile);
	virtual ~PlayAtom() { delete file; delete sample; }

	virtual int setPos(unsigned pos) { return sample->setPos(pos); }
	virtual unsigned getLength()	{ return sample->getLength(); }

	virtual void printOn(std::ostream& out)  { out << "PlayAtom(" << file << ")"; }
	
protected:
	
	char* file;
};

class RecordAtom : public Atom
{
public:

	RecordAtom(Sequencer* sequencer, const char* aFile, unsigned aTime);
	virtual ~RecordAtom() { delete file; delete sample; }

	virtual int setPos(unsigned pos) { return sample->setPos(pos); }
	virtual unsigned getLength()	{ return sample->getLength(); }
	virtual unsigned getStatus()	{ return sample->getLength() == 0 ? _empty : _ok; }

	virtual int isGrowing() { return 1; }

	virtual void printOn(std::ostream& out)  { out << "RecordAtom(" << file << ", " << time << ')'; }

protected:

	char *file;
	unsigned time;
};

class BeepAtom : public Atom
{
public:

	BeepAtom(Sequencer* sequencer, unsigned count);
	virtual ~BeepAtom() { delete sample; }

	virtual int setPos(unsigned pos) { return 1; }
	virtual unsigned getLength()	{ return nBeeps * 250; } // todo fix this
	virtual unsigned getStatus()	{ return _ok; }

	virtual void printOn(std::ostream& out)	{ out << "BeepAtom(" << nBeeps << ')'; }

protected:

	unsigned nBeeps;
};

class TouchtoneAtom : public Atom
{
public:

	TouchtoneAtom(Sequencer* sequencer, const char* att);
	virtual ~TouchtoneAtom() { delete sample; delete tt; }

	virtual int setPos(unsigned pos) { return 1; }
	virtual unsigned getLength()	{ return strlen(tt) * 80; } // this is a rough guess
	virtual unsigned getStatus()	{ return sample->getStatus(); }

	virtual void printOn(std::ostream& out)	{ out << "TouchtoneAtom(" << tt << ')'; }

protected:

	char *tt;
};

class SilenceAtom: public Atom, public TimerClient
{
public:

	// time is in milliseconds, as always
	SilenceAtom(unsigned aLength) : length(aLength) {}
	virtual ~SilenceAtom() {}

	virtual int start(Sequencer* sequencer, void* userData = 0);
	virtual int stop(Sequencer* sequencer);
	virtual int setPos(unsigned pos);
	virtual unsigned getLength()	{ return length; } 
	virtual unsigned getStatus()	{ return _ok; }

	virtual void printOn(std::ostream& out)	{ out << "SilenceAtom(" << length << ')'; }

	// protocol of TimerClient
	virtual void on_timer(const Timer::TimerID &id);

protected:

	Timer::TimerID timer;
	Sequencer *seq;
	unsigned length;
	unsigned pos;
};

class ConferenceAtom : public Atom //, public Termination
{
public:

	ConferenceAtom(unsigned aConf, unsigned aMode);
	virtual ~ConferenceAtom();

	virtual int start(Sequencer* sequencer, void* userData = 0);
	virtual int stop(Sequencer* sequencer);
	virtual int setPos(unsigned pos) { return 1; }
	virtual unsigned getLength()	{ return indefinite; }
	virtual unsigned getStatus()	{ return _ok; }

	virtual void printOn(std::ostream& out)	{ out << "ConferenceAtom()"; }

protected:

	virtual void terminate();

	Conference *conf;
	void *userData;
	Conference::Member* me;
	Time started;
	unsigned mode;
};

class Activity : public DList, public DList::DLink
{
public:

	enum { active = 0x01, discarded = 0x02, idle = 0x04, disabled = 0x08, asap = 0x10 };

	Activity(unsigned aSyncMajor, Sequencer* sequencer);
	virtual ~Activity();

	virtual Molecule* add(Molecule& newMolecule);
	virtual void remove(Molecule* aMolecule);

	virtual int start();
	virtual int stop();

	virtual Molecule* find(unsigned syncMinor);

	unsigned getSyncMajor()  { return syncMajor; }
	int isActive()		{ return flags & active; }
	int isDiscarded()	{ return flags & discarded; }
	int isDisabled()	{ return flags & disabled; }
	int isASAP()		{ return flags & asap; }
	int isIdle()		{ return flags & idle; }

	void setDiscarded() { flags |= discarded; }
	void setDisabled()	{ flags |= disabled; }
	void unsetDisabled(){ flags &= ~disabled; }
	void setASAP()		{ flags |= asap; }
	void unsetASAP()	{ flags &= ~asap; }

protected:

	virtual void freeLink(List::Link* anAtom);

	unsigned syncMajor;
	unsigned flags;
	Sequencer* sequencer;

};

class ActivityIter : public ListIter
{
public:

	ActivityIter(Activity& aList) : ListIter(aList) {}

	Molecule* current()   { return (Molecule*)ListIter::current(); }
};

class ActivityCollection : public DList
{
public:

	ActivityCollection();
	virtual ~ActivityCollection();

	virtual void add(Activity& anActivity);
	virtual void remove(Activity* anActivity);

	void lock() 	{ mutex.lock(); }
	void unlock()	{ mutex.unlock(); }

	Activity* find(unsigned syncMajor);

	int empty();

protected:

	virtual void freeLink(List::Link* anActivity);

	omni_mutex mutex;
};

#endif

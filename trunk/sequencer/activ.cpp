/*
    activ.cpp
*/

// this is a comment

#ifdef _WIN32
#include <windows.h>
#else
#define INCL_BASE
#include <os2.h>
#endif

#include "log.h"
#include "activ.h"
#include "sequence.h"

extern int debug;

extern Log log;

#ifdef __AG__
extern Conferences gConferences;
#endif

char* copyString(const char* aString)
{
    char* s = new char[strlen(aString) +1];
    strcpy(s, aString);

    return s;
}

PlayAtom::PlayAtom(Sequencer* sequencer, const char* aFile)
{ 
	file = copyString(aFile);
	sample = sequencer->getPhone()->createFileSample(aFile)
}

int PlayAtom::start(Sequencer* sequencer, void* userData)
{
    sample->setUserData(userData);

	sequencer->getPhone()->send(sample);

    return 1;
}

int PlayAtom::stop(Sequencer* sequencer)
{
    return sample.stop(sequencer->getPhone());
}

 
RecordAtom::RecordAtom(Sequencer* sequencer, const char* aFile, unsigned aTime, int aMessage)
	: time(aTime)
{
	file = copyString(aFile); 

	sample = sequencer->getPhone()->createRecordFileSample(aFile, aTime);
}

int RecordAtom::start(Sequencer* sequencer, void* userData)
{
    sample->setUserData(userData);

	sequencer->getPhone()->send(sample);

    return 1;
}

int RecordAtom::stop(Sequencer* sequencer)
{
    return sample->stop(sequencer->getPhone());
}

BeepAtom::BeepAtom(Sequencer* sequencer, unsigned count) : nBeeps(count) 
{
	beep = sequencer->getPhone()->createBeeps(count);
}

int BeepAtom::start(Sequencer* sequencer, void* userData)
{
    beep->setUserData(userData);

	sequencer->getPhone()->send(beep);

    return 1;
}

int BeepAtom::stop(Sequencer* sequencer)
{
    return beep->stop(sequencer->getPhone());
}

int EnergyDetectorAtom::start(Sequencer* sequencer, void* userData)
{
    energy.setUserData(userData);

	sequencer->getPhone()->send(energy);

    return 1;
}

int EnergyDetectorAtom::stop(Sequencer* sequencer)
{
    return energy.stop(sequencer->getPhone());
}

ConferenceAtom::ConferenceAtom(unsigned aConference, unsigned aMode)
 : mode(aMode), started(), me(0)
{
#ifndef __AG__
    conf = new Conference(aConference);
#else
	conf = gConferences[aConference];
#endif

    Terminator::getInstance().addLast(this);
}

ConferenceAtom::~ConferenceAtom()
{
    terminate();

    Terminator::getInstance().remove(this);
}

void ConferenceAtom::terminate()
{
    if (me) conf->remove(me);

#ifndef __AG__
	delete conf;
#endif
}

int ConferenceAtom::start(Sequencer* sequencer, void* aUserData)
{
	if (!conf)	return 0;

    userData = aUserData;

    me = conf->add(sequencer->getPhone()->getSlot(), sequencer->getPhone()->getSwitch(), mode);

    started.now();

    return me ? 1 : 0;
}

int ConferenceAtom::stop(Sequencer* sequencer)
{
    DateTime now;

    now.now();

    conf->remove(me);
    me = 0;

    sequencer->addCompleted(sequencer->getPhone(), (Molecule*)userData, 0, now - started);

    return 1;
}

TouchtoneAtom::TouchtoneAtom(Sequencer* sequencer, const char* att)
{
	tt = copyString(att);
	sample = sequencer->getPhone()->createTouchtones(att);
}

int TouchtoneAtom::start(Sequencer* sequencer, void* userData)
{
    sample.setUserData(userData);

	sequencer->getPhone()->send(sample);

    return 1;
}

int TouchtoneAtom::stop(Sequencer* sequencer)
{
    return sample.stop(sequencer->getPhone());
}

int SilenceAtom::start(Sequencer* sequencer, void* aUserData)
{
	entry = sequencer->getTimer().add(length, this, aUserData);

	seq = sequencer;

    return 1;
}

int SilenceAtom::stop(Sequencer* sequencer)
{
    if (entry)
	{
		pos = length - sequencer->getTimer().remove(entry);

		// if we have to use completed() or addCompleted() depends on the thread context
		// here we are in the sequencers context, so we use addCompleted
	    sequencer->addCompleted(sequencer->getPhone(), (Molecule*)entry->userData, 0, pos);

		entry = 0;

		return 1;
	}
	
	return 0;
}

void SilenceAtom::timer(AsyncTimer* server, void* userData)
{
	// if we have to use completed() or addCompleted() depends on the thread context.
	// here we are in the Timers context, so we use completed

    seq->completed(seq->getPhone(), (Molecule*)userData, 1, length);
	entry = 0;
	pos = 0;
}

int SilenceAtom::setPos(unsigned pos)
{
	length = length > pos ? length - pos : 0;

	return 1;
}

Molecule::Molecule(unsigned aMode, int aPriority, int aSyncMinor) : DList()
{
	timeStopped.now();
	priority = aPriority;
	syncMinor = aSyncMinor;
	mode = aMode;
	flags = 0;
	current = 0;
	pos = 0;
	status = 0;
	length = 0;
	nCurrent = 0;
}

Molecule::~Molecule()
{
	for (ListIter i(*this); !i.isDone(); i.next())    freeLink(i.current());
}

void Molecule::freeLink(List::Link* anAtom)
{
    delete (Atom*)anAtom;
}

int Molecule::start(Sequencer* sequencer, void* userData)
{
    int started;

    if (!isStopped())
    {
        if (current)
        {
            if (needRewind())   current->setPos(0);
            started = current->start(sequencer, this);
        }
        else
        {
            nCurrent = 0;
            current = (Atom*)head;
            started = current ? current->start(sequencer, this) : 0;
        }
    }
    else
    {
        if (mode & restart)
        {
            nCurrent = 0;
            current = (Atom*)head;
            current->setPos(0);
            started = current->start(sequencer, this);
        }
        else if ((mode & mute) || (mode & mix))
        {
            DateTime now;
            unsigned timeInactive;

            now.now();
            timeInactive = now - timeStopped;

            if (debug > 3)
			{
				logger->start() << "was inactive: " << timeInactive << '(' << now << ", " << timeStopped << ')' << endl;
				logger->end();
			}

            if (setPos(pos + timeInactive)) started =  current->start(sequencer, this);
            else started = 0;
        }
        else if (mode & pause)
        {
            setPos(pos);
			started =  current->start(sequencer, this);
        }
    }

    flags &= ~stopped;
    if (started) flags |= active;

    return started;
}

int Molecule::stop(Sequencer* sequencer)
{
    if (mode & dont_interrupt)    return 0;

    flags |= stopped;

    return current ? current->stop(sequencer) : 0;
}

int Molecule::done(Sequencer* sequencer, unsigned msecs, unsigned status)
{
    current->done(sequencer, msecs, status);
	if (current->isGrowing())	length += msecs;
    pos += msecs;

	if (debug > 3)
	{
		logger->start() << "Molecule::done. status: " << status << " msecs: " << msecs << " pos: " << pos << " length: " << length << endl;
		logger->end();
	}

    flags &= ~active;

    if (isStopped()) 
    {
        timeStopped.now();
        if (debug > 3)
		{
			logger->start() << "stopped at: " << timeStopped << endl;
			logger->end();
		}
    }

    if (isStopped())
    {
        if (mode & discard)   return 1;

        return 0;
    }
    else
    {
        nCurrent++;
        current = (Atom*)current->next;

        if (current == 0)
        {
            if(mode & loop)
            {
                if (pos >= length) flags |= need_rewind;
                nCurrent = 0;
				pos = 0;
                current = (Atom*)head;
 
                return 0;
            }
            return 1;
        }

        return 0;
    }
}

int Molecule::setPos(unsigned aPos)
{
    unsigned sum = 0;
    unsigned atomarLength;

    if (mode & loop)  aPos %=  length;
    else if (aPos >= length) return 0;

    pos = aPos;
    nCurrent = 0;
    for (ListIter i(*this); !i.isDone(); i.next(),nCurrent++)
    {
        atomarLength = ((Atom*)i.current())->getLength();
        if (pos <= atomarLength && pos > sum) 
        {
            current = (Atom*)i.current();
            return current->setPos(pos - sum);
        }
        sum += atomarLength;
    }
 
    return 0;
}

ostream& operator<<(ostream& out, Molecule& m)  
{ 
    out << "Molecule(" << m.getPriority() << ", " << m.getPos() << ", " << m.getLength() << ", ";

    if (m.getMode() & Molecule::discard)  out << "discard ";
    if (m.getMode() & Molecule::mute)  out << "mute ";
    if (m.getMode() & Molecule::mix)  out << "mix ";
    if (m.getMode() & Molecule::dont_interrupt)  out << "dont_interupt ";
    if (m.getMode() & Molecule::restart)  out << "restart ";
    if (m.getMode() & Molecule::pause)  out << "pause ";
    if (m.getMode() & Molecule::loop)  out << "loop ";

    out << ", " << m.getStatus() << ')' << endl;

    for (ListIter i(m); !i.isDone(); i.next())
    {
        if ((Atom*)i.current() == m.current)    out << "->  ";
        else out << "    ";
        ((Atom*)i.current())->printOn(out);
        out << endl;
    }

    return out;
}


Activity::Activity(unsigned aSyncMajor, Sequencer* aSequencer) : DList()
{ 
	syncMajor = aSyncMajor; 
	sequencer = aSequencer;
	flags = idle;
}

Activity::~Activity()
{ 
	for (ListIter i(*this); !i.isDone(); i.next())    freeLink(i.current());
}

Molecule* Activity::add(Molecule& newMolecule)
{
    Molecule* lastActive = (Molecule*)head;
	Molecule* current;

    for (ActivityIter i(*this); !i.isDone() && i.current()->getPriority() >= newMolecule.getPriority(); i.next());
	current = (Molecule*)i.previous();

	if (current)
	{
		if (debug > 3)
		{
			logger->start() << "adding " << newMolecule << "syncMinor: " << newMolecule.getSyncMinor() << " after " <<  *current << endl;
			logger->end();
		}

 		DList::addAfter(current, &newMolecule);
	}
    else 
    {
        if (debug > 3)
		{
			logger->start() << "adding " << newMolecule << "syncMinor: " << newMolecule.getSyncMinor() << " as first" << endl;
			logger->end();
		}
 
        addFirst(&newMolecule);
    }

    if (isActive())
    {
        if (isIdle() || lastActive == 0)
        {
            if (newMolecule.start(sequencer))   flags &= ~idle;
            else if (debug)
			{
				logger->start() << "new molecule start failed" << endl;
				logger->end();
			}
 
            if (debug > 3)
			{
				logger->start() << "new molecule while idle. flags: " << flags << endl;
				logger->end();
			}
        }
        else
        {
            if (debug > 3)
			{
				logger->start() << "new molecule " << newMolecule.getPriority() << " old " << lastActive->getPriority() << " interruptable: " << !(lastActive->getMode() & Molecule::dont_interrupt) << endl;
				logger->end();
			}

            if (newMolecule.getPriority() > lastActive->getPriority() && !(lastActive->getMode() & Molecule::dont_interrupt))
            {
                if (debug > 3)
				{
					logger->start() << "interrupting " << *lastActive << endl;
					logger->end();
				}
 
                lastActive->stop(sequencer);
            }
        }
    }

    return &newMolecule;
}

// caution: active molecules are not removed, just stopped

void Activity::remove(Molecule* aMolecule)
{
    if (aMolecule->isActive())   
    {
        aMolecule->stop(sequencer);

        return;
    }

    if (aMolecule->prev == 0)   removeFirst();
    else removeAfter(aMolecule->prev);

    delete aMolecule;
}

void Activity::freeLink(List::Link* aMolecule)
{
    delete (Molecule*)aMolecule;
}

int Activity::start()
{
    int started;
	Molecule* molecule;

    flags |= active;

	started = 0;
	while(head && !started)
	{
		molecule = (Molecule*)head;
		started = molecule->start(sequencer);
	
		if (started)
		{
			if (debug > 3)
			{
				logger->start() << "started: " << *molecule;
				logger->end();
			}
		}
		else
		{
			if (debug)
			{
				logger->start() << "start failed: " << *molecule;
				logger->end();
			}
	
			sequencer->sendMoleculeDone(syncMajor, molecule->getSyncMinor(), molecule->getStatus(), molecule->getPos(), molecule->getLength());

			remove((Molecule*)head);
		}
	}		

    if (started) flags &= ~idle;
    else flags |= idle;

    return started;
}

int Activity::stop()
{
    int stopped = 0;

    if (head) stopped = ((Molecule*)head)->stop(sequencer);

    flags &= ~active;
    flags |= idle;

    return stopped;
}

Molecule* Activity::find(unsigned syncMinor)
{
    for (ActivityIter i(*this); !i.isDone(); i.next())
    {
        if (i.current()->getSyncMinor() == syncMinor)  
        {
            return i.current();
        }
    }

    return 0;
}

ActivityCollection::ActivityCollection() : DList()
{
}

ActivityCollection::~ActivityCollection()
{
	for (ListIter i(*this); !i.isDone(); i.next())    freeLink(i.current());
}

void ActivityCollection::add(Activity& anActivity)
{
    addFirst(&anActivity);
}

void ActivityCollection::remove(Activity* anActivity)
{
    DList::remove(anActivity);

    delete anActivity;
}

void ActivityCollection::freeLink(List::Link* anActivity)
{
    delete (Activity*)anActivity;
}

Activity* ActivityCollection::find(unsigned syncMajor)
{
    for (ListIter i(*this); !i.isDone(); i.next())
    {
        if (((Activity*)i.current())->getSyncMajor() == syncMajor)  return (Activity*)i.current();
    }

    return 0;
}

// throws all idle activities away and stops the active one. That one must be thrown away later!
int ActivityCollection::empty()
{
    Activity* activ;
	int success = 1;

    for (ListIter i(*this); !i.isDone(); i.next())    
    {
        activ = (Activity*)i.current();

        if (activ->isIdle()) 
        {
            remove(activ);
        }
        else
        {
            activ->setDiscarded();
            activ->setASAP();
            activ->stop();

            success = 0;
        }
    }

	return success;
}

/*
	activ.cpp
*/


#include <windows.h>

#include "log.h"
#include "names.h"
#include "activ.h"
#include "sequence.h"

extern Conferences gConferences;

char* copyString(const char* aString)
{
	char* s = new char[strlen(aString) +1];
	strcpy(s, aString);

	return s;
}

bool Atom::start(Sequencer *sequencer)
{
	m_sample->start(sequencer->getMedia());

	return true;
}

bool Atom::stop(Sequencer *sequencer)
{
	return m_sample->stop(sequencer->getMedia());
}

PlayAtom::PlayAtom(unsigned channel, Sequencer* sequencer, 
				   const char* aFile) : Atom(channel)
{ 
	m_file = copyString(aFile);
	m_sample = sequencer->getMedia()->createFileSample(aFile);
}
 
RecordAtom::RecordAtom(unsigned channel, Sequencer* sequencer, 
					   const char* aFile, unsigned aTime) : Atom(channel), m_time(aTime)
{
	m_file = copyString(aFile); 

	m_sample = sequencer->getMedia()->createRecordFileSample(aFile, aTime);
}

BeepAtom::BeepAtom(unsigned channel, Sequencer* sequencer, 
				   unsigned count) : Atom(channel), m_nBeeps(count) 
{
	m_sample = sequencer->getMedia()->createBeeps(count);
}

ConferenceAtom::ConferenceAtom(unsigned channel, unsigned aConference, Conference::mode m)
 : Atom(channel), m_mode(m), m_conference(0), m_data(0)
{
	m_conference = gConferences[aConference];
}

bool ConferenceAtom::start(Sequencer* sequencer)
{
	if (!m_conference)
		return false;

	m_conference->add(sequencer->getMedia(), m_mode);

	m_started.now();

	return true;
}

bool ConferenceAtom::stop(Sequencer* sequencer)
{
	Time now;

	now.now();

	if (m_conference)
		m_conference->remove(sequencer->getMedia());

	sequencer->addCompleted(sequencer->getMedia(), (Molecule*)m_data, 0, now - m_started);

	return true;
}

TouchtoneAtom::TouchtoneAtom(unsigned channel, Sequencer* sequencer, 
							 const char* att) : Atom(channel)
{
	m_tt = copyString(att);

	m_sample = sequencer->getMedia()->createTouchtones(att);
}

bool SilenceAtom::start(Sequencer* sequencer)
{
	/* preserve m_timer_m_data */
	m_timer = sequencer->getTimer().add(m_length, this, m_timer.m_data);

	m_seq = sequencer;

	return true;
}

bool SilenceAtom::stop(Sequencer* sequencer)
{
	if (m_timer.is_valid())
	{
		// todo: calculate position

		sequencer->getTimer().remove(m_timer);

		// if we have to use completed() or addCompleted() depends on the thread context
		// here we are in the sequencers context, so we use addCompleted
		sequencer->addCompleted(sequencer->getMedia(), (Molecule*)m_timer.m_data, 0, m_pos);

		m_timer.invalidate();

		return true;
	}
	
	return false;
}

void SilenceAtom::on_timer(const Timer::TimerID &id)
{
	// if we have to use completed() or addCompleted() depends on the thread context.
	// here we are in the Timers context, so we use completed

	Molecule * m = (Molecule*)id.m_data;

	m_seq->completed(m_seq->getMedia(), m, 1, m_length);
	m_timer.invalidate();
	m_pos = 0;
}

bool SilenceAtom::setPos(unsigned pos)
{
	m_length = m_length > pos ? m_length - pos : 0;

	return true;
}

Molecule::Molecule(unsigned channel, unsigned aMode, int aPriority, const std::string &id) 
	: Atom(channel), m_mode(aMode), m_priority(aPriority), m_id(id), m_flags(0), 
	m_current(0), m_pos(0), m_status(0), m_length(0), m_nCurrent(0)
{
	m_timeStopped.now();
}

Molecule::~Molecule()
{
	for (ListIter i(*this); !i.isDone(); i.next())	  
		freeLink(i.current());
}

void Molecule::freeLink(List::Link* anAtom)
{
	delete (Atom*)anAtom;
}

bool Molecule::start(Sequencer* sequencer)
{
	bool started(false);

	if (!isStopped())
	{
		if (m_current)
		{
			if (needRewind())	
				m_current->setPos(0);
		}
		else
		{
			m_nCurrent = 0;
			m_current = (Atom*)head;
		}
	}
	else
	{
		if (m_mode & restart)
		{
			m_nCurrent = 0;
			m_current = (Atom*)head;
			m_current->setPos(0);
		}
		else if (m_mode & mute)
		{
			Time now;
			unsigned timeInactive;

			now.now();
			timeInactive = now - m_timeStopped;

			log(log_debug + 2, "activ") << "was inactive: " << timeInactive << logend();

			if (!setPos(m_pos + timeInactive)) 
			{
				m_flags &= ~stopped;
				return false;
			}
		}
		else if (m_mode & pause)
		{
			setPos(m_pos);
		}
	}

	if (m_current)
	{
		m_current->setUserData(this);
		started = m_current->start(sequencer);
	}

	m_flags &= ~stopped;
	if (started)
	{
		m_flags |= active;
		m_timeStarted.now();
	}

	return started;
}

bool Molecule::stop(Sequencer* sequencer)
{
	if (m_mode & dont_interrupt)
		return false;

	m_flags |= stopped;

	return m_current ? m_current->stop(sequencer) : false;
}

bool Molecule::done(Sequencer* sequencer, unsigned msecs, unsigned status)
{
	m_current->done(sequencer, msecs, status);
	if (m_current->isGrowing())	
		m_length += msecs;

	m_pos += msecs;

	log(log_debug, "activ") << "molecule done. status: " << status << " msecs: " 
		<< msecs << " pos: " << m_pos << " length: " << m_length << logend();

	m_flags &= ~active;

	if (isStopped()) 
	{
		m_timeStopped.now();
		log(log_debug + 2, "activ") << "stopped after: " 
			<< m_timeStopped - m_timeStarted << " ms" << logend();
	}

	if (isStopped())
	{
		if (m_mode & discard)
			return true;

		return false;
	}
	else
	{
		m_nCurrent++;
		m_current = (Atom*)m_current->next;

		if (m_current == 0)
		{
			if (m_mode & loop)
			{
				if (m_pos >= m_length) 
					m_flags |= need_rewind;

				m_nCurrent = 0;
				m_pos = 0;
				m_current = (Atom*)head;
 
				return false;
			}
			return true;
		}

		return false;
	}
}

bool Molecule::setPos(unsigned aPos)
{
	unsigned sum = 0;
	unsigned atomarLength;

	if (m_mode & loop)
		aPos %= m_length;
	else if (aPos >= m_length) 
		return false;

	m_pos = aPos;
	m_nCurrent = 0;
	for (ListIter i(*this); !i.isDone(); i.next(),m_nCurrent++)
	{
		atomarLength = ((Atom*)i.current())->getLength();
		if (m_pos <= atomarLength && m_pos > sum) 
		{
			m_current = (Atom*)i.current();
			return m_current->setPos(m_pos - sum);
		}
		sum += atomarLength;
	}
 
	return false;
}

void Molecule::printOn(std::ostream &out)
{ 
	out << "Molecule(" << getPriority() << ", " << getPos() << ", " << getLength() << ",";

	if (getMode() & Molecule::discard)  out << " discard";
	if (getMode() & Molecule::mute)  out << " mute";
	if (getMode() & Molecule::dont_interrupt)  out << " dont_interupt";
	if (getMode() & Molecule::restart)  out << " restart";
	if (getMode() & Molecule::pause)	out << " pause";
	if (getMode() & Molecule::loop)  out << " loop";

	out << ", " << result_name(getStatus()) << ')' << std::endl;

	for (ListIter i(*this); !i.isDone(); i.next())
	{
		if ((Atom*)i.current() == m_current)	out << "->  ";
		else out << "    ";
		((Atom*)i.current())->printOn(out);
		out << std::endl;
	}
}


std::ostream& operator<<(std::ostream& out, Molecule& m)
{
	m.printOn(out);

	return out;
}

Activity::~Activity()
{ 
	for (ListIter i(*this); !i.isDone(); i.next())	  freeLink(i.current());
}

Molecule* Activity::add(Molecule& newMolecule)
{
	Molecule* lastActive = (Molecule*)head;
	Molecule* current;

	for (ActivityIter i(*this); !i.isDone() && i.current()->getPriority() >= newMolecule.getPriority(); i.next());
	
	current = (Molecule*)i.previous();

	if (current)
	{
		log(log_debug + 2, "activ") << "adding " << newMolecule << "id: " 
			<< newMolecule.getId() << " after " <<  *current << logend();

		DList::addAfter(current, &newMolecule);
	}
	else 
	{
		log(log_debug + 2, "activ") << "adding " << newMolecule << "id: " 
			<< newMolecule.getId() << " as first" << logend();
 
		addFirst(&newMolecule);
	}

	if (lastActive == 0)
	{
		if (newMolecule.start(m_sequencer))	
			setState(active);
		else
		{
			log(log_warning, "activ") << "new molecule start failed" << logend();
		}

		log(log_debug + 2, "activ") << "new molecule while idle" << logend();
	}
	else
	{
		log(log_debug + 2, "activ") << "new molecule " << newMolecule.getPriority() 
			<< " old " << lastActive->getPriority() << " interruptable: " 
			<< !(lastActive->getMode() & Molecule::dont_interrupt) << logend();

		if (newMolecule.getPriority() > lastActive->getPriority() && !(lastActive->getMode() & Molecule::dont_interrupt))
		{
			log(log_debug + 2, "activ") << "interrupting " << *lastActive << logend();

			lastActive->stop(m_sequencer);
		}
	}

	return &newMolecule;
}

// caution: active molecules are not removed, just stopped

void Activity::remove(Molecule* aMolecule)
{
	if (aMolecule->isActive())	 
	{
		aMolecule->stop(m_sequencer);

		return;
	}

	if (aMolecule->prev == 0)	
		removeFirst();
	else 
		removeAfter(aMolecule->prev);

	if (getSize() == 0)
		setState(idle);

	delete aMolecule;
}

void Activity::freeLink(List::Link* aMolecule)
{
	delete (Molecule*)aMolecule;
}

bool Activity::start()
{
	bool started;
	Molecule* molecule;

	started = 0;
	while(head && !started)
	{
		molecule = (Molecule*)head;
		started = molecule->start(m_sequencer);
	
		if (started)
		{
			log(log_debug + 2, "activ") << "started: " << *molecule << logend();
		}
		else
		{
			log(log_error, "activ") << "start failed: " << *molecule << logend();
	
			m_sequencer->sendMoleculeDone(molecule->getId(),
				molecule->getStatus(), molecule->getPos(), molecule->getLength());

			remove((Molecule*)head);
		}
	}		

	setState(started ? active : idle);

	return started;
}

bool Activity::stop()
{
	bool stopped(false);

	setState(stopping);

	if (head)
	{
		stopped = ((Molecule*)head)->stop(m_sequencer);
	}

	if (stopped)
		setState(idle);

	return stopped;
}

bool Activity::abort()
{
	bool stopped(false);

	setState(stopping);

	if (head)
	{
		stopped = ((Molecule*)head)->stop(m_sequencer);
		removeAfter(head);
	}

	if (stopped)
	{
		setState(idle);
	}

	return stopped;
}

Molecule* Activity::find(const std::string &id)
{
	for (ActivityIter i(*this); !i.isDone(); i.next())
	{
		if (i.current()->getId() == id)  
		{
			return i.current();
		}
	}

	return 0;
}


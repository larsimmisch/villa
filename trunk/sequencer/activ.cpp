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

int Atom::start(Sequencer *sequencer, void *userData)
{
	m_sample->setUserData(userData);

	m_sample->start(sequencer->getMedia());

	return true;
}

int Atom::stop(Sequencer *sequencer)
{
	return m_sample->stop(sequencer->getMedia());
}

PlayAtom::PlayAtom(Sequencer* sequencer, const char* aFile)
{ 
	m_file = copyString(aFile);
	m_sample = sequencer->getMedia()->createFileSample(aFile);
}
 
RecordAtom::RecordAtom(Sequencer* sequencer, const char* aFile, unsigned aTime)
	: m_time(aTime)
{
	m_file = copyString(aFile); 

	m_sample = sequencer->getMedia()->createRecordFileSample(aFile, aTime);
}

BeepAtom::BeepAtom(Sequencer* sequencer, unsigned count) : m_nBeeps(count) 
{
	m_sample = sequencer->getMedia()->createBeeps(count);
}

ConferenceAtom::ConferenceAtom(unsigned aConference, unsigned aMode)
 : m_mode(aMode), m_conference(0)
{
	m_conference = gConferences[aConference];
}

int ConferenceAtom::start(Sequencer* sequencer, void* aUserData)
{
	if (!m_conference)
		return 0;

	m_userData = aUserData;

	m_conference->add(sequencer->getMedia(), m_mode);

	m_started.now();

	return 1;
}

int ConferenceAtom::stop(Sequencer* sequencer)
{
	Time now;

	now.now();

	if (m_conference)
		m_conference->remove(sequencer->getMedia());

	sequencer->addCompleted(sequencer->getMedia(), (Molecule*)m_userData, 0, now - m_started);

	return 1;
}

TouchtoneAtom::TouchtoneAtom(Sequencer* sequencer, const char* att)
{
	m_tt = copyString(att);

	m_sample = sequencer->getMedia()->createTouchtones(att);
}

int SilenceAtom::start(Sequencer* sequencer, void* aUserData)
{
	m_timer = sequencer->getTimer().add(m_length, this, aUserData);

	m_seq = sequencer;

	return 1;
}

int SilenceAtom::stop(Sequencer* sequencer)
{
	if (m_timer.is_valid())
	{
		// todo: calculate position

		sequencer->getTimer().remove(m_timer);

		// if we have to use completed() or addCompleted() depends on the thread context
		// here we are in the sequencers context, so we use addCompleted
		sequencer->addCompleted(sequencer->getMedia(), (Molecule*)m_timer.m_data, 0, m_pos);

		m_timer.invalidate();

		return 1;
	}
	
	return 0;
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

int SilenceAtom::setPos(unsigned pos)
{
	m_length = m_length > pos ? m_length - pos : 0;

	return 1;
}

Molecule::Molecule(unsigned aMode, int aPriority, const std::string &id) 
	: DList(), m_mode(aMode), m_priority(aPriority), m_id(id), m_flags(0), 
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

int Molecule::start(Sequencer* sequencer, void* userData)
{
	int started;

	if (!isStopped())
	{
		if (m_current)
		{
			if (needRewind())	
				m_current->setPos(0);
			started = m_current->start(sequencer, this);
		}
		else
		{
			m_nCurrent = 0;
			m_current = (Atom*)head;
			started = m_current ? m_current->start(sequencer, this) : 0;
		}
	}
	else
	{
		if (m_mode & restart)
		{
			m_nCurrent = 0;
			m_current = (Atom*)head;
			m_current->setPos(0);
			started = m_current->start(sequencer, this);
		}
		else if (m_mode & mute)
		{
			Time now;
			unsigned timeInactive;

			now.now();
			timeInactive = now - m_timeStopped;

			log(log_debug + 2, "activ") << "was inactive: " << timeInactive << logend();

			if (setPos(m_pos + timeInactive)) 
				started = m_current->start(sequencer, this);
			else 
				started = 0;
		}
		else if (m_mode & pause)
		{
			setPos(m_pos);
			started = m_current->start(sequencer, this);
		}
	}

	m_flags &= ~stopped;
	if (started)
	{
		m_flags |= active;
		m_timeStarted.now();
	}

	return started;
}

int Molecule::stop(Sequencer* sequencer)
{
	if (m_mode & dont_interrupt)
		return 0;

	m_flags |= stopped;

	return m_current ? m_current->stop(sequencer) : 0;
}

int Molecule::done(Sequencer* sequencer, unsigned msecs, unsigned status)
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
			return 1;

		return 0;
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

	if (m_mode & loop)
		aPos %= m_length;
	else if (aPos >= m_length) 
		return 0;

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
 
	return 0;
}

std::ostream& operator<<(std::ostream& out, Molecule& m)  
{ 
	out << "Molecule(" << m.getPriority() << ", " << m.getPos() << ", " << m.getLength() << ",";

	if (m.getMode() & Molecule::discard)  out << " discard";
	if (m.getMode() & Molecule::mute)  out << " mute";
	if (m.getMode() & Molecule::dont_interrupt)  out << " dont_interupt";
	if (m.getMode() & Molecule::restart)  out << " restart";
	if (m.getMode() & Molecule::pause)	out << " pause";
	if (m.getMode() & Molecule::loop)  out << " loop";

	out << ", " << result_name(m.getStatus()) << ')' << std::endl;

	for (ListIter i(m); !i.isDone(); i.next())
	{
		if ((Atom*)i.current() == m.m_current)	out << "->  ";
		else out << "    ";
		((Atom*)i.current())->printOn(out);
		out << std::endl;
	}

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

int Activity::start()
{
	int started;
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

int Activity::stop()
{
	int stopped = 0;

	if (head) 
		stopped = ((Molecule*)head)->stop(m_sequencer);

	setState(stopped ? idle : stopping);

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


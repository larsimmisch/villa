/*
	Copyright 1995 Immisch, Becker & Partner, Hamburg

	created: Thu Nov 16 15:40:24 GMT+0100 1995

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _CONFERENCE_H_
#define _CONFERENCE_H_

#include "list.h"
#include "omnithread.h"
#include "switch.h"

enum { max_conferences = 64 };

class Conferences;

class Conference : public DList
{
public:

	enum mode { listen = 0x01, speak = 0x02, play = 0x04 };

	class Member : public DList::DLink
	{
	public:
	
		Member(Timeslot aVoice, int aMode, Switch* aSwitch);
		~Member();
		
		void listenTo(Timeslot slot);	
		void speakTo(Timeslot slot);   
		void playTo(Timeslot slot);   
		
		void connect(Member* aMamber);
		void addToConference(unsigned aConf);
		void removeFromConference(unsigned aConf);
		
		void restore();
		
		// return number of auxilliary slots used
		int slots() 	{ return ((mode & speak) || (mode & listen)) && (mode & play) ? 2 : 1; }
		
		Timeslot voice;
		Timeslot trunk;
		Timeslot aux[2];
		Timeslot conf[2];
		int  used;
		Switch* sw;
		int mode;
	};
	
	virtual ~Conference();

	virtual Member* add(Timeslot aVoice, Switch* aSwitch, int aMode);
	virtual void remove(Member* aMember);
	
	void mute(Member* aMember);
		
	unsigned getHandle()	{ return handle; }

	void* getUserData() 	{ return userData; }
	
protected:

	Conference(unsigned aHandle, void* aUserData = 0);
	void freeLink(List::Link* aLink) { delete (Member*)aLink; }

	friend class Conferences;

	omni_mutex mutex;
	unsigned handle;
	unsigned xds;
	unsigned speakers;
	void*	 userData;
};

class ConferencesIterator;
class ConferenceSwitch;

class Conferences
{
public:

	Conferences(unsigned max = max_conferences);
	~Conferences();

	Conference* operator[](unsigned index);

	Conference* create(void* aUserData = 0);
	int close(unsigned index, int force = 0);

	// static void setMVIP(MVIP& instance);
	// static void setMixing(ConferenceSwitch* aMixing);

	void lock() 	{ mutex.lock(); }
	void unlock()	{ mutex.unlock(); }

protected:

	friend class ConferencesIterator;

	omni_mutex mutex;
	Conference** array;
	unsigned size;
	unsigned offset;
};

class ConferencesIterator
{
public:
 
	ConferencesIterator(Conferences c);
	~ConferencesIterator() {}
 
	Conference* next();
	Conference* current()	{ return conferences.array[index]; }	
	int 		isDone()	{ return index < conferences.size; }
  
protected:
 
	Conferences&	conferences;
	unsigned		index;
};

#endif

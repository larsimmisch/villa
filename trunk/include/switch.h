/*
	switch.h

	Copyright 1995-2000 ibp (uk) Ltd.

	$Id: switch.h,v 1.1 2000/10/02 15:52:14 lars Exp $

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _SWITCH_H_
#define _SWITCH_H_

#include <ostream>
#include "exc.h"

class SwitchError : public Exception
{
public:

	SwitchError(const char *file, int line, const char* function, const char* aSwitchFunction, int aResult, Exception* prev = 0) 
		: Exception(file, line,  function, "SwitchError", prev), switchFunc(aSwitchFunction), result(aResult) {}
	virtual ~SwitchError() {}

	virtual void printOn(std::ostream& aStream);

protected:

	const char* switchFunc;
	int result;
};

class Slot;

class Switch
{
public:

	Switch(void* io, int is32bit = 1, int aDevice = 0, const char* aName = 0) 
		: ioctl(io), is32(is32bit), device(aDevice), name(aName) {}
	virtual ~Switch()	{}
	
	virtual void listen(Slot a, Slot b);
	virtual void listen(Slot a, char pattern);
	virtual void connect(Slot a, Slot b);
	virtual void disable(Slot a);
	virtual char sample(Slot a);
	
	virtual Slot query(Slot a);
	
	virtual const char* getName()	{ return name; }

	virtual int contains(Switch* aSwitch) { return 0; }

	int operator==(Switch& aSwitch);

    int device;

protected:
	
	const char* name;
	void* ioctl;
	int is32;
};

class Slot
{
public:

	Slot(int stream = 0, int timeslot = 0) : st(stream), ts(timeslot) {}
	~Slot() {}
		
	void listenTo(Slot& b, Switch& aSwitch)		{ aSwitch.listen(*this, b);  }
	void connectTo(Slot& b, Switch& aSwitch)	{ aSwitch.connect(*this, b); }
	
	Slot operator~()	{ return Slot(st > 7 ? st - 8 : st + 8, ts); }
	int operator==(Slot& s)	{ return (st == s.st && ts == s.ts); }

	friend std::ostream& operator<<(std::ostream& out, const Slot& s)	{ out << s.st << ':' << s.ts; return out; }

	unsigned st:8;
	unsigned ts:8;
};

class MVIP
{
public:

	enum { max_streams = 8 };

	MVIP();
	~MVIP() {}
	
	Slot allocate();
	Slot allocate(Slot aSlot);
	void release(Slot aSlot);

	int inUse(Slot aSlot);
	int inUse(unsigned aStream);
	
protected:
	
	unsigned streams[max_streams];
};

#endif

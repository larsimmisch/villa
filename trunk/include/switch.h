/*
	switch.h

	Copyright 1995-2001 Lars Immisch

	$Id: switch.h,v 1.4 2001/06/07 12:58:25 lars Exp $

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _SWITCH_H_
#define _SWITCH_H_

#include <ostream>
#include <string>
#include "exc.h"

class SwitchError : public Exception
{
public:

	SwitchError(const char *file, int line, const char* function, const char* aSwitchFunction, int aResult, Exception* prev = 0) 
		: Exception(file, line,  function, "SwitchError", prev), switchFunc(aSwitchFunction), result(aResult) {}
	virtual ~SwitchError() {}

	virtual void printOn(std::ostream& out) const
	{
		Exception::printOn(out);
		out << switchFunc << " failed: " << result;
	}

protected:

	const char* switchFunc;
	int result;
};

class Timeslot;

class Switch
{
public:

	Switch(int aDevice = 0, const char* aName = 0) : device(aDevice), name(aName) {}
	virtual ~Switch()	{}
	
	virtual void listen(const Timeslot &a, const Timeslot &b) = 0;
	virtual void listen(const Timeslot &a, char pattern) = 0;
	virtual void connect(const Timeslot &a, const Timeslot &b)
	{
		listen(a, b);
		listen(b, a);
	}

	virtual void disable(const Timeslot &a) = 0;
	virtual char sample(const Timeslot &a) = 0;
	
	virtual Timeslot query(const Timeslot &a) = 0;
	
	virtual const char* getName() const { return name.c_str(); }

	virtual int contains(Switch* aSwitch) { return 0; }

	bool operator==(const Switch& other) const
	{
		return (name == other.name && device == other.device);
	}

    int device;

protected:
	
	std::string name;
};

class Timeslot
{
public:

	Timeslot(unsigned stream = 0, unsigned timeslot = 0) : st(stream), ts(timeslot) {}
	~Timeslot() {}
		
	void listenTo(Timeslot& b, Switch& aSwitch)		{ aSwitch.listen(*this, b);  }
	void connectTo(Timeslot& b, Switch& aSwitch)	{ aSwitch.connect(*this, b); }
	
	Timeslot operator~()	{ return Timeslot(st > 7 ? st - 8 : st + 8, ts); }
	int operator==(Timeslot& s)	{ return (st == s.st && ts == s.ts); }

	friend std::ostream& operator<<(std::ostream& out, const Timeslot& s)
	{ 
		out << s.st << ':' << s.ts; 
		return out; 
	}

	unsigned st;
	unsigned ts;
};

#endif

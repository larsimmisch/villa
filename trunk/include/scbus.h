/*
	scbus.h

	Copyright 1995-2000 ibp (uk) Ltd.

	$Id: scbus.h,v 1.1 2000/10/18 11:10:46 lars Exp $

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef SCBUS_H_
#define SCBUS_H_

#include <bitset>

// we are using the Aculab convention of assigning SCbus the stream number 24

class SCbus
{
public:

	SCbus() {}
	~SCbus() {}
	
	Timeslot allocate()
	{
		for (int i = 0; i < timeslots.bitset_size; ++i)
		{
			if(!timeslots[i])
			{
				timeslots.set(i);

				return Timeslot(24, i);
			}
		}
		throw Exception(__FILE__, __LINE__, "SCbus::allocate()", "SCbus resources exhausted");

		// unreached statement
		return Timeslot(-1,-1);
	}

	Timeslot allocate(const Timeslot &a)
	{
		if (a.st != 24)
			throw Exception(__FILE__, __LINE__, "SCbus::allocate()", "invalid SCbus stream");

		if (timeslots[a.ts])
			throw Exception(__FILE__, __LINE__, "SCbus::allocate()", "SCbus slot in use");

		timeslots.set(a.ts);

		return Timeslot(a);
	}

	void release(const Timeslot &a)
	{
		if (a.st != 24)
			throw Exception(__FILE__, __LINE__, "SCbus::release()", "invalid SCbus stream");

		if (timeslots[a.ts])
			throw Exception(__FILE__, __LINE__, "SCbus::release()", "SCbus timeslot not in use");

		timeslots.reset(a.ts);
	}

	int inUse(const Timeslot &a) const
	{
		if (a.st != 24)
			throw Exception(__FILE__, __LINE__, "SCbus::inUse()", "invalid SCbus stream");

		return timeslots[a.ts];
	}
	
protected:
	
	std::bitset<1024> timeslots;
};

#endif
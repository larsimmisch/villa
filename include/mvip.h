/*
	mvip.h

	Copyright 1995-2001 Lars Immisch

	$Id$

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef MVIP_H_
#define MVIP_H_

#include <bitset>

class MVIP
{
public:

	MVIP() {}
	~MVIP() {}
	
	Timeslot allocate()
	{
		for (int i = 0; i < timeslots.bitset_size; ++i)
		{
			if(!timeslots[i])
			{
				timeslots.set(i);

				return Timeslot(i / 32, i % 32);
			}
		}
		throw Exception(__FILE__, __LINE__, "MVIP::allocate()", "MVIP resources exhausted");

		// unreached statement
		return Timeslot(-1,-1);
	}

	Timeslot allocate(const Timeslot &a)
	{
		if (a.st > 15)
			throw Exception(__FILE__, __LINE__, "MVIP::allocate()", "invalid MVIP stream");

		int p = a.st * 32 + a.ts;

		if (timeslots[p])
			throw Exception(__FILE__, __LINE__, "MVIP::allocate()", "MVIP slot in use");

		timeslots.set(p);

		return Timeslot(a);
	}

	void release(const Timeslot &a)
	{
		if (a.st > 15)
			throw Exception(__FILE__, __LINE__, "MVIP::release()", "invalid MVIP stream");

		int p = a.st * 32 + a.ts;

		if (timeslots[p])
			throw Exception(__FILE__, __LINE__, "MVIP::release()", "MVIP timeslot not in use");

		timeslots.reset(p);
	}

	int inUse(const Timeslot &a) const
	{
		if (a.st > 15)
			throw Exception(__FILE__, __LINE__, "MVIP::inUse()", "invalid MVIP stream");

		return timeslots[a.st * 32 + a.ts];
	}
	
protected:
	
	std::bitset<512> timeslots;
};

#endif
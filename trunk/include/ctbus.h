/*
	ctbus.h

	Copyright 1995-2000 ibp (uk) Ltd.

	$Id$

	Author: Lars Immisch <lars@ibp.de>
*/

// we follow the Aculab convention that H.100 uses streams 0..31 
// with timeslots ranging from 0..127

#ifndef CTBUS_H_
#define CTBUS_H_

#include <bitset>

// protocol class for timeslot allocation

class CTbus
{
public:

	virtual Timeslot allocate() = 0;
	virtual Timeslot allocate(const Timeslot &a) = 0;
	virtual void release(const Timeslot &a) = 0;
	virtual int inUse(const Timeslot &a) const = 0;
};

// we are using the Aculab convention of assigning SCbus the stream number 24

class SCbus : public CTbus
{
public:

	SCbus() {}
	virtual ~SCbus() {}
	
	virtual Timeslot allocate()
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

	virtual Timeslot allocate(const Timeslot &a)
	{
		if (a.st != 24)
			throw Exception(__FILE__, __LINE__, "SCbus::allocate()", "invalid SCbus stream");

		if (timeslots[a.ts])
			throw Exception(__FILE__, __LINE__, "SCbus::allocate()", "SCbus slot in use");

		timeslots.set(a.ts);

		return Timeslot(a);
	}

	virtual void release(const Timeslot &a)
	{
		if (a.st != 24)
			throw Exception(__FILE__, __LINE__, "SCbus::release()", "invalid SCbus stream");

		if (timeslots[a.ts])
			throw Exception(__FILE__, __LINE__, "SCbus::release()", "SCbus timeslot not in use");

		timeslots.reset(a.ts);
	}

	virtual int inUse(const Timeslot &a) const
	{
		if (a.st != 24)
			throw Exception(__FILE__, __LINE__, "SCbus::inUse()", "invalid SCbus stream");

		return timeslots[a.ts];
	}
	
protected:
	
	std::bitset<1024> timeslots;
};

class H100 : public CTbus
{
public:

	H100() {}
	virtual ~H100() {}
	
	virtual Timeslot allocate()
	{
		for (int i = 0; i < timeslots.bitset_size; ++i)
		{
			if(!timeslots[i])
			{
				timeslots.set(i);

				return Timeslot(i / 128, i % 128);
			}
		}
		throw Exception(__FILE__, __LINE__, "H100::allocate()", "H.100 resources exhausted");

		// unreached statement
		return Timeslot((unsigned)-1,(unsigned)-1);
	}

	virtual Timeslot allocate(const Timeslot &a)
	{
		if (a.st > 31)
			throw Exception(__FILE__, __LINE__, "H100::allocate()", "invalid H.100 stream");

		int ts = a.st * 128 + a.ts;

		if (timeslots[ts])
			throw Exception(__FILE__, __LINE__, "H100::allocate()", "H.100 slot in use");

		timeslots.set(ts);

		return Timeslot(a);
	}

	virtual void release(const Timeslot &a)
	{
		if (a.st > 31)
			throw Exception(__FILE__, __LINE__, "H100::release()", "invalid H100 stream");

		int ts = a.st * 128 + a.ts;

		if (timeslots[ts])
			throw Exception(__FILE__, __LINE__, "H100::release()", "H100 timeslot not in use");

		timeslots.reset(ts);
	}

	virtual int inUse(const Timeslot &a) const
	{
		if (a.st > 31)
			throw Exception(__FILE__, __LINE__, "H100::inUse()", "invalid H100 stream");

		return timeslots[a.st * 128 + a.ts];
	}
	
protected:
	
	std::bitset<4096> timeslots;
};

#endif
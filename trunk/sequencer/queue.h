/*
	Queue.h
	
	Copyright 1995 Immisch, Becker & Partner, Hamburg

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _COMPLETED_QUEUE_H_
#define _COMPLETED_QUEUE_H_

#include <Collect/List.h>
#include <Sequence/Activ.h>

class CompletedQueue : public List
{
	public:

	struct Item : public List::Link
	{
		Item(Telephone* aServer, Molecule* aMolecule, unsigned ms, unsigned aStatus)
			: server(aServer), molecule(aMolecule), msecs(ms), status(aStatus) {}

		Telephone* server;
		Molecule* molecule;
		unsigned msecs;
		unsigned status;
	};
	
	CompletedQueue() : List() {}
	virtual ~CompletedQueue() { for( LinkIter i(head); !i.isDone(); i.next() ) freeLink( i.current() );}
	
	void enqueue(Telephone* t, Molecule* m, int c, unsigned ms)	{ addLast(new Item(t, m, c, ms)); }
	Item* dequeue()												{ return (Item*)removeFirst(); }
	virtual void freeLink(List::Link* item)						{ delete (Item*)item; }
};

#endif

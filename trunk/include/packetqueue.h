// packetqueue.h

#ifndef PACKETQUEUE_H_
#define PACKETQUEUE_H_

#include "list.h"

class PacketQueue : public List
{
	public:

	struct Item : public List::Link
	{
		Item(void* aData) : data(aData) {}
		
		void* data;
	};
	
	PacketQueue() : List() {}
	virtual ~PacketQueue() { for( LinkIter i(head); !i.isDone(); i.next() ) freeLink( i.current() );}
	
	void enqueue(void* data)	{ addLast(new Item(data)); }
	void* dequeue()
	{
		Item* i = (Item*)removeFirst();
		void* d = i->data;
		delete i;
		return d;
	}
	virtual void freeLink(List::Link* item)	{ delete (Item*)item; }
};

#endif
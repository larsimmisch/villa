/*
	Services.h

	Copyright 1995 Immisch, Becker & Partner, Hamburg

	Author: Lars Immisch <lars@ibp.de>
*/

#include "omnithread.h"
#include "set.h"
#include "list.h"

class Socket;
class Listener;

class ListenerQueue : public DList
{
public:

	class Item : public DList::DLink
	{
	public:

		Item(Listener* aListener) 
			: result(0), listener(aListener), event(&mutex) {}

		omni_mutex mutex;
		omni_condition event;
		SAP sap;
		int hsocket;
		Listener* listener;
		unsigned result;
	};
	
	ListenerQueue();
	virtual ~ListenerQueue();
	
	Item* enqueue(Listener* aListener);
	Item* dequeue();
	
	void cancel(Item* item);
	
protected:

	virtual void freeLink(List::Link* aLink);
	
	ListenerQueue(ListenerQueue&);
};

class Services : private Set
{
public:

	class Key : public List::Link
	{
		public:

		Key(int aProtocol, int aService) : service(aService), protocol(aProtocol) {}
		virtual ~Key() {}

		int service;
		int protocol;
	};

	Services(int size) : mutex(), Set(size) { }
	virtual ~Services() { empty(); }

	void lock() 	{ mutex.lock(); }
	void unlock()	{ mutex.unlock(); }

	ListenerQueue::Item* add(int protocol, SAP& aService);
	void remove(ListenerQueue::Item* anItem);
	void remove(Listener* aListener);

	protected:

	friend class ListenerQueue; // for mutex

	Listener* contains(int aProtocol, int aService);

	virtual void empty();
	virtual int hasKey(List::Link* anItem, const char* key) { return 0; }
	virtual int isEqual(List::Link* anItem, List::Link* anotherItem) 
		{ return ((Key*)anItem)->service == ((Key*)anotherItem)->service && ((Key*)anItem)->protocol == ((Key*)anotherItem)->protocol; }
	virtual int hasIndex(List::Link* anItem, int anIndex)	{ return 0; }
	virtual unsigned hashAssoc(List::Link* anItem);
		
	omni_mutex mutex;
};				

class Listener : public Services::Key, public omni_thread
{
public:

	Listener(int aProtocol, SAP& aService);
	virtual ~Listener();

	virtual void *run_undetached(void *arg);
	
	ListenerQueue queue;
	int hsocket;
};

/*
	Copyright 1995-2001 Lars Immisch

	created: Tue Nov 12 17:02:03 GMT+0100 1996

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

#include "omnithread.h"
#include "exc.h"
#include "list.h"
#include "set.h"
#include "registry.h"
#include "aculab/acutrunk.h"
#include "interface.h"

class InvalidKey : public Exception
{
public:

	InvalidKey(const char* file, int line, const char* function, const char* aKey, const Exception* previous = 0) 
		: Exception(file, line, function, "invalid key", previous), key(0) 
		{ if (aKey) { key = new char[strlen(aKey) + 1]; strcpy(key, aKey); } }

    virtual ~InvalidKey() { delete key; }

    virtual void printOn(std::ostream& aStream);

	char* key;
};

class ClientQueue : public DList
{
public:

	struct Item : public DList::DLink
	{
		Item(const SAP &aDetail, const SAP &aClient, void* aTag = 0) 
			: details(aDetail), client(aClient), tag(aTag) {}
		
		SAP details;
		SAP client;
		void* tag;
	};
	
	ClientQueue() : DList() {}
	virtual ~ClientQueue() { for (LinkIter i(head); !i.isDone(); i.next() ) freeLink( i.current() );}
	
	void enqueue(const SAP &details, const SAP &client, void* tag = 0)	{ addLast(new Item(details, client, tag)); }
	Item* dequeue();

	void remove(void* tag);
	void remove(void* tag, const SAP &aSAP);

	virtual void freeLink(List::Link* item)	{ delete (Item*)item; }
};

class DDIIterator;

class DDIs
{
public:
	
	class Node
	{
		public:

		Node();

		Node* subKey(char c);
		void setSubKey(char c, Node* next)	{ children[c - '0'] = next; }

		Node* children[10];
		ClientQueue queue;
	};

	DDis();
	~DDIs();

	Node* find(const char* key);
	Node* create(const char* key);
	void remove(const char* key);

	// returns true if key has zero length and the root has children
	// otherwise returns true if key can be found
	int isSubKey(const char* key);

protected:

	friend class DDIIterator;

	void remove(Node* aNode);

	Node root;
};

class DDIIterator
{
public:

	DDIIterator(DDIs& addis);
	~DDIIterator() {}

	DDIs::Node* next();
	DDIs::Node* current()	{ return node; }
	int isDone()	{ return node == 0; }

protected:

	DDIs* ddis;
	DDIs::Node* node;
	char key[32];
};

class TrunkConfiguration : public List::Link
{
public:

	virtual ~TrunkConfiguration();

	virtual int isDigital()			{ return 0; }
	virtual Timeslot preferredSlot()	{ return Timeslot(-1,-1); }

	virtual Trunk* getTrunk(TrunkClient *client = 0) = 0;
	virtual unsigned numLines() = 0;

	virtual int readFromKey(RegistryKey& key);

	char* getName()					{ return name; }
	char* getNumber()				{ return number; }

	// starts sequencers for all lines
	virtual void start() = 0;

	// removes a Client with given tag
	virtual void removeClient(void* tag) = 0;
	virtual void removeClient(void* tag, const SAP& aSAP) = 0;

	virtual void enqueue(const SAP& details, const SAP& client, void* tag) = 0;
	virtual ClientQueue::Item* dequeue(const SAP& details) = 0;
	virtual int isContained(const SAP& details)	{ return 0; }

	virtual unsigned connect(ConnectCompletion* complete) = 0;

protected:

	TrunkConfiguration() {}

	char* name;
	char* number;
	unsigned free;
	omni_mutex mutex;
};

class Sequencer;

// the Aculab PRI can be configured to run less than 30 lines

class AculabPRITrunkConfiguration : public TrunkConfiguration
{
public:

	AculabPRITrunkConfiguration() : lines(30) {}
	virtual ~AculabPRITrunkConfiguration();

	virtual int isDigital()		{ return 1; }

	virtual Trunk* getTrunk(TrunkClient *client = 0)	
	{ 
		return new AculabTrunk(client, device, 0); 
	}
	virtual unsigned numLines()	{ return lines; }

	virtual int readFromKey(RegistryKey& key);

	virtual void start();
	virtual void removeClient(void* tag);
	virtual void removeClient(void* tag, const SAP& aSAP);

	virtual void enqueue(const SAP& details, const SAP& client, void* tag);
	virtual ClientQueue::Item* dequeue(const SAP& details);
	virtual int  isContained(const SAP& details)
	{ 
		return ddis.isSubKey(details.getService()) != 0;
	}

	virtual unsigned connect(ConnectCompletion* complete);

protected:

	unsigned device;
	unsigned swdevice;
	unsigned lines;
	DDIs ddis;
	Sequencer* sequencers[30];
};

class ConfiguredTrunks : public Set
{
	public:
		
	ConfiguredTrunks();
	virtual ~ConfiguredTrunks();
	
	// returns the added item, unlike Set, which returns the previous one
	TrunkConfiguration* add(TrunkConfiguration* configuration);
	void remove(const char* aName);
	TrunkConfiguration* operator[](const char* aName);
	TrunkConfiguration* at(const char* aName) { return(TrunkConfiguration*)Set::basicAt(aName); }

	void lock()	 	{ mutex.lock(); }
	void unlock()	{ mutex.unlock(); }
	
	virtual void empty();
	virtual int hasKey(List::Link* anItem, const char* key);
	virtual int isEqual(List::Link* anItem, List::Link* anotherItem);
	virtual int hasIndex(List::Link* anItem, int anIndex);
	virtual unsigned hashAssoc(List::Link* anItem);

	virtual void freeLink(List::Link* item);
	
protected:
	
	omni_mutex mutex;
	unsigned lines;
};

class ConfiguredTrunksIterator : public Set::AssocIter
{
public:

	ConfiguredTrunksIterator(ConfiguredTrunks& aList) : Set::AssocIter(aList) {}

	TrunkConfiguration* current()   
#ifdef _MSC_VER
	{ return (TrunkConfiguration*)AssocIter::current(); }
#else
	{ return (TrunkConfiguration*)Set::AssocIter::current(); }
#endif

};

#endif
/*
	AsyncTCP.h
*/

#ifndef _ASYNCTEXT_H_
#define _ASYNCTEXT_H_

#include "omnithread.h"
#include "text.h"
#include "socket.h"
#include "client.h"

class TextQueue : public List
{
public:

	struct Item : public List::Link
	{
		Item(void* aData) : data(aData) {}
		
		void* data;
	};
	
	TextQueue() : List() {}
	virtual ~TextQueue() { for( LinkIter i(head); !i.isDone(); i.next() ) freeLink( i.current() );}
	
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

class AsyncText : public TextTransport, public omni_thread
{
public:
		
	AsyncText(TextTransportClient& aClient, void* aPrivateData = 0);
	virtual ~AsyncText();
	
	// Data transfer
	virtual void send(const char* data, int expedited = 0);
	virtual char* receive();

	void run();
	
	virtual void fatal(char* error);
	
protected:
	
	// helper methods
	virtual void aborted();
	virtual void setState(states aState);
	
	virtual int doListen()  { event.signal(); return 0; }
	virtual int doConnect() { event.signal(); return 0; }
	
	TextTransportClient& getClient()	{ return client; }
	
	TextTransportClient& client; 
	TextQueue incoming;
	omni_mutex mutex;
	omni_condition event;
};
#endif /* _ASYNCTEXT_H_ */

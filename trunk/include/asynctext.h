/*
	AsyncTCP.h
*/

#ifndef _ASYNCTEXT_H_
#define _ASYNCTEXT_H_

#include "omnithread.h"
#include "text.h"
#include "socket.h"
#include "client.h"

class AsyncText : public TextTransport, public omni_thread
{
public:
		
	AsyncText(TextTransportClient& aClient, void* aPrivateData = 0);
	virtual ~AsyncText();

	virtual void fatal(char* error);
	
	virtual void run();

	virtual void lock() { m_mutex.lock(); }
	virtual void unlock() { m_mutex.unlock(); }

protected:
	
	// helper methods
	virtual void aborted();
	virtual void setState(states aState);
	
	virtual int doListen()  { m_event.signal(); return 0; }
	virtual int doConnect() { m_event.signal(); return 0; }
	
	TextTransportClient& getClient()	{ return m_client; }
	
	TextTransportClient& m_client; 
	omni_mutex m_mutex;
	omni_condition m_event;
};

#endif /* _ASYNCTEXT_H_ */

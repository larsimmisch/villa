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

protected:
	
	// helper methods
	virtual void aborted();
	virtual void setState(states aState);
	
	virtual int doListen()  { event.signal(); return 0; }
	virtual int doConnect() { event.signal(); return 0; }
	
	TextTransportClient& getClient()	{ return client; }
	
	TextTransportClient& client; 
	omni_mutex mutex;
	omni_condition event;
};
#endif /* _ASYNCTEXT_H_ */

/*
	TCP.h
*/

#ifndef _TEXT_H_
#define _TEXT_H_

#include <iostream>
#include <vector>
#include <assert.h>
#include "socket.h"
#include "omnithread.h"
#include "client.h"

// The streambuf implementation uses its own buffers instead of the buffers
// in basic_streambuf - maybe this could be done more elegantly

class TextTransport : public std::basic_iostream<char>,
					  public std::basic_streambuf<char>,
					  public omni_thread
{
public:

	typedef std::basic_streambuf<char>::int_type int_type;

	enum states  { idle, calling, listening, connecting, connected, disconnecting, dying };

    enum { indefinite = -1 };
		
	TextTransport(TextTransportClient& client, void* userData = 0);
	virtual ~TextTransport();
	
	// Connection establishment 
	virtual int listen(SAP& aLocalSAP, int single = 0);
	virtual int connect(SAP& aRemoteSAP);
	
	// must be called by client after  a connectIndication
	virtual void accept();
	virtual void reject();
	
	// Dissolve a connection
	virtual void disconnect();
	
	// must be called by client after a disconnectRequest
	virtual void disconnectAccept();
	
	// Dissolve a connection rapidly
	virtual void abort();

	states getState()	{ return m_state; }
	int isConnected()	{ return m_state == connected; }
	int isDisconnecting()	{ return m_state == disconnecting; }
	int isIdle()		{ return m_state == idle; }
	void* getuserData()	{ return m_userData; }

    SAP& getLocalSAP()  { return m_local; }
    SAP& getRemoteSAP() { return m_remote; }

	virtual void run();

	void lock() { m_mutex.lock(); }
	void unlock() { m_mutex.unlock(); }

	// lock the mutex. unlocking is done via the io manipulator end()
	TextTransport &begin()
	{
		lock();

		clear();
	
		return *this;
	}

	// streambuf protocol

	virtual int_type overflow(int_type c)
	{
		m_pbuf += c;
		
		return c;
	}

	virtual std::streamsize xsputn(const char *s, std::streamsize l)
	{
		m_pbuf += s;

		return l;
	}

	virtual int_type underflow()
	{
		if (m_gpos >= m_gsize)
			return std::char_traits<char>::eof();

		return m_gbuf[m_gpos];
	}

	virtual int_type uflow()
	{
		if (m_gpos >= m_gsize)
			return std::char_traits<char>::eof();

		return m_gbuf[m_gpos++];
	}

	virtual std::streamsize xsgetn(char *s, std::streamsize l)
	{
		if (m_gpos >= m_gsize)
			return 0;

		int m = l > m_gsize - m_gpos ? m_gsize - m_gpos : l;

		memcpy(s, &m_gbuf[m_gpos], m);

		m_gpos += m;

		return m;
	}

	virtual unsigned send();
	virtual unsigned receive();	

protected:
	
	friend std::ostream& text_end(std::ostream& s);

	// helper methods

	// fills the streambuf get area from the receive buffer
	unsigned fillGBuf();
	
	virtual void assertState(states aState);

	virtual void aborted();
	
	virtual int doListen();
	virtual int doConnect();
	
	// helper methods
	virtual void setState(states aState);
		
	TextTransportClient& getClient()	{ return m_client; }

	volatile states m_state;
	SAP m_local;
	SAP m_remote;
	void* m_userData;
	Socket m_socket;
	// streambuf put area
	std::string m_pbuf;
	// streambuf get area
	char *m_gbuf;
	int m_gpos;
	int m_gsize;
	int m_gmax;
	// receive buffer
	char *m_rbuf;
	int m_rpos;
	int m_rsize;
	int m_rmax;
	TextTransportClient& m_client; 
	omni_mutex m_mutex;
	omni_condition m_event;
};

struct textmanip
{
	textmanip(std::ostream& (*f)(std::ostream&)) 
		: function(f) {}

	std::ostream& (*function)(std::ostream&);
};

inline std::ostream& operator<<(std::ostream& os, textmanip& l)
{
	return l.function(os);
}

// helper
inline std::ostream& text_end(std::ostream& s)
{
	TextTransport &t = dynamic_cast<TextTransport&>(s);

	t << "\r\n";

	t.send();

	t.unlock();

	return s;
}

inline textmanip end()
{
	return textmanip(text_end);
}

#endif /* _TEXT_H_ */

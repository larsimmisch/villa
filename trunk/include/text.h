/*
	TCP.h
*/

#ifndef _TEXT_H_
#define _TEXT_H_

#include "iostream"
#include "socket.h"
#include "vector"

// The streambuf implementation uses its own buffers instead of the buffers
// in basic_streambuf - maybe this could be done more elegantly

class TextTransport : public std::basic_iostream<char>,
					  public std::basic_streambuf<char> 
{
public:

	typedef std::basic_streambuf<char>::int_type int_type;

	enum states  { idle, calling, listening, connecting, connected, disconnecting, dying };

    enum { indefinite = -1 };
		
	TextTransport(void* aPrivateData = 0);
	virtual ~TextTransport();
	
	// Connection establishment 
	virtual int listen(SAP& aLocalSAP, unsigned aTimeout = indefinite, int single = 0);
	virtual int connect(SAP& aRemoteSAP, unsigned aTimeout = indefinite);
	
	// must be called by client after  a connectIndication
	virtual void accept();
	virtual void reject();
	
	// Dissolve a connection
	virtual void disconnect(unsigned aTimeout = indefinite);
	
    // convenience disconnect method. returns if disconnected or timeout
    virtual int disconnectAndWait(unsigned aTimeout = indefinite);

	// must be called by client after a disconnectRequest
	virtual void disconnectAccept();
	
	// Dissolve a connection rapidly
	virtual void abort();

	virtual void fatal(char* error);

	states getState()	{ return m_state; }
	int isConnected()	{ return m_state == connected; }
	int isDisconnecting()	{ return m_state == disconnecting; }
	int isIdle()		{ return m_state == idle; }
	void* getPrivateData()			{ return m_privateData; }

    SAP& getLocalSAP()  { return m_local; }
    SAP& getRemoteSAP() { return m_remote; }

	// lock the mutex. unlocking is done via the io manipulator end()
	TextTransport &begin()
	{
		lock();

		return *this;
	}

	// streambuf protocol

	virtual int_type overflow(int_type c)
	{
		if (c == '\n'
			&& m_pbuf.back() == '\r')
		{
			m_pbuf.push_back(c);

			unsigned rc = sendRaw(&m_pbuf.front(), m_pbuf.size());
			if (rc == 0)
				return std::char_traits<char>::eof();

			m_pbuf.erase(m_pbuf.begin(), m_pbuf.end());

			int x = m_pbuf.capacity();
		}
		else
			m_pbuf.push_back(c);
		
		return c;
	}
/*
	virtual std::streamsize xsputn(char *s, std::streamsize l)
	{
		return 0;
	}
*/
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

	virtual void lock() {}
	virtual void unlock() {}

protected:
	
	// helper methods
	virtual void assertState(states aState);
	virtual unsigned receiveRaw(unsigned aTimeout = indefinite);
	
	virtual unsigned sendRaw(const char* data, unsigned size, 
		unsigned aTimeout = indefinite, int expedited = 0);

	virtual void aborted();
	
	virtual int doListen();
	virtual int doConnect();
	
	virtual void setState(states aState) { m_state = aState; }

	volatile states m_state;
	SAP m_local;
	SAP m_remote;
	volatile unsigned m_timeout;
	void* m_privateData;
	Socket m_socket;
	std::vector<char> m_pbuf;
	char *m_gbuf;
	int m_gpos;
	int m_gsize;
	int m_gmax;
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
	s << "\r\n";

	TextTransport &t = dynamic_cast<TextTransport&>(s);

	t.unlock();

	return s;
}

inline textmanip end()
{
	return textmanip(text_end);
}

#endif /* _TEXT_H_ */

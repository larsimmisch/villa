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

	states getState()	{ return state; }
	int isConnected()	{ return state == connected; }
	int isDisconnecting()	{ return state == disconnecting; }
	int isIdle()		{ return state == idle; }
	void* getPrivateData()			{ return privateData; }

    SAP& getLocalSAP()  { return local; }
    SAP& getRemoteSAP() { return remote; }

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

protected:
	
	// helper methods
	virtual void assertState(states aState);
	virtual unsigned receiveRaw(unsigned aTimeout = indefinite);
	
	virtual unsigned sendRaw(const char* data, unsigned size, 
		unsigned aTimeout = indefinite, int expedited = 0);

	virtual void aborted();
	
	virtual int doListen();
	virtual int doConnect();
	
	virtual void setState(states aState) { state = aState; }

	volatile states state;
	SAP local;
	SAP remote;
	volatile unsigned timeout;
	void* privateData;
	Socket socket;
	std::vector<char> m_pbuf;
	char *m_gbuf;
	int m_gpos;
	int m_gsize;
	int m_gmax;
};

void endl(TextTransport& t);

#endif /* _TEXT_H_ */

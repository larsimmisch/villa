/*
	Transport.h
*/

#ifndef _TRANSPORT_H_
#define _TRANSPORT_H_

#include "Transport\Packet.h"
#include "Transport\SAP.h"

#ifndef _export
#define _export	__declspec( dllexport )
#endif

class _export Transport
{
	public:
	
	enum states  { idle, calling, listening, connecting, connected, disconnecting, dying };
	enum packets { t_connect_request, t_connect_confirm, t_connect_reject, t_disconnect_request, 
				   t_disconnect_confirm, t_disconnect_reject, t_abort, t_last };

    enum { indefinite = -1 };
	
	Transport(void* aPrivateData = 0) : state(idle), privateData(aPrivateData) {}
	virtual ~Transport() {}
	
	// Connection establishment 
	virtual Packet* listen(SAP& aLocalSAP, unsigned aTimeout, int single = 0) = 0;
	virtual Packet* connect(SAP& aRemoteSAP, unsigned aTimeout, Packet* initialPacket = 0) = 0;
	
	// must be called by client after  a t_connect_request
	virtual void accept(Packet* aPacket = 0) = 0;
	virtual void reject(Packet* aPacket = 0) = 0;
	
	// Dissolve a connection
	virtual void disconnect(unsigned aTimeout = indefinite, Packet* aPacket = 0) = 0;
	
	// must be called by client after a disconnectRequest
	virtual void disconnectAccept(Packet* aPacket = 0) = 0;
	virtual void disconnectReject(Packet* aPacket = 0) = 0;
	
	// Dissolve a connection rapidly
	virtual void abort(Packet* aPacket = 0) = 0;

	// Data transfer
	virtual int send(Packet& aPacket, unsigned aTimeout = indefinite, int expedited = 0) = 0;
	virtual Packet* receive(unsigned aTimeout = indefinite) = 0;
	
	virtual states getState()	{ return state; }
	int isConnected()			{ return getState() == connected; }
	int isDisconnecting()		{ return getState() == disconnecting; }
	int isIdle()				{ return getState() == idle; }
	void* getPrivateData()		{ return privateData; }

    SAP& getLocalSAP()  { return local; }
    SAP& getRemoteSAP() { return remote; }
	
	protected:
	
	virtual void setState(states aState) { state = aState; }
	
	volatile states state;
	SAP local;
	SAP remote;
	volatile unsigned timeout;
	void* privateData;
};
#endif /* _TRANSPORT_H_ */

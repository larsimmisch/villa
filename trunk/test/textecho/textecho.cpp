#include <string>

#include "text.h"
#include "transport.h"
#include "client.h"

class Server : public TextTransportClient
{
public:

	// must call server.accept or server.reject
	virtual void connectRequest(TextTransport *server, SAP& remote)
	{
		server->accept();
	}

	virtual void connectRequestTimeout(TextTransport *server) {}
	
	// replies to server.connect from far end
	virtual void connectConfirm(TextTransport *server) {}
	virtual void connectReject(TextTransport *server) {}
	virtual void connectTimeout(TextTransport *server) {}
	
	// must call server.disconnectAccept or server.disconnectReject 
	virtual void disconnectRequest(TextTransport *server)
	{
		server->disconnectAccept();
	}
	
	// replies to server.disconnect from far end
	virtual void disconnectConfirm(TextTransport *server) {}
	virtual void disconnectReject(TextTransport *server) {}
	virtual void disconnectTimeout(TextTransport *server) {}
	
	// sent whenever packet is received
	virtual void dataReady(TextTransport *server) {}
 
	// flow control
	virtual void stopSending(TextTransport *server) {}
	virtual void resumeSending(TextTransport *server) {}
	
	// miscellaneous
	
	virtual void abort(TextTransport *server) 
	{
		SAP local;

		local.setService(2001);

		server->listen(local);
	}
	
	virtual void data(TextTransport *server) 
	{
		std::string x;

		(*server) >> x;

		(*server) << x.c_str() << "\r\n";
	}

    virtual void fatal(const char* e) { std::cerr << e << std::endl; }
    virtual void fatal(Exception& e)  { std::cerr << e; }
};

int main(int argc, char* argv[])
{
	Socket::init();

	SAP local;

	local.setService(2001);

	Server server;

	AsyncText server_connection(server);

	server_connection.listen(local);

	server_connection.run();

	return 0;
}
/*
	Socket.cpp
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "datetime.h"
#include "socket.h"
#include "omnithread.h"

Services Socket::services(3);

HMODULE hWinsock;

InvalidAddress::InvalidAddress(
		const char* fileName, 
		int lineNumber,
		const char* function,
		SAP& anAddress,
		const Exception* prev)
 : Exception(fileName, lineNumber, function, "invalid address", prev), address(anAddress)
{
}

void InvalidAddress::printOn(std::ostream& os) const
{
	Exception::printOn(os);

	os << " offending address: " << address;
}

SocketError::SocketError(
	const char* fileName,
	int lineNumber,
	const char* function,
	unsigned long anError,
	Exception* prev)
 : OSError(fileName,lineNumber,function,name(anError),prev),
   error(anError)
{
}

SocketError::~SocketError()
{
	LocalFree((void*)Exception::m_name);
}

const char* SocketError::name(unsigned long error)
{
	char* aName;
	unsigned size;

	size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
						hWinsock,
						error,
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPTSTR) &aName, 0, NULL);

	if (size) return aName;

	aName = (char*)LocalAlloc(LMEM_FIXED, 32);

	sprintf(aName, "unknown error: %d", error);

	return aName;
} 

ListenerQueue::ListenerQueue()
{
}

ListenerQueue::~ListenerQueue()
{
}

ListenerQueue::Item* ListenerQueue::enqueue(Listener* aListener)
{	
	omni_mutex_lock lock(Socket::services.mutex);

	Item* item = new Item(aListener);

	addLast(item);

	return item;
}

ListenerQueue::Item* ListenerQueue::dequeue()
{
	omni_mutex_lock lock(Socket::services.mutex);

	Item* item = (Item*)removeFirst();

	return item;
}

void ListenerQueue::cancel(Item* item)
{
	omni_mutex_lock lock(Socket::services.mutex);

	if (!item) return;

	item->result = WSAEINTR;
	item->event.signal();

	remove(item);
}

void ListenerQueue::freeLink(List::Link* aLink)
{
	delete (Item*)aLink;
}

Listener::Listener(int aProtocol, SAP& aService) 
: Services::Key(aProtocol, 0), queue(), hsocket(-1)
{
	struct sockaddr_in localAddress;
	int len;
	int rc;

	Socket::fillSocketAddress(aService, &localAddress);

	hsocket = ::socket(protocol, SOCK_STREAM, 0);
	if (hsocket < 0)	throw SocketError(__FILE__, __LINE__, "Listener::Listener(int,SAP&)", GetLastError());

	localAddress.sin_family = aProtocol;
	rc = ::bind(hsocket, (sockaddr*)&localAddress, sizeof(localAddress));
	if (rc < 0) 	throw SocketError(__FILE__, __LINE__, "Listener::Listener(int,SAP&)", GetLastError());

	if (localAddress.sin_port == 0)
	{
		len = sizeof(localAddress);
		rc = ::getsockname(hsocket, (sockaddr*)&localAddress, &len);
		Socket::fillSAP(&localAddress, aService);
	}

	service = htons(localAddress.sin_port);

	rc = ::listen(hsocket, 5);
	if (rc < 0) 	throw SocketError(__FILE__, __LINE__, "Listener::Listener(int,SAP&)", GetLastError());

	start_undetached();
}

Listener::~Listener()
{
	closesocket(hsocket);
}

void *Listener::run_undetached(void *arg)
{
	int aSocket;
	int size;
	unsigned result;
	ListenerQueue::Item* item;
	struct sockaddr_in remoteAddress;

	while(1)
	{	
		size = sizeof(remoteAddress);
		aSocket = ::accept(hsocket, (sockaddr*)&remoteAddress, &size);
		if (aSocket < 0)
		{
			result = GetLastError();
			if (result == WSAENOTSOCK) 
			{
				return NULL;
			}
		}
		else result = 0;

		Socket::services.lock();
		item = queue.dequeue();
		if (item)
		{
			if (result == 0) Socket::fillSAP(&remoteAddress, item->sap);
			item->result = result;
			item->hsocket = aSocket;
			item->event.signal();
		}
		else
		{
			closesocket(aSocket);
		}
		Socket::services.unlock();
	}
}

ListenerQueue::Item* Services::add(int protocol, SAP& aService)
{
	Listener* listener;
	ListenerQueue::Item* item;
	omni_mutex_lock lock(mutex);

	listener = aService.getService() ? contains(protocol, atoi(aService.getService())) : 0;
	if (listener)
	{
		item = listener->queue.enqueue(listener);
	}
	else
	{
		listener = new Listener(protocol, aService);
		item = listener->queue.enqueue(listener);

		basicAdd(listener);
	}

	return item;
}

void Services::remove(ListenerQueue::Item* item)
{
	omni_mutex_lock lock(mutex);

	item->listener->queue.cancel(item);

	item->listener = 0;
}

void Services::remove(Listener* listener)
{
	omni_mutex_lock lock(mutex);

	basicRemoveAt(listener);
	
	delete listener;
}

Listener* Services::contains(int aProtocol, int aService)
{ 
	omni_mutex_lock lock(mutex);

	Key key(aProtocol, aService);

	return (Listener*)basicAt(&key); 
}

unsigned Services::hashAssoc(List::Link* anItem)
{ 
	unsigned value = ((Listener*)anItem)->protocol;
	value += ((Listener*)anItem)->service;
	
	return value;
}

void Services::empty()
{ 
	for(AssocIter i(*this);!i.isDone();i.next())	delete (Listener*)i.current(); 
}

Socket::Socket(int aProtocol) 
 : nonblocking(0), protocol(aProtocol), hsocket(-1), waiting(0), listener(0)
{
}

Socket::~Socket()
{
	close();
}

void Socket::close()
{
	if (waiting)
	{
		services.remove(waiting);
	}

	if (listener && listener->queue.getSize() == 0)
	{
		services.remove(listener);
	}

	if (hsocket != -1)
	{
		int rc = closesocket(hsocket);

		if (rc < 0) SocketError(__FILE__, __LINE__, "Socket::~Socket()", GetLastError());
	}

	hsocket = -1;
	waiting = 0;
}

void Socket::bind(SAP& local, int single)
{
	if (single)
	{
		struct sockaddr_in localAddress;
		int len;
		int rc;

		Socket::fillSocketAddress(local, &localAddress);

		hsocket = ::socket(protocol, SOCK_STREAM, 0);
		if (hsocket < 0)	throw SocketError(__FILE__, __LINE__, "Socket::bind(SAP&,int)", GetLastError());

		localAddress.sin_family = protocol;
		rc = ::bind(hsocket, (sockaddr*)&localAddress, sizeof(localAddress));
		if (rc < 0) 	throw SocketError(__FILE__, __LINE__, "Socket::bind(SAP&,int)", GetLastError());

		if (localAddress.sin_port == 0)
		{
			len = sizeof(localAddress);
			rc = ::getsockname(hsocket, (sockaddr*)&localAddress, &len);
			Socket::fillSAP(&localAddress, local);
		}
	}
	else
	{
		waiting = services.add(protocol, local);
		listener = waiting->listener;
	}
}

int Socket::listen(SAP& remote, unsigned aTimeout)
{
	DateTime now, later;
	unsigned result;
	
	if (aTimeout != infinite) now.now();

	if (waiting)
	{
		int signalled = 1;

		waiting->mutex.lock();

		if (aTimeout == infinite)
			waiting->event.wait();
		else
		{
			unsigned long abs_sec, abs_nsec;

			omni_thread::get_time(&abs_sec, &abs_nsec, 
				aTimeout / 1000, (aTimeout % 1000) * 1000000);

			signalled = waiting->event.timedwait(abs_sec, abs_nsec);
		}

		waiting->mutex.unlock();

		if (signalled)
		{
			hsocket = waiting->hsocket;
			result = waiting->result;
			delete waiting;
			waiting = 0;

			if (hsocket < 0)
			{
				if (result == WSAEINTR) return 0;
				else throw SocketError(__FILE__, __LINE__, "Socket::listen(SAP&,SAP&,int)", result);
			}
		}
		else
		{
			services.remove(waiting);
			delete waiting;

			waiting = 0;
		}
	}
	else
	{
		// just one thread will service this connection

		struct sockaddr_in remoteAddress;
		int aSocket;
		int size;

		result = ::listen(hsocket, 1);
		if (result < 0) 	throw SocketError(__FILE__, __LINE__, "Listener::Listener(int,SAP&)", GetLastError());

		if (aTimeout != infinite)
		{
			if (!waitForData(aTimeout)) 
				return 0;
		}

		size = sizeof(remoteAddress);
		aSocket = ::accept(hsocket, (sockaddr*)&remoteAddress, &size);
		if (aSocket < 0)
		{
			result = GetLastError();
			if (result == WSAENOTSOCK || result == WSAEINTR) 
				return 0;
			else throw SocketError(__FILE__,__LINE__, "Socket::listen(SAP&, unsigned)", result);
		}

		closesocket(hsocket);
		hsocket = aSocket;

		fillSAP(&remoteAddress, remote);
	}

	if (aTimeout != infinite)
	{
		later.now();

		unsigned delta = aTimeout - (later - now);

		return delta < 0 ? 0 : delta;
	}
	else
	{
		return infinite;
	}
}

int Socket::connect(SAP& remote, unsigned aTimeout)
{
	struct sockaddr_in remoteAddress;

	hsocket = socket(protocol, SOCK_STREAM, 0);
	if (hsocket < 0)	SocketError(__FILE__, __LINE__, "Socket::connect(SAP&,int)", GetLastError());

	fillSocketAddress(remote, &remoteAddress);

	if (aTimeout != -1)    setNonblocking(1);

	int rc = ::connect(hsocket, (sockaddr*)&remoteAddress, sizeof(remoteAddress));
	if (rc < 0) 
	{
		// we expect EWOULDBLOCK, but handle the other cases as well

		rc = GetLastError();
		switch(rc)
		{
		case WSAECONNREFUSED:	
			return 0;
		case WSAENETUNREACH:	
			return 0;
		case WSAECONNRESET: 	
			return 0;
		case WSAEINPROGRESS:	break;
		case WSAEWOULDBLOCK:	break;
		default: throw SocketError(__FILE__, __LINE__, "Socket::connect(SAP&,int)", GetLastError());
		}
	}

	aTimeout = waitForSend(aTimeout);
	
	return aTimeout;
}

int Socket::send(void* data, unsigned dataLength, int expedited)
{
	int rc = ::send(hsocket, (char*)data, dataLength, expedited ? MSG_OOB : 0);
	if (rc < 0) 
	{
		rc = GetLastError();
		return 0;
	}
	
	return rc;
}

int Socket::receive(void* data, unsigned dataLength)
{
	int rc = ::recv(hsocket, (char*)data, dataLength, 0);
	if (rc < 0)
	{
		rc = GetLastError();
		return 0;
	}

	return rc;
}


int Socket::waitForData(unsigned aTimeout)
{	
	fd_set read;
	struct timeval timeout;

	// if the timeout is indefinite, we make the socket blocking
	// we hope for better performance (less system calls)
	if (aTimeout == infinite)
	{
		if (nonblocking) setNonblocking(0);
		return infinite;
	}	
	else 
	if (!nonblocking)  setNonblocking(1);

	if (hsocket == -1)	return 0;
	
	int delta;
	DateTime now, later;

	now.now();
	
	FD_ZERO(&read);
	FD_SET(hsocket, &read);

	timeout.tv_sec = aTimeout / 1000;
	timeout.tv_usec = (aTimeout % 1000) * 1000;

	int selected = select(1, &read, 0, 0, &timeout);

	later.now();

	delta = aTimeout - (later - now);
	delta = delta < 0 ? 0 : delta;

	// if we get ENOTSOCK, another thread has destroyed this socket. return 0.
	if (selected < 0 && GetLastError() == WSAENOTSOCK)	
	{
		return delta;
	}

	if (selected < 0) throw SocketError(__FILE__, __LINE__, "Socket::waitForData(unsigned)", GetLastError());

	return delta;
}

int Socket::waitForSend(unsigned aTimeout)
{				 
	if (hsocket == -1)	return 0;

	fd_set write;
	struct timeval timeout;

	// if the timeout is infinite, we make the socket blocking
	// we hope for better performance (less system calls)
	if (aTimeout == infinite)
	{
		if (nonblocking) setNonblocking(0);
		return infinite;
	}	
	else 
	{
		if (!nonblocking)  setNonblocking(1);
	}

	int delta;
	DateTime now, later;

	now.now();

	FD_ZERO(&write);
	FD_SET(hsocket, &write);

	timeout.tv_sec = aTimeout / 1000;
	timeout.tv_usec = (aTimeout % 1000) * 1000;

	int selected = select(0, 0, &write, 0, &timeout);
	
	// if we get ENOTSOCK, another thread has destroyed this socket. return 0.
	if (selected < 0 && GetLastError() == WSAENOTSOCK)	return 0;

	if (selected < 0) throw SocketError(__FILE__, __LINE__, "Socket::waitForSend(unsigned)", GetLastError());

	later.now();

	if (aTimeout == infinite)	return infinite;
	else delta = aTimeout - (later - now);
	
	return delta < 0 ? 0 : delta;
}

void Socket::setReceiveQueueLength(unsigned aLength)
{
	int rc = setsockopt(hsocket, SOL_SOCKET, SO_RCVBUF, (char*)&aLength, sizeof(aLength));
	if (rc < 0) throw SocketError(__FILE__, __LINE__, "Socket::setReceiveQueueLength(unsigned)", GetLastError());
}

void Socket::setSendQueueLength(unsigned aLength)
{	
	int rc = setsockopt(hsocket, SOL_SOCKET, SO_SNDBUF, (char*)&aLength, sizeof(aLength));
	if (rc < 0) throw SocketError(__FILE__, __LINE__, "Socket::setSendQueueLength(unsigned)", GetLastError());
}

void Socket::setLingerTimeout(unsigned aTimeout)
{	
	struct linger l;
	
	if (aTimeout == 0)	l.l_onoff = 0;
	else
	{
		l.l_onoff = 1;
		l.l_linger = aTimeout / 1000; // l_linger is seconds.
	}
	
	int rc = setsockopt(hsocket, SOL_SOCKET, SO_LINGER, (char*)&l, sizeof(l));
	if (rc < 0) throw SocketError(__FILE__, __LINE__, "Socket::setLingerTimeout(unsigned)", GetLastError());
}

void Socket::setNoDelay(int on)
{

	int rc = setsockopt(hsocket, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on));
	if (rc < 0) throw SocketError(__FILE__, __LINE__, "Socket::setNoDelay(int)", GetLastError());
}

void Socket::setKeepAlive(int on)
{
	int rc = setsockopt(hsocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&on, sizeof(on));
	if (rc < 0) throw SocketError(__FILE__, __LINE__, "Socket::setKeepAlive(int)", GetLastError());
}

void Socket::setNonblocking(int on)
{
	nonblocking = on;

	int rc = ioctlsocket(hsocket, FIONBIO, (u_long *)&on);
	if (rc < 0) throw SocketError(__FILE__, __LINE__, "Socket::setNonBlocking(int)", GetLastError());
}

unsigned Socket::bytesPending()
{
	unsigned size;

	int rc = ioctlsocket(hsocket, FIONREAD, (u_long*)&size);
	if (rc < 0) throw SocketError(__FILE__, __LINE__, "Socket::setNonBlocking(int)", GetLastError());

	return size;
}

void Socket::setReuseAddress(int on)
{
	int rc = setsockopt(hsocket, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
	if (rc < 0) throw SocketError(__FILE__, __LINE__, "Socket::setReuseAddress(int)", GetLastError());
}

int Socket::rejected()
{
	return GetLastError() == WSAECONNRESET || GetLastError() == WSAENETUNREACH || GetLastError() == WSAECONNREFUSED;
}

void Socket::getName(SAP& name)
{
	struct sockaddr_in localAddress;
	int len = sizeof(localAddress);

	int rc = ::getsockname(hsocket, (sockaddr*)&localAddress, &len);
	if (rc < 0) throw SocketError(__FILE__, __LINE__, "Socket::getName(SAP&)", GetLastError());

	Socket::fillSAP(&localAddress, name);
}

void Socket::getPeerName(SAP& name)
{
	struct sockaddr_in localAddress;
	int len = sizeof(localAddress);

	int rc = ::getpeername(hsocket, (sockaddr*)&localAddress, &len);
	if (rc < 0) throw SocketError(__FILE__, __LINE__, "Socket::getPeerName(SAP&)", GetLastError());

	Socket::fillSAP(&localAddress, name);
}

void Socket::fillSocketAddress(SAP& aSAP, void* anAddress)
{	
	struct in_addr inAddress;
	struct sockaddr_in *socketAddress = (struct sockaddr_in*)anAddress;
	struct hostent* host;
	struct servent* serv;
	
	if (anAddress == 0) return;
	
	/* written while neural transmitters were being shut down */
	if (aSAP.getAddress())
	{
		inAddress.s_addr = inet_addr(aSAP.getAddress());
		if (inAddress.s_addr == -1)
		{
			host = gethostbyname(aSAP.getAddress());
			if (host == 0)
			{
				throw InvalidAddress(__FILE__,__LINE__,"Socket::fillSocketAddress(SAP&,void*)", aSAP);
			}
			memcpy(&inAddress.s_addr, *host->h_addr_list, sizeof(inAddress));
			if (inAddress.s_addr == -1) 
			{
				throw InvalidAddress(__FILE__,__LINE__,"Socket::fillSocketAddress(SAP&,void*)", aSAP);
			}
		}
	}
	else inAddress.s_addr = INADDR_ANY;

	socketAddress->sin_family = PF_INET;
	socketAddress->sin_addr = inAddress;
	
	if (aSAP.getService()) 
	{
		socketAddress->sin_port = htons(atoi(aSAP.getService()));
		if (socketAddress->sin_port == 0)
		{
			serv = getservbyname(aSAP.getService(), "tcp");
			if (serv == 0)
			{
				throw InvalidAddress(__FILE__,__LINE__,"Socket::fillSocketAddress(SAP&,void*)", aSAP);
			}
			socketAddress->sin_port = serv->s_port;
		}
	}
	else socketAddress->sin_port = 0;

	return;
}

void Socket::fillSAP(void* socketAddress, SAP& aSAP)
{
	struct sockaddr_in* address = (sockaddr_in*)socketAddress;
	
	if (address->sin_addr.S_un.S_addr != INADDR_ANY) aSAP.setAddress(inet_ntoa(address->sin_addr));
	aSAP.setService(htons(address->sin_port));
}

void Socket::init()
{
	WSADATA wsa;

	DWORD rc = WSAStartup(MAKEWORD(2,0), &wsa);

	if (rc != 0)
		throw SocketError(__FILE__, __LINE__, "WSAStartup", rc);
}

extern HMODULE hWinsock;

extern "C" 
{ 
	BOOL WINAPI _CRT_INIT( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved ); 
}

BOOL WINAPI DllMain(HINSTANCE  hDLL, DWORD	reason, LPVOID reserved)
{
	ULONG rc;
	WSADATA wsa;

	switch (reason)
	{

	case DLL_PROCESS_ATTACH:
		_CRT_INIT(hDLL, reason, reserved);

		// get the handle of wsock32.dll to get error messages from it
		hWinsock = LoadLibrary("wsock32.dll");

		rc = WSAStartup(MAKEWORD(2,0), &wsa);

		if (rc != 0)
		{
			printf("WSAStartup failed: %d\n", rc);
			return FALSE;
		}
		break;
	case DLL_PROCESS_DETACH:
		rc = WSACleanup();

		FreeLibrary(hWinsock);

		_CRT_INIT(hDLL, reason, reserved);
		break;
	default:
		break;
	}

	return TRUE;
}

/*
	Socket.cpp
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "datetime.h"
#include "socket.h"
#include "omnithread.h"

HMODULE hWinsock;

InvalidAddress::InvalidAddress(
		const char* fileName, 
		int lineNumber,
		const char* function,
		const SAP& anAddress,
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

Socket::Socket(int protocol, int s) 
 : m_protocol(protocol), m_socket(s), m_nonblocking(0)
{
	if (s == -1)
	{
		m_socket = ::socket(m_protocol, SOCK_STREAM, 0);
		if (m_socket < 0)	throw SocketError(__FILE__, __LINE__, "Socket::Socket()", GetLastError());
	}
}

Socket::~Socket()
{
	close();
}

void Socket::close()
{
	if (m_socket != -1)
	{
		int rc = closesocket(m_socket);

		if (rc < 0) SocketError(__FILE__, __LINE__, "Socket::~Socket()", GetLastError());
	}
	m_socket = -1;
}

void Socket::bind(SAP& local)
{
	struct sockaddr_in localAddress;
	int len;
	int rc;

	Socket::fillSocketAddress(local, &localAddress);
	m_local = local;

	localAddress.sin_family = m_protocol;
	rc = ::bind(m_socket, (sockaddr*)&localAddress, sizeof(localAddress));
	if (rc < 0) 	throw SocketError(__FILE__, __LINE__, "Socket::bind(SAP&,int)", GetLastError());

	if (localAddress.sin_port == 0)
	{
		len = sizeof(localAddress);
		rc = ::getsockname(m_socket, (sockaddr*)&localAddress, &len);
		Socket::fillSAP(&localAddress, local);
	}
}

void Socket::listen(int backlog)
{
	unsigned result = ::listen(m_socket, backlog);
	if (result < 0) 	throw SocketError(__FILE__, __LINE__, "Listener::Listener(int,SAP&)", GetLastError());
}

int Socket::accept(SAP &remote)
{
	struct sockaddr_in remoteAddress;
	int size = sizeof(remoteAddress);
	int s = ::accept(m_socket, (sockaddr*)&remoteAddress, &size);
	if (s < 0)
		throw SocketError(__FILE__,__LINE__, "Socket::listen(SAP&, unsigned)", GetLastError());

	fillSAP(&remoteAddress, remote);

	return s;
}

void Socket::connect(const SAP& remote)
{
	struct sockaddr_in remoteAddress;

	m_socket = socket(m_protocol, SOCK_STREAM, 0);
	if (m_socket < 0)	SocketError(__FILE__, __LINE__, "Socket::connect(SAP&,int)", GetLastError());

	fillSocketAddress(remote, &remoteAddress);

	int rc = ::connect(m_socket, (sockaddr*)&remoteAddress, sizeof(remoteAddress));
	if (rc < 0) 
		throw SocketError(__FILE__, __LINE__, "Socket::connect(SAP&,int)", GetLastError());

	m_remote = remote;
}

int Socket::send(void* data, unsigned dataLength)
{
	int rc = ::send(m_socket, (char*)data, dataLength, 0);
	if (rc < 0) 
		throw SocketError(__FILE__, __LINE__, "Socket::send()", GetLastError());
	
	return rc;
}

int Socket::receive(void* data, unsigned dataLength)
{
	int rc = ::recv(m_socket, (char*)data, dataLength, 0);
	if (rc < 0)
	{
		rc = GetLastError();
		return 0;
	}

	return rc;
}

void Socket::setReceiveQueueLength(unsigned aLength)
{
	int rc = setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char*)&aLength, sizeof(aLength));
	if (rc < 0) throw SocketError(__FILE__, __LINE__, "Socket::setReceiveQueueLength(unsigned)", GetLastError());
}

void Socket::setSendQueueLength(unsigned aLength)
{	
	int rc = setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char*)&aLength, sizeof(aLength));
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
	
	int rc = setsockopt(m_socket, SOL_SOCKET, SO_LINGER, (char*)&l, sizeof(l));
	if (rc < 0) throw SocketError(__FILE__, __LINE__, "Socket::setLingerTimeout(unsigned)", GetLastError());
}

void Socket::setNoDelay(int on)
{

	int rc = setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on));
	if (rc < 0) throw SocketError(__FILE__, __LINE__, "Socket::setNoDelay(int)", GetLastError());
}

void Socket::setKeepAlive(int on)
{
	int rc = setsockopt(m_socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&on, sizeof(on));
	if (rc < 0) throw SocketError(__FILE__, __LINE__, "Socket::setKeepAlive(int)", GetLastError());
}


unsigned Socket::bytesPending()
{
	unsigned size;

	int rc = ioctlsocket(m_socket, FIONREAD, (u_long*)&size);
	if (rc < 0) throw SocketError(__FILE__, __LINE__, "Socket::bytesPending(int)", GetLastError());

	return size;
}

void Socket::setReuseAddress(int on)
{
	int rc = setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
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

	int rc = ::getsockname(m_socket, (sockaddr*)&localAddress, &len);
	if (rc < 0) throw SocketError(__FILE__, __LINE__, "Socket::getName(SAP&)", GetLastError());

	Socket::fillSAP(&localAddress, name);
}

void Socket::getPeerName(SAP& name)
{
	struct sockaddr_in localAddress;
	int len = sizeof(localAddress);

	int rc = ::getpeername(m_socket, (sockaddr*)&localAddress, &len);
	if (rc < 0) throw SocketError(__FILE__, __LINE__, "Socket::getPeerName(SAP&)", GetLastError());

	Socket::fillSAP(&localAddress, name);
}

void Socket::fillSocketAddress(const SAP& aSAP, void* anAddress)
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

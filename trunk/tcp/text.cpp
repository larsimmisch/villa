/*
	TCP.cpp
*/

#include <iostream>
#include <stdio.h>
#include <new.h>
#include "Text.h"

static char get_lost[] = "finger weg!\n";

void endl(TextTransport& t)
{
    static char crlf[] = "\r\n";
 
    t.send(crlf);
}

TextTransport::TextTransport(void* aPrivateData) : socket(), privateData(aPrivateData), state(idle)
{
}

TextTransport::~TextTransport()
{
	if (state == connected)	abort();
	
	socket.close();
}

void TextTransport::assertState(states aState)
{	
	if (aState != getState())
	{
		throw "invalid state";
	} 
}

int TextTransport::listen(SAP& aLocalSAP, unsigned aTimeout)
{	
	assertState(idle);
	
	local = aLocalSAP;
	timeout = aTimeout;

	setState(listening);
	
	return doListen();
}

int TextTransport::connect(SAP& aRemoteSAP, unsigned aTimeout)
{
	assertState(idle);
	
	remote = aRemoteSAP;
	timeout = aTimeout;
	
	setState(calling);
	
	return doConnect();
}

void TextTransport::accept()
{
	assertState(connecting);
	
	setState(connected);
}

void TextTransport::reject()
{
	assertState(connecting);

	setState(idle);

	socket.close();	
}

void TextTransport::disconnect(unsigned aTimeout)
{	
	timeout = aTimeout;

	switch (state)
    {
    case idle:
        return;
    case listening:
    case calling:
    case connected:
        socket.close();
        setState(idle);
        break;
    case disconnecting:
        disconnectAccept();
        break;
    default:
        throw "disconnect in invalid state";
    }
	
}

int TextTransport::disconnectAndWait(unsigned aTimeout)
{
    char* received;

    disconnect(aTimeout);

    while (state != idle)
    {
        received = receiveRaw(timeout);
        if (received == 0)  return 0;
        delete received;
    }

    return 1;
}

void TextTransport::disconnectAccept()
{
	assertState(disconnecting);
	
	setState(idle);
	
	socket.close();	
}

void TextTransport::abort()
{
	assertState(connected);
	
	setState(idle);

	socket.close();
}

int TextTransport::send(const char* aPacket, unsigned aTimeout, int expedited)
{
	if (state != connected)	
	{
		return 0;	
	}
	
	return sendRaw(aPacket, aTimeout, expedited);
}

int TextTransport::sendRaw(const char* aPacket, unsigned aTimeout, int expedited)
{
	int rc;
	unsigned sent = 0;
    unsigned size = strlen(aPacket);
	
	while (sent < size)
	{
		socket.waitForSend(aTimeout);
		rc = socket.send((void*)(aPacket + sent), size - sent, expedited);
		if (rc == 0)
		{	
			if (state == connected)  aborted();
			return 0;
		}
		
		sent += rc;
	}

    return 1;
}

TextTransport& TextTransport::operator<<(int i)
{
    char number[32];
 
    sprintf(number, "%d", i);

    send(number);

    return *this;
}

TextTransport& TextTransport::operator<<(char c)
{
    char cnull[2];
 
    cnull[0] = c;
    cnull[1] = 0;

    send(cnull);

    return *this;
}

char* TextTransport::receive(unsigned aTimeout)
{
	return receiveRaw(aTimeout);
}

char* TextTransport::receiveRaw(unsigned aTimeout)
{
	char* buffer;
	unsigned len = 0;
	unsigned rcvd;
	unsigned bufferSize = 255;

    buffer = new char[bufferSize];

	while(1)
	{
		if ((timeout = socket.waitForData(aTimeout)) == 0)	
		{
			delete buffer;
			return 0;
		}
	
		rcvd = socket.receive(&buffer[len], bufferSize - len);
		if (rcvd == 0)
		{
			if (state == connected)	aborted();
			return 0;
		}
		
		len += rcvd;
		
        if (len >= 2) for (unsigned i = 0; i < len-1; i++)
        {
            if (buffer[i] == '\r' && buffer[i+1] == '\n')
            {
                buffer[i] = '\0';
                return buffer;
            }
        }

        if (len == bufferSize)
        {
            char* temp;
            bufferSize += 255;
            temp = new char[bufferSize];
            strcpy(temp, buffer);
            delete buffer;

            buffer = temp;
        }
	}

    // will never come here
	return 0;
}

void TextTransport::aborted()
{
	setState(idle);
	
	socket.close();
}

void TextTransport::fatal(char* error)
{
	throw "TextTransport::fatal";
}

int TextTransport::doListen()
{
    while(1)
    {
   		timeout = socket.listen(remote, timeout);
    	if (timeout == 0)
    	{
    		setState(idle);
    		socket.close();
    		return 0;
    	}
        break;
	}
	socket.setLingerTimeout(2);
	socket.setNoDelay(1);
	socket.setKeepAlive(1);

	setState(connecting);

	return 1;
}

int TextTransport::doConnect()
{
	timeout = socket.connect(remote, timeout);
	if (timeout == 0)
	{
		setState(idle);
		socket.close();
		return 0;
	}

	socket.setLingerTimeout(2);
	socket.setNoDelay(1);
	socket.setKeepAlive(1);

	setState(connected);
    return 1;
}


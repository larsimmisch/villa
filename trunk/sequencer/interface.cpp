/*
	Copyright 1995-2001 Lars Immisch

	created: Mon Nov 18 17:09:12 GMT+0100 1996

	Author: Lars Immisch <lars@ibp.de>
*/

#include "omnithread.h"
#include "conference.h"
#include "rphone.h"
#include "sequence.h"
#include "configuration.h"
#include "interface.h"

extern int debug;

extern ClientQueue gClientQueue;

extern ConfiguredTrunks gConfiguration;

InterfaceConnection::InterfaceConnection(TransportClient& aClient, SAP& local) 
: AsyncTCPNoThread(aClient)
{
    listen(local);

    packet = new(buffer) Packet(0, sizeof(buffer));
}

void InterfaceConnection::sendConnectDone(unsigned syncMajor, unsigned syncMinor, unsigned result)
{
	omni_mutex_lock lock(mutex);
	
	Packet* reply = staticPacket(1);

	reply->setSync(syncMajor, syncMinor);

	reply->setUnsignedAt(0, result);
	send(*reply);
}

Interface::Interface(SAP& aLocal) : unused(1), local(aLocal)
{
	connections.addFirst(new InterfaceConnection(*this, local));
}

void Interface::run()
{
	((InterfaceConnection*)connections.getHead())->run();
}

void Interface::cleanup(Transport* server)
{
	// remove all listeners for the disconnected app

	// first in the global queue

	log(log_debug, "sequencer") << "client aborted" << logend();

	gClientQueue.remove(server);

	// then in all the trunks

	gConfiguration.lock();
	for (ConfiguredTrunksIterator t(gConfiguration); !t.isDone(); t.next())
	{
		t.current()->removeClient(server);
	}
	gConfiguration.unlock();

	// force close all conferences opened by this app
	/*
	gConferences.lock();
	for (ConferencesIterator c(gConferences); !c.isDone(); c.next())
	{
		if (c.current()->getUserData() == server)
		{
			gConferences.close(c.current()->getHandle(), 1);
		}
	}
	gConferences.unlock();
	*/
}

void Interface::connectRequest(Transport* server, SAP& remote, Packet* initialPacket)
{
	omni_mutex_lock lock(mutex);

	unused--;

	log(log_debug, "sequencer") << "client attached from " << remote << logend();

	if (unused == 0)
	{
		log(log_debug, "sequencer") << "spawning new interface listener" << logend();

		connections.addFirst(new InterfaceConnectionThread(*this, local));
		unused++;
	}

	if (initialPacket->getContent() == if_describe)
	{
		Packet* reply = ((InterfaceConnection*)server)->staticPacket(gConfiguration.numElements() * 3);

		reply->setSync(0, 0);
		reply->setContent(if_describe_done);

		unsigned index = 0;

		for (ConfiguredTrunksIterator c(gConfiguration); !c.isDone(); c.next(), index += 3)
		{
			reply->setStringAt(index, c.current()->getName());
			reply->setStringAt(index + 1, c.current()->getNumber());
			reply->setUnsignedAt(index + 2, c.current()->isDigital() ? 1 : 0);
		}

	    server->accept(reply);
	}
	else server->accept();

}

void Interface::connectRequestTimeout(Transport* server)
{
	SAP remote;

    server->listen(local, indefinite);
}

void Interface::connectConfirm(Transport* server, Packet* first)
{
	// we don't connect currently
}

void Interface::connectReject(Transport* server, Packet* first)
{
	// we don't connect currently
}

void Interface::connectTimeout(Transport* server)
{
	// we don't connect currently
}

void Interface::disconnectRequest(Transport* server, Packet* cause)
{
	SAP remote;

	omni_mutex_lock lock(mutex);
	
	unused++;

	server->disconnectAccept();

    server->listen(local, indefinite);

	cleanup(server);
}

void Interface::disconnectConfirm(Transport* server, Packet* last)
{}

void Interface::disconnectTimeout(Transport* server)
{}

void Interface::disconnectReject(Transport* server, Packet* aPacket)
{
    // We don't actively disconnect
}

void Interface::abort(Transport* server, Packet* finalPacket)
{
	SAP remote;

	omni_mutex_lock lock(mutex);
	
	unused++;

	cleanup(server);

    server->listen(local, indefinite);
}

void Interface::data(Transport* server, Packet* aPacket)
{
    // that's the main packet inspection method...
    Packet* reply;
	InterfaceConnection* ico = (InterfaceConnection*)server;

	omni_mutex_lock lock(ico->getMutex());

    switch(aPacket->getContent())
    {
    case if_open_conference:
	{
        reply = ico->staticPacket(2);
        reply->setContent(if_open_conference_done);
        reply->setSync(aPacket->getSyncMajor(), aPacket->getSyncMinor());

		Conference* conference = 0; //gConferences.create();

        reply->setUnsignedAt(0, conference ? _ok : _failed);
        if (conference) reply->setUnsignedAt(1, conference->getHandle());

        server->send(*reply);
        break;
	}
    case if_close_conference:
        reply = ico->staticPacket(1);
        reply->setContent(if_close_conference_done);
        reply->setSync(aPacket->getSyncMajor(), aPacket->getSyncMinor());

        // check parameters
        if (aPacket->typeAt(0) != Packet::type_unsigned 
		 || 0 /*!gConferences[aPacket->getUnsignedAt(0)]*/)
        {
			reply->setUnsignedAt(0, _failed);
			server->send(*reply);
            break;
        }

		// gConferences.close(aPacket->getUnsignedAt(0));
		reply->setUnsignedAt(0, _not_implemented);
		server->send(*reply);
		break;
	case if_listen:
	{
        reply = ico->staticPacket(1);
        reply->setContent(if_listen_done);
        reply->setSync(aPacket->getSyncMajor(), aPacket->getSyncMinor());
		
		if (aPacket->typeAt(0) != Packet::type_string
		 || aPacket->typeAt(1) != Packet::type_string
		 || aPacket->typeAt(2) != Packet::type_string
		 || aPacket->typeAt(3) != Packet::type_string)
		{
			reply->setUnsignedAt(0, _invalid);
			server->send(*reply);

			break;
		}

		TrunkConfiguration* trunk = 0;
		SAP client, detail;
		
		trunk = gConfiguration[aPacket->getStringAt(0)];
		detail.setService(aPacket->getStringAt(1));

		client.setAddress(aPacket->getStringAt(2));
		client.setService(aPacket->getStringAt(3));

		if (trunk)
		{
			log(log_debug, "sequencer") << "client " << client 
				<< " added listen for " << trunk->getName() 
				<< ' ' << (detail.getService() ? detail.getService() : "any") 
				<< logend();

			trunk->enqueue(detail, client, server);
		}
		else
		{
			if (aPacket->getStringAt(0))
			{
				log(log_error, "sequencer")
					<< "client " << client 
					<< " attempted to add listen for invalid trunk " 
					<< aPacket->getStringAt(0) << logend();

				reply->setUnsignedAt(0, _invalid);
				server->send(*reply);

				break;
			}
			else
			{
				log(log_debug, "sequencer") << "client " << client 
					<< " added listen for any trunk " << logend();

				gClientQueue.enqueue(detail, client, server);
			}
		}

		reply->setUnsignedAt(0, _ok);
		server->send(*reply);

		break;
	}
	case if_connect:
	{
        reply = ico->staticPacket(1);
        reply->setContent(if_connect_done);
        reply->setSync(aPacket->getSyncMajor(), aPacket->getSyncMinor());
		
		if (aPacket->typeAt(0) != Packet::type_string
		 || aPacket->typeAt(1) != Packet::type_string
		 || aPacket->typeAt(2) != Packet::type_string)
		{
			reply->setUnsignedAt(0, _invalid);
			server->send(*reply);

			break;
		}

		SAP client;

		client.setAddress(aPacket->getStringAt(0));
		client.setService(aPacket->getStringAt(1));

		// remove any listeners for this client
		// first in the global queue

		gClientQueue.remove(server, client);

		// then in all the trunks

		gConfiguration.lock();
		for (ConfiguredTrunksIterator t(gConfiguration); !t.isDone(); t.next())
		{
			t.current()->removeClient(server, client);
		}
		gConfiguration.unlock();

		// now start the connect

		TrunkConfiguration* trunk;
		unsigned result = _failed;

		trunk = gConfiguration[aPacket->getStringAt(2)];

		log(log_debug, "sequencer") << "client " << client 
			<< " wants outgoing line on " 
			<< (aPacket->getStringAt(2) ? aPacket->getStringAt(2) : "any trunk") 
			<< logend();

		ConnectCompletion* complete = 
			new ConnectCompletion(*ico, aPacket->getSyncMajor(), aPacket->getSyncMinor(), client);

		if (trunk)
		{
			result = trunk->connect(complete);
		}
		else
		{
			gConfiguration.lock();
			for (ConfiguredTrunksIterator t(gConfiguration); !t.isDone(); t.next())
			{
				result = t.current()->connect(complete);
				if (result == _ok)	break;
			}
			gConfiguration.unlock();
		}

		if (result != _ok)
		{
			delete complete;

			reply->setUnsignedAt(0, result);
			server->send(*reply);
		}

		break;
	}
	case if_stop_listening:
	{
        reply = ((InterfaceConnection*)server)->staticPacket(1);
        reply->setContent(if_stop_listening_done);
        reply->setSync(aPacket->getSyncMajor(), aPacket->getSyncMinor());

		if (aPacket->typeAt(0) != Packet::type_string
		 || aPacket->typeAt(1) != Packet::type_string)
		{
			reply->setUnsignedAt(0, _invalid);
			server->send(*reply);

			break;
		}

		SAP client;

		client.setAddress(aPacket->getStringAt(0));
		client.setService(aPacket->getStringAt(1));

		// remove any listeners for this client
		// first in the global queue

		gClientQueue.remove(server, client);

		// then in all the trunks

		gConfiguration.lock();
		for (ConfiguredTrunksIterator t(gConfiguration); !t.isDone(); t.next())
		{
			t.current()->removeClient(server, client);
		}
		gConfiguration.unlock();

		reply->setUnsignedAt(0, _ok);
		server->send(*reply);

        break;
	}
	case if_shutdown:
	{
		log(log_debug, "sequencer") << "shutting down" << logend();

		gConfiguration.lock();
		for (ConfiguredTrunksIterator t(gConfiguration); !t.isDone(); t.next())
		{
			gConfiguration.basicRemoveAt(t.current());
		}
		gConfiguration.unlock();

		::exit(aPacket->getUnsignedAt(0));
	}
    default:
        break;
    }
}

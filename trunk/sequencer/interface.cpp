/*
	Copyright 1995-2001 Lars Immisch

	created: Mon Nov 18 17:09:12 GMT+0100 1996

	Author: Lars Immisch <lars@ibp.de>
*/

#pragma warning (disable: 4786)

#include "omnithread.h"
#include "conference.h"
#include "rphone.h"
#include "sequence.h"
#include "configuration.h"
#include "interface.h"

extern int debug;

extern ClientQueue gClientQueue;

extern ConfiguredTrunks gConfiguration;

InterfaceConnection::InterfaceConnection(TextTransportClient& aClient, SAP& local) 
: AsyncText(aClient)
{
    listen(local);
}

Interface::Interface(SAP& aLocal) : unused(1), local(aLocal)
{
	connections.addFirst(new InterfaceConnection(*this, local));
}

void Interface::run()
{
	((InterfaceConnection*)connections.getHead())->run();
}

void Interface::cleanup(TextTransport *server)
{
	// remove all listeners for the disconnected app

	// first in the global queue

	log(log_debug, "sequencer") << "client aborted" << logend();

	gClientQueue.remove((InterfaceConnection*)server);

	// then in all the trunks

	gConfiguration.lock();
	for (ConfiguredTrunksIterator t(gConfiguration); !t.isDone(); t.next())
	{
		t.current()->removeClient((InterfaceConnection*)server);
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

void Interface::connectRequest(TextTransport *server, SAP& remote)
{
	omni_mutex_lock lock(mutex);

	unused--;

	log(log_debug, "sequencer") << "client attached from " << remote << logend();

	if (unused == 0)
	{
		log(log_debug, "sequencer") << "spawning new interface listener" << logend();

		connections.addFirst(new InterfaceConnection(*this, local));
		unused++;
	}

	server->accept();

}

void Interface::connectRequestTimeout(TextTransport *server)
{
	SAP remote;

    server->listen(local, indefinite);
}

void Interface::connectConfirm(TextTransport *server)
{
	// we don't connect currently
}

void Interface::connectReject(TextTransport *server)
{
	// we don't connect currently
}

void Interface::connectTimeout(TextTransport *server)
{
	// we don't connect currently
}

void Interface::disconnectRequest(TextTransport *server)
{
	SAP remote;

	omni_mutex_lock lock(mutex);
	
	unused++;

	server->disconnectAccept();

    server->listen(local, indefinite);

	cleanup(server);
}

void Interface::disconnectConfirm(TextTransport *server)
{}

void Interface::disconnectTimeout(TextTransport *server)
{}

void Interface::disconnectReject(TextTransport *server)
{
    // We don't actively disconnect
}

void Interface::abort(TextTransport *server)
{
	SAP remote;

	omni_mutex_lock lock(mutex);
	
	unused++;

	cleanup(server);

    server->listen(local, indefinite);
}

void Interface::data(TextTransport *server)
{
    // this is the main packet inspection method...
	InterfaceConnection* ico = (InterfaceConnection*)server;

	omni_mutex_lock lock(ico->getMutex());

	std::string id;
	std::string scope;

	(*server) >> id;
	(*server) >> scope;

	if (!server->good() || server->eof())
	{
		server->clear();

		(*server) << _syntax_error << ' ' << id.c_str() 
			<< " syntax error\r\n";

		return;
	}

	if (scope == "global")
	{
		std::string command;

		(*server) >> command;

		if (!server->good())
		{
			(*server) << _syntax_error << id.c_str() 
				<< " syntax error\r\n";

			return;
		}

		if (command == "describe")
		{
			for (ConfiguredTrunksIterator c(gConfiguration); !c.isDone(); c.next())
			{
				(*server) << id.c_str() << ' ' << _ok << ' '
					<< c.current()->getName() << ' ' 
					<< c.current()->getNumber() << ' '
					<< (c.current()->isDigital() ? "digital" : "analog")
					<< "\n";
			}

			(*server) << "\r\n";
		}
		else if (command == "open-conference")
		{
			(*server) << id.c_str() << ' ' << _not_implemented 
				<< " not implemented" << "\r\n";
		}
		else if (command == "close-conference")
		{
			(*server) << id.c_str() << ' ' << _not_implemented 
				<< " not implemented" << "\r\n";
		}
		else if (command == "listen")
		{
			std::string trunkname;
			std::string spec;

			(*server) >> trunkname;
			(*server) >> spec;

			if (!server->good())
			{
				server->clear();

				(*server) << id.c_str() << _syntax_error 
					<< " syntax error\r\n";
				return;
			}

			TrunkConfiguration* trunk = 0;
			SAP client, detail;
			
			if (spec != "any")
				detail.setService(spec.c_str());

			if (trunkname != "any")
			{
				trunk = gConfiguration[trunkname.c_str()];

				if (!trunk)
				{
					log(log_error, "sequencer")
						<< "attempted to add listen for invalid trunk " 
						<< trunkname.c_str() << logend();

					(*server) << id.c_str() << ' ' << _invalid 
						<< " invalid trunk " << trunkname.c_str() << "\r\n";

					return;
				}
	
				log(log_debug, "sequencer") << "id " << id.c_str() 
					<< " added listen for " << trunk->getName() 
					<< ' ' << (detail.getService() ? detail.getService() : "any") 
					<< logend();

				trunk->enqueue(id, detail, ico);
			}
			else
			{
				log(log_debug, "sequencer") << "id " << id.c_str()
					<< " added listen for any trunk " << logend();

				gClientQueue.enqueue(id, detail, ico);
			}
		}
		else if (command == "connect")
		{
/*
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
*/
		}
		else if (command == "stop-listening")
		{
/*
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
*/
		}
		else
		{
			// syntax error

			(*server) << id.c_str() << ' ' << _failed 
				<< " syntax error - unknown command " << command.c_str()
				<< "\r\n";
		}
	}
	else
	{
		Sequencer *s = ico->find(scope);

		if (!s)
		{
			(*server) << id.c_str() << ' ' << _failed 
				<< " syntax error - unknown scope " << scope.c_str()
				<< "\r\n";
		}
		else
		{
			s->data(ico, id);
		}
	}
}

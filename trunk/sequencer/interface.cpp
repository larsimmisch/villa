/*
	Copyright 1995-2001 Lars Immisch

	created: Mon Nov 18 17:09:12 GMT+0100 1996

	Author: Lars Immisch <lars@ibp.de>
*/

#pragma warning (disable: 4786)

#include "omnithread.h"
#include "conference.h"
#include "socket.h"
#include "rphone.h"
#include "sequence.h"
#include "configuration.h"
#include "interface.h"

extern int debug;
extern ClientQueue gClientQueue;
extern ConfiguredTrunks gConfiguration;
extern Conferences gConferences;


Interface::Interface(SAP& local)
{
	m_listener.setReuseAddress(1);
	m_listener.bind(local);
	m_listener.listen();
}

void Interface::run()
{
	fd_set read;
	fd_set write;
	fd_set except;

	for(;;)
	{
		try
		{
			FD_ZERO(&read);
			FD_ZERO(&write);
			FD_ZERO(&except);

			FD_SET(m_listener.fd(), &read);

			for (std::list<InterfaceConnection*>::iterator i = m_connections.begin(); 
				 i != m_connections.end(); ++i)
			{
				FD_SET((*i)->fd(), &read);
			}

			// maxfd is ignored on Windows
			int rc = select(0, &read, &write, &except, 0);
			if (rc == SOCKET_ERROR)
			{
				throw SocketError(__FILE__, __LINE__, "Interface::run()", GetLastError());
			}

			if (FD_ISSET(m_listener.fd(), &read))
			{
				SAP remote;

				// accept the connection
				InterfaceConnection *ic = new InterfaceConnection(PF_INET,
					m_listener.accept(remote));

				ic->m_remote = remote;

				m_connections.push_back(ic);

				log(log_debug, "sequencer") << "client " << remote << " attached" << logend();

				ic->begin() << "sequence protocol 0.2" << end();
			}

			std::vector<InterfaceConnection*> remove;

			for (i = m_connections.begin(); i != m_connections.end(); ++i)
			{
				bool exit(false);
				InterfaceConnection *ic = *i;

				if (FD_ISSET(ic->fd(), &read))
				{
					rc = ic->receive();
					ic->clear();

					if (rc)
					{
						exit = !data(ic);			
					}
					if (rc == 0 || exit)
					{
						if (rc == 0)
							log(log_debug, "sequencer") << "client " 
								<< ic->m_remote << " aborted" << logend();
						else
							log(log_debug, "sequencer") << "client "
								<< ic->m_remote << " disconnected " << logend();

						// remove all listeners for the disconnected app
						// first in the global queue
						gClientQueue.remove(ic);

						// then in all the trunks

						gConfiguration.lock();
						for (ConfiguredTrunksIterator t(gConfiguration); !t.isDone(); t.next())
						{
							t.current()->removeClient(ic);
						}
						gConfiguration.unlock();

						// force close all conferences opened by this app
						/*
						gConferences.lock();
						for (ConferencesIterator c(gConferences); !c.isDone(); c.next())
						{
							if (c.current()->getUserData() == ic)
							{
								gConferences.close(c.current()->getHandle(), 1);
							}
						}
						gConferences.unlock();
						*/

						remove.push_back(ic);
					}
				}
			}

			// delete and close all dead connections
			for (std::vector<InterfaceConnection*>::iterator j = remove.begin();
				 j != remove.end(); ++j)
			{
				m_connections.remove(*j);
				delete *j;
			}
		}
		catch (const Exception& e)
		{
			log(log_error, "sequencer") << e << logend();
		}
	}
}

bool Interface::data(InterfaceConnection *ic)
{
    // this is the main packet inspection method...

	std::string id;
	std::string scope;

	(*ic) >> id;
	(*ic) >> scope;

	if (ic->eof())
	{
		ic->clear();

		// exit from telnet - does not restrict tid usage
		if (id == "exit")
		{
			return false;
		}

		ic->begin() << _syntax_error 
			<< " syntax error - expecting id and scope" << end();

		return true;
	}

	if (scope == "global")
	{
		std::string command;

		(*ic) >> command;

		if (!command.size())
		{
			ic->clear();

			ic->begin() << _syntax_error << 
				" syntax error - expecting command" << end();

			return true;
		}

		if (command == "describe")
		{
			ic->begin();

			for (ConfiguredTrunksIterator c(gConfiguration); !c.isDone(); c.next())
			{
				(*ic) << id.c_str() << ' ' << _ok << ' '
					<< c.current()->getName() << ' ' 
					<< c.current()->getNumber() << ' '
					<< (c.current()->isDigital() ? "digital" : "analog")
					<< "\n";
			}

			(*ic) << end();
		}
		else if (command == "open-conference")
		{
			Conference *conf = gConferences.create(ic);

			if (conf)
			{
				char name[32];

				sprintf(name, "Conf[%d]", conf->getHandle());

				ic->begin() << id.c_str() << ' ' << _ok 
					<< " global open-conference-done " << name << end();
			}
			else
			{
				ic->begin() << id.c_str() << _failed 
					<< " global open-conference-done" << end();
			}
		}
		else if (command == "close-conference")
		{
			std::string conf;

			(*ic) >> conf;

			if (conf.size() <= 4 || conf.substr(0, 5) != "Conf[")
			{
				ic->clear();

				ic->begin() << id.c_str() << ' ' << _syntax_error << 
					" global close-conference-done" << end();

				return true;
			}

			unsigned handle(0);

			sscanf(conf.c_str(), "Conf[%d]", &handle);

			if (!handle)
			{
				ic->begin() << _syntax_error << 
					" global close-conference-done" << end();

				return true;
			}

			if (gConferences.close(handle))
			{
				ic->begin() << id.c_str() << ' ' << _ok 
					<< " global close-conference-done" << end();
			}
			else
			{
				ic->begin() << id.c_str() << ' ' << _failed 
					<< " global close-conference-done" << end();
			}
		}
		else if (command == "listen")
		{
			std::string trunkname;
			std::string spec;

			(*ic) >> trunkname;
			(*ic) >> spec;

			if (!trunkname.size() | !spec.size())
			{
				ic->clear();

				ic->begin() << _syntax_error 
					<< " syntax error - expecting trunk name and DID" 
					<< end();

				return true;
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

					ic->begin() << id.c_str() << ' ' << _invalid 
						<< " invalid trunk " << trunkname.c_str() << end();

					return true;
				}
	

				trunk->enqueue(id, detail, ic);

				log(log_debug, "sequencer") << "id " << id.c_str() 
					<< " added listen for " << trunk->getName() 
					<< ' ' << (detail.getService() ? detail.getService() : "any") 
					<< logend();
			}
			else
			{
				gClientQueue.enqueue(id, detail, ic);

				log(log_debug, "sequencer") << "id " << id.c_str()
					<< " added listen for any trunk " << logend();

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

			gClientQueue.remove(ic, client);

			// then in all the trunks

			gConfiguration.lock();
			for (ConfiguredTrunksIterator t(gConfiguration); !t.isDone(); t.next())
			{
				t.current()->removeClient(ic, client);
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
				new ConnectCompletion(*ic, aPacket->getSyncMajor(), aPacket->getSyncMinor(), client);

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
				ic->send(*reply);
			}
*/
		}
		else if (command == "stop-listening")
		{
/*
			reply = ((InterfaceConnection*)ic)->staticPacket(1);
			reply->setContent(if_stop_listening_done);
			reply->setSync(aPacket->getSyncMajor(), aPacket->getSyncMinor());

			if (aPacket->typeAt(0) != Packet::type_string
			 || aPacket->typeAt(1) != Packet::type_string)
			{
				reply->setUnsignedAt(0, _invalid);
				ic->send(*reply);

				break;
			}

			SAP client;

			client.setAddress(aPacket->getStringAt(0));
			client.setService(aPacket->getStringAt(1));

			// remove any listeners for this client
			// first in the global queue

			gClientQueue.remove(ic, client);

			// then in all the trunks

			gConfiguration.lock();
			for (ConfiguredTrunksIterator t(gConfiguration); !t.isDone(); t.next())
			{
				t.current()->removeClient(ic, client);
			}
			gConfiguration.unlock();

			reply->setUnsignedAt(0, _ok);
			ic->send(*reply);
*/
		}
		else
		{
			// syntax error

			ic->begin() << id.c_str() << ' ' << _failed 
				<< " syntax error - unknown command " << command.c_str()
				<< end();
		}
	}
	else
	{
		Sequencer *s = ic->find(scope);

		if (!s)
		{
			ic->begin() << id.c_str() << ' ' << _failed 
				<< " syntax error - unknown scope " << scope.c_str()
				<< end();
		}
		else
		{
			s->data(ic, id);
		}
	}

	return true;
}

/*
	Copyright 1995-2001 Lars Immisch

	created: Mon Nov 18 17:09:12 GMT+0100 1996

	Author: Lars Immisch <lars@ibp.de>
*/

#pragma warning (disable: 4786)

#include "omnithread.h"
#include "conference.h"
#include "socket.h"
#include "sequence.h"
#include "configuration.h"
#include "interface.h"

extern int debug;
extern ClientQueue gClientQueue;
extern ConfiguredTrunks gConfiguration;
extern Conferences gConferences;

void InterfaceConnection::lost_connection()
{
	log(log_info, "sequencer") << "client " 
		<< m_remote << " aborted" << logend();

    // Villa shortcut
    ::exit(0);

	// remove all listeners for the disconnected app
	// first in the global queue
	gClientQueue.remove(this);

	// then in all the trunks

	gConfiguration.lock();
	for (ConfiguredTrunksIterator t(gConfiguration); !t.isDone(); t.next())
	{
		t.current()->removeClient(this);
	}
	gConfiguration.unlock();

	// tell all sequencers we have lost the connection
	const InterfaceConnection::t_calls &calls = get_calls();

	for (InterfaceConnection::t_calls::const_iterator i = calls.begin();
		i != calls.end(); ++i)
	{
		i->second->lost_connection();
	}

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

}

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
				FD_SET((*i)->fd(), &except);
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

				log(log_info, "sequencer") << "client " << remote << " attached" << logend();

				ic->begin() << "sequence protocol 0.2" << end();
			}

			std::vector<InterfaceConnection*> remove;

			for (i = m_connections.begin(); i != m_connections.end(); ++i)
			{
				bool exit(false);
				InterfaceConnection *ic = *i;

				if (FD_ISSET(ic->fd(), &read))
				{
					do
					{
						rc = ic->receive();
						ic->clear();
						if (!data(ic))
						{
							exit = true;
							break;
						}
					} while (rc);

					if (rc == 0 || exit)
					{
						ic->lost_connection();
						remove.push_back(ic);
					}
				}
				if (FD_ISSET(ic->fd(), &except))
				{
					ic->lost_connection();
					remove.push_back(ic);
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
	std::string command;

	(*ic) >> id;
	if (ic->eof())
	{
		// incomplete command
		return true;
	}

	(*ic) >> command;

	if (ic->eof())
	{
		ic->clear();

		// exit from telnet - does not restrict tid usage (irregular syntax)
		if (id == "exit")
		{
			return false;
		}

		ic->begin() << V3_FATAL_SYNTAX << ' ' << id.c_str()
			<< " syntax error - expecting id and command" << end();

		return false;
	}

	if (command == "DESC")
	{
		ic->begin();

		for (ConfiguredTrunksIterator c(gConfiguration); !c.isDone(); c.next())
		{
			(*ic) << V3_OK << ' ' << id.c_str() << " DESC "
				<< c.current()->getName() << ' ' 
				<< c.current()->getNumber() << ' '
				<< (c.current()->isDigital() ? "digital" : "analog")
				<< "\n";
		}

		(*ic) << end();
	}
	else if (command == "CNFO")
	{
		Conference *conf = gConferences.create(ic);

		if (conf)
		{
			ic->begin() << V3_OK << ' ' << id.c_str() << " CNFO conf[" << conf->getHandle()
				<< ']' << end();
		}
		else
		{
			ic->begin() << V3_ERROR_NO_RESOURCE << ' ' << id.c_str() << " CNFO " << end();
		}
	}
	else if (command == "CNFC")
	{
		std::string conf;

		(*ic) >> conf;

		if (conf.size() <= 4 || conf.substr(0, 5) != "conf[")
		{
			ic->clear();

			ic->begin() << V3_ERROR_NOT_FOUND << ' ' << id.c_str() << " CNFC " << end();

			return true;
		}

		unsigned handle(0);

		sscanf(conf.c_str(), "conf[%d]", &handle);

		if (!handle)
		{
			ic->begin() << V3_ERROR_NOT_FOUND << ' ' << id.c_str() << " CFNC " << end();

			return true;
		}

		if (gConferences.close(handle))
		{
			ic->begin() << V3_OK << ' ' << id.c_str() << " CNFC " << end();
		}
		else
		{
			ic->begin() << V3_ERROR_NOT_FOUND << ' ' << id.c_str() << " CNFC " << end();
		}
	}
	else if (command == "BGRO")
	{
		Sequencer *s = new Sequencer(ic);
		if (!s || !s->getMedia())
		{
			ic->begin() << V3_ERROR_NO_RESOURCE << ' ' << id.c_str() << end();
		}
		else
		{
			std::string name = s->getName();

			// loop Prosody channel output to input for conference backgrounds
			s->getMedia()->loopback();

			ic->add(name, s);

			ic->begin() << V3_OK << ' ' << id.c_str() << " BGRO " << name << end();
		}
	}
	else if (command == "BGRC")
	{
		std::string device;

		(*ic) >> device;

		if (!device.size())
		{
			ic->clear();

			ic->begin() << V3_FATAL_SYNTAX << ' ' << id.c_str() << " BGRC " 
				<< " syntax error - expecting device" 
				<< end();

			return false;
		}

		Sequencer *s = ic->find(device);
		if (!s)
		{
			ic->begin() << V3_ERROR_NOT_FOUND << ' ' << id.c_str() << " BGRC " << device 
				<< end();
		}
		else
		{
			ic->remove(device);

			s->BGRC(id);
		}
	}
	else if (command == "LSTN")
	{
		std::string trunkname;
		std::string spec;

		(*ic) >> trunkname;
		(*ic) >> spec;

		if (!trunkname.size() || !spec.size())
		{
			ic->clear();

			ic->begin() << V3_FATAL_SYNTAX << ' ' << id.c_str() << " LSTN " 
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

				ic->begin() << V3_ERROR_NOT_FOUND << ' ' << id.c_str() 
					<< " LSTN unknown trunk: " << trunkname.c_str() << end();

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
	else if (command == "CONN")
	{
		std::string trunkname;
		std::string called;
		std::string calling;
		int timeout = -1;
		SAP remote;
		SAP local;
		
		(*ic) >> trunkname;
		(*ic) >> called;
		(*ic) >> timeout;
		(*ic) >> calling;

		if (!trunkname.size() || !called.size())
		{
			ic->clear();
			
			log(log_error, "sequencer") << "id " << id.c_str()
				<< " syntax error - expected trunk name, timeslot and called address" 
				<< logend();
			

			ic->begin() << V3_FATAL_SYNTAX << ' ' << id.c_str() << " CONN " 
				<< " syntax error - expected trunk name, timeslot and called address" 
				<< end();

			return true;
		}

		remote.setAddress(called.c_str());
		if (calling.size() && calling != "default")
		{
			local.setAddress(calling.c_str());
		}

		// now start the connect

		TrunkConfiguration* trunk = 0;
		int result = V3_ERROR_NO_RESOURCE;

		if (trunkname != "any")
		{
			trunk = gConfiguration[trunkname.c_str()];
			if (!trunk)
			{
				log(log_warning, "sequencer") << "id " << id.c_str()
					<< " trunk " << trunkname << " not found" 
					<< logend();
			
				ic->begin() << V3_ERROR_NOT_FOUND << ' ' << id.c_str() << " CONN " 
					<< " trunk not found" << end();

				return true;
			}
		}


		log(log_debug, "sequencer") << "id " << id.c_str()
			<< " started connect on trunk " << trunkname << logend();

		ConnectCompletion* complete = 
			new ConnectCompletion(ic, id, local, remote, timeout);

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
				if (result == V3_OK)
				{
					break;
				}
			}
			gConfiguration.unlock();

		}

		if (result != V3_OK)
		{
			delete complete;

			ic->begin() << result << ' ' << id.c_str() << " CONN " 
				<< end();
		}

	}
	else if (command == "STOP")
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
		std::string device;

        device.reserve(32);

		(*ic) >> device;
		if (ic->eof())
		{
			ic->clear();

			ic->begin() << V3_FATAL_SYNTAX << ' ' << id.c_str() << " syntax error - missing device name for: " 
				<< command << end();

			return true;
		}

		Sequencer *s = ic->find(device);

		if (!s)
		{
			ic->begin() << V3_FATAL_SYNTAX << ' ' << id.c_str()
				<< " error - unknown device: " << device.c_str()
				<< end();

			return true;
		}
		else
		{
			s->data(ic, command, id);
		}
	}

	return true;
}

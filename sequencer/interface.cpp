/*
	Copyright 1995-2001 Lars Immisch

	created: Mon Nov 18 17:09:12 GMT+0100 1996

	Author: Lars Immisch <lars@ibp.de>
*/

#pragma warning (disable: 4786)

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
    // ::_exit(0);

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

int InterfaceConnection::send(std::stringstream &data)
{
   	std::ostream &o = log(log_info, "text") << m_remote << " sent: ";

    std::string &str = data.str();

    o.write(str.c_str(), str.size() - 2);
    o << logend();

    return Socket::send((char*)str.c_str(), str.size());
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

			std::list<InterfaceConnection*>::iterator i;
			for (i = m_connections.begin(); 
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
                std::stringstream str;

				// accept the connection
				InterfaceConnection *ic = new InterfaceConnection(PF_INET,
					m_listener.accept(remote));

				ic->setNonblocking(1);
				ic->setKeepAlive(1);

				ic->m_remote = remote;

				m_connections.push_back(ic);

				log(log_info, "sequencer") << "client " << remote << " attached" << logend();


				str << "sequence protocol 0.2\r\n";

                ic->send(str);
			}

			std::vector<InterfaceConnection*> remove;

			for (i = m_connections.begin(); i != m_connections.end(); ++i)
			{
				bool exit(false);
				InterfaceConnection *ic = *i;

				if (FD_ISSET(ic->fd(), &except))
				{
					ic->lost_connection();
					remove.push_back(ic);
				}
				else if (FD_ISSET(ic->fd(), &read))
				{
					do
					{
						rc = ic->receive();
						if (rc > 0)
						{
							ic->clear();
							if (!data(ic))
							{
								exit = true;
								break;
							}
						}
					} while (rc > 0);

					if (rc < 0 || exit)
					{
						ic->lost_connection();
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
			log(log_error, "sequencer") << "Interface::run " << e << logend();
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
        std::stringstream str;

		ic->clear();

		// exit from telnet - does not restrict tid usage (irregular syntax)
		if (id == "exit")
		{
			return false;
		}
		else if (id == "shutdown")
		{
			exit(0);
		}

		str << V3_FATAL_SYNTAX << ' ' << id	<< " syntax error - expecting id and command\r\n";

        ic->send(str);

		return false;
	}

	if (command == "DESC")
	{
        std::stringstream str;

		str << V3_OK << ' ' << id << " DESC global";

		for (ConfiguredTrunksIterator c(gConfiguration); !c.isDone(); c.next())
		{
			str << ' ' << c.current()->getName() << ' ' 
				<< c.current()->getNumber() << ' '
				<< c.current()->numLines();
		}

		str << "\r\n";

        ic->send(str);
	}
	else if (command == "CNFO")
	{
		Conference *conf = gConferences.create(ic);

		if (conf)
		{
            std::stringstream str;

			str << V3_OK << ' ' << id << " CNFO conf[" << conf->getHandle()
				<< ']' << "\r\n";

            ic->send(str);
		}
		else
		{
            std::stringstream str;

			str << V3_ERROR_NO_RESOURCE << ' ' << id << " CNFO\r\n";
		}
	}
	else if (command == "CNFC")
	{
		std::string conf;

		(*ic) >> conf;

		if (conf.size() <= 4 || conf.substr(0, 5) != "conf[")
		{
            std::stringstream str;

			ic->clear();

			str << V3_ERROR_NOT_FOUND << ' ' << id << " CNFC\r\n";

            ic->send(str);
                
			return true;
		}

		unsigned handle(0);

		sscanf(conf.c_str(), "conf[%d]", &handle);

		if (!handle)
		{
            std::stringstream str;

			str << V3_ERROR_NOT_FOUND << ' ' << id << " CFNC\r\n";

            ic->send(str);

			return true;
		}

		if (gConferences.close(handle))
		{
            std::stringstream str;

			str << V3_OK << ' ' << id << " CNFC\r\n";

            ic->send(str);
		}
		else
		{
            std::stringstream str;

			str << V3_ERROR_NOT_FOUND << ' ' << id << " CNFC\r\n";

            ic->send(str);
		}
	}
	else if (command == "BGRO")
	{
		Sequencer *s = new Sequencer(ic);
		if (!s || !s->getMedia())
		{
            std::stringstream str;

			str << V3_ERROR_NO_RESOURCE << ' ' << id << "\r\n";

            ic->send(str);
		}
		else
		{
            std::stringstream str;
			std::string name = s->getName();

			// loop Prosody channel output to input for conference backgrounds
			s->getMedia()->loopback();

			ic->add(name, s);

			str << V3_OK << ' ' << id << " BGRO " << name << "\r\n";

            ic->send(str);
		}
	}
	else if (command == "BGRC")
	{
		std::string device;

		(*ic) >> device;

		if (!device.size())
		{
            std::stringstream str;

			ic->clear();

			str << V3_FATAL_SYNTAX << ' ' << id << " BGRC " 
				<< " syntax error - expecting device" 
				<< "\r\n";

            ic->send(str);

			return false;
		}

		Sequencer *s = ic->find(device);
		if (!s)
		{
            std::stringstream str;

			str << V3_ERROR_NOT_FOUND << ' ' << id << " BGRC " << device << "\r\n";

            ic->send(str);
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
            std::stringstream str;

			ic->clear();

			str << V3_FATAL_SYNTAX << ' ' << id << " LSTN " 
				<< " syntax error - expecting trunk name and DID" 
				<< "\r\n";

            ic->send(str);

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
                std::stringstream str;

				log(log_error, "sequencer")
					<< "attempted to add listen for invalid trunk " 
					<< trunkname.c_str() << logend();

				str << V3_ERROR_NOT_FOUND << ' ' << id 
					<< " LSTN unknown trunk: " << trunkname.c_str() << "\r\n";

                ic->send(str);

				return true;
			}


			trunk->enqueue(id, detail, ic);

			log(log_debug, "sequencer") << "id " << id 
				<< " added listen for " << trunk->getName() 
				<< ' ' << (detail.getService() ? detail.getService() : "any") 
				<< logend();
		}
		else
		{
			gClientQueue.enqueue(id, detail, ic);

			log(log_debug, "sequencer") << "id " << id
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
            std::stringstream str;

			ic->clear();
			
			log(log_error, "sequencer") << "id " << id
				<< " syntax error - expected trunk name, timeslot and called address" 
				<< logend();
			

			str << V3_FATAL_SYNTAX << ' ' << id << " CONN " 
				<< " syntax error - expected trunk name, timeslot and called address" 
				<< "\r\n";

            ic->send(str);

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
                std::stringstream str;

				log(log_warning, "sequencer") << "id " << id
					<< " trunk " << trunkname << " not found" 
					<< logend();
			
				str << V3_ERROR_NOT_FOUND << ' ' << id << " CONN trunk not found\r\n";

                ic->send(str);

				return true;
			}
		}


		log(log_debug, "sequencer") << "id " << id
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
            std::stringstream str;

			delete complete;

			str << result << ' ' << id << " CONN\r\n";

            ic->send(str);
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
            std::stringstream str;

			ic->clear();

			str << V3_FATAL_SYNTAX << ' ' << id << " syntax error - missing device name for: " 
				<< command << "\r\n";

            ic->send(str);

			return true;
		}

		Sequencer *s = ic->find(device);

		if (!s)
		{
            std::stringstream str;

			str << V3_ERROR_NOT_FOUND << ' ' << id
				<< " error - unknown device: " << device.c_str()
				<< "\r\n";

            ic->send(str);

			return true;
		}
		else
		{
			s->data(ic, command, id);
		}
	}

	return true;
}

#pragma warning (disable: 4786)

#include "omnithread.h"
#include "configuration.h"
#include "sequence.h"

extern int debug;
// extern Log* logger;

char* copyString(const char* aString);

void InvalidKey::printOn(std::ostream& aStream)
{
	Exception::printOn(aStream);

	aStream << " offending key: " << key;
}

TrunkConfiguration::~TrunkConfiguration()
{
	delete name;
	delete number;
}

int TrunkConfiguration::readFromKey(RegistryKey& key)
{
	name = copyString(key.getStringAt("Name"));

	if (!name)
	{
		return 0;
	}

	number = copyString(key.getStringAt("Number"));

	return 1;
}

AculabPRITrunkConfiguration::~AculabPRITrunkConfiguration()
{
	for (unsigned index = 0; index < lines; index++)
		delete sequencers[index];
}

void AculabPRITrunkConfiguration::init(int d, int sw, int l)
{
	char buffer[32];

	sprintf(buffer, "Aculab%d", d);

	name = copyString(buffer);
	number = copyString("");

	device = d;
	swdevice = sw;
	lines = l;
}

int AculabPRITrunkConfiguration::readFromKey(RegistryKey& key)
{
	if (!TrunkConfiguration::readFromKey(key))	return 0;

	if (!key.hasValue("Device")) return 0;
	if (!key.hasValue("Switch")) return 0;

	device = key.getUnsignedAt("Device");
	swdevice = key.getUnsignedAt("Switch");

	if (key.hasValue("Lines"))	lines = key.getUnsignedAt("Lines");

	free = lines;

	return 1;
}

void AculabPRITrunkConfiguration::start()
{
	omni_mutex_lock lock(mutex);

	for (unsigned index = 0; index < lines; index++)
	{
		sequencers[index] = new Sequencer(this);
	}

	log(log_debug, "sequencer") << "Aculab PRI started " << lines << " lines on device " << device << logend();
}

void AculabPRITrunkConfiguration::removeClient(InterfaceConnection *iface)
{
	omni_mutex_lock lock(mutex);

	for (DDIIterator i(ddis); !i.isDone(); i.next())
	{
		i.current()->queue.remove(iface);
	}
}

void AculabPRITrunkConfiguration::removeClient(InterfaceConnection *iface,
											   const std::string &id)
{
	omni_mutex_lock lock(mutex);

	for (DDIIterator i(ddis); !i.isDone(); i.next())
	{
		i.current()->queue.remove(iface, id);
	}
}

ClientQueue::Item* AculabPRITrunkConfiguration::dequeue(const SAP& details)
{
	omni_mutex_lock lock(mutex);

	DDIs::Node* node = ddis.find(details.getService());

	if (!node) 
		return 0;

	return node->queue.dequeue();
}

void AculabPRITrunkConfiguration::enqueue(const std::string &id, 
										  const SAP& details, 
										  InterfaceConnection *iface)
{
	omni_mutex_lock lock(mutex);

	DDIs::Node* node = ddis.create(details.getService());

	node->queue.enqueue(id, details, iface);
}

unsigned AculabPRITrunkConfiguration::connect(ConnectCompletion* complete)
{
	omni_mutex_lock lock(mutex);

	unsigned result = V3_ERROR_NO_RESOURCE;

	for (unsigned index = 0; index < lines; index++)
	{
		result = sequencers[index]->connect(complete);
		if (result == V3_OK)	
			return V3_OK;
	}

	return result;
}

ClientQueue::Item* ClientQueue::dequeue()
{
	omni_mutex_lock lock(m_mutex);

	return (Item*)removeFirst();
}

void ClientQueue::remove(InterfaceConnection *iface)
{
	omni_mutex_lock lock(m_mutex);

	for (ListIter gc(*this); !gc.isDone(); gc.next())
	{
		Item* item = (Item*)gc.current();
		if (item->m_interface == iface)
		{
			log(log_debug, "sequencer") 
				<< "removed client at: " << item->m_details << logend();

			DList::remove(item);
			delete item;
		}
	}
}

void ClientQueue::remove(InterfaceConnection *iface, const std::string &id)
{
	omni_mutex_lock lock(m_mutex);

	for (ListIter gc(*this); !gc.isDone(); gc.next())
	{
		Item* item = (Item*)gc.current();
		if (item->m_interface == iface && item->m_id == id)
		{
			log(log_debug, "sequencer") 
				<< "removed client at: " << item->m_details << logend();

			DList::remove(item);
			delete item;
		}
	}
}

DDIs::Node::Node()
{
	memset(children, 0, sizeof(children));
}

DDIs::Node* DDIs::Node::subKey(char c)
{
	if (c > '9' || c < '0') throw InvalidKey(__FILE__, __LINE__,"DDIs::Node::subKey(char)", 0);

	return children[c - '0'];
}

DDIs::~DDIs()
{
	remove(&root);
}

void DDIs::remove(Node* aNode)
{
	unsigned index;

	for(index = 0; index < 10; index++)
	{
		if (aNode->children[index]) 
		{
			remove(aNode->children[index]);
			delete aNode->children[index];
		}
	}
}

DDIs::Node* DDIs::find(const char* key)
{
	unsigned index;
	Node* node = &root;

	if (key == 0)	return 0;

	try
	{
		for (index = 0;index < strlen(key); index++)
		{
			node = node->subKey(key[index]);

			if (!node) return 0;
		}
	}
	catch(InvalidKey&)
	{
		throw InvalidKey(__FILE__, __LINE__,"DDIs::find(const char*)", key);
	}
	return node;
}

int DDIs::isSubKey(const char* key)
{
	if (key == 0)	return 0;

	if (*key == '\0')
	{
		for (unsigned index = 0; index < 10; index++)
		{
			if (root.children[index])	return TRUE;
		}

		return FALSE;
	}
	else return (int)find(key);
}

DDIs::Node* DDIs::create(const char* key)
{
	unsigned index;
	Node* node = &root;
	Node* next;

	if (!key) return node;

	try
	{
		for (index = 0;index < strlen(key); index++)
		{
			next = node->subKey(key[index]);

			if (!next)
			{
				next = new Node;

				node->setSubKey(key[index], next);
			}
			node = next;
		}
	}
	catch(InvalidKey&)
	{
		throw InvalidKey(__FILE__, __LINE__,"DDIs::create(const char*)", key);
	}

	return node;
}

void DDIs::remove(const char* aKey)
{
	unsigned index, kids;
	Node* node;
	char key[32];
	int empty;
	char c;

	if (strlen(aKey) > 31) throw InvalidKey(__FILE__, __LINE__,"DDIs::remove(const char*)", aKey);

	strcpy(key, aKey);

	node = find(key);
	if (!node) return;

	remove(node);
	delete node;

	for (index = strlen(key) -1;index >= 0; index--)
	{
		c = key[index];
		key[index] = '\0';

		node = find(key);

		node->setSubKey(c, 0);

		empty = 1;
		for (kids = 0; kids < 10; kids++)
		{
			if (node->children[kids]) return;
		}

		if (empty && index != 0)
		{
			delete node;
		}
	}
}

DDIIterator::DDIIterator(DDIs& addis)
{
	ddis = &addis;
	memset(key, 0, sizeof(key));
	node = &ddis->root;
}

DDIs::Node* DDIIterator::next()
{
	unsigned index;
	unsigned last;

	if (!node)	return 0;

	for (index = 0; index < 10; index++)
	{
		if (node->children[index])
		{
			node = node->children[index];
			key[strlen(key)] = index + '0';
			return node;
		}
	}

	while(strlen(key))
	{
		last = key[strlen(key) -1] - '0';
		key[strlen(key) -1] = '\0';
		node  = ddis->find(key);

		for (index = last + 1; index < 10; index++)
		{
			if (node->children[index]) 
			{
				node = node->children[index];
				key[strlen(key)] = index + '0';
				return node;
			}
		}
	}

	node = 0;
	return node;
}

ConfiguredTrunks::ConfiguredTrunks() : Set(), lines(0)
{
}

ConfiguredTrunks::~ConfiguredTrunks()
{
	empty();
}

TrunkConfiguration* ConfiguredTrunks::add(TrunkConfiguration* configuration)
{
	lock();

	if (at(configuration->getName()))	return 0;

	lines += configuration->numLines();

	Set::basicAdd(configuration);
 
	unlock();

	return configuration;
}

void ConfiguredTrunks::remove(const char* aName)
{
	lock();

	TrunkConfiguration* configuration = (TrunkConfiguration*)basicRemoveAt(aName);

	if (configuration) lines -= configuration->numLines();;

	delete configuration;

	unlock();
}

TrunkConfiguration* ConfiguredTrunks::operator[](const char* aName)
{
	if (!aName)	return 0;

	TrunkConfiguration* configuration;

	lock();

	configuration = (TrunkConfiguration*)basicAt(aName);

	unlock();

	return configuration;
}

int ConfiguredTrunks::hasIndex(List::Link* anItem, int anIndex)
{ 
	return 0;
}

int ConfiguredTrunks::hasKey(List::Link* anItem, const char* aKey)
{
	return stricmp(((TrunkConfiguration*)anItem)->getName(), aKey) == 0;
}

unsigned ConfiguredTrunks::hashAssoc(List::Link* anItem)
{ 
	return hashString(((TrunkConfiguration*)anItem)->getName());
}

void ConfiguredTrunks::empty()
{ 
	lock(); 
	for( AssocIter i(*this); !i.isDone(); i.next() ) freeLink( i.current() );
	clear();
	unlock();
}

int ConfiguredTrunks::isEqual(List::Link* anItem, List::Link* anotherItem)
{
	return stricmp(((TrunkConfiguration*)anItem)->getName(), ((TrunkConfiguration*)anotherItem)->getName()) == 0;
}

void ConfiguredTrunks::freeLink(List::Link* item)
{ 
	delete (TrunkConfiguration*)item;
}

void MediaPool::add(int count)
{
	omni_mutex_lock lock(m_mutex);

	for (int i = 0; i < count; ++i)
	{
		m_media.push_back(new AculabMedia(0));
	}
}

AculabMedia *MediaPool::allocate(Sequencer *s)
{
	omni_mutex_lock lock(m_mutex);

	for (std::vector<AculabMedia*>::iterator i = m_media.begin(); 
		i != m_media.end(); ++i)
	{
		if (!(*i)->getClient())
		{
			(*i)->setClient(s);
			(*i)->setTransmitTimeslot(s->m_transmit);
			(*i)->setReceiveTimeslot(s->m_receive);

			return *i;
		}
	}

	return 0;
}

void MediaPool::release(AculabMedia *m)
{
	omni_mutex_lock lock(m_mutex);

	m->setClient(0);
	m->setTransmitTimeslot(Timeslot(-1, -1));
	m->setReceiveTimeslot(Timeslot(-1, -1));
}

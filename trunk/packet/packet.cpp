/* Packet.cpp */

#include <string.h>
#include "Packet.h"

Packet::Packet(int aNumArgs, int allocatedSize) 
 : magic(packet_magic), controlOffset(0), timestamp(allocatedSize)
{
	getData()->numArgs = aNumArgs;
	
	size = sizeof(Packet) - sizeof(Data) + aNumArgs * sizeof(Descriptor);
}

Packet::~Packet()
{}

int Packet::checkForAdditionalSize(unsigned aSize)
{
	// timestamp is abused as maxSize while packet is constructed.
#if defined(NeXT) || defined(_THINK_C_) || defined(__MWERKS__)// ###MR
	if (controlOffset != 0) { printf("don't touch packets filled with
control data!\n"); exit(23); }
	if (aSize + sizeof(Descriptor) + size > timestamp)     { printf("packet too small!\n"); exit(24); }
#else
	if (controlOffset != 0) throw "don't touch packets filled with control data!";
	if (aSize + sizeof(Descriptor) + size > (int)timestamp)
    {
        throw "packet too small!";
    }
#endif

	return 1;
}

void* Packet::getValueAt(int aPosition) const
{
	if (typeAt(aPosition) == type_string || typeAt(aPosition) == type_binary)
	{
		if (getDescriptor(aPosition)->value == 0)	   return 0;
		return (void*)((unsigned)this + (unsigned)getDescriptor(aPosition)->value);
	}
	else return getDescriptor(aPosition)->value;
}

void* Packet::getControlValueAt(int aPosition) const
{
	if (controlTypeAt(aPosition) == type_string || controlTypeAt(aPosition) == type_binary)
	{
		if (getControlDescriptor(aPosition)->value == 0)	   return 0;
		return (void*)((unsigned)this + (unsigned)getControlDescriptor(aPosition)->value);
	}
	else return getControlDescriptor(aPosition)->value;
}

unsigned Packet::getBinarySizeAt(int aPosition) const
{
	unsigned* value = (unsigned*)getValueAt(aPosition);
	
	return value ? *value : 0;
}

unsigned Packet::aligned(unsigned aNumber)
{ 
	return aNumber % sizeof(int) == 0 ? aNumber : aNumber + sizeof(int) - aNumber % sizeof(int); 
}

void Packet::setIntegerAt(int aPosition, int anInt)					
{ 
	getDescriptor(aPosition)->type = type_integer;
	getDescriptor(aPosition)->value = (void*)anInt;
}

void Packet::setControlIntegerAt(int aPosition, int anInt)					
{ 
	getControlDescriptor(aPosition)->type = type_integer;
	getControlDescriptor(aPosition)->value = (void*)anInt;
}

void Packet::setUnsignedAt(int aPosition, unsigned int anUnsigned)	
{ 
	getDescriptor(aPosition)->type = type_unsigned;
	getDescriptor(aPosition)->value = (void*)anUnsigned;
}

void Packet::setCharAt(int aPosition, char aChar)
{ 
	getDescriptor(aPosition)->type = type_character;
	getDescriptor(aPosition)->value = (void*)aChar;
}

void Packet::setUnsignedCharAt(int aPosition, unsigned char aByte)
{ 
	getDescriptor(aPosition)->type = type_unsignedchar;
	getDescriptor(aPosition)->value = (void*)aByte;
}

void Packet::setStringAt(int aPosition, const char* aString)
{ 
	getDescriptor(aPosition)->type = type_string;

	if (aString == 0)	getDescriptor(aPosition)->value = 0;
	else
	{ 
		int aSize = aligned(strlen(aString) + 1);
		checkForAdditionalSize(aSize);
		memcpy((void*)((unsigned)this + size), aString, aSize);
		getDescriptor(aPosition)->value = (void*)size;
		size += aSize;
	}
}

void Packet::setBinaryAt(int aPosition, unsigned aSize, const void* data)
{
	getDescriptor(aPosition)->type = type_binary;

	if (data == 0)	getDescriptor(aPosition)->value = 0;
	else
	{
		*(unsigned*)((unsigned)this + size) = aSize;
		checkForAdditionalSize(aligned(aSize));
		memcpy((void*)((unsigned)this + sizeof(unsigned) + size), data, aSize);
		getDescriptor(aPosition)->value = (void*)size;
		size += aligned(aSize) + sizeof(unsigned);
	}
}

void Packet::reserveBinaryAt(int aPosition, unsigned aSize)
{
	getDescriptor(aPosition)->type = type_binary;

	*(unsigned*)((unsigned)this + size) = aSize;
	checkForAdditionalSize(aligned(aSize));
	getDescriptor(aPosition)->value = (void*)size;
	size += aligned(aSize) + sizeof(unsigned);
}

void Packet::addControlPart(int numArgs)
{
	controlOffset = size;
	getControlData()->numArgs = numArgs;
	size += 2*sizeof(int) + numArgs * sizeof(Descriptor);
}

void Packet::clear(int aNumArgs)
{
    if (magic == swap(packet_magic))  swapByteOrder(0);
    else magic = packet_magic;

	size = sizeof(Packet) - sizeof(Data) + aNumArgs * sizeof(Descriptor);
	controlOffset = 0;
	getData()->numArgs = aNumArgs;
}

void Packet::setControlContent(int aContent)
{
#if defined(NeXT)
	if (controlOffset == 0) { printf("no control part!\n"); exit(23); }
#else
	if (controlOffset == 0)	throw "no control part!";
#endif	
	getControlData()->content = aContent;
}

int Packet::getControlContent() const
{
    return hasControlData() ? getControlData()->content : -1;
}


void Packet::finalize()
{
}

void Packet::initialize()
{
}

int Packet::verifyMagic(unsigned length, int doSwap)
{
	unsigned mymagic = packet_magic;
    if (doSwap)   mymagic = swap(mymagic);
	
	return memcmp(&magic, &mymagic, length <= sizeof(magic) ? length : sizeof(magic)) == 0;
}

unsigned Packet::swap(unsigned anUnsigned)
{
    unsigned result;

    ((unsigned char*)&result)[0] = ((unsigned char*)&anUnsigned)[3];
    ((unsigned char*)&result)[1] = ((unsigned char*)&anUnsigned)[2];
    ((unsigned char*)&result)[2] = ((unsigned char*)&anUnsigned)[1];
    ((unsigned char*)&result)[3] = ((unsigned char*)&anUnsigned)[0];

    return result;
}

void Packet::swapByteOrder(int isNative)
{
    unsigned index;

    magic = swap(magic);
    timestamp = swap(timestamp);
    size = swap(size);
    syncMajor = swap(syncMajor);
    syncMinor = swap(syncMinor);

    if (!isNative) getData()->numArgs = swap(getData()->numArgs);
    for (index = 0; index < getNumArgs(); index++)
    {
        if (isNative)
        {
            switch (getDescriptor(index)->type)
            {
            case type_binary:
                if (getValueAt(index)) *(unsigned*)(getValueAt(index)) = (unsigned)swap(*(unsigned*)(getValueAt(index)));
                break;
            default:
                break;
            }
        }
        getDescriptor(index)->type = swap(getDescriptor(index)->type);
        getDescriptor(index)->value = (void*)swap((unsigned)getDescriptor(index)->value);
        if (!isNative)
        {
            switch (getDescriptor(index)->type)
            {
            case type_binary:
                if (getValueAt(index)) *(unsigned*)(getValueAt(index)) = (unsigned)swap(*(unsigned*)(getValueAt(index)));
                break;
            default:
                break;
            }
        }
     }
    if (isNative)  getData()->numArgs = swap(getData()->numArgs);
    getData()->content = swap(getData()->content);

    if (hasControlData())
    {
        if (!isNative) 
        {
            controlOffset = swap(controlOffset);
            getControlData()->numArgs = swap(getControlData()->numArgs);
        }
        for (index = 0; index < getControlData()->numArgs; index++)
        {
            if (isNative)
            {
                switch (getDescriptor(index)->type)
                {
                case type_binary:
                    if (getControlValueAt(index)) *(unsigned*)(getControlValueAt(index)) = (unsigned)swap(*(unsigned*)(getControlValueAt(index)));
                    break;
                default:
                    break;
                }
            }
            getControlDescriptor(index)->type = swap(getControlDescriptor(index)->type);
            getControlDescriptor(index)->value = (void*)swap((unsigned)getControlDescriptor(index)->value);
            if (!isNative)
            {
                switch (getDescriptor(index)->type)
                {
                case type_binary:
                    if (getControlValueAt(index)) *(unsigned*)(getControlValueAt(index)) = (unsigned)swap(*(unsigned*)(getControlValueAt(index)));
                    break;
                default:
                    break;
                }
            }

        }
        getControlData()->content = swap(getControlData()->content);
        if (isNative) 
        {
            getControlData()->numArgs = swap(getControlData()->numArgs);
            controlOffset = swap(controlOffset);
        }
    }
}

std::ostream& operator<<(std::ostream& out, const Packet& aPacket)
{
	unsigned index;
	
	out << "Packet. size " << aPacket.size << " syncMajor " << aPacket.getSyncMajor()
		<< " syncMinor " << aPacket.getSyncMinor() << std::endl;
	
	for(index=0;index<aPacket.getNumArgs();index++)
	{
        if (index == 0) out << "content: " << aPacket.getContent() << std::endl;
		out << aPacket.typeAt(index) << ' ';
		switch(aPacket.typeAt(index))
		{
		case Packet::type_unsigned:
			out << "(unsigned)" << aPacket.getUnsignedAt(index) << std::endl;
			break;
		case Packet::type_integer:
			out << "(int)" << aPacket.getIntegerAt(index) << std::endl;
			break;
		case Packet::type_unsignedchar:
			out << "(unsigned char)" << aPacket.getUnsignedCharAt(index) << std::endl;
			break;
		case Packet::type_character:
			out << "(char)" << aPacket.getCharacterAt(index) << std::endl;
			break;
		case Packet::type_string:
			out << "(string)" << aPacket.getStringAt(index) << std::endl;
			break;
		case Packet::type_binary:
			out << "(binary)" << aPacket.getBinarySizeAt(index) << std::endl;
			break;
		}
	}
	if (aPacket.hasControlData()) 
		out << "control part content " << aPacket.getControlContent() << std::endl;
	
	return out;
}

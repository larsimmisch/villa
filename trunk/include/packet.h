/*
    packet.h

	$Id: packet.h,v 1.4 2001/05/19 18:12:38 lars Exp $

	Caution! 
	
	Assumes sizeof(int) == sizeof(void*).

	Copyright 1995, 1996 Immisch, Becker & Partner, Hamburg

	Author: Lars Immisch <lars@ibp.de>
*/

#if !defined(_PACKET_H_)
#define _PACKET_H_

#include <iostream>

#define packet_magic 0xcd011166

class TCP;

class Packet
{
public:
	
	enum types 
	{ 
		type_unsigned = 0,
		type_integer,
		type_unsignedchar,
		type_character,
		type_string,
		type_binary,
		type_double,
		type_point,
		type_date,
		type_time,
		type_timestamp	
	};
	
protected:
	
	class Descriptor
	{
	public:
		
		unsigned type;
		void* value;
	};

	class Data
	{
	public:
		
		unsigned content;
		unsigned numArgs;
	};
	
	inline Data* getData() const;
	inline Descriptor* getDescriptor(int pos) const;
 
	inline Data* getControlData() const;
	inline Descriptor* getControlDescriptor(int pos) const;

public:

	Packet(int numArgs, int maxSize);
	~Packet();
	
	//  user data content manipulation. 
	void setContent(int aContent) { getData()->content = aContent; }
	
	// filling the packet
	void setIntegerAt(int aPosition, int anInt);	
	void setUnsignedAt(int aPosition, unsigned int anUnsigned);
	void setCharAt(int aPosition, char aChar);
	void setUnsignedCharAt(int aPosition, unsigned char aByte);
	void setStringAt(int aPosition, const char* aString);
	void setBinaryAt(int aPosition, unsigned int aSize, const void * data);
    void reserveBinaryAt(int aPosition, unsigned int aSize);
	
	// reading the packet
    int getContent() const { return getData()->content; }
	unsigned typeAt(int aPosition) const { return getDescriptor(aPosition)->type; }
	unsigned getNumArgs() const	{ return getData()->numArgs; }
	void* getValueAt(int aPosition) const;

	int getIntegerAt(int aPosition)	const { return (int)getValueAt(aPosition); }
	unsigned getUnsignedAt(int aPosition) const { return (unsigned)getValueAt(aPosition); }
	char getCharacterAt(int aPosition) const { return (unsigned)getValueAt(aPosition); }
	unsigned char getUnsignedCharAt(int aPosition) const { return (unsigned)getValueAt(aPosition); }
	char* getStringAt(int aPosition) const { return (char*)getValueAt(aPosition); }
	unsigned getBinarySizeAt(int aPosition) const;
	void* getBinaryAt(int aPosition) const;
	
    int isIntegerAt(int aPosition) const { return typeAt(aPosition) == type_integer; }
    int isUnsignedAt(int aPosition) const { return typeAt(aPosition) == type_unsigned; }
    int isCharacterAt(int aPosition) const { return typeAt(aPosition) == type_character; }
    int isStringAt(int aPosition) const { return typeAt(aPosition) == type_string; }

	// clear the packet and prepare it for aNumArgs new arguments
	void clear(int aNumArgs);
	
	// control data manipulation
	void addControlPart(int numArgs);
	void setControlContent(int aContent);
    void setControlIntegerAt(int aPosition, int anInt);
	void setSync(unsigned aSyncMajor, unsigned aSyncMinor)	{ syncMajor = aSyncMajor; syncMinor = aSyncMinor; }
	
	// reading the control data part
	int verifyMagic(unsigned sizeRead, int swap);
	unsigned getSyncMajor()	const { return syncMajor; }
	unsigned getSyncMinor() const { return syncMinor; }
	int hasControlData() const { return controlOffset != 0; }

	int getControlContent() const;
	unsigned controlTypeAt(int aPosition) const	{ return getControlDescriptor(aPosition)->type; }

    void* getControlValueAt(int aPosition) const;
    int getControlIntegerAt(int aPosition) const { return (int)getControlValueAt(aPosition); }
 
	// miscellaneous
	unsigned getSize() const { return size; }
		
	unsigned aligned(unsigned aNumber);
    unsigned swap(unsigned);

    void swapByteOrder(int isNative);
	
	void* operator[](unsigned aPosition) { return (void*)((unsigned)this + aPosition); }
	
	friend std::ostream& operator<<(std::ostream& out, const Packet& aPacket);
	
	static int sizeWithArgs(int numArgs) { return sizeof(Packet) + numArgs * sizeof(Descriptor); }
	
	int checkForAdditionalSize(unsigned aSize);
	
protected:

	friend class TCP;

	// finalize must be called before the packet may be sent
	void finalize();

	// initialize must be called after the packet was received
	void initialize();

	unsigned magic;
	unsigned timestamp;
	unsigned size;
	unsigned syncMajor;
	unsigned syncMinor;
	unsigned controlOffset;
	char fill[2*sizeof(Data)]; // now sizeof(Packet) is large enough for user and control data with 0 args
};

// the ugly side of life....

inline Packet::Data* Packet::getData() const
{ 
    return (Data*)((unsigned)this + sizeof(Packet) - 2*sizeof(Data)); 
}

inline Packet::Descriptor* Packet::getDescriptor(int pos) const
{ 
    return (Descriptor*)((unsigned)this + sizeof(Packet) - sizeof(Data) + pos * sizeof(Descriptor)); 
}
 
inline Packet::Data* Packet::getControlData() const
{
    return controlOffset ? (Data*)((unsigned)this + controlOffset) : 0; 
}

inline Packet::Descriptor* Packet::getControlDescriptor(int pos) const
{ 
    return (Descriptor*)((unsigned)getControlData() + sizeof(Data) + pos * sizeof(Descriptor)); 
}

inline void* Packet::getBinaryAt(int aPosition) const
{ 
    return (void*)((unsigned)getValueAt(aPosition) + sizeof(unsigned)); 
}

#endif /* _PACKET_H_ */

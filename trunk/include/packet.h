/*
    packet.h

	$Id: packet.h,v 1.3 2000/10/18 11:13:10 lars Exp $

	Caution! 
	
	Assumes sizeof(int) == sizeof(void*).

	Copyright 1995, 1996 Immisch, Becker & Partner, Hamburg

	Author: Lars Immisch <lars@ibp.de>
*/

#if !defined(_PACKET_H_)
#define _PACKET_H_

#include <iostream.h>

#ifndef _export
#define _export	__declspec( dllexport )
#endif

#define packet_magic 0xcd011166

class TCP;

class _export Packet
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
	
	inline Data* getData();
	inline Descriptor* getDescriptor(int pos);
 
	inline Data* getControlData();
	inline Descriptor* getControlDescriptor(int pos);

	public:

	Packet(int numArgs, int maxSize);
	~Packet();
	
	//  user data content manipulation. 
	void setContent(int aContent)	{ getData()->content = aContent; }
	
	// filling the packet
	void setIntegerAt(int aPosition, int anInt);	
	void setUnsignedAt(int aPosition, unsigned int anUnsigned);
	void setCharAt(int aPosition, char aChar);
	void setUnsignedCharAt(int aPosition, unsigned char aByte);
	void setStringAt(int aPosition, const char* aString);
	void setBinaryAt(int aPosition, unsigned int aSize, const void * data);
    void reserveBinaryAt(int aPosition, unsigned int aSize);
	
	// reading the packet
    int getContent()                                { return getData()->content; }
	unsigned typeAt(int aPosition)					{ return getDescriptor(aPosition)->type; }
	unsigned getNumArgs()							{ return getData()->numArgs; }
	void* getValueAt(int aPosition);			

	int getIntegerAt(int aPosition)			{ return (int)getValueAt(aPosition); }
	unsigned getUnsignedAt(int aPosition)		{ return (unsigned)getValueAt(aPosition); }
	char getCharacterAt(int aPosition)			{ return (unsigned)getValueAt(aPosition); }
	unsigned char getUnsignedCharAt(int aPosition)	{ return (unsigned)getValueAt(aPosition); }
	char* getStringAt(int aPosition)			{ return (char*)getValueAt(aPosition); }
	unsigned getBinarySizeAt(int aPosition);
	void* getBinaryAt(int aPosition);
	
    int isIntegerAt(int aPosition)      { return typeAt(aPosition) == type_integer; }
    int isUnsignedAt(int aPosition)     { return typeAt(aPosition) == type_unsigned; }
    int isCharacterAt(int aPosition)    { return typeAt(aPosition) == type_character; }
    int isStringAt(int aPosition)       { return typeAt(aPosition) == type_string; }

	// clear the packet and prepare it for aNumArgs new arguments
	void clear(int aNumArgs);
	
	// control data manipulation
	void addControlPart(int numArgs);
	void setControlContent(int aContent);
    void setControlIntegerAt(int aPosition, int anInt);
	void setSync(unsigned aSyncMajor, unsigned aSyncMinor)	{ syncMajor = aSyncMajor; syncMinor = aSyncMinor; }
	
	// reading the control data part
	int verifyMagic(unsigned sizeRead, int swap);
	unsigned getSyncMajor()							{ return syncMajor; }
	unsigned getSyncMinor()							{ return syncMinor; }
	int hasControlData()						{ return controlOffset != 0; }

	int getControlContent();
	unsigned controlTypeAt(int aPosition)				{ return getControlDescriptor(aPosition)->type; }

    void* getControlValueAt(int aPosition);
    int getControlIntegerAt(int aPosition)              { return (int)getControlValueAt(aPosition); }
 

	// miscellaneous
	unsigned getSize() 	{ return size; }
		
	unsigned aligned(unsigned aNumber);
    unsigned swap(unsigned);

    void swapByteOrder(int isNative);
	
	void* operator[](unsigned aPosition)		{ return (void*)((unsigned)this + aPosition); }
	
	friend _export ostream& operator<<(ostream& out, Packet& aPacket);
	
	static int sizeWithArgs(int numArgs)		{ return sizeof(Packet) + numArgs * sizeof(Descriptor); }
	
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

inline Packet::Data* Packet::getData()
{ 
    return (Data*)((unsigned)this + sizeof(Packet) - 2*sizeof(Data)); 
}

inline Packet::Descriptor* Packet::getDescriptor(int pos)
{ 
    return (Descriptor*)((unsigned)this + sizeof(Packet) - sizeof(Data) + pos * sizeof(Descriptor)); 
}
 
inline Packet::Data* Packet::getControlData()	
{ 
    return controlOffset ? (Data*)((unsigned)this + controlOffset) : 0; 
}

inline Packet::Descriptor* Packet::getControlDescriptor(int pos)
{ 
    return (Descriptor*)((unsigned)getControlData() + sizeof(Data) + pos * sizeof(Descriptor)); 
}

inline void* Packet::getBinaryAt(int aPosition)
{ 
    return (void*)((unsigned)getValueAt(aPosition) + sizeof(unsigned)); 
}

#endif /* _PACKET_H_ */

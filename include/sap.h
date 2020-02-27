/*
 	SAP.h

	$Id$

	Copyright 1995-2001 Lars Immisch

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _SAP_H_
#define _SAP_H_

#include <iostream>

class SAP
{
public:
	
	SAP(const char* anAddress = 0, const char* aService = 0, const char* aSelector = 0) : address(0), service(0), selector(0)
	{
		setMember(&address, anAddress);
		setMember(&service, aService);
		setMember(&selector, aSelector);
	}

	SAP(const SAP& aSAP)
	{
		address = 0;
		service = 0;
		selector = 0;

		setMember(&address, aSAP.getAddress());
		setMember(&service, aSAP.getService());
		setMember(&selector, aSAP.getSelector());
	}

	~SAP()
	{
		delete address;
		delete service;
		delete selector;
	}
	
	void setAddress(const char* anAddress)	{ setMember(&address, anAddress); }
	void setService(const char* aService)	{ setMember(&service, aService); }
	void setService(int aService)
	{
		char buffer[16];
		
		sprintf(buffer, "%d", aService);
		setService(buffer);
	}
	void setSelector(const char* aSelector)	{ setMember(&selector, aSelector); }
	void setSelector(int aSelector)
	{
		char buffer[16];
		
		sprintf(buffer, "%d", aSelector);
		setService(buffer);
	}
	
	const char* getAddress() const { return address; }
	const char* getService() const { return service; }
	const char* getSelector() const { return selector; }
	
	void clear()
	{
		setMember(&address, 0);
		setMember(&service, 0);
		setMember(&selector, 0);
	}

    SAP& operator=(const SAP& aSAP)
	{
		setAddress(aSAP.getAddress());
		setService(aSAP.getService());
		setSelector(aSAP.getSelector());

		return *this;
	}

	int operator==(const SAP& aSAP)
	{
		return isEqual(aSAP.getAddress(), getAddress())
			&& isEqual(aSAP.getService(), getService())
			&& isEqual(aSAP.getSelector(), getSelector());
	}

	friend std::ostream& operator<<(std::ostream& out, const SAP& aSAP);

protected:
	
	static int isEqual(const char* a, const char* b)
	{
		// 0 is treated as a wildcard

		return (!a || !b) || (strcmp(a, b) == 0);
	}

	void setMember(char** aMember, const char* aValue)
	{
		if (*aMember) delete *aMember;

		if (aValue != 0)
		{
    		*aMember = new char[strlen(aValue) +1];
    		strcpy(*aMember, aValue);
		} 
		else 
			*aMember = 0;
	}
	
	char* address;
	char* service;
	char* selector;
};

static std::ostream& operator<<(std::ostream& out, const SAP& aSAP)
{
	out << '(';
	if (aSAP.getAddress())
		out << aSAP.getAddress();
	out << ',';
	if (aSAP.getService())
		out << aSAP.getService(); 
	out << ',';
	if (aSAP.getSelector())
		out << aSAP.getSelector();
	out << ')';

	return out;
}


#endif

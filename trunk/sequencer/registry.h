/*
	Copyright 1995-2001 Lars Immisch

	created: Fri Nov 13 15:57:44 GMT+0100 1996

	Author: Lars Immisch <lars@ibp.de>
*/

#ifndef _REGISTRY_H_
#define _REGISTRY_H_

#include <windows.h>

// this is a rough outline.
// error reporting is not sensibly done

class RegistryKey
{
public:
	
	RegistryKey(HKEY root, const char* key);
	~RegistryKey();

	int exists();
	
	char* getStringAt(const char* subKey);
	unsigned getUnsignedAt(const char* subKey);

	int hasValue(const char* subKey);
	
protected:

	HKEY hKey;
	char buffer[256];
};

#endif

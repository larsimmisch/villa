/*
	Copyright 1995-2001 Lars Immisch

	created: Fri Nov 13 15:57:44 GMT+0100 1996

	Author: Lars Immisch <lars@ibp.de>
*/

#include "Registry.h"

RegistryKey::RegistryKey(HKEY root, const char* key)
{
	unsigned result;

	result = RegOpenKeyEx(root, key, 0, KEY_ALL_ACCESS, &hKey);
	if (result != ERROR_SUCCESS)	hKey = NULL;
}

RegistryKey::~RegistryKey()
{
	if (hKey)	RegCloseKey(hKey);
}

int RegistryKey::exists()
{
	return hKey != NULL;
}

char* RegistryKey::getStringAt(const char* subKey)
{
	unsigned long type;
	unsigned long size = sizeof(buffer);
	
	unsigned result = RegQueryValueEx(hKey, subKey, NULL, &type, (LPBYTE)buffer, &size);

	if (result != ERROR_SUCCESS) return 0;

	if (type != REG_SZ)	return 0;

	return buffer;
}

unsigned RegistryKey::getUnsignedAt(const char* subKey)
{
	unsigned long type;
	unsigned value;
	unsigned long size = sizeof(value);

	unsigned result = RegQueryValueEx(hKey, subKey, NULL, &type, (LPBYTE)&value, &size);

	if (result != ERROR_SUCCESS) return 0;

	if (type != REG_DWORD)	return 0;

	return value;
}

int RegistryKey::hasValue(const char* subKey)
{
	unsigned long type;
	unsigned long size = sizeof(buffer);
	
	unsigned result = RegQueryValueEx(hKey, subKey, NULL, &type, (LPBYTE)buffer, &size);

	return result == ERROR_SUCCESS;
}
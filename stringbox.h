#ifndef _STRINGBOX_
#define _STRINGBOX_

/*
	A collection of ASCII strings. Used by elf.cpp, pe.cpp
	etc for creating string tables for those formats.
*/

#include <stdlib.h>

class StringBox {

public:

	StringBox();
	~StringBox();

	char * getText(int);
	int getID(const char *);
	int add(const char *);
	int getInternalID();
	void dump();
	void clear();

	size_t dataSize() { return datap; }
	char * getData() { return data; }
	int offsetOf(int i) { return ids[i - 1]; }

protected:

	int idp;
	int internal_idp;
	size_t datap;
	int * ids;
	char * data;

};

#endif

#ifndef _RTTI_
#define _RTTI_

/*
	Enki images have a .rtti section containing run time
	type information about non-raw structs (i.e. objects). Each
	struct has a pointer to an RTTI record, currently consisting
	of an integer type code used by generic methods and a text
	string with the name of the struct's type.
*/

#include <map>
#include "type.h"

// Builds the rtti section in object files
class Rtti
{
public:

	Rtti()
	{
		data = 0;
		count = 0;
	}

	~Rtti()
	{
		delete[] data;
	}

	void finalise();
	uint64_t lookup(uint64_t);

	uint64_t size()
	{
		return count;
	}

	unsigned char * getData()
	{
		return data;
	}

protected:

        std::map<uint64_t, uint64_t> indexes;
	unsigned char * data;
	uint64_t count;

};

extern Rtti * rtti;

#endif

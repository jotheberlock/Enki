#ifndef _MACHO_
#define _MACHO_

#include "image.h"
#include "stringbox.h"

/*
	Generates Mach-O object files. Currently only fully functional
	for amd64 because Mach-O wants a LC_UNIXTHREAD section describing
	how to set the CPU registers for initial thread of the process
	and this is inherently architecture specific (even though the
	only register we actually care about is the instruction pointer).
*/

class MachOImage : public Image
{
public:

	MachOImage();
	~MachOImage();
	void finalise();
	bool configure(std::string, std::string);
	std::string name() { return "macho"; }

	virtual uint64 importAddress(std::string)
	{
		return 0;
	}

	virtual uint64 importOffset(std::string)
	{
		return 0;
	}

protected:

	bool le;
	int arch_subtype;

};

#endif

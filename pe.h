#ifndef _PE_
#define _PE_

/* 
	Outputs Windows PE-COFF executables, both 32- and 64-bit.
	Little endian only because there has never been to my 
	knowledge a big endian Windows.
*/

#include "image.h"

class PEImage : public Image
{
public:

	PEImage();
	~PEImage();
	std::string name() { return "pe"; }
	bool configure(std::string, std::string);
	void finalise();

	virtual uint64 importAddress(std::string);
	virtual uint64 importOffset(std::string)
	{
		return 0;
	}

	virtual void endOfImports();

protected:

	int subsystem;
	uint64 imports_base;
	uint64 symbols_base;
	int os_major;
	int os_minor;

};

#endif

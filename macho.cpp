#include "macho.h"
#include "platform.h"
#include "symbols.h"
#include <stdio.h>
#include <string.h>

#if defined(POSIX_HOST)
#include <sys/stat.h>
#endif

MachOImage::MachOImage()
{
	bases[IMAGE_UNALLOCED_DATA] = 0x800000;
	sizes[IMAGE_UNALLOCED_DATA] = 4096 * 64;
	le = true;
	arch_subtype = 0;
}

MachOImage::~MachOImage()
{
	for (int loopc = 0; loopc < IMAGE_LAST; loopc++)
	{
		delete[] sections[loopc];
	}
}

bool MachOImage::configure(std::string param, std::string val)
{
	if (param == "endian")
	{
		if (val == "little")
		{
			le = true;
		}
		else if (val == "big")
		{
			le = false;
		}
		else
		{
			return false;
		}
	}
	else if (param == "arch_subtype")
	{
		arch_subtype = strtol(val.c_str(), 0, 10);
	}
	else
	{
		return Image::configure(param, val);
	}

	return true;
}

void MachOImage::finalise()
{
	FILE * f = fopen(fname.c_str(), "wb+");
	if (!f)
	{
		printf("Can't open %s\n", fname.c_str());
		return;
	}

	unsigned char * header = new unsigned char[4096];
	memset(header, 0, 4096);

	unsigned char * ptr = header;

	uint32 magic = (sf_bit) ? 0xfeedfacf : 0xfeedface;
	wee32(le, ptr, magic);
	wee32(le, ptr, arch);
	wee32(le, ptr, arch_subtype);   // cpu subtype
	wee32(le, ptr, 0x2);   // filetype - executable
	wee32(le, ptr, IMAGE_LAST + 2);   // no. cmds
	wee32(le, ptr, sf_bit ? ((72 * (IMAGE_LAST+1)) + 184) : ((56 * (IMAGE_LAST+1)) + 80));   // size of cmds
	wee32(le, ptr, 0x1);   // flags - no undefs
	if (sf_bit)
	{
		wee32(le, ptr, 0x0); // Reserved
	}

	// Write a page zero
	wee32(le, ptr, sf_bit ? 0x19 : 0x1);  // command type
	wee32(le, ptr, sf_bit ? 72 : 56);     // command size
	char buf[16];
	strcpy(buf, "__PAGEZERO");
	strcpy((char *)ptr, buf);
	ptr += 16;
	if (sf_bit)
	{
		wee64(le, ptr, 0x0);
		wee64(le, ptr, 0x1000);
		wee64(le, ptr, 0x0);
		wee64(le, ptr, 0x0);
	}
	else
	{
		wee32(le, ptr, 0x0);
		wee32(le, ptr, 0x1000);
		wee32(le, ptr, 0x0);
		wee32(le, ptr, 0x0);
	}

	wee32(le, ptr, 0);      // maxprot
	wee32(le, ptr, 0);      // initprot
	wee32(le, ptr, 0);      // nsects
	wee32(le, ptr, 0x4);    // flags - no reloc

	for (int loopc = 0; loopc < IMAGE_LAST; loopc++)
	{
		wee32(le, ptr, sf_bit ? 0x19 : 0x1);  // command type
		wee32(le, ptr, sf_bit ? 72 : 56);     // command size
		char buf[16];
		int prot = 0;
		if (loopc == IMAGE_CODE)
		{
			strcpy(buf, "__TEXT");
			prot = 0x5;
		}
		else if (loopc == IMAGE_DATA)
		{
			strcpy(buf, "__DATA");
			prot = 0x3;
		}
		else if (loopc == IMAGE_CONST_DATA)
		{
			strcpy(buf, "__RDATA");
			prot = 0x1;
		}
		else if (loopc == IMAGE_UNALLOCED_DATA)
		{
			strcpy(buf, "__BSS");
			prot = 0x3;
		}
		else if (loopc == IMAGE_RTTI)
		{
			strcpy(buf, "__RTTI");
			prot = 0x1;
		}
		else if (loopc == IMAGE_MTABLES)
		{
			strcpy(buf, "__MTABLES");
			prot = 0x3;
		}
		else if (loopc == IMAGE_EXPORTS)
		{
			strcpy(buf, "__EXPORTS");
			prot = 0x3;
		}
		else
		{
			assert(false);
		}

		strcpy((char *)ptr, buf);
		ptr += 16;
		if (sf_bit)
		{
			wee64(le, ptr, bases[loopc]);
			wee64(le, ptr, roundup(sizes[loopc], 4096));
			wee64(le, ptr, (loopc == IMAGE_UNALLOCED_DATA) ? 0 : bases[loopc] - base_addr);
			wee64(le, ptr, (loopc == IMAGE_UNALLOCED_DATA) ? 0 : roundup(sizes[loopc], 4096));
		}
		else
		{
			wee32(le, ptr, checked_32(bases[loopc]));
			wee32(le, ptr, checked_32(roundup(sizes[loopc], 4096)));
			wee32(le, ptr, checked_32(bases[loopc] - base_addr));
			wee32(le, ptr, (loopc == IMAGE_UNALLOCED_DATA) ? 0 : checked_32(roundup(sizes[loopc], 4096)));
		}
		wee32(le, ptr, prot);   // maxprot
		wee32(le, ptr, prot);   // initprot
		wee32(le, ptr, 0);      // nsects
		wee32(le, ptr, 0x4);    // flags - no reloc
	}

	// note this is x86-specific right now!
	if (sf_bit)
	{
		wee32(le, ptr, 0x5);
		wee32(le, ptr, 184);
		wee32(le, ptr, 0x4);
		wee32(le, ptr, 42);
		for (int loopc = 0; loopc < 42; loopc++)
		{
			wee64(le, ptr, loopc == 16 ? functionAddress(root_function) : 0);
		}
	}
	else
	{
		wee32(le, ptr, 0x5);   // LC_UNIXTHREAD
		wee32(le, ptr, 80);    // size
		wee32(le, ptr, 1);     // flavour
		wee32(le, ptr, 16);    // count
		for (int loopc = 0; loopc < 16; loopc++)
		{
			wee32(le, ptr, loopc == 10 ? checked_32(functionAddress(root_function)) : 0);
		}
	}

	fwrite(header, 4096, 1, f);
	delete[] header;

	for (int loopc = 0; loopc < IMAGE_LAST; loopc++)
	{
		if (loopc != IMAGE_UNALLOCED_DATA)
		{
			fseek(f, (long)(bases[loopc] - base_addr), SEEK_SET);
			fwrite(sections[loopc], sizes[loopc], 1, f);
		}
	}

	fclose(f);
#if defined(POSIX_HOST)
	chmod(fname.c_str(), 0755);
#endif
}

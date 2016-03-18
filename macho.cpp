#include "macho.h"
#include "platform.h"
#include "symbols.h"
#include <stdio.h>
#include <string.h>

#if defined(LINUX_HOST) || defined(CYGWIN_HOST)
#include <sys/stat.h>
#endif

MachOImage::MachOImage()
{
    bases[IMAGE_UNALLOCED_DATA] = 0x800000;
    sizes[IMAGE_UNALLOCED_DATA] = 4096;
    le = true;
    arch_subtype = 0;
}

MachOImage::~MachOImage()
{
    for (int loopc=0; loopc<4; loopc++)
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

    uint32_t magic = (sf_bit) ? 0xfeedfacf : 0xfeedface;
    wee32(le, ptr, magic);
    wee32(le, ptr, arch);
    wee32(le, ptr, arch_subtype);   // cpu subtype
    wee32(le, ptr, 0x2);   // filetype - executable
    wee32(le, ptr, 0x0);   // no. cmds
    wee32(le, ptr, 0x0);   // size of cmds
    wee32(le, ptr, 0x1);   // flags - no undefs
    if (sf_bit)
    {
        wee32(le, ptr, 0x0); // Reserved
    }

    for (int loopc=0; loopc<4; loopc++)
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
	strcpy((char *)ptr, buf);
	ptr += 16;
	if (sf_bit)
	{
  	    wee64(le, ptr, bases[loopc]);
	    wee64(le, ptr, sizes[loopc]);
	    wee64(le, ptr, bases[loopc]-base_addr);
	    wee64(le, ptr, (loopc == IMAGE_UNALLOCED_DATA) ? 0 : sizes[loopc]);
        }
	else
        {
  	    wee32(le, ptr, bases[loopc]);
	    wee32(le, ptr, sizes[loopc]);
	    wee32(le, ptr, bases[loopc]-base_addr);
	    wee32(le, ptr, (loopc == IMAGE_UNALLOCED_DATA) ? 0 : sizes[loopc]);
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
	for (int loopc=0; loopc<42; loopc++)
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
        for (int loopc=0; loopc<16; loopc++)
        {
  	    wee32(le, ptr, loopc == 10 ? functionAddress(root_function) : 0);
        }
    }
    
    fwrite(header, 4096, 1, f);
    delete[] header;
    
    fclose(f);
#if defined(LINUX_HOST) || defined(CYGWIN_HOST)
    chmod(fname.c_str(), 0755);
#endif
}

void MachOImage::materialiseSection(int s)
{
    sections[s] = new unsigned char[sizes[s]];
    bases[s] = next_addr;
    next_addr += sizes[s];
    next_addr += 4096;
    while (next_addr % 4096)
    {
        next_addr++;
    }

    if (guard_page)
    {
        next_addr += 4096;   // Create a guard page
    }
}


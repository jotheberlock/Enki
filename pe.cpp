#include "pe.h"
#include "platform.h"
#include "symbols.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

PEImage::PEImage()
{
    base_addr = 0x400000;
    next_addr = base_addr + 12288;
    fname = "a.exe";
    sf_bit = false;
    arch = 0;

    bases[3] = 0x800000;
    sizes[3] = 4096;    
}

PEImage::~PEImage()
{
    for (int loopc=0; loopc<4; loopc++)
    {
        delete[] sections[loopc];
    }
}

bool PEImage::configure(std::string param, std::string val)
{
    if (param == "file")
    {
      fname = val;
    }
    else if (param == "bits")
    {
        if (val == "64")
	{
  	  sf_bit = true;
	}
        else if (val == "32")
	{
  	  sf_bit = false;
	}
        else
	{
	  return false;
	}
    }
    else if (param == "arch")
    {
      arch = strtol(val.c_str(), 0, 10);
    }
    else
    {
      return false;
    }

    return true;
}

void PEImage::finalise()
{
    FILE * f = fopen(fname.c_str(), "w+");
    if (!f)
    {
        printf("Can't open %s\n", fname.c_str());
        return;
    }

    // Little-endian only
    
    unsigned char dosheader[256];
    memset(dosheader, 0, 256);

    dosheader[0] = 0x4d;
    dosheader[1] = 0x5a;
    dosheader[2] = 0x50;
    dosheader[4] = 0x2;
    dosheader[8] = 0x4;
    dosheader[0xa] = 0xf;
    dosheader[0xc] = 0xff;
    dosheader[0xd] = 0xff;
    dosheader[0x10] = 0xb8;
    dosheader[0x18] = 0x40;
    dosheader[0x1a] = 0x1a;
    dosheader[0x3d] = 0x1;
    dosheader[0x40] = 0xba;
    dosheader[0x41] = 0x10;
    dosheader[0x43] = 0xe;
    dosheader[0x44] = 0x1f;
    dosheader[0x45] = 0xb4;
    dosheader[0x46] = 0x9;
    dosheader[0x47] = 0xcd;
    dosheader[0x48] = 0x21;
    dosheader[0x49] = 0xb8;
    dosheader[0x4a] = 0x01;
    dosheader[0x4b] = 0x4c;
    dosheader[0x4c] = 0xcd;
    dosheader[0x4d] = 0x21;
    dosheader[0x4e] = 0x90;
    dosheader[0x4f] = 0x90;
    strcpy((char *)&dosheader[0x50], "This program must be run under Win32");
    dosheader[0x74] = 0xd;
    dosheader[0x75] = 0xa;
    dosheader[0x76] = 0x24;
    dosheader[0x77] = 0x37;
    fwrite(dosheader, 256, 1, f);
    
    // Signature
    unsigned char sbuf[4];
    sbuf[0]='P';
    sbuf[1]='E';
    sbuf[2]=0;
    sbuf[3]=0;
    fwrite((void *)sbuf, 4, 1, f);

    unsigned char * header = new unsigned char[4096];
    memset(header, 0, 4096);

    // COFF header
    unsigned char * ptr = header;
    wle16(ptr, arch);
    wle16(ptr, 5);  // sections
    wle32(ptr, 0);  // timestamp
    wle32(ptr, 0);  // symbol table ptr
    wle32(ptr, 0);  // no. symbols
    wle16(ptr, (sf_bit ? 112 : 96) + (16*8));  // Optional header size
    wle16(ptr, 0x1 | 0x2 | 0x4 | 0x8 | 0x100 | 0x200);  // flags

    // COFF optional header
    wle16(ptr, sf_bit ? 0x20b : 0x10b);  // magic
    *ptr = 6;           // linker major/minor
    ptr++;
    *ptr = 0;
    ptr++;
    wle32(ptr, checked_32(sizes[IMAGE_CODE]));
    wle32(ptr, checked_32(sizes[IMAGE_CONST_DATA]+sizes[IMAGE_DATA]));
    wle32(ptr, checked_32(sizes[IMAGE_UNALLOCED_DATA]));
    wle32(ptr, checked_32(functionAddress(root_function)-base_addr));
    wle32(ptr, checked_32(bases[IMAGE_CODE]-base_addr));
    if (!sf_bit)
    {
        wle32(ptr, checked_32(bases[IMAGE_DATA]-base_addr));
    }

    // PE header
    if (sf_bit)
    {
		wle64(ptr, base_addr); 
    }
    else
    {
        wle32(ptr, checked_32(base_addr));
    }
    wle32(ptr, 4096); // Align
    wle32(ptr, 512);  // File align
    wle16(ptr, 6);    // OS major
    wle16(ptr, 0);    // OS minor
    wle16(ptr, 0);    // Image version
    wle16(ptr, 0);
    wle16(ptr, 0);    // Subsystem version
    wle16(ptr, 0);
    wle32(ptr, 0);    // Reserved
    wle32(ptr, checked_32(base_addr - next_addr));   // Image size
    wle32(ptr, 0x400);  // Headers size
    wle32(ptr, 0);  // Checksum
    wle16(ptr, 0x3);  // Subsystem - Windows CLI
    wle16(ptr, 0); // DLL Flags
    if (sf_bit)
    {
        wle64(ptr, 0x100000);  // Stack reserve
        wle64(ptr, 0x4000);  // Stack commit
        wle64(ptr, 0x100000);  // Heap reserve
        wle64(ptr, 0x4000);   // Heap commit
    }
    else
    {
        wle32(ptr, 0x100000);  // Stack reserve
        wle32(ptr, 0x4000);  // Stack commit
        wle32(ptr, 0x100000);  // Heap reserve
        wle32(ptr, 0x4000);   // Heap commit
    }
    wle32(ptr, 0);  // Loader flags
    wle32(ptr, 16);   // RVA number
    fwrite(header, 4096, 1, f);
    delete[] header;
    fclose(f);
}

void PEImage::materialiseSection(int s)
{
    sections[s] = new unsigned char[sizes[s]];
    bases[s] = next_addr;
    next_addr += sizes[s];
    next_addr += 4096;
    while (next_addr % 4096)
    {
        next_addr++;
    }
    next_addr += 4096;   // Create a guard page
}

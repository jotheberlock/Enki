#include "pe.h"
#include "platform.h"
#include "symbols.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#if defined(LINUX_HOST) || defined(CYGWIN_HOST)
#include <sys/stat.h>
#endif

uint64_t roundup(uint64_t in, uint64_t align)
{
    if (in % align == 0)
    {
       return in;
    }
    return in + (align - (in % align));
}

PEImage::PEImage()
{
    base_addr = 0x400000;
    next_addr = base_addr + (4096 * 3);
    fname = "a.exe";
    sf_bit = false;
    arch = 34404;
    subsystem = 1; // Windows CLI
    bases[IMAGE_UNALLOCED_DATA] = 0x800000;
    sizes[IMAGE_UNALLOCED_DATA] = 4096;
    guard_page = false;
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
    else if (param == "subsystem")
    {
        subsystem = strtol(val.c_str(), 0, 10);
    }
    else if (param == "guard")
    {
        if (val == "true")
	{
    	    guard_page = true;
	}
    }
    else if (param == "baseaddr")
    {
        base_addr = strtol(val.c_str(), 0, 0);
	next_addr = base_addr + 12288;
    }
    else if (param == "heapaddr")
    {
        bases[IMAGE_UNALLOCED_DATA] = strtol(val.c_str(), 0, 0);
    }
    else
    {
        return false;
    }

    return true;
}

void PEImage::endOfImports()
{
    materialiseSection(3);
    imports_base = next_addr;
    next_addr += 4096;
    symbols_base = next_addr;
    next_addr += 4096;
}

uint64_t PEImage::importAddress(std::string s)
{
    uint64_t table_size = (imports.size() * 20)+20;   // import directory table
    uint64_t ilt_size = imports.size() + total_imports;
    ilt_size *= (sf_bit ? 8 : 4);
    uint64_t iat_offset = table_size+ilt_size;
    uint64_t base_offset = imports_base + iat_offset;
    uint64_t stride = sf_bit ? 8 : 4;
    for (unsigned int loopc=0; loopc<imports.size(); loopc++)
    {
        LibImport & l = imports[loopc];
	for (unsigned int loopc2=0; loopc2<l.imports.size(); loopc2++)
	{
  	    if (l.imports[loopc2] == s)
	    {
	      return base_offset;
	    }
	    base_offset += stride;
        }
	base_offset += stride;
    }
    printf("Can't find import [%s]!\n", s.c_str());
    return 0xdeadbeef;
}

void PEImage::finalise()
{
    FILE * f = fopen(fname.c_str(), "wb+");
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
    wle16(ptr, 6);  // sections
    wle32(ptr, checked_32(time(0)));  // timestamp
    wle32(ptr, symbols_base);  // symbol table ptr
    wle32(ptr, 0);  // no. symbols
    wle16(ptr, (sf_bit ? 112 : 96) + (16*8));  // Optional header size

    uint16_t characteristics = 0;
    characteristics |= 0x1;  // Not relocatable
    characteristics |= 0x2;  // Valid file
    characteristics |= 0x4;  // Line numbers removerd
    //characteristics |= 0x8;  // Symbol tables removed
    characteristics |= 0x20; // > 2gig aware
    characteristics |= 0x80; // Little endian
    if (!sf_bit)
    {
	characteristics |= 0x100;    // 32-bit word
    }
    characteristics |= 0x200;   // Debugging information removed
    
    wle16(ptr, characteristics);  // flags

    // COFF optional header
    wle16(ptr, sf_bit ? 0x20b : 0x10b);  // magic
    *ptr = 2;           // linker major/minor
    ptr++;
    *ptr = 24;
    ptr++;
    wle32(ptr, checked_32(roundup(sizes[IMAGE_CODE], 4096)));
    wle32(ptr, checked_32(roundup(sizes[IMAGE_CONST_DATA],4096)+roundup(sizes[IMAGE_DATA],4096)));
    wle32(ptr, checked_32(roundup(sizes[IMAGE_UNALLOCED_DATA],4096)));
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
    wle16(ptr, 4);    // OS major
    wle16(ptr, 0);    // OS minor
    wle16(ptr, 0);    // Image version
    wle16(ptr, 0);
    wle16(ptr, 5);    // Subsystem version
    wle16(ptr, 2);
    wle32(ptr, 0);    // Reserved
    wle32(ptr, checked_32(next_addr - base_addr));   // Image size
    wle32(ptr, 0x3000);  // Headers size
    wle32(ptr, 0x0);  // Checksum
    wle16(ptr, subsystem);  // Subsystem - Windows CLI
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

    // Data directories go here
    for (int loopc=0; loopc<16; loopc++)
    {
        if (loopc == 1)
        {
            wle32(ptr, checked_32(imports_base - base_addr)); // imports base
            wle32(ptr, 4096); // imports size
        }
        else
        {
            wle32(ptr, 0);
            wle32(ptr, 0);
        }
    }
    
    uint64_t prev_base = 0;
    
    for (int loopc=0; loopc<4; loopc++)
    { 
        int the_one = -1;
        uint64_t lowest_diff = 0xffffffff;
        for (int loopc2=0; loopc2<4; loopc2++)
        {
            uint64_t diff = bases[loopc2] - prev_base;
            if ((bases[loopc2] > prev_base) && (diff < lowest_diff))
            {
                lowest_diff = diff;
                the_one = loopc2;
            }
        }

        if (the_one == -1)
        {
            printf("Erk!\n");
            return;
        }
        
        prev_base = bases[the_one];
        
        char sname[8];
        memset(sname, 0, 8);
        uint32_t flags;

        if (the_one == IMAGE_CODE)
        {  
            strcpy(sname, ".text");
            flags = 0x20 | 0x20000000 | 0x40000000;
        }
        else if (the_one == IMAGE_DATA)
        {
            strcpy(sname, ".data");
            flags = 0x40 | 0x40000000 | 0x80000000;
        }
        else if (the_one == IMAGE_CONST_DATA)
        {
            strcpy(sname, ".rdata");
            flags = 0x40 | 0x40000000;
        }
        else if (the_one == IMAGE_UNALLOCED_DATA)
        {
            strcpy(sname, ".bss");
            flags = 0x80 | 0x40000000 | 0x80000000 | 0x600000;
        }

        memcpy(ptr, sname, 8);
        ptr += 8;
        wle32(ptr, checked_32(roundup(sizes[the_one] ? sizes[the_one] : 4096,4096)));
        wle32(ptr, checked_32(bases[the_one] - base_addr));
        wle32(ptr, (the_one == IMAGE_UNALLOCED_DATA) ? 0 : checked_32(roundup(sizes[the_one],4096)));
        wle32(ptr, (the_one == IMAGE_UNALLOCED_DATA) ? 0 : checked_32(bases[the_one] - base_addr));
        wle32(ptr, 0);  // No relocations
        wle32(ptr, 0);  // No line numbers
        wle16(ptr, 0);  // No relocations
        wle16(ptr, 0);  // No line numbers;
        wle32(ptr, flags);
    }

    char sname[8];
    memset(sname, 0, 8);
    strcpy(sname, ".idata");
    memcpy(ptr, sname, 8);
    ptr += 8;
    wle32(ptr, 4096);
    wle32(ptr, checked_32(imports_base - base_addr));
    wle32(ptr, 4096);
    wle32(ptr, checked_32(imports_base - base_addr));
    wle32(ptr, 0);
    wle32(ptr, 0);
    wle16(ptr, 0);
    wle16(ptr, 0);
    wle32(ptr, 0x40 | 0x40000000 | 0x80000000);

    memset(sname, 0, 8);
    strcpy(sname, ".symtab");
    memcpy(ptr, sname, 8);
    ptr += 8;
    wle32(ptr, 4096);
    wle32(ptr, checked_32(symbols_base - base_addr));
    wle32(ptr, 4096);
    wle32(ptr, checked_32(symbols_base - base_addr));
    wle32(ptr, 0);
    wle32(ptr, 0);
    wle16(ptr, 0);
    wle16(ptr, 0);
    wle32(ptr, 0x40 | 0x40000000);
    fwrite(header, 4096, 1, f);
    delete[] header;
    
    for (int loopc=0; loopc<4; loopc++)
    {
        if (loopc != IMAGE_UNALLOCED_DATA)
        {
            fseek(f, checked_32(bases[loopc]-base_addr), SEEK_SET);
            fwrite(sections[loopc], sizes[loopc], 1, f);
        }
    }
    
    uint64_t table_size = (imports.size() * 20)+20;   // import directory table
    uint64_t ilt_size = imports.size() + total_imports;
    ilt_size *= (sf_bit ? 8 : 4);
    uint64_t hints_offset = table_size+ilt_size+ilt_size;
    
    size_t count = 0;
    
    unsigned char buf[4096];
    ptr = buf;
    unsigned char * namebase = buf+hints_offset;
    unsigned char * nameptr = namebase;
    
    for (unsigned int loopc = 0; loopc<imports.size(); loopc++)
    {
        uint64_t table_offset = (imports_base - base_addr)+table_size+count;
        wle32(ptr, checked_32(table_offset));  // Lookup table
        wle32(ptr, 0);   // Timestamp
        wle32(ptr, 0);   // Forwarder
        strcpy((char *)nameptr, imports[loopc].name.c_str());
        uint64_t offy = (imports_base - base_addr) + hints_offset + (nameptr-namebase);
        wle32(ptr, checked_32(offy));   // DLL name
        nameptr += strlen(imports[loopc].name.c_str())+1;
        wle32(ptr, checked_32((imports_base - base_addr)+table_size+ilt_size+count));   // Address of IAT
        count += ((imports[loopc].imports.size()+1) * (sf_bit ? 8 : 4));
    }
    // null entry
    wle32(ptr, 0);
    wle32(ptr, 0);
    wle32(ptr, 0);
    wle32(ptr, 0);
    wle32(ptr, 0);

    for (int loopc=0; loopc<2; loopc++)
    {
            // First is ILT, second is IAT
        for (unsigned int loopc2 = 0; loopc2<imports.size(); loopc2++)
        {
	    LibImport & l = imports[loopc2];
            for (unsigned int loopc3=0;loopc3<l.imports.size(); loopc3++)
            {
		       uint64_t addr = (nameptr-namebase) + (imports_base - base_addr) + hints_offset;
               if (((uint64_t)nameptr) & 0x1)
               {
                    *nameptr = 0;
                    nameptr++;
                    addr++;
                }
                *nameptr = 0x0;
                nameptr++;
                *nameptr = 0x0;
                nameptr++;
                strcpy((char *)nameptr, l.imports[loopc3].c_str());
                
                nameptr += strlen(l.imports[loopc3].c_str())+1;
			  
                if (sf_bit)
                {
                    wle64(ptr, addr);
                }
                else
                {
                    wle32(ptr, checked_32(addr));
                }
            }
            if (sf_bit)
            {
                wle64(ptr, 0);
            }
            else
            {
                wle32(ptr, 0);
            }	
        }
    } 
           
    fseek(f, checked_32(imports_base - base_addr), SEEK_SET);    
    fwrite(buf, 4096, 1, f);

    memset(buf, 0, 4096);
    fseek(f, checked_32(symbols_base - base_addr), SEEK_SET);
    fwrite(buf, 4096, 1, f);

    fclose(f);
    
#if defined(LINUX_HOST) || defined(CYGWIN_HOST)
    chmod(fname.c_str(), 0755);
#endif
}

void PEImage::materialiseSection(int s)
{
    sections[s] = new unsigned char[sizes[s]];
    bases[s] = next_addr;
    next_addr += sizes[s];
    if (!sizes[s])
    {
        next_addr++;
    }
    while (next_addr % 4096)
    {
        next_addr++;
    }

    if (guard_page)
    {
        next_addr += 4096;   
    }
}

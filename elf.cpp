#include "elf.h"
#include "platform.h"
#include "symbols.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

ElfImage::ElfImage()
{
    base_addr = 0x400000;
    next_addr = base_addr + 12288;
    fname = "a.out";
    sf_bit = false;
    le = true;
    arch = 0;

    bases[3] = 0x800000;
    sizes[3] = 4096;    
}

ElfImage::ElfImage(const char * f, bool s, bool l, int a)
{
    base_addr = 0x400000;
    next_addr = base_addr + 12288;
    fname = "a.out";
    sf_bit = false;
    le = true;
    arch = 0;

    bases[3] = 0x800000;
    sizes[3] = 4096;    

    fname = f;
    sf_bit = s;
    le = l;
    arch = a;
}

bool ElfImage::configure(std::string param, std::string val)
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
  else if (param == "endian")
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

ElfImage::~ElfImage()
{
    for (int loopc=0; loopc<4; loopc++)
    {
        delete[] sections[loopc];
    }
}

int ElfImage::stringOffset(const char * c)
{
    int ret=0;
    int tmpid=stringtable.getID(c);
    if(tmpid>0)
    {
        ret=stringtable.offsetOf(tmpid);
    }
    return ret;
}

void ElfImage::finalise()
{
    stringtable.clear();
    stringtable.add(""); // Null byte at start
    stringtable.add(".text");
    stringtable.add(".rodata");
    stringtable.add(".data");
    stringtable.add(".bss");
    stringtable.add(".symtab");
    for (unsigned int loopc=0; loopc<fptrs.size(); loopc++)
    {
        stringtable.add(fptrs[loopc]->name().c_str());
    }
    
    FILE * f = fopen(fname.c_str(), "w+");
    if (!f)
    {
        printf("Can't open %s\n", fname.c_str());
        return;
    }

    unsigned char * header = new unsigned char[4096];
    memset(header, 0, 4096);
    
    unsigned char * ptr = header;
    *ptr = 0x7f;
    ptr++;
    *ptr = 0x45;
    ptr++;
    *ptr = 0x4c;
    ptr++;
    *ptr = 0x46;
    ptr++;
    *ptr = (sf_bit) ? 2 : 1;
    ptr++;
    *ptr = (le) ? 1 : 2;
    ptr++;
    *ptr = 0x1;
    ptr++;
    *ptr = 0x0;  // Target ABI
    ptr++;
    wee64(le, ptr, 0x0);  // ABI version and pad
    wee16(le, ptr, 0x2);  // Executable
    uint16_t elf_arch = 0;
    if (arch == ARCH_AMD64)
    {
        elf_arch = 0x3e;
    }
    else
    {
        printf("Unknown arch for ELF! %d\n", arch);
    }
    wee16(le, ptr, elf_arch);
    wee32(le, ptr, 0x1);  // Version

    int no_pheaders = 4;
    
    if (sf_bit)
    {
        wee64(le, ptr, functionAddress(root_function));
        wee64(le, ptr, 0x40);  // Program header offset
        wee64(le, ptr, 0x40+(56*no_pheaders));  // End of program headers; start of section headers
    }
    else
    {
        wee32(le, ptr, checked_32(functionAddress(root_function)));
        wee32(le, ptr, 0x34);  // Program header offset
        wee32(le, ptr, 0x40+(32*no_pheaders));  // End of program headers; start of section headers
    }
    wee32(le, ptr, 0x0); // Flags
    wee16(le, ptr, sf_bit ? 64 : 52); // This header size
    wee16(le, ptr, sf_bit ? 56 : 32); // pheader size
    wee16(le, ptr, no_pheaders);
    wee16(le, ptr, sf_bit ? 64 : 40); // section header size
    wee16(le, ptr, 7);  // Number of sections
    wee16(le, ptr, 1);  // Section with strings

        /*
        // Program header for image header
    wee32(le, ptr, 0x1);
    wee32(le, ptr, 0x4);
    if (sf_bit)
    {
        wee64(le, ptr, 0);
        wee64(le, ptr, base_addr);
        wee64(le, ptr, base_addr);
        wee64(le, ptr, 8192);
        wee64(le, ptr, 0);
        wee64(le, ptr, 0x4);
    }
    else
    {
        wee32(le, ptr, 0);
        wee32(le, ptr, base_addr);
        wee32(le, ptr, base_addr);
        wee32(le, ptr, 8192);
        wee32(le, ptr, 0);
        wee32(le, ptr, 0x4);
    }
        */
    
    uint64_t prev_base = 0;
    
    for (int loopc=0; loopc<4; loopc++)
    {
        int the_one = 0;
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
        prev_base = bases[the_one];
        
        wee32(le, ptr, 0x1);  // Loadable

        int flags = 0;
        if (the_one == IMAGE_CODE)
        {
            flags = 0x5;
        }
        else if (the_one == IMAGE_DATA)
        {
            flags = 0x6;
        }
        else if (the_one == IMAGE_CONST_DATA)
        {
            flags = 0x4;
        }
        else if (the_one == IMAGE_UNALLOCED_DATA)
        {
            flags = 0x6;
        }
       
        wee32(le, ptr, flags);  // Flags
        if (sf_bit)
        {
            wee64(le, ptr, bases[the_one]-base_addr);
            wee64(le, ptr, bases[the_one]);
            wee64(le, ptr, bases[the_one]);
            wee64(le, ptr, (the_one == IMAGE_UNALLOCED_DATA) ? 0 : sizes[the_one]);
            wee64(le, ptr, sizes[the_one]);
            wee64(le, ptr, 0);
        }
        else
        {
            wee32(le, ptr, checked_32(bases[the_one]-base_addr));
            wee32(le, ptr, checked_32(bases[the_one]));
            wee32(le, ptr, checked_32(bases[the_one]));
            wee32(le, ptr, (the_one == IMAGE_UNALLOCED_DATA) ? 0 : checked_32(sizes[the_one]));
            wee32(le, ptr, checked_32(sizes[the_one]));
            wee32(le, ptr, 0);
        }
    }

    wee32(le, ptr, 0);  // name
    wee32(le, ptr, 0);  // type - NULL
    if (sf_bit)
    {
        wee64(le, ptr, 0);  // flags
        wee64(le, ptr, 0);  // addr
        wee64(le, ptr, 0);  // ofset
        wee64(le, ptr, 0);  // size
    }
    else
    {
        wee32(le, ptr, 0);  // flags
        wee32(le, ptr, 0);  // addr
        wee32(le, ptr, 0);  // ofset
        wee32(le, ptr, 0);  // size
    }
    wee32(le, ptr, 0);  // link - UNDEF
    wee32(le, ptr, 0);  // info
    if (sf_bit)
    {
        wee64(le, ptr, 0);   // align
        wee64(le, ptr, 0);   // entsize
    }
    else
    {
        wee32(le, ptr, 0);   // align
        wee32(le, ptr, 0);   // entsize
    }

    size_t stringtablesize = stringtable.dataSize()+1;
    while (stringtablesize % 8)
    {
        stringtablesize++;
    }
    
    wee32(le, ptr, 0);  // name
    wee32(le, ptr, 3);  // type - string table
    if (sf_bit)
    {
        wee64(le, ptr, 0);  // flags
        wee64(le, ptr, 0);  // addr
        wee64(le, ptr, 4096);  // offset
        wee64(le, ptr, stringtablesize);  // size
    }
    else
    {
        wee32(le, ptr, 0);  // flags
        wee32(le, ptr, 0);  // addr
        wee32(le, ptr, 4096);  // offset
        wee32(le, ptr, checked_32(stringtablesize));  // size
    }
    wee32(le, ptr, 0);  // link - UNDEF
    wee32(le, ptr, 0);  // info
    if (sf_bit)
    {
        wee64(le, ptr, 0);   // align
        wee64(le, ptr, 0);   // entsize
    }
    else
    {
        wee32(le, ptr, 0);   // align
        wee32(le, ptr, 0);   // entsize
    }
    
    wee32(le, ptr, stringOffset(".symtab"));  // name
    wee32(le, ptr, 2);  // type - symtab
    if (sf_bit)
    {
        wee64(le, ptr, 0);  // flags
        wee64(le, ptr, 0);  // addr
        wee64(le, ptr, 8192);  // offset
        wee64(le, ptr, (fptrs.size()+1)*24);  // size
    }
    else
    {
        wee32(le, ptr, 0);  // flags
        wee32(le, ptr, 0);  // addr
        wee32(le, ptr, 8192);  // offset
        wee32(le, ptr, checked_32((fptrs.size()+1)*16));  // size
    }
    wee32(le, ptr, 1);  // link - string table
    wee32(le, ptr, 0);  // info - last local symbol
    if (sf_bit)
    {
        wee64(le, ptr, 0);   // align
        wee64(le, ptr, 24);   // entsize
    }
    else
    {
        wee32(le, ptr, 0);   // align
        wee32(le, ptr, 16);   // entsize
    }

    for (int loopc=0; loopc<4; loopc++)
    {
        int name = 0;
        int type = 0;
        int flags = 0;
        if (loopc == IMAGE_CODE)
        {
            name = stringOffset(".text");
            flags = 0x6;
            type = 1;
        }
        else if (loopc == IMAGE_DATA)
        {
            name = stringOffset(".data");
            flags = 0x3;
            type = 1;
        }
        else if (loopc == IMAGE_CONST_DATA)
        {
            name = stringOffset(".rodata");
            flags = 0x2;
            type = 1;
        }
        else if (loopc == IMAGE_UNALLOCED_DATA)
        {
            name = stringOffset(".bss");
            flags = 0x3;
            type = 8;
        }

        wee32(le, ptr, name);  // name        
        wee32(le, ptr, type);  // type
        if (sf_bit)
        {
            wee64(le, ptr, flags);  // flags
            wee64(le, ptr, bases[loopc]);  // addr
            wee64(le, ptr, bases[loopc]-base_addr);  // offset
            wee64(le, ptr, sizes[loopc]);  // size
        }
        else
        {
            wee32(le, ptr, flags);  // flags
            wee32(le, ptr, checked_32(bases[loopc]));  // addr
            wee32(le, ptr, checked_32(bases[loopc]-base_addr));  // offset
            wee32(le, ptr, checked_32(sizes[loopc]));  // size
        }
        wee32(le, ptr, 0);  // link - UNDEF
        wee32(le, ptr, 0);  // info
        if (sf_bit)
        {
            wee64(le, ptr, 0);   // align
            wee64(le, ptr, 0);   // entsize
        }
        else
        {
            wee32(le, ptr, 0);   // align
            wee32(le, ptr, 0);   // entsize
        }
    }
    
    fwrite(header, 4096, 1, f);
    delete[] header;

    unsigned char * symtab = new unsigned char[4096];
    memset(symtab, 0, 4096);
    ptr = symtab;

    if (sf_bit)
    {
        ptr += 24;  // STN_UNDEF
    }
    else
    {
        ptr += 16;  // STN_UNDEF
    }
        
    for (unsigned int loopc=0; loopc<fptrs.size(); loopc++)
    {
        int idx = stringOffset(fptrs[loopc]->name().c_str());
        if (sf_bit)
        {
            wee32(le, ptr, idx);
            *ptr = 0x12;
            ptr++;
            *ptr = 0;
            ptr++;
            wee16(le, ptr, 3);
            wee64(le, ptr, foffsets[loopc]+bases[IMAGE_CODE]);
            wee64(le, ptr, fsizes[loopc]);
        }
        else
        {
            wee32(le, ptr, idx);
            wee32(le, ptr, checked_32(foffsets[loopc]+bases[IMAGE_CODE]));
            wee32(le, ptr, checked_32(fsizes[loopc]));
            *ptr = 0x12; // info - function, global
            ptr++;
            *ptr = 0;
            ptr++;
            wee16(le, ptr, 3);  // section - .text
        }
    }
    
    fseek(f, 8192, SEEK_SET);
    fwrite(symtab, 4096, 1, f);
    delete[] symtab;

    fseek(f, 4096, SEEK_SET);
    fwrite(stringtable.getData(), stringtable.dataSize(), 1, f);
    
    for (int loopc=0; loopc<4; loopc++)
    {
        if (loopc != IMAGE_UNALLOCED_DATA)
        {
            fseek(f, bases[loopc]-base_addr, SEEK_SET);
            fwrite(sections[loopc], sizes[loopc], 1, f);
        }
    }
    fclose(f);
}

void ElfImage::materialiseSection(int s)
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


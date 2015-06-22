#include "elf.h"
#include "platform.h"
#include <stdio.h>
#include <string.h>

ElfImage::ElfImage(const char * f, bool s, bool l, int a)
{
    base_addr = 0x400000;
    next_addr = base_addr + 4096;
    fname = f;
    sf_bit = s;
    le = l;
    arch = a;

    bases[3] = 0x800000;
    sizes[3] = 4096;
}

ElfImage::~ElfImage()
{
    for (int loopc=0; loopc<4; loopc++)
    {
        delete[] sections[loopc];
    }
}

void ElfImage::finalise()
{
    FILE * f = fopen(fname, "w+");
    if (!f)
    {
        printf("Can't open %s\n", fname);
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
    wee32(le, ptr, 0x1);

    int no_pheaders = 5;
    
    if (sf_bit)
    {
        wee64(le, ptr, functionAddress(root_function));
        wee64(le, ptr, 0x40);  // Program header offset
        wee64(le, ptr, 0x40+(56*no_pheaders));  // End of program headers; start of section headers
    }
    else
    {
        wee32(le, ptr, functionAddress(root_function) & 0xffffffff);
        wee64(le, ptr, 0x34);  // Program header offset
        wee64(le, ptr, 0x40+(32*no_pheaders));  // End of program headers; start of section headers
    }
    wee32(le, ptr, 0x0); // Flags
    wee16(le, ptr, sf_bit ? 64 : 52); // This header size
    wee16(le, ptr, sf_bit ? 56 : 32); // pheader size
    wee16(le, ptr, no_pheaders);
    wee16(le, ptr, sf_bit ? 64 : 40); // section header size
    wee16(le, ptr, 0);  // Number of sections
    wee16(le, ptr, 0);  // Section with strings

        // Program header for image header
    wee32(le, ptr, 0x1);
    wee32(le, ptr, 0x0);
    if (sf_bit)
    {
        wee64(le, ptr, base_addr);
        wee64(le, ptr, base_addr);
        wee64(le, ptr, 4096);
        wee64(le, ptr, 4096);
        wee64(le, ptr, 0);
        wee64(le, ptr, 0x4);
    }
    else
    {
        wee32(le, ptr, base_addr);
        wee32(le, ptr, base_addr);
        wee32(le, ptr, 4096);
        wee32(le, ptr, 4096);
        wee32(le, ptr, 0);
        wee32(le, ptr, 0x4);
    }

    for (int loopc=0; loopc<4; loopc++)
    {
        wee32(le, ptr, 0x1);  // Loadable

        int flags = 0;
        if (loopc == IMAGE_CODE)
        {
            flags = 0x5;
        }
        else if (loopc == IMAGE_DATA)
        {
            flags = 0x6;
        }
        else if (loopc == IMAGE_CONST_DATA)
        {
            flags = 0x4;
        }
        else if (loopc == IMAGE_UNALLOCED_DATA)
        {
            flags = 0x6;
        }
        
        wee32(le, ptr, flags);  // Flags
        if (sf_bit)
        {
            wee64(le, ptr, bases[loopc]-base_addr);
            wee64(le, ptr, bases[loopc]);
            wee64(le, ptr, bases[loopc]);
            wee64(le, ptr, (loopc == IMAGE_UNALLOCED_DATA) ? 0 : sizes[loopc]);
            wee64(le, ptr, sizes[loopc]);
            wee64(le, ptr, 4096);
        }
        else
        {
            wee32(le, ptr, bases[loopc]-base_addr);
            wee32(le, ptr, bases[loopc]);
            wee32(le, ptr, bases[loopc]);
            wee32(le, ptr, (loopc == IMAGE_UNALLOCED_DATA) ? 0 : sizes[loopc]);
            wee32(le, ptr, sizes[loopc]);
            wee32(le, ptr, 4096);
        }
    }

    fwrite(header, 4096, 1, f);
    delete[] header;
    
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
}


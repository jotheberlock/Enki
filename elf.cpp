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
    wee32(le, ptr, 0x7f454c46);
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

    int no_pheaders = 4;
    
    if (sf_bit)
    {
        wee64(le, ptr, functionAddress(root_function));
        wee64(le, ptr, 0x40);  // Program header offset
        wee64(le, ptr, 0x40+(56*no_pheaders));  // End of program headers; section headers
    }
    else
    {
        wee32(le, ptr, functionAddress(root_function) & 0xffffffff);
        wee64(le, ptr, 0x34);  // Program header offset
        wee64(le, ptr, 0x40+(32*no_pheaders));  // End of program headers; section headers
    }
    wee32(le, ptr, 0x0); // Flags
    wee16(le, ptr, sf_bit ? 64 : 52); // This header size
    wee16(le, ptr, sf_bit ? 56 : 32); // pheader size
    wee16(le, ptr, no_pheaders);
    wee16(le, ptr, sf_bit ? 64 : 40); // section header size
    wee16(le, ptr, no_pheaders);
    wee16(le, ptr, 0);  // String header
    wee16(le, ptr, 0);  // Section with strings
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
    next_addr &= ~4096;
}


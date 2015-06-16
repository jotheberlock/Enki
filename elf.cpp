#include "elf.h"
#include <stdio.h>

ElfImage::ElfImage(const char * f)
{
    base_addr = 0x400000;
    next_addr = base_addr + 4096;
    fname = f;
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

    for (int loopc=0; loopc<4; loopc++)
    {
        fseek(f, bases[loopc]-base_addr, SEEK_SET);
        fwrite(sections[loopc], sizes[loopc], 1, f);
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


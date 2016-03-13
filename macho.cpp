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


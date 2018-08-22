#include "inanna.h"
#include "platform.h"
#include "symbols.h"
#include "imports.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(POSIX_HOST)
#include <sys/stat.h>
#endif

#define INANNA_RELOC_64     1    // 64-bit value written in place
#define INANNA_RELOC_32     2
#define INANNA_RELOC_16     3
#define INANNA_RELOC_MASKED 4
#define INANNA_RELOC_END    5

class InannaSection
{
public:

    uint32 arch;
    uint32 type;
    uint32 offset;
    uint32 size;
    uint64 vmem;

};

InannaImage::InannaImage()
{
    arch = 0;
    base_addr = 0;
    next_addr = 0;
    fname = "a.enk";
}

InannaImage::~InannaImage()
{
}

void InannaImage::materialiseSection(int s)
{
    sections[s] = new unsigned char[sizes[s]];
    memset(sections[s], 0, sizes[s]);
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

    materialised[s] = true;
}

void InannaImage::finalise()
{
    if (bases[IMAGE_MTABLES] == 0)
    {
        materialiseSection(IMAGE_MTABLES);
    }

    if (bases[IMAGE_EXPORTS] == 0)
    {
        materialiseSection(IMAGE_EXPORTS);
    }

    FILE * f = fopen(fname.c_str(), "wb+");
    if (!f)
    {
        printf("Can't open %s\n", fname.c_str());
        return;
    }

    unsigned char * header = new unsigned char[4096];
    memset((char *)header, 0, 4096);
    strcpy((char *)header, "#!/usr/bin/env enkiloader\n");
    strcpy((char *)header + 512, "enki");
    unsigned char * ptr = header + 516;

    
    uint32 next_offset = 4096;
    next_offset += imports->size();
    while (next_offset % 4096)
    {
        next_offset++;
    }

    std::vector<InannaSection> sections;
    for (int loopc = 0; loopc < IMAGE_LAST; loopc++)
    {
        if (sizes[loopc])
        {
            InannaSection s;
            s.arch = 0;
            s.type = loopc;
            s.offset = next_offset;
            s.size = sizes[loopc];
            s.vmem = bases[loopc];
            next_offset += s.size;
            while (next_offset % 4096)
            {
                next_offset++;
            }
            sections.push_back(s);
        }
    }

    wle64(ptr, functionAddress(root_function)-bases[IMAGE_CODE]);
    wle32(ptr, sections.size());
    for (unsigned int loopc = 0; loopc < sections.size(); loopc++)
    {
        InannaSection & is = sections[loopc];
        printf(">>>> %x %x %x %x %lx\n", is.arch, is.type, is.offset, is.size, is.vmem);
        wle32(ptr, is.arch);
        wle32(ptr, is.type);
        wle32(ptr, is.offset);
        wle32(ptr, is.size);
        wle64(ptr, is.vmem);
    }

    for (unsigned int loopc=0; loopc<relocs.size(); loopc++)
    {
        BaseRelocation * br = relocs[loopc];
        if (br->isAbsolute())
        {
            uint64 v = br->getValue();
            unsigned char * p = br->getPtr();
            int secto,secfrom;
            uint64 offto, offfrom;
            if (!getSectionOffset(v, secto, offto))
            {
                continue;
            }
            if (!getSectionOffset(p, secfrom, offfrom))
            {
                continue;
            }
            printf("Section %d offset %llx points to section %d offset %llx\n",
                   secfrom, offfrom, secto, offto);
            std::list<Reloc> & relocs = br->relocs;
            for (std::list<Reloc>::iterator it = relocs.begin();
                 it != relocs.end(); it++)
            {
                printf("  Off %llx rshift %d mask %llx lshift %d bits %d\n",
                       (*it).offset, (*it).rshift, (*it).mask, (*it).lshift,
                       (*it).bits);
                if ((*it).bits == 64 && (*it).mask == 0)
                {
                    wle64(ptr, INANNA_RELOC_64);
                    wle64(ptr, secfrom);
                    wle64(ptr, offfrom);
                    wle64(ptr, secto);
                    wle64(ptr, offto);
                }
            }
            uint64 * up = (uint64 *)p;
            printf("Expected %llx is %llx\n", v, *up);
        }
    }
    wle64(ptr, INANNA_RELOC_END);
    
    fwrite(header, 4096, 1, f);
    
    fwrite(imports->getData(), imports->size(), 1, f);

    for (unsigned int loopc = 0; loopc < sections.size(); loopc++)
    {
        fseek(f, sections[loopc].offset, SEEK_SET);
        unsigned char * ptr = getPtr(sections[loopc].type);
        fwrite(ptr, sizes[sections[loopc].type], 1, f);
        printf("Writing %x bytes at %x\n", sizes[sections[loopc].type], sections[loopc].offset);
        if (sections[loopc].type == 2)
        {
            printf(">> %p %d %d %d\n", ptr, ptr[0], ptr[1], ptr[2]);
        }
    }

    fclose(f);
#if defined(POSIX_HOST)
    chmod(fname.c_str(), 0755);
#endif
}

bool InannaImage::configure(std::string param, std::string val)
{
    if (false)
    {

    }
    else
    {
        return Image::configure(param, val);
    }

    return true;
}

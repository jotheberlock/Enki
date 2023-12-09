#include "inanna.h"
#include "imports.h"
#include "inanna_structures.h"
#include "platform.h"
#include "symbols.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(POSIX_HOST)
#include <sys/stat.h>
#endif

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

int InannaImage::stringOffset(const char *c)
{
    int ret = 0;
    int tmpid = stringtable.getID(c);
    if (tmpid > 0)
    {
        ret = stringtable.offsetOf(tmpid);
    }
    return ret;
}

void InannaImage::finalise()
{
    stringtable.clear();
    stringtable.add(""); // Null byte at start
    stringtable.add(".text");
    stringtable.add(".rodata");
    stringtable.add(".data");
    stringtable.add(".bss");
    stringtable.add(".symtab");
    stringtable.add(".rtti");
    stringtable.add(".mtables");
    stringtable.add(".exports");

    if (bases[IMAGE_MTABLES] == 0)
    {
        materialiseSection(IMAGE_MTABLES);
    }

    if (bases[IMAGE_EXPORTS] == 0)
    {
        materialiseSection(IMAGE_EXPORTS);
    }

    int seccount = 0;
    for (int loopc = 0; loopc < IMAGE_LAST; loopc++)
    {
        if (sizes[loopc])
        {
            seccount++;
        }
    }

    FILE *f = fopen(fname.c_str(), "wb+");
    if (!f)
    {
        printf("Can't open %s\n", fname.c_str());
        return;
    }

    size_t stablesize = stringtable.dataSize();
    while (stablesize % 8)
    {
        stablesize++;
    }

    int no_subrelocs = 0;

    for (unsigned int loopc = 0; loopc < relocs.size(); loopc++)
    {
        BaseRelocation *br = relocs[loopc];
        if (br->isAbsolute())
        {
            no_subrelocs += (int)br->relocs.size();
        }
    }

    int headersize = (int)(INANNA_PREAMBLE + InannaHeader::size() + InannaArchHeader::size() + stablesize +
                           imports->size() + (seccount * InannaSection::size()) + (no_subrelocs * InannaReloc::size()));

    uint32_t next_offset = headersize;
    while (next_offset % 4096)
    {
        next_offset++;
    }

    std::vector<InannaSection> isections;
    for (int loopc = 0; loopc < IMAGE_LAST; loopc++)
    {
        if (sizes[loopc])
        {
            InannaSection s;
            s.type = loopc;
            s.offset = next_offset;
            s.length = (uint32_t)sizes[loopc];
            s.vmem = bases[loopc];

            if (loopc == IMAGE_CODE)
            {
                s.name = stringOffset(".text");
            }
            else if (loopc == IMAGE_DATA)
            {
                s.name = stringOffset(".data");
            }
            else if (loopc == IMAGE_CONST_DATA)
            {
                s.name = stringOffset(".rodata");
            }
            else if (loopc == IMAGE_UNALLOCED_DATA)
            {
                s.name = stringOffset(".bss");
            }
            else if (loopc == IMAGE_RTTI)
            {
                s.name = stringOffset(".rtti");
            }
            else if (loopc == IMAGE_MTABLES)
            {
                s.name = stringOffset(".mtables");
            }
            else if (loopc == IMAGE_EXPORTS)
            {
                s.name = stringOffset(".exports");
            }

            next_offset += s.length;
            while (next_offset % 4096)
            {
                next_offset++;
            }
            isections.push_back(s);
        }
    }

    unsigned char *header = new unsigned char[headersize];
    memset(header, 0, headersize);
    strcpy((char *)header, "#!/usr/bin/env enkiloader\n");

    unsigned char *ptr = header + INANNA_PREAMBLE;
    strcpy((char *)ptr, "enki");
    ptr += 4;
    wle32(ptr, 1);
    wle32(ptr, 1);
    wle32(ptr, INANNA_PREAMBLE + InannaHeader::size() + InannaArchHeader::size());
    wle32(ptr, stablesize);
    wle32(ptr, 0);

    size_t relocs_count = 0;

    for (unsigned int loopc = 0; loopc < relocs.size(); loopc++)
    {
        if (relocs[loopc]->isAbsolute())
        {
            relocs_count += relocs[loopc]->relocs.size();
        }
    }

    wle32(ptr, arch);
    wle32(ptr,
          (uint32_t)(INANNA_PREAMBLE + InannaHeader::size() + InannaArchHeader::size() + stablesize + imports->size()));
    wle32(ptr, (uint32_t)isections.size());
    wle32(ptr, (uint32_t)relocs_count);
    wle64(ptr, functionAddress(root_function) - bases[IMAGE_CODE]);
    wle32(ptr, (uint32_t)(INANNA_PREAMBLE + InannaHeader::size() + InannaArchHeader::size() + stablesize));
    wle32(ptr, imports->size());

    memcpy(ptr, stringtable.getData(), stringtable.dataSize());
    ptr += stablesize;

    memcpy(ptr, imports->getData(), imports->size());
    ptr += imports->size();

    for (unsigned int loopc = 0; loopc < isections.size(); loopc++)
    {
        InannaSection &is = isections[loopc];
        wle32(ptr, is.type);
        wle32(ptr, is.offset);
        wle32(ptr, is.length);
        wle32(ptr, is.name);
        wle64(ptr, is.vmem);
    }

    for (unsigned int loopc = 0; loopc < relocs.size(); loopc++)
    {
        BaseRelocation *br = relocs[loopc];
        if (br->isAbsolute())
        {
            uint64_t v = br->getValue();
            unsigned char *p = br->getPtr();
            int secto, secfrom;
            uint64_t offto, offfrom;
            if (!getSectionOffset(v, secto, offto))
            {
                continue;
            }
            if (!getSectionOffset(p, secfrom, offfrom))
            {
                continue;
            }
            printf("\nSection %d %s offset %lx points to section %d %s offset %lx\n", secfrom,
                   sectionName(secfrom).c_str(), offfrom, secto, sectionName(secto).c_str(), offto);
            std::list<Reloc> &relocs = br->relocs;
            for (std::list<Reloc>::iterator it = relocs.begin(); it != relocs.end(); it++)
            {
                printf("  Off %lx rshift %d mask %lx lshift %d bits %d\n", (*it).offset, (*it).rshift, (*it).mask,
                       (*it).lshift, (*it).bits);
                int type = INANNA_RELOC_INVALID;

                uint32_t rshift = 0;
                uint64_t mask = 0;
                uint32_t lshift = 0;
                uint32_t bits = 0;
                uint64_t offset = 0;

                if ((*it).bits == 64 && (*it).mask == 0)
                {
                    type = INANNA_RELOC_64;
                }
                else if ((*it).bits == 32 && (*it).mask == 0)
                {
                    type = INANNA_RELOC_32;
                }
                else if ((*it).bits == 16 && (*it).mask == 0)
                {
                    type = INANNA_RELOC_16;
                }
                else if ((*it).bits == 64)
                {
                    type = INANNA_RELOC_MASKED_64;
                    rshift = (*it).rshift;
                    mask = (*it).mask;
                    lshift = (*it).lshift;
                    bits = (*it).bits;
                    offset = (*it).offset;
                }
                else if ((*it).bits == 32)
                {
                    type = INANNA_RELOC_MASKED_32;
                    rshift = (*it).rshift;
                    mask = (*it).mask;
                    lshift = (*it).lshift;
                    bits = (*it).bits;
                    offset = (*it).offset;
                }
                else if ((*it).bits == 16)
                {
                    type = INANNA_RELOC_MASKED_16;
                    rshift = (*it).rshift;
                    mask = (*it).mask;
                    lshift = (*it).lshift;
                    bits = (*it).bits;
                    offset = (*it).offset;
                }
                else
                {
                    printf("Unknown relocation type! Bits %d mask %lx\n", (*it).bits, (*it).mask);
                }

                printf("  Writing t %x sf %x st %x rs %x m %lx ls %x b %x o %lx off %lx ot %lx\n", type, secfrom, secto,
                       rshift, mask, lshift, bits, offset, offfrom, offto);

                unsigned char *optr = sections[secfrom] + offset + offfrom;
                if ((*it).bits == 64)
                {
                    printf("  Currently %lx\n", *((uint64_t *)optr));
                }
                else if ((*it).bits == 32)
                {
                    printf("  Currently %x\n", *((uint32_t *)optr));
                }
                else
                {
                    printf("  Currently %x\n", *((uint16_t *)optr));
                }

                wle32(ptr, type);
                wle32(ptr, secfrom);
                wle32(ptr, secto);
                wle32(ptr, rshift);
                wle64(ptr, mask);
                wle32(ptr, lshift);
                wle32(ptr, bits);
                wle64(ptr, offset);
                wle64(ptr, offfrom);
                wle64(ptr, offto);
            }
        }
    }

    fwrite(header, headersize, 1, f);

    for (unsigned int loopc = 0; loopc < isections.size(); loopc++)
    {
        fseek(f, isections[loopc].offset, SEEK_SET);
        unsigned char *ptr = getPtr(isections[loopc].type);
        fwrite(ptr, sizes[isections[loopc].type], 1, f);
    }

    fclose(f);
#if defined(POSIX_HOST)
    chmod(fname.c_str(), 0755);
#endif
}

bool InannaImage::configure(std::string param, std::string val)
{
    return Image::configure(param, val);
}

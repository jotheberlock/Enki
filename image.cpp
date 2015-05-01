#include <stdio.h>
#include "image.h"
#include "symbols.h"
#include "asm.h"
#include "platform.h"

FunctionRelocation::FunctionRelocation(Image * i, FunctionScope * p, uint64_t po, FunctionScope * l, uint64_t lo)
{
    image = i;
    to_patch = p;
    patch_offset = po;
    to_link = l;
    link_offset = lo;
}

void FunctionRelocation::apply()
{
    uint64_t paddr = image->functionAddress(to_patch);
    paddr += patch_offset;
    paddr -= image->getAddr(IMAGE_CODE);
    unsigned char * patch_site = image->getPtr(IMAGE_CODE)+paddr;
    uint64_t laddr = image->functionAddress(to_link);
    laddr += link_offset;
    wee64(image->littleEndian(),patch_site,laddr);
}

BasicBlockRelocation::BasicBlockRelocation(Image * i, FunctionScope * p, uint64_t po, BasicBlock * l)
{
    image = i;
    to_patch = p;
    patch_offset = po;
    to_link = l;
}

void BasicBlockRelocation::apply()
{
    uint64_t baddr = to_link->getAddr();
    uint64_t oaddr = image->functionAddress(to_patch) + patch_offset;
            
    int32_t diff;
    if (oaddr > baddr)
    {
        diff = -((int32_t)(oaddr-baddr));
    }
    else
    {
        diff = (int32_t)(baddr-oaddr);
    }
    
    uint64_t paddr = image->functionAddress(to_patch);
    paddr += patch_offset;
    paddr -= image->getAddr(IMAGE_CODE);
    unsigned char * patch_site = image->getPtr(IMAGE_CODE)+paddr;
    wees32(image->littleEndian(), patch_site, diff);
}
    
Image::Image()
{
    for (int loopc=0; loopc<4; loopc++)
    {
        sections[loopc] = 0;
        bases[loopc] = 0;
        sizes[loopc] = 0;
    }
    current_offset = 0;
    align = 8;
}

void Image::addFunction(FunctionScope * ptr, uint64_t size)
{
    foffsets.push_back(current_offset);
    fptrs.push_back(ptr);
    
    current_offset += size;
    while (current_offset % align)
    {
        current_offset++;
    }
}

unsigned char * Image::functionPtr(FunctionScope * ptr)
{
    for (unsigned int loopc=0; loopc<fptrs.size(); loopc++)
    {
        if (fptrs[loopc] == ptr)
        {
            return foffsets[loopc] + getPtr(IMAGE_CODE);
        }
    }

    printf("Can't find function [%s]!\n", ptr->name().c_str());
    return 0;
}

uint64_t Image::functionAddress(FunctionScope * ptr)
{
    for (unsigned int loopc=0; loopc<fptrs.size(); loopc++)
    {
        if (fptrs[loopc] == ptr)
        {
            return foffsets[loopc] + getAddr(IMAGE_CODE);
        }
    }

    return INVALID_ADDRESS;
}

void Image::addImport(std::string lib, std::string name)
{
    import_names.push_back(name);
    import_libraries.push_back(lib);
 }

uint64_t MemoryImage::importAddress(std::string name)
{
    for (unsigned int loopc=0; loopc<import_names.size(); loopc++)
    {
        if (import_names[loopc] == name)
        {
            return  (uint64_t)(&import_pointers[loopc]);
        }
    }

    return INVALID_ADDRESS;
}

void Image::setSectionSize(int t, uint64_t l)
{
    sizes[t] = l;
}

uint64_t Image::sectionSize(int t)
{
    return sizes[t];
}

uint64_t Image::getAddr(int t)
{
    if (!bases[t])
    {
        materialiseSection(t);
    }
    
    return bases[t];
}

unsigned char * Image::getPtr(int t)
{
    if (!bases[t])
    {
        materialiseSection(t);
    }
    
    return sections[t];
}

MemoryImage::~MemoryImage()
{
    Mem mem;
    for (int loopc=0; loopc<4; loopc++)
    {
        mem.releaseBlock(mems[loopc]);
    }
}

void MemoryImage::materialiseSection(int s)
{
    Mem mem;
    mems[s] = mem.getBlock(sizes[s], MEM_READ | MEM_WRITE);
    bases[s] = (uint64_t)mems[s].ptr;
    sections[s] = mems[s].ptr;
}

void MemoryImage::setImport(std::string name, uint64_t addr)
{    
    for (unsigned int loopc=0; loopc<import_names.size(); loopc++)
    {
        if (import_names[loopc] == name)
        {
            import_pointers[loopc] = addr;
        }
    }

    printf("Couldn't set address for import %s!\n", name.c_str());
}

void MemoryImage::finalise()
{
    import_pointers = new uint64_t[import_names.size()];
    Mem mem;
    mem.changePerms(mems[IMAGE_CODE], MEM_READ | MEM_EXEC);
    mem.changePerms(mems[IMAGE_DATA], MEM_READ | MEM_WRITE);
    mem.changePerms(mems[IMAGE_CONST_DATA], MEM_READ);
    mem.changePerms(mems[IMAGE_UNALLOCED_DATA], MEM_READ | MEM_WRITE);
}


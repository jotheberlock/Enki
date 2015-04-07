#include <stdio.h>
#include "image.h"

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

void Image::addFunction(std::string name, uint64_t size)
{
    fnames.push_back(name);
    foffsets.push_back(current_offset);
    current_offset += size;
    while (current_offset % align)
    {
        current_offset++;
    }
}

unsigned char * Image::functionPtr(std::string name)
{
    for (unsigned int loopc=0; loopc<fnames.size(); loopc++)
    {
        if (fnames[loopc] == name)
        {
            return foffsets[loopc] + getPtr(IMAGE_CODE);
        }
    }

    printf("Can't find function [%s]!\n", name.c_str());
    return 0;
}

uint64_t Image::functionAddress(std::string name)
{
    for (unsigned int loopc=0; loopc<fnames.size(); loopc++)
    {
        if (fnames[loopc] == name)
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
    for (unsigned int loopc=0; loopc<fnames.size(); loopc++)
    {
        if (fnames[loopc] == name)
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
    for (unsigned int loopc=0; loopc<fnames.size(); loopc++)
    {
        if (fnames[loopc] == name)
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


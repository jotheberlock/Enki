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

void Image::setSectionSize(int t, uint64_t l)
{
    sizes[t] = l;
}

uint64_t Image::getAddr(int t)
{
    return bases[t];
}

unsigned char * Image::getPtr(int t)
{
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

void MemoryImage::materialise()
{
    Mem mem;
    for (int loopc=0; loopc<4; loopc++)
    {
        mems[loopc] = mem.getBlock(sizes[loopc], MEM_READ | MEM_WRITE);
        bases[loopc] = (uint64_t)mems[loopc].ptr;
        sections[loopc] = mems[loopc].ptr;
    }
}

void MemoryImage::finalise()
{
    Mem mem;
    mem.changePerms(mems[IMAGE_CODE], MEM_READ | MEM_EXEC);
    mem.changePerms(mems[IMAGE_DATA], MEM_READ | MEM_WRITE);
    mem.changePerms(mems[IMAGE_CONST_DATA], MEM_READ);
    mem.changePerms(mems[IMAGE_UNALLOCED_DATA], MEM_READ | MEM_WRITE);
}

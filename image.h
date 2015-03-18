#ifndef _IMAGE_
#define _IMAGE_

#include <stdint.h>
#include <string>
#include <vector>
#include "mem.h"

#define IMAGE_CODE 0
#define IMAGE_DATA 1
#define IMAGE_CONST_DATA 2
#define IMAGE_UNALLOCED_DATA 3

#define INVALID_ADDRESS 0xdeadbeefdeadbeef

class Image
{
  public:

    Image();
    virtual ~Image() {}

    void setSectionSize(int, uint64_t);
    uint64_t getAddr(int);
    unsigned char * getPtr(int);
        // Make memory accessible
    virtual void materialise() = 0;
        // Fix up perms
    virtual void finalise() = 0;
    void addFunction(std::string, uint64_t);
    uint64_t functionAddress(std::string);
    
  protected:

    unsigned char * sections[4];
    uint64_t bases[4];
    uint64_t sizes[4];
    std::vector<std::string> fnames;
    std::vector<uint64_t> foffsets;
    uint64_t current_offset;
    uint64_t align;
    
};

class MemoryImage : public Image
{
  public:

    ~MemoryImage();
    void materialise();
    void finalise();
    
  protected:

    MemBlock mems[4];
    
};

#endif

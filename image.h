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
        // Fix up perms
    virtual void finalise() = 0;
    void addFunction(std::string, uint64_t);
    uint64_t functionAddress(std::string);
    void addImport(std::string);
    virtual uint64_t importAddress(std::string) = 0;
    virtual void materialiseSection(int) = 0;
    
  protected:

    unsigned char * sections[4];
    uint64_t bases[4];
    uint64_t sizes[4];
    std::vector<std::string> fnames;
    std::vector<uint64_t> foffsets;
    std::vector<std::string> import_names;
    
    uint64_t current_offset;
    uint64_t align;
    
};

class MemoryImage : public Image
{
  public:

    MemoryImage()
    {
        import_pointers=0;
    }
    
    ~MemoryImage();
    void finalise();
    void setImport(std::string, uint64_t);
    uint64_t importAddress(std::string);

        // TODO remove
    MemBlock & getMemBlock(int i)
    {
        return mems[i];
    }
    
  protected:

    void materialiseSection(int s);
    MemBlock mems[4];
    uint64_t * import_pointers;
    
};

#endif

#ifndef _ELF_
#define _ELF_

#include "image.h"
#include "stringbox.h"

class ElfImage : public Image
{
  public:

    ElfImage();
    ElfImage(const char *, bool, bool, int);
    ~ElfImage();
    void finalise();
    bool configure(std::string, std::string);
    
    virtual uint64 importAddress(std::string)
    {
        return 0;
    }
    
    virtual uint64 importOffset(std::string)
    {
        return 0;
    }

    std::string name() { return "elf"; }
    
  protected:
    
    int stringOffset(const char * c);
    bool le;
    StringBox stringtable;
    
};

#endif

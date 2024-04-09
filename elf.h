#ifndef _ELF_
#define _ELF_

/*
    Generates ELF executables; can handle 32 and 64 bit
    little and big endian.
*/

#include "image.h"
#include "stringbox.h"

class ElfImage : public Image
{
  public:
    ElfImage();
    ~ElfImage();
    void finalise();
    bool configure(std::string, std::string) override;

    virtual uint64_t importAddress(std::string)
    {
        return 0;
    }

    virtual uint64_t importOffset(std::string)
    {
        return 0;
    }

    std::string name()
    {
        return "elf";
    }

  protected:
    int stringOffset(const char *c);
    bool le;
    StringBox stringtable;
};

#endif

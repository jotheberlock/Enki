#ifndef _MACHO_
#define _MACHO_

#include "image.h"
#include "stringbox.h"

class MachOImage : public Image
{
  public:

    MachOImage();
    ~MachOImage();
    void finalise();
    bool configure(std::string, std::string);
    std::string name() { return "macho"; }

    virtual uint64_t importAddress(std::string)
    {
        return 0;
    }
    
    virtual uint64_t importOffset(std::string)
    {
        return 0;
    }

  protected:
    
    void materialiseSection(int s);
    bool le;
    int arch_subtype;
    
};

#endif

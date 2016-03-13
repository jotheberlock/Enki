#ifndef _PE_
#define _PE_

#include "image.h"

class PEImage : public Image
{
 public:

    PEImage();
    ~PEImage();
    std::string name() { return "pe"; }
    bool configure(std::string, std::string);
    void finalise();
    
    virtual uint64_t importAddress(std::string);
    virtual uint64_t importOffset(std::string)
    {
        return 0;
    }

    virtual void endOfImports();
    
 protected:

    void materialiseSection(int s);
    int subsystem;
    uint64_t imports_base;
    uint64_t symbols_base;
    int os_major;
    int os_minor;
    
};

#endif

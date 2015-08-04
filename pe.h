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
    uint64_t base_addr;
    uint64_t next_addr;
    std::string fname;
    bool sf_bit;
    int arch;
    
};

#endif

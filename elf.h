#ifndef _ELF_
#define _ELF_

#include "image.h"

class ElfImage : public Image
{
  public:

    ElfImage(const char *, bool, bool, int);
    ~ElfImage();
    void finalise();

  protected:
    
    void materialiseSection(int s);
    uint64_t base_addr;
    uint64_t next_addr;
    const char * fname;
    bool sf_bit;
    bool le;
    int arch;
    
};

#endif

#ifndef _ELF_
#define _ELF_

#include "image.h"

class ElfImage : public Image
{
  public:

    ElfImage(const char *);
    ~ElfImage();
    void finalise();

  protected:
    
    void materialiseSection(int s);
    uint64_t base_addr;
    uint64_t next_addr;
    const char * fname;
    
};

#endif

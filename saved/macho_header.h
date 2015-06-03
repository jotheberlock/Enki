#ifndef _MACHO_HEADER_
#define _MACHO_HEADER_

class MachOHeader
{
  public:

    uint32_t magic;    // FEEDFACE (!) or FACF for 64 bit, CAFEBABE fat
    uint32_t cputype;  // 1000000 | 7 for amd64, 12 for arm,  6 for 68k
    uint32_t cpusubtype;
    uint32_t filetype;
    uint32_t ncmds;
    uint32_t sizeofcmds;
    uint32_t flags;
    uint32_t reserved;   // 64 bit only
};


#endif

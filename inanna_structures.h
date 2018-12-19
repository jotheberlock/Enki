#ifndef INANNA_STRUCTURES
#define INANNA_STRUCTURES

#include "platform.h"

#define INANNA_RELOC_64     1    // 64-bit value written in place
#define INANNA_RELOC_32     2
#define INANNA_RELOC_16     3
#define INANNA_RELOC_MASKED 4
#define INANNA_RELOC_END    5

/*
   File format is:
   512 bytes of anything
   Inanna header
   <archs_count> arch headers
   arch1 section headers
   arch1 relocs
   arch2 section headers
   arch2 relocs
   arch1 sections
   arch2 sections
*/

class InannaHeader
{
public:

    char magic[4];
    uint32 version;
    uint64 start_address;
    uint32 archs_count;
    uint32 strings_offset;
    
};

class InannaArchHeader
{
  public:

    uint32 arch;
    uint32 offset;
    uint32 sec_count;
    uint32 reloc_count;
    
};

class InannaSection
{
public:

    uint32 arch;
    uint32 type;
    uint32 offset;
    uint32 size;
    uint32 relocs;   // offset from start of file
    uint32 name;
    uint64 vmem;

};

class InannaReloc
{
public:

    uint32 type;
    uint32 secfrom;
    uint32 secto;
    uint32 rshift;
    uint64 mask;
    uint32 lshift;
    uint32 bits;
    uint64 offset;
    uint64 offrom;
    uint64 offto;
    
};

#endif

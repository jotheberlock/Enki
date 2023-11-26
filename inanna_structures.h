#ifndef INANNA_STRUCTURES
#define INANNA_STRUCTURES

#include "platform.h"

#define INANNA_RELOC_INVALID   0
#define INANNA_RELOC_64        1    // 64-bit value written in place
#define INANNA_RELOC_32        2
#define INANNA_RELOC_16        3
#define INANNA_RELOC_MASKED_64 4
#define INANNA_RELOC_MASKED_32 5
#define INANNA_RELOC_MASKED_16 6
#define INANNA_RELOC_END       7

/*
   File format is:
   512 bytes of anything
   Inanna header
   <archs_count> arch headers
   String table
   <align to 64 bits>
   Imports
   arch1 section headers
   arch1 relocs
   arch2 section headers
   arch2 relocs
   <align to 4k>
   arch1 sections
   arch2 sections
*/

#define INANNA_PREAMBLE 512

class InannaHeader
{
public:

    char magic[4];
    uint32_t version;
    uint32_t archs_count;
    uint32_t strings_offset;
    uint32_t strings_size;
    uint32_t dummy;
    
    static int size() { return 24; }
    
};

class InannaArchHeader
{
  public:

    uint32_t arch;
    uint32_t offset;
    uint32_t sec_count;
    uint32_t reloc_count;
    uint64_t start_address;
    uint32_t imports_offset;
    uint32_t imports_size;

    static int size() { return 32; }
    
};

class InannaSection
{
public:

    uint32_t type;
    uint32_t offset;
    uint32_t length;
    uint32_t name;
    uint64_t vmem;

    static int size() { return 24; }
    
};

class InannaReloc
{
public:

    uint32_t type;
    uint32_t secfrom;
    uint32_t secto;
    uint32_t rshift;
    uint64_t mask;
    uint32_t lshift;
    uint32_t bits;
    uint64_t offset;
    uint64_t offrom;
    uint64_t offto;

    static int size() { return 56; }
    
};

#endif

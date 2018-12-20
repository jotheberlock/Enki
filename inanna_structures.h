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
   String table
   Imports
   arch1 section headers
   arch1 relocs
   arch2 section headers
   arch2 relocs
   arch1 sections
   arch2 sections
*/

#define INANNA_PREAMBLE 512

class InannaHeader
{
public:

    char magic[4];
    uint32 version;
    uint64 start_address;
    uint32 archs_count;
    uint32 strings_offset;
    uint32 imports_offset;
    uint32 dummy;
    
    static int size() { return 32; }
    
};

class InannaArchHeader
{
  public:

    uint32 arch;
    uint32 offset;
    uint32 sec_count;
    uint32 reloc_count;

    static int size() { return 16; }
    
};

class InannaSection
{
public:

    uint32 type;
    uint32 offset;
    uint32 length;
    uint32 name;
    uint64 vmem;

    static int size() { return 24; }
    
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

    static int size() { return 56; }
    
};

#endif

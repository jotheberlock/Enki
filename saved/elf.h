#ifndef _ELF_
#define _ELF_

#include "image.h"
#include "stringbox.h"

class Elf : public Image {
	
public:
	
    virtual const char * componentName() { return "elf"; }
    virtual const char * componentDescription() { return "ELF"; }
	
    virtual void allocateBases();
    virtual void write();
	
protected:
	
    void dump(char *);
	
    int stringOffset(char *);
	
    StringBox stringtable;
	
    BiggestInt totaldisksize;
    BiggestInt totalmemsize;
    int elf_header_size;
	
};

#endif

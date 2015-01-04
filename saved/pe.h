#ifndef _PE_
#define _PE_

#include "driver.h"
#include "stringbox.h"


class HintBox {
	
public:
	
    HintBox();
    ~HintBox();
	
    char * getText(int);
    int getID(const char *);
    int add(unsigned short, const char *);
    int getInternalID();
    void dump();
    void clear();
	
    int dataSize() { return datap; }
    char * getData() { return data; }
    int offsetOf(int i) {  return ids[i-1]; }
	
protected:
    
    int idp;
    int internal_idp;
    int datap;
    int * ids;
    char * data;
	
};

class PE : public Image {
	
public:
	
    virtual const char * componentName() { return "pe"; }
    virtual const char * componentDescription() { return "PE"; }
	
    virtual void allocateBases();
    virtual void write();
	
protected:
	
    void buildImports();
    void emitImport(LibraryImport * lib, unsigned char * ptr, int & offset, int & ilt_offset, int & iat_offset, 
		HintBox & names, int stringtable_offset, StringBox * global_names);
	
    StringBox stringtable;
	
    BiggestInt totaldisksize;
    BiggestInt totalmemsize;
    int pe_header_size;
	
};

#endif

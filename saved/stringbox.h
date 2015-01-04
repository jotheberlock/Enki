#ifndef _STRINGBOX_
#define _STRINGBOX_

class StringBox {
	
public:
	
    StringBox();
    ~StringBox();
	
    char * getText(int);
    int getID(const char *);
    int add(const char *);
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

#endif

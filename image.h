#ifndef _IMAGE_
#define _IMAGE_

#include <stdint.h>
#include <string>
#include <vector>
#include "mem.h"

#define IMAGE_CODE 0
#define IMAGE_DATA 1
#define IMAGE_CONST_DATA 2
#define IMAGE_UNALLOCED_DATA 3

#define INVALID_ADDRESS 0xdeadbeefdeadbeef

class FunctionScope;
class BasicBlock;
class BaseRelocation;

class Image
{
  public:

    Image();
    virtual ~Image() {}

    void setSectionSize(int, uint64_t);
    uint64_t sectionSize(int);
    
    uint64_t getAddr(int);
    unsigned char * getPtr(int);
        // Fix up perms
    virtual void finalise() = 0;
	void relocate();
    void addFunction(FunctionScope *, uint64_t);
    uint64_t functionAddress(FunctionScope *);
    unsigned char * functionPtr(FunctionScope *);
    
    void addImport(std::string, std::string);
    virtual uint64_t importAddress(std::string) = 0;
    virtual void materialiseSection(int) = 0;
    bool littleEndian()
    {
        return true;
    }
    
	void addReloc(BaseRelocation * b)
	{
		relocs.push_back(b);
	}

  protected:

    unsigned char * sections[4];
    uint64_t bases[4];
    uint64_t sizes[4];

    std::vector<uint64_t> foffsets;
    std::vector<FunctionScope *> fptrs;
    
    std::vector<std::string> import_names;
    std::vector<std::string> import_libraries;
    
    uint64_t current_offset;
    uint64_t align;
	
    std::vector<BaseRelocation *> relocs;

    
};

class MemoryImage : public Image
{
  public:

    MemoryImage()
    {
        import_pointers=0;
    }
    
    ~MemoryImage();
    void finalise();
    void setImport(std::string, uint64_t);
    uint64_t importAddress(std::string);

        // TODO remove
    MemBlock & getMemBlock(int i)
    {
        return mems[i];
    }
    
  protected:

    void materialiseSection(int s);
    MemBlock mems[4];
    uint64_t * import_pointers;

};

class BaseRelocation
{
  public:

	BaseRelocation(Image * i)
	{
		i->addReloc(this);
	}

    virtual void apply() = 0;

  protected:

    Image * image;
    
};

class FunctionRelocation : public BaseRelocation
{
  public:

    FunctionRelocation(Image *,
                       FunctionScope *, uint64_t, FunctionScope *, uint64_t);
    void apply();
    
  protected:
    
    FunctionScope * to_patch;
    FunctionScope * to_link;
    uint64_t patch_offset;
    uint64_t link_offset;
    
};


class BasicBlockRelocation : public BaseRelocation
{
  public:

    BasicBlockRelocation(Image *,
                         FunctionScope *, uint64_t, uint64_t, BasicBlock *);

	
    BasicBlockRelocation(Image *,
                         FunctionScope *, uint64_t, BasicBlock *);
    void apply();
    
  protected:
    
    FunctionScope * to_patch;
    BasicBlock * to_link;
    uint64_t patch_offset;
    uint64_t patch_relative;
	bool absolute;

};


#endif

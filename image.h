#ifndef _IMAGE_
#define _IMAGE_

#include <string>
#include <vector>
#include "mem.h"
#include "component.h"

#define IMAGE_CODE 0
#define IMAGE_DATA 1
#define IMAGE_CONST_DATA 2
#define IMAGE_UNALLOCED_DATA 3
#define IMAGE_LAST 4

#define INVALID_ADDRESS 0xdeadbeefdeadbeef

#define ARCH_AMD64 1

class FunctionScope;
class BasicBlock;
class BaseRelocation;

class LibImport
{
public:

	std::string name;
	std::vector<std::string> imports;
};

class Image : public Component
{
  public:

    Image();
    virtual ~Image() {}

    virtual bool configure(std::string, std::string);
    void setSectionSize(int, uint64);
    uint64 sectionSize(int);
    
    uint64 getAddr(int);
    unsigned char * getPtr(int);
        // Fix up perms
    virtual void finalise() = 0;
    void relocate();
    void addFunction(FunctionScope *, uint64);
    uint64 functionAddress(FunctionScope *);
    uint64 functionSize(FunctionScope *);
    unsigned char * functionPtr(FunctionScope *);
    void setRootFunction(FunctionScope *);
    
    void addImport(std::string, std::string);
    virtual uint64 importAddress(std::string) = 0;

    virtual void materialiseSection(int) = 0;
    bool littleEndian()
    {
        return true;
    }
    
    void addReloc(BaseRelocation * b)
    {
        relocs.push_back(b);
    }

    virtual void endOfImports()
    {}

    virtual std::string name() = 0;

  protected:

    unsigned char * sections[4];
    uint64 bases[4];
    uint64 sizes[4];

    std::vector<uint64> foffsets;
    std::vector<uint64> fsizes;
    std::vector<FunctionScope *> fptrs;
    
	std::vector<LibImport> imports;
	uint64 total_imports;
    
    uint64 current_offset;
    uint64 align;
	
    std::vector<BaseRelocation *> relocs;
    FunctionScope * root_function;

    int arch;
    bool guard_page;
    uint64 base_addr;
    uint64 next_addr;
    std::string fname;
    bool sf_bit;
    
};

class MemoryImage : public Image
{
  public:

    MemoryImage();
    ~MemoryImage();
    void finalise();
    void setImport(std::string, uint64);
    uint64 importAddress(std::string);
    uint64 importOffset(std::string);
    void endOfImports();

    std::string name() { return "memory"; }
    
  protected:

    void materialiseSection(int s);
    MemBlock mems[4];
	std::vector<std::string> import_names;
    uint64 * import_pointers;

};

class BaseRelocation
{
  public:

    BaseRelocation(Image * i)
    {
      image=i;
      i->addReloc(this);
    }

    virtual ~BaseRelocation()
    {
    }
    
    virtual void apply() = 0;

  protected:

    Image * image;
    
};

class FunctionRelocation : public BaseRelocation
{
  public:

    FunctionRelocation(Image *,
                       FunctionScope *, uint64, FunctionScope *, uint64);
    void apply();
    
  protected:
    
    FunctionScope * to_patch;
    FunctionScope * to_link;
    uint64 patch_offset;
    uint64 link_offset;
    
};

class BasicBlockRelocation : public BaseRelocation
{
  public:

    BasicBlockRelocation(Image *,
                         FunctionScope *, uint64, uint64, BasicBlock *);

	
    BasicBlockRelocation(Image *,
                         FunctionScope *, uint64, BasicBlock *);
    void apply();
    
  protected:
    
    FunctionScope * to_patch;
    BasicBlock * to_link;
    uint64 patch_offset;
    uint64 patch_relative;
	bool absolute;

};

class SectionRelocation : public BaseRelocation
{
  public:

    SectionRelocation(Image *, int, uint64, int, uint64);
    void apply();

  protected:

    int patch_section;
    uint64 patch_offset;
    int dest_section;
    uint64 dest_offset;
    
};

class ExtFunctionRelocation : public BaseRelocation
{
   public:
 
    ExtFunctionRelocation(Image *, FunctionScope *, uint64, std::string);
    void apply();
    
   protected:

    FunctionScope * to_patch;
    uint64 patch_offset;
    std::string fname;
  
};

#endif

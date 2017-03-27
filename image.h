#ifndef _IMAGE_
#define _IMAGE_

/*
	Base class for generating executable images (e.g. ELF and PE-COFF)
	binaries. Also has the infrastructure for relocations, both relative
	and absolute, which are applied once the image has been generated and
	fix up e.g. branch displacements and Windows library imports.
*/

#include <string>
#include <vector>
#include <list>
#include "mem.h"
#include "component.h"

#define IMAGE_CODE 0
#define IMAGE_DATA 1
#define IMAGE_CONST_DATA 2
#define IMAGE_UNALLOCED_DATA 3
#define IMAGE_RTTI 4
#define IMAGE_MTABLES 5    // generic method tables
#define IMAGE_LAST 6

#define INVALID_ADDRESS 0xdeadbeefdeadbeefLL

#define ARCH_AMD64 1
#define ARCH_ARM32 2

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
	virtual ~Image();

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

	virtual void materialiseSection(int);
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

	unsigned char * sections[IMAGE_LAST];
	uint64 bases[IMAGE_LAST];
	uint64 sizes[IMAGE_LAST];

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
	MemBlock mems[IMAGE_LAST];
	std::vector<std::string> import_names;
	uint64 * import_pointers;

};

class Reloc
{
public:

	uint64 offset;  // from base pointer
		// Extract bits to write - shift right then mask
	uint64 rshift;
	uint64 mask;
	// How many bits to shift left into relocation
	uint64 lshift;
	bool sf;

	void apply(bool, unsigned char *, uint64);

};

class BaseRelocation
{
public:

	BaseRelocation(Image * i)
	{
		image = i;
		i->addReloc(this);
	}

	virtual ~BaseRelocation()
	{
	}

	void addReloc(uint64 o, uint64 r, uint64 m,
		uint64 l, bool sf)
	{
		Reloc reloc;
		reloc.offset = o;
		reloc.rshift = r;
		reloc.mask = m;
		reloc.lshift = l;
		reloc.sf = sf;
		relocs.push_back(reloc);
	}

	// Simple 'just write it here no masking' helpers
	void add32()
	{
		addReloc(0, 0, 0, 0, false);
	}

	void add64()
	{
		addReloc(0, 0, 0, 0, true);
	}

	void apply();
	virtual uint64 getValue() = 0;
	virtual unsigned char * getPtr() = 0;

	std::list<Reloc> relocs;

protected:

	Image * image;

};

class FunctionRelocation : public BaseRelocation
{
public:

	FunctionRelocation(Image *,
		FunctionScope *, uint64, FunctionScope *, uint64);
	uint64 getValue();
	unsigned char * getPtr();

protected:

	FunctionScope * to_patch;
	FunctionScope * to_link;
	uint64 patch_offset;
	uint64 link_offset;

};

class MtableRelocation : public BaseRelocation
{
  public:

    MtableRelocation(Image *, FunctionScope *, uint64);
    uint64 getValue();
    unsigned char * getPtr();
    
  protected:

    uint64 patch_offset;
    FunctionScope * to_link;
    
};

class BasicBlockRelocation : public BaseRelocation
{
public:

	BasicBlockRelocation(Image *,
		FunctionScope *, uint64, uint64, BasicBlock *);

	uint64 getValue();
	unsigned char * getPtr();

protected:

	FunctionScope * to_patch;
	BasicBlock * to_link;
	uint64 patch_offset;
	uint64 patch_relative;

};


class AbsoluteBasicBlockRelocation : public BaseRelocation
{
public:

	AbsoluteBasicBlockRelocation(Image *,
		FunctionScope *, uint64, BasicBlock *);
	uint64 getValue();
	unsigned char * getPtr();

protected:

	FunctionScope * to_patch;
	BasicBlock * to_link;
	uint64 patch_offset;
	uint64 patch_relative;

};


class SectionRelocation : public BaseRelocation
{
public:

	SectionRelocation(Image *, int, uint64, int, uint64);
	uint64 getValue();
	unsigned char * getPtr();

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
	uint64 getValue();
	unsigned char * getPtr();

protected:

	FunctionScope * to_patch;
	uint64 patch_offset;
	std::string fname;

};

#endif

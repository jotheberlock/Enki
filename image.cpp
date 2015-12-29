#include <stdio.h>
#include "image.h"
#include "symbols.h"
#include "asm.h"
#include "platform.h"

FunctionRelocation::FunctionRelocation(Image * i, FunctionScope * p, uint64_t po, FunctionScope * l, uint64_t lo)
	: BaseRelocation(i)
{
    image = i;
    to_patch = p;
    patch_offset = po;
    to_link = l;
    link_offset = lo;
}

void FunctionRelocation::apply()
{
    unsigned char * patch_site = image->functionPtr(to_patch)+patch_offset;
    uint64_t laddr = image->functionAddress(to_link);
    laddr += link_offset;
    wee64(image->littleEndian(),patch_site,laddr);
}

BasicBlockRelocation::BasicBlockRelocation(Image * i, FunctionScope * p, uint64_t po, uint64_t pr, BasicBlock * l)
	: BaseRelocation(i)
{
    image = i;
    to_patch = p;
    patch_offset = po;
	patch_relative = pr;
	to_link = l;
	absolute = false;
}

BasicBlockRelocation::BasicBlockRelocation(Image * i, FunctionScope * p, uint64_t po, BasicBlock * l)
	: BaseRelocation(i)
{
    image = i;
    to_patch = p;
    patch_offset = po;
	patch_relative = po;
	to_link = l;
	absolute = true;
}

void BasicBlockRelocation::apply()
{
    unsigned char * patch_site = image->functionPtr(to_patch)+patch_offset;
	if (absolute)
	{
		wees64(image->littleEndian(), patch_site, to_link->getAddr());
		return;
	}

    uint64_t baddr = to_link->getAddr();
    uint64_t oaddr = image->functionAddress(to_patch) + patch_relative;
    
    int32_t diff;
    if (oaddr > baddr)
    {
        diff = -((int32_t)(oaddr-baddr));
    }
    else
    {
        diff = (int32_t)(baddr-oaddr);
    }
    
    uint64_t paddr = image->functionAddress(to_patch);
    paddr += patch_offset;
    paddr -= image->getAddr(IMAGE_CODE);

    wees32(image->littleEndian(), patch_site, diff);
}

SectionRelocation::SectionRelocation(Image * i,
                                     int p, uint64_t po,
                                     int d, uint64_t dof)
    : BaseRelocation(i)
{
    patch_section = p;
    patch_offset = po;
    dest_section = d;
    dest_offset = dof;
}

void SectionRelocation::apply()
{
    unsigned char * patch_site = image->getPtr(patch_section)+patch_offset;
    uint64_t addr = image->getAddr(dest_section)+dest_offset;
    wee64(image->littleEndian(), patch_site, addr);
}

ExtFunctionRelocation::ExtFunctionRelocation(Image * i,
					     FunctionScope * f, uint64_t o,
					     std::string n)
  : BaseRelocation(i)
{
    to_patch = f;
    patch_offset = o;
    fname = n;
}

void ExtFunctionRelocation::apply()
{
    unsigned char * patch_site = image->getPtr(IMAGE_CODE)+patch_offset;
    uint64_t addr = image->importAddress(fname);
    wee64(image->littleEndian(), patch_site, addr);
}

Image::Image()
{
    for (int loopc=0; loopc<4; loopc++)
    {
        sections[loopc] = 0;
        bases[loopc] = 0;
        sizes[loopc] = 0;
    }
    current_offset = 0;
    align = 8;
    root_function = 0;
    total_imports = 0;
}

void Image::setRootFunction(FunctionScope * f)
{
    root_function = f;
}

void Image::relocate()
{
	for (unsigned int loopc=0; loopc<relocs.size(); loopc++)
	{
		relocs[loopc]->apply();
		delete relocs[loopc];
	}
	relocs.clear();
}

void Image::addFunction(FunctionScope * ptr, uint64_t size)
{
    foffsets.push_back(current_offset);
    fptrs.push_back(ptr);
    fsizes.push_back(size);
    
    current_offset += size;
    while (current_offset % align)
    {
        current_offset++;
    }
}

unsigned char * Image::functionPtr(FunctionScope * ptr)
{
    for (unsigned int loopc=0; loopc<fptrs.size(); loopc++)
    {
        if (fptrs[loopc] == ptr)
        {
            return foffsets[loopc] + getPtr(IMAGE_CODE);
        }
    }
    
    printf("functionPtr - can't find function [%s]!\n", ptr ? ptr->name().c_str() : "<null>");
    return 0;
}

uint64_t Image::functionAddress(FunctionScope * ptr)
{
    for (unsigned int loopc=0; loopc<fptrs.size(); loopc++)
    {
        if (fptrs[loopc] == ptr)
        {
            return foffsets[loopc] + getAddr(IMAGE_CODE);
        }
    }
	
    printf("functionAddress - can't find function [%s]!\n", ptr ? ptr->name().c_str() : "<null>");
    return INVALID_ADDRESS;
}

uint64_t Image::functionSize(FunctionScope * ptr)
{
    for (unsigned int loopc=0; loopc<fptrs.size(); loopc++)
    {
        if (fptrs[loopc] == ptr)
        {
            return fsizes[loopc];
        }
    }
	
    printf("functionSize - can't find function [%s]!\n", ptr ? ptr->name().c_str() : "<null>");
    return 0;
}

void Image::addImport(std::string lib, std::string name)
{
	for (unsigned int loopc=0; loopc<imports.size(); loopc++)
	{
		if (imports[loopc].name == lib)
		{
			LibImport & l = imports[loopc];
			for (unsigned int loopc2=0; loopc2<l.imports.size(); loopc2++)
			{
				if (l.imports[loopc2] == name)
				{
					return;
				}
			}
			l.imports.push_back(name);
			total_imports++;
			return;
		}
	}

	LibImport l;
	l.name = lib;
	l.imports.push_back(name);
	imports.push_back(l);
	total_imports++;
 }

uint64_t MemoryImage::importAddress(std::string name)
{
    for (unsigned int loopc=0; loopc<import_names.size(); loopc++)
    {
        if (import_names[loopc] == name)
        {
            return  (uint64_t)(&import_pointers[loopc]);
        }
    }

    return INVALID_ADDRESS;
}

void Image::setSectionSize(int t, uint64_t l)
{
    sizes[t] = l;
}

uint64_t Image::sectionSize(int t)
{
    return sizes[t];
}

uint64_t Image::getAddr(int t)
{
    if (!bases[t])
    {
        materialiseSection(t);
    }
    
    return bases[t];
}

unsigned char * Image::getPtr(int t)
{
    if (!bases[t])
    {
        materialiseSection(t);
    }
    
    return sections[t];
}

MemoryImage::MemoryImage()
{
    import_pointers=0;
}
    
MemoryImage::~MemoryImage()
{
    Mem mem;
    for (int loopc=0; loopc<4; loopc++)
    {
        mem.releaseBlock(mems[loopc]);
    }
}

void MemoryImage::materialiseSection(int s)
{
    Mem mem;
    mems[s] = mem.getBlock(sizes[s], MEM_READ | MEM_WRITE);
    bases[s] = (uint64_t)mems[s].ptr;
    sections[s] = mems[s].ptr;
}

void MemoryImage::setImport(std::string name, uint64_t addr)
{    
    for (unsigned int loopc=0; loopc<import_names.size(); loopc++)
    {
        if (import_names[loopc] == name)
        {
            import_pointers[loopc] = addr;
			return;
        }
    }

    printf("Couldn't set address for import [%s]!\n", name.c_str());
}

void MemoryImage::endOfImports()
{
    for (unsigned int loopc=0; loopc<imports.size(); loopc++)
    {
        LibImport & l = imports[loopc];
        for (unsigned int loopc2=0; loopc2<l.imports.size(); loopc2++)
	{
            import_names.push_back(l.imports[loopc2]);
	}
    }
    import_pointers = new uint64_t[import_names.size()];
}

void MemoryImage::finalise()
{
    Mem mem;
    mem.changePerms(mems[IMAGE_CODE], MEM_READ | MEM_EXEC);
    mem.changePerms(mems[IMAGE_DATA], MEM_READ | MEM_WRITE);
    mem.changePerms(mems[IMAGE_CONST_DATA], MEM_READ);
    mem.changePerms(mems[IMAGE_UNALLOCED_DATA], MEM_READ | MEM_WRITE);
}


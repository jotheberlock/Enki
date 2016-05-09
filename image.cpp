#include <stdio.h>
#include <stdlib.h>
#include "image.h"
#include "symbols.h"
#include "asm.h"
#include "platform.h"

FunctionRelocation::FunctionRelocation(Image * i, FunctionScope * p, uint64 po, FunctionScope * l, uint64 lo)
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
    wee64(image->littleEndian(),patch_site,getValue());
}

BasicBlockRelocation::BasicBlockRelocation(Image * i, FunctionScope * p, uint64 po, uint64 pr, BasicBlock * l)
	: BaseRelocation(i)
{
    image = i;
    to_patch = p;
    patch_offset = po;
    patch_relative = pr;
    to_link = l;
}

AbsoluteBasicBlockRelocation::AbsoluteBasicBlockRelocation(Image * i, FunctionScope * p, uint64 po, BasicBlock * l)
	: BaseRelocation(i)
{
    image = i;
    to_patch = p;
    patch_offset = po;
    patch_relative = po;
    to_link = l;
}

void AbsoluteBasicBlockRelocation::apply()
{
    unsigned char * patch_site = image->functionPtr(to_patch)+patch_offset;
    wees64(image->littleEndian(), patch_site, getValue());
}

void BasicBlockRelocation::apply()
{
    unsigned char * patch_site = image->functionPtr(to_patch)+patch_offset;
    uint64 swizzle = getValue();
    int64 sswizzle = *((int64 *)&swizzle);
    int32 sswizzle32 = (int32)sswizzle;
    wees32(image->littleEndian(), patch_site, sswizzle32);
}

SectionRelocation::SectionRelocation(Image * i,
                                     int p, uint64 po,
                                     int d, uint64 dof)
    : BaseRelocation(i)
{
    patch_section = p;
    patch_offset = po;
    dest_section = d;
    dest_offset = dof;
}

void Reloc::apply(bool le, unsigned char * ptr, uint64 val)
{
    val = val >> rshift;
    val = val & mask;
    val = val << lshift;

    if (sf)
    {
        uint64 to_write = ree64(le, ptr+offset);
        to_write = to_write | val;
        unsigned char * poffset = ptr+offset;
        wee64(le, poffset, to_write);
    }
    else
    {
        uint32 to_write = ree32(le, ptr+offset);
        to_write = to_write | (uint32)val;
        unsigned char * poffset = ptr+offset;
        wee32(le, poffset, to_write);
    }
}

void SectionRelocation::apply()
{
    unsigned char * patch_site = image->getPtr(patch_section)+patch_offset;

    if (relocs.size() > 0)
    {
        for (std::list<Reloc>::iterator it = relocs.begin();
             it !=relocs.end(); it++)
        {
            (*it).apply(image->littleEndian(), patch_site, getValue());
        }
        
        return;
    }
    
    wee64(image->littleEndian(), patch_site, getValue());
}

ExtFunctionRelocation::ExtFunctionRelocation(Image * i,
					     FunctionScope * f, uint64 o,
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
    wee64(image->littleEndian(), patch_site, getValue());
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
    base_addr = 0x400000;
    next_addr = base_addr + (4096 * 3);
    fname = "a.out";
    sf_bit = false;
    arch = 0;
    guard_page = false;
}

bool Image::configure(std::string param, std::string val)
{
    if (param == "file")
    {
        fname = val;
    }
    else if (param == "bits")
    {
        if (val == "64")
        {
            sf_bit = true;
        }
        else if (val == "32")
        {
            sf_bit = false;
        }
        else
        {
            return false;
        }
    }
    else if (param == "arch")
    {
        arch = strtol(val.c_str(), 0, 10);
    }
    else if (param == "baseaddr")
    {
        base_addr = strtol(val.c_str(), 0, 0);
        next_addr = base_addr + 12288;
    }
    else if (param == "heapaddr")
    {
        bases[IMAGE_UNALLOCED_DATA] = strtol(val.c_str(), 0, 0);
    }
    else if (param == "guard")
    {
        if (val == "true")
        {
    	    guard_page = true;
        }
        else if (val == "false")
        {
            guard_page = false;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return Component::configure(param, val);
    }
    
    return true;
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

void Image::addFunction(FunctionScope * ptr, uint64 size)
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

uint64 Image::functionAddress(FunctionScope * ptr)
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

uint64 Image::functionSize(FunctionScope * ptr)
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

uint64 MemoryImage::importAddress(std::string name)
{
    for (unsigned int loopc=0; loopc<import_names.size(); loopc++)
    {
        if (import_names[loopc] == name)
        {
            return  (uint64)(&import_pointers[loopc]);
        }
    }

    return INVALID_ADDRESS;
}

void Image::setSectionSize(int t, uint64 l)
{
    sizes[t] = l;
}

uint64 Image::sectionSize(int t)
{
    return sizes[t];
}

uint64 Image::getAddr(int t)
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
    for (int loopc=0; loopc<IMAGE_LAST; loopc++)
    {
        mem.releaseBlock(mems[loopc]);
    }
}

void MemoryImage::materialiseSection(int s)
{
    Mem mem;
    mems[s] = mem.getBlock(sizes[s], MEM_READ | MEM_WRITE);
    bases[s] = (uint64)mems[s].ptr;
    sections[s] = mems[s].ptr;
}

void MemoryImage::setImport(std::string name, uint64 addr)
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
    import_pointers = new uint64[import_names.size()];
}

void MemoryImage::finalise()
{
    Mem mem;
    mem.changePerms(mems[IMAGE_CODE], MEM_READ | MEM_EXEC);
    mem.changePerms(mems[IMAGE_DATA], MEM_READ | MEM_WRITE);
    mem.changePerms(mems[IMAGE_CONST_DATA], MEM_READ);
    mem.changePerms(mems[IMAGE_UNALLOCED_DATA], MEM_READ | MEM_WRITE);
}

uint64 FunctionRelocation::getValue()
{
    uint64 laddr = image->functionAddress(to_link);
    laddr += link_offset;
    return laddr;
}

uint64 AbsoluteBasicBlockRelocation::getValue()
{
    return to_link->getAddr();
}

uint64 BasicBlockRelocation::getValue()
{
    uint64 baddr = to_link->getAddr();
    uint64 oaddr = image->functionAddress(to_patch) + patch_relative;
    
    int64 diff;
    if (oaddr > baddr)
    {
        diff = -((int32)(oaddr-baddr));
    }
    else
    {
        diff = (int32)(baddr-oaddr);
    }
    
    uint64 * ptr = (uint64 *)(&diff);
    return *ptr;
}

uint64 SectionRelocation::getValue()
{
    uint64 addr = image->getAddr(dest_section)+dest_offset;
    return addr;
}

uint64 ExtFunctionRelocation::getValue()
{
    uint64 addr = image->importAddress(fname);
    return addr;
}

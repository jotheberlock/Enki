#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"
#include "symbols.h"
#include "asm.h"
#include "platform.h"
#include "type.h"

FunctionRelocation::FunctionRelocation(Image * i, FunctionScope * p, uint64 po, FunctionScope * l, uint64 lo)
	: BaseRelocation(i)
{
	image = i;
	to_patch = p;
	patch_offset = po;
	to_link = l;
	link_offset = lo;
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

FunctionTableRelocation::FunctionTableRelocation(Image * i, FunctionScope * f,
                                   uint64 o, int s)
    : BaseRelocation(i)
{
    patch_offset = o;
    to_link = f;
    section = s;
}

void Reloc::apply(bool le, unsigned char * ptr, uint64 val)
{
	if (mask == 0)
	{
		if (bits == 8)
		{
            assert(((val | 0xff) == 0x0000000000ff) || ((val | 0xff) == 0xffffffffffffffff));
            *ptr = val & 0xff;
		}
        else if (bits == 16)
        {
            assert(((val | 0xffff) == 0x00000000ffff) || ((val | 0xffff) == 0xffffffffffffffff));
            wee16(le, ptr, val & 0xffff);
        }
        else if (bits == 32)
        {
            assert(((val | 0xffffffff) == 0x0000ffffffff) || ((val | 0xffffffff) == 0xffffffffffffffff));
            wee32(le, ptr, val & 0xffffffff);
        }
		else
		{
			wee64(le, ptr, val);
		}
		return;
	}

	val = val >> rshift;
	val = val & mask;
	val = val << lshift;

    if (bits==8)
    {
        unsigned char to_write = *(ptr+offset);
        to_write = to_write | (unsigned char)val;
        *(ptr+offset) = to_write;
    }
    else if (bits==16)
    {
        uint16 to_write = ree16(le, ptr + offset);
        to_write = to_write | (uint16)val;
        unsigned char * poffset = ptr+offset;
        wee16(le, poffset, to_write);
    }
	else if (bits==32)
	{
		uint32 to_write = ree32(le, ptr + offset);
		to_write = to_write | (uint32)val;
		unsigned char * poffset = ptr + offset;
		wee32(le, poffset, to_write);
	}
	else
	{
		uint64 to_write = ree64(le, ptr + offset);
		to_write = to_write | val;
		unsigned char * poffset = ptr + offset;
		wee64(le, poffset, to_write);
	}
}

void BaseRelocation::apply()
{
	unsigned char * patch_site = getPtr();

    uint64 val = getValue();
    
    uint64 written = val;
    
    int bits = 0;
	for (std::list<Reloc>::iterator it = relocs.begin();
	it != relocs.end(); it++)
	{
        uint64 mask = (*it).mask << (*it).rshift;

        if (mask == 0)
        {
            for (int loopc=0; loopc<(*it).bits; loopc++)
            {
                mask |= 0x1 << loopc;
            }
        }
        
        written = written & ~mask;
        
            // this should really be a BaseRelocation thing...
        if (bits < (*it).bits)
        {
            bits = (*it).bits;
        }
        
		(*it).apply(image->littleEndian(), patch_site, val);
	}
    
    if (written)
    {
            // this is not ideal
        if (written != 0xfffffffffffff000 && written != 0xfffffffffc000000)
        {
            printf("Unwritten bits in %d bit relocation of %llx! %llx\n", bits, val, written);
            display_failure();
        }
    }
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

Image::Image()
{
	for (int loopc = 0; loopc < IMAGE_LAST; loopc++)
	{
		sections[loopc] = 0;
		bases[loopc] = 0;
		sizes[loopc] = 0;
        materialised[loopc] = false;
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

Image::~Image()
{
	for (unsigned int loopc = 0; loopc < relocs.size(); loopc++)
	{
		delete relocs[loopc];
	}
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
	for (unsigned int loopc = 0; loopc < relocs.size(); loopc++)
	{
		relocs[loopc]->apply();
	}
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
	for (unsigned int loopc = 0; loopc < fptrs.size(); loopc++)
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
	for (unsigned int loopc = 0; loopc < fptrs.size(); loopc++)
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
	for (unsigned int loopc = 0; loopc < fptrs.size(); loopc++)
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
	for (unsigned int loopc = 0; loopc < ext_imports.size(); loopc++)
	{
		if (ext_imports[loopc].name == lib)
		{
			LibImport & l = ext_imports[loopc];
			for (unsigned int loopc2 = 0; loopc2 < l.imports.size(); loopc2++)
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
	ext_imports.push_back(l);
	total_imports++;
}

uint64 MemoryImage::importAddress(std::string name)
{
	for (unsigned int loopc = 0; loopc < import_names.size(); loopc++)
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
	if (!materialised[t])
	{
		materialiseSection(t);
	}

	return bases[t];
}

unsigned char * Image::getPtr(int t)
{
	if (!materialised[t])
	{
		materialiseSection(t);
	}
    
	return sections[t];
}

bool Image::getSectionOffset(unsigned char * ptr, int & section, uint64 & offset)
{
    for (int loopc = 0; loopc < IMAGE_LAST; loopc++)
    {
        if (sections[loopc] && ptr >= sections[loopc])
        {
            uint64 off = ptr - sections[loopc];
            if (off < sizes[loopc])
            {
                section = loopc;
                offset = off;
                return true;
            }
        }
    }

    printf("Pointer %p not found in any section!\n", ptr);
    return false;
}


bool Image::getSectionOffset(uint64 addr, int & section,
                             uint64 & offset)
{
    for (int loopc = 0; loopc < IMAGE_LAST; loopc++)
    {
        if (addr >= bases[loopc])
        {
            uint64 off = addr - bases[loopc];
            if (off < sizes[loopc])
            {
                section = loopc;
                offset = off;
                return true;
            }
        }
    }

    printf("Address %lld %llx not found in any section!\n", addr, addr);
    return false;
}

void Image::materialiseSection(int s)
{
	assert(sections[s] == 0);
    
	sections[s] = new unsigned char[sizes[s]];
	memset(sections[s], 0, sizes[s]);
	bases[s] = next_addr;
	next_addr += sizes[s];
	if (!sizes[s])
	{
		next_addr++;
	}
	while (next_addr % 4096)
	{
		next_addr++;
	}

	if (guard_page)
	{
		next_addr += 4096;
	}

    materialised[s] = true;
}


std::string Image::sectionName(int s)
{
    if (s == IMAGE_CODE)
    {
        return "Code";
    }
    else if (s == IMAGE_DATA)
    {
        return "Data";
    }
    else if (s == IMAGE_CONST_DATA)
    {
        return "ConstData";
    }
    else if (s == IMAGE_UNALLOCED_DATA)
    {
        return "UnallocedData";
    }
    else if (s == IMAGE_RTTI)
    {
        return "Rtti";
    }
    else if (s == IMAGE_MTABLES)
    {
        return "Mtables";
    }
    else if (s == IMAGE_EXPORTS)
    {
        return "Exports";
    }

    return "<Unknown!>";
}

MemoryImage::MemoryImage()
{
	import_pointers = 0;
}

MemoryImage::~MemoryImage()
{
	Mem mem;
	for (int loopc = 0; loopc < IMAGE_LAST; loopc++)
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
	for (unsigned int loopc = 0; loopc < import_names.size(); loopc++)
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
	for (unsigned int loopc = 0; loopc < ext_imports.size(); loopc++)
	{
		LibImport & l = ext_imports[loopc];
		for (unsigned int loopc2 = 0; loopc2 < l.imports.size(); loopc2++)
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
    uint64 laddr = 0;
    if (to_link->getType()->isGeneric())
    {
        laddr = mtables->lookup(to_link) + image->getAddr(IMAGE_MTABLES);
    }
    else
    {
        laddr = image->functionAddress(to_link);
    }
    
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
		diff = -((int32)(oaddr - baddr));
	}
	else
	{
		diff = (int32)(baddr - oaddr);
	}

	uint64 * ptr = (uint64 *)(&diff);
	return *ptr;
}

uint64 SectionRelocation::getValue()
{
	uint64 addr = image->getAddr(dest_section) + dest_offset;
	return addr;
}

uint64 ExtFunctionRelocation::getValue()
{
	uint64 addr = image->importAddress(fname);
	return addr;
}

unsigned char * FunctionRelocation::getPtr()
{
	return image->functionPtr(to_patch) + patch_offset;
}

unsigned char * AbsoluteBasicBlockRelocation::getPtr()
{
	return image->functionPtr(to_patch) + patch_offset;
}

unsigned char * BasicBlockRelocation::getPtr()
{
	return image->functionPtr(to_patch) + patch_offset;
}

unsigned char * SectionRelocation::getPtr()
{
	return image->getPtr(patch_section) + patch_offset;
}

unsigned char * ExtFunctionRelocation::getPtr()
{
	return image->getPtr(IMAGE_CODE) + patch_offset;
}

unsigned char * FunctionTableRelocation::getPtr()
{
    return image->getPtr(section) + patch_offset;
}

uint64 FunctionTableRelocation::getValue()
{
    return image->functionAddress(to_link);
}

void BasicBlockRelocation::display_failure()
{
    printf("Failure is a relative bb relocation in function %s, %llx to %llx (%s)\n",
           to_patch->name().c_str(), image->functionAddress(to_patch)+patch_offset, to_link->getAddr(),
           to_link->name().c_str());
}


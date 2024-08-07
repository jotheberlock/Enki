#include "image.h"
#include "asm.h"
#include "platform.h"
#include "symbols.h"
#include "type.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FunctionRelocation::FunctionRelocation(Image *i, FunctionScope *p, uint64_t po, FunctionScope *l, uint64_t lo)
    : BaseRelocation(i)
{
    image = i;
    to_patch = p;
    patch_offset = po;
    to_link = l;
    link_offset = lo;
}

BasicBlockRelocation::BasicBlockRelocation(Image *i, FunctionScope *p, uint64_t po, uint64_t pr, BasicBlock *l)
    : BaseRelocation(i)
{
    image = i;
    to_patch = p;
    patch_offset = po;
    patch_relative = pr;
    to_link = l;
}

AbsoluteBasicBlockRelocation::AbsoluteBasicBlockRelocation(Image *i, FunctionScope *p, uint64_t po, BasicBlock *l)
    : BaseRelocation(i)
{
    image = i;
    to_patch = p;
    patch_offset = po;
    patch_relative = po;
    to_link = l;
}

SectionRelocation::SectionRelocation(Image *i, int p, uint64_t po, int d, uint64_t dof) : BaseRelocation(i)
{
    patch_section = p;
    patch_offset = po;
    dest_section = d;
    dest_offset = dof;
}

FunctionTableRelocation::FunctionTableRelocation(Image *i, FunctionScope *f, uint64_t o, int s) : BaseRelocation(i)
{
    patch_offset = o;
    to_link = f;
    section = s;
}

void Reloc::apply(bool le, unsigned char *ptr, uint64_t val)
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

    if (bits == 8)
    {
        unsigned char to_write = *(ptr + offset);
        to_write = to_write | (unsigned char)val;
        *(ptr + offset) = to_write;
    }
    else if (bits == 16)
    {
        uint16_t to_write = ree16(le, ptr + offset);
        to_write = to_write | (uint16_t)val;
        unsigned char *poffset = ptr + offset;
        wee16(le, poffset, to_write);
    }
    else if (bits == 32)
    {
        uint32_t to_write = ree32(le, ptr + offset);
        to_write = to_write | (uint32_t)val;
        unsigned char *poffset = ptr + offset;
        wee32(le, poffset, to_write);
    }
    else
    {
        uint64_t to_write = ree64(le, ptr + offset);
        to_write = to_write | val;
        unsigned char *poffset = ptr + offset;
        wee64(le, poffset, to_write);
    }
}

void BaseRelocation::apply()
{
    unsigned char *patch_site = getPtr();

    uint64_t val = getValue();

    uint64_t written = val;

    int bits = 0;
    for (auto &it : relocs)
    {
        uint64_t mask = it.mask << it.rshift;

        if (mask == 0)
        {
            for (int loopc = 0; loopc < it.bits; loopc++)
            {
                mask |= (uint64_t)0x1 << loopc;
            }
        }

        written = written & ~mask;

        // this should really be a BaseRelocation thing...
        if (bits < it.bits)
        {
            bits = it.bits;
        }

        it.apply(image->littleEndian(), patch_site, val);
    }

    if (written)
    {
        // this is not ideal
        if (written != 0xfffffffffffff000 && written != 0xfffffffffc000000 && written != 0xfffffffff &&
	    written != 0xffffffff00000000 && written != 0xfffffffff0000000)
        {
            printf("Unwritten bits in %d bit relocation of %lx! %lx\n", bits, val, written);
            display_failure();
        }
    }
}

ExtFunctionRelocation::ExtFunctionRelocation(Image *i, FunctionScope *f, uint64_t o, std::string n) : BaseRelocation(i)
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
    for (auto &it : relocs)
    {
        delete it;
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

void Image::setRootFunction(FunctionScope *f)
{
    root_function = f;
}

void Image::relocate(bool relative_only)
{
    for (auto &it : relocs)
    {
        if ((!relative_only) || (!it->isAbsolute()))
        {
            it->apply();
        }
    }
}

void Image::addFunction(FunctionScope *ptr, uint64_t size)
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

unsigned char *Image::functionPtr(FunctionScope *ptr)
{
    for (int loopc = 0; loopc < fptrs.size(); loopc++)
    {
        if (fptrs[loopc] == ptr)
        {
            return foffsets[loopc] + getPtr(IMAGE_CODE);
        }
    }

    printf("functionPtr - can't find function [%s]!\n", ptr ? ptr->name().c_str() : "<null>");
    return 0;
}

uint64_t Image::functionAddress(FunctionScope *ptr)
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

uint64_t Image::functionSize(FunctionScope *ptr)
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
    auto it = std::find_if(ext_imports.begin(), ext_imports.end(),
			   [&name](LibImport& import) -> bool { return name == import.name; });
    if (it != std::end(ext_imports))
    {
	LibImport &l = *it;
	auto it2 = std::find(l.imports.begin(), l.imports.end(), name);
	if (it2 != std::end(l.imports))
	{
	    return;
	}
	l.imports.push_back(name);
	total_imports++;
	return;
    }

    LibImport l;
    l.name = lib;
    l.imports.push_back(name);
    ext_imports.push_back(l);
    total_imports++;
}

uint64_t MemoryImage::importAddress(std::string name)
{
    for (unsigned int loopc = 0; loopc < import_names.size(); loopc++)
    {
        if (import_names[loopc] == name)
        {
            return (uint64_t)(&import_pointers[loopc]);
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
    if (!materialised[t])
    {
        materialiseSection(t);
    }

    return bases[t];
}

unsigned char *Image::getPtr(int t)
{
    if (!materialised[t])
    {
        materialiseSection(t);
    }

    return sections[t];
}

bool Image::getSectionOffset(unsigned char *ptr, int &section, uint64_t &offset)
{
    for (int loopc = 0; loopc < IMAGE_LAST; loopc++)
    {
        if (sections[loopc] && ptr >= sections[loopc])
        {
            uint64_t off = ptr - sections[loopc];
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

bool Image::getSectionOffset(uint64_t addr, int &section, uint64_t &offset)
{
    for (int loopc = 0; loopc < IMAGE_LAST; loopc++)
    {
        if (addr >= bases[loopc])
        {
            uint64_t off = addr - bases[loopc];
            if (off < sizes[loopc])
            {
                section = loopc;
                offset = off;
                return true;
            }
        }
    }

    printf("Address %ld %lx not found in any section!\n", addr, addr);
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
    bases[s] = (uint64_t)mems[s].ptr;
    sections[s] = mems[s].ptr;
}

void MemoryImage::setImport(std::string name, uint64_t addr)
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
        LibImport &l = ext_imports[loopc];
        for (unsigned int loopc2 = 0; loopc2 < l.imports.size(); loopc2++)
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

uint64_t FunctionRelocation::getValue()
{
    uint64_t laddr = 0;
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

uint64_t AbsoluteBasicBlockRelocation::getValue()
{
    return to_link->getAddr();
}

uint64_t BasicBlockRelocation::getValue()
{
    uint64_t baddr = to_link->getAddr();
    uint64_t oaddr = image->functionAddress(to_patch) + patch_relative;

    int64_t diff;
    if (oaddr > baddr)
    {
        diff = -((int32_t)(oaddr - baddr));
    }
    else
    {
        diff = (int32_t)(baddr - oaddr);
    }

    uint64_t *ptr = (uint64_t *)(&diff);
    return *ptr;
}

uint64_t SectionRelocation::getValue()
{
    uint64_t addr = image->getAddr(dest_section) + dest_offset;
    return addr;
}

uint64_t ExtFunctionRelocation::getValue()
{
    uint64_t addr = image->importAddress(fname);
    return addr;
}

unsigned char *FunctionRelocation::getPtr()
{
    return image->functionPtr(to_patch) + patch_offset;
}

unsigned char *AbsoluteBasicBlockRelocation::getPtr()
{
    return image->functionPtr(to_patch) + patch_offset;
}

unsigned char *BasicBlockRelocation::getPtr()
{
    return image->functionPtr(to_patch) + patch_offset;
}

unsigned char *SectionRelocation::getPtr()
{
    return image->getPtr(patch_section) + patch_offset;
}

unsigned char *ExtFunctionRelocation::getPtr()
{
    return image->getPtr(IMAGE_CODE) + patch_offset;
}

unsigned char *FunctionTableRelocation::getPtr()
{
    return image->getPtr(section) + patch_offset;
}

uint64_t FunctionTableRelocation::getValue()
{
    return image->functionAddress(to_link);
}

void BasicBlockRelocation::display_failure()
{
    printf("Failure is a relative bb relocation in function %s, %lx to %lx (%s)\n", to_patch->name().c_str(),
           image->functionAddress(to_patch) + patch_offset, to_link->getAddr(), to_link->name().c_str());
}

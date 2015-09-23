#include "component.h"
#include "image.h"
#include "elf.h"
#include "pe.h"
#include "amd64.h"
#include "pass.h"
#include "entrypoint.h"

Component * make_windowsentrypoint()
{
    return new WindowsEntrypoint();
}

Component * make_linuxentrypoint()
{
    return new LinuxEntrypoint();
}

Component * make_memoryimage()
{
    return new MemoryImage();
}

Component * make_elf()
{
    return new ElfImage();
}

Component * make_pe()
{
    return new PEImage();
}

Component * make_amd64()
{
    return new Amd64();   
}

Component * make_amd64_unix_syscall()
{
    return new Amd64UnixSyscallCallingConvention();
}

Component * make_amd64_unix()
{
    return new Amd64UnixCallingConvention();
}

Component * make_amd64_windows()
{
    return new Amd64WindowsCallingConvention();
}

Component * make_threetotwo()
{
    return new ThreeToTwoPass();
}

Component * make_sillyregalloc()
{
    return new SillyRegalloc();
}

Component * make_conditionalbranchsplitter()
{
    return new ConditionalBranchSplitter();
}

Component * make_branchremover()
{
    return new BranchRemover();
}

Component * make_addressof()
{
    return new AddressOfPass();
}

Component * make_constmover()
{
    return new ConstMover();
}

Component * make_resolveconstaddr()
{
    return new ResolveConstAddr();
}

Component * make_stacksizepass()
{
    return new StackSizePass();
}

ComponentFactory::ComponentFactory()
{
    add(make_memoryimage, "image", "memory");
    add(make_elf, "image", "elf");
    add(make_pe, "image", "pe");
    add(make_amd64, "asm", "amd64");
    add(make_amd64_unix_syscall, "cconv", "amd64_unix_syscall");
    add(make_amd64_unix, "cconv", "amd64_unix");
    add(make_amd64_windows, "cconv", "amd64_windows");
    add(make_threetotwo, "pass", "threetotwo");
    add(make_sillyregalloc, "pass", "sillyregalloc");
    add(make_conditionalbranchsplitter, "pass", "conditionalbranchsplitter");
    add(make_branchremover, "pass", "branchremover");
    add(make_addressof, "pass", "addressof");
    add(make_constmover, "pass", "constmover");
    add(make_resolveconstaddr, "pass", "resolveconstaddr");
    add(make_stacksizepass, "pass", "stacksize");
    add(make_windowsentrypoint, "entrypoint", "windowsentrypoint");
    add(make_linuxentrypoint, "entrypoint", "linuxentrypoint");
}

void ComponentFactory::add(ComponentMaker ptr, std::string c, std::string n)
{
    makers[c+"/"+n] = ptr;
}

Component * ComponentFactory::make(std::string c, std::string n)
{
    std::map<std::string, ComponentMaker>::const_iterator it =
      makers.find(c+"/"+n);
    if (it == makers.end())
    {
	printf("Couldn't find object %s of class %s in components\n",
	       n.c_str(), c.c_str());
	return 0;
    }
    
    const ComponentMaker ptr = (*it).second;
    return ptr();
}

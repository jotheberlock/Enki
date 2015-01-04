#include "elf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "elf_header.h"
#include "driver.h"

void Elf_Ehdr::dump()
{
	doLog("\nELF header:\n");
	doLog("Type %x\n",e_type);
	doLog("Machine %x\n",e_machine);
	doLog("Version %x\n",e_version);
	doLog("Entry %llx\n",e_entry);
	doLog("Phoff %llx\n",e_phoff);
	doLog("Shoff %llx\n",e_shoff);
	doLog("Flags %x\n",e_flags);
	doLog("Ehsize %x\n",e_ehsize);
	doLog("Phentsize %x\n",e_phentsize);
	doLog("Phnum %x\n",e_phnum);
	doLog("Shentsize %x\n",e_shentsize);
	doLog("Shnum %x\n",e_shnum);
	doLog("Shstrndx %x\n",e_shstrndx);
}


void Elf_Phdr::dump()
{
	doLog("\nProgram header:\n");
	doLog("Type %x\n",p_type);
	doLog("Vaddr %llx\n",p_vaddr);
	doLog("Paddr %llx\n",p_paddr);
	doLog("Filesz %llx\n",p_filesz);
	doLog("Memsz %llx\n",p_memsz);
	doLog("Flags %x\n",p_flags);
	doLog("Align %llx\n",p_align);
	doLog("Offset %llx\n",p_offset);
}

void Elf_Ehdr::write(bool be,bool sf,FILE * f)
{
    unsigned char buf[64];
    memset((void *)buf,0,52);
    unsigned char * ptr=buf;
    memcpy(ptr,e_ident,16);
    ptr+=16;
    if(be) {
		put16b(ptr,e_type);
		put16b(ptr,e_machine);
		put32b(ptr,e_version);
		if(sf) {
			put64b(ptr,e_entry);
			put64b(ptr,e_phoff);
			put64b(ptr,e_shoff);
		} else {
			put32bb(ptr,e_entry);
			put32bb(ptr,e_phoff);
			put32bb(ptr,e_shoff);
		}
        put32b(ptr,e_flags);
		put16b(ptr,e_ehsize);
		put16b(ptr,e_phentsize);
		put16b(ptr,e_phnum);
		put16b(ptr,e_shentsize);
		put16b(ptr,e_shnum);
		put16b(ptr,e_shstrndx);
    } else {
		put16l(ptr,e_type);
		put16l(ptr,e_machine);
		put32l(ptr,e_version);
		if(sf) {
			put64l(ptr,e_entry);
			put64l(ptr,e_phoff);
			put64l(ptr,e_shoff);
		} else {
			put32lb(ptr,e_entry);
			put32lb(ptr,e_phoff);
			doLog("Shoff actual write %ld\n",e_shoff);
			put32lb(ptr,e_shoff);
		}
		put32l(ptr,e_flags);
		put16l(ptr,e_ehsize);
		put16l(ptr,e_phentsize);
		put16l(ptr,e_phnum);
		put16l(ptr,e_shentsize);
		put16l(ptr,e_shnum);
		put16l(ptr,e_shstrndx);
    }
    fwrite((void *)buf,size(sf),1,f);
}

void Elf_Phdr::write(bool be,bool sf,FILE * f)
{
	unsigned char buf[60];
	unsigned char * ptr=buf;
	if(be) {
		put32b(ptr,p_type);
		if(sf) {
			put32b(ptr,p_flags);
			put64b(ptr,p_offset);
			put64b(ptr,p_vaddr);
			put64b(ptr,p_paddr);
			put64b(ptr,p_filesz);
			put64b(ptr,p_memsz);
			put64b(ptr,p_align);
		} else {
			put32bb(ptr,p_offset);
			put32bb(ptr,p_vaddr);
			put32bb(ptr,p_paddr);
			put32bb(ptr,p_filesz);
			put32bb(ptr,p_memsz);
			put32b(ptr,p_flags);
			put32bb(ptr,p_align);
		}
	} else {
		put32l(ptr,p_type);
		if(sf) {
			put32l(ptr,p_flags);
			put64l(ptr,p_offset);
			put64l(ptr,p_vaddr);
			put64l(ptr,p_paddr);
			put64l(ptr,p_filesz);
			put64l(ptr,p_memsz);
			put64l(ptr,p_align);
		} else {
			put32lb(ptr,p_offset);
			put32lb(ptr,p_vaddr);
			put32lb(ptr,p_paddr);
			put32lb(ptr,p_filesz);
			put32lb(ptr,p_memsz);
			put32l(ptr,p_flags);
			put32lb(ptr,p_align);
		}
	}
	fwrite((void *)buf,size(sf),1,f);
}

void Elf_Shdr::write(bool be,bool sf,FILE * f)
{
	unsigned char buf[64];
	unsigned char * ptr=buf;
	if(be) {
		put32b(ptr,sh_name);
		put32b(ptr,sh_type);
		if(sf) {
			put64b(ptr,sh_flags);
			put64b(ptr,sh_addr);
			put64b(ptr,sh_offset);
			put64b(ptr,sh_size);
		} else {
			put32bb(ptr,sh_flags);
			put32bb(ptr,sh_addr);
			put32bb(ptr,sh_offset);
			put32bb(ptr,sh_size);
		}
		put32b(ptr,sh_link);
		put32b(ptr,sh_info);
		if(sf) {
			put64b(ptr,sh_addralign);
			put64b(ptr,sh_entsize);
		} else {
			put32bb(ptr,sh_addralign);
			put32bb(ptr,sh_entsize);
		}
	} else {
		put32l(ptr,sh_name);
		put32l(ptr,sh_type);
		if(sf) {
			put64l(ptr,sh_flags);
			put64l(ptr,sh_addr);
			put64l(ptr,sh_offset);
			put64l(ptr,sh_size);
		} else {
			put32lb(ptr,sh_flags);
			put32lb(ptr,sh_addr);
			put32lb(ptr,sh_offset);
			put32lb(ptr,sh_size);
		}
		put32l(ptr,sh_link);
		put32l(ptr,sh_info);
		if(sf) {
			put64l(ptr,sh_addralign);
			put64l(ptr,sh_entsize);
		} else {
			put32lb(ptr,sh_addralign);
			put32lb(ptr,sh_entsize);
		}
    }
    fwrite((void *)buf,size(sf),1,f);
}

void Elf::allocateBases()
{
    int count=0;
    int loopc;
    for(loopc=FirstSegment;loopc<=LastSegment;loopc++) {
		if(sections[loopc].size!=0)
			count++;
    }
    count++;
	
    elf_header_size=Elf_Ehdr::size(nobits==64)+(Elf_Phdr::size(nobits==64) *
		count);
	
    doLog("Elf header size %d nobits %d\n",elf_header_size,nobits);
	
    totaldisksize=round_upl(elf_header_size,pagesize);
    totalmemsize=round_upl(elf_header_size,pagesize);
	
    doLog("Rounded up size here %d %d\n",totaldisksize,totalmemsize);
	
    for(loopc=FirstSegment;loopc<=LastSegment;loopc++) {
		if(sections[loopc].size!=0) {
			sections[loopc].base=round_upl(virtualbase+totalmemsize,pagesize);
			sections[loopc].diskbase=round_upl(totaldisksize,pagesize);
			BiggestInt tmp=round_upl(sections[loopc].size,pagesize);
			doLog("Rounded from %d to %lld\n",sections[loopc].size,tmp);
			totalmemsize+=tmp;
			if(loopc!=UninitialisedData) {
				doLog("Doing disk size too\n");
				totaldisksize+=tmp;
			}
			doLog("Section %d size %d goes to %lld %lld\n",
				loopc,sections[loopc].size,totaldisksize,totalmemsize);
		}
    }
}

int Elf::stringOffset(char * c)
{
    int ret=0;
    int tmpid=stringtable.getID(c);
    if(tmpid>0) {
		ret=stringtable.offsetOf(tmpid)+1;
    }
    return ret;
}

void Elf::write()
{
    int sectioncount=0;
    int loopc;
	
    stringtable.clear();
    BiggestInt tmemsize=totalmemsize;
    BiggestInt tdisksize=totaldisksize;
	
    for(loopc=FirstSegment;loopc<=LastSegment;loopc++) {
		if(sections[loopc].size!=0) {
			if(loopc==Code) {
				stringtable.add(".text");
			} else if(loopc==ConstantData) {
				stringtable.add(".rodata");
			} else if(loopc==InitialisedData) {
				stringtable.add(".data");
			} else if(loopc==UninitialisedData) {
				stringtable.add(".bss");
			}
			sectioncount++;
		}
    }
	
    sectioncount+=2;
	
    BiggestInt end_of_data=tdisksize;
	
    doLog("End of data is %lld\n",end_of_data);
	
    int sectiontablesize=(sectioncount * Elf_Shdr::size(nobits==64));
    int stringtablesize=stringtable.dataSize()+1;
    stringtablesize=round_up(stringtablesize,8);
	
    tdisksize+=stringtablesize;
    tmemsize+=stringtablesize;
    tdisksize+=sectiontablesize;
    tmemsize+=sectiontablesize;
	
    Elf_Ehdr header;
	
    if(!mainMethod()) {
		doLog("No main()!\n");
    } else {
		doLog("Have main()!\n");
    }
	
    BiggestInt mainAddr=mainMethod() ? mainMethod()->absoluteAddress() : 0;
	
    //unsigned int mainAddr=codebase;
    FILE * f=fopen("a.out","wb");
    if(!f) {
		do_misc_error(CantWriteImage,"a.out");
		return;
    }
	
    header.e_ident[0]=0x7f;
    header.e_ident[1]='E';
    header.e_ident[2]='L';
    header.e_ident[3]='F';
    header.e_ident[4]=(nobits==64) ? 2 : 1;    // 32-bit
    header.e_ident[5]=image_big_endian ? 2 : 1;
    header.e_ident[6]=1;    // Version 1
    header.e_ident[7]=0;
    header.e_ident[8]=0;
    header.e_ident[9]=0;
    header.e_ident[10]=0;
    header.e_ident[11]=0;
    header.e_ident[12]=0;
    header.e_ident[13]=0;
    header.e_ident[14]=0;
    header.e_ident[15]=0;
	
    header.e_type=2;   // Executable file
    header.e_machine=config_db->find("elf_machine",target).intval();
    header.e_version=1;
    header.e_entry=mainAddr;
    header.e_phoff=Elf_Ehdr::size(nobits==64);
    header.e_shoff=(unsigned int)end_of_data+stringtablesize;
    doLog("Section header offset %lld from %lld %d\n",header.e_shoff,
		end_of_data,stringtablesize);
    header.e_flags=0;
    header.e_ehsize=Elf_Ehdr::size(nobits==64);
    header.e_phentsize=Elf_Phdr::size(nobits==64);
    header.e_phnum=sectioncount-1;
    header.e_shentsize=Elf_Shdr::size(nobits==64);
    header.e_shnum=sectioncount;
    header.e_shstrndx=sectioncount-1;
    header.write(image_big_endian,nobits==64,f);
	
    // Program header for ELF image header
    Elf_Phdr pheader;
    pheader.p_type=1;
    pheader.p_offset=0;
    pheader.p_vaddr=virtualbase;
    pheader.p_paddr=virtualbase;
    pheader.p_memsz=elf_header_size;
    pheader.p_filesz=elf_header_size;
    pheader.p_align=0;
    pheader.p_flags=PF_R;
    pheader.write(image_big_endian,nobits==64,f);
	
    for(loopc=FirstSegment;loopc<=LastSegment;loopc++) {
		if(sections[loopc].size!=0 && loopc!=Imports) {
			pheader.p_type=1;
			pheader.p_offset=sections[loopc].diskbase;
			pheader.p_vaddr=sections[loopc].base;
			pheader.p_paddr=sections[loopc].base;
			pheader.p_memsz=sections[loopc].size;
			pheader.p_filesz=sections[loopc].size;
			pheader.p_align=pagesize;
			if(loopc==Code) {
				pheader.p_flags=PF_X | PF_R;
			} else if(loopc==ConstantData) {
				pheader.p_flags=PF_X | PF_R;
				// Just specifying PF_R fails with syscalls on x86
			} else if(loopc==InitialisedData) {
				pheader.p_flags=PF_R | PF_W;
			} else if(loopc==UninitialisedData) {
				pheader.p_flags=PF_R | PF_W;
				pheader.p_filesz=0;
			}
			doLog("[%d] at %llx length %lld file offset %lld flags %d align %llx\n",
				loopc,pheader.p_vaddr,pheader.p_memsz,pheader.p_offset,
				pheader.p_flags,pheader.p_align);
			pheader.write(image_big_endian,nobits==64,f);
		}
    }
	
	
    doLog("Memory size %lld bytes\n",tmemsize);
	
    int outputcount=elf_header_size;
	
    // Round up to next page
    int to_add=4096-elf_header_size;
    for(loopc=0;loopc<to_add;loopc++) {
		outputcount++;
		fputc(0,f);
    }
	
    for(loopc=FirstSegment;loopc<=LastSegment;loopc++) {
		if(loopc!=UninitialisedData && loopc!=Imports) {
			fwrite((void *)sections[loopc].data,(unsigned int)sections[loopc].size,1,f);
			outputcount+=(unsigned int)sections[loopc].size;
		}
    }
	
    // Write string table
    fputc(0,f);
    outputcount++;
    fwrite((void *)stringtable.getData(),stringtable.dataSize(),1,f);
    outputcount+=stringtable.dataSize();
    int tmp=stringtablesize-(stringtable.dataSize()+1);
    for(loopc=0;loopc<tmp;loopc++) {
		fputc(0,f);
		outputcount++;
    }
	
    doLog("Output count %d, offset %d, section count %d, end of data %d\n",
		outputcount,end_of_data+stringtablesize,sectioncount,end_of_data);
	
    Elf_Shdr shdr;
	
    // Write section headers
	
    // Start with 0
	
    shdr.sh_name=0;
    shdr.sh_type=SHT_NULL;
    shdr.sh_flags=0;
    shdr.sh_addr=0;
    shdr.sh_offset=0;
    shdr.sh_size=0;
    shdr.sh_link=SHN_UNDEF;
    shdr.sh_info=0;
    shdr.sh_addralign=0;
    shdr.sh_entsize=0;
	
    shdr.write(image_big_endian,nobits==64,f);
	
    for(loopc=FirstSegment;loopc<=LastSegment;loopc++) {
		if(sections[loopc].size!=0 && loopc != Imports) {
			shdr.sh_addr=sections[loopc].base;
			shdr.sh_offset=sections[loopc].diskbase;
			shdr.sh_size=sections[loopc].size;
			shdr.sh_addralign=pagesize;
			shdr.sh_entsize=0;
			shdr.sh_link=SHN_UNDEF;
			shdr.sh_info=0;
			if(loopc==Code) {
				shdr.sh_name=stringOffset(".text");
				shdr.sh_type=SHT_PROGBITS;
				shdr.sh_flags=SHF_ALLOC | SHF_EXECINSTR;
			} else if(loopc==ConstantData) {
				shdr.sh_name=stringOffset(".rodata");
				shdr.sh_type=SHT_PROGBITS;
				shdr.sh_flags=SHF_ALLOC;
			} else if(loopc==InitialisedData) {
				shdr.sh_name=stringOffset(".data");
				shdr.sh_type=SHT_PROGBITS;
				shdr.sh_flags=SHF_ALLOC | SHF_WRITE;
			} else if(loopc==UninitialisedData) {
				shdr.sh_name=stringOffset(".bss");
				shdr.sh_type=SHT_NOBITS;
				shdr.sh_flags=SHF_ALLOC | SHF_WRITE;
			}
			shdr.write(image_big_endian,nobits==64,f);
		}
    }
	
    doLog("Actual position here %ld\n",ftell(f));
	
    // String table section
    shdr.sh_name=0;
    shdr.sh_type=SHT_STRTAB;
    shdr.sh_flags=0;
    shdr.sh_addr=0;
    shdr.sh_offset=end_of_data;
    shdr.sh_size=stringtablesize;
    shdr.sh_link=SHN_UNDEF;
    shdr.sh_info=0;
    shdr.sh_addralign=0;
    shdr.sh_entsize=0;
	
    shdr.write(image_big_endian,nobits==64,f);
	
    fclose(f);
    doLog("Code starts at %lld %llx\n",mainAddr,mainAddr);
    doLog("Code base is %lld %llx\n",segmentBase(Code),segmentBase(Code));
	
    dump("a.out");
}

void Elf::dump(char * file)
{
    return;
	
    doLog("Dumping %s\n",file);
    unsigned char buf[40000];
    FILE * f=fopen(file,"r+");
    if(!f) {
		do_misc_error(CantAccessFile,file);
		return;
    }
    fread((void *)buf,40000,1,f);
    fclose(f);
	
    Elf_Ehdr * header=(Elf_Ehdr *)buf;
    Elf_Phdr * programheader=(Elf_Phdr *)(buf+sizeof(Elf_Ehdr));
    header->dump();
    programheader->dump();
    unsigned int tmp=*(unsigned int *)(buf+(header->e_entry-
		programheader->p_vaddr));
    doLog("\nInstruction %x\n",tmp);
    unsigned int stroff=(unsigned int)header->e_shoff;
    Elf_Shdr * hdr=(Elf_Shdr *)(buf+stroff);
    hdr+=header->e_shstrndx;
    doLog("Type %d offset %lld index %d\n",hdr->sh_type,header->e_shoff,
		header->e_shstrndx);
}

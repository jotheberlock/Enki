#include "pe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pe_header.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HintBox::HintBox()
{
   idp=0;
   internal_idp=-1;
   datap=0;
   data=0;
   ids=0;
}

HintBox::~HintBox()
{
   if(data)
      free(data);
   if(ids)
      free(ids);
}

void HintBox::clear()
{
   if(data)
      free(data);
   if(ids)
      free(ids);
   idp=0;
   internal_idp=-1;
   datap=0;
   data=0;
   ids=0;
}

char * HintBox::getText(int i)
{
   if(i<0) {
      i*=-1;
      // Awoogah! Memory leak!
      char * buf=(char *)malloc(30);
      sprintf(buf,"<Internal ID %d>",i);
      return buf;
   } else {
      i--;
      if(i==-1 || i>=idp) {
         doLog("Eek! Illegal ID\n");
         abort();
         return 0;
      }
      return data+ids[i];
   }
}

int HintBox::getID(const char * c)
{
   for(int loopc=0;loopc<idp;loopc++) {
      if(!strcmp(data+ids[loopc]+2,c))
         return loopc+1;
   }
   return 0;
}

int HintBox::getInternalID()
{
   int ret=internal_idp;
   internal_idp--;
   return ret;
}

int HintBox::add(unsigned short int hint, const char * c)
{
   int t=getID(c);
   if(t)
      return t;
   int l=strlen(c)+3;
   int newsize=datap+l+1;
   if(!data) {
      data=(char *)malloc(newsize);
   } else {
      data=(char *)realloc(data,newsize);
   }
   if(!ids) {
      ids=(int *)malloc((idp+1)*sizeof(int));
   } else {
      ids=(int *)realloc(ids,(idp+1)*sizeof(int));
   }

   doLog("Adding hint to hintbox at %d\n", datap);
   *(data+datap) = hint & 0xff;
   *(data+(datap+1)) = (hint >> 8) & 0xff;
   strcpy(data+datap+2,c);
   ids[idp]=datap;
   int ret=idp;
   if (l & 0x1)
      l++;
   datap+=l;
   idp++;
   return ret+1;
}

void HintBox::dump()
{
   doLog("IDs %d datasize %d\n\n",idp,datap);
   for(int loopc=0;loopc<idp;loopc++) {
      doLog("%d [%s]\n",loopc+1,data+ids[loopc]);
   }
}


// There's probably no such thing as big-endian PE. Oh well

void COFFHeader::write(bool be, FILE * f)
{
   unsigned char buf[20];
   memset((void *)buf, 0, 20);
   unsigned char * ptr = buf;

   if(be) {
      put16b(ptr, machine);
      put16b(ptr, no_sections);
      put32b(ptr, timestamp);
      put32b(ptr, symboltableptr);
      put32b(ptr, no_symbols);
      put16b(ptr, optional_header_size);
      put16b(ptr, flags);
   } else {
      put16l(ptr, machine);
      put16l(ptr, no_sections);
      put32l(ptr, timestamp);
      put32l(ptr, symboltableptr);
      put32l(ptr, no_symbols);
      put16l(ptr, optional_header_size);
      put16l(ptr, flags);
   }

   fwrite((void *)buf,size(),1,f);
}

void COFFOptionalHeader::write(bool be, bool sf, FILE * f)
{
   unsigned char buf[28];
   memset((void *)buf, 0, 28);
   unsigned char * ptr = buf;

   if(be) {
      put16b(ptr, magic);
      put8(ptr, linker_major);
      put8(ptr, linker_minor);
      put32b(ptr, size_code);
      put32b(ptr, size_init_data);
      put32b(ptr, size_uninit_data);
      put32b(ptr, entry_point);
      put32b(ptr, code_base);
      if(!sf) {
         put32b(ptr, data_base);
      }
   } else {
      put16l(ptr, magic);
      put8(ptr, linker_major);
      put8(ptr, linker_minor);
      put32l(ptr, size_code);
      put32l(ptr, size_init_data);
      put32l(ptr, size_uninit_data);
      put32l(ptr, entry_point);
      put32l(ptr, code_base);
      if(!sf) {
         put32l(ptr, data_base);
      }
   }

   fwrite((void *)buf,size(sf),1,f);
}

void PEOptionalHeader::write(bool be, bool sf, FILE * f)
{
   unsigned char buf[88];
   memset((void *)buf, 0, 88);
   unsigned char * ptr = buf;

   if(be) {
      if(sf) {
         put64b(ptr, image_base);
      } else {
         put32bb(ptr, image_base);
      }
      put32b(ptr, section_align);
      put32b(ptr, file_align);
      put16b(ptr, os_major);
      put16b(ptr, os_minor);
      put16b(ptr, image_major);
      put16b(ptr, image_minor);
      put16b(ptr, subsystem_major);
      put16b(ptr, subsystem_minor);
      put32b(ptr, reserved);
      put32b(ptr, image_size);
      put32b(ptr, headers_size);
      put32b(ptr, checksum);
      put16b(ptr, subsystem);
      put16b(ptr, dll_flags);
      if(sf) {
         put64b(ptr, stack_reserve);
         put64b(ptr, stack_commit);
         put64b(ptr, heap_reserve);
         put64b(ptr, heap_commit);
      } else {
         put32bb(ptr, stack_reserve);
         put32bb(ptr, stack_commit);
         put32bb(ptr, heap_reserve);
         put32bb(ptr, heap_commit);
      }
      put32b(ptr, loader_flags);
      put32b(ptr, rva_number);
   } else {
      if(sf) {
         put64l(ptr, image_base);
      } else {
         put32lb(ptr, image_base);
      }
      put32l(ptr, section_align);
      put32l(ptr, file_align);
      put16l(ptr, os_major);
      put16l(ptr, os_minor);
      put16l(ptr, image_major);
      put16l(ptr, image_minor);
      put16l(ptr, subsystem_major);
      put16l(ptr, subsystem_minor);
      put32l(ptr, reserved);
      put32l(ptr, image_size);
      put32l(ptr, headers_size);
      put32l(ptr, checksum);
      put16l(ptr, subsystem);
      put16l(ptr, dll_flags);
      if(sf) {
         put64l(ptr, stack_reserve);
         put64l(ptr, stack_commit);
         put64l(ptr, heap_reserve);
         put64l(ptr, heap_commit);
      } else {
         put32lb(ptr, stack_reserve);
         put32lb(ptr, stack_commit);
         put32lb(ptr, heap_reserve);
         put32lb(ptr, heap_commit);
      }
      put32l(ptr, loader_flags);
      put32l(ptr, rva_number);
   }

   fwrite((void *)buf,size(sf),1,f);
}

void PESectionHeader::write(bool be, FILE * f)
{
   unsigned char buf[40];
   memset((void *)buf, 0, 40);
   unsigned char * ptr = buf;

   memcpy((void *)buf, (void *)name, 8);
   ptr += 8;

   if(be) {
      put32b(ptr, virtual_size);
      put32b(ptr, virtual_address);
      put32b(ptr, raw_size);
      put32b(ptr, raw_ptr);
      put32b(ptr, relocations_ptr);
      put32b(ptr, linenums_ptr);
      put16b(ptr, no_relocs);
      put16b(ptr, no_linenums);
      put32b(ptr, flags);
   } else {
      put32l(ptr, virtual_size);
      put32l(ptr, virtual_address);
      put32l(ptr, raw_size);
      put32l(ptr, raw_ptr);
      put32l(ptr, relocations_ptr);
      put32l(ptr, linenums_ptr);
      put16l(ptr, no_relocs);
      put16l(ptr, no_linenums);
      put32l(ptr, flags);
   }

   fwrite((void *)buf,size(),1,f);
}

void PEImportDirectory::write(bool be, unsigned char * ptr)
{
   if(be) {
      put32b(ptr, ilt_rva);
      put32b(ptr, timestamp);
      put32b(ptr, forwarder_chain);
      put32b(ptr, name_rva);
      put32b(ptr, iat_rva);
   } else {
      put32l(ptr, ilt_rva);
      put32l(ptr, timestamp);
      put32l(ptr, forwarder_chain);
      put32l(ptr, name_rva);
      put32l(ptr, iat_rva);
   }
}

void PE::allocateBases()
{
   // Allocate import section
   int imports_size = 1024; //PEImportDirectory::size() * 2) + (8*4) + strlen("kernel32.dll") + strlen("ntdll.dll") + 2;
   allocateSection(Imports, imports_size);

   int count=0;
   int loopc;
   for(loopc=FirstSegment;loopc<=LastSegment;loopc++) {
      if(sections[loopc].size!=0) {
         count++;
      }
   }
   //count++;

   // 256-byte DOS header, PE signature, COFF header, optional header and data directories
   pe_header_size = 256 + 4 + COFFHeader::size() + COFFOptionalHeader::size(nobits == 64) + PEOptionalHeader::size(nobits == 64);
   pe_header_size += (count * PESectionHeader::size());

   doLog("PE header size %d nobits %d\n",pe_header_size,nobits);

   totaldisksize=round_upl(pe_header_size,pagesize);
   totalmemsize=round_upl(pe_header_size,pagesize);

   doLog("Rounded up size here %d %d\n",totaldisksize,totalmemsize);

   for(loopc=FirstSegment;loopc<=LastSegment;loopc++) {
      ImageSection & sec = sections[loopc];
      if(sec.size!=0) {
         sec.base=round_upl(virtualbase+totalmemsize,pagesize);
         sec.diskbase=round_upl(totaldisksize,pagesize);
         BiggestInt tmp=round_upl(sec.size,pagesize);
         doLog("Rounded from %d to %lld\n",sec.size,tmp);
         totalmemsize+=tmp;
         if(loopc!=UninitialisedData) {
            doLog("Doing disk size too\n");
            totaldisksize+=tmp;
         }
         doLog("Section %d size %d goes to %lld %lld\n",
            loopc,sec.size,totaldisksize,totalmemsize);
      }
   }
}

void PE::write()
{
   buildImports();

   unsigned char dosheader[256];
   memset(dosheader, 0, 256);

   dosheader[0] = 0x4d;
   dosheader[1] = 0x5a;
   dosheader[2] = 0x50;
   dosheader[4] = 0x2;
   dosheader[8] = 0x4;
   dosheader[0xa] = 0xf;
   dosheader[0xc] = 0xff;
   dosheader[0xd] = 0xff;
   dosheader[0x10] = 0xb8;
   dosheader[0x18] = 0x40;
   dosheader[0x1a] = 0x1a;
   dosheader[0x3d] = 0x1;
   dosheader[0x40] = 0xba;
   dosheader[0x41] = 0x10;
   dosheader[0x43] = 0xe;
   dosheader[0x44] = 0x1f;
   dosheader[0x45] = 0xb4;
   dosheader[0x46] = 0x9;
   dosheader[0x47] = 0xcd;
   dosheader[0x48] = 0x21;
   dosheader[0x49] = 0xb8;
   dosheader[0x4a] = 0x01;
   dosheader[0x4b] = 0x4c;
   dosheader[0x4c] = 0xcd;
   dosheader[0x4d] = 0x21;
   dosheader[0x4e] = 0x90;
   dosheader[0x4f] = 0x90;
   strcpy((char *)&dosheader[0x50], "This program must be run under Win32");
   dosheader[0x74] = 0xd;
   dosheader[0x75] = 0xa;
   dosheader[0x76] = 0x24;
   dosheader[0x77] = 0x37;


   int sectioncount=0;
   int loopc;

   BiggestInt mainAddr=mainMethod() ? mainMethod()->absoluteAddress() : 0;

   BiggestInt code_size = 0;
   BiggestInt init_size = 0;
   BiggestInt uninit_size = 0;
   BiggestInt code_base = 0;
   BiggestInt init_base = 0;
   BiggestInt uninit_base = 0;

   for(loopc=FirstSegment;loopc<=LastSegment;loopc++) {
      if(sections[loopc].size!=0) {
         if(loopc==Code) {
            code_size = sections[loopc].size;
            code_base = sections[loopc].base;
         } else if(loopc==ConstantData) {
         } else if(loopc==InitialisedData || loopc==ConstantData) {
            init_size += sections[loopc].size;
            if (init_base == 0 || (sections[loopc].base < init_base))
            {
               init_base = sections[loopc].base;
            }
         } else if(loopc==UninitialisedData) {
            uninit_size = sections[loopc].size;
            uninit_base = sections[loopc].base;
         }
         sectioncount++;
      }
   }

   if (init_base == 0) {
      init_base = code_base;
   }

   int optionalheadersize = COFFOptionalHeader::size(nobits == 64) + PEOptionalHeader::size(nobits == 64) + (16*8);

   FILE * f=fopen("a.exe","wb");
   if(!f) {
      do_misc_error(CantWriteImage,"a.exe");
      return;
   }

   fwrite(dosheader, 256, 1, f);

   // Signature
   unsigned char sbuf[4];
   sbuf[0]='P';
   sbuf[1]='E';
   sbuf[2]=0;
   sbuf[3]=0;
   fwrite((void *)sbuf, 4, 1, f);

   COFFHeader hdr;
   hdr.machine = 0x14c;
   hdr.no_sections = sectioncount;
   hdr.timestamp = 0x2a425e19;
   hdr.symboltableptr = 0;
   hdr.no_symbols = 0;
   hdr.optional_header_size = optionalheadersize;
   hdr.flags = 0x1 | 0x2 | 0x4 | 0x8 | 0x100 | 0x200;
   hdr.write(image_big_endian, f);

   COFFOptionalHeader ohdr;
   ohdr.magic = 0x10b;
   ohdr.linker_major = 6;
   ohdr.linker_minor = 0;
   ohdr.size_code = (unsigned int)code_size;
   ohdr.size_init_data = (unsigned int)init_size;
   ohdr.size_uninit_data = (unsigned int)uninit_size;
   ohdr.entry_point = (unsigned int)(mainAddr - virtualbase);
   ohdr.code_base = (unsigned int)(code_base - virtualbase);
   ohdr.data_base = (unsigned int)(init_base - virtualbase);
   ohdr.write(image_big_endian, nobits==64, f);

   PEOptionalHeader phdr;
   phdr.image_base = virtualbase;
   phdr.section_align = pagesize;
   phdr.file_align = pagesize;
   phdr.os_major = 4;
   phdr.os_minor = 0;
   phdr.image_major = 1;
   phdr.image_minor = 0;
   phdr.subsystem_major = 4;
   phdr.subsystem_minor = 0;
   phdr.reserved = 0;
   phdr.image_size = (unsigned int)round_upl(totalmemsize, pagesize);
   phdr.headers_size = 0x400;
   phdr.checksum = 0;
   phdr.subsystem = 3;
   phdr.dll_flags = 0;
   phdr.stack_reserve = 0x100000;
   phdr.stack_commit = 0x4000;
   phdr.heap_reserve = 0x100000;
   phdr.heap_commit = 0x4000;
   phdr.loader_flags = 0;
   phdr.rva_number = 16;
   phdr.write(image_big_endian, nobits==64, f);

   // Data directories
   uint32_t rva;
   uint32_t size;
   for(loopc=0; loopc<16; loopc++)
   {
      if (loopc==1) {
         rva=(unsigned int)(sections[Imports].base - virtualbase);
         size=(unsigned int)sections[Imports].size;
      } else {
         rva=0;
         size=0;
      }

      unsigned char buf[8];
      unsigned char * ptr = buf;
      put32l(ptr, rva);
      put32l(ptr, size);
      fwrite((void *)buf, 8, 1, f);
   }

   // Section headers
   for(loopc=FirstSegment;loopc<=LastSegment;loopc++) {
      if(sections[loopc].size!=0) {
         PESectionHeader shdr;

         shdr.virtual_size = (unsigned int)sections[loopc].size;
         shdr.virtual_address = (unsigned int)(sections[loopc].base - virtualbase);
         shdr.raw_size = (loopc == UninitialisedData) ? 0 : (unsigned int)sections[loopc].size;
         shdr.raw_ptr = (unsigned int)sections[loopc].diskbase;
         shdr.relocations_ptr = 0;
         shdr.linenums_ptr = 0;
         shdr.no_relocs = 0;
         shdr.no_linenums = 0;

         memset(shdr.name, 0, 8);

         if(loopc==Code) {
            strcpy(shdr.name, ".text");
            shdr.flags = 0x20 | 0x20000000 | 0x40000000;
         } else if(loopc==ConstantData) {
            strcpy(shdr.name, ".rdata");
            shdr.flags = 0x40 | 0x40000000;
         } else if(loopc==InitialisedData) {
            strcpy(shdr.name, ".data");
            shdr.flags = 0x40 | 0x40000000 | 0x80000000;
         } else if(loopc==Imports) {
            strcpy(shdr.name, ".idata");
            shdr.flags = 0x40 | 0x40000000 | 0x80000000;
         } else if(loopc==UninitialisedData) {
            strcpy(shdr.name, ".bss");
            shdr.flags = 0x80 | 0x40000000 | 0x80000000;
            shdr.raw_ptr = 0;
         } else {
            strcpy(shdr.name, "???");
            shdr.flags = 0;
         }

         shdr.write(image_big_endian, f);
      }
   }

   for(loopc=FirstSegment;loopc<=LastSegment;loopc++) {
      if (loopc != UninitialisedData) {
         ImageSection & sec = sections[loopc];

         if(sec.size!=0) {
            doLog("Section %d seeking to %d\n", loopc, sec.diskbase);
            fseek(f, (unsigned int)sec.diskbase, SEEK_SET);
            fwrite((void *)sec.data,(unsigned int)sec.size,1,f);
         }
      }
   }

   fclose(f);
}

void PE::emitImport(LibraryImport * lib, unsigned char * ptr, int & offset, int & ilt_offset, int & iat_offset,
                    HintBox & names, int stringtable_offset, StringBox * global_names)
{
   ilt_offset = offset;
   iat_offset = ilt_offset + ((lib->methods.length()+1) * (nobits == 64 ? 8 : 4));
   offset = iat_offset + ((lib->methods.length()+1) * (nobits == 64 ? 8 : 4));

   int loopc;

   // Emit ilt
   for(loopc=0; loopc<lib->methods.length(); loopc++)
   {
      int method_id = names.getID(global_names->getText(lib->methods[loopc].name));
      int name_offset = names.offsetOf(method_id) + stringtable_offset;

      if(nobits==64) {
         if (image_big_endian)
         {
            put64b(ptr, name_offset);
         }
         else
         {
            put32b(ptr, name_offset);
         }
      } else {
         if (image_big_endian)
         {
            put64l(ptr, name_offset);
         }
         else
         {
            put32l(ptr, name_offset);
         }
      }
   }
   if(nobits==64) {
      if (image_big_endian)
      {
         put64b(ptr, 0);
      }
      else
      {
         put32b(ptr, 0);
      }
   } else {
      if (image_big_endian)
      {
         put64l(ptr, 0);
      }
      else
      {
         put32l(ptr, 0);
      }
   }


   // Emit iat
   for(loopc=0; loopc<lib->methods.length(); loopc++)
   {
      sections[Code].applyFixup(lib->library_name, lib->methods[loopc].name, sections[Imports].base +
         iat_offset + (loopc * (nobits==64 ? 8 : 4)));

      int method_id = names.getID(global_names->getText(lib->methods[loopc].name));
      int name_offset = names.offsetOf(method_id) + stringtable_offset;

      if(nobits==64) {
         if (image_big_endian)
         {
            put64b(ptr, name_offset);
         }
         else
         {
            put32b(ptr, name_offset);
         }
      } else {
         if (image_big_endian)
         {
            put64l(ptr, name_offset);
         }
         else
         {
            put32l(ptr, name_offset);
         }
      }
   }
   if(nobits==64) {
      if (image_big_endian)
      {
         put64b(ptr, 0);
      }
      else
      {
         put32b(ptr, 0);
      }
   } else {
      if (image_big_endian)
      {
         put64l(ptr, 0);
      }
      else
      {
         put32l(ptr, 0);
      }
   }
}

void PE::buildImports()
{
   int loopc, loopc2;

   HintBox names;

   int method_count = 0;
   int library_count = imports.length();

   for(loopc=0; loopc<imports.length(); loopc++) {
      doLog("Library %s\n", stringBox()->getText(imports[loopc]->library_name));
      names.add(0, stringBox()->getText(imports[loopc]->library_name));
      for(loopc2=0; loopc2<imports[loopc]->methods.length();loopc2++) {
         doLog("  %s\n", stringBox()->getText(imports[loopc]->methods[loopc2].name));
         names.add(imports[loopc]->methods[loopc2].hint, stringBox()->getText(imports[loopc]->methods[loopc2].name));
         method_count++;
      }
   }

   ImageSection & sec = sections[Imports];

   int ilt_len = (nobits==64) ? 16 : 8;

   unsigned int mybase = ((unsigned int)(sec.base - virtualbase));
   PEImportDirectory pid;
   int header_end = pid.size() * (library_count + 1);
   int ilt_end = header_end + (method_count * (ilt_len * 2)) + (library_count * (ilt_len * 2));

   int offset = header_end;

   sec.data = new unsigned char[ilt_end+names.dataSize()+1];
   unsigned char * ptr = sec.data;

   for (loopc=0; loopc<imports.length(); loopc++) {
      int ilt_rva;
      int iat_rva;
      LibraryImport * lib = imports[loopc];

      emitImport(lib, sec.data + offset, offset, ilt_rva, iat_rva, names, mybase+ilt_end, stringBox());

      pid.ilt_rva = mybase+ilt_rva;
      pid.timestamp = 0;
      pid.forwarder_chain = 0;

      int lib_id = names.getID(stringBox()->getText(lib->library_name));
      int name_offset = names.offsetOf(lib_id);

      pid.name_rva = mybase + ilt_end + name_offset + 2;
      pid.iat_rva = mybase+iat_rva;
      pid.write(image_big_endian, ptr);
      ptr += pid.size();
   }

   // Null header entry
   pid.ilt_rva = 0;
   pid.timestamp = 0;
   pid.forwarder_chain = 0;
   pid.name_rva = 0;
   pid.iat_rva = 0;
   pid.write(image_big_endian, ptr);
   ptr += pid.size();

   memcpy(sec.data + (ilt_end), names.getData(), names.dataSize());
   sections[Imports].size = ilt_end+names.dataSize();
   //sections[Imports].size = round_upl(ilt_end+names.dataSize(), pagesize);
}

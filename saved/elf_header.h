#ifndef _ELF_HEADER_
#define _ELF_HEADER_

typedef struct {
	
    unsigned char     e_ident[16];
    uint16_t          e_type;
    uint16_t          e_machine;
    uint32_t          e_version;
    BiggestInt        e_entry;
    BiggestInt        e_phoff;
    BiggestInt        e_shoff;
    uint32_t          e_flags;
    uint16_t          e_ehsize;
    uint16_t          e_phentsize;
    uint16_t          e_phnum;
    uint16_t          e_shentsize;
    uint16_t          e_shnum;
    uint16_t          e_shstrndx;
	
    static int size(bool sf) { return sf ? 64  : 52; }
	
    void dump();
    void write(bool,bool,FILE *);
	
} Elf_Ehdr;

typedef struct {
	
    uint32_t p_type;
    BiggestInt p_offset;
    BiggestInt p_vaddr;
    BiggestInt p_paddr;
    BiggestInt p_filesz;
    BiggestInt p_memsz;
    uint32_t p_flags;
    BiggestInt p_align;
	
    static int size(bool sf) { return sf ? 56 : 32; }
	
    void dump();
    void write(bool,bool,FILE *);
	
} Elf_Phdr;

typedef struct {
	
    uint32_t	sh_name;
    uint32_t	sh_type;
    BiggestInt	sh_flags;
    BiggestInt	sh_addr;
    BiggestInt	sh_offset;
    BiggestInt	sh_size;
    uint32_t	sh_link;
    uint32_t	sh_info;
    BiggestInt	sh_addralign;
    BiggestInt	sh_entsize;
	
    static int size(bool sf) { return sf ? 64 : 40; }
	
    void write(bool,bool,FILE *);
	
} Elf_Shdr;

#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_NOBITS 8
#define SHT_STRTAB 3

#define SHN_UNDEF 0

#define SHF_EXECINSTR 0x4
#define SHF_ALLOC 0x2
#define SHF_WRITE 0x1

#define PF_X 0x1
#define PF_R 0x2
#define PF_W 0x4

#endif

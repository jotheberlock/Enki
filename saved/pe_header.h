#ifndef _PE_HEADER_
#define _PE_HEADER_

typedef struct {
	
	uint16_t machine;
	uint16_t no_sections;
	uint32_t timestamp;
	uint32_t symboltableptr;
	uint32_t no_symbols;
	uint16_t optional_header_size;
	uint16_t flags;
	
	static int size() { return 20; }
	void write(bool, FILE *);
	
} COFFHeader;

typedef struct {
	
	uint16_t magic;
	uint8_t linker_major;
	uint8_t linker_minor;
	uint32_t size_code;
	uint32_t size_init_data;
	uint32_t size_uninit_data;
	uint32_t entry_point;
	uint32_t code_base;
	uint32_t data_base;
	
	static int size(bool sf) { return sf ? 24 : 28; }
	void write(bool, bool, FILE *);
	
} COFFOptionalHeader;

typedef struct {
	
	BiggestInt image_base;
	uint32_t section_align;
	uint32_t file_align;
	uint16_t os_major;
	uint16_t os_minor;
	uint16_t image_major;
	uint16_t image_minor;
	uint16_t subsystem_major;
	uint16_t subsystem_minor;
	uint32_t reserved;
	uint32_t image_size;
	uint32_t headers_size;
	uint32_t checksum;
	uint16_t subsystem;
	uint16_t dll_flags;
	BiggestInt stack_reserve;
	BiggestInt stack_commit;
	BiggestInt heap_reserve;
	BiggestInt heap_commit;
	uint32_t loader_flags;
	uint32_t rva_number;
	
	static int size(bool sf) { return sf ? 88 : 68; }
	void write(bool, bool, FILE *);
	
} PEOptionalHeader;

typedef struct {
	
	char name[8];
	uint32_t virtual_size;
	uint32_t virtual_address;
	uint32_t raw_size;
	uint32_t raw_ptr;
	uint32_t relocations_ptr;
	uint32_t linenums_ptr;
	uint16_t no_relocs;
	uint16_t no_linenums;
	uint32_t flags;
	
	static int size() { return 40; }
	void write(bool, FILE *);
	
} PESectionHeader;

typedef struct {
	
	uint32_t ilt_rva;
	uint32_t timestamp;
	uint32_t forwarder_chain;
	uint32_t name_rva;
	uint32_t iat_rva;
	
	static int size() { return 20; }
	void write(bool, unsigned char *);
	
} PEImportDirectory;

#endif

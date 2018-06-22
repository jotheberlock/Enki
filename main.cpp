
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "platform.h"
#include "lexer.h"
#include "ast.h"
#include "error.h"
#include "type.h"
#include "codegen.h"
#include "mem.h"
#include "amd64.h"
#include "pass.h"
#include "cfuncs.h"
#include "symbols.h"
#include "image.h"
#include "elf.h"
#include "component.h"
#include "configfile.h"
#include "backend.h"
#include "rtti.h"
#include "exports.h"
#include "imports.h"

Assembler * assembler = 0;
CallingConvention * calling_convention = 0;
FILE * log_file = 0;
Constants * constants = 0;
FunctionScope * root_scope = 0;
ComponentFactory * component_factory = 0;
Types * types = 0;
Rtti * rtti = 0;
Mtables * mtables = 0;
Exports * exports = 0;
Imports * imports = 0;

typedef uint64(*TestFunc)(uint64);

Codegen * root_gc = 0;
unsigned char * root_buf = 0;

void dumpstack()
{
	if (root_buf && root_gc)
	{
		printf("Dumping stack:\n%s\n", root_gc->display(root_buf).c_str());
	}
	else
	{
		printf("Nothing to dump\n");
	}
}

uint64 text_base = 0;
uint64 text_len = 0;
uint64 data_base = 0;
uint64 data_len = 0;
uint64 rodata_base = 0;
uint64 rodata_len = 0;

bool valid_pointer(unsigned char * ptr)
{
	uint64 val = (uint64)ptr;
	if (val >= text_base && val < (text_base + text_len))
	{
		return true;
	}
	if (val >= data_base && val < (data_base + data_len))
	{
		return true;
	}
	if (val >= rodata_base && val < (rodata_base + rodata_len))
	{
		return true;
	}
	return false;
}

void dump_codegen(Codegen * cg)
{
	fprintf(log_file, "\n\nCode for %s:\n", cg->getScope()->name().c_str());
	std::vector<BasicBlock *> & bbs = cg->getBlocks();
	for (unsigned int loopc = 0; loopc < bbs.size(); loopc++)
	{
		std::string bp = bbs[loopc]->toString();
		fprintf(log_file, "\n%s\n", bp.c_str());
	}
}

uint32 getUtf8(char * & f)
{
	uint32 val = *f;
	f++;

	if (!(val & 0x80))
		return val;

	val &= ~0x80;
	int nobytes = 0;
	if (val & 0x40)
	{
		nobytes++;
		val &= ~0x40;
	}
	if (val & 0x20)
	{
		nobytes++;
		val &= ~0x20;
	}
	if (val & 0x10)
	{
		printf("Invalid UTF-8! Too many bytes\n");
		return 0;
	}

	uint32 ret = val;

	for (int loopc = 0; loopc < nobytes; loopc++)
	{
		val = *f;
		f++;

		if ((!(val & 0x80)) || (val & 0x40))
		{
			printf("Invalid UTF-8! Wrong succeeding byte %x, nobytes %d\n", val, nobytes);
			return 0;
		}

		ret = ret << 6;
		ret |= val;
	}

	return ret;
}

FILE * findFile(std::string name)
{
	FILE * f = fopen(name.c_str(), "rb");
	if (f)
	{
		return f;
	}

	name = std::string("../") + name;
	f = fopen(name.c_str(), "rb");
	if (f)
	{
		return f;
	}

	return 0;
}

void readFile(FILE * f, Chars & input)
{
	fseek(f, 0, SEEK_END);
	int len = ftell(f);
	char * text = new char[len];
	fseek(f, 0, SEEK_SET);
	int read = fread(text, len, 1, f);
	fclose(f);
    if (read < 1)
    {
        printf("readFile got read result of %d!\n", read);
        return;
    }
    
	char * ptr = text;
	while (ptr < text + len)
	{
		uint32 v = getUtf8(ptr);
		if (v)
		{
			input.push_back(v);
		}
	}
	delete[] text;

	input.push_back('\n');
	input.push_back('\n');
}

bool sanity_check()
{
	if (sizeof(uint16) != 2)
	{
		printf("Uint16 is size %ld!\n", sizeof(uint16));
		return false;
	}
	if (sizeof(uint32) != 4)
	{
		printf("Uint32 is size %ld!\n", sizeof(uint32));
		return false;
	}
	if (sizeof(uint64) != 8)
	{
		printf("Uint64 is size %ld!\n", sizeof(uint64));
		return false;
	}
	uint32 test = 0xdeadbeef;
	unsigned char * ptr = (unsigned char *)(&test);
#ifdef HOST_BIG_ENDIAN
	if (*ptr != 0xde)
	{
		printf("This platform is little-endian, expected big-endian!\n");
		return false;
	}
#else
	if (*ptr != 0xef)
	{
		printf("This platform is big-endian, expected little-endian!\n");
		return false;
	}
#endif
	return true;
}

int main(int argc, char ** argv)
{
    bool auto_run = false;
    
	if (!sanity_check())
	{
		return 1;
	}

	component_factory = new ComponentFactory();
	rtti = new Rtti();
    exports = new Exports();
    imports = new Imports();
    
	FILE * hfile = findFile(ConfigFile::hostConfig().c_str());
	if (!hfile)
	{
		printf("Can't find host config [%s], running without it\n", ConfigFile::hostConfig().c_str());
	}
	Configuration hostconfig;

	if (hfile)
	{
		ConfigFile hcf(hfile, &hostconfig);
		hcf.process();
	}
    fclose(hfile);
    
	Configuration config;
    configuration = &config;
    
	ConfigFile * current_config_file = 0;

	bool done_ini = false;
	bool no_stdlib = false;

	for (int loopc = 1; loopc < argc; loopc++)
	{
		if (strstr(argv[loopc], ".ini"))
		{
			delete current_config_file;
			FILE * cfile = findFile(argv[loopc]);
			current_config_file = new ConfigFile(cfile, &config);
			current_config_file->process();
			done_ini = true;
		}
		else if (strstr(argv[loopc], "=") && current_config_file)
		{
			char buf[4096];
			sprintf(buf, "set %s", argv[loopc]);
			if (!current_config_file->processLine(buf))
			{
				printf("Don't know how to set option %s\n", argv[loopc]);
			}
		}
		else if (!strcmp(argv[loopc], "-nostdlib"))
		{
			no_stdlib = true;
		}
        else if (!strcmp(argv[loopc], "-r"))
        {
                // Outputting a relocatable binary
            config.relocatable = true;
        }
		else if (!strcmp(argv[loopc], "-o"))
		{
			loopc++;
			if (loopc < argc)
			{
				if (!current_config_file->processLine(argv[loopc]))
				{
					printf("Don't understand option [%s]\n", argv[loopc]);
				}
			}
			else
			{
				printf("-o without option!\n");
				break;
			}
		}
        else if (!strcmp(argv[loopc], "-r"))
        {
            auto_run = true;
        }
	}

	if (!done_ini)
	{
		FILE * cfile;

        if (config.relocatable)
        {
            cfile = findFile(ConfigFile::relocatableTargetConfig());
        }
        else
        {
            cfile = findFile(ConfigFile::nativeTargetConfig());
        }
        
		if (!cfile)
		{
			printf("Can't find native target config [%s]\n", ConfigFile::nativeTargetConfig().c_str());
			return 1;
		}
		current_config_file = new ConfigFile(cfile, &config);
		current_config_file->process();
        fclose(cfile);
	}

	for (int loopc = 1; loopc < argc; loopc++)
	{
		if (strstr(argv[loopc], "=") && current_config_file)
		{
			char buf[4096];
			sprintf(buf, "set %s", argv[loopc]);
			current_config_file->processLine(buf);
		}
	}

	delete current_config_file;

	bool jit = false;

	Image * image = config.image;
	//MemoryImage * macros = new MemoryImage();

	log_file = fopen("log.txt", "w");
	if (!log_file)
	{
		printf("Failed to open log file\n");
		return 1;
	}

	constants = new Constants();
    mtables = new Mtables();
	assembler = config.assembler;
	types = new Types();
	FunctionType * root_type = new FunctionType(false);
	root_type->setReturn(register_type);
    types->add(root_type, "@root");
    
	root_scope = new FunctionScope(0, "__root", root_type);

	FunctionType * syscall_type = new ExternalFunctionType(config.syscall, true);
	syscall_type->setReturn(register_type);
	Value * fptr = new Value("__syscall", syscall_type);  // Value never actually used
    types->add(syscall_type, "__syscall");
	root_scope->add(fptr);

	calling_convention = config.cconv;

	if (jit)
	{
		set_cfuncs((MemoryImage *)image);
	}

    Lexer lex;
    Lexer ilex;

	if (!no_stdlib)
	{
		for (unsigned int loopc = 0; loopc < config.preloads.size(); loopc++)
		{
			FILE * f = config.open(config.preloads[loopc].c_str());

			if (!f)
			{
				printf("Preload input file %s not found!\n", config.preloads[loopc].c_str());
				return 1;
			}

			Chars input;
			readFile(f, input);
			lex.setFile(config.preloads[loopc]);
			lex.lex(input);
		}
	}

	for (int loopc = 1; loopc < argc; loopc++)
	{
		if (strstr(argv[loopc], ".e"))
		{
			const char * fname = argv[loopc];
			FILE * f = config.open(fname);

			if (!f)
			{
				printf("Input file %s not found\n", fname);
				return 1;
			}

			Chars input;
			readFile(f, input);
			lex.setFile(fname);
			lex.lex(input);
		}
	}

    bool found_interface = false;
    for (int loopc = 1; loopc < argc; loopc++)
    {
        char * ptr = strstr(argv[loopc], ".i");
        if (ptr && *(ptr+2) == 0)
        {
            if (found_interface)
            {
                printf("More than one interface file specified!\n");
                return 1;
            }
            const char * fname = argv[loopc];
            FILE * f = config.open(fname);

            if (!f)
            {
                printf("Input file %s not found\n", fname);
                return 1;
            }

            Chars input;
            readFile(f, input);
            lex.setFile(fname);
            lex.lex(input);
        }
    }

	lex.endLexing();

	if (errors.size() != 0)
	{
		printf("Lexer errors:\n\n");
		for (std::list<Error>::iterator it = errors.begin();
		it != errors.end(); it++)
		{
			printf("\n");
			(*it).print();
		}
		return 2;
	}

	std::vector<Token> & tokens = lex.tokens();
	for (unsigned int loopc = 0; loopc < tokens.size(); loopc++)
	{
		tokens[loopc].print();
	}

	Type * byteptr = types->lookup("Byte^");
	assert(byteptr);

	root_scope->add(new Value("__activation", byteptr));
	root_scope->add(new Value("__stackptr", byteptr));
    root_scope->add(new Value("__exports", register_type));
    if (config.relocatable)
    {
            // Value is initialised by the loader
        root_scope->add(new Value("__imports", register_type));
    }
    
	Parser parse(&lex);

	if (errors.size() != 0)
	{
		printf("Parse errors:\n\n");
		for (std::list<Error>::iterator it = errors.begin();
		it != errors.end(); it++)
		{
			(*it).print();
			printf("\n");
		}
		return 3;
	}

	if (parse.tree())
	{
		parse.tree()->print(0);
	}
	else
	{
		printf("Null tree\n");
		return 4;
	}

	fprintf(log_file, "\n\n\nCodegen:\n\n\n");
    
	rtti->finalise();
    exports->finalise();

	Backend output(&config, parse.tree());
	int ret = output.process();

    delete root_scope;
    delete types;
    delete mtables;
    delete component_factory;
    delete rtti;
    delete constants;
    fclose(log_file);

#ifdef POSIX_HOST    
    if (auto_run)
    {
        ret = system(config.image->fileName().c_str());
    }
#endif
    
    return ret;
}

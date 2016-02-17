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

Assembler * assembler = 0;
CallingConvention * calling_convention = 0;
FILE * log_file = 0;
Constants * constants = 0;
FunctionScope * root_scope = 0;
ComponentFactory * component_factory = 0;

typedef uint64_t (*TestFunc)(uint64_t);

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

uint64_t text_base = 0;
uint64_t text_len = 0;
uint64_t data_base = 0;
uint64_t data_len = 0;
uint64_t rodata_base = 0;
uint64_t rodata_len = 0;

bool valid_pointer(unsigned char * ptr)
{
    uint64_t val = (uint64_t)ptr;
    if (val >= text_base && val < (text_base+text_len))
    {
        return true;
    }
    if (val >= data_base && val < (data_base+data_len))
    {
        return true;
    }
    if (val >= rodata_base && val < (rodata_base+rodata_len))
    {
        return true;
    }
    return false;
}


#ifdef POSIX_SIGNALS
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

pthread_mutex_t dumper = PTHREAD_MUTEX_INITIALIZER;

ucontext * the_ucontext = 0;

void write_reg(const char * rn, uint64_t rv)
{
    char buf[8];
    write(1, rn, strlen(rn));
    write(1, ": 0x", 2);
    for (int loopc=0; loopc<8; loopc++)
    {
       unsigned char val = rv & 0xff00000000000000;
       if (val < 10) 
       { 
            buf[loopc] = '0' + val;
       }
       else
       {
            buf[loopc] = 'a' + (val-10);
       }
       val <<= 8;
    }

    write(1, buf, 8);
    write(1, "\n", 1);
}

void * dumpstacker(void *)
{
    printf("Waiting\n");
    pthread_mutex_lock(&dumper);
    dumpstack();
    if (the_ucontext)
    {
#ifdef CYGWIN_HOST
        write_reg("rax", the_ucontext->rax);
        write_reg("rbx", the_ucontext->rbx);
        write_reg("rcx", the_ucontext->rcx);
        write_reg("rdx", the_ucontext->rdx);
        write_reg("r15", the_ucontext->r15);
        write_reg("rsp", the_ucontext->rsp);
        write_reg("rsi", the_ucontext->rsi);
#endif
#ifdef LINUX_HOST
#ifdef __x86_64__
        printf("rax: %llx\n", the_ucontext->uc_mcontext.gregs[REG_RAX]);
        printf("rbx: %llx\n", the_ucontext->uc_mcontext.gregs[REG_RBX]);
        printf("rcx: %llx\n", the_ucontext->uc_mcontext.gregs[REG_RCX]);
        printf("rdx: %llx\n", the_ucontext->uc_mcontext.gregs[REG_RDX]);
        printf("rsp: %llx\n", the_ucontext->uc_mcontext.gregs[REG_RSP]);
        printf("rbp: %llx\n", the_ucontext->uc_mcontext.gregs[REG_RBP]);
        printf("rsi: %llx\n", the_ucontext->uc_mcontext.gregs[REG_RSI]);
        printf("rdi: %llx\n", the_ucontext->uc_mcontext.gregs[REG_RDI]);
        printf("r8: %llx\n", the_ucontext->uc_mcontext.gregs[REG_R8]);
        printf("r9: %llx\n", the_ucontext->uc_mcontext.gregs[REG_R9]);
        printf("r10: %llx\n", the_ucontext->uc_mcontext.gregs[REG_R10]);
        printf("r11: %llx\n", the_ucontext->uc_mcontext.gregs[REG_R11]);
        printf("r12: %llx\n", the_ucontext->uc_mcontext.gregs[REG_R12]);
        printf("r13: %llx\n", the_ucontext->uc_mcontext.gregs[REG_R13]);
        printf("r14: %llx\n", the_ucontext->uc_mcontext.gregs[REG_R14]);
        printf("r15: %llx\n", the_ucontext->uc_mcontext.gregs[REG_R15]);
        printf("rip: %llx\n", the_ucontext->uc_mcontext.gregs[REG_RIP]);
#endif
#endif
    }
    else
    {
        write(1, "No ucontext\n", 11);
    }

    return 0;
}

void segv_handler(int signo, siginfo_t * info, void * ctx)
{
    printf("Segfault! IP is %p (%s) signal %d code %d\n", info->si_addr,
           valid_pointer((unsigned char *)info->si_addr) ? "valid" : "invalid",
           info->si_signo, info->si_code);
    the_ucontext = (ucontext *)ctx;
    pthread_mutex_unlock(&dumper);
    sleep(100);
    exit(0);
}

void dumpstack_unlocker()
{
    pthread_mutex_unlock(&dumper);
}
#endif

void dump_codegen(Codegen * cg)
{
    fprintf(log_file, "\n\nCode for %s:\n", cg->getScope()->name().c_str());
    std::vector<BasicBlock *> & bbs = cg->getBlocks();
    for (unsigned int loopc=0; loopc<bbs.size(); loopc++)
    {
        std::string bp = bbs[loopc]->toString();
        fprintf(log_file, "\n%s\n", bp.c_str());
    }
}

uint32_t getUtf8(char * & f)
{   
    uint32_t val = *f;
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

    uint32_t ret = val;
    
    for (int loopc=0; loopc<nobytes; loopc++)
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
    FILE * f = 0;
    if (f = fopen(name.c_str(), "rb"))
    {
        return f;
    }
    name = std::string("../")+name;

    if (f = fopen(name.c_str(), "rb"))
    {
        return f;
    }

    return 0;
}

int main(int argc, char ** argv)
{
    component_factory = new ComponentFactory();

    FILE * hfile = findFile(ConfigFile::hostConfig().c_str());
    if (!hfile)
    {
        printf("Can't find host config [%s]\n", ConfigFile::hostConfig().c_str());
        return 1;
    }
    Configuration hostconfig;

    ConfigFile hcf(hfile, &hostconfig);
    hcf.process();

    Configuration config;
    
    ConfigFile * current_config_file = 0;
    
    bool done_ini = false;

    for (int loopc=1; loopc<argc; loopc++)
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
		printf("Don't know how to set %s\n", argv[loopc]);
	    }
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
    }

    if (!done_ini)
    {
        FILE * cfile = findFile(ConfigFile::nativeTargetConfig());
        if (!cfile)
        {
  	    printf("Can't find native target config [%s]\n", ConfigFile::nativeTargetConfig().c_str());
	    return 1;
        }
        current_config_file = new ConfigFile(cfile, &config);
	current_config_file->process();
    }
    
    for (int loopc=1; loopc<argc; loopc++)
    {
        if (strstr(argv[loopc], "=") && current_config_file)
	{
  	    char buf[4096];
  	    sprintf(buf, "set %s", argv[loopc]);
	    current_config_file->processLine(buf);
        }
    }
    
    delete current_config_file;
    
    uint64_t result = 1;

    bool jit = false;
    
    Image * image = config.image;
    MemoryImage * macros = new MemoryImage();

    log_file = fopen("log.txt", "w");
    if (!log_file)
    {
        printf("Failed to open log file\n");
        return 1;
    }

    constants = new Constants();
    assembler = config.assembler;
    initialiseTypes();
    FunctionType * root_type = new FunctionType(false);
    root_type->addReturn(register_type);
    
    root_scope = new FunctionScope(0, "@root", root_type);

    FunctionType * syscall_type = new ExternalFunctionType(config.syscall, true);
    syscall_type->addReturn(register_type);
    Value * fptr = new Value("__syscall", syscall_type);  // Value never actually used
    root_scope->add(fptr);

    calling_convention = config.cconv;

    if (jit)
    {
      	set_cfuncs((MemoryImage *)image);
    }

    Lexer lex;
    
    for (int loopc=0; loopc<config.preloads.size(); loopc++)
    {      
        FILE * f = config.open(config.preloads[loopc].c_str());
      
        if(!f)
	{
  	    printf("No preload input file %s\n", config.preloads[loopc].c_str());
	    return 1;
	}
	    
        Chars input;
      
        fseek(f, 0, SEEK_END);
        int len = ftell(f);
        char * text = new char[len];
        fseek(f, 0, SEEK_SET);
        fread(text, len, 1, f);
        fclose(f);
      
        char * ptr = text;
        while (ptr < text+len)
	{
	    uint32_t v = getUtf8(ptr);
	    if (v)
	    {
	        input.push_back(v);
	    }
	}
        delete[] text;
      
        lex.setFile(config.preloads[loopc]);
        lex.lex(input);
    }
    
    for (int loopc=1; loopc<argc; loopc++)
    {
        if (strstr(argv[loopc], ".e"))
        {
            const char * fname = argv[loopc];
    
            FILE * f = config.open(fname);

            if(!f)
            {
                printf("No input file\n");
                return 1;
            }
	    
            Chars input;

            fseek(f, 0, SEEK_END);
            int len = ftell(f);
            char * text = new char[len];
            fseek(f, 0, SEEK_SET);
            fread(text, len, 1, f);
            fclose(f);
    
            char * ptr = text;
            while (ptr < text+len)
            {
                uint32_t v = getUtf8(ptr);
                if (v)
                {
                    input.push_back(v);
                }
            }
            delete[] text;
    
            lex.setFile(fname);
            lex.lex(input);
        }
    }

    lex.endLexing();
    
    if (errors.size() != 0)
    {
        printf("Lexer errors:\n\n");
        for(std::list<Error>::iterator it = errors.begin();
            it != errors.end(); it++)
        {
            printf("\n");
            (*it).print();
        }
        return 2;
    }

    std::vector<Token> & tokens = lex.tokens();
    for (unsigned int loopc=0; loopc<tokens.size(); loopc++)
    {
        tokens[loopc].print();
    } 

    Parser parse(&lex);

    if (errors.size() != 0)
    {
        printf("Parse errors:\n\n");
        for(std::list<Error>::iterator it = errors.begin();
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

    Type * byteptr = lookupType("Byte^");
    assert(byteptr);

    root_scope->add(new Value("__activation", byteptr));
    root_scope->add(new Value("__stackptr", byteptr));

    Backend output(&config, parse.tree());
    return output.process();
}

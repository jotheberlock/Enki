#include <stdio.h>
#include <string.h>
#include <assert.h>

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

Assembler * assembler = 0;
CallingConvention * calling_convention = 0;
CFuncs * cfuncs = 0;
std::list<Codegen *> * codegens = 0;
FILE * log_file = 0;
Constants * constants = 0;
FunctionScope * root_scope = 0;

typedef uint64_t (*TestFunc)(uint64_t);

Codegen * root_gc = 0;
unsigned char * root_buf = 0;

#define HEAP_SIZE 4096

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
        fprintf(log_file, "%s\n", bp.c_str());
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

    
int main(int argc, char ** argv)
{
    uint64_t result = 1;

    MemoryImage * image = new MemoryImage();
    MemoryImage * macros = new MemoryImage();
    
    log_file = fopen("log.txt", "w");
    if (!log_file)
    {
        printf("Failed to open log file\n");
        return 1;
    }

    constants = new Constants();
    codegens = new std::list<Codegen *>;
    assembler = new Amd64();
    initialiseTypes();
    FunctionType * root_type = new FunctionType("@root", false);
    root_type->addReturn(register_type);
    
    root_scope = new FunctionScope(0, "@root", root_type);
    
#ifdef WINDOWS_CC
    calling_convention = new Amd64WindowsCallingConvention();
#else
    calling_convention = new Amd64UnixCallingConvention();
#endif
    
    cfuncs = new CFuncs();
    cfuncs->add((uint64_t)dumpstack, "dumpstack");
#ifdef POSIX_SIGNALS
    cfuncs->add((uint64_t)dumpstack_unlocker, "dumpstack_unlocker");
#endif

    const char * fname = (argc > 1) ? argv[1] : "test.e";
    
    Lexer lex;
    FILE * f = fopen(fname, "rb");

	if(!f && argc == 1)
	{
        fname = "../test.e";
		f = fopen(fname, "rb");
	}

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
            printf("\n");
            (*it).print();
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
    
    Codegen * gc = new Codegen(parse.tree(), root_scope);
    gc->setExtCall();
    cfuncs->addToCodegen(gc);
    codegens->push_back(gc);
        
    root_gc = gc;
        
    BasicBlock * prologue = gc->block();
    calling_convention->generatePrologue(prologue, root_scope);
    BasicBlock * body = gc->newBlock("body");
    gc->setBlock(body);
    gc->generate();

    if (errors.size() != 0)
    {
        printf("Codegen errors:\n\n");
        for(std::list<Error>::iterator it = errors.begin();
	    it != errors.end(); it++)
        {
            printf("\n");
            (*it).print();
        }
        return 4;
    }
    
    BasicBlock * epilogue = gc->newBlock("epilogue");
    gc->block()->add(Insn(BRA, epilogue));
    calling_convention->generateEpilogue(epilogue, root_scope);

    image->setSectionSize(IMAGE_CONST_DATA, constants->getSize());
    constants->setAddress(image->getAddr(IMAGE_CONST_DATA));
    constants->fillPool(image->getPtr(IMAGE_CONST_DATA));
    rodata_base = image->getAddr(IMAGE_CONST_DATA);
    rodata_len = constants->getSize();
    
    for(std::list<Codegen *>::iterator cit = codegens->begin();
        cit != codegens->end(); cit++)
    {
        Codegen * cg = *cit;
        cg->allocateStackSlots();

        BasicBlock::calcRelationships(cg->getBlocks());
        
        std::vector<OptimisationPass *> passes;
        passes.push_back(new ResolveConstAddr);
        passes.push_back(new ConstMover);
        passes.push_back(new AddressOfPass);
        passes.push_back(new ThreeToTwoPass);
        passes.push_back(new ConditionalBranchSplitter);
        passes.push_back(new BranchRemover);
        passes.push_back(new SillyRegalloc);
        passes.push_back(new StackSizePass(cg));

        int count = 0;
        for(std::vector<OptimisationPass *>::iterator it = passes.begin();
            it != passes.end(); it++)
        {
            OptimisationPass * op = *it;
            FILE * keep_log = log_file;
            char buf[4096];
            sprintf(buf, "%s_%d_%s.txt",cg->getScope()->name().c_str(),
                    count, op->name().c_str());
            log_file = fopen(buf, "w");
            fprintf(log_file, "Running pass %s, before:\n\n",
                    op->name().c_str());
            dump_codegen(cg);
            op->init(cg);
            op->run();
            fprintf(log_file, "\nAfter\n\n");
            dump_codegen(cg);
            fclose(log_file);
            log_file = keep_log;
            count++;
        }

        for(std::vector<OptimisationPass *>::iterator it = passes.begin();
            it != passes.end(); it++)
        {
            delete *it;
        }
        
    }

    int code_size = 0;
    int macro_size = 0;
    
    for(std::list<Codegen *>::iterator cit = codegens->begin();
        cit != codegens->end(); cit++)
    {
        bool is_macro = (*cit)->getScope()->getType()->isMacro();
        
        while (code_size % 8)
        {
            code_size++;
        }

        while (macro_size % 8)
        {
            macro_size++;
        }
        
        std::vector<BasicBlock *> & bbs = (*cit)->getBlocks();

        int func_size = 0;
        for (unsigned int loopc=0; loopc<bbs.size(); loopc++)
        {
            int siz = assembler->size(bbs[loopc]);
            code_size += siz;
            func_size += siz;
        }

        FunctionScope * fs = (*cit)->getScope();
        if (is_macro)
        {
            macros->addFunction(fs, func_size);
        }
        else
        {
            image->addFunction(fs, func_size);
        }
    }

    printf("Code size is %d bytes\n", code_size);
    printf("Macro size is %d bytes\n", macro_size);
    
    image->setSectionSize(IMAGE_CODE, code_size);
    macros->setSectionSize(IMAGE_CODE, macro_size);
    
    text_base = image->getAddr(IMAGE_CODE);
    text_len = code_size;
        
    uint32_t * fillptr = (uint32_t *)text_base;
    for (int loopc=0; loopc<code_size/4; loopc++)
    {
        *fillptr = 0xdeadbeef;
        fillptr++;
    }
        
    assembler->setMem(image->getMemBlock(IMAGE_CODE));
    assembler->setAddr(image->getAddr(IMAGE_CODE));
        
    for(std::list<Codegen *>::iterator cit = codegens->begin();
        cit != codegens->end(); cit++)
    {
        bool is_macro = (*cit)->getScope()->getType()->isMacro();
        Image * the_image = is_macro ? macros : image;

        FunctionScope * fs = (*cit)->getScope();
        assembler->setAddr(the_image->functionAddress(fs));
        assembler->setPtr(the_image->functionPtr(fs));
        (*cit)->getScope()->setAddr(assembler->currentAddr());
        assembler->newFunction(*cit);
        std::vector<BasicBlock *> & bbs = (*cit)->getBlocks();
        for (unsigned int loopc=0; loopc<bbs.size(); loopc++)
        {
            assembler->assemble(bbs[loopc], 0, the_image);
        }
        
        FILE * keep_log = log_file;
        char buf[4096];
        sprintf(buf, "%s_codegen.txt",(*cit)->getScope()->name().c_str());
        log_file = fopen(buf, "w");
        dump_codegen(*cit);
        fclose(log_file);
        log_file = keep_log;
    }
    
    assembler->applyRelocs();

    image->setSectionSize(IMAGE_DATA, HEAP_SIZE);
    macros->setSectionSize(IMAGE_DATA, HEAP_SIZE);
    fillptr = (uint32_t *)image->getPtr(IMAGE_DATA);
    for (int loopc=0; loopc<HEAP_SIZE/4; loopc++)
    {
        *fillptr = 0xdeadbeef;
        fillptr++;
    }
    
    fillptr = (uint32_t *)macros->getPtr(IMAGE_DATA);
    for (int loopc=0; loopc<HEAP_SIZE/4; loopc++)
    {
        *fillptr = 0xdeadbeef;
        fillptr++;
    }
    
    data_base = image->getAddr(IMAGE_DATA);
    data_len = 4096;
        
    FILE * dump = fopen("out.bin", "w");
    fwrite(image->getPtr(IMAGE_CODE), image->sectionSize(IMAGE_CODE), 1, dump);
    fclose(dump);
    
    dump = fopen("macros.bin", "w");
    fwrite(macros->getPtr(IMAGE_CODE), macros->sectionSize(IMAGE_CODE), 1, dump);
    fclose(dump);
    

    image->finalise();

    fprintf(log_file, "TestFunc is at %lx buf at %lx\n",
            image->getAddr(IMAGE_CODE), image->getAddr(IMAGE_DATA));
    
    fflush(log_file);
    
    root_buf = image->getPtr(IMAGE_DATA);
    
#ifdef POSIX_SIGNALS
    pthread_mutex_lock(&dumper);
    
    struct sigaction sa, oldact;
    sa.sa_sigaction = segv_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_SIGINFO; 
    if (sigaction(SIGSEGV, &sa, &oldact))
    {
        printf("Failure to install segv signal handler!\n");
    }
    if (sigaction(SIGILL, &sa, &oldact))
    {
        printf("Failure to install sigill signal handler!\n");
    }
    if (sigaction(SIGBUS, &sa, &oldact))
    {
        printf("Failure to install sigbus signal handler!\n");
    }
    
    pthread_t tid;
    pthread_create(&tid, 0, dumpstacker, 0);
    pthread_detach(tid); 
#endif

    unsigned char * fptr = image->functionPtr(gc->getScope());
    if (!fptr)
    {
        printf("Can't find root function!\n");
    }
    
    TestFunc tf = (TestFunc)fptr;
    result = tf(image->getAddr(IMAGE_DATA));
    fprintf(log_file, ">>> Result %ld %lx\n", result, result);
    
    fprintf(log_file, "Stack:\n%s\n",
            gc->display(image->getPtr(IMAGE_DATA)).c_str());
    
    delete calling_convention;
    delete cfuncs;
    delete assembler;

    for (std::list<Codegen *>::iterator cdit = codegens->begin();
         cdit != codegens->end(); cdit++)
    {
        delete (*cdit);
    }

    delete codegens;
    delete constants;
    delete root_scope;
    
    destroyTypes();

    delete image;
    delete macros;
    
    fclose(log_file);
    printf("Result: %ld\n", result);
    return (int)result;
}

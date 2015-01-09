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

#ifdef POSIX_SIGNALS
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

pthread_mutex_t dumper = PTHREAD_MUTEX_INITIALIZER;

void * dumpstacker(void *)
{
    printf("Waiting\n");
    pthread_mutex_lock(&dumper);
    dumpstack();
    return 0;
}

void segv_handler(int signo)
{
    printf("Segfault!\n");
    pthread_mutex_unlock(&dumper);
    sleep(100);
    exit(0);
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
        
        if ((!val & 0x80) || (val & 0x40))
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
    root_scope = new FunctionScope(0, "@root", new FunctionType("@root", false));
    
#ifdef WINDOWS_CC
    calling_convention = new Amd64WindowsCallingConvention();
#else
    calling_convention = new Amd64UnixCallingConvention();
#endif
    
    cfuncs = new CFuncs();
    cfuncs->add((uint64_t)dumpstack, "dumpstack");
    
    Lexer lex;
    FILE * f = fopen("test.e", "rb"); 
	bool flange = false;

	if(!f)
	{
		f = fopen("../test.e", "rb");
		flange = true;
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
    
    lex.setFile(flange ? "../test.e" : "test.e");
    lex.lex(input);

    if (errors.size() != 0)
    {
        printf("Lex errors:\n");
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
        printf("Parse errors:\n");
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
        printf("Codegen errors:\n");
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

    unsigned char * constantbuf = new unsigned char[constants->getSize()];
    constants->setAddress((uint64_t)constantbuf);
    fprintf(log_file, "Constant pool is %ld bytes at %p\n",
            constants->getSize(), constantbuf);
    constants->fillPool(constantbuf);
    
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
    
    Mem mem;

    MemBlock mb = mem.getBlock(4096, MEM_READ | MEM_WRITE);

    if (!mb.isNull())
    {
        uint32_t * fillptr = (uint32_t *)mb.ptr;
        for (int loopc=0; loopc<4096/4; loopc++)
        {
            *fillptr = 0xdeadbeef;
            fillptr++;
        }
        
        assembler->setMem(mb);
        assembler->setAddr((uint64_t)mb.ptr);
        
        for(std::list<Codegen *>::iterator cit = codegens->begin();
        cit != codegens->end(); cit++)
        {
			assembler->align(8);
            (*cit)->getScope()->setAddr(assembler->currentAddr());
			assembler->newFunction(*cit);
            std::vector<BasicBlock *> & bbs = (*cit)->getBlocks();
            for (unsigned int loopc=0; loopc<bbs.size(); loopc++)
            {
                assembler->assemble(bbs[loopc], 0);
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
        
        unsigned char * buf = new unsigned char[4096];
        fillptr = (uint32_t *)buf;
        for (int loopc=0; loopc<4096/4; loopc++)
        {
            *fillptr = 0xdeadbeef;
            fillptr++;
        }
        
        FILE * f = fopen("out.bin", "w");
        fwrite(mb.ptr, (size_t)assembler->len(), 1, f);
        fclose(f);
        
        mem.changePerms(mb, MEM_READ | MEM_EXEC);

        fprintf(log_file, "TestFunc is at %p buf at %p\n", mb.ptr, buf);

        fflush(log_file);

        root_buf = buf;
        
#ifdef POSIX_SIGNALS
        pthread_mutex_lock(&dumper);
        signal(SIGSEGV, segv_handler);

        pthread_t tid;
        pthread_create(&tid, 0, dumpstacker, 0);
        pthread_detach(tid); 
#endif

        TestFunc tf = (TestFunc)mb.ptr;
        result = tf((uint64_t)buf);
        fprintf(log_file, ">>> Result %ld %lx\n", result, result);

        fprintf(log_file, "Stack:\n%s\n", gc->display(buf).c_str());
        delete[] buf;
    }
    
    mem.releaseBlock(mb);

    delete calling_convention;
    delete cfuncs;
    delete assembler;

    for (std::list<Codegen *>::iterator cdit = codegens->begin();
         cdit != codegens->end(); cdit++)
    {
        delete (*cdit);
    }

    delete[] constantbuf;
    delete codegens;
    delete constants;
    delete root_scope;
    
    destroyTypes();

    fclose(log_file);
    printf("Result: %ld\n", result);
    return result;
}

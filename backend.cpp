#include "backend.h"
#include "pass.h"
#include "symbols.h"
#include "image.h"
#include "error.h"
#include "entrypoint.h"

extern FILE * log_file;
extern void dump_codegen(Codegen * cg);
extern Constants * constants;
extern FunctionScope * root_scope;
extern Codegen * root_gc;

Configuration * configuration = 0;
std::list<Codegen *> * codegensptr = 0;
#define HEAP_SIZE 4096

Backend::Backend(Configuration * c, Expr * r)
{
    config = c;
    root_expr = r;
    configuration = c;
}

void Backend::process()
{
    codegensptr = &codegens;
    Codegen * gc = new Codegen(root_expr, root_scope);
    codegens.push_back(gc);
        
    root_gc = gc;

    bool jit = false;
    
    if (jit)
    {   
        gc->setCallConvention(CCONV_C);
        BasicBlock * prologue = gc->block();
        config->cconv->generatePrologue(prologue, root_scope);
    }
    else
    {
        gc->setCallConvention(CCONV_RAW);
        gc->block()->add(Insn(MOVE, Operand::reg(config->assembler->framePointer()),
                              Operand::section(IMAGE_DATA, 0)));
        
        Value * v = root_scope->lookupLocal("__activation");
        assert(v);
        gc->block()->add(Insn(MOVE, Operand(v), Operand::reg(config->assembler->framePointer())));
    
        v = root_scope->lookupLocal("__stackptr");
        assert(v);
        gc->block()->add(Insn(MOVE, Operand(v), Operand::reg(config->assembler->framePointer())));
    
        Value * stacksize = new Value("__stacksize", register_type);
        stacksize->setOnStack(true);
        root_scope->add(stacksize);
        gc->block()->add(Insn(GETSTACKSIZE, stacksize));
        gc->block()->add(Insn(ADD, Operand(v), Operand(v), stacksize));
	config->entrypoint->generatePrologue(gc->block(), root_scope);
    }
	
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
        return;
    }
    
    BasicBlock * epilogue = gc->newBlock("epilogue");

    if (jit)
    {
        gc->block()->add(Insn(BRA, epilogue));
        config->cconv->generateEpilogue(epilogue, root_scope);
    }
    else
    {
	config->entrypoint->generateEpilogue(epilogue, root_scope);
    }
    
    config->image->setSectionSize(IMAGE_CONST_DATA, constants->getSize());
    constants->setAddress(config->image->getAddr(IMAGE_CONST_DATA));
    constants->fillPool(config->image->getPtr(IMAGE_CONST_DATA));
     
	std::vector<OptimisationPass *> passes = config->passes;
    
    for(std::list<Codegen *>::iterator cit = codegens.begin();
        cit != codegens.end(); cit++)
    {
        Codegen * cg = *cit;
        cg->allocateStackSlots();
	
        BasicBlock::calcRelationships(cg->getBlocks());
        
	
        int count = 0;
        for(std::vector<OptimisationPass *>::iterator it = passes.begin();
            it != passes.end(); it++)
        {
            OptimisationPass * op = *it;
            FILE * keep_log = log_file;
            char buf[4096];
            sprintf(buf, "%s_%s_%d_%s.txt",config->name.c_str(), cg->getScope()->name().c_str(),
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
 
    }

    int code_size = 0;
    
    for(std::list<Codegen *>::iterator cit = codegens.begin();
        cit != codegens.end(); cit++)
    {
        bool is_macro = (*cit)->getScope()->getType()->isMacro();
	if (is_macro)
	{
  	    printf("Skipping macro %s\n", (*cit)->getScope()->name().c_str());
	    continue;
	}
	
        while (code_size % 8)
        {
            code_size++;
        }
        
        std::vector<BasicBlock *> & bbs = (*cit)->getBlocks();

        int func_size = 0;
        for (unsigned int loopc=0; loopc<bbs.size(); loopc++)
        {
            int siz = config->assembler->size(bbs[loopc]);
            code_size += siz;
            func_size += siz;
        }

	code_size += 8;   // stack size marker at start of normal function
	func_size += 8;
	
        FunctionScope * fs = (*cit)->getScope();
	config->image->addFunction(fs, func_size);
    }

    printf("Code size is %d bytes\n", code_size);
    
    config->image->setSectionSize(IMAGE_CODE, code_size);

    uint64_t * fillptr = 0;
    
    config->assembler->setAddr(config->image->getAddr(IMAGE_CODE));
    config->assembler->setMem(config->image->getPtr(IMAGE_CODE),
			      config->image->getPtr(IMAGE_CODE)+
			      config->image->sectionSize(IMAGE_CODE));
    
    for(std::list<Codegen *>::iterator cit = codegens.begin();
        cit != codegens.end(); cit++)
    {
        FunctionScope * fs = (*cit)->getScope();
        config->assembler->setAddr(config->image->functionAddress(fs));
        config->assembler->setPtr(config->image->functionPtr(fs));
        (*cit)->getScope()->setAddr(config->assembler->currentAddr());
        config->assembler->newFunction(*cit);
        std::vector<BasicBlock *> & bbs = (*cit)->getBlocks();
        for (unsigned int loopc=0; loopc<bbs.size(); loopc++)
        {
            config->assembler->assemble(bbs[loopc], 0, config->image);
        }
        
        FILE * keep_log = log_file;
        char buf[4096];
        sprintf(buf, "%s_%s_codegen.txt",config->name.c_str(),(*cit)->getScope()->name().c_str());
        log_file = fopen(buf, "w");
        dump_codegen(*cit);
        fclose(log_file);
        log_file = keep_log;
    }
    
    config->image->setSectionSize(IMAGE_DATA, HEAP_SIZE);
    fillptr = (uint64_t *)config->image->getPtr(IMAGE_DATA);
    for (int loopc=0; loopc<HEAP_SIZE/8; loopc++)
    {
        *fillptr = 0xdeadbeefdeadbeef;
        fillptr++;
    }
    
    config->image->endOfImports();
    config->image->relocate();
    config->image->setRootFunction(gc->getScope());    
    config->image->finalise();

    char buf[4096];
    sprintf(buf, "%s_out.bin", config->name.c_str());
    FILE * dump = fopen(buf, "w");
    fwrite(config->image->getPtr(IMAGE_CODE), config->image->sectionSize(IMAGE_CODE), 1, dump);
    fclose(dump);
}

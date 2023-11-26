#include "backend.h"
#include "pass.h"
#include "symbols.h"
#include "image.h"
#include "error.h"
#include "entrypoint.h"
#include "rtti.h"
#include "exports.h"
#include "imports.h"

#include <string.h>

extern FILE * log_file;
extern void dump_codegen(Codegen * cg);
extern Constants * constants;
extern FunctionScope * root_scope;
extern Codegen * root_gc;

Configuration * configuration = 0;
std::list<Codegen *> * codegensptr = 0;
#define DATA_SIZE 4096

Backend::Backend(Configuration * c, Expr * r)
{
	config = c;
	root_expr = r;
	configuration = c;
}

Backend::~Backend()
{
    for (std::list<Codegen *>::iterator it = codegens.begin();
         it != codegens.end(); it++)
    {
        delete *it;
    }
}

int Backend::process()
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
        if (config->relocatable)
        {
            gc->setCallConvention(CCONV_STANDARD);
            // Create a fake parent, which is actually the loader's frame
        }
        else
        {
            gc->setCallConvention(CCONV_RAW);
            config->entrypoint->generatePrologue(gc->block(), root_scope, config->image);

            gc->block()->add(Insn(MOVE, Operand::reg(config->assembler->framePointer()),
                                  Operand::section(IMAGE_UNALLOCED_DATA, 0)));

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

            Value * exports_ptr = root_scope->lookupLocal("__exports");
            assert(exports_ptr);
            gc->block()->add(Insn(MOVE, exports_ptr,
                                  Operand::section(IMAGE_EXPORTS, 0)));

            // Will need some 'magic' for Thumb. Maybe prologue puts in hi register which is 'osstackptr' here
            Value * os_stack_ptr = root_scope->lookupLocal("__osstackptr");
            assert(os_stack_ptr);
            gc->block()->add(Insn(MOVE, os_stack_ptr,
                Operand::reg(config->assembler->osStackPointer())));
        }
    }

	BasicBlock * epilogue = gc->newBlock("epilogue");

	BasicBlock * body = gc->newBlock("body");
	gc->setBlock(body);
	gc->generate();

	if (errors.size() != 0)
	{
		printf("Codegen errors:\n\n");
		for (std::list<Error>::iterator it = errors.begin();
		it != errors.end(); it++)
		{
			printf("\n");
			(*it).print();
		}
		return 1;
	}

	if (jit)
	{
		gc->block()->add(Insn(BRA, epilogue));
		gc->setBlock(epilogue);
		config->cconv->generateEpilogue(epilogue, root_scope);
	}
	else
	{
		gc->setBlock(epilogue);
		config->entrypoint->generateEpilogue(epilogue, root_scope, config->image);
	}

	config->image->setSectionSize(IMAGE_CONST_DATA, constants->getSize());
	constants->fillPool(config->image->getPtr(IMAGE_CONST_DATA));

	config->image->setSectionSize(IMAGE_RTTI, rtti->size());
	memcpy(config->image->getPtr(IMAGE_RTTI), rtti->getData(), rtti->size());

    config->image->setSectionSize(IMAGE_EXPORTS, exports->size());
    memcpy(config->image->getPtr(IMAGE_EXPORTS), exports->getData(), exports->size());

	std::vector<OptimisationPass *> passes = config->passes;

	for (std::list<Codegen *>::iterator cit = codegens.begin();
	cit != codegens.end(); cit++)
	{
		Codegen * cg = *cit;
		cg->allocateStackSlots();

		BasicBlock::calcRelationships(cg->getBlocks());

		int count = 0;
		for (std::vector<OptimisationPass *>::iterator it = passes.begin();
		it != passes.end(); it++)
		{
			OptimisationPass * op = *it;
			FILE * keep_log = log_file;
			char buf[4096];
			sprintf(buf, "%s_%s_%d_%s.txt", config->name.c_str(), cg->getScope()->name().c_str(),
				count, op->name().c_str());
			log_file = fopen(buf, "w");
			fprintf(log_file, "Running pass %s, before:\n\n",
				op->name().c_str());
			dump_codegen(cg);
			op->init(cg, config);
			op->run();
			fprintf(log_file, "\nAfter pass %s:\n\n", op->name().c_str());
			dump_codegen(cg);
			fclose(log_file);
			log_file = keep_log;
			count++;
		}

	}

	int code_size = 0;

	for (std::list<Codegen *>::iterator cit = codegens.begin();
	cit != codegens.end(); cit++)
	{
		bool is_macro = (*cit)->getScope()->getType()->isMacro();
		if (is_macro)
		{
			printf("Skipping macro %s\n", (*cit)->getScope()->name().c_str());
			continue;
		}

		while (code_size % config->assembler->functionAlignment())
		{
			code_size++;
		}

		std::vector<BasicBlock *> & bbs = (*cit)->getBlocks();

		int func_size = 0;
		for (unsigned int loopc = 0; loopc < bbs.size(); loopc++)
		{
            bbs[loopc]->setEstimatedAddr(code_size);
			int siz = config->assembler->size(bbs[loopc]);
			code_size += siz;
			func_size += siz;
		}

		code_size += 8;   // stack size marker at start of normal function
		func_size += 8;

		FunctionScope * fs = (*cit)->getScope();
		config->image->addFunction(fs, func_size);
	}

	config->image->setSectionSize(IMAGE_CODE, code_size);

	uint64_t * fillptr = 0;

	config->assembler->setAddr(config->image->getAddr(IMAGE_CODE));
	config->assembler->setMem(config->image->getPtr(IMAGE_CODE),
		config->image->getPtr(IMAGE_CODE) +
		config->image->sectionSize(IMAGE_CODE));

    mtables->generateTables();
    mtables->createSection(config->image, config->assembler);

    imports->finalise();

	for (std::list<Codegen *>::iterator cit = codegens.begin();
	cit != codegens.end(); cit++)
	{
		std::vector<BasicBlock *> & ubbs = (*cit)->getUnplacedBlocks();
		if (ubbs.size() != 0)
		{
			printf("WARNING unplaced blocks:\n");
			for (unsigned int loopc = 0; loopc < ubbs.size(); loopc++)
			{
				printf("%d: %s\n", loopc, ubbs[loopc]->toString().c_str());
			}
			return 1;
		}


		FunctionScope * fs = (*cit)->getScope();
		config->assembler->setAddr(config->image->functionAddress(fs));
		config->assembler->setPtr(config->image->functionPtr(fs));
		(*cit)->getScope()->setAddr(config->assembler->currentAddr());
		config->assembler->newFunction(*cit);
		std::vector<BasicBlock *> & bbs = (*cit)->getBlocks();
		for (unsigned int loopc = 0; loopc < bbs.size(); loopc++)
		{
			config->assembler->assemble(bbs[loopc], 0, config->image);
		}

		FILE * keep_log = log_file;
		char buf[4096];
		sprintf(buf, "%s_%s_codegen.txt", config->name.c_str(), (*cit)->getScope()->name().c_str());
		log_file = fopen(buf, "w");
		dump_codegen(*cit);
		fclose(log_file);
		log_file = keep_log;
	}

    FILE * debug = fopen(config->relocatable ? (
                             config->image->fileName()+"_debug.txt").c_str()
                         : "debug.txt", "w");

    if (config->assembler->littleEndian())
    {
        fputs("endian little\n", debug);
    }
    else
    {
        fputs("endian big\n", debug);
    }

    char buf[4096];
    sprintf(buf, "arch %d\n", config->assembler->arch());
    fputs(buf, debug);

    sprintf(buf, "textbase %ld\n", config->image->getAddr(IMAGE_CODE));
    fputs(buf, debug);

    auto & typelist = types->get();
    for (auto it = typelist.begin(); it != typelist.end();
         it++)
    {
        char buf[4096];
        sprintf(buf, "type %u %s %u\n", (unsigned int)(*it).second->classId(),
                (*it).first.c_str(), (unsigned int)(*it).second->size());
        fputs(buf, debug);
    }

	for (std::list<Codegen *>::iterator cit = codegens.begin();
         cit != codegens.end(); cit++)
	{
        FunctionScope * fs = (*cit)->getScope();
        char buf[4096];
        sprintf(buf, "function %s %u %u\n", fs->name().c_str(),
                (unsigned int)config->image->functionAddress(fs),
                (unsigned int)config->image->functionSize(fs));
        fputs(buf, debug);
        std::vector<Value *> locals = (*cit)->getLocals();
        for (unsigned int loopc = 0; loopc<locals.size(); loopc++)
        {
            Value * v = locals[loopc];
            sprintf(buf, "local %s %u %u\n", v->name.c_str(),
                    (unsigned int)(v->type ? v->type->classId() : 0),
                    (unsigned int)v->stackOffset());
            fputs(buf, debug);
        }
    }
    fclose(debug);

	config->image->setSectionSize(IMAGE_DATA, DATA_SIZE);
	fillptr = (uint64_t *)config->image->getPtr(IMAGE_DATA);
	for (int loopc = 0; loopc < DATA_SIZE / 8; loopc++)
	{
		*fillptr = 0xdeadbeefdeadbeefLL;
		fillptr++;
	}

	config->image->endOfImports();
	config->image->relocate(config->relocatable);
	config->image->setRootFunction(gc->getScope());
	config->image->finalise();

    std::string fname = config->name+"_out.bin";
	FILE * dump = fopen(fname.c_str(), "wb");
	fwrite(config->image->getPtr(IMAGE_CODE), config->image->sectionSize(IMAGE_CODE), 1, dump);
	fclose(dump);

	return 0;
}

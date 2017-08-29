#ifndef _PASS_
#define _PASS_

/*
	All the various optimisation/code transformation passes.
	Currently more the latter than the former since optimisation
	is a low priority for me right now. E.g. does things like
	convert:

	a = b + c
	
	to

	a = b
	a += c

	to humour x86 with its generally two-operand instruction format,
	and of course basic register allocation.
*/

#include "codegen.h"
#include "regset.h"
#include "component.h"
#include "configfile.h"

class OptimisationPass : public Component
{
public:

	OptimisationPass();
	virtual ~OptimisationPass() {}

	virtual void processInsn() {}
	virtual void beginBlock() {}
	virtual void endBlock() {}

	virtual std::string name()
	{
		return "<null>";
	}

	void run();

	void prepend(Insn);
	void append(Insn);
	void change(Insn);
	void removeInsn();

	virtual void init(Codegen *, Configuration *);

protected:

	Codegen * cg;
    Configuration * config;
    
	BasicBlock * block;
	BasicBlock * next_block;
	Insn insn;

	std::list<Insn> to_append;
	std::vector<BasicBlock *>::iterator bit;
	std::list<Insn>::iterator iit;

};

class ThreeToTwoPass : public OptimisationPass
{
public:

	ThreeToTwoPass()
		: OptimisationPass()
	{}

	virtual std::string name()
	{
		return "ThreeToTwo";
	}

	virtual void processInsn();

};

#define MAXREGS 256

class SillyRegalloc : public OptimisationPass
{
public:

	SillyRegalloc()
		: OptimisationPass()
	{
        regs = 0;
        input = 0;
        output = 0;
    }

    ~SillyRegalloc()
    {
        delete[] regs;
        delete[] input;
        delete[] output;
    }
    
	virtual void processInsn();
	virtual void init(Codegen *, Configuration *);

	virtual std::string name()
	{
		return "SillyRegalloc";
	}

protected:

	int alloc(Value *, RegSet &, RegSet &);
	int findFree(RegSet &, RegSet &);

	Value ** regs;
	bool * input;
	bool * output;
    int numregs;
    
};

class ConditionalBranchSplitter : public OptimisationPass
{
public:

	ConditionalBranchSplitter()
		: OptimisationPass()
	{}

	virtual std::string name()
	{
		return "ConditionalBranchSplitter";
	}

	virtual void processInsn();

};

class BranchRemover : public OptimisationPass
{
public:

	BranchRemover()
		: OptimisationPass()
	{}

	virtual std::string name()
	{
		return "BranchRemover";
	}

	virtual void processInsn();

};

class AddressOfPass : public OptimisationPass
{
public:

	AddressOfPass()
		: OptimisationPass()
	{}

	virtual std::string name()
	{
		return "AddressOf";
	}

	virtual void processInsn();

};

class ConstMover : public OptimisationPass
{
public:

	ConstMover()
		: OptimisationPass()
	{
        const_temporary = new Value *[3];
        const_temporary[0] = 0;
        const_temporary[1] = 0;
        const_temporary[2] = 0;
    }

    ~ConstMover()
    {
        delete[] const_temporary;
    }
    
	virtual std::string name()
	{
		return "ConstMover";
	}

	virtual void processInsn();

	virtual void init(Codegen * cg, Configuration * cf)
    {
        OptimisationPass::init(cg, cf);
        const_temporary[0] = 0;
        const_temporary[1] = 0;
        const_temporary[2] = 0;
    }
    
  protected:

    Value ** const_temporary;
    
};

class ResolveConstAddr : public OptimisationPass
{
public:

	ResolveConstAddr()
		: OptimisationPass()
	{}

	virtual std::string name()
	{
		return "ResolveConstAddr";
	}

	virtual void processInsn();

};

class StackSizePass : public OptimisationPass
{
public:

	StackSizePass()
		: OptimisationPass()
	{
	}

	virtual std::string name()
	{
		return "StackSizePass";
	}

	virtual void processInsn();

};

class BitSizePass : public OptimisationPass
{
public:

	BitSizePass()
		: OptimisationPass()
	{
	}

	virtual std::string name()
	{
		return "BitSizePass";
	}

	virtual void processInsn();

};

class RemWithDivPass : public OptimisationPass
{
public:

	RemWithDivPass()
		: OptimisationPass()
	{
	}

	virtual std::string name()
	{
		return "RemWithDivPass";
	}

	virtual void processInsn();

};

// turn mov <hi>, something into mov <lo>, something ; mov <hi> <lo>
class ThumbMoveConstantPass : public OptimisationPass
{
  public:

    ThumbMoveConstantPass()
        : OptimisationPass()
    {
    }

    virtual std::string name()
    {
        return "ThumbMoveConstantPass";
    }

    virtual void processInsn();
    
};

#endif

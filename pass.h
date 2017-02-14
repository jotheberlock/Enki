#ifndef _PASS_
#define _PASS_

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
	{}

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
	{}

	virtual std::string name()
	{
		return "ConstMover";
	}

	virtual void processInsn();

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

class AdjustRegisterBasePass : public OptimisationPass
{

  public:

    AdjustRegisterBasePass()
        : OptimisationPass()
    {
        current_adjustments = 0;
    }

    virtual ~AdjustRegisterBasePass()
    {
        delete[] current_adjustments;
    }
    
    virtual std::string name()
    {
        return "AdjustRegisterBasePass";
    }

	virtual void init(Codegen *, Configuration *);

    virtual void processInsn();
	virtual void beginBlock();
	virtual void endBlock();

  protected:

    void flush();
    void flushOne(int);
    
    int * current_adjustments;
    
};

#endif

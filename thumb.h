#ifndef _THUMB_
#define _THUMB_

/*
Converts intermediate code into Thumb(1)
Aimed at the Cortex M0 as a baseline
*/

#include "asm.h"

class Thumb : public Assembler
{
public:

	Thumb()
	{
		psize = 32;
	}

    int arch()
    {
        return ARCH_ARMTHUMB;
    }
    
	int regnum(std::string);
    int numRegs()
    {
        return 16;
    }
    
	int size(BasicBlock *);
	bool assemble(BasicBlock *, BasicBlock *, Image *);
	std::string transReg(uint32);
	ValidRegs validRegs(Insn &);

	int framePointer()
	{
		return 13;  // Thumb has special support for SP-relative addressing, so let's use that
	}

	int functionAlignment()
	{
		return 2;
	}

	bool validConst(Insn & i, int idx);

	virtual void newFunction(Codegen *);
	virtual void align(uint64 a);

	virtual bool configure(std::string, std::string);

    virtual bool validRegOffset(Insn &, int);
    
protected:

	bool calcImm(uint64 raw, uint32 & result);

};

class ThumbLinuxSyscallCallingConvention : public CallingConvention
{
public:

	virtual void generatePrologue(BasicBlock *, FunctionScope *)
	{}

	virtual void generateEpilogue(BasicBlock *, FunctionScope *)
	{}

	virtual Value * generateCall(Codegen *, Value *, std::vector<Value *> &);

};

#endif

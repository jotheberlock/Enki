#ifndef _ARM_
#define _ARM_

/*
Converts intermediate code into 32-bit ARM machinecode.
Assumes ARMv7 or better because it assumes movw/movt is available.
*/

#include "asm.h"

class Arm32 : public Assembler
{
public:

	Arm32()
	{
		psize = 32;
	}

    int arch()
    {
        return ARCH_ARM32;
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
		return 12;
	}

    int osStackPointer()
    {
        return 13;
    }

	int functionAlignment()
	{
		return 8;
	}

	bool validConst(Insn & i, int idx);

	virtual void newFunction(Codegen *);
	virtual void align(uint64 a);

	virtual bool configure(std::string, std::string);

    virtual bool validRegOffset(Insn &, int);
    
protected:

	bool calcImm(uint64 raw, uint32 & result);

};

class ArmLinuxSyscallCallingConvention : public CallingConvention
{
public:

	virtual void generatePrologue(BasicBlock *, FunctionScope *)
	{}

	virtual void generateEpilogue(BasicBlock *, FunctionScope *)
	{}

	virtual Value * generateCall(Codegen *, Value *, std::vector<Value *> &);

};

#endif

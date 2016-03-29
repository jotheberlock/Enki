#ifndef _ARM_
#define _ARM_

#include "asm.h"

class Arm : public Assembler
{
  public:

    Arm()
    {
        psize=64;
    }

    int regnum(std::string);
    int size(BasicBlock *);
    bool assemble(BasicBlock *, BasicBlock *, Image *);
    std::string transReg(uint32_t);
    ValidRegs validRegs(Insn &);
    
    int framePointer()
    {
        return 12;
    }
    
    int functionAlignment()
    {
        return 8;
    }
    
    bool validConst(Insn & i, int idx);

    virtual void newFunction(Codegen *);
	virtual void align(uint64_t a);
    
  protected:

    uint32_t calcImm(uint64_t raw);
  
};

class ArmUnixSyscallCallingConvention : public CallingConvention
{
  public:
    
    virtual void generatePrologue(BasicBlock *, FunctionScope *) 
    {}

    virtual void generateEpilogue(BasicBlock *, FunctionScope *)
    {}

    virtual Value * generateCall(Codegen *, Value *, std::vector<Value *> &);
    
};

#endif

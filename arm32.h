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
    std::string transReg(uint32_t);
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

    bool validConst(Insn &i, int idx);

    virtual void newFunction(Codegen *);
    virtual void align(uint64_t a);

    virtual bool configure(std::string, std::string) override;

    virtual bool validRegOffset(Insn &, int);

  protected:
    bool calcImm(uint64_t raw, uint32_t &result);
};

class Arm32LinuxSyscallCallingConvention : public CallingConvention
{
  public:
    virtual void generatePrologue(BasicBlock *, FunctionScope *) override
    {
    }

    virtual void generateEpilogue(BasicBlock *, FunctionScope *) override
    {
    }

    virtual Value *generateCall(Codegen *, Value *, std::vector<Value *> &) override;
};

#endif

#ifndef _ARM64_
#define _ARM64_

/*
Converts intermediate code into 64-bit ARM machinecode.
*/

#include "asm.h"

class Arm64 : public Assembler
{
  public:
    Arm64()
    {
        psize = 64;
    }

    int arch()
    {
        return ARCH_ARM64;
    }

    int regnum(std::string);
    int numRegs()
    {
        return 32;
    }

    int size(BasicBlock *);
    bool assemble(BasicBlock *, BasicBlock *, Image *);
    std::string transReg(uint32_t);
    ValidRegs validRegs(Insn &);

    int framePointer()
    {
        return 29;
    }

    int osStackPointer()
    {
        return 31;
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

class Arm64LinuxSyscallCallingConvention : public CallingConvention
{
  public:
    virtual void generatePrologue(BasicBlock *, FunctionScope *)
    {
    }

    virtual void generateEpilogue(BasicBlock *, FunctionScope *)
    {
    }

    virtual Value *generateCall(Codegen *, Value *, std::vector<Value *> &);
};

#endif

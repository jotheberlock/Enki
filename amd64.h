#ifndef _AMD64_
#define _AMD64_

/*
    Converts intermediate code into amd64 (x86-64)) machinecode.
*/

#include "asm.h"

class Amd64 : public Assembler
{
  public:
    Amd64()
    {
        psize = 64;
    }

    int arch()
    {
        return ARCH_AMD64;
    }

    int regnum(std::string);
    int size(BasicBlock *);
    bool assemble(BasicBlock *, BasicBlock *, Image *);
    std::string transReg(uint32_t);

    int numRegs()
    {
        return 16;
    }

    ValidRegs validRegs(Insn &);
    int framePointer()
    {
        return 15;
    }

    int osStackPointer()
    {
        return 4;
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
    // Is it r1, r1, r2 or r1, r1, const?
    bool isRRC(Insn &);
};

class Amd64WindowsCallingConvention : public CallingConvention
{
  public:
    virtual void generatePrologue(BasicBlock *, FunctionScope *);
    virtual void generateEpilogue(BasicBlock *, FunctionScope *);
    virtual Value *generateCall(Codegen *, Value *, std::vector<Value *> &);

  protected:
    Value *addRegStore(const char *rname, BasicBlock *b, FunctionScope *f);

    Value *rbx_backup;
    Value *rbp_backup;
    Value *rdi_backup;
    Value *rsi_backup;
    Value *rsp_backup;
    Value *r12_backup;
    Value *r13_backup;
    Value *r14_backup;
    Value *r15_backup;
};

class Amd64UnixSyscallCallingConvention : public CallingConvention
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

class Amd64UnixCallingConvention : public CallingConvention
{
  public:
    virtual void generatePrologue(BasicBlock *, FunctionScope *) override;
    virtual void generateEpilogue(BasicBlock *, FunctionScope *) override;
    virtual Value *generateCall(Codegen *, Value *, std::vector<Value *> &) override;

  protected:
    Value *addRegStore(const char *rname, BasicBlock *b, FunctionScope *f);

    Value *rbx_backup;
    Value *rbp_backup;
    Value *r12_backup;
    Value *r13_backup;
    Value *r14_backup;
    Value *r15_backup;
};

#endif

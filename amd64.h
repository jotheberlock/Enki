#ifndef _AMD64_
#define _AMD64_

#include "asm.h"

class Amd64 : public Assembler
{
  public:

    Amd64()
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
        return 15;
    }

    bool validConst(Insn & i, int idx)
    {
        if (i.ins == DIV || i.ins == IDIV || i.ins == REM ||
            i.ins == IREM || i.ins == SELEQ || i.ins == SELGT ||
            i.ins == SELGE || i.ins == SELGTS || i.ins == SELGES ||
            i.ins == MUL || i.ins == IMUL)
        {
            return false;
        }
        
        if (i.ins == ADD || i.ins == SUB || i.ins == MUL
            || i.ins == IMUL || i.ins == AND || i.ins == OR
            || i.ins == XOR || i.ins == SHL || i.ins == SHR)
        {
            if (idx != 2)
            {
                return false;
            }
        }

        if (i.ins == STORE)
        {
            if (idx == 2)
            {
                return false;
            }
        }
        
        return true;
    }
    
	virtual void newFunction(Codegen *);
	virtual void align(uint64_t a);

  protected:

        // Is it r1, r1, r2 or r1, r1, const?
    bool isRRC(Insn &);
        
};

class Amd64WindowsCallingConvention : public CallingConvention
{
  public:

    virtual void generatePrologue(BasicBlock *, FunctionScope *);
    virtual void generateEpilogue(BasicBlock *, FunctionScope *);
    virtual Value * generateCall(Codegen *, Value *, std::vector<Value *> &);
    
  protected:

    Value * addRegStore(const char * rname, BasicBlock * b, FunctionScope * f);
        
    Value * rbx_backup;
    Value * rbp_backup;
    Value * rdi_backup;
    Value * rsi_backup;
    Value * rsp_backup;
    Value * r12_backup;
    Value * r13_backup;
    Value * r14_backup;
    Value * r15_backup;
    
};

class Amd64UnixSyscallCallingConvention : public CallingConvention
{
  public:
    
    virtual void generatePrologue(BasicBlock *, FunctionScope *) 
    {}

    virtual void generateEpilogue(BasicBlock *, FunctionScope *)
    {}

    virtual Value * generateCall(Codegen *, Value *, std::vector<Value *> &);
    
};

    
class Amd64UnixCallingConvention : public CallingConvention
{
  public:

    virtual void generatePrologue(BasicBlock *, FunctionScope *);
    virtual void generateEpilogue(BasicBlock *, FunctionScope *);
    virtual Value * generateCall(Codegen *, Value *, std::vector<Value *> &);
    
  protected:

    Value * addRegStore(const char * rname, BasicBlock * b, FunctionScope * f);
        
    Value * rbx_backup;
    Value * rbp_backup;
    Value * r12_backup;
    Value * r13_backup;
    Value * r14_backup;
    Value * r15_backup;
    
};

#endif

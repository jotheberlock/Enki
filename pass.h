#ifndef _PASS_
#define _PASS_

#include "codegen.h"
#include "regset.h"
#include "component.h"

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
    
    virtual void init(Codegen *);
    
  protected:

    Codegen * cg;
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

class SillyRegalloc : public OptimisationPass
{
  public:

    SillyRegalloc()
        : OptimisationPass()
    {}
    
    virtual void processInsn();
    virtual void init(Codegen *);

    virtual std::string name()
    {
        return "SillyRegalloc";
    }
    
  protected:

    int alloc(Value *, RegSet &, RegSet &);
    int findFree(RegSet &, RegSet &);
    
    Value * regs[256];
    bool input[256];
    bool output[256];
    
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

class StackSizePass :  public OptimisationPass
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

#endif

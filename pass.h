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
#include "component.h"
#include "configfile.h"
#include "regset.h"

class OptimisationPass : public Component
{
  public:
    OptimisationPass();
    virtual ~OptimisationPass()
    {
    }

    virtual void processInsn()
    {
    }
    virtual void beginBlock()
    {
    }
    virtual void endBlock()
    {
    }

    virtual std::string name()
    {
        return "<null>";
    }

    void run();

    void prepend(Insn);
    void append(Insn);
    void change(Insn);
    void removeInsn();
    void moveInsn(std::list<Insn>::iterator &it);

    virtual void init(Codegen *, Configuration *);

  protected:
    Codegen *cg;
    Configuration *config;

    BasicBlock *block;
    BasicBlock *next_block;
    Insn insn;

    std::list<Insn> to_append;
    std::vector<BasicBlock *>::iterator bit;
    std::list<Insn>::iterator iit;
};

class ThreeToTwoPass : public OptimisationPass
{
  public:
    ThreeToTwoPass() : OptimisationPass()
    {
    }

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
    SillyRegalloc() : OptimisationPass()
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
    void handleInstruction(std::list<Insn>::iterator &);
    int alloc(Value *, RegSet &, RegSet &);
    int findFree(RegSet &, RegSet &);

    Value **regs;
    bool *input;
    bool *output;
    int numregs;
};

class ConditionalBranchSplitter : public OptimisationPass
{
  public:
    ConditionalBranchSplitter() : OptimisationPass()
    {
    }

    virtual std::string name()
    {
        return "ConditionalBranchSplitter";
    }

    virtual void processInsn();
};

class BranchRemover : public OptimisationPass
{
  public:
    BranchRemover() : OptimisationPass()
    {
    }

    virtual std::string name()
    {
        return "BranchRemover";
    }

    virtual void processInsn();
};

class AddressOfPass : public OptimisationPass
{
  public:
    AddressOfPass() : OptimisationPass()
    {
    }

    virtual std::string name()
    {
        return "AddressOf";
    }

    virtual void processInsn();
};

class ConstMover : public OptimisationPass
{
  public:
    ConstMover() : OptimisationPass()
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

    virtual void init(Codegen *cg, Configuration *cf)
    {
        OptimisationPass::init(cg, cf);
        const_temporary[0] = 0;
        const_temporary[1] = 0;
        const_temporary[2] = 0;
    }

  protected:
    Value **const_temporary;
};

class ResolveConstAddr : public OptimisationPass
{
  public:
    ResolveConstAddr() : OptimisationPass()
    {
    }

    virtual std::string name()
    {
        return "ResolveConstAddr";
    }

    virtual void processInsn();
};

class StackSizePass : public OptimisationPass
{
  public:
    StackSizePass() : OptimisationPass()
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
    BitSizePass() : OptimisationPass()
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
    RemWithDivPass() : OptimisationPass()
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
    ThumbMoveConstantPass() : OptimisationPass()
    {
    }

    virtual std::string name()
    {
        return "ThumbMoveConstantPass";
    }

    virtual void processInsn();
};

// load r0, sp+256 -> mov foo, sp; add foo, foo, 256; load r0, foo
class StackRegisterOffsetPass : public OptimisationPass
{
  public:
    StackRegisterOffsetPass() : OptimisationPass()
    {
    }

    virtual std::string name()
    {
        return "StackRegisterOffsetPass";
    }

    virtual void processInsn();
};

// load r8, r8 -> move r7, r8 ; load r7, r7  ; move r8, r7
class ThumbHighRegisterPass : public OptimisationPass
{

  public:
    ThumbHighRegisterPass() : OptimisationPass()
    {
    }

    virtual std::string name()
    {
        return "ThumbHighRegisterPass";
    }

    virtual void processInsn();
};

// make sure cmp immediately precedes conditional branch/select
class CmpMover : public OptimisationPass
{
  public:
    CmpMover() : OptimisationPass()
    {
    }

    virtual std::string name()
    {
        return "CmpMover";
    }

    virtual void processInsn();
};

// fix up conditional branches that might exceed the maximum encodable displacement
class ConditionalBranchExtender : public OptimisationPass
{
  public:
    ConditionalBranchExtender() : OptimisationPass()
    {
    }

    virtual std::string name()
    {
        return "ConditionalBranchExtender";
    }

    virtual void processInsn();
};

// as a hack, rewrite 64-bit load/stores to 32-bit. Only works for little
// endian targets...

class Convert64to32 : public OptimisationPass
{
  public:
  public:
    Convert64to32() : OptimisationPass()
    {
    }

    virtual std::string name()
    {
        return "Convert64to32";
    }

    virtual void processInsn();
};

class AddSplitter : public OptimisationPass
{
  public:
  public:
    AddSplitter() : OptimisationPass()
    {
    }

    virtual std::string name()
    {
        return "AddSplitter";
    }

    virtual void processInsn();
};

#endif

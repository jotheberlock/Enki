#ifndef _ASM_
#define _ASM_

#include <string>
#include <vector>
#include <list>
#include <stdint.h>
#include <stdio.h>

#include "mem.h"
#include "regset.h"
#include "component.h"

#define LOAD 1
#define STORE 2
#define ADD 3
#define MOVE 4
#define GETADDR 5
#define MUL 6
#define LOAD8 7
#define STORE8 8
#define GETCONSTADDR 9
#define CMP 10
#define SYSCALL 15
#define RET 16
#define LOAD16 17
#define STORE16 18
#define LOAD32 19
#define STORE32 20
#define LOAD64 21
#define STORE64 22
#define LOADS8 23
#define LOADS16 24
#define LOADS32 25
#define SUB 26
#define AND 27
#define OR 28
#define XOR 29
#define NOT 30
#define SHL 31
#define SHR 32
#define SAL SHL
#define SAR 34
#define RCL 35
#define RCR 36
#define ROL 37
#define ROR 38
#define IMUL 39

#define BEQ 11
#define BNE 12
#define BRA 14
#define BG 40
#define BLE 41
#define BL 42
#define BGE 43
#define DIV 44
#define IDIV 45
#define NOP 46
#define REM 47
#define IREM 48

#define BREAKP 49
#define CALL 50

// (cond) ? a = b : a = c
#define SELEQ 51
#define SELGE 53
#define SELGT 54
// signed versions
#define SELGES 55
#define SELGTS 56

#define GETSTACKSIZE 57

class Value;
class BasicBlock;
class FunctionScope;

class Assembler;

class Operand
{
  public:

    Operand()
    {
        type=0;
    }
    
    Operand(Value *);
    Operand(BasicBlock *);
    Operand(FunctionScope *);

    uint32_t getReg();
    uint64_t getUsigc();
    int64_t getSigc();
    Value * getValue();
    BasicBlock * getBlock();
    FunctionScope * getFunction();
    uint64_t getSection(int &);   // e.g. 'data segment address'
    std::string getExtFunction();
    
    static Operand sigc(int64_t);
    static Operand usigc(uint64_t);
    static Operand reg(int32_t);
    static Operand reg(std::string);
    static Operand section(int, uint64_t);
    static Operand extFunction(std::string);
    
    bool eq(Operand &);
    
    std::string toString();

    bool isUsigc();
    bool isSigc();
    bool isReg();
    bool isValue();
    bool isBlock();
    bool isFunction();
    bool isSection();
    bool isExtFunction();
    
  protected:

    int type;
    union
    {
        Value * v;
        uint64_t c;
        int64_t sc;
        BasicBlock * b;
        uint32_t r;
        FunctionScope * f;
        uint64_t s;
        std::string * e;
    } contents;
};

class Insn
{
  public:

    Insn()
    {
        ins=NOP;
        oc=0;
        addr=0;
    }
    
    Insn(uint64_t i)
    {
        ins=i;
        oc=0;
        addr=0;
    }

    Insn(uint64_t i, Operand o1)
    {
        ins=i;
        ops[0]=o1;
        oc=1;
        addr=0;
    }
    
    Insn(uint64_t i, Operand o1, Operand o2)
    {
        ins=i;
        ops[0]=o1;
        ops[1]=o2;
        oc=2;
        addr=0;
    }
    
    Insn(uint64_t i, Operand o1, Operand o2, Operand o3)
    {
        ins=i;
        ops[0]=o1;
        ops[1]=o2;
        ops[2]=o3;
        oc=3;
        addr=0;
    }

    std::string toString();  // Full instruction with operands
    std::string insToString();

    bool isIn(int);
    bool isOut(int);
    
    uint64_t ins;
    uint64_t addr;
    int oc;
    Operand ops[4];
    std::string comment;
    
};

class Type;
int storeForType(Type *);
int loadForType(Type *);

class BasicBlock
{
  public:

    BasicBlock(std::string n)
    {
        nam=n;
        addr=0;
    }
    
    std::string name()
    {
        return nam;
    }

    void addChild(BasicBlock * b)
    {
        for (unsigned int loopc=0; loopc<children.size(); loopc++)
        {
            if (children[loopc] == b)
            {
                return;
            }            
        }
        
        children.push_back(b);
        b->parents.push_back(this);
    }
    
    void add(Insn i)
    {
        insns.push_back(i);
    }
    
    std::string toString();

    void setCode(std::list<Insn> ins)
    {
        insns = ins;
    }
    
    void append(BasicBlock & b)
    {
        std::list<Insn>::iterator it;
        for (it=b.insns.begin(); it!=b.insns.end(); it++)
        {
            insns.push_back(*it);
        }
    }

    std::vector<BasicBlock *> & getChildren()
    {
        return children;
    }

    std::vector<BasicBlock *> getParents()
    {
        return parents;
    }

    static void calcRelationships(std::vector<BasicBlock *> & blocks);
    
    std::list<Insn> & getCode()
    {
        return insns;
    }

    bool hasAddr()
    {
        return (addr != 0);
    }

    uint64_t getAddr()
    {
        return addr;
    }

    void setAddr(uint64_t a)
    {
        addr=a;
    }

    RegSet getReservedRegs()
    {
        return reserved_regs;
    }

    void setReservedRegs(RegSet r)
    {
        reserved_regs = r;
    }
    
  protected:

    std::vector<BasicBlock *> children;
    std::vector<BasicBlock *> parents;
    std::list<Insn> insns;
    std::string nam;
    uint64_t addr;
    RegSet reserved_regs;
    
};

class ValidRegs
{
  public:
    
    RegSet ops[3];
    RegSet clobbers;
};

class Function;
class Funcall;
class Codegen;
class Image;

class Assembler : public Component
{
  public:

    Assembler()
    {
        current=0;
        address=0;
        limit=0;
        psize=0;
        base=0;
    }

    virtual ~Assembler()
    {
    }
    
    void setMem(unsigned char * b, unsigned char * l)
    {
        base = b;
        limit = l;
    }

    void setPtr(unsigned char * ptr)
    {
        current = ptr;
    }
    
    int staticLinkOffset()
    {
        return (pointerSize() * 3) / 8;
    }
    
    int returnOffset()
    {
        return (pointerSize() * 4) / 8;
    }

    virtual int regnum(std::string) = 0;
    virtual int size(BasicBlock *) = 0;  // Size in bytes of machine code
    virtual bool assemble(BasicBlock *, BasicBlock * next, Image * image) = 0;
    virtual std::string transReg(uint32_t) = 0;
    virtual ValidRegs validRegs(Insn &) = 0;
    virtual bool validConst(Insn &, int) = 0;
    virtual int framePointer() = 0;
    int pointerSize()  // in bits
    {
        assert(psize != 0);
        return psize;
    }
    
    uint64_t len() { return current-base; }

    uint64_t currentAddr()
    {
        return address+len();
    }
    
    void setAddr(uint64_t a)
    {
        address=a;
    }
    
    virtual void align(uint64_t a) = 0;  // Pads with NOPs
    virtual void newFunction(Codegen *);
    
  protected:

    unsigned char * base;
    unsigned char * current;
    unsigned char * limit;
    uint64_t address;
    int psize;
	FunctionScope * current_function;
    
};

class CallingConvention : public Component
{ 
  public:

    virtual ~CallingConvention()
    {
    }
    
    virtual void generatePrologue(BasicBlock *, FunctionScope *) = 0;
    virtual void generateEpilogue(BasicBlock *, FunctionScope *) = 0;
    virtual Value * generateCall(Codegen *, Value *,
                                 std::vector<Value *> &) = 0;
    
};

extern Assembler * assembler;
extern CallingConvention * calling_convention;

#endif

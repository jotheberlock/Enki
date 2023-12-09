#ifndef _CODEGEN_
#define _CODEGEN_

/* Tne name 'Codegen' below is a bit misleading and should probably refactored. A Codegen object holds the basic blocks
   associated with a single function and keeps track of its local variables. AST classes call it to get basic blocks
   which they fill at the code generation stage. Values represent variables/constants/integers and have a name (which
   may be automatically generated in the case of temporaries) and a type. A null type pointer indicates this particular
   Value is an integer. */

#include "asm.h"
#include "ast.h"
#include "type.h"
#include <algorithm>
#include <assert.h>
#include <stack>
#include <vector>

class Value
{
  public:
    Value(std::string n, Type *t)
    {
        name = n;
        type = t;
        is_number = false;
        stack_offset = 0;
        on_stack = false;
        is_const = false;
    }

    Value()
    {
        type = 0;
        is_number = false;
        stack_offset = 0;
        on_stack = false;
        is_const = false;
    }

    Value(uint64_t v)
    {
        type = 0;
        val = v;
        is_number = true;
        stack_offset = 0;
        on_stack = false;
        is_const = true;
    }

    Value(Value *c)
    {
        *this = *c;
    }

    bool onStack()
    {
        return on_stack;
    }

    void setOnStack(bool b)
    {
        on_stack = b;
    }

    uint64_t stackOffset()
    {
        return stack_offset;
    }

    void setStackOffset(uint64_t o)
    {
        stack_offset = o;
    }

    bool isConst()
    {
        return is_const;
    }

    void setConst(bool c)
    {
        is_const = c;
    }

    uint64_t stack_offset;

    bool is_number;
    uint64_t val;

    std::string name;
    Type *type;
    bool on_stack;
    bool is_const;
};

class Constant
{
  public:
    uint64_t offset;
    char *data;
    int len;
};

class Constants
{
  public:
    Constants()
    {
        constantp = 0;
    }

    ~Constants();

    uint64_t addConstant(const char *data, int len, int align);
    uint64_t lookupOffset(uint64_t idx)
    {
        return constants[idx].offset;
    }

    uint64_t getSize()
    {
        return constantp;
    }

    void fillPool(unsigned char *);

  protected:
    std::vector<Constant> constants;
    uint64_t constantp;
};

/* Calling conventions - mostly relevant right now on Windows where
   the only way to get things done is to call functions in e.g. ntdll.dll which
   have the standard Microsoft calling convention. Raw means 'no calling convention'
   and is intended for things like the executable's entry point. */

#define CCONV_STANDARD 0
#define CCONV_C 1
#define CCONV_RAW 2
#define CCONV_MACRO 3

class Codegen
{
  public:
    Codegen(Expr *, FunctionScope *);
    ~Codegen();

    Codegen *copy();

    void generate();

    FunctionScope *getScope()
    {
        return scope;
    }

    Value *getRet()
    {
        return retvar;
    }

    Value *getIp()
    {
        return ipvar;
    }

    Value *getStaticLink()
    {
        return staticlink;
    }

    int callConvention()
    {
        return cconv;
    }

    void setCallConvention(int c)
    {
        cconv = c;
    }

    void allocateStackSlots();
    uint64_t stackSize()
    {
        assert(stack_size != 0xdeadbeef);
        return stack_size;
    }

    Value *getTemporary(Type *t, std::string n = "")
    {
        if (allocated_slots)
        {
            char buf[4096];
            if (n == "")
            {
                sprintf(buf, "@latetemp%d", count);
            }
            else
            {
                sprintf(buf, "@late_%s_%d", n.c_str(), count);
            }

            Value *ret = new Value(buf, t);
            count++;
            locals.push_back(ret);

            int alignment = t->align() / 8;
            while (stack_size % alignment)
            {
                stack_size++;
            }

            ret->setStackOffset(stack_size);
            stack_size += ret->type->size() / 8;
            return ret;
        }
        else
        {
            char buf[4096];
            if (n == "")
            {
                sprintf(buf, "@temp%d", count);
            }
            else
            {
                sprintf(buf, "@%s_%d", n.c_str(), count);
            }
            Value *ret = new Value(buf, t);
            ret->setOnStack(true);
            count++;
            locals.push_back(ret);
            // addScopeEntry(ret);
            return ret;
        }
    }

    BasicBlock *block()
    {
        return current_block;
    }

    void setBlock(BasicBlock *b)
    {
        current_block = b;
        blocks.push_back(b);
        std::vector<BasicBlock *>::iterator it = std::find(unplaced_blocks.begin(), unplaced_blocks.end(), b);
        if (it != unplaced_blocks.end())
        {
            unplaced_blocks.erase(it);
        }
    }

    std::vector<BasicBlock *> &getBlocks()
    {
        return blocks;
    }

    std::vector<BasicBlock *> &getUnplacedBlocks()
    {
        return unplaced_blocks;
    }

    // Creates and returns a new block but does not yet add it to the
    // list of blocks to codegen (this happens when and where setBlock()
    // is called). It /is/ added to a list of unplaced blocks until then,
    // and the code generator will bork at assembly time if any blocks
    // remain in that list since it's likely to be an error.
    BasicBlock *newBlock(std::string = "");

    BasicBlock *getUnplacedBlock(std::string name)
    {
        for (unsigned int loopc = 0; loopc < blocks.size(); loopc++)
        {
            if (unplaced_blocks[loopc]->name() == name)
            {
                return unplaced_blocks[loopc];
            }
        }

        return 0;
    }

    std::string display(unsigned char *);

    void addBreak(BasicBlock *b)
    {
        break_targets.push_back(b);
    }

    BasicBlock *currentBreak()
    {
        return break_targets.back();
    }

    void addContinue(BasicBlock *b)
    {
        continue_targets.push_back(b);
    }

    BasicBlock *currentContinue()
    {
        return continue_targets.back();
    }

    void removeBreak()
    {
        break_targets.pop_back();
    }

    void removeContinue()
    {
        continue_targets.pop_back();
    }

    std::vector<Value *> &getLocals()
    {
        return locals;
    }

    Value *getInteger(uint64_t val)
    {
        Value *ret = new Value(val);
        integers.push_back(ret);
        return ret;
    }

  protected:
    std::list<BasicBlock *> break_targets;
    std::list<BasicBlock *> continue_targets;

    std::vector<BasicBlock *> blocks;
    std::vector<BasicBlock *> unplaced_blocks;

    std::vector<Value *> locals;
    std::vector<Value *> integers;

    Expr *base;
    int count;
    int bbcount;
    BasicBlock *current_block;
    uint64_t stack_size;

    bool allocated_slots;
    int cconv;

    FunctionScope *scope;

    Value *retvar;
    Value *ipvar;
    Value *staticlink;
};

extern std::list<Codegen *> *codegensptr;
extern Constants *constants;

#endif

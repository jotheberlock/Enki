#ifndef _CODEGEN_
#define _CODEGEN_

#include "type.h"
#include "ast.h"
#include "asm.h"
#include <stack>
#include <vector>
#include <assert.h>

class Value
{
  public:
    
    Value(std::string n, Type * t)
    {
        name=n;
        type=t;
        is_number=false;
        stack_offset=0;
        on_stack=false;
        is_const=false;
    }

    Value()
    {
        type=0;
        is_number=false;
        stack_offset=0;
        on_stack=false;
        is_const=false;
    }
    
    Value(uint64_t v)
    {
        type=0;
        val=v;
        is_number=true;
        stack_offset=0;
        on_stack=false;
        is_const=true;
    }

    bool onStack()
    {
        return on_stack;
    }

    void setOnStack(bool b)
    {
        on_stack=b;
    }

    uint64_t stackOffset()
    {
        return stack_offset;
    }

    void setStackOffset(uint64_t o)
    {
        stack_offset=o;
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
    Type * type;
    bool on_stack;
    bool is_const;
    
};

class Constant
{
  public:

    uint64_t offset;
    char * data;
    int len;
};

class Constants
{
  public:

    Constants()
    {
        constantp=0;
        addr=0;
    }

    ~Constants();
    
    uint64_t addConstant(const char * data, int len, int align);
    uint64_t lookupOffset(uint64_t idx)
    {
        return constants[idx].offset;
    }

    void setAddress(uint64_t a)
    {
        addr=a;
    }
    
    uint64_t getAddress()
    {
        assert(addr);
        return addr;
    }

    uint64_t getSize()
    {
        return constantp;
    }

    void fillPool(unsigned char *);
    
  protected:

    std::vector<Constant> constants;
    uint64_t constantp;
    uint64_t addr;
    
};

class Codegen
{
  public:

    Codegen(Expr *, FunctionScope *);
    ~Codegen();
    
    void generate();

    FunctionScope * getScope()
    {
        return scope;
    }

    Value * getRet()
    {
        return retvar;
    }

    Value * getStackPtr()
    {
        return stackptrvar;
    }

    Value * getIp()
    {
        return ipvar;
    }

    Value * getStaticLink()
    {
        return staticlink;
    }
    
	bool extCall()   // C calling convention
	{
		return ext_call;
	}
	
	void setExtCall()
	{
		ext_call = true;
	}

    void allocateStackSlots();
    uint64_t stackSize()
    {
        assert(stack_size != 0xdeadbeef);
        return stack_size;
    }

    Value * getTemporary(Type * t, std::string n = "")
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
            
            Value * ret = new Value(buf, t);
            count++;
            locals.push_back(ret);
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
            Value * ret = new Value(buf, t);
            ret->setOnStack(true);
            count++;
            locals.push_back(ret);
                //addScopeEntry(ret);
            return ret;
        }        
    }
    
    BasicBlock * block()
    {
        return current_block;
    }

    void setBlock(BasicBlock * b)
    {
        current_block = b;
    }

    std::vector<BasicBlock *> & getBlocks()
    {
        return blocks;
    }
    
    BasicBlock * newBlock(std::string = "");

    std::string display(unsigned char *);

    void addBreak(BasicBlock * b)
    {
        break_targets.push_back(b);
    }

    BasicBlock * currentBreak()
    {
        return break_targets.back();
    }

    void addContinue(BasicBlock * b)
    {
        continue_targets.push_back(b);
    }
    
    BasicBlock * currentContinue()
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

    std::vector<Value *> & getLocals()
    {
        return locals;
    }
    
  protected:

    std::list<BasicBlock *> break_targets;
    std::list<BasicBlock *> continue_targets;
    
    std::vector<BasicBlock *> blocks;
    std::vector<Value *> locals;
    
    Expr * base;
    int count;
    int bbcount;
    BasicBlock * current_block;
    uint64_t stack_size;

    bool allocated_slots;
    bool ext_call;

    FunctionScope * scope;

    Value * retvar;
    Value * stackptrvar;
    Value * ipvar;
    Value * staticlink;
    
};

extern std::list<Codegen *> * codegens;
extern Constants * constants;

#endif

#ifndef _SYMBOLS_
#define _SYMBOLS_

#include <string>
#include <map>
#include <list>
#include <vector>
#include <assert.h>
#include <stdint.h>

class Value;
class FunctionScope;
class FunctionType;

class SymbolScope
{
  public:

    SymbolScope(SymbolScope * p)
    {
        parent_scope = p;
        if (p)
        {
            p->addChild(this);
        }
    }

    virtual ~SymbolScope()
    {
        for (std::list<SymbolScope *>::iterator it = children.begin();
             it != children.end(); it++)
        {
            delete (*it);
        }
    }

    void addChild(SymbolScope * s)
    {
        children.push_back(s);
    }
    
    void add(Value *);
    Value * lookup(std::string);
    
    FunctionScope * lookup_function(std::string);
    virtual FunctionScope * currentFunction();
    virtual FunctionScope * parentFunction();

    virtual bool isFunction()
    {
        return false;
    }
    
    SymbolScope * parent()
    {
        return parent_scope;
    }

    void addFunction(FunctionScope *);

    std::list<SymbolScope *> & getChildren()
    {
        return children;
    }

    void getValues(std::vector<Value *> & values);
    
  protected:

    SymbolScope * parent_scope;
    std::map<std::string, Value *> contents;
    std::vector<Value *> sorted_contents;
    std::map<std::string, FunctionScope *> functions;
    std::list<SymbolScope *> children;
    
};

class FunctionScope : public SymbolScope
{
  public:
 
    FunctionScope(SymbolScope * p, std::string n, FunctionType * ft);
    
    virtual FunctionScope * currentFunction();
    virtual FunctionScope * parentFunction();

    std::vector<Value *> args()
    {
        std::vector<Value *> ret;
        return ret;
    }
    
    FunctionType * getType()
    {
        return type;
    }

    virtual bool isFunction()
    {
        return true;
    }
    
    std::string name()
    {
        return function_name;
    }

    uint64_t getAddr()
    {
        assert(addr);
        return addr;
    }

    void setAddr(uint64_t a)
    {
        addr=a;
    }

    void setStackSize(uint64_t s)
    {
        stack_size = s;
    }

    uint64_t getStackSize()
    {
        assert(stack_size);
        return stack_size;
    }
    
  protected:

    std::string function_name;
    FunctionScope * function;
    FunctionType * type;
    uint64_t addr;
    uint64_t stack_size;
    
};

extern FunctionScope * root_scope;

#endif

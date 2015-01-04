#include "symbols.h"
#include "codegen.h"

void SymbolScope::add(Value * v)
{
    assert(contents.find(v->name) == contents.end());
    contents[v->name] = v;
    sorted_contents.push_back(v);
}

void SymbolScope::addFunction(FunctionScope * f)
{
    if(functions.find(f->name()) != functions.end())
    {
        fprintf(log_file, "Trying to add previously-added %s!\n",
               f->name().c_str());
    }
    
    functions[f->name()] = f;
}

Value * SymbolScope::lookup(std::string n)
{
    std::map<std::string, Value *>::iterator it = contents.find(n);
    if (it == contents.end())
    {
        return parent() ? parent()->lookup(n) : 0;
    }
    return (*it).second;
}

FunctionScope * SymbolScope::lookup_function(std::string n)
{
    std::map<std::string, FunctionScope *>::iterator it = functions.find(n);
    if (it != functions.end())
    {
        return (*it).second;
    }
    
    FunctionScope * fs = currentFunction();
    if (fs->name() == n)
    {
        return fs;
    }

    return parent() ? parent()->lookup_function(n) : 0;
}

void SymbolScope::getValues(std::vector<Value *> & values)
{
    for (std::vector<Value *>::iterator it = sorted_contents.begin();
         it != sorted_contents.end(); it++)
    {
        (*it)->setOnStack(true);
        values.push_back(*it);
    }

    for (std::list<SymbolScope *>::iterator it = children.begin();
         it != children.end(); it++)
    {
        if (!(*it)->isFunction())
        {
            (*it)->getValues(values);
        }
    }
}

FunctionScope * SymbolScope::currentFunction()
{
    return parent() ? parent()->currentFunction() : 0;
}

FunctionScope * SymbolScope::parentFunction()
{
    return currentFunction()->parentFunction();
}

FunctionScope::FunctionScope(SymbolScope * p, std::string n, FunctionType * ft)
    : SymbolScope(p)
{
    function_name = n;
    type = ft;
    if (p)
    {
        p->addFunction(this);
    }
    addr = 0;
    stack_size = 0;
    
    Type * byteptr = lookupType("Byte^");
    assert(byteptr);
    
    add(new Value("__oldframe", byteptr));
    add(new Value("__oldip", byteptr));
    add(new Value("__ret", lookupType("Uint64")));
    add(new Value("__ip", byteptr));
}

FunctionScope * FunctionScope::currentFunction()
{
    return this;
}

FunctionScope * FunctionScope::parentFunction()
{
    return parent() ? parent()->parentFunction() : 0;
}

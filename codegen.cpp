#include "codegen.h"
#include "symbols.h"
#include <string.h>

Codegen::Codegen(Expr * e, FunctionScope * fs)
{
    base = e;
    count = 0;
    bbcount = 0;
    stack_size = 0xdeadbeef;
    current_block = newBlock("prologue");
    allocated_slots = false;
	ext_call = false;
    scope = fs;
    retvar = 0;
    stackptrvar = 0;
    ipvar = 0;
    staticlink = 0;
}

Codegen::~Codegen()
{
    for (unsigned int loopc=0; loopc<blocks.size(); loopc++)
    {
        delete blocks[loopc];
    }   
}

BasicBlock * Codegen::newBlock(std::string n)
{
    if (n == "")
    {
        char buf[4096];
        sprintf(buf, "%d", bbcount);
        bbcount++;
        n = buf;
    }

    BasicBlock * ret = new BasicBlock(n);
    blocks.push_back(ret);
    return ret;
}

void Codegen::generate()
{
    scope->getValues(locals);

    for (unsigned int loopc=0; loopc<locals.size(); loopc++)
    {
        if (locals[loopc]->name == "__ret")
        {
            retvar = locals[loopc];
        }
        else if (locals[loopc]->name == "__stackptr")
        {
            stackptrvar = locals[loopc];
        }
        else if (locals[loopc]->name == "__ip")
        {
            ipvar = locals[loopc];
        }
        else if (locals[loopc]->name == "__staticlink")
        {
            staticlink = locals[loopc];
        }        
    }
    
    base->codegen(this);
    
        // Todo, extract out calling convention stuff
    if (!extCall())
    {
        Return * ret = new Return(0);
        ret->codegen(this);
    }
}

Constants::~Constants()
{
    for (unsigned int loopc=0; loopc<constants.size(); loopc++)
    {
        delete[] constants[loopc].data;
    }
}

void Constants::fillPool(unsigned char * ptr)
{
    for (unsigned int loopc=0; loopc<constants.size(); loopc++)
    {
        Constant & c = constants[loopc];
        memcpy(ptr+c.offset, c.data, c.len);
    }
}

uint64_t Constants::addConstant(const char * data, int len, int align)
{
    while (constantp % align)
        constantp++;
    
    uint64_t ret = constants.size();
    Constant c;
    c.offset = constantp;
    c.data = new char[len];
    memcpy(c.data, data, len);
    c.len = len;
    constants.push_back(c);
    constantp += len;

    return ret;
}

void Codegen::allocateStackSlots()
{
    stack_size = 0;
    for (unsigned int loopc=0; loopc<locals.size(); loopc++)
    {
        Value * v = locals[loopc];
        if (v->onStack())
        {
            fprintf(log_file, ">>> Putting %s at stack offset %ld\n",
                   v->name.c_str(), stack_size);
            
            while (stack_size % (v->type->align()/8))
            {
                stack_size++;
            }
            v->setStackOffset(stack_size);
            stack_size += v->type->size() / 8;
        }
    }

    allocated_slots = true;
}

std::string Codegen::display(unsigned char * addr)
{
    std::string ret;
    for (unsigned int loopc=0; loopc<locals.size(); loopc++)
    {
        char buf[4096];
        sprintf(buf, "%p %-20s %4ld: %s\n",
                addr+locals[loopc]->stackOffset(),
                locals[loopc]->name.c_str(),
                locals[loopc]->stackOffset(),
                locals[loopc]->type->display
                (addr+locals[loopc]->stackOffset()).c_str());
        ret += buf;
    }

    return ret;
}

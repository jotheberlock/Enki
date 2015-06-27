#include <assert.h>

#include "asm.h"
#include "symbols.h"
#include "codegen.h"
#include "platform.h"

#define OPERAND_NULL 0
#define OPERAND_VALUE 1
#define OPERAND_CONST 2
#define OPERAND_SIGNEDCONST 3
#define OPERAND_BASICBLOCK 4
#define OPERAND_REGISTER 5
#define OPERAND_FUNCTION 6
#define OPERAND_SECTION 7

bool Operand::eq(Operand & o)
{
    if (type != o.type)
    {
        return false;
    }

    switch (type)
    {
        case OPERAND_NULL:
            return true;
        case OPERAND_VALUE:
            return contents.v == o.contents.v;
        case OPERAND_CONST:
            return contents.c == o.contents.c;
        case OPERAND_SIGNEDCONST:
            return contents.sc == o.contents.sc;
        case OPERAND_BASICBLOCK:
            return contents.b == o.contents.b;
        case OPERAND_REGISTER:
            return contents.r == o.contents.r;
        case OPERAND_FUNCTION:
            return contents.f == o.contents.f;
        case OPERAND_SECTION:
            return contents.s == o.contents.s;
        default:
            assert(false);
    }

    return true;
}

bool Operand::isUsigc()
{
    return (type == OPERAND_CONST);
}

bool Operand::isSigc()
{
    return (type == OPERAND_SIGNEDCONST);
}

bool Operand::isReg()
{
    return (type == OPERAND_REGISTER);
}

bool Operand::isValue()
{
    return (type == OPERAND_VALUE);
}

bool Operand::isBlock()
{
    return (type == OPERAND_BASICBLOCK);
}

bool Operand::isFunction()
{
    return (type == OPERAND_FUNCTION);
}

bool Operand::isSection()
{
    return (type == OPERAND_SECTION);
}

Operand::Operand(Value * v)
{
    if (!v)
    {
        fprintf(log_file, "Argh! Null value in operand\n");
    }
    else if (v->is_number)
    {
        type = OPERAND_CONST;
        contents.c = v->val;
    }
    else
    {
        type = OPERAND_VALUE;
        contents.v = v;
    }
}

Operand::Operand(BasicBlock * b)
{
    type = OPERAND_BASICBLOCK;
    contents.b = b;
}

Operand::Operand(FunctionScope * f)
{
    type = OPERAND_FUNCTION;
    contents.f = f;
}

Value * Operand::getValue()
{
    assert(type == OPERAND_VALUE);
    return contents.v;
}

uint32_t Operand::getReg()
{
    if (type != OPERAND_REGISTER)
    {
        char * foo = 0;
        *foo = 0;
    }
    
    assert(type == OPERAND_REGISTER);
    return contents.r;
}

uint64_t Operand::getUsigc()
{
    assert(type == OPERAND_CONST);
    return contents.c;
}

int64_t Operand::getSigc()
{
    assert(type == OPERAND_SIGNEDCONST);
    return contents.sc;
}

BasicBlock * Operand::getBlock()
{
    if (type != OPERAND_BASICBLOCK)
    {
        char * foo = 0;
        *foo = 0;
    }
    
    assert(type == OPERAND_BASICBLOCK);
    return contents.b;
}

FunctionScope * Operand::getFunction()
{
    assert(type == OPERAND_FUNCTION);
    return contents.f;
}

uint64_t Operand::getSection(int & sec)
{
    sec = contents.s & 0xff;
    return contents.s >> 8;
}

Operand Operand::sigc(int64_t s)
{
    Operand ret;
    ret.type = OPERAND_SIGNEDCONST;
    ret.contents.sc = s;
    return ret;
}

Operand Operand::usigc(uint64_t u)
{
    Operand ret;
    ret.type = OPERAND_CONST;
    ret.contents.c = u;
    return ret;
}

Operand Operand::reg(int32_t r)
{
    Operand ret;
    ret.type = OPERAND_REGISTER;
    ret.contents.r = r;
    return ret;
}

Operand Operand::reg(std::string s)
{
    return reg(assembler->regnum(s));
}

Operand Operand::section(int s, uint64_t o)
{
    Operand ret;
    ret.type = OPERAND_SECTION;
    ret.contents.s = (o << 8) | s;
    return ret;
}

std::string Operand::toString()
{
    std::string ret;
    if (type == OPERAND_NULL)
    {
        ret = "<null>";
    }
    else if (type == OPERAND_VALUE)
    {
        if (((contents.v)->name) == "")
        {
            ret = "<blank value!>";
        }
        else
        {
            ret = (contents.v)->name;
        }
    }
    else if (type == OPERAND_CONST)
    {
        char buf[4096];
        sprintf(buf, "%lu", contents.c);
        ret = buf;
    }
    else if (type == OPERAND_SIGNEDCONST)
    {
        char buf[4096];
        sprintf(buf, "%ld", contents.sc);
        ret = buf;
    }
    else if (type == OPERAND_BASICBLOCK)
    {
        char buf[4096];
        sprintf(buf, "->%s", (contents.b)->name().c_str());
        ret = buf;
    }
    else if (type == OPERAND_REGISTER)
    {
        ret = assembler->transReg(contents.r);
    }
    else if (type == OPERAND_FUNCTION)
    {
        char buf[4096];
        sprintf(buf, "(%s)", (contents.f)->name().c_str());
        ret = buf;
    }
    else if (type == OPERAND_SECTION)
    {
        char buf[4096];
        sprintf(buf, "{%d:%lx}", contents.s & 0xff, contents.s >> 8);
        ret = buf;
    }
    else
    {
        ret = "<unknown!>";
    }
    return ret;
}

std::string Insn::toString()
{
    std::string ret = insToString();
    for (int loopc=0; loopc<oc; loopc++)
    {
        if ((ins == LOAD || ins == LOAD8 || ins == LOAD16 ||
             ins == LOAD32 || ins == LOAD64) && loopc == 2 && oc == 3)
        {
            ret += "+";
        }
        else if ((ins == STORE || ins == STORE8 || ins == STORE16 ||
                  ins == STORE32 || ins == STORE64) && loopc == 1 && oc == 3)
        {
            ret += "+";
        }
        else
        {
            ret += " ";
        }
        
        ret += ops[loopc].toString();
    }

    if (comment != "")
    {
        ret += " (";
        ret += comment;
        ret += ")";
    }

    if (addr != 0)
    {
        char buf[4096];
        sprintf(buf, "%8lx ", addr);
        ret = std::string(buf)+ret;
    }
    
    return ret;
}

std::string Insn::insToString()
{
    switch(ins)
    {
        case LOAD: return "load";
        case STORE: return "store";
        case ADD: return "add";
        case MOVE: return "move";
        case GETADDR: return "addressof";
        case MUL: return "mul";
        case STORE8 : return "store8";
        case LOAD8 : return "load8";
        case GETCONSTADDR : return "getconstaddr";
        case CMP: return "cmp";
        case SYSCALL: return "syscall";
        case RET: return "ret";
        case LOAD16: return "load16";
        case STORE16: return "store16";
        case LOAD32: return "load32";
        case STORE32: return "store32";
        case LOAD64: return "load64";
        case STORE64: return "store64";
        case LOADS8: return "loads8";
        case LOADS16: return "loads16";
        case LOADS32: return "loads32";
        case SUB: return "sub";
        case AND: return "and";
        case OR: return "or";
        case XOR: return "xor";
        case NOT: return "not";
        case SHL: return "shl";
        case SHR: return "shr";
        case SAR: return "sar";
        case RCL: return "rcl";
        case RCR: return "rcr";
        case ROL: return "rol";
        case ROR: return "ror";
        case IMUL: return "imul";
        case BEQ: return "beq";
        case BNE: return "bne";
        case BRA: return "bra";
        case BG: return "bg";
        case BLE: return "ble";
        case BL: return "bl";
        case BGE: return "bge";
        case DIV: return "div";
        case IDIV: return "idiv";
        case REM: return "rem";
        case IREM: return "irem";
        case BREAKP: return "breakp";
        case CALL: return "call";
        case SELEQ: return "seleq";
        case SELGE: return "selge";
        case SELGT: return "selgt";
        case SELGES: return "selges";
        case SELGTS: return "selgts";
        case GETSTACKSIZE: return "getstacksize";
        default: return "<unknown!>";
    }
}

void BasicBlock::calcRelationships(std::vector<BasicBlock *> & blocks)
{
    for (unsigned int loopc=0; loopc<blocks.size(); loopc++)
    {
        blocks[loopc]->children.clear();
        blocks[loopc]->parents.clear();
    }
    
    for (unsigned int loopc=0; loopc<blocks.size(); loopc++)
    {
        BasicBlock * b = blocks[loopc];
        for (std::list<Insn>::iterator it = b->getCode().begin();
             it != b->getCode().end(); it++)
        {
            Insn & i = *it;
            for (int loopc2=0; loopc2<i.oc; loopc2++)
            {
                if (i.ops[loopc2].isBlock())
                {
                    b->addChild(i.ops[loopc2].getBlock());
                }   
            }
        }
    }
}

std::string BasicBlock::toString()
{
    std::string ret;

    ret += "Block ";
    ret += name();
    ret += ":\n";

    unsigned int loopc = 0;
    for (std::list<Insn>::iterator it = insns.begin();
         it != insns.end(); it++)
    {
        std::string i = (*it).toString();
        char buf[4096];
        sprintf(buf, "%u: %s\n", loopc++, i.c_str());
        ret += buf;
    }
    return ret;
}

bool Insn::isIn(int i)
{
    return !isOut(i);
}

bool Insn::isOut(int i)
{
    if (ins == CMP || ins == RET || ins == BNE || ins == BEQ || ins == BRA
        || ins == BEQ || ins == BNE || ins == BL || ins == BGE ||
        ins == BG || ins == BLE || ins == STORE8 || ins == STORE16 ||
        ins == STORE32 || ins == STORE64 || ins == STORE || ins == CALL)
    {
        return false;
    }
    
    return (i == 0);
}

int storeForType(Type * t) 
{
    if (t->size() == 8)
    {
        return STORE8;
    }
    else if (t->size() == 16)
    {
        return STORE16;
    }
    else if (t->size() == 32)
    {
        return STORE32;
    }
    else if (t->size() == 64)
    {
        return STORE64;
    }
    else
    {
        // assert(false);
    }

	return STORE;
}

int loadForType(Type * t)
{
    if (t->size() == 8)
    {
        return LOAD8;
    }
    else if (t->size() == 16)
    {
        return LOAD16;
    }
    else if (t->size() == 32)
    {
        return LOAD32;
    }
    else if (t->size() == 64)
    {
        return LOAD64;
    }
    else
    {
      //  assert(false);
    }

	return LOAD;
}

void Assembler::newFunction(Codegen * c)
{
	current_function = c->getScope();
	assert(current_function);
}

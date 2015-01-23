#include <assert.h>
#include "amd64.h"
#include "platform.h"
#include "codegen.h"
#include "ast.h"
#include "cfuncs.h"
#include "symbols.h"

const char * regnames[] =
{
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
    0
};

unsigned char reg(uint32_t r)
{
    return r;
}

unsigned char rexreg(uint32_t r1, uint32_t r2)
{
    unsigned char ret = 0;
    if (r1 > 7)
        ret |= 0x4;
    if (r2 > 7)
        ret |= 0x1;
    return ret;
}

bool Amd64::isRRC(Insn & i)
{
    if (i.oc != 3)
    {
        return false;
    }

    if (!i.ops[0].isReg())
    {
        return false;
    }

    if (!i.ops[0].eq(i.ops[1]))
    {
        return false;
    }

    if (!(i.ops[0].isReg() || i.ops[0].isSigc() || i.ops[0].isUsigc()))
    {
        return false;
    }

    return true;
}

int Amd64::regnum(std::string s)
{
    int count = 0;
    while(regnames[count] != 0)
    {
        if (s == regnames[count])
        {
            return count;
        }
        count++;
    }

    fprintf(log_file, "Can't translate amd64 regname %s!\n", s.c_str());
    return -1;
}

ValidRegs Amd64::validRegs(Insn & i)
{
    ValidRegs ret;

    bool useAll = true;
    if (i.ins == MUL || i.ins == IMUL || i.ins == DIV || i.ins == IDIV)
    {
        ret.ops[0].set(0);  // Must be RAX
        ret.ops[1].set(0);
        ret.clobbers.set(2);
        useAll = false;
    }
    else if (i.ins == REM || i.ins == IREM)
    {
            // Ugh how to handle. Input goes into RAX, output comes
            // out in RDX.
        ret.ops[0].set(0);  // Must be RDX
        ret.ops[1].set(0);
        ret.clobbers.set(2);
        useAll = false;
    }
    
    for (int loopc=0; loopc<16; loopc++)
    {
        if (loopc != 4 && loopc != framePointer())  // rsp
        {
            if (useAll)
            {
                ret.ops[0].set(loopc);
                ret.ops[1].set(loopc);
            }
            ret.ops[2].set(loopc);
        }
    }

    return ret;
}

bool Amd64::assemble(BasicBlock * b, BasicBlock * next)
{
    std::list<Insn> & code = b->getCode();

    b->setAddr(address+len());
    
    for (std::list<Insn>::iterator it = code.begin(); it != code.end();
         it++)
    {
        Insn & i = *it;
        i.addr = address+len();
        switch (i.ins)
        {
            case STORE8:
            case STORE16:
            {
                assert(i.oc == 2 || i.oc == 3);
                    
                int sr = 0;
                int dr = 0;
                int oreg = 0;

                int32_t offs = 0;
                
                if (i.oc == 3)
                {
                    sr = 2;
                    dr = 0;
                    oreg = 1;
                }
                else
                {
                    sr = 1;
                    dr = 0;
                }

                if (i.oc == 3)
                {
                    offs = (int32_t)i.ops[oreg].getSigc();
                }

                if (i.ins == STORE16)
                {
                    *current++ = 0x66;
                }
                
                *current++ = (0x40 |
                              rexreg(i.ops[sr].getReg(), i.ops[dr].getReg()));

                if (i.ins == STORE8)
                {
                    *current++ = 0x88;
                }
                else if (i.ins == STORE16)
                {
                    *current++ = 0x89;
                }

                unsigned char rr = (reg(i.ops[sr].getReg()) & 0x7) << 3;
                rr |= reg(i.ops[dr].getReg()) & 0x7;

                if (offs != 0)
                {
                    rr |= 0x80;
                }
                else
                {
                    if (i.ops[dr].getReg() == 5 || i.ops[dr].getReg() == 13)
                    {
                        rr |= 0x40;
                    }
                }
                
                *current++ = rr;
                
                if (i.ops[dr].getReg() == 4 || i.ops[dr].getReg() == 12)
                {
                    *current++ = 0x24;
                }

                if (offs != 0)
                {
                    wles32(current, offs);
                }
                else
                {
                    if (i.ops[dr].getReg() == 5 || i.ops[dr].getReg() == 13)
                    {
                        *current++ = 0x0;
                    }
                }
                
                break;
            }
            case LOAD8:
            case LOAD16:
            case LOADS8:
            case LOADS16:
            {
                assert(i.oc == 2 || i.oc == 3);                
                int sr = 0;
                int dr = 0;
                int oreg = 0;

                int32_t offs = 0;
                
                if (i.oc == 3)
                {
                    sr = 0;
                    oreg = 2;
                    dr = 1;
                }
                else
                {
                    sr = 0;
                    dr = 1;
                }

                if (i.oc == 3)
                {
                    offs = (int32_t)i.ops[oreg].getSigc();
                }
                
                *current++ = (0x48 |
                              rexreg(i.ops[sr].getReg(), i.ops[dr].getReg()));
                *current++ = 0x0f;

                if (i.ins == LOAD8)
                {
                    *current++ = 0xb6;
                }
                else if (i.ins == LOAD16)
                {
                    *current++ = 0xb7;
                }
                else if (i.ins == LOADS8)
                {
                    *current++ = 0xbe;
                }
                else
                {
                    *current++ = 0xbf;
                }

                unsigned char rr = (reg(i.ops[sr].getReg()) & 0x7) << 3;
                rr |= reg(i.ops[dr].getReg()) & 0x7;

                if (offs != 0)
                {
                    rr |= 0x80;
                }
                else
                {
                    if (i.ops[dr].getReg() == 5 || i.ops[dr].getReg() == 13)
                    {
                        rr |= 0x40;
                    }
                }
                
                *current++ = rr;
                
                if (i.ops[dr].getReg() == 4 || i.ops[dr].getReg() == 12)
                {
                    *current++ = 0x24;
                }

                if (offs != 0)
                {
                    wles32(current, offs);
                }
                else
                {
                    if (i.ops[dr].getReg() == 5 || i.ops[dr].getReg() == 13)
                    {
                        *current++ = 0x0;
                    }
                }
                
                break;
            }
            case LOAD32:
            case STORE32:
            case LOAD64:
            case STORE64:
            case LOAD:
            case STORE:
            case LOADS32:
            {
                assert(i.oc == 2 || i.oc == 3);

                int sr = 0;
                int dr = 0;
                int oreg = 0;

                int32_t offs = 0;

                bool is_load = (i.ins == LOAD || i.ins == LOAD32 || i.ins == LOAD64 || i.ins == LOADS32);
                
                if (i.oc == 3)
                {
                    if (!is_load)
                    {
                        sr = 2;
                        dr = 0;
                        oreg = 1;
                    }
                    else
                    {
                        sr = 0;
                        oreg = 2;
                        dr = 1;
                    }
                }
                else
                {
                    if (!is_load)
                    {
                        sr = 1;
                        dr = 0;
                    }
                    else
                    {
                        sr = 0;
                        dr = 1;
                    }
                }
                
                if (i.oc == 3)
                {
                    offs = (int32_t)i.ops[oreg].getSigc();
                }

                unsigned char do64 = (i.ins == LOAD || i.ins == LOAD64
                                      || i.ins == STORE || i.ins == STORE64 || i.ins == LOADS32)
                    ? 0x8 : 0x0;
                
                *current++ = (0x40 | do64 |
                              rexreg(i.ops[sr].getReg(), i.ops[dr].getReg()));

                if (i.ins == LOADS32)
                {
                    *current++ = 0x63;
                }
                else
                {
                    *current++ = is_load ? 0x8b : 0x89;
                }
                
                unsigned char rr = (reg(i.ops[sr].getReg()) & 0x7) << 3;
                rr |= reg(i.ops[dr].getReg()) & 0x7;

                if (offs != 0)
                {
                    rr |= 0x80;
                }
                else
                {
                    if (i.ops[dr].getReg() == 5 || i.ops[dr].getReg() == 13)
                    {
                        rr |= 0x40;
                    }
                }
                
                *current++ = rr;
                
                if (i.ops[dr].getReg() == 4 || i.ops[dr].getReg() == 12)
                {
                    *current++ = 0x24;
                }

                if (offs != 0)
                {
                    wles32(current, offs);
                }
                else
                {
                    if (i.ops[dr].getReg() == 5 || i.ops[dr].getReg() == 13)
                    {
                        *current++ = 0x0;
                    }
                }
                
                break;
            }
            case MOVE:
            {
                assert(i.oc == 2);

                assert (i.ops[0].isReg());
                assert (i.ops[1].isUsigc() || i.ops[1].isSigc() ||
                        i.ops[1].isFunction() || i.ops[1].isReg() ||
                        i.ops[1].isBlock());
                
                if (i.ops[1].isUsigc() || i.ops[1].isSigc() ||
                    i.ops[1].isFunction() || i.ops[1].isBlock())
                {
                    unsigned char rex = 0x48;
                    if (i.ops[0].getReg() > 7)
                    {
                        rex |= 0x1;
                    }
                    *current++ = rex;
                    
                    if (i.ops[1].isFunction())
                    {
                        uint64_t reloc = 0xdeadbeefdeadbeef;
                        unsigned char r = 0xb8;
                        r |= reg(i.ops[0].getReg() & 0x7);
                        *current++ = r;
                        
                        relocs.push_back(Relocation(REL_A64, len()+8, len(),
                                                    i.ops[1].getFunction()));
                        wle64(current, reloc);
                    }
                    else if (i.ops[1].isBlock())
                    {
                        uint64_t reloc = 0xdeadbeefdeadbeef;
                        unsigned char r = 0xb8;
                        r |= reg(i.ops[0].getReg() & 0x7);
                        *current++ = r;
                        
                        relocs.push_back(Relocation(REL_A64, len()+8, len(),
                                                    i.ops[1].getBlock()));
                        wle64(current, reloc);
                    }
                    else
                    {
                        uint64_t val;
                        if (i.ops[1].isUsigc())
                        {
                            val = i.ops[1].getUsigc();
                        }
                        else
                        {
                            int64_t tmp = i.ops[1].getSigc();
                            val = *((uint64_t *)&tmp);
                        }
                        
                        bool big = (val > 0xffffffff);
                        if (big)
                        {
                            fprintf(log_file, "!!! Big!\n");
                            unsigned char r = 0xb8;
                            r |= reg(i.ops[0].getReg() & 0x7);
                            *current++ = r;
                            wle64(current, val);
                        }
                        else
                        {
                            *current++ = 0xc7;
                            *current++ = 0xc0 | reg(i.ops[0].getReg() & 0x7);
                            wle32(current, (uint32_t)val);
                        }
                    }
                }
                else
                {
                    *current++ = 0x48 | rexreg(i.ops[0].getReg(), i.ops[1].getReg());
                    *current++ = 0x8b;
                    unsigned char rr = (0x3 << 6)
                        | (reg(i.ops[0].getReg() & 0x7) << 3)  // dest
                        | reg(i.ops[1].getReg() & 0x7); // src
                    *current++ = rr;
                }
                
                break;
            }
            case ADD:
            case SUB:
            case AND:
            case OR:
            case XOR:
            {
                assert(isRRC(i));
                
                if (i.ops[2].isUsigc() || i.ops[2].isSigc())
                {                    
                    unsigned char rex = 0x48;
                    if (i.ops[0].getReg() > 7)
                    {
                        rex |= 0x1;
                    }
                    *current++ = rex;
                    *current++ = 0x81;
                    unsigned char rr = reg(i.ops[0].getReg()) & 0x7;
                    
                    switch(i.ins)
                    {
                        case ADD: *current++ = 0xc0 | rr;
                            break;
                        case SUB: *current++ = 0xe8 | rr;
                            break;
                        case AND: *current++ = 0xe0 | rr;
                            break;
                        case OR: *current++ = 0xc8 | rr;
                            break;
                        case XOR: *current++ = 0xf0 | rr;
                            break;
                        default:
                            fprintf(log_file, "Awoogah!\n");
                            break;
                    }

                    if (i.ops[2].isSigc())
                    {
                        int64_t val = i.ops[2].getSigc();
                        int32_t v = (int32_t)val;
                        uint32_t * ptr = (uint32_t *)&v;
                        wle32(current, *ptr);
                    }
                    else
                    {
                        uint64_t imm = i.ops[2].getUsigc();
                        assert(imm < 0x100000000);
                        wle32(current, (uint32_t)imm);
                    }
                }
                else
                {
                    *current++ = 0x48 | rexreg(i.ops[2].getReg(), i.ops[0].getReg());

                    switch(i.ins)
                    {
                        case ADD: *current++ = 0x01;
                            break;
                        case SUB: *current++ = 0x29;
                            break;
                        case AND: *current++ = 0x21;
                            break;
                        case OR: *current++ = 0x09;
                            break;
                        case XOR: *current++ = 0x31;
                            break;
                        default:
                            fprintf(log_file, "Awoogah!\n");
                            break;
                    }

                    unsigned char rr = 0xc0 
                        | (reg(i.ops[2].getReg() & 0x7) << 3)  // src
                        | reg(i.ops[0].getReg() & 0x7); // dest
                    *current++ = rr;                    
                }
                break;
            }
            case NOT:
            {
                assert(i.oc == 2);
                assert(i.ops[0].eq(i.ops[1]));
                
                if (reg(i.ops[0].getReg()) < 8)
                {
                    *current++ = 0x48;
                }
                else
                {
                    *current++ = 0x49;
                }

                *current++ = 0xf7;
                *current++ = 0xd0 | (i.ops[0].getReg() & 0x7);
                break;
            }
            case SHL:
            case SHR:
            case SAR:
            case RCL:
            case RCR:
            case ROL:
            case ROR:
            {
                assert(isRRC(i));
                assert(i.ops[2].isUsigc());
                
                if (reg(i.ops[0].getReg()) < 8)
                {
                        *current++ = 0x48;
                }
                else
                {
                        *current++ = 0x49;
                }

                unsigned char rr = reg(i.ops[0].getReg()) & 0x7;

                *current++ = 0xc1;
                switch (i.ins)
                {
                case SHL: *current++ = 0xe0 | rr;
                        break;
                case SHR: *current++ = 0xe8 | rr; 
                        break;
                case SAR: *current++ = 0xf8 | rr;
                        break;
                case RCL: *current++ = 0xd0 | rr;
                        break;
                case RCR: *current++ = 0xd8 | rr;
                        break;
                case ROL: *current++ = 0xc0 | rr;
                        break;
                case ROR: *current++ = 0xc8 | rr;
                        break;
                }

                uint64_t sr = i.ops[2].getUsigc();
                if (sr > 0xff)
                {
                    fprintf(log_file, "Too large shift!\n");
                }
                
                *current++ = (sr & 0xff);
                break;
            }
            case RET:
            {
                assert(i.oc == 0);
                *current++ = 0xc3;
                break;
            }
            case MUL:
            {
                assert(isRRC(i));
                assert(i.ops[0].getReg() == 0);
                
                if (reg(i.ops[2].getReg()) < 8)
                {
                        *current++ = 0x48;
                }
                else
                {
                        *current++ = 0x49;
                }
                unsigned char rr = reg(i.ops[2].getReg()) & 0x7;
                *current++ = 0xf7;
                *current++ = 0xe0 | rr;
                break;
            }
            case CMP:
            {
                assert(i.oc == 2);
                assert(i.ops[0].isReg());
                assert(i.ops[1].isReg() || i.ops[1].isUsigc());
                
                if (i.ops[1].isReg())
                {
                    *current++ = (0x48 |
                                  rexreg(i.ops[1].getReg(),
                                         i.ops[0].getReg()));
                    *current++ = 0x39;
                    unsigned char rr = 0xc0;
                    rr |= i.ops[0].getReg() & 0x7;
                    rr |= (i.ops[1].getReg() & 0x7) << 3;
                    *current++ = rr;
                }
                else
                {   
                    if (reg(i.ops[0].getReg()) < 8)
                    {
                        *current++ = 0x48;
                    }
                    else
                    {
                        *current++ = 0x49;
                    }

                    *current++ = 0x81;
                    *current++ = 0xf8 | (i.ops[0].getReg() & 0x7);
                    
                    uint64_t imm = i.ops[1].getUsigc();
                    assert(imm < 0x100000000);
                    wle32(current, (uint32_t)imm);
                }
                
                break;
            }
            case BRA:
            case CALL:
            {
                assert(i.oc == 1);
                assert(i.ops[0].isReg() || i.ops[0].isBlock());

                if (i.ops[0].isReg())
                {
                    if (i.ops[0].getReg() > 7)
                    {
                        *current++ = 0x41;
                    }
                    *current++ = 0xff;

                    if (i.ins == BRA)
                    {
                        *current++ = 0xe0 | (i.ops[0].getReg() & 0x7);
                    }
                    else
                    {
                        *current++ = 0xd0 | (i.ops[0].getReg() & 0x7);
                    }
                }               
                else
                {
                    assert(i.ins == BRA);
                    *current++ = 0xe9;
                    relocs.push_back(Relocation(REL_S32, currentAddr()+4, len(),
                                                i.ops[0].getBlock()));
                    wle32(current, 0xdeadbeef);
                }
                
                break;
            }
            case BNE:
            case BEQ:
            case BG:
            case BLE:
            case BL:
            case BGE:
            {
                assert(i.oc == 1);
                assert(i.ops[0].isBlock());
                *current++ = 0x0f;
                switch(i.ins)
                {
                    case BNE:
                        *current++ = 0x85;
                        break;
                    case BEQ:
                        *current++ = 0x84;
                        break;
                    case BG:
                        *current++ = 0x8f;
                        break;
                    case BLE:
                        *current++ = 0x8e;
                        break;
                    case BL:
                        *current++ = 0x8c;
                        break;
                    case BGE:
                        *current++ = 0x8d;
                        break;
                    default:
                        assert(false);
                }
                
                relocs.push_back(Relocation(REL_S32, currentAddr()+4, len(),
                                            i.ops[0].getBlock()));
                wle32(current, 0xdeadbeef);
                break;
            }
            case SYSCALL:
            {
                assert(i.oc == 0);
                *current++ = 0x0f;
                *current++ = 0x05;
                break;
            }
            case BREAKP:
            {
                assert(i.oc == 0);
                *current++ = 0xcc;   // int3
                break;
            }
            case DIV:
            case IDIV:
            case REM:
            case IREM:
            {
                assert(i.oc == 3);
                assert(i.ops[0].isReg());
                assert(i.ops[1].isReg());
                assert(i.ops[2].isReg());
                assert(i.ops[0].getReg() == i.ops[1].getReg());
                assert(i.ops[0].getReg() == 0); // RAX
                
                if (i.ops[2].getReg() > 7)
                {
                    *current++ = 0x49;
                }
                else
                {
                    *current++ = 0x48;
                }

                *current++ = 0xf7;

                if (i.ins == DIV || i.ins == REM)
                {
                    *current++ = 0xf0 | (i.ops[2].getReg() & 0x7);
                }
                else
                {
                    *current++ = 0xf8 | (i.ops[2].getReg() & 0x7);
                }

                if (i.ins == REM || i.ins == IREM)
                {
                        // hack MOV RAX RDX
                    *current++ = 0x48;
                    *current++ = 0x8b;
                    unsigned char rr = (0x3 << 6)
                        | (reg(0 & 0x7) << 3)  // dest
                        | reg(2 & 0x7); // src
                    *current++ = rr;                    
                }
                
                break;
            }
            case SELEQ:
            case SELGE:
            case SELGT:
            case SELGES:
            case SELGTS:
            {
                assert(i.ops[0].isReg());
                assert(i.ops[1].isReg());
                assert(i.ops[2].isReg());

                unsigned char op1;
                unsigned char op2;

                if (i.ins == SELEQ)
                {
                    op1 = 0x44;
                    op2 = 0x45;
                }
                else if (i.ins == SELGE)
                {
                    op1 = 0x43;
                    op2 = 0x42;
                }
                else if (i.ins == SELGT)
                {
                    op1 = 0x47;
                    op2 = 0x46;
                }
                else if (i.ins == SELGES)
                {
                    op1 = 0x4d;
                    op2 = 0x4c;
                }
                else  // SELGTS
                {
                    op1 = 0x4f;
                    op2 = 0x4e;
                }
                
                unsigned char rex = 0x48;
                if (i.ops[1].getReg() > 7)
                {
                    rex |= 0x1;
                }
                if (i.ops[0].getReg() > 7)
                {
                    rex |= 0x4;
                }
                *current++ = rex;
                *current++ = 0x0f;
                *current++ = op1;
                unsigned char rr = 0xc0 | (i.ops[1].getReg() & 0x7) |
                    ((i.ops[0].getReg() & 0x7) << 4);
                *current++ = rr;
                
                rex = 0x48;
                if (i.ops[2].getReg() > 7)
                {
                    rex |= 0x1;
                }
                if (i.ops[0].getReg() > 7)
                {
                    rex |= 0x4;
                }
                *current++ = rex;
                *current++ = 0x0f;
                *current++ = op2;
                rr = 0xc0 | (i.ops[2].getReg() & 0x7) |
                    ((i.ops[0].getReg() & 0x7) << 4);
                *current++ = rr;
                break;
            }
            default:
            {
                fprintf(log_file, "Don't know how to turn %ld into amd64!\n", i.ins);
                assert(false);
            }
        }

        if (current >= limit)
        {
            fprintf(log_file, "Ran out of space to assemble into\n");
            return false;
        }
    }
    
    return true;
}

std::string Amd64::transReg(uint32_t r)
{
    std::string ret;
    
    char buf[4096];
    if (r < 16)
    {
        ret = regnames[r];
    }
    else
    {
        sprintf(buf, "???%u", r);
        ret = buf;
    }
    
    return ret;
}

void Amd64::newFunction(Codegen * c)
{
	if (!c->extCall())
	{
		uint64_t addr = c->stackSize();
		wle64(current, addr);
	}
}	

void Amd64::align(uint64_t a)
{
	while (currentAddr() % a)
	{
		fprintf(log_file, "%llx %llu!!\n", currentAddr(), a);
		*current = 0x90;
		current++;
	}
}

Value * Amd64WindowsCallingConvention::addRegStore(const char * rname, BasicBlock * b, FunctionScope * f)
{
    char buf[4096];
    sprintf(buf, "__%s", rname);
    Value * tmp = new Value(buf, register_type);
    tmp->setOnStack(true);
    f->add(tmp);
    b->add(Insn(MOVE, tmp, Operand::reg(rname)));
    return tmp;
}

void Amd64WindowsCallingConvention::generatePrologue(BasicBlock * b, FunctionScope * f)
{
    b->add(Insn(MOVE, Operand::reg("r11"), Operand::reg(assembler->framePointer())));
    b->add(Insn(MOVE, Operand::reg(assembler->framePointer()), Operand::reg("rcx")));
    
    int count = 0;

    std::vector<Value *> args = f->args();
    
        // NB something buggy here re: args vs entries
    for (unsigned int loopc=1; loopc<args.size(); loopc++)
    {
        if (count == 0)
        {
            b->add(Insn(MOVE, Operand(args[loopc]), Operand::reg("rcx")));
        }
        else if (count == 1)
        {
            b->add(Insn(MOVE, Operand(args[loopc]), Operand::reg("rdx")));
        }
        else if (count == 2)
        {
            b->add(Insn(MOVE, Operand(args[loopc]), Operand::reg("r8")));
        }
        else if (count == 3)
        {
            b->add(Insn(MOVE, Operand(args[loopc]), Operand::reg("r9")));
        }
        else
        {
            b->add(Insn(LOAD, Operand(args[loopc]), Operand::reg("rsp"),
                        Operand::sigc(32+(count-4)*8)));
        }
        count++;
    }

    r15_backup=addRegStore("r11",b,f);
    rbx_backup=addRegStore("rbx",b,f);
    rbp_backup=addRegStore("rbp",b,f);
    rdi_backup=addRegStore("rdi",b,f);
    rsi_backup=addRegStore("rsi",b,f);
    rsp_backup=addRegStore("rsp",b,f);
    r12_backup=addRegStore("r12",b,f);
    r13_backup=addRegStore("r13",b,f);
    r14_backup=addRegStore("r14",b,f);

    Value * v = f->lookupLocal("__activation");
    assert(v);
    b->add(Insn(MOVE, Operand(v), Operand::reg(assembler->framePointer())));
    
    v = f->lookupLocal("__stackptr");
    assert(v);
    b->add(Insn(MOVE, Operand(v), Operand::reg(assembler->framePointer())));
    
    Value * stacksize = new Value("__stacksize", register_type);
    stacksize->setOnStack(true);
    f->add(stacksize);
    b->add(Insn(GETSTACKSIZE, stacksize));
    b->add(Insn(ADD, Operand(v), Operand(v), stacksize));
}

void Amd64WindowsCallingConvention::generateEpilogue(BasicBlock * b, FunctionScope * f)
{
    b->add(Insn(MOVE, Operand::reg("rbx"), rbx_backup));
    b->add(Insn(MOVE, Operand::reg("rbp"), rbp_backup));
    b->add(Insn(MOVE, Operand::reg("rdi"), rdi_backup));
    b->add(Insn(MOVE, Operand::reg("rsi"), rsi_backup));
    b->add(Insn(MOVE, Operand::reg("rsp"), rsp_backup));
    b->add(Insn(MOVE, Operand::reg("r12"), r12_backup));
    b->add(Insn(MOVE, Operand::reg("r13"), r13_backup));
    b->add(Insn(MOVE, Operand::reg("r14"), r14_backup));

    Value * v = f->lookupLocal("__ret");
    assert(v);
    
    b->add(Insn(MOVE, Operand::reg("rax"), v));
    b->add(Insn(MOVE, Operand::reg(assembler->framePointer()), r15_backup));
    b->add(Insn(RET));

    RegSet res;
    res.set(assembler->regnum("rbx"));
    res.set(assembler->regnum("rbp"));
    res.set(assembler->regnum("rdi"));
    res.set(assembler->regnum("rsi"));
    res.set(assembler->regnum("rsp"));
    res.set(assembler->regnum("r12"));
    res.set(assembler->regnum("r13"));
    res.set(assembler->regnum("r14"));
    res.set(assembler->regnum("r15"));
    res.set(assembler->regnum("rax"));
    b->setReservedRegs(res);
}

Value * Amd64WindowsCallingConvention::generateCall(Codegen * c,
                                                    Funcall * f,
                                                    std::vector<Value *> & args)
{
    BasicBlock * current = c->block();
    RegSet res;
        // args rcx rdx r8 r9, rax r10 r11 volatile
    res.set(assembler->regnum("rcx"));
    res.set(assembler->regnum("rdx"));
    res.set(assembler->regnum("r8"));
    res.set(assembler->regnum("r9"));
    res.set(assembler->regnum("rax"));
    res.set(assembler->regnum("r10"));
    res.set(assembler->regnum("r11"));
    current->setReservedRegs(res);
    
    uint64_t stack_size = 0x28;
    if (args.size() > 4)
    {
        stack_size += (args.size() - 4) * 8;
    }
    
    for (unsigned int loopc=0; loopc<args.size(); loopc++)
    {
        int dest = 9999;
        if (loopc == 0)
        {
            dest = assembler->regnum("rcx");
        }
        else if (loopc == 1)
        {
            dest = assembler->regnum("rdx");
        }
        else if (loopc == 2)
        {
            dest = assembler->regnum("r8");
        }
        else if (loopc == 3)
        {
            dest = assembler->regnum("r9");
        }
        else
        {
            current->add(Insn(STORE, Operand::reg("rsp"),
                              Operand::sigc((32+(loopc-4)*8)-stack_size), Operand(args[loopc])));
            continue;
        }
        current->add(Insn(MOVE, Operand::reg(dest), args[loopc]));
    }

    uint64_t offset;
    bool found = cfuncs->find(f->name(), offset);
    assert(found == true);

        // Location where function address is stored
    Value * jaddr = c->getTemporary(register_type, "jaddr");
    current->add(Insn(MOVE, jaddr, Operand::usigc(cfuncs->base())));
    current->add(Insn(ADD, jaddr, jaddr, Operand::usigc(offset)));
    Value * to_jump = c->getTemporary(register_type, "to_jump");
    
    current->add(Insn(SUB, Operand::reg("rsp"), Operand::reg("rsp"),
                      Operand::usigc(stack_size)));
    current->add(Insn(LOAD, to_jump, jaddr));
    current->add(Insn(CALL, to_jump));
    
    Value * ret = c->getTemporary(register_type, "ret");
    current->add(Insn(MOVE, ret, Operand::reg("rax")));

    current->add(Insn(ADD, Operand::reg("rsp"), Operand::reg("rsp"),
                      Operand::usigc(stack_size)));
    return ret;
}


Value * Amd64UnixCallingConvention::generateCall(Codegen * c,
                                                 Funcall * f,
                                                 std::vector<Value *> & args)
{
    BasicBlock * current = c->block();
    RegSet res;
        // everything but rbp, rbx, r12-r15 volatile
    res.set(assembler->regnum("rax"));
    res.set(assembler->regnum("rcx"));
    res.set(assembler->regnum("rdx"));
    res.set(assembler->regnum("rsp"));
    res.set(assembler->regnum("rsi"));
    res.set(assembler->regnum("rdi"));
    res.set(assembler->regnum("r8"));
    res.set(assembler->regnum("r9"));
    res.set(assembler->regnum("r10"));
    res.set(assembler->regnum("r11"));
    
    current->setReservedRegs(res);
    
    uint64_t stack_size = 0x0;
    if (args.size() > 6)
    {
        stack_size += (args.size() - 6) * 8;
    }
    
    for (unsigned int loopc=0; loopc<args.size(); loopc++)
    {
        int dest = 9999;
        if (loopc == 0)
        {
            dest = assembler->regnum("rdi");
        }
        else if (loopc == 1)
        {
            dest = assembler->regnum("rsi");
        }
        else if (loopc == 2)
        {
            dest = assembler->regnum("rdx");
        }
        else if (loopc == 3)
        {
            dest = assembler->regnum("rcx");
        }
        else if (loopc == 4)
        {
            dest = assembler->regnum("r8");
        }
        else if (loopc == 5)
        {
            dest = assembler->regnum("r9");
        }
        else
        {
            current->add(Insn(STORE, Operand::reg("rsp"),
                              Operand::sigc(((loopc-6)*8)-stack_size),
                              Operand(args[loopc])));
            continue;
        }
        
        current->add(Insn(MOVE, Operand::reg(dest), args[loopc]));
    }

    uint64_t offset;
    bool found = cfuncs->find(f->name(), offset);
    assert(found == true);

        // Location where function address is stored
    Value * jaddr = c->getTemporary(register_type, "jaddr");
    current->add(Insn(MOVE, jaddr, Operand::usigc(cfuncs->base())));
    current->add(Insn(ADD, jaddr, jaddr, Operand::usigc(offset)));
    Value * to_jump = c->getTemporary(register_type, "to_jump");

    if (stack_size>0)
    {
        current->add(Insn(SUB, Operand::reg("rsp"), Operand::reg("rsp"),
                          Operand::usigc(stack_size)));
    }
    
    current->add(Insn(LOAD, to_jump, jaddr));
    current->add(Insn(CALL, to_jump));

    if (stack_size>0)
    {
        current->add(Insn(ADD, Operand::reg("rsp"), Operand::reg("rsp"),
                          Operand::usigc(stack_size)));
    }
    
    Value * ret = c->getTemporary(register_type, "ret");
    current->add(Insn(MOVE, ret, Operand::reg("rax")));

    return ret;
}

Value * Amd64UnixCallingConvention::addRegStore(const char * rname, BasicBlock * b, FunctionScope * f)
{
    char buf[4096];
    sprintf(buf, "__%s", rname);
    Value * tmp = new Value(buf, register_type);
    tmp->setOnStack(true);
    f->add(tmp);
    b->add(Insn(MOVE, tmp, Operand::reg(rname)));
    return tmp;
}

void Amd64UnixCallingConvention::generatePrologue(BasicBlock * b, FunctionScope * f)
{
    b->add(Insn(MOVE, Operand::reg("r11"), Operand::reg(assembler->framePointer())));
    b->add(Insn(MOVE, Operand::reg(assembler->framePointer()), Operand::reg("rdi")));
    
    int count = 0;

    std::vector<Value *> args = f->args();
    
    for (unsigned int loopc=1; loopc<args.size(); loopc++)
    {
        if (count == 0)
        {
            b->add(Insn(MOVE, Operand(args[loopc]), Operand::reg("rsi")));
        }
        else if (count == 1)
        {
            b->add(Insn(MOVE, Operand(args[loopc]), Operand::reg("rdx")));
        }
        else if (count == 2)
        {
            b->add(Insn(MOVE, Operand(args[loopc]), Operand::reg("rcx")));
        }
        else if (count == 3)
        {
            b->add(Insn(MOVE, Operand(args[loopc]), Operand::reg("r8")));
        }
        else if (count == 4)
        {
            b->add(Insn(MOVE, Operand(args[loopc]), Operand::reg("r9")));
        }
        else
        {
            b->add(Insn(LOAD, Operand(args[loopc]), Operand::reg("rsp"), Operand::usigc((count-5)*8)));
        }
        count++;
    }
    
    rbx_backup=addRegStore("rbx",b,f);
    rbp_backup=addRegStore("rbp",b,f);
    r12_backup=addRegStore("r12",b,f);
    r13_backup=addRegStore("r13",b,f);
    r14_backup=addRegStore("r14",b,f);
    r15_backup=addRegStore("r11",b,f);
}

void Amd64UnixCallingConvention::generateEpilogue(BasicBlock * b, FunctionScope * f)
{
	
    RegSet res;
    res.set(assembler->regnum("rbx"));
    res.set(assembler->regnum("rbp"));
    res.set(assembler->regnum("rdi"));
    res.set(assembler->regnum("rsi"));
    res.set(assembler->regnum("rsp"));
    res.set(assembler->regnum("r12"));
    res.set(assembler->regnum("r13"));
    res.set(assembler->regnum("r14"));
    res.set(assembler->framePointer());
    res.set(assembler->regnum("rax"));
    b->setReservedRegs(res);

    b->add(Insn(MOVE, Operand::reg("rbx"), rbx_backup));
    b->add(Insn(MOVE, Operand::reg("rbp"), rbp_backup));
    b->add(Insn(MOVE, Operand::reg("r12"), r12_backup));
    b->add(Insn(MOVE, Operand::reg("r13"), r13_backup));
    b->add(Insn(MOVE, Operand::reg("r14"), r14_backup));

    Value * v = f->lookupLocal("__ret");
    assert(v);
    
    b->add(Insn(MOVE, Operand::reg("rax"), v));
    b->add(Insn(MOVE, Operand::reg(assembler->framePointer()), r15_backup));
    b->add(Insn(RET));
}

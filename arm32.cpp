#include "arm32.h"
#include "platform.h"
#include "codegen.h"

int Arm32::regnum(std::string s)
{
    int ret = -1;
    if (s[0] == 'r')
    {
        sscanf(s.c_str()+1, "%d", &ret);
            assert(ret<16);
    }
    else if (s == "sp")
    {
            ret = 13;
    }
    else if (s == "lr")
    {
        ret = 14;
    }
    else if (s == "pc")
    {
        ret = 15;
    }   

    assert(ret != -1);
    return ret;
}

int Arm32::size(BasicBlock * b)
{
    int ret = 0;
    std::list<Insn> & code = b->getCode();
    
    for (std::list<Insn>::iterator it = code.begin(); it != code.end();
         it++)
    {
        Insn & i = *it;
        i.addr = address+len();

        switch(i.ins)
        {
            case SELEQ:
            case SELGE:
            case SELGT:
            case SELGES:
            case SELGTS:
            {
                ret += 8;
                i.size = 8;
                break;
            }
	    case MOVE:
	    {
 	        if (i.ops[1].isReloc() || i.ops[1].isSigc() || i.ops[1].isUsigc())
		{
		    ret += 8;
		    i.size = 8;
		}
		else
		{
		    ret += 4;
		    i.size = 4;
	        }
	        break;
	    }
            default:
            {
                ret += 4;
                i.size = 4;
            }
        }
    }
    return ret;
}

bool Arm32::calcImm(uint64 raw, uint32 & result)
{
    uint32 shift = 0;
    while (shift < 4)
    {
        uint32 trial = (raw & 0xff) << shift;
        if (trial == raw)
        {
            result = (raw & 0xff) | (shift << 8);
            return true;
        }
        shift += 1;
    }

    return false;
}

bool Arm32::assemble(BasicBlock * b, BasicBlock * next, Image * image)
{

    std::list<Insn> & code = b->getCode();

    b->setAddr(address+flen());

    uint64 current_addr = (uint64)current;
    assert((current_addr & 0x3) == 0);

    for (std::list<Insn>::iterator it = code.begin(); it != code.end();
         it++)
    {
        Insn & i = *it;
        i.addr = address+flen();

        uint32 mc = 0;

        unsigned char * oldcurrent = current;
	
        switch (i.ins)
        {
	    case SYSCALL:
	    {
	        assert(i.oc == 1 || i.oc == 0);
		if (i.oc == 1)
		{
		    assert(i.ops[0].isUsigc());
		    assert(i.ops[0].getUsigc() > 0xffffff);
		    mc = (0xe << 28) | (0xf << 24) | i.ops[1].getUsigc();
		}
		else
		{
		    mc = (0xe << 28) | (0xf << 24);  // assume SWI 0 if not specified
		}
	        break;
	    }
            case MOVE:
            {
                assert(i.oc == 2);
                
                assert (i.ops[0].isReg());
                assert (i.ops[1].isUsigc() || i.ops[1].isSigc() ||
                        i.ops[1].isFunction() || i.ops[1].isReg() ||
                        i.ops[1].isBlock() || i.ops[1].isSection() || i.ops[1].isExtFunction());

                if (i.ops[1].isUsigc() || i.ops[1].isSigc() ||
                    i.ops[1].isFunction() || i.ops[1].isBlock() ||
                    i.ops[1].isSection() || i.ops[1].isExtFunction())
                {
 		    uint32_t val = 0xdeadbeef;
                    if (i.ops[1].isFunction())
                    {

                    }
                    else if (i.ops[1].isBlock())
                    {

                    }
                    else if (i.ops[1].isSection())
                    {

                    }
                    else if (i.ops[1].isExtFunction())
                    {

                    }
                    else
                    {
                        if (i.ops[1].isUsigc())
                        {
                            val = i.ops[1].getUsigc();
                        }
                        else
                        {
                            int32 tmp = i.ops[1].getSigc();
                            val = *((uint32 *)&tmp);
                        }
			
			// ARMv7 movw/movt goes here
                    }
                }
                else
                {
                    mc = 0xe0d00000 | (i.ops[1].getReg() << 16) |
                        i.ops[0].getReg() << 12;
                }
                
                break;
            }
            default:
            {
                fprintf(log_file, "Don't know how to turn %lld [%s] into arm!\n", i.ins, i.toString().c_str());
                    // assert(false);
            }
            
            unsigned int siz = (unsigned int)(current - oldcurrent);
            if (siz > i.size)
            {
                printf("Unexpectedly large instruction! estimate %lld actual %d %s\n",
                       i.size, siz, i.toString().c_str());
            }
        }

        wee32(le, current, mc);
        if (current >= limit)
        {
            printf("Ran out of space to assemble into, %d\n", (int)(limit-base));
            fprintf(log_file, "Ran out of space to assemble into, %d\n", (int)(limit-base));
            return false;
        }
    }
    
    return true;
}

std::string Arm32::transReg(uint32 r)
{
    assert(r < 16);
    if (r < 13)
    {
        char buf[4096];
        sprintf(buf, "r%d", r);
        return buf;
    }
    else if (r == 13)
    {
        return "sp";
    }
    else if (r == 14)
    {
        return "lr";
    }
    else
    {
        return "pc";
    }
}

bool Arm32::configure(std::string param, std::string val)
{
    if (param == "bits")
    {
        return false;
    }

    return Assembler::configure(param, val);
}

ValidRegs Arm32::validRegs(Insn & i)
{
    ValidRegs ret;

    for (int loopc=0; loopc<16; loopc++)
    {
        if (loopc != 15 && loopc != 13  && loopc != framePointer())  // pc, sp
        {
            ret.ops[0].set(loopc);
            ret.ops[1].set(loopc);
            ret.ops[2].set(loopc);
        }
    }

    return ret;
}

bool Arm32::validConst(Insn & i, int idx)
{
    if (i.ins == DIV || i.ins == IDIV || i.ins == REM ||
        i.ins == IREM || i.ins == SELEQ || i.ins == SELGT ||
            i.ins == SELGE || i.ins == SELGTS || i.ins == SELGES ||
        i.ins == MUL || i.ins == IMUL)
    {
        return false;
        }
    
    if (i.ins == ADD || i.ins == SUB || i.ins == MUL
        || i.ins == IMUL || i.ins == AND || i.ins == OR
        || i.ins == XOR || i.ins == SHL || i.ins == SHR)
    {
        if (idx != 2)
        {
            return false;
        }
    }
    
    if (i.ins == STORE || i.ins == STORE8 || i.ins == STORE16 || i.ins == STORE32 || i.ins == STORE64)
    {
        if (idx == 2)
        {
            return false;
        }
    }
    
    if (i.ins == CMP)
    {
        if (idx == 0)
        {
            return false;
        }
    }
    
    return true;
}

void Arm32::newFunction(Codegen * c)
{
	Assembler::newFunction(c);
	if (c->callConvention() == CCONV_STANDARD)
	{
		uint64 addr = c->stackSize();
        wle32(current, checked_32(addr));
	}
}

void Arm32::align(uint64 a)
{
    uint32 nop = 0xf3af1000;
	while (currentAddr() % a)
	{
		*((uint32 *)current) = nop;
		current += 4;
	}
}

Value * ArmLinuxSyscallCallingConvention::generateCall(Codegen * c,
                                                      Value * fptr,
                                                      std::vector<Value *> & args)
{
    BasicBlock * current = c->block();
    RegSet res;
        // kernel destroys rcx, r11
    res.set(assembler->regnum("r0"));
    res.set(assembler->regnum("r1"));
    res.set(assembler->regnum("r2"));
    res.set(assembler->regnum("r3"));
    res.set(assembler->regnum("r4"));
    res.set(assembler->regnum("r5"));

    res.set(assembler->regnum("r7"));
    
    current->setReservedRegs(res);
    
    if (args.size() > 7)
    {
        fprintf(log_file, "Warning, syscall passed more than 6 args!\n");
    }
    else if (args.size() < 1)
    {
        fprintf(log_file, "No syscall number passed!\n");
    }
    
    for (unsigned int loopc=0; loopc<args.size(); loopc++)
    {
        int dest = 9999;
        if (loopc == 0)
        {
                // Syscall number
            dest = assembler->regnum("r7");
        }
        else if (loopc == 1)
        {
            dest = assembler->regnum("r0");
        }
        else if (loopc == 2)
        {
            dest = assembler->regnum("r1");
        }
        else if (loopc == 3)
        {
            dest = assembler->regnum("r2");
        }
        else if (loopc == 4)
        {
            dest = assembler->regnum("r3");
        }
        else if (loopc == 5)
        {
            dest = assembler->regnum("r4");
        }
        else if (loopc == 6)
        {
            dest = assembler->regnum("r5");
        }
        current->add(Insn(MOVE, Operand::reg(dest), args[loopc]));
    }

    current->add(Insn(SYSCALL));
    Value * ret = c->getTemporary(register_type, "ret");
    current->add(Insn(MOVE, ret, Operand::reg("r0")));
    
    BasicBlock * postsyscall = c->newBlock("postsyscall");
    current->add(Insn(BRA, Operand(postsyscall)));
    c->setBlock(postsyscall);
    
    return ret;
}


    

    

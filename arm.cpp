#include "arm.h"
#include "platform.h"
#include "codegen.h"

int Arm::regnum(std::string s)
{
    int ret = -1;
    if (psize == 64)
    {
        if (s[0] == 'x')
        {
            sscanf(s.c_str()+1, "%d", &ret);
            assert(ret<32);
        }
    }
    else
    {
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
    }

    assert(ret != -1);
    return ret;
}

int Arm::size(BasicBlock * b)
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
            default:
            {
                ret += 4;
                i.size = 4;
            }
        }
    }
    return ret;
}

bool Arm::assemble(BasicBlock * b, BasicBlock * next, Image * image)
{

    std::list<Insn> & code = b->getCode();

    b->setAddr(address+flen());

	uint64_t current_addr = (uint64_t)current;
    assert((current_addr & 0x3) == 0);

    for (std::list<Insn>::iterator it = code.begin(); it != code.end();
         it++)
    {
        Insn & i = *it;
        i.addr = address+flen();

	uint32_t * ins_ptr = (uint32_t *)current;
       
        unsigned char * oldcurrent = current;
	
        switch (i.ins)
        {
            
            default:
            {
                fprintf(log_file, "Don't know how to turn %ld [%s] into arm!\n", i.ins, i.toString().c_str());
                assert(false);
            }
            
            unsigned int siz = (unsigned int)(current - oldcurrent);
            if (siz > i.size)
            {
                printf("Unexpectedly large instruction! estimate %d actual %d %s\n",
                       i.size, siz, i.toString().c_str());
            }
        }

	current += 4;
        if (current >= limit)
        {
            printf("Ran out of space to assemble into, %d\n", (int)(limit-base));
            fprintf(log_file, "Ran out of space to assemble into, %d\n", (int)(limit-base));
            return false;
        }
    }
    
    return true;
}

std::string Arm::transReg(uint32_t r)
{
    if (psize == 64)
    {
        assert (r < 32);
        char buf[4096];
        sprintf(buf, "x%d", r);
        return buf;
    }
    else
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
}

ValidRegs Arm::validRegs(Insn & i)
{
    ValidRegs ret;
    return ret;
}

bool Arm::validConst(Insn & i, int idx)
{
    return true;
}

void Arm::newFunction(Codegen * c)
{
	Assembler::newFunction(c);
	if (c->callConvention() == CCONV_STANDARD)
	{
		uint64_t addr = c->stackSize();
        if (psize == 64)
        {
            wle64(current, addr);
        }
        else
        {
            wle32(current, checked_32(addr));
        }
	}
}

void Arm::align(uint64_t a)
{
    uint32_t nop = (psize == 64) ? 0xd503201f : 0xf3af1000;
	while (currentAddr() % a)
	{
		*((uint32_t *)current) = nop;
		current += 4;
	}
}

Value * ArmUnixSyscallCallingConvention::generateCall(Codegen * c,
                                                      Value * fptr,
                                                      std::vector<Value *> & args)
{
    return 0;
}


    

    

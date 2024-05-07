#include "arm64.h"
#include "codegen.h"
#include "image.h"
#include "platform.h"

bool Arm64::validRegOffset(Insn &i, int off)
{
    return false;
}

int Arm64::regnum(std::string s)
{
    int ret = -1;
    if (s[0] == 'r')
    {
        sscanf(s.c_str() + 1, "%d", &ret);
        assert(ret < 32);
    }
    else if (s == "sp")
    {
        ret = 31;
    }

    assert(ret != -1);
    return ret;
}

int Arm64::size(BasicBlock *b)
{
    int ret = 0;
    std::list<Insn> &code = b->getCode();

    for (auto &i :code)
    {
        i.addr = address + len();

        switch (i.ins)
        {
        case MOVE: {
	    if (i.ops[1].isReg())
	    {
		ret += 4;
		i.size = 4;
	    }
	    else
	    {
		// Worst case, a 64 bit immediate
		ret += 16;
		i.size = 16;
	    }
            break;
        }
        default: {
            ret += 4;
            i.size = 4;
        }
        }
    }
    return ret;
}

bool Arm64::calcImm(uint64_t raw, uint32_t &result)
{
    printf("Error, cannot encode %lx as an ARM constant!\n", raw);
    return false;
}

bool Arm64::assemble(BasicBlock *b, BasicBlock *next, Image *image)
{
    std::list<Insn> &code = b->getCode();

    b->setAddr(address + flen());

    uint64_t current_addr = (uint64_t)current;
    assert((current_addr & 0x3) == 0);

    for (auto &i : code)
    {
        i.addr = address + flen();

        uint32_t mc = 0xd503201f;  // NOP

        unsigned char *oldcurrent = current;

        switch (i.ins)
        {
        case SYSCALL: {
            assert(i.oc == 1 || i.oc == 0);
            if (i.oc == 1)
            {
                assert(i.ops[0].isUsigc());
                assert(i.ops[0].getUsigc() <= 0xffff);
                mc = 0xd4000001 | ((uint32_t)i.ops[0].getUsigc() << 1);
            }
            else
            {
                mc = 0xd4000001; // assume 0 if not specified
            }
            break;
        }
        case BREAKP: {
            assert(i.oc == 0);
	    mc = 0xd2000000;
            break;
        }
        case LOAD8:
        case LOADS8:
        case LOAD16:
        case LOADS16:
        case LOADS32:
        case LOAD:
        case LOAD32: {
            break;
        }
        case STORE8:
        case STORE16:
        case STORE:
        case STORE32: {
            break;
        }
        case LOAD64:
        case STORE64: {
            break;
        }
        case MOVE: {
            assert(i.oc == 2);

            assert(i.ops[0].isReg());
            assert(i.ops[1].isUsigc() || i.ops[1].isSigc() || i.ops[1].isFunction() || i.ops[1].isReg() ||
                   i.ops[1].isBlock() || i.ops[1].isSection() || i.ops[1].isExtFunction());

            if (i.ops[1].isUsigc() || i.ops[1].isSigc() || i.ops[1].isFunction() || i.ops[1].isBlock() ||
                i.ops[1].isSection() || i.ops[1].isExtFunction())
            {
                uint64_t val = 0x0;
                BaseRelocation *br = 0;

                if (i.ops[1].isFunction())
                {
                    assert(current_function);
                    br = new FunctionRelocation(image, current_function, flen(), i.ops[1].getFunction(), 0);
                }
                else if (i.ops[1].isBlock())
                {
                    br = new AbsoluteBasicBlockRelocation(image, current_function, flen(), i.ops[1].getBlock());
                }
                else if (i.ops[1].isSection())
		{
                    int s;
                    uint64_t o = i.ops[1].getSection(s);
                    br = new SectionRelocation(image, IMAGE_CODE, len(), s, o);
                }
                else if (i.ops[1].isExtFunction())
                {
                    br = new ExtFunctionRelocation(image, current_function, len(), i.ops[1].getExtFunction());
                }
                else
                {
                    if (i.ops[1].isUsigc())
                    {
                        val = (uint64_t)i.ops[1].getUsigc();
                    }
                    else
                    {
                        int64_t tmp = (int64_t)i.ops[1].getSigc();
                        val = *((uint64_t *)&tmp);
                    }
                }

		if (br)
		{
		}
		else
		{
		    // If this is a known value we can be a bit cleverer about emitting it
		    mc = 0xd2800000 | ((val & 0xffff) << 5) | i.ops[0].getReg();
		    val >>= 16;
		    if (val != 0)
		    {
			wee32(le, current, mc);
			mc = 0xf2a00000 | ((val & 0xffff) << 5) | i.ops[0].getReg();
		    }
		    val >>= 16;
		    if (val != 0)
		    {
			wee32(le, current, mc);
			mc = 0xf2c00000 | ((val & 0xffff) << 5) | i.ops[0].getReg();
		    }
		    val >>= 16;
		    if (val != 0)
		    {
			wee32(le, current, mc);
			mc = 0xf2e00000 | ((val & 0xffff) << 5) | i.ops[0].getReg();
		    }
	        }
            }
            else
            {
		// MOV to/from SP is actually an add
		if (i.ops[0].getReg() == osStackPointer())
		{
		    if (i.ops[1].getReg() == osStackPointer())
		    {
			// This is just a NOP
			break;
		    }

		    mc = 0x9100001f | i.ops[1].getReg() << 5;
		}
		else if (i.ops[1].getReg() == osStackPointer())
		{
		    mc = 0x910003e0 | i.ops[0].getReg();
		}
		else
		{
		    mc = 0xaa0003e0 | i.ops[1].getReg() << 16 | i.ops[0].getReg();
		}
            }

            break;
        }
        case ADD:
        case SUB:
        case AND:
        case OR:
        case XOR: {
            break;
        }
        case SHL:
        case SHR:
        case SAR:
        case RCL:
        case RCR:
        case ROL:
        case ROR: {
            break;
        }
        case CMP: {
	    break;
        }
        case NOT: {
            break;
        }
        case SELEQ:
        case SELGE:
        case SELGT:
        case SELGES:
        case SELGTS: {
            break;
        }
        case BRA: {
            break;
        }
        case BNE:
        case BEQ:
        case BG:
        case BLE:
        case BL:
        case BGE: {
            break;
        }
        case DIV:
        case DIVS: {
            break;
        }
        case REM:
        case REMS: {
            break;
        }
        case MUL: {
            break;
        }
        case MULS: {
            break;
        }
        default: {
            fprintf(log_file, "Don't know how to turn %ld [%s] into arm!\n", i.ins, i.toString().c_str());
            assert(false);
        }

            unsigned int siz = (unsigned int)(current - oldcurrent);
            if (siz > i.size)
            {
                printf("Unexpectedly large instruction! estimate %ld actual %d %s\n", i.size, siz,
                       i.toString().c_str());
            }
        }

        wee32(le, current, mc);
        if (current >= limit)
        {
            printf("Ran out of space to assemble into, %d\n", (int)(limit - base));
            fprintf(log_file, "Ran out of space to assemble into, %d\n", (int)(limit - base));
            return false;
        }
    }

    return true;
}

std::string Arm64::transReg(uint32_t r)
{
    assert(r < 32);
    if (r < 31)
    {
        char buf[4096];
        sprintf(buf, "r%d", r);
        return buf;
    }
    else if (r == 31)
    {
        return "sp";
    }
    else
    {
        assert(false);
    }


    return ""; // Shut compiler up
}

bool Arm64::configure(std::string param, std::string val)
{
    if (param == "bits")
    {
        return false;
    }

    return Assembler::configure(param, val);
}

ValidRegs Arm64::validRegs(Insn &i)
{
    ValidRegs ret;
    for (int loopc = 0; loopc < 31; loopc++)
    {
	ret.ops[0].set(loopc);
	ret.ops[1].set(loopc);
	ret.ops[2].set(loopc);
    }
    return ret;
}

bool Arm64::validConst(Insn &i, int idx)
{
    return true;
}

void Arm64::newFunction(Codegen *c)
{
    Assembler::newFunction(c);
    if (c->callConvention() == CCONV_STANDARD)
    {
        uint64_t addr = c->stackSize();
        wle32(current, checked_32(addr));
    }
}

void Arm64::align(uint64_t a)
{
}

Value *Arm64LinuxSyscallCallingConvention::generateCall(Codegen *c, Value *fptr, std::vector<Value *> &args)
{
    BasicBlock *current = c->block();

    if (args.size() > 6)
    {
        fprintf(log_file, "Warning, syscall passed more than 6 args!\n");
    }
    else if (args.size() < 1)
    {
        fprintf(log_file, "No syscall number passed!\n");
    }

    for (unsigned int loopc = 0; loopc < args.size(); loopc++)
    {
        int dest = 9999;
        if (loopc == 0)
        {
            // Syscall number
            dest = assembler->regnum("r8");
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
    Value *ret = c->getTemporary(register_type, "ret");
    current->add(Insn(MOVE, ret, Operand::reg("r0")));

    BasicBlock *postsyscall = c->newBlock("postsyscall");
    current->add(Insn(BRA, Operand(postsyscall)));
    c->setBlock(postsyscall);

    return ret;
}

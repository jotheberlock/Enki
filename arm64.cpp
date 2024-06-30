#include "arm64.h"
#include "codegen.h"
#include "image.h"
#include "platform.h"

bool Arm64::validRegOffset(Insn &i, int off)
{
    if (i.ins == LOAD64 || i.ins == STORE64 || i.ins == LOAD || i.ins == STORE)
    {
	if (!(off & 0x7))
	{
	    if ((off << 8) < 0x200)
	    {
		return true;
	    }
	}
    }
    else if (i.ins == LOAD32 || i.ins == LOADS32 || i.ins == STORE32)
    {
	if (!(off & 0x3))
	{
	    if ((off << 4) < 0x200)
	    {
		return true;
	    }
	}
    }
    else if (i.ins == LOAD16 || i.ins == LOADS16 || i.ins == STORE16)
    {
	if (!(off & 0x1))
	{
	    if ((off << 2) < 0x200)
	    {
		return true;
	    }
	}
    }
    else
    {
	if (off < 0x200)
	{
	    return true;
	}
    }
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
	case REM:
	case REMS: {
	    ret += 8;
	    i.size = 8;
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
        case LOAD32:
	case LOAD64: {
            assert(i.oc == 2 || i.oc == 3);

            int32_t val = 0;
            uint32_t uval = 0;
            bool negative_offset = false;

            if (i.oc == 3)
            {
                val = (int32_t)i.ops[2].getSigc();
                assert(validRegOffset(i, val));
                if (val < 0)
                {
                    negative_offset = true;
                    uval = -val;
                }
                else
                {
                    uval = val;
                }
            }

            if ((i.ins == LOAD || i.ins == LOAD64 || i.ins == LOAD32) && !negative_offset)
            {
		mc = 0xb9400000 | ((i.ins != LOAD32 ? 0x1 : 0x0) << 30) |
		    (i.ins == LOAD32 ? uval >> 2 : uval >> 3) << 10 | i.ops[0].getReg() | i.ops[1].getReg() << 5;
	    }
	    else if (i.ins == LOAD8)
	    {
		mc = 0x39400000 | uval << 10 | i.ops[0].getReg() | i.ops[1].getReg() << 5;
	    }
	    else if (i.ins == LOADS8)
	    {
		mc = 0x39c00000 | uval << 10 | i.ops[0].getReg() | i.ops[1].getReg() << 5;
	    }
	    else if (i.ins == LOAD16)
	    {
		mc = 0x79400000 | (uval >> 1) << 10 | i.ops[0].getReg() | i.ops[1].getReg() << 5;
	    }
	    else if (i.ins == LOADS16)
	    {
		mc = 0x79c00000 | (uval >> 1) << 10 | i.ops[0].getReg() | i.ops[1].getReg() << 5;
	    }
            break;
        }
        case STORE8:
        case STORE16:
        case STORE:
        case STORE32:
	case STORE64: {
            assert(i.oc == 2 || i.oc == 3);

            int32_t val = 0;
            uint32_t uval = 0;
            bool negative_offset = false;

	    int dest = 1;
            if (i.oc == 3)
            {
                val = (int32_t)i.ops[1].getSigc();
                assert(validRegOffset(i, val));
                if (val < 0)
                {
                    negative_offset = true;
                    uval = -val;
                }
                else
                {
                    uval = val;
                }
		dest = 2;
            }

            if ((i.ins == STORE || i.ins == STORE64 || i.ins == STORE32) && !negative_offset)
            {
		mc = 0xb9000000 | ((i.ins != STORE32 ? 0x1 : 0x0) << 30) | (i.ins == STORE32 ? uval >> 2 : uval >> 3) << 10 | i.ops[dest].getReg() | i.ops[0].getReg() << 5;
	    }
	    else if (i.ins == STORE16)
	    {
		mc = 0x79000000 | (uval >> 1) << 10 | i.ops[dest].getReg() | i.ops[0].getReg() << 5;
	    }
	    else if (i.ins == STORE8)
	    {
		mc = 0x39000000 | uval << 10 | i.ops[dest].getReg() | i.ops[0].getReg() << 5;
	    }
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
                    br->addReloc(0, 0, 0xffff, 5, 32);
                    br->addReloc(4, 48, 0xffff, 5, 32);
                    br->addReloc(8, 32, 0xffff, 5, 32);
                    br->addReloc(12, 16, 0xffff, 5, 32);
		    wee32(le, current, 0xd2800000 | ((val & 0xffff) << 5) | i.ops[0].getReg());
		    wee32(le, current, 0xf2e00000 | (((val >> 48) & 0xffff) << 5) | i.ops[0].getReg());
		    wee32(le, current, 0xf2c00000 | (((val >> 32) & 0xffff) << 5) | i.ops[0].getReg());
		    mc = 0xf2a00000 | (((val >> 16) & 0xffff) << 5) | i.ops[0].getReg();
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
	    // FIXME: looks like the logical ops have a different encoding
            assert(i.oc == 3);
            assert(i.ops[0].isReg());
            assert(i.ops[1].isReg());
            assert(i.ops[2].isReg() || i.ops[2].isUsigc() || i.ops[2].isSigc());

            uint32_t op = 0;
	    bool is_immediate = i.ops[2].isUsigc() || i.ops[2].isSigc();
            if (i.ins == ADD)
            {
                op = is_immediate ? 0x91 : 0x8b;
            }
            else if (i.ins == SUB)
            {
                op = is_immediate ? 0xd1 : 0xcb;
            }
            else if (i.ins == AND)
            {
                op = is_immediate ? 0x92 : 0x8a;
            }
            else if (i.ins == OR)
            {
                op = is_immediate ? 0xb2 : 0xaa;
            }
            else if (i.ins == XOR)
            {
                op = is_immediate ? 0xd2 : 0xca;
            }

            if (is_immediate)
            {
                uint32_t val = 0;
                if (i.ops[2].isUsigc())
                {
                    val = (uint32_t)i.ops[2].getUsigc();
                }
                else
                {
                    int32_t tmp = (int32_t)i.ops[2].getSigc();
                    val = *((uint32_t *)&tmp);
                }

		assert(val < 0x1000);

		if (i.ins == AND || i.ins == OR || i.ins == XOR)
		{
		    // This is a) hacky and b) probably not right
		    assert (val < 0x10);
		    // N is 1 (64 bit element)
		    mc = (op << 24) | i.ops[0].getReg() | 0x1 << 22 | (i.ops[1].getReg() << 5) | (val << 10);
		}
		else
		{
		    mc = (op << 24) | i.ops[0].getReg() | (i.ops[1].getReg() << 5) | (val << 10);
		}
            }
            else
            {
		mc = (op << 24) | i.ops[0].getReg() | (i.ops[1].getReg() << 5) | (i.ops[2].getReg() << 16);
            }

            break;
        }
        case SHL:
        case SHR:
        case SAR:
        case RCL:
        case RCR:
        case ROL:
        case ROR: {
            assert(i.oc == 3);
            assert(i.ops[0].isReg());
            assert(i.ops[1].isReg());
            assert(i.ops[2].isReg());

	    uint32_t op = 0;
	    if (i.ins == SHL)
	    {
		op = 0x9ac02000;
	    }
	    else if (i.ins == SHR)
	    {
		op = 0x9ac02400;
	    }
	    else if (i.ins == SAR)
	    {
		op = 0x9ac02800;
	    }
	    else if (i.ins == ROR)
	    {
		op = 0x9ac02c00;
	    }
	    else
	    {
		printf("Do not support shift operation %lu!\n", i.ins);
		break;
	    }

	    mc = op | i.ops[0].getReg() | i.ops[1].getReg() << 5 | i.ops[2].getReg() << 16;
            break;
        }
        case CMP: {
            assert(i.oc == 2);
            assert(i.ops[0].isReg());
            assert(i.ops[1].isReg() || i.ops[1].isUsigc());

            if (i.ops[1].isReg())
            {
		mc = 0xeb00001f | i.ops[0].getReg() << 5 | i.ops[1].getReg() << 16;
	    }
	    else
	    {
		assert(i.ops[1].getUsigc() < 4096);
		mc = 0xf100001f | i.ops[1].getUsigc() << 10 | i.ops[0].getReg() << 5;
	    }

	    break;
        }
        case NOT:{
            assert(i.oc == 2);
            assert(i.ops[0].isReg());
            assert(i.ops[1].isReg());
	    mc = 0xaa2003e0 | i.ops[0].getReg() | i.ops[1].getReg() << 16;
            break;
        }
        case SELEQ:
        case SELGE:
        case SELGT:
        case SELGES:
        case SELGTS: {
            assert(i.oc == 3);
            assert(i.ops[0].isReg());
            assert(i.ops[1].isReg());
            assert(i.ops[2].isReg());
	    uint32_t cond = 0;
            if (i.ins == SELEQ)
            {
                cond = 0x0;
            }
            else if (i.ins == SELGE)
            {
		// This is the /signed/ one, need to figure this one out
		// Probably do the GT select and then a CMOV if EQ after
                cond = 0xa;
            }
            else if (i.ins == SELGT)
	    {
                cond = 0x8;
            }
            else if (i.ins == SELGES)
            {
                cond = 0xa;
            }
            else if (i.ins == SELGTS)
            {
                cond = 0xc;
            }
	    mc = 0x9a800000 | cond << 12 | i.ops[0].getReg() | i.ops[1].getReg() << 5 | i.ops[2].getReg() << 16;
            break;
        }
        case BRA: {
            assert(i.oc == 1);
            assert(i.ops[0].isReg() || i.ops[0].isBlock());
            if (i.ops[0].isReg())
            {
                mc = 0xd61f0000 | i.ops[0].getReg() << 5;
            }
            else
            {
                mc = 0x14000000;
                // Branch offset is stored >> 2
                BasicBlockRelocation *bbr =
                    new BasicBlockRelocation(image, current_function, flen(), flen(), i.ops[0].getBlock());
                bbr->addReloc(0, 2, 0x03ffffff, 0, 32);
            }
            break;
        }
        case BNE:
        case BEQ:
        case BG:
        case BLE:
        case BL:
        case BGE: {
            assert(i.oc == 1);
            assert(i.ops[0].isBlock());
	    uint32_t cond = 0;
            if (i.ins == BNE)
            {
                cond = 0x1;
            }
            else if (i.ins == BEQ)
            {
                cond = 0x0;
            }
            else if (i.ins == BG)
            {
                cond = 0xc;
            }
            else if (i.ins == BLE)
            {
                cond = 0xd;
            }
            else if (i.ins == BL)
            {
                cond = 0xb;
            }
            else if (i.ins == BGE)
            {
                cond = 0xa;
            }
	    mc = 0x54000000 | cond;
                // Branch offset is stored >> 2
            BasicBlockRelocation *bbr =
                new BasicBlockRelocation(image, current_function, flen(), flen(), i.ops[0].getBlock());
            bbr->addReloc(0, 2, 0x007ffff, 5, 32);
            break;
        }
        case DIV:
	case DIVS: {
            assert(i.ops[0].isReg());
            assert(i.ops[1].isReg());
            assert(i.ops[2].isReg());
	    mc = (i.ins == DIV ? 0x9ac00800 : 0x9ac00c00) | i.ops[0].getReg() | i.ops[1].getReg() << 5 | i.ops[2].getReg() << 16;
	    break;
	}
        case REM:
        case REMS: {
            assert(i.ops[0].isReg());
            assert(i.ops[1].isReg());
            assert(i.ops[2].isReg());
	    // (u|s)div followed by msub
	    uint32_t div = (i.ins == REM ? 0x9ac00800 : 0x9ac00c00);
	    div |= i.ops[0].getReg();
	    div |= i.ops[1].getReg() << 5;
	    div |= i.ops[2].getReg() << 16;
	    wee32(le, current, div);
	    mc = 0x9b008000 | i.ops[0].getReg() | i.ops[0].getReg() << 5 | i.ops[1].getReg() << 10 | i.ops[2].getReg() << 16;
            break;
        }
        case MUL:
	case MULS: {
            assert(i.ops[0].isReg());
            assert(i.ops[1].isReg());
            assert(i.ops[2].isReg());
	    mc = 0x9b007c00 | i.ops[0].getReg() | i.ops[1].getReg() << 5 | i.ops[2].getReg() << 16;
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
    // Assume same as ARM32 until proved different
    if (i.ins == DIV || i.ins == DIVS || i.ins == REM || i.ins == REMS || i.ins == SELEQ || i.ins == SELGT ||
        i.ins == SELGE || i.ins == SELGTS || i.ins == SELGES || i.ins == MUL || i.ins == MULS || i.ins == NOT ||
	i.ins == SHL || i.ins == SHR || i.ins == SAR || i.ins == AND || i.ins == OR || i.ins == XOR)
    {
        return false;
    }

    if (i.ins == ADD || i.ins == SUB)
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

void Arm64::newFunction(Codegen *c)
{
    Assembler::newFunction(c);
    if (c->callConvention() == CCONV_STANDARD)
    {
        uint64_t addr = c->stackSize();
        wle64(current, addr);
    }
}

void Arm64::align(uint64_t a)
{
    uint32_t nop = 0xd503201f;
    while (currentAddr() % a)
    {
        *((uint32_t *)current) = nop;
        current += 4;
    }
}

Value *Arm64LinuxSyscallCallingConvention::generateCall(Codegen *c, Value *fptr, std::vector<Value *> &args)
{
    BasicBlock *current = c->block();
    RegSet res;
    res.set(assembler->regnum("r0"));
    res.set(assembler->regnum("r1"));
    res.set(assembler->regnum("r2"));
    res.set(assembler->regnum("r3"));
    res.set(assembler->regnum("r4"));
    res.set(assembler->regnum("r5"));

    res.set(assembler->regnum("r8"));

    current->setReservedRegs(res);

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

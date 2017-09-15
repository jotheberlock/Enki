#include "thumb.h"
#include "platform.h"
#include "codegen.h"
#include "image.h"

bool Thumb::validRegOffset(Insn & i, int off)
{
    if (i.ins == LOAD8 || i.ins == LOADS8 || i.ins == STORE8)
    {
        return (off >= 0) && (off < 32);
    }
    else if (i.ins == LOAD16 || i.ins == LOADS16 || i.ins == STORE16)
    {
        return (off >= 0) && (off < 63);
    }
    else
    {
        int pos;
        if (i.ins == LOAD32 || i.ins == LOAD)
        {
            pos = 1;
        }
        else
        {
            pos = 0;
        }

        if (i.ops[pos].getReg() == 13)
        {
            return (off >= 0) && (off < 1021);
        }
        
        return (off >= 0) && (off < 125);
    }
    return false;
}

int Thumb::regnum(std::string s)
{
	int ret = -1;
	if (s[0] == 'r')
	{
		sscanf(s.c_str() + 1, "%d", &ret);
		assert(ret < 16);
	}
	else if (s == "ip")
	{
		ret = 12;
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

int Thumb::size(BasicBlock * b)
{
	int ret = 0;
	std::list<Insn> & code = b->getCode();

	for (std::list<Insn>::iterator it = code.begin(); it != code.end();
	it++)
	{
		Insn & i = *it;
		i.addr = address + len();

		switch (i.ins)
		{
            case ENTER_THUMB_MODE:
            {
                ret += 12;
                break;
            }
            case MOVE:
            {
                if (i.ops[1].isReg())
                {
                    ret += 2;
                }
                else
                {
                    ret += 10;
                }
            }
            default:
            {
                ret += 2;
                i.size = 2;
            }
		}
	}
    
	return ret;
}

bool Thumb::calcImm(uint64 raw, uint32 & result)
{
	// ARM encodes constants as an 8-bit value, rotated right by
	// 0-30 bits

	uint32 shift = 0;
	uint32 trial = raw & 0xff;
	for (shift = 0; shift < 31; shift++)
	{
		if (trial == raw)
		{
			result = (raw & 0xff) | (shift << 8);
			return true;
		}

		bool lsb_set = trial & 0x1;
		trial = trial >> 1;
		if (lsb_set)
		{
			trial = trial | 0x80;
		}
	}

	printf("Error, cannot encode %llx as an ARM constant!\n", raw);
	return false;
}

bool Thumb::assemble(BasicBlock * b, BasicBlock * next, Image * image)
{
	std::list<Insn> & code = b->getCode();

	b->setAddr(address + flen());

	uint64 current_addr = (uint64)current;
	assert((current_addr & 0x1) == 0);

	for (std::list<Insn>::iterator it = code.begin(); it != code.end();
	it++)
	{
		Insn & i = *it;
		i.addr = address + flen();

		uint16 mc = 0x46c0;  // nop

		unsigned char * oldcurrent = current;

		switch (i.ins)
		{
            case ENTER_THUMB_MODE:
            {
            
                    // Temporary hack - load r0 with pc then bx to Thumb
                wee32(le, current, 0xe1a0000f);   // mov r0, pc - r0 is now this instruction+8
                wee32(le, current, 0xe2800005);   // add r0, r0, #5 - make sure to set thumb bit
                wee32(le, current, 0xe12fff10);   // bx r0
                break;
            }
            
		case SYSCALL:
		{
            
			assert(i.oc == 1 || i.oc == 0);
			if (i.oc == 1)
			{
				assert(i.ops[0].isUsigc());
				assert(i.ops[0].getUsigc() <= 0xff);
				mc = 0xdf00 | (i.ops[0].getUsigc() & 0xff);
			}
			else
			{
				mc = 0xdf00;
			}
			break;
		}
		case BREAKP:
		{
			break;
		}
		case LOAD8:
		case LOADS8:
		case LOAD16:
		case LOADS16:
		case LOADS32:
		case LOAD:
		case LOAD32:
		{
			assert(i.oc == 2 || i.oc == 3);
			assert(i.ops[0].isReg());
			assert(i.ops[1].isReg());
			assert(i.ops[0].getReg() < 8);
            
			bool regreg = false;
			uint64 offset = 0;
			if (i.oc == 3)
			{
				if (i.ops[2].isReg())
				{
					regreg = true;
				}
				else
				{
					assert(i.ops[2].isUsigc() || i.ops[2].isSigc());
					if (i.ops[2].isUsigc())
					{
						offset = i.ops[2].getUsigc();
					}
					else
					{
						assert(i.ops[2].getSigc() >= 0);
						offset = i.ops[2].getSigc();
					}
				}
			}
            
			if ((i.ins == LOAD32 || i.ins == LOADS32 || i.ins == LOAD)
                && i.ops[1].getReg() == 13 && offset < 1021 && ((offset & 0x3) == 0))
			{
				mc = 0x9800 | i.ops[0].getReg() << 8 | (uint16)offset >> 2;
			}
            else if (i.ins == LOAD8 || i.ins == LOADS8)
            {
				if (regreg)
				{
					assert(i.ops[1].getReg() < 8);
					assert(i.ops[2].getReg() < 8);
					mc = (i.ins == LOAD8 ? 0x5c00 : 0x5600) | i.ops[0].getReg() | i.ops[1].getReg() << 3 | i.ops[2].getReg() << 6;
				}
				else
				{
					assert(offset < 32);
					if (i.ins == LOADS8)
					{
						assert(i.ops[2].isReg());
					}
					else
					{
						assert(i.ops[1].getReg() < 8);
						mc = 0x7800 | i.ops[1].getReg() << 3 | i.ops[0].getReg();
						if (i.oc == 3)
						{
							mc |= offset << 6;
						}
					}
				}
            }
            else if (i.ins == LOAD16 || i.ins == LOADS16)
            {
				if (regreg)
				{
					assert(i.ops[1].getReg() < 8);
					assert(i.ops[2].getReg() < 8);
					mc = (i.ins == LOAD16 ? 0x5a00 : 0x5e00) | i.ops[0].getReg() | i.ops[1].getReg() << 3 | i.ops[2].getReg() << 6;
				}
				else
				{
					assert(offset < 63);
					if (i.ins == LOADS16)
					{
						assert(i.ops[2].isReg());
					}
					else
					{
						assert(i.ops[1].getReg() < 8);
						mc = 0x8800 | i.ops[1].getReg() << 3 | i.ops[0].getReg();
						if (i.oc == 3)
						{
							assert((offset & 0x1) == 0);
							offset >>= 1;
							mc |= offset << 6;
						}
					}
				}
            }
            else if (i.ins == LOAD32 || i.ins == LOADS32 || i.ins == LOAD)
            {
				if (regreg)
				{
					assert(i.ops[1].getReg() < 8);
					assert(i.ops[2].getReg() < 8);
					mc = 0x5800 | i.ops[0].getReg() | i.ops[1].getReg() << 3 | i.ops[2].getReg() << 6;
				}
				else
				{
					assert(offset < 125);
					if (i.ins == LOADS32)
					{
						assert(i.ops[2].isReg());
					}
					else
					{
						assert(i.ops[1].getReg() < 8);
						mc = 0x6800 | i.ops[1].getReg() << 3 | i.ops[0].getReg();
						if (i.oc == 3)
						{
							assert((offset & 0x3) == 0);
							offset >>= 2;
							mc |= offset << 6;
						}
					}
				}
            }
			break;
		}
		case STORE8:
		case STORE16:
		case STORE:
		case STORE32:
		{
			assert(i.oc == 2 || i.oc == 3);
			assert(i.ops[0].isReg());
			assert(i.ops[i.oc == 3 ? 2 : 1].isReg());
			assert(i.ops[i.oc == 3 ? 2 : 1].getReg() < 8);
			uint64 offset = 0;
            int sreg = (i.oc == 3) ? 2 : 1;
			bool regreg = false;
			if (i.oc == 3)
			{
				if (i.ops[2].isReg())
				{
					regreg = true;
				}
				else
				{
					assert(i.ops[1].isUsigc() || i.ops[1].isSigc());
					if (i.ops[1].isUsigc())
					{
						offset = i.ops[1].getUsigc();
					}
					else
					{
						assert(i.ops[1].getSigc() >= 0);
						offset = i.ops[1].getSigc();
					}
				}
			}

			if ((i.ins == STORE32 || i.ins == STORE) && 
				i.ops[0].getReg() == 13 && 
				offset < 1021 && 
				(offset & 0x3) == 0)
			{
				mc = 0x9000 | i.ops[(i.oc == 3 ? 2 : 1)].getReg() << 8 | (uint16)offset >> 2;
			}
            else if (i.ins == STORE8)
            {
				if (regreg)
				{
					assert(i.ops[1].getReg() < 8);
					assert(i.ops[2].getReg() < 8);
					mc = 0x5400 | i.ops[2].getReg() | i.ops[0].getReg() << 3 | i.ops[1].getReg() << 6;
				}
				else
				{
					assert(offset < 32);

					assert(i.ops[sreg].getReg() < 8);
					mc = 0x7000 | i.ops[0].getReg() << 3 | i.ops[sreg].getReg();
					if (i.oc == 3)
					{
						mc |= offset << 6;
					}
				}
            }
            else if (i.ins == STORE16)
            {
				if (regreg)
				{
					assert(i.ops[1].getReg() < 8);
					assert(i.ops[2].getReg() < 8);
					mc = 0x5200 | i.ops[2].getReg() | i.ops[0].getReg() << 3 | i.ops[1].getReg() << 6;
				}
				else
				{
					assert(offset < 63);
					assert(i.ops[sreg].getReg() < 8);
					mc = 0x8000 | i.ops[0].getReg() << 3 | i.ops[sreg].getReg();
					if (i.oc == 3)
					{
						assert((offset & 0x1) == 0);
						offset >>= 1;
						mc |= offset << 5;
					}
				}
            }
            else if (i.ins == STORE32 || i.ins == STORE)
            {
				if (regreg)
				{
					assert(i.ops[1].getReg() < 8);
					assert(i.ops[2].getReg() < 8);
					mc = 0x5000 | i.ops[2].getReg() | i.ops[0].getReg() << 3 | i.ops[1].getReg() << 6;
				}
				else
				{
					assert(offset < 125);
					assert(i.ops[sreg].getReg() < 8);
					mc = 0x6000 | i.ops[0].getReg() << 3 | i.ops[sreg].getReg();
					if (i.oc == 3)
					{
						assert((offset & 0x3) == 0);
						offset >>= 2;
						mc |= offset << 5;
					}
				}
            }
            
			break;
		}
		case LOAD64:
		case STORE64:
		{
			printf("64-bit load/store attempted on 32-bit ARM!\n");
            assert(false);
			break;
		}
		case MOVE:
		{
			assert(i.oc == 2);

			assert(i.ops[0].isReg());
			assert(i.ops[1].isUsigc() || i.ops[1].isSigc() ||
				i.ops[1].isFunction() || i.ops[1].isReg() ||
				i.ops[1].isBlock() || i.ops[1].isSection() || i.ops[1].isExtFunction());

			if (i.ops[1].isUsigc() || i.ops[1].isSigc() ||
				i.ops[1].isFunction() || i.ops[1].isBlock() ||
				i.ops[1].isSection() || i.ops[1].isExtFunction())
			{
                if (i.ops[1].isUsigc() && i.ops[1].getUsigc() < 256)
                {
                    mc = 0x2000;
                    mc |= i.ops[0].getReg() << 8;
                    mc |= i.ops[1].getUsigc();
                }
                else
                {
                    assert(i.ops[0].getReg() < 8);

                    if (!(current_addr & 0x3))
                    {
                        wee16(le, current, 0x46c0); // 32-bit align with nop
                    }

                        // ldr r<x>, pc+0 (which is this instruction+4)
                    wee16(le, current, 0x4800 | (i.ops[0].getReg() << 8));
                        // branch over constant
                    wee16(le, current, 0xe001);

                    if (i.ops[1].isFunction())
                    {
                        uint32 reloc = 0xdeadbeef;
                        FunctionRelocation * fr = new FunctionRelocation(image, current_function, flen(), i.ops[1].getFunction(), 0);
                        fr->add32();
                        wle32(current, reloc);
                    }
                    else if (i.ops[1].isBlock())
                    {
                        uint32 reloc = 0xdeadbeef;
                        AbsoluteBasicBlockRelocation * abbr = new AbsoluteBasicBlockRelocation(image, current_function, flen(), i.ops[1].getBlock());
                        abbr->add32();
                        wle32(current, reloc);
                    }
                    else if (i.ops[1].isSection())
                    {
                        uint32 reloc = 0xdeadbeef;
                        int s;
                        uint64 o = i.ops[1].getSection(s);
                        SectionRelocation * sr = new SectionRelocation(image, IMAGE_CODE, len(), s, o);
                        sr->add32();
                        wle32(current, reloc);
                    }
                    else if (i.ops[1].isExtFunction())
                    {
                        uint32 reloc = 0xdeadbeef;
                        ExtFunctionRelocation * efr = new ExtFunctionRelocation(image, current_function, len(), i.ops[1].getExtFunction());
                        efr->add32();
                        wle32(current, reloc);
                    }
                    else
                    {
                        uint32 val;
                        if (i.ops[1].isUsigc())
                        {
                            val = i.ops[1].getUsigc();
                        }
                        else if (i.ops[1].isSigc())
                        {
                            int32 sval = i.ops[1].getSigc();
                            val = *((uint32 *)&sval);
                        }
                        wle32(current, val);
                    }

                    break;
                }    
            }
            else
            {
                if (i.ops[0].getReg() < 8 && i.ops[1].getReg() < 8)
                {
                        // Actually an add of 0
                    mc = 0x1c00;
                    mc |= i.ops[0].getReg();
                    mc |= i.ops[1].getReg() << 3;
                }
                else
                {
                    mc = 0x4600;
                    mc |= (i.ops[1].getReg() & 0x7) <<  3;
                    mc |= i.ops[0].getReg() & 0x7;
                    if (i.ops[1].getReg() > 7)
                    {
                        mc |= 0x40;
                    }
                    if (i.ops[0].getReg() > 7)
                    {
                        mc |= 0x80;
                    }
                }    
            }
            
			break;
		}
		case ADD:
		case SUB:
        {
			assert(i.oc == 3);
			assert(i.ops[0].isReg());
			assert(i.ops[1].isReg());
			assert(i.ops[2].isReg() || i.ops[2].isUsigc());

            if (i.ops[2].isUsigc())
            {
                assert(i.ops[0].getReg() == i.ops[1].getReg());
                assert(i.ops[0].getReg() < 8);
                assert(i.ops[2].getUsigc() < 256);
                mc = (i.ins == ADD) ? 0x3000 : 0x3800;
                mc |= i.ops[0].getReg() << 8;
                mc |= i.ops[2].getUsigc();
            }
            else
            {
                if (i.ops[0].getReg() < 8 && i.ops[1].getReg() < 8 &&
                    i.ops[2].getReg() < 8)
                {
                    mc = (i.ins == ADD) ? 0x1800 : 0x1a00;
                    mc |= i.ops[1].getReg() << 3;
                    mc |= i.ops[2].getReg() << 6;
                    mc |= i.ops[0].getReg();
                }
            }
            
			break;
		}
		case AND:
		case OR:
		case XOR:
		{
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
			break;
		}
		case CMP:
		{
			break;
		}
		case NOT:
		{
			break;
		}
		case SELEQ:
		case SELGE:
		case SELGT:
		case SELGES:
		case SELGTS:
		{
			assert(i.oc == 3);
			assert(i.ops[0].isReg());
			assert(i.ops[1].isReg());
			break;
		}
		case BRA:
		{
			assert(i.oc == 1);
			assert(i.ops[0].isReg() || i.ops[0].isBlock());
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
			break;
		}
		case DIV:
		case DIVS:
		{
            assert(i.oc == 3);
			break;
		}
		case REM:
		case REMS:
		{
			assert(false);  // Doesn't exist on ARM
			break;
		}
		case MUL:
		{
            assert(i.oc == 3);
			break;
		}
		case MULS:
		{
			assert(false);   // Need to handle 64 bit result
			break;
		}
		default:
		{
			fprintf(log_file, "Don't know how to turn %lld [%s] into arm!\n", i.ins, i.toString().c_str());
			assert(false);
		}

		unsigned int siz = (unsigned int)(current - oldcurrent);
		if (siz > i.size)
		{
			printf("Unexpectedly large instruction! estimate %lld actual %d %s\n",
				i.size, siz, i.toString().c_str());
		}
		}

		wee16(le, current, mc);
		if (current >= limit)
		{
			printf("Ran out of space to assemble into, %d\n", (int)(limit - base));
			fprintf(log_file, "Ran out of space to assemble into, %d\n", (int)(limit - base));
			return false;
		}
	}

	return true;
}

std::string Thumb::transReg(uint32 r)
{
	assert(r < 16);
	if (r < 12)
	{
		char buf[4096];
		sprintf(buf, "r%d", r);
		return buf;
	}
	else if (r == 12)
	{
		return "ip";
	}
	else if (r == 13)
	{
		return "sp";
	}
	else if (r == 14)
	{
		return "lr";
	}
	else if (r == 15)
	{
		return "pc";
	}
	else
	{
		assert(false);
	}

	return ""; // Shut compiler up
}

bool Thumb::configure(std::string param, std::string val)
{
	if (param == "bits")
	{
		return false;
	}

	return Assembler::configure(param, val);
}

ValidRegs Thumb::validRegs(Insn & i)
{
	ValidRegs ret;

	for (int loopc = 0; loopc < 16; loopc++)
	{
		if (loopc != 15 && loopc != 13 && loopc != framePointer())  // pc, sp
		{
			ret.ops[0].set(loopc);
			ret.ops[1].set(loopc);
			ret.ops[2].set(loopc);
		}
	}

	return ret;
}

bool Thumb::validConst(Insn & i, int idx)
{
	return true;
}

void Thumb::newFunction(Codegen * c)
{
	Assembler::newFunction(c);
	if (c->callConvention() == CCONV_STANDARD)
	{
		uint64 addr = c->stackSize();
		wle32(current, checked_32(addr));
	}
}

void Thumb::align(uint64 a)
{
	uint32 nop = 0xf3af1000;
	while (currentAddr() % a)
	{
		*((uint32 *)current) = nop;
		current += 4;
	}
}

Value * ThumbLinuxSyscallCallingConvention::generateCall(Codegen * c,
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

	for (unsigned int loopc = 0; loopc < args.size(); loopc++)
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





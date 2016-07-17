#include "arm32.h"
#include "platform.h"
#include "codegen.h"
#include "image.h"

int Arm32::regnum(std::string s)
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

int Arm32::size(BasicBlock * b)
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

bool Arm32::assemble(BasicBlock * b, BasicBlock * next, Image * image)
{
	std::list<Insn> & code = b->getCode();

	b->setAddr(address + flen());

	uint64 current_addr = (uint64)current;
	assert((current_addr & 0x3) == 0);

	for (std::list<Insn>::iterator it = code.begin(); it != code.end();
	it++)
	{
		Insn & i = *it;
		i.addr = address + flen();

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
				assert(i.ops[0].getUsigc() <= 0xffffff);
				mc = (0xe << 28) | (0xf << 24) | (uint32)i.ops[0].getUsigc();
			}
			else
			{
				mc = (0xe << 28) | (0xf << 24);  // assume SWI 0 if not specified
			}
			break;
		}
		case BREAKP:
		{
			mc = 0xe1200070;
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

			int32 val = 0;
			uint32 uval = 0;
			if (i.oc == 3)
			{
				val = (int32)i.ops[2].getSigc();
				// Can probably be higher for 32-bits...
				assert(val > -256);
				assert(val < 256);
				uval = *((uint32 *)(&val));
			}

			if (i.ins == LOAD || i.ins == LOAD32 || i.ins == LOADS32)
			{
				mc = 0xe5900000 | (uval & 0xfff) | i.ops[0].getReg() << 12
					| i.ops[1].getReg() << 16;
			}
			else if (i.ins == LOAD8)
			{
				mc = 0xe5d00000 | (uval & 0xfff) | i.ops[0].getReg() << 12
					| i.ops[1].getReg() << 16;
			}
			else if (i.ins == LOAD16)
			{
				mc = 0xe1d000b0 | (uval & 0xf) | (uval & 0xf0 << 4) |
					i.ops[0].getReg() << 12 || i.ops[1].getReg() << 16;
			}
			else if (i.ins == LOADS8)
			{
				mc = 0xe1d000d0 | (uval & 0xf) | (uval & 0xf0 << 4) |
					i.ops[0].getReg() << 12 || i.ops[1].getReg() << 16;
			}
			else if (i.ins == LOADS16)
			{
				mc = 0xe1d000f0 | (uval & 0xf) | (uval & 0xf0 << 4) |
					i.ops[0].getReg() << 12 || i.ops[1].getReg() << 16;
			}
			break;
		}
		case STORE8:
		case STORE16:
		case STORE:
		case STORE32:
		{
			assert(i.oc == 2 || i.oc == 3);
			int32 val = 0;
			uint32 uval = 0;

			int dest = 1;
			if (i.oc == 3)
			{
				val = (int32)i.ops[1].getSigc();
				// Can probably be higher for 32-bits...
				assert(val > -256);
				assert(val < 256);
				uval = *((uint32 *)(&val));
				dest = 2;
			}

			if (i.ins == STORE || i.ins == STORE32)
			{
				mc = 0xe5800000 | (uval & 0xfff) | i.ops[dest].getReg() << 12
					| i.ops[0].getReg() << 16;
			}
			else if (i.ins == STORE16)
			{
				mc = 0xe1c000b0 | (uval & 0xf) | (uval & 0xf0 << 4) |
					i.ops[dest].getReg() << 12 || i.ops[0].getReg() << 16;
			}
			else if (i.ins == STORE8)
			{
				mc = 0xe5c00000 | (uval & 0xfff) |
					i.ops[dest].getReg() << 12 | i.ops[0].getReg() << 16;
			}

			break;
		}
		case LOAD64:
		case STORE64:
		{
			printf("64-bit load/store attempted on 32-bit ARM!\n");
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
				uint32 val = 0x0;
				BaseRelocation * br = 0;
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
					uint64 o = i.ops[1].getSection(s);
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
						val = (uint32)i.ops[1].getUsigc();
					}
					else
					{
						int32 tmp = (int32)i.ops[1].getSigc();
						val = *((uint32 *)&tmp);
					}
				}

				if (br)
				{
					br->addReloc(0, 0, 0xfff, 0, false);
					br->addReloc(0, 12, 0xf, 16, false);
					br->addReloc(4, 16, 0xfff, 0, false);
					br->addReloc(4, 28, 0xf, 16, false);
				}

				// ARMv7 movw/movt
				mc = 0xe3000000 | (val & 0xfff) | (((val >> 12) & 0xf) << 16) | (i.ops[0].getReg() << 12);
				wee32(le, current, mc);
				val = val >> 16;
				mc = 0xe3400000 | (val & 0xfff) | (((val >> 12) & 0xf) << 16) | (i.ops[0].getReg() << 12);
			}
			else
			{
				mc = 0xe1a00000 | (i.ops[1].getReg()) |
					i.ops[0].getReg() << 12;
			}

			break;
		}
		case ADD:
		case SUB:
		case AND:
		case OR:
		case XOR:
		{
			assert(i.oc == 3);
			assert(i.ops[0].isReg());
			assert(i.ops[1].isReg());
			assert(i.ops[2].isReg() || i.ops[2].isUsigc() ||
				i.ops[2].isSigc());

			uint32 op = 0;
			if (i.ins == ADD)
			{
				op = 0x00800000;
			}
			else if (i.ins == SUB)
			{
				op = 0x00400000;
			}
			else if (i.ins == AND)
			{
				op = 0x00000000;
			}
			else if (i.ins == OR)
			{
				op = 0x01800000;
			}
			else if (i.ins == XOR)
			{
				op = 0x00200000;
			}

			if (i.ops[2].isUsigc() || i.ops[2].isSigc())
			{
				uint32 val = 0;
				if (i.ops[2].isUsigc())
				{
					val = (uint32)i.ops[2].getUsigc();
				}
				else
				{
					int32 tmp = (int32)i.ops[2].getSigc();
					val = *((uint32 *)&tmp);
				}
				uint32 cooked = 0;
				assert(calcImm(val, cooked));
				mc = 0xe2000000 | op | (i.ops[0].getReg() << 12) | (i.ops[1].getReg() << 16) | cooked;
			}
			else
			{
				mc = 0xe0000000 | op | (i.ops[0].getReg() << 12) | (i.ops[1].getReg() << 16) | i.ops[2].getReg();
			}

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
			assert(i.oc == 3);
			assert(i.ops[0].isReg());
			assert(i.ops[1].isReg());
			assert(i.ops[2].isReg() || i.ops[2].isUsigc());
			uint32 op = 0;
			uint32 const_shift = (i.ops[2].isUsigc()) ?
				(uint32)i.ops[2].getUsigc() : 0;

			if (i.ops[2].isUsigc())
			{
				assert(const_shift < 32);
				assert(const_shift > 0);
			}

			if (i.ins == SHL)
			{
				op = 0xe1a00000;
			}
			else if (i.ins == SHR)
			{
				op = 0xe1a00020;
			}
			else if (i.ins == SAR)
			{
				op = 0xe1a00040;
			}
			else if (i.ins == RCL)
			{
				// ? carry
				assert(false);
			}
			else if (i.ins == RCR)
			{
				// ? carry
				assert(false);
			}
			else if (i.ins == ROL)
			{
				op = 0xe1a00060;
				const_shift = 32 - const_shift;
			}
			else if (i.ins == ROR)
			{
				op = 0xe1a00060;
			}

			mc = op | i.ops[0].getReg() << 12 | i.ops[1].getReg();

			if (i.ops[2].isReg())
			{
				mc |= 0x10;
				mc |= i.ops[2].getReg() << 8;
			}
			else
			{
				mc |= const_shift << 7;
			}
			break;
		}
		case CMP:
		{
			assert(i.oc == 2);
			assert(i.ops[0].isReg());
			assert(i.ops[1].isReg() || i.ops[1].isUsigc() || i.ops[1].isSigc());
			if (i.ops[1].isUsigc() || i.ops[1].isSigc())
			{
				uint32 val = 0;
				if (i.ops[1].isUsigc())
				{
					val = (uint32)i.ops[1].getUsigc();
				}
				else
				{
					int32 tmp = (int32)i.ops[1].getSigc();
					val = *((uint32 *)&tmp);
				}
				uint32 cooked = 0;
				assert(calcImm(val, cooked));
				mc = 0xe3500000 | (i.ops[0].getReg() << 16) | cooked;
			}
			else
			{
				mc = 0xe1500000 | (i.ops[0].getReg() << 16) | i.ops[1].getReg();
			}

			break;
		}
		case NOT:
		{
			assert(i.oc == 2);
			assert(i.ops[0].isReg());
			assert(i.ops[1].isReg());
			mc = 0xe1e00000 | (i.ops[0].getReg() << 12) |
				(i.ops[1].getReg() << 16);
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
			assert(i.ops[2].isReg());
			uint32 op1 = 0;
			uint32 op2 = 0;
			if (i.ins == SELEQ)
			{
				op1 = 0x00000000;
				op2 = 0x10000000;
			}
			else if (i.ins == SELGE)
			{
				op1 = 0x20000000;
				op2 = 0x30000000;
			}
			else if (i.ins == SELGT)
			{
				op1 = 0x80000000;
				op2 = 0x90000000;
			}
			else if (i.ins == SELGES)
			{
				op1 = 0xa0000000;
				op2 = 0xb0000000;
			}
			else if (i.ins == SELGTS)
			{
				op1 = 0xc0000000;
				op2 = 0xd0000000;
			}

			mc = op1 | 0x01a00000 | (i.ops[1].getReg()) |
				i.ops[0].getReg() << 12;
			wee32(le, current, mc);
			mc = op2 | 0x01a00000 | (i.ops[2].getReg()) |
				i.ops[0].getReg() << 12;
			break;
		}
		case BRA:
		{
			assert(i.oc == 1);
			assert(i.ops[0].isReg() || i.ops[0].isBlock());
			if (i.ops[0].isReg())
			{
				// mov pc, <reg>
				mc = 0xe1a0f000 | i.ops[0].getReg();
			}
			else
			{
				mc = 0xea000000;
				// Branch offset is stored >> 2
				BasicBlockRelocation * bbr = new BasicBlockRelocation(image,
					current_function, flen(), flen() + 8, i.ops[0].getBlock());
				bbr->addReloc(0, 2, 0x00ffffff, 0, false);
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
			uint32 cond = 0;
			if (i.ins == BNE)
			{
				cond = 0x10000000;
			}
			else if (i.ins == BEQ)
			{
				cond = 0x00000000;
			}
			else if (i.ins == BG)
			{
				cond = 0x80000000;
			}
			else if (i.ins == BLE)
			{
				cond = 0x90000000;
			}
			else if (i.ins == BL)
			{
				cond = 0x30000000;
			}
			else if (i.ins == BGE)
			{
				cond = 0x20000000;
			}

			mc = cond | 0x0a000000;
			BasicBlockRelocation * bbr = new BasicBlockRelocation(image,
				current_function, flen(), flen() + 8, i.ops[0].getBlock());
			bbr->addReloc(0, 2, 0x00ffffff, 0, false);
			break;
		}
		case DIV:
		case DIVS:
		{
			mc = (i.ins == DIV) ? 0xe730f010 : 0x3710f010;
			mc = mc | i.ops[0].getReg() << 16 | i.ops[1].getReg() |
				i.ops[2].getReg() << 8;
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
			mc = 0xe0000090 | i.ops[0].getReg() << 16 |
				i.ops[1].getReg() | i.ops[2].getReg() << 8;
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
			mc = 0xe1a00000; // NOP
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
			printf("Ran out of space to assemble into, %d\n", (int)(limit - base));
			fprintf(log_file, "Ran out of space to assemble into, %d\n", (int)(limit - base));
			return false;
		}
	}

	return true;
}

std::string Arm32::transReg(uint32 r)
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

bool Arm32::validConst(Insn & i, int idx)
{
	if (i.ins == DIV || i.ins == DIVS || i.ins == REM ||
		i.ins == REMS || i.ins == SELEQ || i.ins == SELGT ||
		i.ins == SELGE || i.ins == SELGTS || i.ins == SELGES ||
		i.ins == MUL || i.ins == MULS || i.ins == NOT)
	{
		return false;
	}

	if (i.ins == ADD || i.ins == SUB || i.ins == MUL
		|| i.ins == MULS || i.ins == AND || i.ins == OR
		|| i.ins == XOR || i.ins == SHL || i.ins == SHR || i.ins == SAR)
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






#include <assert.h>
#include "amd64.h"
#include "platform.h"
#include "codegen.h"
#include "ast.h"
#include "cfuncs.h"
#include "symbols.h"
#include "image.h"

bool Amd64::validRegOffset(Insn & i, int off)
{
    return true;
    
    if (off < 0)
    {
        return false;
    }
    
    if (i.ins == LOAD8 || i.ins == LOADS8 || i.ins == STORE8)
    {
        return (off < 32);
    }
    else if (i.ins == LOAD16 || i.ins == LOADS16 || i.ins == STORE16)
    {
        return (off < 64) && ((off & 0x1) == 0);
    }
    else
    {
    
        return (off < 128) && ((off & 0x3) == 0);
    }
    return false;
}

const char * regnames[] =
{
	"rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
	"r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
	0
};

unsigned char reg(uint32 r)
{
	return r;
}

unsigned char rexreg(uint32 r1, uint32 r2)
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
	while (regnames[count] != 0)
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

bool Amd64::configure(std::string param, std::string val)
{
	if (param == "bits" || param == "endian")
	{
		return false;
	}

	return Assembler::configure(param, val);
}

ValidRegs Amd64::validRegs(Insn & i)
{
	ValidRegs ret;

	bool useAll = true;
	if (i.ins == MUL || i.ins == MULS || i.ins == DIV || i.ins == DIVS)
	{
		ret.ops[0].set(0);  // Must be RAX
		ret.ops[1].set(0);
		ret.clobbers.set(2);
		useAll = false;
	}
	else if (i.ins == REM || i.ins == REMS)
	{
		// Ugh how to handle. Input goes into RAX, output comes
		// out in RDX.
		ret.ops[0].set(0);  // Must be RDX
		ret.ops[1].set(0);
		ret.clobbers.set(2);
		useAll = false;
	}
	else if (i.ins == SHL || i.ins == SHR || i.ins == SAR)
	{
		if (!i.ops[2].isSigc() && !i.ops[2].isUsigc())
		{
			// Must be RCX
			ret.ops[2].set(1);
		}
	}

	for (int loopc = 0; loopc < 16; loopc++)
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

bool Amd64::validConst(Insn & i, int idx)
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

int Amd64::size(BasicBlock * b)
{
	int ret = 0;
	std::list<Insn> & code = b->getCode();

	for (std::list<Insn>::iterator it = code.begin(); it != code.end();
	it++)
	{
		Insn & i = *it;
		i.addr = address + len();

		int oldret = ret;

		switch (i.ins)
		{
		case STORE8:
		case STORE16:
		{
			if (i.ins == STORE16)
			{
				ret++;
			}

			ret++;

			if (i.ins == STORE8)
			{
				ret++;
			}
			else if (i.ins == STORE16)
			{
				ret++;
			}

			ret += (i.oc == 3) ? 7 : 3;  // Conservative estimates for now
			break;
		}
		case LOAD8:
		case LOAD16:
		case LOADS8:
		case LOADS16:
		{
			ret += (i.oc == 3) ? 9 : 6;
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
			ret += (i.oc == 3) ? 8 : 7;
			break;
		}
		case MOVE:
		{
			if (i.ops[1].isUsigc() || i.ops[1].isSigc() ||
				i.ops[1].isFunction() || i.ops[1].isBlock() ||
				i.ops[1].isSection() || i.ops[1].isExtFunction())
			{
				ret++;
				if (i.ops[1].isFunction())
				{
					ret += 9;
				}
				else if (i.ops[1].isBlock())
				{
					ret += 9;
				}
				else if (i.ops[1].isSection())
				{
					ret += 9;
				}
				else if (i.ops[1].isExtFunction())
				{
					ret += 9;
				}
				else
				{
					uint64 val;
					if (i.ops[1].isUsigc())
					{
						val = i.ops[1].getUsigc();
					}
					else
					{
						int64 tmp = i.ops[1].getSigc();
						val = *((uint64 *)&tmp);
					}

					bool big = (val > 0xffffffff);
					if (big)
					{
						ret += 9;
					}
					else
					{
						ret += 6;
					}
				}
			}
			else
			{
				ret += 4;
			}

			break;
		}
		case ADD:
		case SUB:
		case AND:
		case OR:
		case XOR:
		{
			if (i.ops[2].isUsigc() || i.ops[2].isSigc())
			{
				ret += 7;
			}
			else
			{
				ret += 3;
			}
			break;
		}
		case NOT:
		{
			ret += 3;
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
			ret += 3;
			break;
		}
		case RET:
		{
			ret++;
			break;
		}
		case MUL:
		{
			ret += 3;
			break;
		}
		case CMP:
		{
			if (i.ops[1].isReg())
			{
				ret += 3;
			}
			else
			{
				ret += 7;
			}

			break;
		}
		case BRA:
		case CALL:
		{
			if (i.ops[0].isReg())
			{
				if (i.ops[0].getReg() > 7)
				{
					ret++;
				}
				ret += 3;
			}
			else
			{
				ret += 5;
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
			ret += 6;
			break;
		}
		case SYSCALL:
		{
			ret += 2;
			break;
		}
		case BREAKP:
		{
			ret++;
			break;
		}
		case DIV:
		case DIVS:
		case REM:
		case REMS:
		{
			ret += 3;
			if (i.ins == REM || i.ins == REMS)
			{
				// hack MOV RAX RDX
				ret += 3;
			}

			break;
		}
		case SELEQ:
		case SELGE:
		case SELGT:
		case SELGES:
		case SELGTS:
		{
			ret += 8;
			break;
		}
		default:
		{
			fprintf(log_file, "Don't know how to turn %lld into amd64!\n", i.ins);
			assert(false);
		}
		}

		i.size = (ret - oldret);
	}

	return ret;
}

bool Amd64::assemble(BasicBlock * b, BasicBlock * next, Image * image)
{
	std::list<Insn> & code = b->getCode();

	b->setAddr(address + flen());

	for (std::list<Insn>::iterator it = code.begin(); it != code.end();
	it++)
	{
		Insn & i = *it;
		i.addr = address + flen();

		unsigned char * oldcurrent = current;

		switch (i.ins)
		{
		case STORE8:
		case STORE16:
		{
			assert(i.oc == 2 || i.oc == 3);

			int sr = 0;
			int dr = 0;
			int oreg = 0;

			int32 offs = 0;

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
				offs = (int32)i.ops[oreg].getSigc();
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

			int32 offs = 0;

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
				offs = (int32)i.ops[oreg].getSigc();
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

			int32 offs = 0;

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
				offs = (int32)i.ops[oreg].getSigc();
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

			assert(i.ops[0].isReg());
			assert(i.ops[1].isUsigc() || i.ops[1].isSigc() ||
				i.ops[1].isFunction() || i.ops[1].isReg() ||
				i.ops[1].isBlock() || i.ops[1].isSection() || i.ops[1].isExtFunction());

			if (i.ops[1].isUsigc() || i.ops[1].isSigc() ||
				i.ops[1].isFunction() || i.ops[1].isBlock() ||
				i.ops[1].isSection() || i.ops[1].isExtFunction())
			{
				unsigned char rex = 0x48;
				if (i.ops[0].getReg() > 7)
				{
					rex |= 0x1;
				}
				*current++ = rex;

				if (i.ops[1].isFunction())
				{
					uint64 reloc = 0xdeadbeefdeadbeefLL;
					unsigned char r = 0xb8;
					r |= reg(i.ops[0].getReg() & 0x7);
					*current++ = r;

					assert(current_function);
					FunctionRelocation * fr = new FunctionRelocation(image, current_function, flen(), i.ops[1].getFunction(), 0);
					fr->add64();
					//relocs.push_back(Relocation(REL_A64, len()+8, len(),
					//                            i.ops[1].getFunction()));
					wle64(current, reloc);
				}
				else if (i.ops[1].isBlock())
				{
					uint64 reloc = 0xdeadbeefdeadbeefLL;
					unsigned char r = 0xb8;
					r |= reg(i.ops[0].getReg() & 0x7);
					*current++ = r;

					AbsoluteBasicBlockRelocation * abbr = new AbsoluteBasicBlockRelocation(image, current_function, flen(), i.ops[1].getBlock());
					abbr->add64();
					//relocs.push_back(Relocation(REL_A64, len()+8, len(),
					//                            i.ops[1].getBlock()));
					wle64(current, reloc);
				}
				else if (i.ops[1].isSection())
				{
					uint64 reloc = 0xdeadbeefdeadbeefLL;
					unsigned char r = 0xb8;
					r |= reg(i.ops[0].getReg() & 0x7);
					*current++ = r;

					int s;
					uint64 o = i.ops[1].getSection(s);
					SectionRelocation * sr = new SectionRelocation(image, IMAGE_CODE, len(), s, o);
					sr->add64();
					//relocs.push_back(Relocation(REL_A64, len()+8, len(),
					//                            i.ops[1].getBlock()));
					wle64(current, reloc);
				}
				else if (i.ops[1].isExtFunction())
				{
					uint64 reloc = 0xdeadbeefdeadbeefLL;
					unsigned char r = 0xb8;
					r |= reg(i.ops[0].getReg() & 0x7);
					*current++ = r;
					ExtFunctionRelocation * efr = new ExtFunctionRelocation(image, current_function, len(), i.ops[1].getExtFunction());
					efr->add64();
					//relocs.push_back(Relocation(REL_A64, len()+8, len(),
					//                            i.ops[1].getBlock()));
					wle64(current, reloc);
				}
				else
				{
					uint64 val;
					if (i.ops[1].isUsigc())
					{
						val = i.ops[1].getUsigc();
					}
					else
					{
						int64 tmp = i.ops[1].getSigc();
						val = *((uint64 *)&tmp);
					}

					unsigned char r = 0xb8;
					r |= reg(i.ops[0].getReg() & 0x7);
					*current++ = r;
					wle64(current, val);
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

				switch (i.ins)
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
					fprintf(log_file, "Unknown logop %lld!\n", i.ins);
					break;
				}

				if (i.ops[2].isSigc())
				{
					int64 val = i.ops[2].getSigc();
					int32 v = (int32_t)val;
					uint32 * ptr = (uint32 *)&v;
					wle32(current, *ptr);
				}
				else
				{
					uint64 imm = i.ops[2].getUsigc();
					assert(imm < 0x100000000LL);
					wle32(current, (uint32)imm);
				}
			}
			else
			{
				*current++ = 0x48 | rexreg(i.ops[2].getReg(), i.ops[0].getReg());

				switch (i.ins)
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
					fprintf(log_file, "Unknown logop %lld!\n", i.ins);
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
			assert(i.oc == 3);

			unsigned char rr = reg(i.ops[0].getReg()) & 0x7;

			if (i.ops[0].isReg() && i.ops[1].isReg() && i.ops[2].isReg())
			{
				assert(i.ins == SHL || i.ins == SHR || i.ins == SAR);
				if (i.ins == SAR)
				{
					if (reg(i.ops[0].getReg()) < 8)
					{
						*current++ = 0x48;
					}
					else
					{
						*current++ = 0x49;
					}
					*current++ = 0xd3;
					*current++ = 0xf8 | rr;
				}
				else
				{
					unsigned char rr1 = reg(i.ops[1].getReg()) & 0x7;
					*current++ = 0x48 | rexreg(i.ops[2].getReg(), i.ops[0].getReg());
					*current++ = 0xf;
					*current++ = (i.ins == SHL ? 0xa5 : 0xad);
					*current++ = 0xc0 | (rr1 << 3) | rr;
				}
				break;
			}

			if (reg(i.ops[0].getReg()) < 8)
			{
				*current++ = 0x48;
			}
			else
			{
				*current++ = 0x49;
			}

			assert(isRRC(i));
			assert(i.ops[2].isUsigc());

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

			uint64 sr = i.ops[2].getUsigc();
			if (sr > 0xff)
			{
				fprintf(log_file, "Too large shift %lld!\n", sr);
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

				uint64 imm = i.ops[1].getUsigc();
				assert(imm < 0x100000000LL);
				wle32(current, (uint32)imm);
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
				BasicBlockRelocation * bbr = new BasicBlockRelocation(image, current_function, flen(), flen() + 4, i.ops[0].getBlock());
				bbr->add32();
				//relocs.push_back(Relocation(REL_S32, currentAddr()+4, len(),
				//                            i.ops[0].getBlock()));
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
			switch (i.ins)
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

			BasicBlockRelocation * bbr = new BasicBlockRelocation(image, current_function, flen(), flen() + 4, i.ops[0].getBlock());
			bbr->add32();
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
		case DIVS:
		case REM:
		case REMS:
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

			if (i.ins == REM || i.ins == REMS)
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
			assert(i.oc == 3);
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
			fprintf(log_file, "Don't know how to turn %lld [%s] into amd64!\n", i.ins, i.toString().c_str());
			assert(false);
		}

		unsigned int siz = (unsigned int)(current - oldcurrent);
		if (siz > i.size)
		{
			printf("Unexpectedly large instruction! estimate %lld actual %d %s\n",
				i.size, siz, i.toString().c_str());
		}
		}

		if (current >= limit)
		{
			printf("Ran out of space to assemble into, %d\n", (int)(limit - base));
			fprintf(log_file, "Ran out of space to assemble into, %d\n", (int)(limit - base));
			return false;
		}
	}

	return true;
}

std::string Amd64::transReg(uint32 r)
{
	std::string ret;

	if (r < 16)
	{
		ret = regnames[r];
	}
	else
	{
		char buf[16];
		sprintf(buf, "???%u", r);
		ret = buf;
	}

	return ret;
}

void Amd64::newFunction(Codegen * c)
{
	Assembler::newFunction(c);
	if (c->callConvention() == CCONV_STANDARD)
	{
		uint64 addr = c->stackSize();
		wle64(current, addr);
	}
}

void Amd64::align(uint64 a)
{
	while (currentAddr() % a)
	{
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
	for (unsigned int loopc = 1; loopc < args.size(); loopc++)
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
				Operand::sigc(32 + (count - 4) * 8)));
		}
		count++;
	}

	r15_backup = addRegStore("r11", b, f);
	rbx_backup = addRegStore("rbx", b, f);
	rbp_backup = addRegStore("rbp", b, f);
	rdi_backup = addRegStore("rdi", b, f);
	rsi_backup = addRegStore("rsi", b, f);
	rsp_backup = addRegStore("rsp", b, f);
	r12_backup = addRegStore("r12", b, f);
	r13_backup = addRegStore("r13", b, f);
	r14_backup = addRegStore("r14", b, f);

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
	Value * fptr,
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

	uint64 stack_size = 0x28;
	if (args.size() > 4)
	{
		stack_size += (args.size() - 4) * 8;
	}

	for (unsigned int loopc = 0; loopc < args.size(); loopc++)
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
				Operand::sigc((32 + (loopc - 4) * 8) - stack_size), Operand(args[loopc])));
			continue;
		}
		current->add(Insn(MOVE, Operand::reg(dest), args[loopc]));
	}

	current->add(Insn(SUB, Operand::reg("rsp"), Operand::reg("rsp"),
		Operand::usigc(stack_size)));
	current->add(Insn(CALL, fptr));

	Value * ret = c->getTemporary(register_type, "ret");
	current->add(Insn(MOVE, ret, Operand::reg("rax")));

	current->add(Insn(ADD, Operand::reg("rsp"), Operand::reg("rsp"),
		Operand::usigc(stack_size)));

	BasicBlock * postcall = c->newBlock("postcall");
	current->add(Insn(BRA, Operand(postcall)));
	c->setBlock(postcall);

	return ret;
}


Value * Amd64UnixCallingConvention::generateCall(Codegen * c,
	Value * fptr,
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

	uint64 stack_size = 0x0;
	if (args.size() > 6)
	{
		stack_size += (args.size() - 6) * 8;
	}

	for (unsigned int loopc = 0; loopc < args.size(); loopc++)
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
				Operand::sigc(((loopc - 6) * 8) - stack_size),
				Operand(args[loopc])));
			continue;
		}

		current->add(Insn(MOVE, Operand::reg(dest), args[loopc]));
	}

	if (stack_size > 0)
	{
		current->add(Insn(SUB, Operand::reg("rsp"), Operand::reg("rsp"),
			Operand::usigc(stack_size)));
	}

	current->add(Insn(CALL, fptr));

	if (stack_size > 0)
	{
		current->add(Insn(ADD, Operand::reg("rsp"), Operand::reg("rsp"),
			Operand::usigc(stack_size)));
	}

	Value * ret = c->getTemporary(register_type, "ret");
	current->add(Insn(MOVE, ret, Operand::reg("rax")));

	BasicBlock * postcall = c->newBlock("postcall");
	current->add(Insn(BRA, Operand(postcall)));
	c->setBlock(postcall);

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
	// b->add(Insn(MOVE, Operand::reg(assembler->framePointer()),
	//        Operand::section(IMAGE_DATA, 0)));
	int count = 0;

	std::vector<Value *> args = f->args();

	for (unsigned int loopc = 1; loopc < args.size(); loopc++)
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
			b->add(Insn(LOAD, Operand(args[loopc]), Operand::reg("rsp"), Operand::usigc((count - 5) * 8)));
		}
		count++;
	}

	rbx_backup = addRegStore("rbx", b, f);
	rbp_backup = addRegStore("rbp", b, f);
	r12_backup = addRegStore("r12", b, f);
	r13_backup = addRegStore("r13", b, f);
	r14_backup = addRegStore("r14", b, f);
	r15_backup = addRegStore("r11", b, f);

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


Value * Amd64UnixSyscallCallingConvention::generateCall(Codegen * c,
	Value *,
	std::vector<Value *> & args)
{
	BasicBlock * current = c->block();
	RegSet res;
	// kernel destroys rcx, r11
	res.set(assembler->regnum("rcx"));
	res.set(assembler->regnum("r11"));
	res.set(assembler->regnum("rax"));
	res.set(assembler->regnum("rdi"));
	res.set(assembler->regnum("rsi"));
	res.set(assembler->regnum("rdx"));
	res.set(assembler->regnum("r8"));
	res.set(assembler->regnum("r9"));

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
			dest = assembler->regnum("rax");
		}
		else if (loopc == 1)
		{
			dest = assembler->regnum("rdi");
		}
		else if (loopc == 2)
		{
			dest = assembler->regnum("rsi");
		}
		else if (loopc == 3)
		{
			dest = assembler->regnum("rdx");
		}
		else if (loopc == 4)
		{
			dest = assembler->regnum("rcx");
		}
		else if (loopc == 5)
		{
			dest = assembler->regnum("r8");
		}
		else if (loopc == 6)
		{
			dest = assembler->regnum("r9");
		}
		current->add(Insn(MOVE, Operand::reg(dest), args[loopc]));
	}

	current->add(Insn(SYSCALL));
	Value * ret = c->getTemporary(register_type, "ret");
	current->add(Insn(MOVE, ret, Operand::reg("rax")));

	BasicBlock * postsyscall = c->newBlock("postsyscall");
	current->add(Insn(BRA, Operand(postsyscall)));
	c->setBlock(postsyscall);

	return ret;
}

#include "thumb.h"
#include "codegen.h"
#include "image.h"
#include "platform.h"
#include "symbols.h"

bool Thumb::validRegOffset(Insn &i, int off)
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
        if (i.ins == LOAD32 || i.ins == LOAD || i.ins == LOADS32)
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

int Thumb::size(BasicBlock *b)
{
    int ret = 0;
    std::list<Insn> &code = b->getCode();

    for (auto &i : code)
    {
        i.addr = address + len();

        switch (i.ins)
        {
        case ENTER_THUMB_MODE: {
            ret += 14;
            i.size = 14;
            break;
        }
        case MOVE: {
            if (i.ops[1].isReg())
            {
                ret += 2;
                i.size = 2;
            }
            else
            {
                ret += 12;
                i.size = 12;
            }
            break;
        }
        case SELGE:
        case SELGES:
        case SELEQ:
        case SELGT:
        case SELGTS: {
            ret += 8;
            i.size = 8;
            break;
        }
            // Worst case
        case BEQ:
        case BNE:
        case BG:
        case BLE:
        case BL:
        case BGE: {
            // Worst case:
            // align, load ip+x, cond branch,
            // uncond branch, nop, 32-bit constant
            ret += 14;
            i.size = 14;
            break;
        }
        case BRA: {
            // As above less cond branch and nop
            ret += 10;
            i.size = 10;
            break;
        }
        default: {
            ret += 2;
            i.size = 2;
        }
        }
    }

    ret += 64; // Size is wrong somewhere

    return ret;
}

bool Thumb::assemble(BasicBlock *b, BasicBlock *next, Image *image)
{
    std::list<Insn> &code = b->getCode();

    b->setAddr(address + flen());

    uint64_t current_addr = (uint64_t)current;
    assert((current_addr & 0x1) == 0);

    unsigned char *block_base = current;

    for (auto &i : code)
    {
        i.addr = address + flen();

        uint16_t mc = 0x46c0; // nop
        bool no_mc = false;

        unsigned char *oldcurrent = current;

        switch (i.ins)
        {
        case ENTER_THUMB_MODE: {
            assert(i.oc == 0);
            // Temporary hack - load r0 with pc then bx to Thumb
            wee32(le, current, 0xe1a0000f); // mov r0, pc - r0 is now this instruction+8
            wee32(le, current, 0xe2800005); // add r0, r0, #5 - make sure to set thumb bit
            wee32(le, current, 0xe12fff10); // bx r0
            wee16(le, current, 0x466f);     // mov r7, sp
            break;
        }

        case SYSCALL: {

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
        case BREAKP: {
            assert(i.oc == 0);
            mc = 0xbe00;
            break;
        }
        case LOAD8:
        case LOADS8:
        case LOAD16:
        case LOADS16:
        case LOADS32:
        case LOAD:
        case LOAD32: {
            assert(i.oc == 2 || i.oc == 3);
            assert(i.ops[0].isReg());
            assert(i.ops[1].isReg());
            assert(i.ops[0].getReg() < 8);

            bool regreg = false;
            uint64_t offset = 0;
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

            if ((i.ins == LOAD32 || i.ins == LOADS32 || i.ins == LOAD) && i.ops[1].getReg() == 13 && offset < 1021 &&
                ((offset & 0x3) == 0))
            {
                mc = 0x9800 | i.ops[0].getReg() << 8 | (uint16_t)offset >> 2;
            }
            else if (i.ins == LOAD8 || i.ins == LOADS8)
            {
                if (regreg)
                {
                    assert(i.ops[1].getReg() < 8);
                    assert(i.ops[2].getReg() < 8);
                    mc = (i.ins == LOAD8 ? 0x5c00 : 0x5600) | i.ops[0].getReg() | i.ops[1].getReg() << 3 |
                         i.ops[2].getReg() << 6;
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
                    mc = (i.ins == LOAD16 ? 0x5a00 : 0x5e00) | i.ops[0].getReg() | i.ops[1].getReg() << 3 |
                         i.ops[2].getReg() << 6;
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
            break;
        }
        case STORE8:
        case STORE16:
        case STORE:
        case STORE32: {
            assert(i.oc == 2 || i.oc == 3);
            assert(i.ops[0].isReg());
            assert(i.ops[i.oc == 3 ? 2 : 1].isReg());
            assert(i.ops[i.oc == 3 ? 2 : 1].getReg() < 8);
            uint64_t offset = 0;
            int sreg = (i.oc == 3) ? 2 : 1;
            bool regreg = false;
            if (i.oc == 3)
            {
                if (i.ops[1].isReg())
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

            if ((i.ins == STORE32 || i.ins == STORE) && i.ops[0].getReg() == 13 && offset < 1021 && (offset & 0x3) == 0)
            {
                mc = 0x9000 | i.ops[(i.oc == 3 ? 2 : 1)].getReg() << 8 | (uint16_t)offset >> 2;
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
                        mc |= offset << 6;
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
                        mc |= offset << 6;
                    }
                }
            }

            break;
        }
        case LOAD64:
        case STORE64: {
            printf("64-bit load/store attempted on 32-bit ARM!\n");
            assert(false);
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
                if (i.ops[1].isUsigc() && i.ops[1].getUsigc() < 256)
                {
                    mc = 0x2000;
                    mc |= i.ops[0].getReg() << 8;
                    mc |= i.ops[1].getUsigc();
                }
                else
                {
                    assert(i.ops[0].getReg() < 8);

                    if ((uint64_t)current & 0x3) // Address is not 32 bit aligned
                    {
                        wee16(le, current, 0x46c0); // 32-bit align with nop
                    }

                    assert(((uint64_t)current & 0x3) == 0);
                    no_mc = true;

                    // ldr r<x>, pc+0 (which is this instruction+4)
                    wee16(le, current, 0x4800 | (i.ops[0].getReg() << 8));
                    // branch over constant
                    wee16(le, current, 0xe001);

                    assert(((uint64_t)current & 0x3) == 0);
                    if (i.ops[1].isFunction())
                    {
                        uint32_t reloc = 0xdeadbeef;
                        FunctionRelocation *fr =
                            new FunctionRelocation(image, current_function, flen(), i.ops[1].getFunction(), 0);
                        fr->add32();
                        wle32(current, reloc);
                    }
                    else if (i.ops[1].isBlock())
                    {
                        uint32_t reloc = 0xdeadbeef;
                        AbsoluteBasicBlockRelocation *abbr =
                            new AbsoluteBasicBlockRelocation(image, current_function, flen(), i.ops[1].getBlock());
                        abbr->add32();
                        wle32(current, reloc);
                    }
                    else if (i.ops[1].isSection())
                    {
                        uint32_t reloc = 0xdeadbeef;
                        int s;
                        uint64_t o = i.ops[1].getSection(s);
                        SectionRelocation *sr = new SectionRelocation(image, IMAGE_CODE, len(), s, o);
                        sr->add32();
                        wle32(current, reloc);
                    }
                    else if (i.ops[1].isExtFunction())
                    {
                        uint32_t reloc = 0xdeadbeef;
                        ExtFunctionRelocation *efr =
                            new ExtFunctionRelocation(image, current_function, len(), i.ops[1].getExtFunction());
                        efr->add32();
                        wle32(current, reloc);
                    }
                    else
                    {
                        uint32_t val;
                        if (i.ops[1].isUsigc())
                        {
                            val = (uint32_t)i.ops[1].getUsigc();
                        }
                        else if (i.ops[1].isSigc())
                        {
                            int32_t sval = (int32_t)i.ops[1].getSigc();
                            val = *((uint32_t *)&sval);
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
                    mc |= (i.ops[1].getReg() & 0x7) << 3;
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
        case SUB: {
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
                if (i.ops[0].getReg() < 8 && i.ops[1].getReg() < 8 && i.ops[2].getReg() < 8)
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
        case SHL:
        case SHR:
        case SAR:
        case ROR:
        case MUL: {
            assert(i.oc == 3);
            assert(i.ops[0].isReg());
            assert(i.ops[1].isReg());
            assert(i.ops[2].isReg());
            assert(i.ops[0].getReg() == i.ops[1].getReg());
            assert(i.ops[0].getReg() < 8);
            assert(i.ops[2].getReg() < 8);

            if (i.ins == AND)
            {
                mc = 0x4000;
            }
            else if (i.ins == OR)
            {
                mc = 0x4300;
            }
            else if (i.ins == SHL)
            {
                mc = 0x4080;
            }
            else if (i.ins == SHR)
            {
                mc = 0x40c0;
            }
            else if (i.ins == SAR)
            {
                mc = 0x4100;
            }
            else if (i.ins == ROR)
            {
                mc = 0x41c0;
            }
            else if (i.ins == MUL)
            {
                mc = 0x4340;
            }
            else // XOR
            {
                mc = 0x4040;
            }

            mc |= i.ops[0].getReg() | (i.ops[2].getReg() << 3);
            break;
        }
        case RCL:
        case RCR:
        case ROL: {
            assert(false);
            break;
        }
        case CMP: {
            assert(i.oc == 2);
            assert(i.ops[0].isReg());
            assert(i.ops[1].isReg() || i.ops[1].isUsigc());
            assert(i.ops[0].getReg() < 8);
            if (i.ops[1].isReg())
            {
                mc = 0x4280 | (i.ops[0].getReg()) | (i.ops[1].getReg() << 3);
            }
            else
            {
                assert(i.ops[1].getUsigc() < 256);
                mc = 0x2800 | (i.ops[0].getReg() << 8) | (uint16_t)i.ops[1].getUsigc();
            }
            break;
        }
        case NOT: {
            assert(i.oc == 2);
            assert(i.ops[0].isReg());
            assert(i.ops[1].isReg());
            assert(i.ops[0].getReg() < 8);
            assert(i.ops[1].getReg() < 8);
            mc = 0x43c0 | i.ops[0].getReg() | (i.ops[1].getReg() << 3);
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

            // Ugh fixed signed/unsigned at some point
            if (i.ins == SELEQ)
            {
                wee16(le, current, 0xd101);
            }
            else if (i.ins == SELGE)
            {
                wee16(le, current, 0xdb01);
            }
            else if (i.ins == SELGT)
            {
                wee16(le, current, 0xdd01);
            }
            else if (i.ins == SELGES)
            {
                wee16(le, current, 0xdb01);
            }
            else if (i.ins == SELGTS)
            {
                wee16(le, current, 0xdd01);
            }
            wee16(le, current, 0x1c00 | i.ops[0].getReg() | i.ops[1].getReg() << 3);
            wee16(le, current, 0xe000);
            mc = 0x1c00 | i.ops[0].getReg() | i.ops[2].getReg() << 3;

            break;
        }
        case BRA: {
            assert(i.oc == 1);
            assert(i.ops[0].isReg() || i.ops[0].isBlock());

            if (i.ops[0].isReg())
            {
                assert(i.ops[0].getReg() < 8);
                mc = 0x4687 | i.ops[0].getReg() << 3; // mov pc, <reg>
            }
            else
            {
                mc = 0xe000;
                // Branch offset is stored >> 1

                int offset = b->getEstimatedBlockOffset(i.ops[0].getBlock(), current - block_base);
                if (offset < -2047 || offset > 2048 || always_do_long_range)
                {
                    printf(">>> Unconditional branch offset overflow %d %x %s %s\n", offset, offset,
                           current_function->name().c_str(), i.ops[0].getBlock()->name().c_str());
                    if ((uint64_t)current & 0x3)
                    {
                        // align
                        wee16(le, current, 0x46c0);
                    }
                    wee16(le, current, 0x4f00); // ldr r7, pc (this+4)
                    wee16(le, current, 0x46bf); // mov pc, r7
                    uint32_t reloc = 0xdeadbeef;
                    AbsoluteBasicBlockRelocation *abbr =
                        new AbsoluteBasicBlockRelocation(image, current_function, flen(), i.ops[0].getBlock());
                    abbr->add32();
                    wle32(current, reloc);
                    no_mc = true;
                }
                else
                {
                    BasicBlockRelocation *bbr =
                        new BasicBlockRelocation(image, current_function, flen(), flen() + 4, i.ops[0].getBlock());
                    bbr->addReloc(0, 1, 0x07ff, 0, 16);
                }
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
            assert(i.ops[0].isBlock() || i.ops[0].isSigc());

            if (i.ins == BNE)
            {
                mc = 0xd100;
            }
            else if (i.ins == BEQ)
            {
                mc = 0xd000;
            }
            else if (i.ins == BG)
            {
                mc = 0xdc00;
            }
            else if (i.ins == BLE)
            {
                mc = 0xdd00;
            }
            else if (i.ins == BL)
            {
                mc = 0xdb00;
            }
            else
            {
                // BGE
                mc = 0xda00;
            }
            // Branch offset is stored >> 1
            // Offset needs checking

            if (i.ops[0].isBlock())
            {
                int offset = b->getEstimatedBlockOffset(i.ops[0].getBlock(), current - block_base);

                if (offset < -2047 || offset > 2048 || always_do_long_range)
                {
                    printf(">>> Conditional branch offset double overflow %d %x %s %s\n", offset, offset,
                           current_function->name().c_str(), i.ops[0].getBlock()->name().c_str());

                    if ((uint64_t)current & 0x3)
                    {
                        // align
                        wee16(le, current, 0x46c0);
                    }
                    wee16(le, current, 0x4f01); // ldr r7, pc+4
                    wee16(le, current, mc);     // conditional branch 4 bytes ahead
                    wee16(le, current, 0xe002); // branch over constant
                    wee16(le, current, 0x46bf); // mov pc, r7

                    uint32_t reloc = 0xdeadbeef;
                    AbsoluteBasicBlockRelocation *abbr =
                        new AbsoluteBasicBlockRelocation(image, current_function, flen(), i.ops[0].getBlock());
                    abbr->add32();
                    wle32(current, reloc);
                    no_mc = true;
                }
                else if (offset < -252 || offset > 258)
                {
                    // Reverse the sense of the conditional branch
                    if (i.ins == BNE)
                    {
                        mc = 0xd000;
                    }
                    else if (i.ins == BEQ)
                    {
                        mc = 0xd100;
                    }
                    else if (i.ins == BG)
                    {
                        mc = 0xdd00;
                    }
                    else if (i.ins == BLE)
                    {
                        mc = 0xdc00;
                    }
                    else if (i.ins == BL)
                    {
                        mc = 0xda00;
                    }
                    else
                    {
                        // BGE
                        mc = 0xdb00;
                    }
                    // no offset == +4, i.e. instruction after next
                    wee16(le, current, mc);

                    // unconditional branch with wider range
                    mc = 0xe000;
                    // Branch offset is stored >> 1

                    BasicBlockRelocation *bbr =
                        new BasicBlockRelocation(image, current_function, flen(), flen() + 4, i.ops[0].getBlock());
                    bbr->addReloc(0, 1, 0x07ff, 0, 16);
                }
                else
                {
                    BasicBlockRelocation *bbr =
                        new BasicBlockRelocation(image, current_function, flen(), flen() + 4, i.ops[0].getBlock());
                    bbr->addReloc(0, 1, 0x00ff, 0, 16);
                }
            }
            else
            {
                int16_t val = (int16_t)i.ops[0].getSigc();
                val -= 2;
                val >>= 1;
                assert(val < 129 && val > -128);
                uint16_t *us = (uint16_t *)&val;
                mc |= (*us) & 0xff;
            }
            break;
        }
        case DIV:
        case DIVS: {
            assert(false); // Not in 16 bit Thumb
            break;
        }
        case REM:
        case REMS: {
            assert(false); // Doesn't exist on ARM
            break;
        }
        case MULS: {
            assert(false); // Need to handle 64 bit result
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

        if (!no_mc)
        {
            wee16(le, current, mc);
        }

        if (current >= limit)
        {
            printf("Ran out of space to assemble into, %d %s\n", (int)(limit - base), i.toString().c_str());
            fprintf(log_file, "Ran out of space to assemble into, %d\n", (int)(limit - base));
            return false;
        }
    }

    return true;
}

std::string Thumb::transReg(uint32_t r)
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
    else if (param == "always_long_branch")
    {
        if (val == "true")
        {
            always_do_long_range = true;
            return true;
        }
    }

    return Assembler::configure(param, val);
}

ValidRegs Thumb::validRegs(Insn &i)
{
    ValidRegs ret;

    // r7 is reserved
    for (int loopc = 0; loopc < 7; loopc++)
    {
        ret.ops[0].set(loopc);
        ret.ops[1].set(loopc);
        ret.ops[2].set(loopc);
    }

    if (i.ins == MOVE)
    {
        for (int loopc = 8; loopc < 16; loopc++)
        {
            ret.ops[0].set(loopc);
            ret.ops[1].set(loopc);
            ret.ops[2].set(loopc);
        }
    }

    if (i.ins == STORE || i.ins == STORE8 || i.ins == STORE16 || i.ins == STORE32 || i.ins == LOAD || i.ins == LOAD8 ||
        i.ins == LOAD16 || i.ins == LOAD32 || i.ins == LOADS32)
    {
        ret.ops[0].set(13);
        ret.ops[1].set(13);
        ret.ops[2].set(13);
    }

    return ret;
}

bool Thumb::validConst(Insn &i, int idx)
{
    if (i.ins == DIV || i.ins == DIVS || i.ins == REM || i.ins == REMS || i.ins == SELEQ || i.ins == SELGT ||
        i.ins == SELGE || i.ins == SELGTS || i.ins == SELGES || i.ins == MUL || i.ins == MULS || i.ins == NOT ||
        i.ins == MUL || i.ins == MULS || i.ins == AND || i.ins == OR || i.ins == XOR || i.ins == SHL || i.ins == SHR ||
        i.ins == SAR)
    {
        return false;
    }

    if (i.ins == ADD || i.ins == SUB)
    {
        if (idx != 2)
        {
            return false;
        }
        else
        {
            return (i.ops[2].isUsigc() && i.ops[2].getUsigc() < 256);
        }
    }

    if (i.ins == LOAD || i.ins == LOAD8 || i.ins == LOAD16 || i.ins == LOAD32 || i.ins == LOAD64)
    {
        if (idx != 2)
        {
            return false;
        }

        if (!(i.ops[2].isSigc() || i.ops[2].isUsigc()))
        {
            return false;
        }

        if (i.ops[2].isSigc() && i.ops[2].getSigc() < 0)
        {
            return false;
        }

        uint64_t val = i.ops[2].isSigc() ? i.ops[2].getSigc() : i.ops[2].getUsigc();

        if (i.ins == LOAD8)
        {
            return (val < 32);
        }
        else if (i.ins == LOAD16)
        {
            return (val < 63);
        }
        else if (i.ins == LOAD || i.ins == LOAD32)
        {
            return (val < 125);
        }
    }

    if (i.ins == STORE || i.ins == STORE8 || i.ins == STORE16 || i.ins == STORE32 || i.ins == STORE64)
    {
        if (idx != 1)
        {
            return false;
        }

        if (!(i.ops[1].isSigc() || i.ops[1].isUsigc()))
        {
            return false;
        }

        if (i.ops[1].isSigc() && i.ops[1].getSigc() < 0)
        {
            return false;
        }

        uint64_t val = i.ops[1].isSigc() ? i.ops[1].getSigc() : i.ops[1].getUsigc();

        if (i.ins == STORE8)
        {
            return (val < 32);
        }
        else if (i.ins == STORE16)
        {
            return (val < 63);
        }
        else if (i.ins == STORE || i.ins == STORE32)
        {
            return (val < 125);
        }
    }

    if (i.ins == CMP)
    {
        if (idx == 0)
        {
            return false;
        }
        else if (idx == 1)
        {
            return (i.ops[1].isUsigc() && i.ops[1].getUsigc() < 256);
        }
    }

    return true;
}

void Thumb::newFunction(Codegen *c)
{
    Assembler::newFunction(c);
    if (c->callConvention() == CCONV_STANDARD)
    {
        uint64_t addr = c->stackSize();
        wle32(current, checked_32(addr));
    }
}

void Thumb::align(uint64_t a)
{
    uint16_t nop = 0xb000;
    while (currentAddr() % a)
    {
        *((uint16_t *)current) = nop;
        current += 2;
    }
}

Value *ThumbLinuxSyscallCallingConvention::generateCall(Codegen *c, Value *fptr, std::vector<Value *> &args)
{
    BasicBlock *call = c->newBlock("syscall");
    c->setBlock(call);
    RegSet res;
    res.set(assembler->regnum("r0"));
    res.set(assembler->regnum("r1"));
    res.set(assembler->regnum("r2"));
    res.set(assembler->regnum("r3"));
    res.set(assembler->regnum("r4"));
    res.set(assembler->regnum("r5"));

    res.set(assembler->regnum("r7"));

    call->setReservedRegs(res);

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
        call->add(Insn(MOVE, Operand::reg(dest), args[loopc]));
    }

    call->add(Insn(SYSCALL));
    Value *ret = c->getTemporary(register_type, "ret");
    call->add(Insn(MOVE, ret, Operand::reg("r0")));

    BasicBlock *postsyscall = c->newBlock("postsyscall");
    call->add(Insn(BRA, Operand(postsyscall)));
    c->setBlock(postsyscall);

    return ret;
}

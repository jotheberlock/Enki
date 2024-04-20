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
    return ret;
}

int Arm64::size(BasicBlock *b)
{
    int ret = 0;
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

        uint32_t mc = 0;

        unsigned char *oldcurrent = current;

        switch (i.ins)
        {
        case SYSCALL: {
            break;
        }
        case BREAKP: {
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
            mc = 0xe1a00000; // NOP
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
    return 0;
}

#include "pass.h"
#include <stdio.h>

OptimisationPass::OptimisationPass()
{
    block = 0;
    next_block = 0;
}

void OptimisationPass::init(Codegen * c)
{
    cg = c;
    block = 0;
}

void SillyRegalloc::init(Codegen * c)
{
    OptimisationPass::init(c);
    for (int loopc=0; loopc<256; loopc++)
    {
        regs[loopc] = 0;
        input[loopc] = 0;
        output[loopc] = 0;
    }
}

void OptimisationPass::prepend(Insn i)
{
    block->getCode().insert(iit, i);
}

void OptimisationPass::append(Insn i)
{
    to_append.push_back(i);
}

void OptimisationPass::change(Insn i)
{
    *iit = i;
}

void OptimisationPass::removeInsn()
{
    iit = block->getCode().erase(iit);
}

void OptimisationPass::run()
{
    std::vector<BasicBlock *> & b = cg->getBlocks();
    for (bit = b.begin();
         bit != b.end(); bit++)
    {
        block = *bit;
        std::vector<BasicBlock *>::iterator nextbit = bit;
        nextbit++;
        if (nextbit == b.end())
        {
            next_block=0;
        }
        else
        {
            next_block = *nextbit;
        }
            
        for (iit = block->getCode().begin();
             iit != block->getCode().end(); iit++)
        {
            for (std::list<Insn>::iterator ait =
                     to_append.begin(); ait != to_append.end(); ait++)
            {
                prepend(*ait);
            }
            to_append.clear();
            
            insn = (*iit);
            processInsn();

	    if (iit == block->getCode().end())
	    {
		break;
	    }
        }
        
        for (std::list<Insn>::iterator ait =
                 to_append.begin(); ait != to_append.end(); ait++)
        {
            block->getCode().push_back(*ait);
        }
        to_append.clear();
    }
}

void ThreeToTwoPass::processInsn()
{
    if (insn.ins == ADD || insn.ins == SUB || insn.ins == MUL ||
        insn.ins == MULS || insn.ins == DIV || insn.ins == DIV ||
        insn.ins == AND || insn.ins == OR || insn.ins == XOR ||
        insn.ins == NOT || insn.ins == REM || insn.ins == REMS ||
        insn.ins == SHL || insn.ins == SHR || insn.ins == SAR)
    {
        if (!insn.ops[0].eq(insn.ops[1]))
        {
            Insn mover(MOVE, insn.ops[0], insn.ops[1]);
            prepend(mover);
            insn.ops[1] = insn.ops[0];
            change(insn);
        }
    }
}

void ConditionalBranchSplitter::processInsn()
{
    if (insn.ins == BNE)
    {
        Insn eq(BRA, insn.ops[1]);
        append(eq);
        
        insn.oc = 1;
        change(insn);
    }
    else if (insn.ins == BEQ)
    {
        Insn neq(BRA, insn.ops[1]);
        append(neq);
        insn.oc = 1;
        change(insn);
    }
}

void BranchRemover::processInsn()
{
    std::list<Insn>::iterator nextinsn = iit;
    nextinsn++;

    if (nextinsn == block->getCode().end())
    {
        if (insn.ins == BRA && insn.ops[0].isBlock() &&
            insn.ops[0].getBlock() == next_block)
        {
            removeInsn();
        }
    }    
}

int SillyRegalloc::findFree(RegSet & r, RegSet & c)
{
    for (int loopc=0; loopc<256; loopc++)
    {
        if (!c.isSet(loopc) && r.isSet(loopc) && regs[loopc] == 0 &&
            !(block->getReservedRegs().isSet(loopc)))
        {
            return loopc;
        }
    }

    // Should probably learn how to spill here
    assert(false);    
    return 0;
}

int SillyRegalloc::alloc(Value * v, RegSet & r, RegSet & c)
{
    for (int loopc=0; loopc<256; loopc++)
    {
        if (c.isSet(loopc))
        {
            fprintf(log_file, "Skipping register %d since it's clobbered\n",
                   loopc);
            
            continue;
        }
        
        if (regs[loopc] == v)
        {
            if (r.isSet(loopc))
            {
                return loopc;
            }
            else
            {
                int nr = findFree(r, c);
                Insn move(MOVE, Operand::reg(nr), Operand::reg(loopc));
                prepend(move);
                regs[nr] = regs[loopc];
                regs[loopc] = 0;
                return loopc;
            }
        }
    }

    int nr = findFree(r, c);
    regs[nr] = v;
    return nr;
}


void SillyRegalloc::processInsn()
{
    for (int loopc=0; loopc<256; loopc++)
    {
        regs[loopc] = 0;
        input[loopc] = 0;
        output[loopc] = 0;
    }
    
    ValidRegs vr = assembler->validRegs(insn);
    
    for (int loopc=0; loopc<insn.oc; loopc++)
    {
        if (insn.ops[loopc].isValue())
        {
            int regnum = alloc(insn.ops[loopc].getValue(), vr.ops[loopc],
                               vr.clobbers);
            if (insn.isIn(loopc))
            {
                input[regnum] = true;
            }
            if (insn.isOut(loopc))
            {
                output[regnum] = true;
            }
            insn.ops[loopc] = Operand::reg(regnum);
        }
    }

    change(insn);

    for (int loopc=0; loopc<256; loopc++)
    {
        int fp = assembler->framePointer();
        if (input[loopc])
        {
            int64 off = regs[loopc]->stackOffset();
            Insn load(loadForType(regs[loopc]->type),
                      Operand::reg(loopc), Operand::reg(fp),
                      Operand::sigc(off));
            load.comment = "Load "+regs[loopc]->name;
            prepend(load);
        }
        if (output[loopc])
        {
            int64 off = regs[loopc]->stackOffset();
            Insn store(storeForType(regs[loopc]->type), Operand::reg(fp),
                      Operand::sigc(off), Operand::reg(loopc));
            store.comment = "Store "+regs[loopc]->name;
            append(store);
        }
    }

    for (int loopc=0; loopc<256; loopc++)
    {
        if (vr.clobbers.isSet(loopc))
        {
            Insn zeroer(MOVE, Operand::reg(loopc), Operand::usigc(0));
            prepend(zeroer);
        }        
    }
}

void AddressOfPass::processInsn()
{
    if (insn.ins == GETADDR)
    {
        uint64 depth = insn.ops[2].getUsigc();
        Insn mover(MOVE, insn.ops[0], Operand::reg(assembler->framePointer()));
        prepend(mover);
        while (depth > 0)
        {
            Insn updoer(LOAD, insn.ops[0], insn.ops[0], Operand::sigc(assembler->staticLinkOffset()));
            prepend(updoer);  // Check ordering...
            depth--;
        }

        Value * v = insn.ops[1].getValue();
        Insn adder(ADD, insn.ops[0], insn.ops[0], Operand::sigc(v->stackOffset()));
        change(adder);
    }
}

void ConstMover::processInsn()
{
    for (int loopc=0; loopc<insn.oc; loopc++)
    {
        if (insn.isIn(loopc) && (insn.ops[loopc].isSigc() ||
                                 insn.ops[loopc].isUsigc()))
        {
            if (!assembler->validConst(insn, loopc))
            {
                Value * v = cg->getTemporary(register_type, "constmover");
                Insn mover(MOVE, v, insn.ops[loopc]);
                prepend(mover);
                insn.ops[loopc] = Operand(v);
                change(insn);
            }
        }
    }
}

void ResolveConstAddr::processInsn()
{
    if (insn.ins == GETCONSTADDR)
    {
        insn.ins = MOVE;
        uint64 offs = insn.ops[1].getUsigc();
        uint64 addr = constants->lookupOffset(offs)+constants->getAddress();
        insn.ops[1] = Operand::usigc(addr);
        change(insn);
    }
}

void StackSizePass::processInsn()
{
    if (insn.ins == GETSTACKSIZE)
    {
        insn.ins = MOVE;
        insn.oc = 2;
        insn.ops[1] = Operand::usigc(cg->stackSize());
        change(insn);
    }
}

void RemWithDivPass::processInsn()
{
    if (insn.ins == REM || insn.ins == REMS)
    {
            // Some architectures e.g. ARM have hardware division
            // but no remainder instruction. Because division truncates,
            // we can calculate remainder as
            // rem = dividend - ((dividend / divisor) * divisor)
        bool is_signed = (insn.ins == REMS);
        insn.ins = is_signed ? DIVS : DIV;
        change(insn);
        Insn mul(is_signed ? MULS : MUL, insn.ops[0], insn.ops[0], insn.ops[2]);
        append(mul);
        Insn sub(SUB, insn.ops[0], insn.ops[1], insn.ops[0]);
        append(sub);
    }
}

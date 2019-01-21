#include "pass.h"
#include <stdio.h>

OptimisationPass::OptimisationPass()
{
	block = 0;
	next_block = 0;
}

void OptimisationPass::init(Codegen * c, Configuration * cf)
{
	cg = c;
    config = cf;
	block = 0;
}

void SillyRegalloc::init(Codegen * c, Configuration * cf)
{
	OptimisationPass::init(c, cf);

    delete[] regs;
    delete[] input;
    delete[] output;
    
    numregs = cf->assembler->numRegs();
    regs = new Value *[numregs];
    input = new bool[numregs];
    output = new bool[numregs];
    
	for (int loopc = 0; loopc < numregs; loopc++)
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

void OptimisationPass::moveInsn(std::list<Insn>::iterator & it)
{
    block->getCode().insert(it, *iit);
    iit = block->getCode().erase(iit);
}

void OptimisationPass::run()
{
    assert(cg);
    
	std::vector<BasicBlock *> & b = cg->getBlocks();
	for (bit = b.begin();
	bit != b.end(); bit++)
	{
		block = *bit;
		std::vector<BasicBlock *>::iterator nextbit = bit;
		nextbit++;
		if (nextbit == b.end())
		{
			next_block = 0;
		}
		else
		{
			next_block = *nextbit;
		}

        beginBlock();
        
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

        endBlock();
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
	for (int loopc = 0; loopc < numregs; loopc++)
	{
		if (!c.isSet(loopc) && r.isSet(loopc) && regs[loopc] == 0 &&
			!(block->getReservedRegs().isSet(loopc)))
		{
            return loopc;
		}
	}

        /*
    // Look for candidate to spill
	for (int loopc = 0; loopc < numregs; loopc++)
	{
        printf("%d %s %s %s %s %s\n", loopc, input[loopc] ? "y" : "n",
               output[loopc] ? "y" : "n", r.isSet(loopc) ? "y" : "n",
               c.isSet(loopc) ? "y" : "n", block->getReservedRegs().isSet(loopc) ? "y" : "n");
        
        if (!input[loopc] && !output[loopc] && r.isSet(loopc) && !c.isSet(loopc) &&
            !(block->getReservedRegs().isSet(loopc)))
        {
            int fp = assembler->framePointer();
			int64 off = regs[loopc]->stackOffset();
			Insn store(storeForType(regs[loopc]->type), Operand::reg(fp),
				Operand::sigc(off), Operand::reg(loopc));
            printf(">> Spilling %d\n", loopc);
			store.comment += "Store " + regs[loopc]->name;
            prepend(store);
            return loopc;
        }
    }
        */
    
	// Should probably learn how to spill here
	assert(false);
	return 0;
}

int SillyRegalloc::alloc(Value * v, RegSet & r, RegSet & c)
{
	for (int loopc = 0; loopc < numregs; loopc++)
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
	for (int loopc = 0; loopc < numregs; loopc++)
	{
		regs[loopc] = 0;
    }
    
    handleInstruction(iit);
    if (insn.ins == CMP)
    {
        assert(iit != block->getCode().end());
        std::list<Insn>::iterator next = iit;
        next++;
        insn = *next;
        handleInstruction(next);
        iit = next;
    }
}


void SillyRegalloc::handleInstruction(std::list<Insn>::iterator & it)
{
    Insn & ins = *it;
	for (int loopc = 0; loopc < numregs; loopc++)
	{
		input[loopc] = 0;
		output[loopc] = 0;
	}
    
	ValidRegs vr = assembler->validRegs(ins);

	for (int loopc = 0; loopc < ins.oc; loopc++)
	{
		if (ins.ops[loopc].isValue())
		{
			int regnum = alloc(ins.ops[loopc].getValue(), vr.ops[loopc],
				vr.clobbers);
			if (ins.isIn(loopc))
			{
				input[regnum] = true;
			}
			if (ins.isOut(loopc))
			{
				output[regnum] = true;
			}
			ins.ops[loopc] = Operand::reg(regnum);
		}
	}

    *it = ins;
    
	for (int loopc = 0; loopc < numregs; loopc++)
	{
		int fp = assembler->framePointer();
		if (input[loopc])
		{
			int64 off = regs[loopc]->stackOffset();
			Insn load(loadForType(regs[loopc]->type),
				Operand::reg(loopc), Operand::reg(fp),
				Operand::sigc(off));
			load.comment += "Load " + regs[loopc]->name;
			prepend(load);
		}
		if (output[loopc])
		{
			int64 off = regs[loopc]->stackOffset();
			Insn store(storeForType(regs[loopc]->type), Operand::reg(fp),
				Operand::sigc(off), Operand::reg(loopc));
			store.comment += "Store " + regs[loopc]->name;
			append(store);
		}
	}

	for (int loopc = 0; loopc < numregs; loopc++)
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
	for (int loopc = 0; loopc < insn.oc; loopc++)
	{
		if (insn.isIn(loopc) && (insn.ops[loopc].isSigc() ||
			insn.ops[loopc].isUsigc()))
		{
			if (!assembler->validConst(insn, loopc))
			{
                if (!const_temporary[loopc])
                {
                    const_temporary[loopc] = cg->getTemporary(register_type, "constmover");
                }
                
				Insn mover(MOVE, const_temporary[loopc], insn.ops[loopc]);
				prepend(mover);
				insn.ops[loopc] = Operand(const_temporary[loopc]);
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
		uint64 addr = constants->lookupOffset(offs);
		insn.ops[1] = Operand::section(IMAGE_CONST_DATA, addr);
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

void BitSizePass::processInsn()
{
	if (insn.ins == GETBITSIZE)
	{
		insn.ins = MOVE;
		insn.oc = 2;
		insn.ops[1] = Operand::usigc(config->assembler->pointerSize() / 8);
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

void ThumbMoveConstantPass::processInsn()
{
    if (insn.ins == MOVE && insn.ops[0].isReg() && insn.ops[0].getReg() > 7)
    {
        Value * tconst = cg->getTemporary(register_type, "thumb_constant");
        int regnum = insn.ops[0].getReg();
        insn.ops[0] = Operand(tconst);
        change(insn);
        Insn mover(MOVE, Operand::reg(regnum), tconst);
        append(mover);
    }
}

void StackRegisterOffsetPass::processInsn()
{
    if ((insn.ins == LOAD || insn.ins == LOAD8 || insn.ins == LOAD16 ||
         insn.ins == LOAD32 || insn.ins == LOADS32 || insn.ins == LOAD64)
        && insn.oc == 3)
    {
        int offset = 0;
        if (insn.ops[2].isSigc())
        {
            offset = insn.ops[2].getSigc();
        }
        else if (insn.ops[2].isUsigc())
        {
            offset = insn.ops[2].getUsigc();
        }
        else
        {
            return;
        }

        if (!assembler->validRegOffset(insn, offset) || insn.ins == LOAD8 || insn.ins == LOAD16)
        {
            Insn get_sp(MOVE, Operand::reg(7), Operand::reg(13));
            prepend(get_sp);
            Insn add_offset(ADD, Operand::reg(7), Operand::reg(7), Operand::usigc(offset));
            prepend(add_offset);
            insn.ops[1] = Operand::reg(7);
            insn.oc = 2;
            change(insn);
        }
    }
    else if ((insn.ins == STORE || insn.ins == STORE8 || insn.ins == STORE16 ||
              insn.ins == STORE32 || insn.ins == STORE64) && insn.oc == 3)
    {
        int offset = 0;
        if (insn.ops[1].isSigc())
        {
            offset = insn.ops[1].getSigc();
        }
        else if (insn.ops[1].isUsigc())
        {
            offset = insn.ops[1].getUsigc();
        }
        else
        {
            return;
        }

        if (!assembler->validRegOffset(insn, offset) || insn.ins == STORE8 || insn.ins == STORE16)
        {
            Insn get_sp(MOVE, Operand::reg(7), Operand::reg(13));
            prepend(get_sp);
            Insn add_offset(ADD, Operand::reg(7), Operand::reg(7), Operand::usigc(offset));
            prepend(add_offset);
            insn.ops[0] = Operand::reg(7);
            insn.ops[1] = insn.ops[2];
            insn.oc = 2;
            change(insn);
        }    
    }
}

void ThumbHighRegisterPass::processInsn()
{
    if ((insn.ins == LOAD || insn.ins == LOAD8 || insn.ins == LOAD16 ||
         insn.ins == LOAD32 || insn.ins == LOAD64 || insn.ins == LOADS32)
        && insn.oc == 2)
    {
        if (insn.ops[1].isReg() && insn.ops[1].getReg() > 7)
        {
            Insn move7(MOVE, Operand::reg(7), insn.ops[1]);
            prepend(move7);
            Insn load7(insn.ins, Operand::reg(7), Operand::reg(7));
            prepend(load7);
            insn.ins = MOVE;
            insn.ops[1] = Operand::reg(7);
            change(insn);
        }
    }
    else if ((insn.ins == STORE || insn.ins == STORE8 || insn.ins == STORE16 ||
              insn.ins == STORE32 || insn.ins == STORE64) && insn.oc == 2)
    {
        if (insn.ops[1].isReg() && insn.ops[1].getReg() > 7)
        {
            Insn move7(MOVE, Operand::reg(7), insn.ops[1]);
            prepend(move7);
            insn.ops[1] = Operand::reg(7);
            change(insn);
        }
    }    
}

void CmpMover::processInsn()
{
    if (insn.ins == CMP)
    {
        std::list<Insn>::iterator it = iit;
        while (it != block->getCode().end())
        {
            Insn ca = *it;
            int o = ca.ins;
            if (o == BG || o == BLE || o == BL || o == BGE ||
                o == SELEQ || o == SELGE || o == SELGT || o == SELGES ||
                o == SELGTS)
            {
                moveInsn(it);
                break;
            }
            it++;
        }
    }
}

void ConditionalBranchExtender::processInsn()
{
    if (insn.ins == BEQ || insn.ins == BNE || insn.ins == BG || insn.ins == BLE ||
        insn.ins == BL || insn.ins == BGE)
    {
        if (insn.ops[0].isBlock())
        {
            int count_to_source = 0;
            int count_to_target = 0;
            bool found_insn = false;
            bool found_block = false;
            
            std::vector<BasicBlock *>::iterator it;
            for (it = cg->getBlocks().begin(); it != cg->getBlocks().end(); it++)
            {
                BasicBlock * bb = *it;
                if (bb == insn.ops[0].getBlock())
                {
                    found_block = true;
                }
                std::list<Insn>::iterator it2;
                for (it2 = bb->getCode().begin(); it2 != bb->getCode().end(); it2++)
                {
                    if (it == bit && it2 == iit)
                    {
                        found_insn = true;
                        if (found_block)
                        {
                            break;
                        }
                    }

                    if (!found_insn)
                    {
                        count_to_source++;
                    }
                    if (!found_block)
                    {
                        count_to_target++;
                    }
                }
                if (found_insn && found_block)
                {
                    break;
                }
            }

            int diff = (count_to_target > count_to_source) ? count_to_target - count_to_source : count_to_source - count_to_target;
            if (diff > 250)  // Just to be on the same side; it's -252 to +258
            {
                // Special form of branch with encoded displacement
                int opposite;
                switch (insn.ins)
                {
                case BEQ:
                    opposite = BNE;
                    break;
                case BNE:
                    opposite = BEQ;
                    break;
                case BG:
                    opposite = BLE;
                    break;
                case BLE:
                    opposite = BG;
                    break;
                case BL:
                    opposite = BGE;
                    break;
                default: // BGE
                    opposite = BL;
                }

                Insn branchover(opposite, Operand::sigc(2));
                prepend(branchover);
                insn.ins = BRA;
                change(insn);
            }
        }
    }
}

void Convert64to32::processInsn()
{
    if (insn.ins == LOAD64)
    {
        insn.ins = LOAD32;
        change(insn);
    }
    else if (insn.ins == STORE64)
    {
        insn.ins = STORE32;
        change(insn);
    }
}

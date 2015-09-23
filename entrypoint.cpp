#include "entrypoint.h"
#include "symbols.h"
#include "asm.h"

void WindowsEntrypoint::generateEpilogue(BasicBlock * block, FunctionScope * scope)
{
    block->add(Insn(MOVE, Operand::reg(1), scope->lookupLocal("__ret")));
    block->add(Insn(MOVE, Operand::reg(7), Operand::extFunction("ExitProcess")));
    block->add(Insn(LOAD, Operand::reg(7), Operand::reg(7)));
    block->add(Insn(CALL, Operand::reg(7)));
}

void LinuxEntrypoint::generateEpilogue(BasicBlock * block, FunctionScope * scope)
{
    block->add(Insn(MOVE, Operand::reg(7), scope->lookupLocal("__ret")));
    block->add(Insn(MOVE, Operand::reg(0), Operand::usigc(0x3c)));
    block->add(Insn(SYSCALL));
}

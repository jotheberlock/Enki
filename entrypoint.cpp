#include "entrypoint.h"
#include "symbols.h"
#include "asm.h"
#include "image.h"

void WindowsEntrypoint::generateEpilogue(BasicBlock * block, FunctionScope * scope, Image * image)
{
    image->addImport("KERNEL32.DLL", "ExitProcess");
    block->add(Insn(MOVE, Operand::reg(1), scope->lookupLocal("__ret")));
    block->add(Insn(MOVE, Operand::reg(7), Operand::extFunction("ExitProcess")));
    block->add(Insn(LOAD, Operand::reg(7), Operand::reg(7)));
    block->add(Insn(CALL, Operand::reg(7)));
}

void LinuxEntrypoint::generateEpilogue(BasicBlock * block, FunctionScope * scope, Image *)
{
    block->add(Insn(MOVE, Operand::reg(7), scope->lookupLocal("__ret")));
    block->add(Insn(MOVE, Operand::reg(0), Operand::usigc(0x3c)));
    block->add(Insn(SYSCALL));
}

void MacOSEntrypoint::generateEpilogue(BasicBlock * block, FunctionScope * scope, Image *)
{
	block->add(Insn(MOVE, Operand::reg(7), scope->lookupLocal("__ret")));
	block->add(Insn(MOVE, Operand::reg(0), Operand::usigc(0x2000001)));
	block->add(Insn(SYSCALL));
}

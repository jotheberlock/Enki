#include <stdlib.h>

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

bool UnixEntrypoint::configure(std::string param, std::string val)
{
	if (param == "syscall_number")
	{
	    syscall_number = strtol(val.c_str(), 0, 0);
	    return true;
	}
	else if (param == "syscall_register")
	{
	    syscall_reg = strtol(val.c_str(), 0, 0);
	    return true;
	}
	else if (param == "exitcode_register")
	{
	    exitcode_reg = strtol(val.c_str(), 0, 0);
	    return true;
	}
	else
	{
	    return Component::configure(param, val);
	}

	return false;
}

void UnixEntrypoint::generateEpilogue(BasicBlock * block, FunctionScope * scope, Image *)
{
    assert(exitcode_reg != -1);
    assert(syscall_reg != -1);
    assert(syscall_number != -1);
    block->add(Insn(MOVE, Operand::reg(exitcode_reg), scope->lookupLocal("__ret")));
    block->add(Insn(MOVE, Operand::reg(syscall_reg), Operand::usigc(syscall_number)));
    block->add(Insn(SYSCALL));
}


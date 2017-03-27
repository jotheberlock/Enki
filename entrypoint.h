#ifndef _ENTRYPOINT_
#define _ENTRYPOINT_

/*
	Equivalent to crtn in gcc-land I suppose? Does the necessary setup and teardown
	before and after entering Enki's outer function, e.g. explicitly calling the exit
	syscall.
*/

#include "component.h"

// Platform-specific stuff equivalent to the stuff before and
// after calling main() in C

class BasicBlock;
class FunctionScope;
class Image;

class Entrypoint : public Component
{
public:

	virtual std::string name() = 0;
	virtual void generatePrologue(BasicBlock *, FunctionScope *, Image *) {}
	virtual void generateEpilogue(BasicBlock *, FunctionScope *, Image *) {}

};

class WindowsEntrypoint : public Entrypoint
{
public:

	virtual std::string name() { return "windowsentrypoint"; }
	virtual void generateEpilogue(BasicBlock *, FunctionScope *, Image *);
};

// Ends with an exit() syscall as appropriate for the platform
class UnixEntrypoint : public Entrypoint
{
public:

	UnixEntrypoint() { syscall_number = -1; exitcode_reg = -1; }
	virtual bool configure(std::string, std::string);
	virtual std::string name() { return "unixentrypoint"; }
	virtual void generateEpilogue(BasicBlock *, FunctionScope *, Image *);

protected:

	int syscall_number;
	int syscall_reg;
	int exitcode_reg;

};

#endif

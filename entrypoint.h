#ifndef _ENTRYPOINT_
#define _ENTRYPOINT_

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

class LinuxEntrypoint : public Entrypoint
{
 public:

  virtual std::string name() { return "linuxentrypoint"; }
  virtual void generateEpilogue(BasicBlock *, FunctionScope *, Image *);
  
};

class MacOSEntrypoint : public Entrypoint
{
public:

	virtual std::string name() { return "macosentrypoint"; }
	virtual void generateEpilogue(BasicBlock *, FunctionScope *, Image *);

};
  
#endif

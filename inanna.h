#ifndef _INANNA_
#define _INANNA_

/*
Generates Inanna (Enki native) binaries
*/

#include "image.h"
#include "stringbox.h"
#include "codegen.h"

// Inanna files are supposed to be platform independent, so no external
// calls.

class IllegalCall : public CallingConvention
{
  public:

    virtual void generatePrologue(BasicBlock *, FunctionScope *)
    {
        printf("Illegal attempt to generate prologue!\n");
    }
    
    virtual void generateEpilogue(BasicBlock *, FunctionScope *)
    {
        printf("Illegal attempt to generate epilogue!\n");
    }
    
    virtual Value * generateCall(Codegen *, Value *, std::vector<Value *> &)
    {
        printf("Illegal attempt to generate call!\n");
        return 0;
    }
};

class InannaImage : public Image
{
public:

    InannaImage();
    ~InannaImage();
    void finalise();
    bool configure(std::string, std::string);

    virtual uint64 importAddress(std::string)
    {
        return 0;
    }

    virtual uint64 importOffset(std::string)
    {
        return 0;
    }

    std::string name() { return "inanna"; }

    virtual void materialiseSection(int);

protected:

    int arch;

};

#endif

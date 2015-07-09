#ifndef _TYPE_
#define _TYPE_

#include <stdio.h>
#include <string>
#include <list>
#include <vector>
#include <stdint.h>

#include "asm.h"
#include "platform.h"

class Codegen;
class Value;
class Expr;

class Type
{
  public:

    virtual std::string name() = 0;
    virtual std::string display(unsigned char *) = 0;
    virtual ~Type()
    {
    }
    
    virtual bool canCopy(Type *)
    {
        return false;
    }

    virtual bool canFuncall()
    {
        return false;
    }

    virtual Value * generateFuncall(Codegen *, Funcall *,
                                    std::vector<Value *> & args)
    {
        return 0;
    }
    
    // Address of target, source
    virtual void copy(Codegen *,Value *, Value *)
    {}

    // Current address, expression
    virtual void calcAddress(Codegen *, Value *, Expr *)
    {
    }

    virtual bool inRegister()
    {
        return false;
    }

    virtual bool canDeref() 
    {
        return false;
    }
    
    virtual bool canField()
    {
        return false;
    }

    virtual Type * derefType()
    {
        return 0;
    }

    virtual Type * fieldType(std::string)
    {
        return 0;
    }

    virtual bool canIndex()
    {
        return false;
    }

    virtual Type * indexType()
    {
        return 0;
    }

    virtual bool isMacro()
    {
        return false;
    }
    
    virtual int size() = 0;
    virtual int align() = 0;
    
};

class VoidType : public Type
{
 public:

  VoidType() {}

  std::string name()
  {
      return "Void";
  }

  std::string display(unsigned char *)
  {
      return "void";
  }
  
  int size()
  {
      return 0;
  }

  int align()
  {
      return 0;
  }
};

class BoolType : public Type
{
  public:
    
    BoolType() {}

    std::string name()
    {
        return "Bool";
    }

    std::string display(unsigned char *);
    
    virtual bool inRegister()
    {
        return true;
    }

    int size()
    {
        return 8;
    }

    int align()
    {
        return 8;
    }
    
};


class IntegerType : public Type
{
  public:

    IntegerType(bool u, int s)
    {
        un_signed = u;
        bits = s;
    }

    std::string display(unsigned char *);
    
    // Address of target, source
    virtual bool canCopy(Type *)
    { 
        return true;
    }
    virtual void copy(Codegen *, Value *, Value *);

    virtual bool inRegister()
    {
        return true;
    }

    std::string name()
    {
        std::string ret;
        if (un_signed)
            ret += "u";
        char buf[4096];
        sprintf(buf, "int%d", bits);
        ret += buf;
        return ret;
    }

    int size()
    {
        return bits;
    }

    int align()
    {
        return (bits/8)*8;
    }

  protected:

    bool un_signed;
    int bits;
    
};


class PointerType : public Type
{
  public:
    
    PointerType(Type * t)
    {
        pointed_type = t;
    }

    std::string display(unsigned char *);
    
    // Address of target, source
    virtual void copy(Codegen *, Value *, Value *);
    virtual bool canCopy(Type *)
    {
        return true;
    }

    virtual void calcAddress(Codegen *, Value *, Expr *);

    virtual bool inRegister()
    {
        return true;
    }

    std::string name()
    {
        std::string ret = pointed_type->name();
        ret += "^";
        return ret;
    }
    
    bool canDeref()
    {
        return true;
    }

    int size()
    {
        return assembler->pointerSize();
    }

    int align()
    {
        return assembler->pointerSize();
    }

    Type * derefType()
    {
        return pointed_type;
    }
    
  protected:

    Type * pointed_type;
    
};

class GeneratorType : public Type
{
  public:

    GeneratorType(Type * t)
    {
        pointed_type=t;
    }
    
    std::string name()
    {
        std::string ret = pointed_type->name();
        ret += "$";
        return ret;
    }
    
    virtual bool inRegister()
    {
        return false;
    }

    
    // Address of target, source
    virtual bool canCopy(Type *)
    { 
        return true;
    }
    virtual void copy(Codegen *, Value *, Value *);

    int size()
    {
        return assembler->pointerSize();
    }

    int align()
    {
        return assembler->pointerSize();
    }

    std::string display(unsigned char *)
    {
        return "<generator>";
    }
    
  protected:

    Type * pointed_type;
};


class ArrayType : public Type
{
  public:
    
    ArrayType(Type * t, uint64_t s = 0)
    {
        pointed_type = t;
        siz = (int)s;
    }

    std::string display(unsigned char *);
    
    std::string name()
    {
        std::string ret = pointed_type->name();
        char buf[4096];
        sprintf(buf, "[%d]", siz);
        ret += buf;
        return ret;
    }

    virtual void calcAddress(Codegen *, Value *, Expr *);

    virtual bool canIndex()
    {
        return true;
    }

    virtual Type * indexType()
    {
        return pointed_type;
    }
    
    int size()
    {
        return siz*(pointed_type->size());
    }

    int align()
    {
        return 64;
    }
    
  protected:

    Type * pointed_type;
    int siz;
    
};

class StructElement
{
  public:

    std::string name;
    Type * type;
    uint64_t offset;

};

class StructType : public Type
{
  public:

    StructType(std::string n, bool u)
    {
        nam=n;
        siz=0;
        is_union=u;
    }

    std::string display(unsigned char *);
    
    void addMember(std::string n, Type * t)
    {
        fprintf(log_file, "Adding member %s\n", n.c_str());
        StructElement se;
        se.name = n;
        se.type = t;
        members.push_back(se);
    }

    virtual void copy(Codegen *, Value *, Value *);
    virtual bool canCopy(Type *);
    
    void calc();

    bool canField()
    {
        return true;
    }

    Type * fieldType(std::string n);

    virtual void calcAddress(Codegen *, Value *, Expr *);
    
    int size()
    {
        return siz;
    }

    int align()
    {
        return (size()/8)*8;
    }
    
    std::string name()
    {
        return nam;
    }
    
  protected:

    std::vector<StructElement> members;
    std::string nam;
    int siz;
    bool is_union;
    
};


class FunctionType : public Type
{
  public:

    FunctionType(std::string n, bool m)
    {
        nam=n;
        siz=0;
        is_macro = m;
    }
    
    virtual bool isMacro()
    {
        return is_macro;
    }
    
    std::string display(unsigned char *)
    {
        return is_macro ? "<macro>" : "<func>";
    }
    
    void addParam(std::string n, Type * t)
    {
        fprintf(log_file, "Adding param %s\n", n.c_str());
        StructElement se;
        se.name = n;
        se.type = t;
        params.push_back(se);
    }
    
    void addReturn(Type * t)
    {
        returns.push_back(t);
    }

    std::vector<Type *> & getReturns()
    {
        return returns;
    }
    
    void calc();

    bool canFuncall()
    {
        return true;
    }

    virtual Value * generateFuncall(Codegen *, Funcall *,
                            std::vector<Value *> & args);
    
    int size()
    {
        return 64;
    }

    int align()
    {
        return 64;
    }

    std::string name()
    {
        return nam;
    }

    Value * getFunctionPointer(Codegen *, Funcall *);
    Value * initStackFrame(Codegen *, Value *, Value *&, Funcall *);
    
  protected:

    std::vector<StructElement> params;
    std::vector<Type *> returns;

    std::string nam;
    int siz;
    bool is_macro;
    
};

class CallingConvention;

class ExternalFunctionType : public FunctionType
{
  public:

    ExternalFunctionType(std::string n, CallingConvention * c)
        : FunctionType(n, false)
    {
        convention=c;
    }
    
    virtual Value * generateFuncall(Codegen * c, Funcall * f,
				    std::vector<Value *> & args);
    
    int size()
    {
        return 0;
    }
    
  protected:

    CallingConvention * convention;
    
};


Type * lookupType(std::string);
void addType(Type *, std::string);
void initialiseTypes();
void destroyTypes();

extern Type * register_type;
extern Type * byte_type;

#endif

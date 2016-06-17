#ifndef _TYPE_
#define _TYPE_

#include <stdio.h>
#include <string>
#include <list>
#include <vector>

#include "asm.h"
#include "platform.h"

class Codegen;
class Value;
class Expr;

// Should probably distinguish ranges for basic v
// struct/union types at some point
extern uint64 class_id_counter;

class Type
{
  public:

    virtual std::string name() = 0;
    virtual std::string display(unsigned char *) = 0;

    Type()
    {
        is_const=false;
        class_id = class_id_counter++;
    }
    
    virtual ~Type()
    {
    }

    uint64 classId()
    {
        return class_id;
    }
    
    virtual bool canCopy(Type *)
    {
        return false;
    }

    virtual bool canFuncall()
    {
        return false;
    }

    virtual bool validArgList(std::vector<Value *> &, std::string &)
    {
        return false;
    }
    
    virtual Value * generateFuncall(Codegen *, Funcall *, Value *,
                                    std::vector<Value *> & args)
    {
        printf("Illegal generateFuncall!\n");
        return 0;
    }

    virtual bool canActivate()
    {
        return false;
    }

    virtual Type * activatedType()
    {
        return 0;
    }

    virtual Value * getActivatedValue(Codegen *, Value *)
    {
        printf("Illegal attempt to get activated value!\n");
        return 0;
    }
    
    virtual void activate(Codegen *, Value * v)
    {
        printf("Illegal activation!\n");
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

    virtual bool isSigned()
    {
        return false;
    }

    bool isConst()
    {
        return is_const;
    }

    void setConst(bool b)
    {
        is_const=b;
    }
    
    virtual int size() = 0;
    virtual int align() = 0;

	// make an instance of this type optionally with = v
	virtual bool construct(Codegen *, Value * t, Value * v) { return v ? false : true;  }

    virtual Type * getParent()
    {
        return 0;
    }
    
  protected:

    uint64 class_id;
    bool is_const;
    
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
        is_signed = u;
        bits = s;
    }

    bool isSigned()
    {
        return !is_signed;
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
        if (!is_signed)
            ret += "U";
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

	virtual bool construct(Codegen *, Value * t, Value * v);

  protected:

    bool is_signed;
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

	virtual bool construct(Codegen *, Value * t, Value * v);

  protected:

    Type * pointed_type;

};

class ActivationType : public Type
{
  public:

    ActivationType(Type * t)
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
        return true;
    }

    
    // Address of target, source
    virtual bool canCopy(Type *)
    { 
        return true;
    }
    virtual void copy(Codegen *, Value *, Value *);

    bool canActivate()
    {
        return true;
    }

    virtual Type * activatedType()
    {
        return pointed_type;
    }
    
    void activate(Codegen *, Value *);

    virtual Value * getActivatedValue(Codegen *, Value *);
    
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

	virtual bool construct(Codegen *, Value * t, Value * v);
    
  protected:

    Type * pointed_type;
};


class ArrayType : public Type
{
  public:
    
    ArrayType(Type * t, uint64 s = 0)
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
    uint64 offset;

};

class StructType : public Type
{
  public:

    StructType(std::string n, bool u);

    std::string display(unsigned char *);
    
    void addMember(std::string n, Type * t)
    {
        fprintf(log_file, "Adding member %s\n", n.c_str());
        StructElement se;
        se.name = n;
        se.type = t;
        members.push_back(se);
    }

    void setParent(Type * st)
    {
        parent = st;
    }

    Type * getParent()
    {
        return parent;
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

	virtual bool construct(Codegen *, Value * t, Value * v);
    
  protected:

    Type * parent;
    std::vector<StructElement> members;
    std::string nam;
    int siz;
    bool is_union;
    
};


class FunctionType : public Type
{
  public:

    FunctionType(bool m)
    {
        siz=0;
        is_macro = m;
    }
    
    // Address of target, source
    virtual void copy(Codegen *, Value *, Value *);
    virtual bool canCopy(Type *)
    {
        return true;
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
    
    virtual bool validArgList(std::vector<Value *> & args, std::string &);

    std::vector<StructElement> & getParams()
    {
	return params;
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

    // Value will be an ActivationType, activated once
    virtual Value * generateFuncall(Codegen *, Funcall *, Value * fp,
                            std::vector<Value *> & args);
    
    int size()
    {
        return assembler->pointerSize();
    }

    int align()
    {
        return assembler->pointerSize();
    }

    Value * allocStackFrame(Codegen *, Value *, Value *&, Funcall *, Type *);

    std::string name()
    {
        std::string ret;
        for (unsigned int loopc=0; loopc<returns.size(); loopc++)
	{
	    if (loopc>0)
 	    {
  	        ret += ",";
	    }
	    ret += returns[loopc]->name();
	}
	ret += "(";
	for (unsigned int loopc=0; loopc<params.size(); loopc++)
	{
	    if (loopc>0)
	    {
	        ret += ",";
	    }
		if (params[loopc].type)
		{
			ret += params[loopc].type->name()+" ";
		}
		ret += params[loopc].name;
        }
	ret += ")";
	return ret;
    }
    
  protected:

    std::vector<StructElement> params;
    std::vector<Type *> returns;

    int siz;
    bool is_macro;
    
};

class CallingConvention;

class ExternalFunctionType : public FunctionType
{
  public:

  ExternalFunctionType(CallingConvention * c, bool ia = false)
      : FunctionType(false)
    {
        convention=c;
        ignore_arguments = ia;   // If true, can take arbitrary arguments
    }

    // Value will be the literal return
    virtual Value * generateFuncall(Codegen * c, Funcall * f, Value * fp,
				    std::vector<Value *> & args);
    
    int size()
    {
        return assembler->pointerSize();
    }

    std::string name()
    {
        std::string ret = "extern ";
        ret += FunctionType::name();
        return ret;
    }

    virtual bool validArgList(std::vector<Value *> & args, std::string & reason)
    {
        return ignore_arguments ? true : FunctionType::validArgList(args, reason);
    }
    
  protected:

    CallingConvention * convention;
    bool ignore_arguments;
    
};

class GenericFunctionType : public FunctionType
{
public:

	GenericFunctionType()
		: FunctionType(false)
	{
	}

	virtual Value * generateFuncall(Codegen * c, Funcall * f, Value * fp,
		std::vector<Value *> & args);

	int size()
	{
		return assembler->pointerSize();
	}

	std::string name()
	{
		std::string ret = "generic ";
		ret += FunctionType::name();
		return ret;
	}

	// Should at least validate sizes/basic types match...
	virtual bool validArgList(std::vector<Value *> & args, std::string & reason)
	{
		return args.size() == params.size();
	}

};

class Types
{
  public:
    
    Types();
    ~Types();
    Type * lookup(std::string);
    void add(Type *, std::string);
    std::map<std::string, Type *> & get();

  protected:
    
    std::map<std::string, Type *> types;
    
};

extern Types * types;

void initialiseTypes();
void destroyTypes();

extern Type * register_type;
extern Type * signed_register_type;
extern Type * byte_type;

#endif

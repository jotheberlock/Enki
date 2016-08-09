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
class FunctionType;

class FunctionScope;

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
		is_const = false;
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

	virtual int argCount()
	{
		return 0;
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
	virtual void copy(Codegen *, Value *, Value *)
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

        // Given Foo^^^ return Foo and depth of dereferences
        // Assumes count is 0-initialised
    Type * baseDeref(int & count)
    {
        count += 1;
        if (canDeref())
        {
            return derefType()->baseDeref(count);
        }
        else
        {
            return this;
        }
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
		is_const = b;
	}

	virtual bool isGeneric()
	{
		return false;
	}

	virtual int size() = 0;
	virtual int align() = 0;

	// make an instance of this type optionally with = v
	virtual bool construct(Codegen *, Value * t, Value * v) { return v ? false : true; }

	virtual Type * getParent()
	{
		return 0;
	}
    
	// Used for picking which overloaded function to use
	// Lower the better (more specialised) match, or -1 means doesn't match
	virtual int score(Type * t)
	{
		return (t==this) ? 0 : -1;
	}

	virtual void registerSpecialiser(FunctionScope *) {}

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
		return (bits / 8) * 8;
	}

	int score(Type * t)
	{
		// Probably needs handling better
		if (!t->canDeref() && t->inRegister())
		{
			if (t->size() == size())
			{
				return 0;
			}
			else if (t->size() < size())
			{
				return 1;
			}
		}
		return -1;
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
		pointed_type = t;
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

	StructType(std::string n, bool u, StructType * p);

	std::string display(unsigned char *);

	void addMember(std::string n, Type * t)
	{
		fprintf(log_file, "Adding member %s\n", n.c_str());
		StructElement se;
		se.name = n;
		se.type = t;
		members.push_back(se);
	}

	StructType * getParent()
	{
		return parent;
	}

    void registerChild(StructType * t)
    {
        children.push_back(t);
    }

    std::vector<StructType *> & getChildren()
    {
        return children;
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
		return (size() / 8) * 8;
	}

	std::string name()
	{
		return nam;
	}

	int score(Type * t)
	{
		if (t == this)
		{
			return 0;
		}

		int score = 1;
		Type * p = getParent();
		while (p)
		{
			if (p == t)
			{
				return score;
			}
			score++;
			p = p->getParent();
		}

		return -1;
	}

	virtual bool construct(Codegen *, Value * t, Value * v);

protected:

	StructType * parent;
	std::vector<StructElement> members;
    std::vector<StructType *> children;
	std::string nam;
	int siz;
	bool is_union;

};

typedef std::vector<uint64> FunctionSignature;

class FunctionType : public Type
{
public:

	FunctionType(bool m)
	{
		siz = 0;
		is_macro = m;
        ret_type = 0;
	}

	// Address of target, source
	virtual void copy(Codegen *, Value *, Value *);
	virtual bool canCopy(Type *)
	{
		return true;
	}

    FunctionSignature getSignature()
    {
        std::vector<uint64> ret;
        for (unsigned int loopc=0; loopc<params.size(); loopc++)
        {
            ret.push_back(params[loopc].type->classId());
        }
        return ret;
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
        ret_type = t;
	}

	virtual bool validArgList(std::vector<Value *> & args, std::string &);

	std::vector<StructElement> & getParams()
	{
		return params;
	}

	Type * getReturn()
	{
		return ret_type;
	}

	int argCount()
	{
		return (int)params.size();
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
        if (ret_type)
		{
			ret += ret_type->name();
		}
		ret += "(";
		for (unsigned int loopc = 0; loopc < params.size(); loopc++)
		{
			if (loopc > 0)
			{
				ret += ",";
			}
			if (params[loopc].type)
			{
				ret += params[loopc].type->name() + " ";
			}
			ret += params[loopc].name;
		}
		ret += ")";
		return ret;
	}

protected:

	std::vector<StructElement> params;
	Type * ret_type;

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
		convention = c;
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

class MtablesEntry
{
  public:
    std::vector<uint64> table;
    FunctionScope * ptr;
};

class Mtable
{
  public:
    
    FunctionScope * fun;
    std::vector<MtablesEntry> entries;
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

    void generateTable();
    
	std::string name()
	{
		std::string ret = "generic ";
		ret += FunctionType::name();
		return ret;
	}

	// Should at least validate sizes/basic types match...
	virtual bool validArgList(std::vector<Value *> & args, std::string & reason)
	{
        if (args.size() != params.size())
        {
            reason = "Differing number of arguments";
        }
        
		return args.size() == params.size();
	}

	bool isGeneric()
	{
		return true;
	}

	void registerSpecialiser(FunctionScope * ft)
	{
		specialisations.push_back(ft);
	}

    std::vector<FunctionScope *> & getSpecialisations();
    
protected:
    
	std::vector<FunctionScope *> specialisations;
    
};

class Types
{
public:

	Types();
	~Types();
	Type * lookup(std::string);
    Type * lookup(uint64); // By class ID
	void add(Type *, std::string);
	std::map<std::string, Type *> & get();

protected:

	std::map<std::string, Type *> types;

};

class Image;

class MtableEntry
{
  public:

    MtableEntry()
    {
        target=0;
    }
    
    std::vector<uint64> table;
    FunctionScope * target;
    
};

class Mtables
{
  public:

    Mtables()
    {
        sf_bit = false;
        offset = 0;
    } 
    
    void add(FunctionScope * e)
    {
        entries.push_back(e);
    }
    uint64 lookup(FunctionScope* fs)
    {
        return offsets[fs] * (sf_bit ? 8 : 4);
    }

        // This fills num_data with ints
    void generateTables();
        // This turns them into bytes in the image
    void createSection(Image *, Assembler *);
    
  protected:

    void processFunction(FunctionScope *);
    
    std::list<FunctionScope *> entries;
    std::map<FunctionScope *, uint64> offsets;
    std::vector<MtableEntry> data;
    uint64 offset;  // in words
    bool sf_bit;
    
};

extern Types * types;
extern Mtables * mtables;

void initialiseTypes();
void destroyTypes();

extern Type * register_type;
extern Type * signed_register_type;
extern Type * byte_type;

#endif

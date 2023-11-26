#ifndef _SYMBOLS_
#define _SYMBOLS_

/*
	The symbol table. At top level in root_scope is a FunctionScope,
	which may contain other FunctionScopes (child functions)
	and SymbolScopes (e.g. blocks).
*/

#include <string>
#include <unordered_map>
#include <list>
#include <vector>
#include <assert.h>

#include "platform.h"

class Value;
class FunctionScope;
class FunctionType;

class SymbolScope
{
public:

	SymbolScope(SymbolScope * p, std::string n)
	{
		parent_scope = p;
		if (p)
		{
			p->addChild(this);
		}
		symbol_name = n;
        values_gotten = false;
	}

	virtual ~SymbolScope()
	{
		for (std::list<SymbolScope *>::iterator it = children.begin();
		it != children.end(); it++)
		{
			delete (*it);
		}
	}

	void addChild(SymbolScope * s)
	{
		children.push_back(s);
	}

	void add(Value *);
	Value * lookup(std::string, int &, bool = true);

	Value * lookupLocal(std::string n)
	{
		int static_depth = 0;
		Value * ret = lookup(n, static_depth, false);
		return ret;
	}

	FunctionScope * lookup_function(std::string);
	virtual FunctionScope * currentFunction();
	virtual FunctionScope * parentFunction();

	virtual bool isFunction()
	{
		return false;
	}

	SymbolScope * parent()
	{
		return parent_scope;
	}

	void addFunction(FunctionScope *);

	std::list<SymbolScope *> & getChildren()
	{
		return children;
	}

        // Caller becomes responsible for deleting values
	void getValues(std::vector<Value *> & values);

	std::string name() { return symbol_name; }
	std::string fqName();
    std::string longName();

protected:

	SymbolScope * parent_scope;
	std::unordered_map<std::string, Value *> contents;
	std::vector<Value *> sorted_contents;
	std::unordered_map<std::string, FunctionScope *> functions;
	std::list<SymbolScope *> children;
	std::string symbol_name;

    bool values_gotten;

};

class FunctionScope : public SymbolScope
{
public:

	FunctionScope(SymbolScope * p, std::string n, FunctionType * ft);
    ~FunctionScope();

	bool isGeneric();

	virtual FunctionScope * currentFunction();
	virtual FunctionScope * parentFunction();

	void addArg(Value * s)
	{
		add(s);
		args_list.push_back(s);
	}

	std::vector<Value *> & args()
	{
		return args_list;
	}

	FunctionType * getType()
	{
		return type;
	}

	virtual bool isFunction()
	{
		return true;
	}

	uint64_t getAddr()
	{
		assert(addr);
		return addr;
	}

	void setAddr(uint64_t a)
	{
		addr = a;
	}

	void setStackSize(uint64_t s)
	{
		stack_size = s;
	}

	uint64_t getStackSize()
	{
		assert(stack_size);
		return stack_size;
	}

protected:

	std::vector<Value *> args_list;
	FunctionType * type;
	uint64_t addr;
	uint64_t stack_size;

    Value * oldframe;
    Value * ip;
    Value * staticlink;
    Value * retvar;

};

extern FunctionScope * root_scope;

#endif

#ifndef _AST_
#define _AST_

#include <stdio.h>
#include <string>
#include "lexer.h"
#include "type.h"

class Codegen;
class Value;
class BasicBlock;
class SymbolScope;

class Expr
{
public:

	virtual void print(int i) = 0;

	virtual void printOp(uint64 val)
	{
		uint32 first = val & 0xffffffff;
		uint64 big_second = val >> 32;
		uint32 second = (uint32)big_second;
		fprintf(log_file, "%c", (char)first);
		if (second)
		{
			fprintf(log_file, "%c", (char)second);
		}
	}

	virtual Value * codegen(Codegen *)
	{
		fprintf(log_file, "Null codegen called!\n");
		return 0;
	}

	virtual ~Expr()
	{
	}

protected:

	void indent(int i)
	{
		for (int loopc = 0; loopc < i; loopc++)
		{
			fprintf(log_file, "  ");
		}
	}

};

class IntegerExpr : public Expr
{
public:

	IntegerExpr(Token *);

	virtual void print(int i)
	{
		indent(i);
		fprintf(log_file, "Int %lld", val);
	}

	uint64 getVal()
	{
		return val;
	}

	virtual Value * codegen(Codegen *);

protected:

	uint64 val;

};

// The idea with these is they are defined in the config
// files and can have a different value for each config;
// meanwhile the AST stays the same between all possible configs.
// Codegen is per config, so e.g. if (WINDOWS) <foo> can get optimised
// out at that point if WINDOWS is set to 0 for a Linux config.
class ConfigConstantExpr : public Expr
{
  public:

    ConfigConstantExpr(std::string n)
    {
        constant_name = n;
    }

    virtual void print(int i)
    {
        indent(i);
        fprintf(log_file, "ConfigConstant %s", constant_name.c_str());
    }
    
	virtual Value * codegen(Codegen *);
    
  protected:

    std::string constant_name;

};

class BreakpointExpr : public Expr
{
public:

	virtual void print(int i)
	{
		indent(i);
		fprintf(log_file, "Breakpoint");
	}

	virtual Value * codegen(Codegen * c);

};

class IdentifierExpr : public Expr
{
public:

	IdentifierExpr(Token *);

	virtual void print(int i)
	{
		indent(i);
		fprintf(log_file, "Identifier %s", val.c_str());
	}

	std::string getString()
	{
		return val;
	}

	virtual Value * codegen(Codegen *);

	void setValue(Value * v, int s)
	{
		value = v;
		static_depth = s;
	}

protected:

	std::string val;
	Value * value;
	int static_depth;

};

class StringLiteralExpr : public Expr
{
public:

	StringLiteralExpr(Token *);

	virtual void print(int i)
	{
		indent(i);
		fprintf(log_file, "'%s'", val.c_str());
	}

	std::string getString()
	{
		return val;
	}

	uint64 getIndex()
	{
		return idx;
	}

	virtual Value * codegen(Codegen *);

protected:

	std::string val;
	uint64 idx;

};

class VarDefExpr : public Expr
{
public:

	VarDefExpr(Value * v, Expr * a, bool c)
	{
		value = v;
		assigned = a;
		is_const = c;
	}

	~VarDefExpr()
	{
		delete assigned;
	}

	virtual void print(int i);

	virtual Value * codegen(Codegen * c);

protected:

	Type * type;
	Value * value;
	Expr * assigned;
	bool is_const;

};

class AddressOfExpr : public Expr
{
public:

	AddressOfExpr(Expr * e)
	{
		expr = e;
	}

	~AddressOfExpr()
	{
		delete expr;
	}

	virtual void print(int i)
	{
		indent(i);
		fprintf(log_file, "&");
		expr->print(0);
	}

protected:

	Expr * expr;

};

class UnaryExpr : public Expr
{
public:

	UnaryExpr(Token * t, Expr * e)
	{
		if (t->value.size() == 2)
		{
			uint64 second = t->value[1];
			op = (second << 32) | t->value[0];
		}
		else if (t->value.size() == 1)
		{
			op = t->value[0];
		}
		else
		{
			fprintf(log_file, "Trying to make a unop out of %d!\n", t->type);
		}

		expr = e;
		token = *t;
	}

	~UnaryExpr()
	{
		delete expr;
	}

	virtual void print(int i)
	{
		indent(i);
		fprintf(log_file, "unop ");
		printOp(op);
		expr->print(i + 1);
	}

	virtual Value * codegen(Codegen *);

protected:

	uint64 op;
	Expr * expr;
	Token token;

};

class Block : public Expr
{
public:

	Block()
	{
	}

	~Block()
	{
		std::list<Expr *>::iterator it = contents.begin();
		while (it != contents.end())
		{
			delete *it;
			it++;
		}
	}

	void add(Expr * e)
	{
		contents.push_back(e);
	}

	void absorb(Block * b)
	{
		std::list<Expr *>::iterator it = b->contents.begin();
		while (it != b->contents.end())
		{
			add(*it);
		}
		b->contents.clear();
	}

	virtual void print(int i)
	{
		indent(i);
		fprintf(log_file, "Block\n");
		std::list<Expr *>::iterator it = contents.begin();
		while (it != contents.end())
		{
			(*it)->print(i + 1);
			fprintf(log_file, "\n");
			it++;
		}
	}

	virtual Value * codegen(Codegen * c);

protected:

	std::list<Expr *> contents;

};

class IfClause
{
public:

	IfClause(Expr * c, Block * b)
	{
		condition = c;
		body = b;
	}

	~IfClause()
	{
		delete condition;
		delete body;
	}

	Expr * condition;
	Block * body;
};

class If : public Expr
{
public:

	If()
	{
		elseblock = 0;
	}

	~If()
	{
		for (std::list<IfClause *>::iterator it = clauses.begin();
		it != clauses.end(); it++)
		{
			delete *it;
		}
		delete elseblock;
	}

	void addElse(Block * e)
	{
		elseblock = e;
	}

	void addClause(Expr * c, Block * b)
	{
		clauses.push_back(new IfClause(c, b));
	}

	virtual void print(int i)
	{
		for (std::list<IfClause *>::iterator it = clauses.begin();
		it != clauses.end(); it++)
		{
			indent(i);
			fprintf(log_file, (it == clauses.begin()) ? "if " : "elif ");
			(*it)->condition->print(0);
			fprintf(log_file, "\n");
			(*it)->body->print(i + 1);
		}

		if (elseblock)
		{
			indent(i);
			fprintf(log_file, "else\n");
			elseblock->print(i + 1);
		}
	}

	virtual Value * codegen(Codegen * c);

protected:

	std::list<IfClause *> clauses;
	Block * elseblock;

};

class While : public Expr
{

public:

	While(Expr * c, Block * b)
	{
		condition = c;
		body = b;
	}

	~While()
	{
		delete condition;
		delete body;
	}

	virtual void print(int i)
	{
		indent(i);
		fprintf(log_file, "while ");
		condition->print(0);
		body->print(i + 1);
	}

	virtual Value * codegen(Codegen * c);

protected:

	Expr * condition;
	Block * body;

};

class VarExpr : public Expr
{
public:

	VarExpr(Token * t, Token * ty, Expr * v = 0)
	{
		name = t->toString();
		type = ty->toString();
		value = v;
	}

	~VarExpr()
	{
		delete value;
	}

	virtual void print(int i)
	{
		indent(i);
		fprintf(log_file, "var %s %s", name.c_str(), type.c_str());
		if (value)
		{
			fprintf(log_file, " = ");
			value->print(0);
		}
	}

protected:

	std::string name;
	std::string type;

	Expr * value;

};

class BinaryExpr : public Expr
{
public:

	BinaryExpr(Token * t, Expr * l, Expr * r)
	{

		if (t->value.size() == 2)
		{
			uint64 second = t->value[1];
			op = (second << 32) | t->value[0];
		}
		else if (t->value.size() == 1)
		{
			op = t->value[0];
		}
		else
		{
			// E.g. 'or', 'and'
			op = 0;
		}

		token = *t;
		lhs = l;
		rhs = r;
	}

	~BinaryExpr()
	{
		delete lhs;
		delete rhs;
	}

	virtual void print(int i)
	{
		indent(i);
		fprintf(log_file, "binop ");
		if (op == 0)
		{
			fputs(token.toString().c_str(), log_file);
		}
		else
		{
			printOp(op);
		}
		fprintf(log_file, " ");
		lhs->print(0);
		fprintf(log_file, " ");
		rhs->print(0);
	}

	virtual Value * codegen(Codegen *);

protected:

	uint64 op;
	Expr * lhs;
	Expr * rhs;
	Token token;

};

class Square : public Expr
{
public:

	Square(Expr * ss)
	{
		subscript = ss;
	}

	~Square()
	{
		delete subscript;
	}

	virtual void print(int i)
	{
		indent(i);
		fprintf(log_file, "[");
		if (subscript)
			subscript->print(0);
		fprintf(log_file, "]");
	}

	virtual Value * codegen(Codegen * c)
	{
		return subscript->codegen(c);
	}

protected:

	Expr * subscript;

};

class Pass : public Expr
{
public:

	Pass() {}
	virtual void print(int i)
	{
		indent(i);
		fprintf(log_file, "pass");
	}
};

class Break : public Expr
{
public:

	Break(Token t) { token = t; }
	virtual void print(int i)
	{
		indent(i);
		fprintf(log_file, "break");
	}

	virtual Value * codegen(Codegen * c);
	Token token;

};

class Continue : public Expr
{
public:

	Continue(Token t) { token = t; }

	virtual void print(int i)
	{
		indent(i);
		fprintf(log_file, "continue");
	}

	virtual Value * codegen(Codegen * c);
	Token token;

};

class Return : public Expr
{
public:

	Return(Expr * r) { ret = r; }

	~Return()
	{
		delete ret;
	}

	virtual void print(int i)
	{
		indent(i);
		fprintf(log_file, "return ");
		if (ret)
			ret->print(0);
	}

	virtual Value * codegen(Codegen * c);

protected:

	Expr * ret;

};


class Yield : public Expr
{
public:

	Yield(Expr * r) { ret = r; }
	~Yield()
	{
		delete ret;
	}

	virtual void print(int i)
	{
		indent(i);
		fprintf(log_file, "yield ");
		if (ret)
			ret->print(0);
	}

	virtual Value * codegen(Codegen * c);

protected:

	Expr * ret;

};

class VarRefElement
{
public:

	VarRefElement(int i, Expr * s = 0)
	{
		type = i;
		subs = s;
	}

	~VarRefElement()
	{
	}

	int type;
	Expr * subs;
};

#define VARREF_DEREF 1
#define VARREF_ARRAY 2
#define VARREF_MEMBER 3

//foo^^[1].bar
class VarRefExpr : public Expr
{
public:

	VarRefExpr()
	{
		value = 0;
		scope = 0;
		depth = 0;
	}

	~VarRefExpr()
	{
        for (std::vector<VarRefElement>::iterator it = elements.begin();
             it != elements.end(); it++)
        {
            delete (*it).subs;
        }
    }

	virtual void print(int i);

	Type * checkType(Codegen *);

	void add(VarRefElement vre)
	{
		elements.push_back(vre);
	}

	virtual Value * codegen(Codegen * c);
	void store(Codegen * c, Value * v);

	Value * value;
	std::vector<VarRefElement> elements;
	SymbolScope * scope;
	Token token;
	int depth;

};

class MemberExpr : public Expr
{

public:

	MemberExpr(IdentifierExpr * m, Expr * e)
	{
		expr = e;
		member = m;
	}

	~MemberExpr()
	{
		delete expr;
		delete member;
	}

	virtual void print(int i)
	{
		fprintf(log_file, ".%s", member->getString().c_str());
		expr->print(0);
	}

protected:

	Expr * expr;
	IdentifierExpr * member;

};

class StructExpr : public Expr
{
public:

	StructExpr(Type * t)
	{
		type = t;
	}

	virtual void print(int i)
	{
		indent(i);
		fprintf(log_file, "struct %s", type->name().c_str());
	}

protected:

	Type * type;

};

class DefExpr : public Expr
{
public:

	DefExpr(FunctionType * t, FunctionScope * fs, bool is_m, bool is_e, bool is_g, Value * p, std::string n, std::string l = "")
	{
		type = t;
		scope = fs;
		is_macro = is_m;
		is_extern = is_e;
		is_generic = is_g;
		name = n;
		libname = l;
		body = 0;
		ptr = p;
	}

	~DefExpr()
	{
		delete body;
	}

	FunctionScope * getFunction()
	{
		return scope;
	}

	void setBody(Expr * e)
	{
		body = e;
	}

	virtual void print(int i)
	{
		indent(i);
		fprintf(log_file, "%s %s\n", is_extern ? "extern" : (is_macro ? "macro" : "def"),
			name.c_str());
		if (!is_extern)
		{
			if (body)
			{
				body->print(i + 1);
			}
			else
			{
				fprintf(log_file, "Null body!\n");
			}
		}
	}

	virtual Value * codegen(Codegen * c);

protected:

	FunctionType * type;
	Expr * body;
	FunctionScope * scope;
	bool is_macro;
	bool is_extern;
	bool is_generic;
	std::string name;
	std::string libname;
	Value * ptr;

};

class Funcall : public Expr
{
public:

	Funcall(IdentifierExpr * ie)
	{
		ident = ie;
		scope = 0;
	}

	~Funcall()
	{
		delete ident;
		for (unsigned int loopc = 0; loopc < args.size(); loopc++)
		{
			delete args[loopc];
		}
	}

	void setScope(SymbolScope * s)
	{
		scope = s;
	}

	SymbolScope * getScope()
	{
		return scope;
	}

	void addArg(Expr * e)
	{
		args.push_back(e);
	}

	virtual void print(int i)
	{
		indent(i);
		fprintf(log_file, "funcall ");
		ident->print(0);

		if (args.size() == 0)
		{
			fprintf(log_file, "(");
		}
		else
		{
			for (unsigned int loopc = 0; loopc < args.size(); loopc++)
			{
				fprintf(log_file, (loopc == 0) ? "(" : ",");
				args[loopc]->print(0);
			}
		}
		fprintf(log_file, ")");
	}

	std::string name()
	{
		return ident->getString();
	}

	std::vector<Expr *> & getArgs()
	{
		return args;
	}

	virtual Value * codegen(Codegen * c);

	Token token;

protected:

	SymbolScope * scope;
	std::vector<Expr *> args;
	IdentifierExpr * ident;

};

class SymbolScope;

class Parser
{
public:

	Parser(Lexer *);
	~Parser()
	{
		delete root;
	}

	Expr * tree()
	{
		return root;
	}

protected:

	Expr * parseVarExpr(Type *);
	Expr * parseBodyLine();
	Expr * parseInteger();
	Expr * parseStringLiteral();
	Expr * parseIdentifier();
	Expr * parseParen();
	Expr * parseExpr();
	Expr * parsePrimary();
	Expr * parseBinopRHS(int prec, Expr * lhs);
	Expr * parseUnary();
	Expr * parseBlock();
	Expr * parseIf();
	Expr * parseWhile();
	Expr * parseSquare();
	Expr * parseReturn();
	Expr * parseYield();
	Expr * parseStruct();
	Expr * parseDef();
	Expr * parseFuncall(IdentifierExpr *);
	Expr * parseVarRef(Expr *);
	Expr * parseAddressOf();
	Expr * parseFptr();

	Type * parseType();

	int getPrecedence();

    std::string getIdentifier(std::string errstring);
    
	Token next()
	{
		if (count < tokens.size())
		{
			current = tokens[count];
			count++;
			fprintf(log_file, "Returning %d %c\n", current.type, (char)(current.value.size() > 0 ? current.value[0] : ' '));
			return current;
		}

		current = Token();
		current.type = DONE;
		fprintf(log_file, "Out of tokens\n");
		return current;
	}

	void push()
	{
		count -= 2;
		next();
	}

	void expectedEol();

	Lexer * lexer;
	std::vector<Token> tokens;
	unsigned int count;
	Token current;
	Expr * root;
	SymbolScope * current_scope;
	int generic_counter;

};

extern std::list<Codegen *> macros;

#endif

#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "error.h"
#include "codegen.h"
#include "cfuncs.h"
#include "symbols.h"
#include "configfile.h"
#include "image.h"
#include "exports.h"
#include "lexer.h"
#include "imports.h"

#define LOGICAL_TRUE 1

IntegerExpr::IntegerExpr(Token * t)
{
	if (t->value.size() > 19)
	{
		printf("Invalidly huge integer! [%s]\n", t->toString().c_str());
		val = 0;
		return;
	}

	val = 0;
	unsigned int begin = 0;
	unsigned int base = 10;

	bool is_negative = false;
	if (t->value.size() > 1 && t->value[0] == '-')
	{
		is_negative = true;
		begin++;
	}

	if (t->value.size() > begin + 2 && t->value[begin] == '0' && t->value[begin + 1] == 'x')
	{
		begin += 2;
		base = 16;
	}

	// Do our own strtol(3) to make sure we can do 64 bits
	for (unsigned int loopc = begin; loopc < t->value.size(); loopc++)
	{
		unsigned char v = t->value[loopc];
		unsigned char n = 0;
		if (v >= '0' && v <= '9')
		{
			n = (v - '0');
		}
		else if ((base == 16) && v >= 'a' && v <= 'f')
		{
			n = (v - 'a') + 10;
		}
		else if ((base == 16) && v >= 'A' && v <= 'F')
		{
			n = (v - 'A') + 10;
		}
		else
		{
			printf("Invalid digit [%c]!\n", v);
		}

		val *= base;
		val = val + n;
	}

	if (is_negative)
	{
		int64 * sval = (int64 *)(&val);
		*sval = -*sval;
	}
}

IdentifierExpr::IdentifierExpr(Token * t)
{
    val = t->toString();
	static_depth = 0;
	value = 0;
}

ImportExpr::ImportExpr(std::string t)
{
    val = t;
}

StringLiteralExpr::StringLiteralExpr(Token * t)
{
	char buf[4096];
	unsigned int loopc;
	for (loopc = 0; loopc < t->value.size(); loopc++)
	{
		buf[loopc] = (char)t->value[loopc];
	}
	buf[loopc] = 0;

	val = buf;
	idx = constants->addConstant(buf, loopc + 1, 1);
}

Parser::Parser(Lexer * l, bool pi, std::string mo)
{
	current_scope = root_scope;
	lexer = l;
	tokens = lexer->tokens();
	count = 0;
	generic_counter = 0;
    parsing_imports = pi;
    module = mo;
	next();
	if (current.type != BEGIN)
	{
		addError(Error(&current, "Expected beginning block"));
        root = 0;
	}
	else
	{
		root = parseBlock();
	}
}

std::string Parser::getIdentifier(std::string errstring)
{
    std::string ret = "";
    IdentifierExpr * ie = (IdentifierExpr *)parseIdentifier();
    if (!ie)
    {
        addError(Error(&current, errstring));
    }
    else
    {
        ret = ie->getString();
        delete ie;
    }
    
    return ret;
}

Expr * Parser::parseInteger()
{
	Expr * ret = new IntegerExpr(&current);
	next();
	return ret;
}

Expr * Parser::parseStringLiteral()
{
	Expr * ret = new StringLiteralExpr(&current);
	next();
	return ret;
}

Expr * Parser::parseIdentifier()
{
	IdentifierExpr * ret = new IdentifierExpr(&current);
	next();

	int depth = 0;
	Value * v = current_scope->lookup(ret->getString(), depth);
	ret->setValue(v, depth);
	return ret;
}

Expr * Parser::parseParen()
{
	next();
	Expr * e = parseExpr();
	if (!e)
	{
		addError(Error(&current, "Couldn't find expr in parens"));
		return 0;
	}

	if (current.type != CLOSE_BRACKET)
	{
		addError(Error(&current, "Unterminated parenthesis expression"));
	}
	next();
	return e;
}

Expr * Parser::parseExpr()
{
	Expr * lhs = parseUnary();
	if (!lhs)
	{
		addError(Error(&current, "No left hand side in expression"));
		return 0;
	}

	return parseBinopRHS(0, lhs);
}

Expr * Parser::parseCast()
{
    next();
    if (current.type != OPEN_BRACKET)
    {
        addError(Error(&current, "Expected ( after cast"));
        return 0;
    }
    next();
    Expr * to_cast = parseExpr();
    if (!to_cast)
    {
        return 0;
    }
    if (current.type != COMMA)
    {
        addError(Error(&current, "Expected , in cast"));
        return 0;
    }
    next();
    Type * cast_type = parseType();
    if (!cast_type)
    {
        addError(Error(&current, "Expected type in cast"));
        return 0;
    }
    if (current.type != CLOSE_BRACKET)
    {
        addError(Error(&current, "Expected ) at end of cast"));
        return 0;
    }
    next();
    return new CastExpr(to_cast, cast_type);
}

int Parser::getPrecedence()
{
	bool dummy;
	OpRec rec;

	if (current.value.size() == 0)
	{
		return -1;
	}

	if (!lexer->isOp(current.value[0], current.value.size() == 1 ? 0 : current.value[1],
                     dummy, rec, current.toString()))
	{
		return -1;
	}

	if (rec.type != BINOP)
	{
		return -1;
	}

	return rec.pri;
}

Expr * Parser::parseBinopRHS(int prec, Expr * lhs)
{
	while (true)
	{
		int tprec = getPrecedence();
		if (tprec < prec)
			return lhs;
		Token binop = current;
		next();
		Expr * rhs = parseUnary();
		if (!rhs)
		{
			return 0;
		}
		int nprec = getPrecedence();
		if (tprec < nprec)
		{
			rhs = parseBinopRHS(tprec + 1, rhs);
			if (rhs == 0)
			{
				return 0;
			}
		}

		lhs = new BinaryExpr(&binop, lhs, rhs);

		if (current.type == DONE)
		{
			return lhs;
		}
	}
}

Expr * Parser::parsePrimary()
{
	if (current.type == INTEGER_LITERAL)
	{
		return parseInteger();
	}
	else if (current.type == STRING_LITERAL)
	{
		return parseStringLiteral();
	}
	else if (current.type == IDENTIFIER)
	{
        std::string token_value = current.toString();
        uint64 dummy;

        if (configuration->lookupConfigConstant(token_value, dummy))
        {
            next();
            ConfigConstantExpr * ret = new ConfigConstantExpr(token_value);
            return ret;
        }
        
		Expr * ident = parseIdentifier();
 		if (current.type == OPEN_BRACKET)
		{
			return parseFuncall((IdentifierExpr *)ident);
		}
		else
		{
			return parseVarRef(ident);
		}
	}
	else if (current.type == OPEN_BRACKET)
	{
		return parseParen();
	}
	else if (current.type == OPEN_SQUARE)
	{
		return parseSquare();
	}
	else if (current.type == ADDRESSOF)
	{
		return parseAddressOf();
	}
    else if (current.type == CAST)
    {
        return parseCast();
    }
	else
	{
		char buf[4096];
		sprintf(buf, "%d", current.type);
		addError(Error(&current,
			"Unknown token type parsing primary expression",
			buf));
		return 0;
	}
}

Expr * Parser::parseUnary()
{
	bool dummy;
	OpRec rec;

	uint32 first = 0;
	uint32 second = 0;
	if (current.value.size() > 0)
	{
		first = current.value[0];
	}
	if (current.value.size() > 1)
	{
		second = current.value[1];
	}

	if (first == 0)
	{
		return parsePrimary();
	}

	if (!lexer->isOp(first, second, dummy, rec, current.toString()))
	{
		return parsePrimary();
	}

	if (rec.type != UNARYOP)
	{
		return parsePrimary();
	}

	Token op = current;
	next();
	if (Expr * expr = parseUnary())
	{
		return new UnaryExpr(&op, expr);
	}

	return 0;
}

Expr * Parser::parseBodyLine()
{
	if (current.type == DONE)
	{
		fprintf(log_file, "Null because done\n");
		return 0;
	}

	if (current.type == EOL)
	{
		fprintf(log_file, "Null because eol\n");
		return 0;
	}

	if (current.type == IDENTIFIER)
	{
		bool is_funcall = false;

		next();
		if (current.type == OPEN_BRACKET)
		{
			is_funcall = true;
		}
		push();

		if (!is_funcall)
		{
			fprintf(log_file, "Looking up type\n");
			Type * t = parseType();
			if (t)
			{
				fprintf(log_file, "Trying var\n");
				Expr * ret = parseVarExpr(t);
				if (current.type != EOL)
				{
					addError(Error(&current, "Expected EOL after vardef"));
					expectedEol();
				}
				else
				{
					next();
				}

				return ret;
			}
		}
	}
	else if (current.type == IF || current.type == CONSTIF)
	{
		return parseIf();
	}
	else if (current.type == WHILE)
	{
		return parseWhile();
	}
	else if (current.type == RETURN)
	{
		return parseReturn();
	}
	else if (current.type == STRUCT || current.type == UNION)
	{
		return parseStruct();
	}
	else if (current.type == FPTR)
	{
		return parseFptr();
	}
	else if (current.type == YIELD)
	{
		return parseYield();
	}
	else if (current.type == PASS)
	{
		next();
		if (current.type != EOL)
		{
			addError(Error(&current, "Expected EOL after pass"));
			expectedEol();
		}
		else
		{
			next();
		}
		return new Pass();
	}
	else if (current.type == BREAKPOINT)
	{
		next();
		if (current.type != EOL)
		{
			addError(Error(&current, "Expected EOL after breakpoint"));
			expectedEol();
		}
		else
		{
			next();
		}
		return new BreakpointExpr();
	}
	else if (current.type == BREAK)
	{
		next();
		if (current.type != EOL)
		{
			addError(Error(&current, "Expected EOL after break"));
			expectedEol();
		}
		else
		{
			next();
		}

		return new Break(current);
	}
	else if (current.type == CONTINUE)
	{
		next();
		if (current.type != EOL)
		{
			addError(Error(&current, "Expected EOL after continue"));
			expectedEol();
		}
		else
		{
			next();
		}

		return new Continue(current);
	}
	else if (current.type == DEF || current.type == MACRO || current.type == EXTERN || current.type == GENERIC)
	{
		return parseDef();
	}
    else if (current.type == IMPORT)
    {
        next();
        if (current.type != IDENTIFIER)
        {
            addError(Error(&current, "Expected identifier for import"));
            return 0;
        }

        Token t = current;

        next();
        if (current.type != EOL)
        {
            addError(Error(&current, "Expected EOL after import"));
            expectedEol();
        }
        else
        {
            next();
        }

        return parseImport(t);
    }
    else if (current.type == MODULE)
    {
        return parseModule();
    }

	Expr * ret = parseExpr();
	if (current.type != EOL)
	{
		addError(Error(&current, "Expected EOL at end of line"));
		expectedEol();
	}
	else
	{
		next();
	}

	return ret;
}

extern void readFile(FILE * f, Chars & input);

Expr * Parser::parseImport(Token t)
{
    std::string name = t.toString();
    std::string fname = name + ".i";
    FILE * ifile = fopen(fname.c_str(), "rb");
    if (!ifile)
    {
        fname = "../" + fname;
        ifile = fopen(fname.c_str(), "rb");
    }

    if (!ifile)
    {
        addError(Error(&t, "Can't find module definition for " + name));
        return 0;
    }

    Lexer lex;
    Chars input;
    readFile(ifile, input);
    lex.setFile(fname);
    lex.lex(input);
    lex.endLexing();

    if (errors.size() != 0)
    {
        printf("Lexer errors reading interface %s:\n\n", name.c_str());
        for (std::list<Error>::iterator it = errors.begin();
        it != errors.end(); it++)
        {
            (*it).print();
            printf("\n");
        }
    }

    Parser subparser(&lex, true, name);

    if (errors.size() != 0)
    {
        printf("Parser errors reading interface %s:\n\n", name.c_str());
        for (std::list<Error>::iterator it = errors.begin();
        it != errors.end(); it++)
        {
            (*it).print();
            printf("\n");
        }
    }

    return new ImportExpr(name);
}

Expr * Parser::parseSquare()
{
	next();
	Expr * c = parseExpr();
	if (current.type != CLOSE_SQUARE)
	{
		addError(Error(&current, "Unterminated ["));
		return 0;
	}
	else
	{
		next();
		return new Square(c);
	}
}

Expr * Parser::parseIf()
{
    bool constif = (current.type == CONSTIF);

	next();
	Expr * c = parseExpr();

	Block * b = 0;
	Block * e = 0;

	If * ifexpr = new If(constif);

	if (current.type != EOL)
	{
		addError(Error(&current, "Expected EOL after if expr"));
		expectedEol();
		return 0;
	}
	next();

	if (current.type != BEGIN)
	{
		addError(Error(&current, "Expected block after if"));
		return 0;
	}
	else
	{
		b = (Block *)parseBlock();
		ifexpr->addClause(c, b);
	}

	while (current.type == ELIF)
	{
		next();
		Expr * elifc = parseExpr();

		if (current.type != EOL)
		{
			addError(Error(&current, "Expected EOL after elif expr"));
			expectedEol();
			return 0;
		}
		next();

		if (current.type != BEGIN)
		{
			addError(Error(&current, "Expected block after elif"));
			return 0;
		}
		else
		{
			Block * elifb = (Block *)parseBlock();
			ifexpr->addClause(elifc, elifb);
		}
	}

	if (current.type == ELSE)
	{
		next();
		if (current.type != EOL)
		{
			addError(Error(&current, "Expected EOL after else"));
			expectedEol();
			return 0;
		}
		next();
		if (current.type != BEGIN)
		{
			addError(Error(&current, "Expected block after else"));
			return 0;
		}
		else
		{
			e = (Block *)parseBlock();
			ifexpr->addElse(e);
		}
	}

	return ifexpr;
}

Expr * Parser::parseWhile()
{
	next();
	Expr * c = parseExpr();
    if (!c)
    {
        addError(Error(&current, "Expected expression after while"));
        return 0;
    }
    
	Block * b = 0;

	if (current.type != EOL)
	{
		addError(Error(&current, "Expected EOL after while expr"));
		expectedEol();
	}
	next();

	if (current.type != BEGIN)
	{
		addError(Error(&current, "Expected block after while"));
	}
	else
	{
		b = (Block *)parseBlock();
	}

	if (b)
	{
		return new While(c, b);
	}
	else
	{
        addError(Error(&current, "Failed to parse while block"));
		return 0;
	}
}

Expr * Parser::parseReturn()
{
	next();
	if (current.type == EOL)
	{
		next();
		return new Return(0);
	}
	else
	{
		Expr * e = parseExpr();
		if (!e)
		{
			addError(Error(&current, "Expected expr following return"));
			return 0;
		}

		if (current.type != EOL)
		{
			addError(Error(&current, "Expected EOL after return expression"));
			expectedEol();
			return 0;
		}
		else
		{
			next();
		}

		return new Return(e);
	}
}

Expr * Parser::parseYield()
{
	next();
	if (current.type == EOL)
	{
		next();
		return new Yield(0);
	}
	else
	{
		Expr * e = parseExpr();
		if (!e)
		{
			addError(Error(&current, "Expected expr following yield"));
			return 0;
		}

		if (current.type != EOL)
		{
			addError(Error(&current, "Expected EOL after yield expression"));
			expectedEol();
			return 0;
		}
		else
		{
			next();
		}

		return new Yield(e);
	}
}

Expr * Parser::parseVarExpr(Type * t)
{
	bool is_c = false;
	if (current.type == CONST)
	{
		next();
		is_c = true;
	}
    
    std::string name = getIdentifier("Type not followed by variable name");
	if (name != "")
	{
		Expr * assigned = 0;
		if (current.toString() == "=")
		{
			fprintf(log_file, "Caught assign\n");
			next();
			assigned = parseExpr();
		}

		Value * v = new Value(name,t);
		current_scope->add(v);
		return new VarDefExpr(v, assigned, is_c);
	}
	else
	{
		return 0;
	}
}

Expr * Parser::parseFuncall(IdentifierExpr * fname)
{
	Funcall * ret = new Funcall(fname);
	ret->setScope(current_scope);
	ret->token = current;
	next();

	while (true)
	{
		if (current.type == CLOSE_BRACKET)
		{
			next();
			break;
		}

		Expr * e = parseExpr();
		if (!e)
		{
			addError(Error(&current, "Not an expression"));
		}
		else
		{
			ret->addArg(e);
		}

		if (current.type == COMMA)
		{
			next();
		}
		else if (current.type == CLOSE_BRACKET)
		{
			next();
			break;
		}
		else
		{
			addError(Error(&current, "Non-comma between args"));
			break;
		}
	}

	return ret;
}

Expr * Parser::parseBlock()
{
	next();

	SymbolScope * nb = new SymbolScope(current_scope, "<block>");
	current_scope = nb;

	Block * ret = new Block();

	while (true)
	{
		if (current.type == DONE || current.type == END)
		{
			if (current.type == END)
			{
				next();
			}
			current_scope = current_scope->parent();
			return ret;
		}
		else
		{
			Expr * i = parseBodyLine();
			if (!i && (current.type == DONE))
			{
				current_scope = current_scope->parent();
				return ret;
			}
			else if (!i && (current.type == EOL))
			{
				next();
			}
			else if (!i)
			{
				//addError(Error(&current, "Can't parse expression in block"));
				// break;
				continue;
			}
			else
			{
				ret->add(i);
			}
		}
	}

	current_scope = current_scope->parent();
	return ret;
}

Expr * Parser::parseStruct()
{
	int type = current.type;

	next();

    bool cooked = true;
    if (current.type == RAW)
    {
        cooked = false;
        next();
    }
    
	if (current.type != IDENTIFIER)
	{
		addError(Error(&current, "Expected identifier for struct"));
		return 0;
    }
    
    std::string sname = getIdentifier("Expected type name");
	Type * t = types->lookup(sname);
	if (t)
	{
		addError(Error(&current, "Type already defined", sname));
		return 0;
	}

	StructType * st = 0;

	if (current.type == OPEN_BRACKET)
	{
		next();
		if (current.type != IDENTIFIER)
		{
			addError(Error(&current, "Expected identifier for parent"));
			return 0;
		}
        
		std::string pname = getIdentifier("Expected parent type name");
		Type * pt = types->lookup(pname);
		if (!pt)
		{
			addError(Error(&current, "Could not find parent type", pname));
			return 0;
		}
		if (!pt->canField())
		{
			addError(Error(&current, "Parent type must be a struct or union"));
			return 0;
		}

        if (pt && !cooked && pt->hasRtti())
        {
            addError(Error(&current, "Cannot declare raw struct/union as a subtype of managed struct/union"));
            return 0;
        }
        
		st = new StructType(sname, type == UNION, (StructType *)pt, cooked);
		next();
		next();
	}
	else
	{
		st = new StructType(sname, type == UNION, 0, cooked);
            //st->addMember("rtti", register_type);
		next();
	}

	types->add(st, sname); // Add here so it can reference itself

	if (current.type != BEGIN)
	{
		addError(Error(&current, "Expected block for struct"));
		return 0;
	}
	next();

	while (current.type != END)
	{
        if (current.type == PASS)
        {
            next();
            if (current.type != EOL)
            {
                addError(Error(&current, "Expected eol after pass"));
                return 0;
            }
            next();
            break;
        }
        
		Type * t = parseType();
		if (!t)
		{
			addError(Error(&current, "Is not a type"));
			return 0;
		}

		if (current.type != IDENTIFIER)
		{
			addError(Error(&current, "Expected field name"));
			return 0;
		}

        std::string mname = getIdentifier("Expected member type name");
		st->addMember(mname, t);

		if (current.type != EOL)
		{
			addError(Error(&current, "Expected EOL in struct"));
			expectedEol();
			return 0;
		}
		next();
	}

	st->calc();
	next();
	return new StructExpr(st);
}

Expr * Parser::parseVarRef(Expr * e)
{
	IdentifierExpr * ie = dynamic_cast<IdentifierExpr *>(e);
	if (!ie)
	{
		addError(Error(&current, "Unexpected expression type"));
		return 0;
	}

	VarRefExpr * vre = new VarRefExpr();
	vre->token = current;
	vre->scope = current_scope;

	std::string name = ((IdentifierExpr *)ie)->getString();
    delete ie;
	vre->depth = 0;
	vre->value = vre->scope->lookup(name, vre->depth);
	if (!vre->value)
	{
		addError(Error(&current, "Unknown variable", name));
		delete vre;
		return 0;
	}
    
	while (true)
	{
		fprintf(log_file, ">>>> parseVarRef %d\n", current.type);
		if (current.type == POINTER)
		{
			vre->add(VarRefElement(VARREF_DEREF));
			next();
		}
		else if (current.type == DOT)
		{
			next();
			if (current.type != IDENTIFIER)
			{
				addError(Error(&current, "Need identifier for member access"));
				return vre;
			}
			IdentifierExpr * m = (IdentifierExpr *)parseIdentifier();
			vre->add(VarRefElement(VARREF_MEMBER, m));
		}
		else if (current.type == OPEN_SQUARE)
		{
			Expr * s = parseSquare();
			if (!s)
			{
				addError(Error(&current, "Invalid array subscript"));
				return 0;
			}
			vre->add(VarRefElement(VARREF_ARRAY, s));
		}
		else
		{
			return vre;
		}
	}

	return vre;
}

Expr * Parser::parseDef()
{
	fprintf(log_file, "Entering def\n");

	bool is_macro = (current.type == MACRO);
	bool is_extern = (current.type == EXTERN);
	bool is_generic = (current.type == GENERIC);

	next();

    if (parsing_imports && (is_macro || is_extern || is_generic))
    {
        addError(Error(&current, "Can't have non-plain functions in interface"));
    }

	if (current.type != IDENTIFIER)
	{
		addError(Error(&current, "Expected identifier after def"));
		return 0;
    }

	FunctionType * ft;

	std::string name = getIdentifier("Expected function name");
	std::string lib = "";
	size_t pos = name.find(":");
	if (pos != std::string::npos)
	{
		lib = name.substr(0, pos);
		name = name.substr(pos + 1);
	}

	if (is_extern)
	{
		ft = new ExternalFunctionType(calling_convention);
	}
	else if (is_generic)
	{
		ft = new GenericFunctionType();
        types->add(ft, name);
	}
	else
	{
		ft = new FunctionType(is_macro);
	}
    
	bool already_added = false;

	// Need to add proper argument overloading for generics...
    FunctionScope * prev = 0;
    
    prev = current_scope->lookup_function(name);

	// So, for a generic function, only the generic function exists in the scope, and it is
	// the address of the function table. Ignore specialisers other than tacking them onto the generic.

	Value * v = 0;  

	if (prev)
	{
		if (prev->isGeneric())
		{
                //printf("Found generic!\n");
		}
		else
		{
                //printf("Found non-generic %s\n", prev->fqName().c_str());
		}
	}

	if (prev && (is_extern || prev->isGeneric()))
	{
		already_added = true;
		if (is_extern)
		{
			printf("Already added\n");
			// Needs fixed
		}
		else
		{
			char buf[4096];
			sprintf(buf, "_generic_%s_%d", name.c_str(), generic_counter);
			name = buf;
			v = new Value(name, ft);
			current_scope->add(v);
			generic_counter++;
		}
	}
	else
	{
        if (!parsing_imports)
        {
            v = new Value(name, ft);
            current_scope->add(v);
        }
	}

	FunctionScope * fs = new FunctionScope(current_scope,
		name,
		ft);

    if (prev && prev->isGeneric())
    {
        prev->getType()->registerSpecialiser(fs);
    }
    else
    {
        current_scope->addFunction(fs);
    }

	DefExpr * ret = new DefExpr(ft, fs, is_macro, is_extern, is_generic,
		v, name, lib);

	if (current.type != OPEN_BRACKET)
	{
		addError(Error(&current, "Expected open bracket"));
		return ret;
	}

	current_scope = fs;

	next();

	while (true)
	{
		if (current.type == CLOSE_BRACKET)
		{
			next();

			if (current.type == EOL)
			{
				next();

				if (is_extern || is_generic)
				{
					current_scope = current_scope->parent();
					if (already_added)
					{
						return 0; // Already defined. Should check arguments are compatible here
					}
					return ret;
				}

				if (current.type != BEGIN)
				{
                    if (parsing_imports)
                    {
                        imports->add(module, name, ft);
                        return ret;
                    }
                    else
                    {
                        addError(Error(&current, "Expected def body"));
                    }
				}
				else
				{
					Expr * body = parseBlock();
					ret->setBody(body);
				}

				current_scope = current_scope->parent();
                types->add(ft, name);
                return ret;
			}
			else
			{
				Type * t = parseType();
				if (t)
				{
					ft->setReturn(t);
					if (current.type != EOL)
					{
						fprintf(log_file, ">>>>> %d\n", current.type);

						addError(Error(&current,
							"Expected EOL after return type"));
						expectedEol();
					}
					else
					{
						next();

						if (is_extern || is_generic || parsing_imports)
						{
							current_scope = current_scope->parent();
                            if (parsing_imports)
                            {
                                imports->add(module, name, ft);
                            }
                            
							return ret;
						}

                        if (current.type != BEGIN)
						{
							addError(Error(&current, "Expected def body"));
						}
						else
						{
							Expr * body = parseBlock();
							ret->setBody(body);
						}
                        
                        types->add(ft, name);
						current_scope = current_scope->parent();
						return ret;
					}
				}
				else
				{
					addError(Error(&current, "Expected type"));
				}
			}
		}
		else if (current.type == COMMA)
		{
			next();
		}
		else
		{
			if (is_generic)
			{
				if (current.type != IDENTIFIER)
				{
					addError(Error(&current, "Expected identifier"));
				}
                std::string pname = getIdentifier("Expected parameter name");
				ft->addParam(pname, 0);
			}
			else
			{
				Type * t = parseType();
				if (!t)
				{
					addError(Error(&current, "Expected type"));
					current_scope = current_scope->parent();
					return ret;
				}

				if (current.type != IDENTIFIER)
				{
					addError(Error(&current, "Expected identifier"));
				}

                std::string pname = getIdentifier("Expected parameter name");
				ft->addParam(pname, t);
				fs->addArg(new Value(pname, t));
			}
		}
	}

	current_scope = current_scope->parent();
	return 0;

}

Expr * Parser::parseAddressOf()
{
	next();
	Expr * e = parsePrimary();
	if (!e)
	{
		addError(Error(&current, "Cannot take address of non-primary expr"));
		return 0;
	}
	else
	{
		return new AddressOfExpr(e);
	}
}

Expr * Parser::parseFptr()
{
	next();

	bool is_extern = false;
	if (current.type == EXTERN)
	{
		is_extern = true;
		next();
	}

	FunctionType * ft = is_extern ?
		new ExternalFunctionType(calling_convention) : new FunctionType(false);

	if (current.type != OPEN_BRACKET)
	{
		addError(Error(&current, "Expected open bracket in fptr definition"));
		return 0;
	}
	next();

	while (current.type != CLOSE_BRACKET)
	{
		Type * t = parseType();
		if (!t)
		{
			return 0;
		}

		std::string n = "";
		if (current.type == IDENTIFIER)
		{
			n = getIdentifier("Expected param name");
            if (n == "")
            {
                return 0;
            }
		}

		if (current.type == COMMA)
		{
			next();
		}
		else if (current.type == CLOSE_BRACKET)
		{
			next();
			break;
		}
		else
		{
			addError(Error(&current, "Expected comma in fptr definition"));
            return 0;
		}
		ft->addParam(n, t);
	}

	Type * t = parseType();
	if (t)
	{
		ft->setReturn(t);
	}
	else
	{
		printf("Couldn't figure out return type\n");
		return 0;
	}

    std::string tname = getIdentifier("Expected type name");
	if (tname == "")
	{
		push();
		return 0;
	}

	types->add(ft, tname);
	return 0;
}

Type * Parser::parseType()
{
	bool is_const = false;
	if (current.type == CONST)
	{
		is_const = true;
		next();
	}

	// Look up base type
	std::string i = getIdentifier("Expected type name");
    if (i == "")
    {
        push();
        return 0;
    }
    
	Type * ret_type = types->lookup(i);
	if (!ret_type)
	{
		fprintf(log_file, "Looked up [%s], didn't find it\n", i.c_str());
		push();
		return 0;
	}

	bool is_activation = false;

	while (true)
	{
		fprintf(log_file, ">>> parseType %d\n", current.type);

		if (current.type == POINTER)
		{
			fprintf(log_file, "Pointy!\n");
			i += "^";

			Type * t = types->lookup(i);
			if (!t)
			{
				fprintf(log_file, "New pointer type!\n");
				t = new PointerType(ret_type);
				ret_type = t;
				types->add(ret_type, i);
			}
			else
			{
				fprintf(log_file, "Found pointer type!\n");
				ret_type = t;
			}
		}
		else if (current.type == FUNCVAR)
		{
			if (is_activation)
			{
				addError(Error(&current, "Activation declared twice"));
				return 0;
			}
			else
			{
				is_activation = true;
			}
		}
		else if (current.type == OPEN_SQUARE)
		{
			i += "[";
			next();
			if (current.type == CLOSE_SQUARE)
			{
				i += "]";
				Type * t = types->lookup(i);

				if (!t)
				{
					t = new ArrayType(ret_type);
					ret_type = t;
					types->add(ret_type, i);
				}
				else
				{
					ret_type = t;
				}
			}
			else if (current.type == INTEGER_LITERAL)
			{
				IntegerExpr * ie = (IntegerExpr *)parseInteger();

				if (current.type != CLOSE_SQUARE)
				{
					addError(Error(&current, "Unterminated type"));
					return 0;
				}

				char buf[4096];
				sprintf(buf, "%lld", ie->getVal());
				i += buf;
				i += "]";

				Type * t = types->lookup(i);
				if (!t)
				{
					t = new ArrayType(ret_type, ie->getVal());
					ret_type = t;
					types->add(ret_type, i);
				}
				else
				{
					ret_type = t;
				}
                delete ie;
			}
			else
			{
				addError(Error(&current, "[] in type def must be integer"));
				return 0;
			}
		}
		else
		{
			break;
		}

		next();
	}

	if (ret_type && is_activation)
	{
		ret_type = new ActivationType(ret_type);
        types->add(ret_type, "@"+ret_type->name());
	}

	if (is_const)
	{
		ret_type->setConst(true);
	}

	return ret_type;
}

Expr * Parser::parseModule()
{
    next();
    if (current.type != IDENTIFIER)
    {
        addError(Error(&current, "Expected identifier for module"));
        return 0;
    }

    exports->setName(current.toString());
    
    next();
    if (current.type != EOL)
    {
        addError(Error(&current, "Expected EOL after module"));
        expectedEol();
    }
    else
    {
        next();
    }

    if (current.type != BEGIN)
    {
        addError(Error(&current, "Expected interface body"));
        return 0;
    }
    next();

    while (current.type != EOF)
    {
        if (current.type == END)
        {
            next();
            return 0;
        }
        else if (current.type == DEF)
        {
            if (parsing_imports)
            {
                parseDef();
            }
            else
            {
                parseInterfaceDef();
            }
        }
        else
        {
            addError(Error(&current, "Unexpected interface contents"));
            return 0;
        }
    }

    return 0;
}

void Parser::checkInterfaceTypes(Token & current, std::string & name, FunctionScope * prev, std::vector<Type *> & ftypes, Type * ret)
{
    FunctionType * prevt = prev->getType();
    if (prevt->argCount() != ftypes.size())
    {
        char buf[4096];
        sprintf(buf, "Interface for function %s has %ld arguments, implementation %ld", name.c_str(), ftypes.size(), prevt->getParams().size());
        addError(Error(&current, buf));
        return;
    }

    if (prevt->getReturn() != ret)
    {
        char buf[4096];
        sprintf(buf, "Interface for function %s has %s return type, implementation %s", name.c_str(), ret ? ret->name().c_str() : "<null>",
            prevt->getReturn() ? prevt->getReturn()->name().c_str() : "<null>");
        addError(Error(&current, buf));
        return;
    }

    bool ok = true;

    for (unsigned int loopc = 0; loopc < ftypes.size(); loopc++)
    {
        if (ftypes[loopc] != prevt->getParams()[loopc].type)
        {
            char buf[4096];
            sprintf(buf, "Function %s, argument %d: interface has type %s, implementation %s", name.c_str(), loopc, 
                ftypes[loopc]->name().c_str(), prevt->getParams()[loopc].type->name().c_str());
            addError(Error(&current, buf));
            ok = false;
        }
    }

    if (ok)
    {
        exports->addExport(name.c_str(), prev);
    }
}

Expr * Parser::parseInterfaceDef()
{
    if (current.type != DEF)
    {
        addError(Error(&current, "Can't have non-plain functions in interface"));
    }

    next();

    if (current.type != IDENTIFIER)
    {
        addError(Error(&current, "Expected identifier after def"));
        return 0;
    }

    std::string name = getIdentifier("Expected function name");

    FunctionScope * prev = 0;
    prev = current_scope->lookup_function(name);

    if (!prev)
    {
        addError(Error(&current, "Interface defines unimplemented function!\n", name.c_str()));
        return 0;
    }
 
    if (current.type != OPEN_BRACKET)
    {
        addError(Error(&current, "Expected open bracket"));
        return 0;
    }

    next();

    std::vector<Type *> ftypes;
    Type * ret = 0;

    while (true)
    {
        if (current.type == CLOSE_BRACKET)
        {
            next();

            if (current.type == EOL)
            {
                next();
                checkInterfaceTypes(current, name, prev, ftypes, ret);
                return 0;
            }
            else
            {
                Type * t = parseType();
                ret = t;
                if (t)
                {
                    if (current.type != EOL)
                    {
                        addError(Error(&current,
                            "Expected EOL after return type"));
                        expectedEol();
                    }
                    else
                    {
                        next();
                        checkInterfaceTypes(current, name, prev, ftypes, ret);
                        return 0;
                    }
                }
                else
                {
                    addError(Error(&current, "Expected type"));
                }
            }
        }
        else if (current.type == COMMA)
        {
            next();
        }
        else
        {
            Type * t = parseType();
            if (!t)
            {
                addError(Error(&current, "Expected type"));
                return 0;
            }

            ftypes.push_back(t);
            
            if (current.type != IDENTIFIER)
            {
                addError(Error(&current, "Expected identifier"));
            }

            std::string pname = getIdentifier("Expected parameter name");
        }
    }

    checkInterfaceTypes(current, name, prev, ftypes, ret);
    return 0;
}

void Parser::expectedEol()
{
	while (current.type != EOL && current.type != DONE)
	{
		next();
	}
}

Type * VarRefExpr::checkType(Codegen * c)
{
	Type * ret = value->type;
	if (!ret)
	{
		printf("Null type in checkType!\n");
		return 0;
	}

	for (unsigned int loopc = 0; loopc < elements.size(); loopc++)
	{
		VarRefElement & vre = elements[loopc];
		switch (vre.type)
		{
		case VARREF_DEREF:
		{
			if (ret->canDeref())
			{
				ret = ret->derefType();
			}
			else
			{
				fprintf(log_file, "Not derefable - type %s\n", ret->name().c_str());
				return 0;
			}
			break;
		}
		case VARREF_ARRAY:
		{
			if (ret->canIndex())
			{
				fprintf(log_file, ">>> index\n");
				ret = ret->indexType();
			}
			else
			{
				fprintf(log_file, "Not indexable - type %s\n", ret->name().c_str());
				return 0;
			}
			break;
		}
		case VARREF_MEMBER:
		{
			if (ret->canField())
			{
				fprintf(log_file, ">>> field\n");
				ret = ret->fieldType(((IdentifierExpr *)vre.subs)->getString());
			}
			else
			{
				fprintf(log_file, "Not fieldable - type %s\n", ret->name().c_str());
				return 0;
			}
			break;
		}
		default:
		{
			fprintf(log_file, "???");
			break;
		}
		}
	}

	return ret;
}

void VarRefExpr::print(int i)
{
	indent(i);
	fputs(value->name.c_str(), log_file);
	for (unsigned int loopc = 0; loopc < elements.size(); loopc++)
	{
		VarRefElement & vre = elements[loopc];
		switch (vre.type)
		{
		case VARREF_DEREF:
		{
			fprintf(log_file, "^");
			break;
		}
		case VARREF_ARRAY:
		{
			fprintf(log_file, "[");
			vre.subs->print(0);
			fprintf(log_file, "]");
			break;
		}
		case VARREF_MEMBER:
		{
			fprintf(log_file, ".");
			vre.subs->print(0);
			break;
		}
		default:
		{
			fprintf(log_file, "??? <%d>", vre.type);
			break;
		}
		}
	}
}

void VarDefExpr::print(int i)
{
	indent(i);
	fprintf(log_file, "Vardef type %s name %s %s", value->type->name().c_str(), value->name.c_str(), is_const ? "const" : "");
	if (assigned)
	{
		fprintf(log_file, " = ");
		assigned->print(0);
	}
}

Value * VarDefExpr::codegen(Codegen * c)
{
	Value * r = 0;
	if (assigned)
	{
		r = assigned->codegen(c);
		if (r && r->type && r->type->canActivate() && (!value->type->canActivate()))
		{
			r = r->type->getActivatedValue(c, r);
		}
	}

	if (!value->type->construct(c, value, r))
	{
		printf("Can't construct! Type %s to %s\n", value->type->name().c_str(), r->type ? r->type->name().c_str() : "<number>");
	}
	return 0;
}

Value * Block::codegen(Codegen * c)
{
	fprintf(log_file, "In block!\n");

	Value * ret = c->getRet();

	std::list<Expr *>::iterator it = contents.begin();
	while (it != contents.end())
	{
		Value * v = (*it)->codegen(c);
		it++;
		if (v && ret)
		{
			c->block()->add(Insn(MOVE, ret, v));
		}
	}
	return 0;
}

Value * IntegerExpr::codegen(Codegen * c)
{
    return c->getInteger(getVal());
}

Value * ConfigConstantExpr::codegen(Codegen * c)
{
    uint64 ret;
    if (configuration->lookupConfigConstant(constant_name, ret))
    {
        return c->getInteger(ret);
    }
    
    printf("Config constant %s not found in this config!\n",
           constant_name.c_str());
    return 0;
}

bool evil_hack = false;

Value * IdentifierExpr::codegen(Codegen * c)
{
	return value;
}

static bool binary_result(Value * l, Value * r, Type * & t)
{
	if (!l->type || !r->type)
	{
		t = register_type;
		return false;
	}

	// C rules - if either operand is unsigned, so is result
	if (l->type->isSigned() && r->type->isSigned())
	{
		t = signed_register_type;
		return true;
	}
	else
	{
		t = register_type;
	}

	return false;
}

Value * BinaryExpr::codegen(Codegen * c)
{
	Value * rh = rhs->codegen(c);
	if (!rh)
	{
		printf("No right hand side in binaryexpr!\n");
		return 0;
	}

	uint64 equality_op = (((uint64)'=' << 32) | '=');
	uint64 inequality_op = (((uint64)'=' << 32) | '!');
	uint64 lte_op = (((uint64)'=' << 32) | '<');
	uint64 gte_op = (((uint64)'=' << 32) | '>');
	uint64 eval_op = (((uint64)':' << 32) | '=');
	uint64 lshift_op = (((uint64)'<' << 32) | '<');
	uint64 rshift_op = (((uint64)'>' << 32) | '>');
    uint64 a_rshift_op = (((uint64)'>' << 32) | '-');

	if (token.toString() == "and")
	{
		Value * lh = lhs->codegen(c);
		Value * cmp1 = c->getTemporary(register_type, "land1");
		Value * cmp2 = c->getTemporary(register_type, "land2");
		Value * cmp3 = c->getTemporary(register_type, "land3");
		c->block()->add(Insn(CMP, lh, Operand::usigc(0)));
		c->block()->add(Insn(SELEQ, cmp1, Operand::usigc(0),
			Operand::usigc(LOGICAL_TRUE)));
		c->block()->add(Insn(CMP, rh, Operand::usigc(0)));
		c->block()->add(Insn(SELEQ, cmp2, Operand::usigc(0),
			Operand::usigc(LOGICAL_TRUE)));
		c->block()->add(Insn(AND, cmp3, cmp1, cmp2));
		return cmp3;
	}
	else if (token.toString() == "or")
	{
		Value * lh = lhs->codegen(c);
		Value * v = c->getTemporary(register_type, "lor");
		c->block()->add(Insn(OR, v, lh, rh));
		c->block()->add(Insn(CMP, v, Operand::usigc(0)));
		c->block()->add(Insn(SELEQ, v, Operand::usigc(0),
			Operand::usigc(LOGICAL_TRUE)));
		return v;
	}
	else if (token.toString() == "xor")
	{
		Value * lh = lhs->codegen(c);
		Value * cmp1 = c->getTemporary(register_type, "land1");
		Value * cmp2 = c->getTemporary(register_type, "land2");
		Value * cmp3 = c->getTemporary(register_type, "land3");
		c->block()->add(Insn(CMP, lh, Operand::usigc(0)));
		c->block()->add(Insn(SELEQ, cmp1, Operand::usigc(0),
			Operand::usigc(LOGICAL_TRUE)));
		c->block()->add(Insn(CMP, rh, Operand::usigc(0)));
		c->block()->add(Insn(SELEQ, cmp2, Operand::usigc(0),
			Operand::usigc(LOGICAL_TRUE)));
		c->block()->add(Insn(XOR, cmp3, cmp1, cmp2));
		return cmp3;
	}
	else if (op == '+')
	{
		Value * lh = lhs->codegen(c);
		Type * t = 0;
		binary_result(lh, rh, t);
		Value * v = c->getTemporary(t, "add");
		fprintf(log_file, ">>> Adding an add\n");
		c->block()->add(Insn(ADD, v, lh, rh));
		return v;
	}
	else if (op == '-')
	{
		Value * lh = lhs->codegen(c);
		Type * t = 0;
		binary_result(lh, rh, t);
		Value * v = c->getTemporary(t, "sub");
		c->block()->add(Insn(SUB, v, lh, rh));
		return v;
	}
	else if (op == '*')
	{
		Value * lh = lhs->codegen(c);
		Type * t = 0;
		bool is_signed = binary_result(lh, rh, t);
		Value * v = c->getTemporary(t, "mul");
		c->block()->add(Insn(is_signed ? MULS : MUL, v, lh, rh));
		return v;
	}
	else if (op == '/')
	{
		Value * lh = lhs->codegen(c);
		Type * t = 0;
		bool is_signed = binary_result(lh, rh, t);
		Value * v = c->getTemporary(t, "div");
		c->block()->add(Insn(is_signed ? DIVS : DIV, v, lh, rh));
		return v;
	}
	else if (op == '%')
	{
		Value * lh = lhs->codegen(c);
		Type * t = 0;
		bool is_signed = binary_result(lh, rh, t);
		Value * v = c->getTemporary(t, "rem");
		c->block()->add(Insn(is_signed ? REMS : REM, v, lh, rh));
		return v;
	}
	else if (op == lshift_op)
	{
		Value * lh = lhs->codegen(c);
		Value * v = c->getTemporary(register_type, "lshift");
		c->block()->add(Insn(SHL, v, lh, rh));
		return v;
	}
	else if (op == rshift_op)
	{
		Value * lh = lhs->codegen(c);
		Value * v = c->getTemporary(lh->type ? lh->type : register_type, "rshift");
		c->block()->add(Insn(SHR, v, lh, rh));
		return v;
	}
    else if (op == a_rshift_op)
    {
        Value * lh = lhs->codegen(c);
        Value * v = c->getTemporary(lh->type ? lh->type : register_type, "a_rshift");
        c->block()->add(Insn(SAR, v, lh, rh));
        return v;
    }
	else if (op == '&')
	{
		Value * lh = lhs->codegen(c);
		Value * v = c->getTemporary(register_type, "aand");
		c->block()->add(Insn(AND, v, lh, rh));
		return v;
	}
	else if (op == '|')
	{
		Value * lh = lhs->codegen(c);
		Value * v = c->getTemporary(register_type, "aor");
		c->block()->add(Insn(OR, v, lh, rh));
		return v;
	}
	else if (op == equality_op)
	{
		Value * lh = lhs->codegen(c);
		Value * v = c->getTemporary(register_type, "eq");
		c->block()->add(Insn(CMP, lh, rh));
		c->block()->add(Insn(SELEQ, v, Operand::usigc(1), Operand::usigc(0)));
		return v;
	}
	else if (op == inequality_op)
	{
		Value * lh = lhs->codegen(c);
		Value * v = c->getTemporary(register_type, "neq");
		c->block()->add(Insn(CMP, lh, rh));
		c->block()->add(Insn(SELEQ, v, Operand::usigc(0), Operand::usigc(1)));
		return v;
	}
	else if (op == '<')
	{
		Value * lh = lhs->codegen(c);
		Value * v = c->getTemporary(register_type, "lt");
		c->block()->add(Insn(CMP, lh, rh));
		c->block()->add(Insn(SELGE, v, Operand::usigc(0), Operand::usigc(1)));
		return v;
	}
	else if (op == '>')
	{
		Value * lh = lhs->codegen(c);
		Value * v = c->getTemporary(register_type, "gt");
		c->block()->add(Insn(CMP, lh, rh));
		c->block()->add(Insn(SELGT, v, Operand::usigc(1), Operand::usigc(0)));
		return v;
	}
	else if (op == lte_op)
	{
		Value * lh = lhs->codegen(c);
		Value * v = c->getTemporary(register_type, "lte");
		c->block()->add(Insn(CMP, lh, rh));
		c->block()->add(Insn(SELGT, v, Operand::usigc(0), Operand::usigc(1)));
		return v;
	}
	else if (op == gte_op)
	{
		Value * lh = lhs->codegen(c);
		Value * v = c->getTemporary(register_type, "gte");
		c->block()->add(Insn(CMP, lh, rh));
		c->block()->add(Insn(SELGE, v, Operand::usigc(1), Operand::usigc(0)));
		return v;
	}
	else if (op == '^')
	{
		Value * lh = lhs->codegen(c);
		Value * v = c->getTemporary(register_type, "axor");
		c->block()->add(Insn(XOR, v, lh, rh));
		return v;
	}
	else if (op == eval_op)
	{
            // TODO: hmm old dead code before ! ?
            // CALL is for OS-native calls
		// Do eval here
		VarRefExpr * vre = dynamic_cast<VarRefExpr *>(lhs);
		if (!vre)
		{
			addError(Error(&token, "Expected varrefexpr"));
			fprintf(log_file, "Expected varrefexpr!\n");
			return 0;
		}

		ActivationType * rhgt = dynamic_cast<ActivationType *>(rh->type);
		if (!rhgt)
		{
			addError(Error(&token, "Eval only works with generator type"));
			return 0;
		}

		BasicBlock * eval_return = c->newBlock("eval_return");
		Value * ipvar = c->getIp();
		if (!ipvar)
		{
			fprintf(log_file, "Can't find __ipvar in return!\n");
			return 0;
		}
		c->block()->add(Insn(MOVE, ipvar, eval_return));
		Value * jaddr = c->getTemporary(register_type, "jaddr");
		c->block()->add(Insn(LOAD, jaddr, rh, Operand::usigc(8)));  // __ip
		c->block()->add(Insn(CALL, jaddr));
		c->setBlock(eval_return);
		Value * ret = c->getTemporary(register_type, "ret");
		c->block()->add(Insn(LOAD, ret, jaddr, Operand::usigc(0))); // __ret
		return ret;
	}
	else if (op == '=')
	{
		fprintf(log_file, ">>>> Checking assign\n");
		VarRefExpr * vre = dynamic_cast<VarRefExpr *>(lhs);
		if (!vre)
		{
			addError(Error(&token, "Expected varrefexpr"));
			fprintf(log_file, "Expected varrefexpr!\n");
			return 0;
		}

		Type * t = vre->checkType(c);
		if (!t)
		{
			fprintf(log_file, "Type check failed!\n");
			addError(Error(&token, "Type not assignable"));
			return 0;
		}
		else
		{
			fprintf(log_file, "Type check succeeded! %s\n", t->name().c_str());
		}

		if (vre->value->isConst())
		{
			addError(Error(&token, "Cannot assign to const"));
			fprintf(log_file, "Attempt to assign to const!\n");
			return 0;
		}

		vre->store(c, rh);
		return rh;
	}
	else
	{
		addError(Error(&token, "Unknown binop"));
		fprintf(log_file, "Argl unknown binop %llx %c for codegen!\n", op,
                (unsigned char)(op & 0xff));
	}

    delete rh;
	return 0;
}

Value * UnaryExpr::codegen(Codegen * c)
{
	Value * e = expr->codegen(c);
	if (token.toString() == "not")
	{
		Value * v = c->getTemporary(register_type, "lnot");
		c->block()->add(Insn(CMP, e, Operand::usigc(0)));
		c->block()->add(Insn(SELEQ, v, Operand::usigc(LOGICAL_TRUE),
			Operand::usigc(0)));
		return v;
	}
	else if (op == '@')
	{
		Value * v = c->getTemporary(register_type, "addrof");
		c->block()->add(Insn(GETADDR, v, e, Operand::usigc(0)));  // FIXME
		return v;
	}
	else if (op == '~')
	{
		Value * v = c->getTemporary(register_type, "anot");
		c->block()->add(Insn(NOT, v, e));
		return v;
	}
	else if (op == '!')
	{
		if (!e->type->canActivate())
		{
			addError(Error(&token, "Can't activate non-activation"));
			return 0;
		}

		e->type->activate(c, e);
		return e;
	}
	else
	{
		fprintf(log_file, "Argl unknown unop %llx %c for codegen!\n", op,
			(char)(op & 0xff));
		addError(Error(&token, "Unknown unop"));
	}

	return 0;
}

Value * StringLiteralExpr::codegen(Codegen * c)
{
	Value * a = c->getTemporary(register_type, "stringaddr");
	c->block()->add(Insn(GETCONSTADDR, a, Operand::usigc(getIndex())));
	return a;
}

Value * Yield::codegen(Codegen * c)
{
	if (ret)
	{
		Value * returnvar = c->getRet();
		if (!returnvar)
		{
			fprintf(log_file, "Can't find return variable!\n");
			return 0;
		}

		Value * to_ret = ret->codegen(c);
		if (!to_ret)
		{
			// Probably undefined function
			return 0;
		}

		Type * to_ret_type = to_ret->type;
		Type * returnvar_type = returnvar->type;

		if (!returnvar_type->canActivate() && to_ret_type  &&
			to_ret_type->canActivate())
		{
			to_ret = to_ret_type->getActivatedValue(c, to_ret);
		}

		c->block()->add(Insn(MOVE, returnvar, to_ret));
	}


	BasicBlock * retblock = c->newBlock("yield");
	c->block()->add(Insn(BRA, Operand(retblock)));
	c->setBlock(retblock);
	BasicBlock * resumeblock = c->newBlock("resume");

	Value * ipvar = c->getIp();
	if (!ipvar)
	{
		fprintf(log_file, "Can't find __ipvar in return!\n");
		return 0;
	}

	c->block()->add(Insn(MOVE, ipvar, resumeblock));

	RegSet res;
	res.set(0);
	c->block()->setReservedRegs(res);

	c->block()->add(Insn(LOAD, Operand::reg(assembler->framePointer()),
		Operand::reg(assembler->framePointer())));
	c->block()->add(Insn(LOAD, Operand::reg(0),
		Operand::reg(assembler->framePointer()),
		Operand::sigc(assembler->ipOffset())));
	c->block()->add(Insn(BRA, Operand::reg(0)));

	c->setBlock(resumeblock);

	// Add args to yield later
	return 0;
}

Value * Return::codegen(Codegen * c)
{
	if (ret)
	{
		Value * returnvar = c->getRet();
		if (!returnvar)
		{
			fprintf(log_file, "Can't find __ret in return!\n");
			return 0;
		}

		Value * to_ret = ret->codegen(c);
		if (!to_ret)
		{
			// Probably undefined function
			return 0;
		}

		Type * to_ret_type = to_ret->type;
		Type * returnvar_type = returnvar->type;

        if (!returnvar_type)
        {
            fprintf(log_file, "Return type is null\n");
            return 0;
        }
        
		if (!returnvar_type->canActivate() && to_ret_type  &&
			to_ret_type->canActivate())
		{
			to_ret = to_ret_type->getActivatedValue(c, to_ret);
		}

		c->block()->add(Insn(MOVE, returnvar, to_ret));
	}

	if (c->callConvention() == CCONV_STANDARD)
	{
		BasicBlock * retblock = c->newBlock("ret");
		c->block()->add(Insn(BRA, Operand(retblock)));
		c->setBlock(retblock);

		Value * ipvar = c->getIp();
		if (!ipvar)
		{
			fprintf(log_file, "Can't find return address in return!\n");
			return 0;
		}

		c->block()->add(Insn(MOVE, ipvar, retblock));

		RegSet res;
		res.set(0);
		c->block()->setReservedRegs(res);

		// Restore old framepointer
		c->block()->add(Insn(LOAD, Operand::reg(assembler->framePointer()),
			Operand::reg(assembler->framePointer())));
		// Load ip from it
		c->block()->add(Insn(LOAD, Operand::reg(0),
			Operand::reg(assembler->framePointer()),
			Operand::sigc(assembler->ipOffset())));
		// Jump back
		c->block()->add(Insn(BRA, Operand::reg(0)));
	}
	else if (c->callConvention() == CCONV_C)
	{
		BasicBlock * ret = c->newBlock("ret");
		c->block()->add(Insn(BRA, ret));
		c->setBlock(ret);
		calling_convention->generateEpilogue(ret, c->getScope());
	}

	return 0;
}

Value * While::codegen(Codegen * c)
{
	BasicBlock * header = c->newBlock("whileheader");
	BasicBlock * loopbody = c->newBlock("whilebody");
	BasicBlock * endblock = c->newBlock("endwhile");

	c->addBreak(endblock);
	c->addContinue(header);

	c->setBlock(header);
	Value * v = condition->codegen(c);
	c->block()->add(Insn(CMP, v, Operand::usigc(0)));
	// if false we jump out, if true we do loop body
	c->block()->add(Insn(BEQ, endblock, loopbody));

	c->setBlock(loopbody);
	body->codegen(c);
	c->block()->add(Insn(BRA, header));

	c->setBlock(endblock);

	c->removeBreak();
	c->removeContinue();

	return 0;
}

bool BinaryExpr::constEval(uint64 & ret)
{
    uint64 lh;
    uint64 rh;
    if (!lhs->constEval(lh))
    {
        return false;
    }
    if (!rhs->constEval(rh))
    {
        return false;
    }
    
	uint64 equality_op = (((uint64)'=' << 32) | '=');
	uint64 inequality_op = (((uint64)'=' << 32) | '!');

    if (op == equality_op)
    {
        ret = (lh == rh) ? LOGICAL_TRUE : 0;
        return true;
    }
    else if (op == inequality_op)
    {
        ret = (lh != rh) ? LOGICAL_TRUE : 0;
        return true;
    }
    else if (token.toString() == "and")
    {
        ret = (lh && rh) ? LOGICAL_TRUE : 0;
        return true;
    }
    else if (token.toString() == "or")
    {
        ret = (lh || rh) ? LOGICAL_TRUE : 0;
        return true;
    }
    else if (token.toString() == "xor")
    {
        ret = (lh && (!rh)) || (rh && (!lh))? LOGICAL_TRUE : 0;
        return true;
    }
    else if (op == '|')
    {
        ret = lh | rh;
        return true;
    }
    else if (op == '&')
    {
        ret = lh & rh;
        return true;
    }
    else if (op == '^')
    {
        ret = lh ^ rh;
        return true;
    }
    
    return false;
}

bool ConfigConstantExpr::constEval(uint64 & ret)
{
    ret = configuration->config_constants[constant_name];
    return true;
}

Value * If::codegen(Codegen * c)
{
    if (constif)
    {
        std::list<IfClause *>::iterator it;

        for (it = clauses.begin(); it != clauses.end(); it++)
        {
            bool evaled;
            uint64 ret;
            IfClause * ic = *it;
            evaled = ic->condition->constEval(ret);
            if (!evaled)
            {
                printf("Failed to evaluate constant if!\n");
                FILE * f = log_file;
                log_file = stdout;
                ic->condition->print(0);
                log_file = f;
                return 0;
            }

            if (ret != 0)
            {
                return ic->body->codegen(c);
            }
        }
        
        if (elseblock)
        {
            return elseblock->codegen(c);
        }
        return 0;
    }

	BasicBlock * elb = 0;
	if (elseblock)
	{
		elb = c->newBlock("if_false");
	}

	BasicBlock * endb = c->newBlock("endif");

	unsigned int count = 0;

	BasicBlock ** ifs = new BasicBlock *[clauses.size() + 2];
	BasicBlock ** ifcs = new BasicBlock *[clauses.size() + 2];
	for (count = 0; count < clauses.size(); count++)
	{
		char buf[4096];
		sprintf(buf, "if_true_%d", count);
		ifs[count] = c->newBlock(buf);

		sprintf(buf, "if_clause_%d", count);
		ifcs[count] = c->newBlock(buf);
	}
	ifs[count] = (elb ? elb : endb);
	ifcs[count] = ifs[count];

	count = 0;
	for (std::list<IfClause *>::iterator it = clauses.begin();
	it != clauses.end(); it++)
	{
		if (count > 0)
		{
			ifs[count - 1]->add(Insn(BRA, ifs[count]));
		}

		c->setBlock(ifcs[count]);

		IfClause * ic = *it;
		Value * v = ic->condition->codegen(c);
		c->block()->add(Insn(CMP, v, Operand::usigc(0)));

		c->block()->add(Insn(BNE, ifs[count], ifcs[count + 1]));
		c->setBlock(ifs[count]);
		ic->body->codegen(c);
		c->block()->add(Insn(BRA, endb));
		count++;
	}

	if (elb)
	{
		c->setBlock(elb);
		elseblock->codegen(c);
		c->block()->add(Insn(BRA, endb));
	}

	c->setBlock(endb);

	delete[] ifs;
	delete[] ifcs;

	return 0;
}

Value * VarRefExpr::codegen(Codegen * c)
{
	// FIXME: fill this in properly
	if (elements.size() > 0 || depth > 0)
	{
		Type * etype = value->type;

		Value * r = c->getTemporary(register_type, "refaddr");

		c->block()->add(Insn(GETADDR, r, value, Operand::usigc(depth)));

		for (unsigned int loopc = 0; loopc < elements.size(); loopc++)
		{
			VarRefElement & vre = elements[loopc];
			etype->calcAddress(c, r, vre.subs);

			switch (vre.type)
			{
			case VARREF_DEREF:
			{
				etype = etype->derefType();
                if (!etype)
                {
                    printf("Failed to deref type!\n");
                    return 0;
                }
                
				break;
			}
			case VARREF_ARRAY:
			{
				etype = etype->indexType();
                if (!etype)
                {
                    printf("Failed to index type!\n");
                    return 0;
                }
				break;
			}
			case VARREF_MEMBER:
			{
				std::string fname = ((IdentifierExpr *)vre.subs)->getString();
				etype = etype->fieldType(fname);

				if (!etype)
				{
					printf("Cannot find member field %s!\n", fname.c_str());
					return 0;
				}

				break;
			}
			default:
			{
				fprintf(log_file, "???");
                return 0;
				break;
			}
			}
		}

		Value * ret = c->getTemporary(etype, "deref_ret");
		c->block()->add(Insn(loadForType(etype), ret, r));
		return ret;
	}

	return value;
}

void VarRefExpr::store(Codegen * c, Value * v)
{
	Value * i = value;

	Value * copied = v;

	Type * vtype = v->type;
	if ((!i->type->canActivate()) && vtype && vtype->canActivate() && vtype->canCopy(vtype->activatedType()))
	{
		copied = vtype->getActivatedValue(c, v);
	}
	else
	{
		copied = v;
	}

	if (!copied->type)
	{
		if (!copied->is_number)
		{
			fprintf(log_file, "Null value type to store!\n");
			return;
		}
		else
		{
			Value * v2 = c->getTemporary(register_type, "varrefintcopy");
			c->block()->add(Insn(MOVE, v2, copied));
			copied = v2;
		}
	}

	if (i->type->inRegister() && copied->type->inRegister() && elements.size() == 0
		&& depth == 0)
	{
		fprintf(log_file, ">>> Adding a move\n");
		c->block()->add(Insn(MOVE, i, copied));
		return;
	}

	Value * r = c->getTemporary(register_type, "refaddr");

	c->block()->add(Insn(GETADDR, r, i, Operand::usigc(depth)));
	Type * etype = i->type;

	for (unsigned int loopc = 0; loopc < elements.size(); loopc++)
	{
		VarRefElement & vre = elements[loopc];
		etype->calcAddress(c, r, vre.subs);
		switch (vre.type)
		{
            case VARREF_DEREF:
            {
                etype = etype->derefType();
                break;
            }
            case VARREF_ARRAY:
            {
                etype = etype->indexType();
                break;
            }
            case VARREF_MEMBER:
            {
                etype = etype->fieldType(((IdentifierExpr *)vre.subs)
                                         ->getString());
                break;
            }
            default:
            {
                assert(false);
                break;
            }
		}
	}

	if (!etype->canCopy(copied->type))
	{
		addError(Error(&token, "Don't know how to copy this type", value->name));
		fprintf(log_file, "Don't know how to copy value of type %s to type %s!\n",
			copied->type ? copied->type->name().c_str() : "constant",
			etype->name().c_str());
		return;
	}

	// FIXME: how to do generator type...
	etype->copy(c, r, copied);
}

Value * BreakpointExpr::codegen(Codegen * c)
{
	c->block()->add(Insn(BREAKP));
	return 0;
}

extern Codegen * root_gc;

Value * Funcall::codegen(Codegen * c)
{
    bool is_import_call = false;
    
    if (name().find(':') != std::string::npos)
    {
        int pos = name().find(':');
        std::string m = name().substr(0, pos);
        std::string f = name().substr(pos+1, std::string::npos);
        
        ImportRec * ir = imports->lookup(m, f);
        if (ir && (!ir->value))
        {
            Value * v = new Value(name(), ir->type);
            printf("Found new import! %s %p %p\n", name().c_str(), v,
                   root_gc->getScope());
            v->setOnStack(true);
            root_gc->getScope()->add(v);
            root_gc->getLocals().push_back(v);
            ir->value = v;
            is_import_call = true;
        }
        else if (!ir)
        {
            printf("Failure to look up import [%s]\n", name().c_str());
        }
        else
        {
                // We found it once already, lookup should work normally
            is_import_call = true;
        }
    }
    
	Value * ptr = scope->lookupLocal(name());

    int depth = 0;
	if (!ptr)
	{
		ptr = scope->lookup(name(), depth);
        
		if (ptr && (depth > 0))
		{
			Value * addrof = c->getTemporary(register_type, "addr_of_ptr");
			c->block()->add(Insn(GETADDR, addrof, ptr, Operand::usigc(depth)));
			c->block()->add(Insn(LOAD, addrof, addrof));
			Value * localptr = c->getTemporary(ptr->type, "local_ptr");
			c->block()->add(Insn(MOVE, localptr, addrof));
			ptr = localptr;
		}
	}
    
	if (!ptr)
	{
		addError(Error(&token, "Could not find function at codegen time", name()));
		return 0;
	}

	if (ptr->type->isMacro())
	{
		addError(Error(&token, "Can't call macros directly",
			name()));
		return 0;
	}
	else if (c->getScope()->getType()->isMacro() && (ptr->type->isMacro() == false))
	{
		addError(Error(&token, "Can't call normal code from macro",
			name()));
		return 0;
	}

	if (!ptr->type->canFuncall())
	{
		addError(Error(&token, "Attempt to call non-function", name()));
		return 0;
	}

	std::vector<Value *> evaled_args;
	for (unsigned int loopc = 0; loopc < args.size(); loopc++)
	{
		Value * v = args[loopc]->codegen(c);
        if (!v)
        {
            printf("Null value in funcall!\n");
            return 0;
        }
        
		if (v->is_number)
		{
			Value * v2 = c->getTemporary(register_type, "funintcopy");
			c->block()->add(Insn(MOVE, v2, v));
			v = v2;
		}
		evaled_args.push_back(v);
	}

	std::string reason;
	if (!ptr->type->validArgList(evaled_args, reason))
	{
		addError(Error(&token, "Parameter mismatch!", name() + " - " + reason));
		return 0;
	}

    Value * sl = c->getTemporary(register_type, "static_link");
    if (depth == 0 && (!is_import_call))
    {
            // Calling our own child, we are static link
        c->block()->add(Insn(MOVE, sl, Operand::reg(assembler->framePointer())));
    }
    else
    {
            // FIXME: doesn't work for import calls. Should be able to
            // call parents
            // Calling sibling, static link is same as ours
        c->block()->add(Insn(LOAD, sl, Operand::reg(assembler->framePointer()),
                             Operand::sigc(assembler->staticLinkOffset())));
        int count = depth - 1;
        if (is_import_call)
        {
            count++;
        }
        for (int loopc=0; loopc<count; loopc++)
        {
            c->block()->add(Insn(LOAD, sl, sl,
                                 Operand::sigc(assembler->staticLinkOffset())));
        }
    }
    
	Value * ret = ptr->type->generateFuncall(c, this, sl, ptr, evaled_args);
	return ret;
}

std::list<Codegen *> macros;

Value * DefExpr::codegen(Codegen * c)
{
	if (is_extern)
	{
		configuration->image->addImport(libname, name);
		Value * addr_of_extfunc = c->getTemporary(register_type, "addr_of_extfunc_" + name);
		c->block()->add(Insn(MOVE, addr_of_extfunc, Operand::extFunction(name)));
		c->block()->add(Insn(LOAD, ptr, addr_of_extfunc));
		return ptr;
	}

	FunctionScope * to_call = scope->lookup_function(name);
	assert(to_call);
	c->block()->add(Insn(MOVE, ptr, Operand(to_call)));

    if (is_generic)
    {
        mtables->add(to_call);
        return ptr;
    }
    
	scope->addFunction(new FunctionScope(scope, type->name(), type));

	Codegen * ch = new Codegen(body, scope);
	if (is_macro)
	{
        ch->setCallConvention(CCONV_MACRO);
		BasicBlock * prologue = ch->block();
		calling_convention->generatePrologue(prologue, root_scope);
		macros.push_back(c);
	}
	else
	{
		codegensptr->push_back(ch);
	}

	ch->generate();

	if (is_macro)
	{
		BasicBlock * epilogue = ch->newBlock("epilogue");
		ch->block()->add(Insn(BRA, epilogue));
		c->setBlock(epilogue);
		calling_convention->generateEpilogue(epilogue, root_scope);
	}

	return ptr;
}

Value * Break::codegen(Codegen * c)
{
	if (!c->currentBreak())
	{
		addError(Error(&token, "Break outside of loop"));
		return 0;
	}

	c->block()->add(Insn(BRA, Operand(c->currentBreak())));
	return 0;
}

Value * Continue::codegen(Codegen * c)
{
	if (!c->currentBreak())
	{
		addError(Error(&token, "Continue outside of loop"));
		return 0;
	}

	c->block()->add(Insn(BRA, Operand(c->currentContinue())));
	return 0;
}

Value * CastExpr::codegen(Codegen * c)
{
    Value * v = c->getTemporary(cast_type, "cast");
    Value * r = to_cast->codegen(c);
    c->block()->add(Insn(MOVE, v, r));
    return v;
}

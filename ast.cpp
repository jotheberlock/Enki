#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "error.h"
#include "codegen.h"
#include "cfuncs.h"
#include "symbols.h"
#include "configfile.h"
#include "image.h"

#define LOGICAL_TRUE 1

IntegerExpr::IntegerExpr(Token * t)
{
    if (t->value.size() > 18)
    {
        printf("Invalidly huge integer! [%s]\n", t->toString().c_str());
        val = 0;
        return;
    }

    val = 0;
    int begin = 0;
    unsigned int base = 10;
    if (t->value.size() > 2 && t->value[0] == '0' && t->value[1] == 'x')
    {
        begin = 2;
        base = 16;
    }
    
    // Do our own strol(l) to make sure we can do 64 bits
    for (unsigned int loopc=begin; loopc<t->value.size(); loopc++)
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
}

IdentifierExpr::IdentifierExpr(Token * t)
{
    char buf[4096];
    unsigned int loopc;
    for (loopc=0; loopc<t->value.size(); loopc++)
    {
       buf[loopc] = (char)t->value[loopc];
    }
    buf[loopc] = 0;

    val = buf;
    static_depth = 0;
    value = 0;
}

StringLiteralExpr::StringLiteralExpr(Token * t)
{
    char buf[4096];
    unsigned int loopc;
    for (loopc=0; loopc<t->value.size(); loopc++)
    {
        buf[loopc] = (char)t->value[loopc];
    }
    buf[loopc] = 0;

    val = buf;
    idx = constants->addConstant(buf, loopc+1, 1);
}

Parser::Parser(Lexer * l)
{
    current_scope = root_scope;
    lexer=l;
    tokens=lexer->tokens();
    count=0;
    next();
    if (current.type != BEGIN)
    {
        addError(Error(&current, "Expected beginning block"));
    }
    else
    {
        root=parseBlock();
    }
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
    Value * v = current_scope->lookup(ret->getString(),depth);
    ret->setValue(v,depth);
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

int Parser::getPrecedence()
{
    bool dummy;
    OpRec rec;

    if (current.value.size() == 0)
    {
	return -1;
    }

    if (!lexer->isOp(current.value[0], current.value.size() == 1 ? 0 : current.value[1], dummy, rec, current.toString()))
    {
        return -1;
    }

    if (rec.type != BINOP)
        return -1;

    return rec.pri;
}

Expr * Parser::parseBinopRHS(int prec, Expr * lhs)
{
    while(true)
    {
        int tprec = getPrecedence();
        if (tprec < prec)
            return lhs;
        Token binop = current;
        next();
        Expr * rhs = parseUnary();
        if (!rhs)
            return 0;
        int nprec = getPrecedence();
        if (tprec < nprec)
        {
            rhs = parseBinopRHS(tprec+1, rhs);
            if (rhs == 0)
                return 0;
        }

        lhs = new BinaryExpr(&binop, lhs, rhs);

        if (current.type == DONE)
            return lhs;
    }
}

Expr * Parser::parsePrimary()
{
    if(current.type == INTEGER_LITERAL)
    { 
        return parseInteger();
    }
    else if(current.type == STRING_LITERAL)
    {
        return parseStringLiteral();
    }
    else if(current.type == IDENTIFIER)
    {
        Expr * ret = parseIdentifier();
        if (current.type == OPEN_BRACKET)
        {
            return parseFuncall((IdentifierExpr *)ret);
        }
        else
        {
            return parseVarRef(ret);
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

    uint32_t first = 0;
    uint32_t second = 0;
    if (current.value.size() > 0)
        first = current.value[0];
    if (current.value.size() > 1)
        second = current.value[1];

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
                }
                else
                {
                    next();
                }
            
                return ret;
            }
        }
    }
    else if (current.type == IF)
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
        }
        else
        {
            next();
        }

        return new Continue(current);
    }
    else if (current.type == DEF || current.type == MACRO || current.type == EXTERN)
    {
        return parseDef();
    }

    Expr * ret = parseExpr();
    if (current.type != EOL)
    {
        addError(Error(&current, "Expected EOL at end of line"));
    }
    else
    {
        next();
    }
    
    return ret;
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
    next();
    Expr * c = parseExpr();

    Block * b = 0;
    Block * e = 0;
    
	if (current.type != EOL)
	{
	    addError(Error(&current, "Expected EOL after if expr"));
	}
	next();
    
	if (current.type != BEGIN)
	{
  	    addError(Error(&current, "Expected block after if"));
    }
    else
    {
        b = (Block *)parseBlock();
    }
    
    if (current.type == ELSE)
    {
        next();
        if (current.type != EOL)
        {
            addError(Error(&current, "Expected EOL after else"));
        }
        next();
        if (current.type != BEGIN)
        {
            addError(Error(&current, "Expected block after else"));
        }
        else
        {
            e = (Block *)parseBlock();
        }
    }
    
    if(c && b)
    {
        return new If(c,b,e);
    }
    else
    {
        return 0;
    }
}

Expr * Parser::parseWhile()
{
    next();
    Expr * c = parseExpr();

    Block * b = 0;
    
	if (current.type != EOL)
	{
	    addError(Error(&current, "Expected EOL after while expr"));
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

    if (c && b)
    {
        return new While(c,b);
    }
    else
    {
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
    
    Expr * i = parseIdentifier();
    if (i)
    {
        Expr * assigned = 0;
        if (current.toString() == "=")
        {
            fprintf(log_file, "Caught assign\n");
            next();
            assigned = parseExpr();
        }

        Value * v = new Value(((IdentifierExpr *)i)->getString(),
                              t);
        current_scope->add(v);
        return new VarDefExpr(v, assigned, is_c);
    }
    else
    {
        addError(Error(&current, "Type not followed by variable name"));
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

    SymbolScope * nb = new SymbolScope(current_scope);
    current_scope = nb;
    
    Block * ret = new Block();

    while(true)
    {
        if (current.type == DONE || current.type == END)
        {
            if (current.type == END)
                next();
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
                addError(Error(&current, "Can't parse expression in block"));
                break;
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
    if (current.type != IDENTIFIER)
    {
        addError(Error(&current, "Expected identifier for struct"));
        return 0;
    }

    IdentifierExpr * ie = (IdentifierExpr *)parseIdentifier();
    std::string sname = ie->getString();
    Type * t = lookupType(sname);
    if (t)
    {
        addError(Error(&current, "Type already defined", sname));
        return 0;
    }

    StructType * st = new StructType(sname, type == UNION);
    addType(st, sname); // Add here so it can reference itself

    next();
    
    if (current.type != BEGIN)
    {
        addError(Error(&current, "Expected block for struct"));
        return 0;
    }
    next();
    
    while (current.type != END)
    {
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
        
        IdentifierExpr * ie = (IdentifierExpr *)parseIdentifier();
        st->addMember(ie->getString(), t);

        if (current.type != EOL)
        {
            addError(Error(&current, "Expected EOL in struct"));
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
    
    next();

    if (current.type != IDENTIFIER)
    {
        addError(Error(&current, "Expected identifier after def"));
        return 0;
    }

    IdentifierExpr * ie = (IdentifierExpr *)parseIdentifier();
    FunctionType * ft;

	std::string name = ie->getString();
	std::string lib = "";
	size_t pos = name.find(":");
	if (pos != std::string::npos)
	{
		lib = name.substr(0, pos);
		name = name.substr(pos+1);
	}

    if (is_extern)
    {
        ft = new ExternalFunctionType(calling_convention);
    }
    else
    {
        ft = new FunctionType(is_macro);
    }
	
	Value * v = new Value(name, ft);
	current_scope->add(v);

    FunctionScope * fs = new FunctionScope(current_scope,
                                           name,
                                           ft);

    DefExpr * ret = new DefExpr(ft, fs, is_macro, is_extern, v, name, lib);
    
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

		if (is_extern)
		{
		    current_scope = current_scope->parent();
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

                current_scope = current_scope->parent();
                return ret;
            }
            else
            {
                Type * t = parseType();
                if (t)
                {
                    ft->addReturn(t);
                    if (current.type != EOL)
                    {
                        fprintf(log_file, ">>>>> %d\n", current.type);

                        addError(Error(&current, 
                                       "Expected EOL after return type"));
                    }
                    else
                    {
		        next();

			if (is_extern)
			{
			    current_scope = current_scope->parent();
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

                        addType(ft, ie->getString());
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
            
            IdentifierExpr * ie = (IdentifierExpr *)parseIdentifier();
            ft->addParam(ie->getString(), t);
            fs->addArg(new Value(ie->getString(), t));
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

    while(current.type != CLOSE_BRACKET)
    {
        Type * t = parseType();
	if (!t)
	{
            return 0;
        }

	std::string n = "";
	if (current.type == IDENTIFIER)
	{
	    IdentifierExpr * ie = (IdentifierExpr *)parseIdentifier();
	    if (!ie)
	    {
	        return 0;
	    }
	    n = ie->getString();
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
        }
	ft->addParam(n, t);
    }
    
    Type * t = parseType();
    if (t)
    {
        ft->addReturn(t);
    }
    else
    {
        printf("Couldn't figure out return type\n");
	return 0;
    }
    
    Expr * ie = parseIdentifier();
    if (!ie)
    {
        push();
        return 0;
    }

    std::string tname = ((IdentifierExpr *)ie)->getString();
    addType(ft, tname);
    return 0;
}

Type * Parser::parseType()
{
    Expr * ie = parseIdentifier();
    if (!ie)
    {
        push();
        return 0;
    }

        // Look up base type
    std::string i = ((IdentifierExpr *)ie)->getString();
    
    Type * ret_type = lookupType(i);
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
            
            Type * t = lookupType(i);
            if (!t)
            {
                fprintf(log_file, "New pointer type!\n");
                t = new PointerType(ret_type);
                ret_type = t;
                addType(ret_type, i);    
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
                Type * t = lookupType(i);
                
                if (!t)
                {
                    t = new ArrayType(ret_type);
                    ret_type = t;
                    addType(ret_type, i);    
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
                sprintf(buf, "%ld", ie->getVal());
                i += buf;
                i += "]";
                
                Type * t = lookupType(i);
                if (!t)
                {
                    t = new ArrayType(ret_type, ie->getVal());
                    ret_type = t;
                    addType(ret_type, i);    
                }
                else
                {
                    ret_type = t;
                }
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

    if(ret_type && is_activation)
    {
        ret_type = new ActivationType(ret_type);
    }
    
    return ret_type;
}

Type * VarRefExpr::checkType(Codegen * c)
{
    Type * ret = value->type;
    
    for (unsigned int loopc=0; loopc<elements.size(); loopc++)
    {
        VarRefElement & vre = elements[loopc];
        switch(vre.type)
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
    for (unsigned int loopc=0; loopc<elements.size(); loopc++)
    {
        VarRefElement & vre = elements[loopc];
        switch(vre.type)
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
                fprintf(log_file, "???");
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
    if (assigned)
    {
        Value * r = assigned->codegen(c);
        c->block()->add(Insn(MOVE, value, r));
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
    return new Value(getVal());
}

bool evil_hack = false;

Value * IdentifierExpr::codegen(Codegen * c)
{
    return value;
}

Value * BinaryExpr::codegen(Codegen * c)
{
    Value * rh = rhs->codegen(c);
    if (!rh)
    {
        printf("No right hand side in binaryexpr!\n");
        return 0;
    }
    
    uint64_t equality_op = (((uint64_t)'=' << 32) | '=');
    uint64_t inequality_op = (((uint64_t)'=' << 32) | '!');
    uint64_t lte_op = (((uint64_t)'=' << 32) | '<');
    uint64_t gte_op = (((uint64_t)'=' << 32) | '>');
    uint64_t eval_op = (((uint64_t)':' << 32) | '=');
    uint64_t lshift_op = (((uint64_t)'<' << 32) | '<');
    uint64_t rshift_op = (((uint64_t)'>' << 32) | '>');

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
        Value * v = c->getTemporary(register_type, "add");
        fprintf(log_file, ">>> Adding an add\n");
        c->block()->add(Insn(ADD, v, lh, rh));
        return v;
    }
    else if (op == '-')
    {
        Value * lh = lhs->codegen(c);
        Value * v = c->getTemporary(register_type, "sub");
        c->block()->add(Insn(SUB, v, lh, rh));
        return v;        
    }
    else if (op == '*')
    {
        Value * lh = lhs->codegen(c);
        Value * v = c->getTemporary(register_type, "mul");
        c->block()->add(Insn(MUL, v, lh, rh));
        return v;
    }
    else if (op == '/')
    {
        Value * lh = lhs->codegen(c);
        Value * v = c->getTemporary(register_type, "div");
        c->block()->add(Insn(DIV, v, lh, rh));
        return v;
    }
    else if (op == '%')
    {
        Value * lh = lhs->codegen(c);
        Value * v = c->getTemporary(register_type, "rem");
        c->block()->add(Insn(REM, v, lh, rh));
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
        Value * v = c->getTemporary(register_type, "rshift");
        c->block()->add(Insn(SHR, v, lh, rh));
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
        fprintf(log_file, "Argl unknown binop %x %c for codegen!\n", op,
                op & 0xff);
    }
    
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

	e->type->activate(c,e);
	return e;
    }
    else
    {
        fprintf(log_file, "Argl unknown unop %x %c for codegen!\n", op,
                op & 0xff);
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
            fprintf(log_file, "Can't find __ret in return!\n");
            return 0;
        }

        Value * to_ret = ret->codegen(c);
        
        c->block()->add(Insn(MOVE, returnvar, to_ret));
    }

    
    BasicBlock * retblock = c->newBlock("yield");
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
        
	c->block()->add(Insn(STORE, Operand::reg(assembler->framePointer()), Operand::sigc(assembler->returnOffset()), to_ret));
    }

    if (c->callConvention() == CCONV_STANDARD)
    {
        BasicBlock * retblock = c->newBlock("ret");
        c->setBlock(retblock);

        Value * ipvar = c->getIp();
        if (!ipvar)
        {
            fprintf(log_file, "Can't find __ret in return!\n");
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

Value * If::codegen(Codegen * c)
{
    Value * v = condition->codegen(c);
    c->block()->add(Insn(CMP, v, Operand::usigc(0)));
        // Generate the if part
    BasicBlock * ifb = c->newBlock("iftrue");
    BasicBlock * elb = 0;
    if (elseblock)
    {
        elb = c->newBlock("iffalse");
    }

    BasicBlock * endb = c->newBlock("endif");

    c->block()->add(Insn(BNE, ifb, elb ? elb : endb));
    
    c->setBlock(ifb);
    body->codegen(c);
    ifb->add(Insn(BRA, endb));
    
    if (elb)
    {
        c->setBlock(elb);
        elseblock->codegen(c);
        elb->add(Insn(BRA, endb));
    }
    
    c->setBlock(endb);
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
    
        for (unsigned int loopc=0; loopc<elements.size(); loopc++)
        {
            VarRefElement & vre = elements[loopc];
            value->type->calcAddress(c, r, vre.subs);
            
            switch(vre.type)
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
                    fprintf(log_file, "???");
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
        copied = c->getTemporary(vtype->activatedType(), "varrefcopy");
        Value * r = c->getTemporary(register_type, "varrefcopyaddr");
        c->block()->add(Insn(GETADDR, r, copied, Operand::usigc(0)));
        vtype->copy(c, r, v);
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
    
    for (unsigned int loopc=0; loopc<elements.size(); loopc++)
    {
        VarRefElement & vre = elements[loopc];
        etype->calcAddress(c, r, vre.subs);
        switch(vre.type)
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
                fprintf(log_file, "???");
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
    etype->copy(c,r,copied);
}

Value * BreakpointExpr::codegen(Codegen * c)
{
    c->block()->add(Insn(BREAKP));
    return 0;
}

Value * Funcall::codegen(Codegen * c)
{
	Value * ptr = scope->lookupLocal(name());
	if (!ptr)
	{
		int depth = 0;
		ptr = scope->lookup(name(), depth);
	    Value * addrof = c->getTemporary(register_type, "addr_of_ptr");
	    c->block()->add(Insn(GETADDR, addrof, ptr, Operand::usigc(depth)));
		c->block()->add(Insn(LOAD, ptr, addrof));
	}

	assert(ptr);

    if (ptr->type->isMacro())
    {
        addError(Error(&token, "Can't call macros directly",
                       name()));
    }
    else if (c->getScope()->getType()->isMacro() && (ptr->type->isMacro() == false))
    {
        addError(Error(&token, "Can't call normal code from macro",
                       name()));
    }
    
    assert(ptr->type->canFuncall());

    std::vector<Value *> evaled_args;
    for (unsigned int loopc=0; loopc<args.size(); loopc++)
    {
        Value * v = args[loopc]->codegen(c);
        if (v->is_number)
        {
            Value * v2 = c->getTemporary(register_type, "funintcopy");
            c->block()->add(Insn(MOVE, v2, v));
            v = v2;
        }
        evaled_args.push_back(v);
    }

    if (!ptr->type->validArgList(evaled_args))
    {
        addError(Error(&token, "Parameter mismatch!", name()));
        return 0;
    }

    Value * ret = ptr->type->generateFuncall(c, this, ptr, evaled_args);
    return ret;
}

std::list<Codegen *> macros;

Value * DefExpr::codegen(Codegen * c)
{
    if (is_extern)
    {
        configuration->image->addImport(libname, name);	
	Value * addr_of_extfunc = c->getTemporary(register_type, "addr_of_extfunc_"+name);
	c->block()->add(Insn(MOVE, addr_of_extfunc, Operand::extFunction(name)));
	c->block()->add(Insn(LOAD, ptr, addr_of_extfunc));
        return ptr;
    }
	
	FunctionScope * to_call = scope->lookup_function(name);
    assert(to_call);
	c->block()->add(Insn(MOVE, ptr, Operand(to_call)));

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

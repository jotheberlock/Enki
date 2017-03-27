#ifndef _BACKEND_
#define _BACKEND_

/*
  The Backend takes in an AST produced by main.cpp/the Parser class
  and does all the steps to turn it into e.g. an ARM Linux binary.
  In theory the idea is the AST can be parsed once and multiple Backends
  can operate in multiple threads generating multiple targets simultaneously.
*/

#include "configfile.h"
#include "ast.h"
#include "codegen.h"

class Backend
{
public:

	Backend(Configuration *, Expr *);
    ~Backend();
    
	int process();

protected:

	Configuration * config;
	Expr * root_expr;
	std::list<Codegen *> codegens;

};

#endif

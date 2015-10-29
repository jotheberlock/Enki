#ifndef _BACKEND_
#define _BACKEND_

#include "configfile.h"
#include "ast.h"
#include "codegen.h"

class Backend
{
 public:

    Backend(Configuration *, Expr *);
    int process();
    
 protected:

    Configuration * config;
    Expr * root_expr;
    std::list<Codegen *> codegens;

};

#endif

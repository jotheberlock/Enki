#ifndef _BACKEND_
#define _BACKEND_

#include "configfile.h"
#include "ast.h"

class Backend
{
 public:

    Backend(Configuration *, Expr *);

 protected:

    Configuration * config;
    Expr * ast;

};

#endif

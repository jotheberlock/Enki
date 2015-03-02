#include <stdio.h>
#include <string.h>
#include "cfuncs.h"
#include "codegen.h"
#include "symbols.h"

int numbers(int a, int b, int c, int d, int e, int f, int g, int h)
{
    printf("Numbers [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d]\n",
           a,b,c,d,e,f,g,h);
    return 0;
}

int test(int a, int b)
{
    printf("Got test %d %d!\n", a, b);
    return a+b;
}

int hello_true()
{
    printf("Hello true!\n");
    return 1;
}

int hello_false()
{
    printf("Hello false!\n");
    return 0;
}
 
int print(char * c, int l)
{
    if (l == 0)
    {
        puts(c);
    }
    else
    {
        char * p = c;
        for(int loopc=0; loopc<l; loopc++)
        {
            putc(*p, stdout);
            p++;
        }
        fflush(stdout);
    }
	return 0;
}

int read(char * c, int l)
{
	fgets(c, l, stdin);
	return strlen(c);
}

int crash()
{
    unsigned char * ptr = 0;
    *ptr = 0;
    return 0;
}

CFuncs::CFuncs()
{
    table = new uint64_t[4096/8];
    count = 0;

    add((uint64_t)test, "test");
    add((uint64_t)hello_true, "saytrue");
    add((uint64_t)hello_false, "sayfalse");
    add((uint64_t)print, "print");
    add((uint64_t)read, "read");
    add((uint64_t)numbers, "numbers");
    add((uint64_t)crash, "crash");
}

CFuncs::~CFuncs()
{
    delete[] table;
}

void CFuncs::add(uint64_t addr, std::string name)
{
    table[count] = addr;
    CFuncRec cfr;
    cfr.name = name;
    cfr.type = new ExternalFunctionType(name, calling_convention);
    addType(cfr.type, name);
    names.push_back(cfr);
    count++;
}

bool CFuncs::find(std::string name, uint64_t & addr)
{
    int count = 0;
    for (std::vector<CFuncRec>::iterator it = names.begin();
         it != names.end(); it++)
    {
        if ((*it).name == name)
        {
            addr = count * assembler->pointerSize() / 8;
            return true;
        }
        count++;
    }

    return false;
}

void CFuncs::addToCodegen(Codegen * c)
{
    for (std::vector<CFuncRec>::iterator it = names.begin();
         it != names.end(); it++)
    {
        new FunctionScope(c->getScope(), (*it).name, (*it).type);
    }
}

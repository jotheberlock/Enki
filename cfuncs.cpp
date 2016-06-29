#include <stdio.h>
#include <string.h>
#include "cfuncs.h"
#include "image.h"
#include "symbols.h"
#include "type.h"

int numbers(int a, int b, int c, int d, int e, int f, int g, int h)
{
	printf("Numbers [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d]\n",
		a, b, c, d, e, f, g, h);
	return 0;
}

int test(int a, int b)
{
	printf("Got test %d %d!\n", a, b);
	return a + b;
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
		for (int loopc = 0; loopc < l; loopc++)
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
	return (int)strlen(c);
}

int crash()
{
	unsigned char * ptr = 0;
	*ptr = 0;
	return 0;
}

void set_cfuncs(MemoryImage * mi)
{
	new FunctionScope(root_scope, "test", new ExternalFunctionType(calling_convention));
	mi->addImport("", "test");
}

void register_cfuncs(MemoryImage * mi)
{
	mi->setImport("test", (uint64)test);
}


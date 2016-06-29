#include <stdio.h>
#include <string.h>

#include "error.h"
#include "lexer.h"

std::list<Error> errors;

Error::Error(Token * t, std::string m, std::string d)
{
	file = t->file;
	message = m;
	detail = d;
	br = t->bline;
	bc = t->bcol;
	er = t->eline;
	ec = t->ecol;
}

void Error::print()
{
	FILE * f = fopen(file.c_str(), "rb");
	if (!f)
	{
		f = fopen(("../" + file).c_str(), "rb");
	}

	if (!f)
	{
		printf("<Can't open [%s]>\n", file.c_str());
	}
	else
	{
		char buf[4096];
		for (int loopc = 0; loopc <= br; fgets(buf, 4096, f), loopc++)
		{
		}

		puts(buf);
		for (int loopc = 0; loopc < bc; loopc++)
		{
			printf(" ");
		}
		uint64 todo;
		if (ec >= bc)
		{
			todo = ec - bc + 1;
		}
		else
		{
			todo = strlen(buf) - bc;
		}

		for (unsigned int loopc = 0; loopc < todo; loopc++)
		{
			printf("^");
		}
		printf("\n");
		fclose(f);
	}
	printf("Line %d column %d - ", br, bc);
	printf("%s%s%s\n", message.c_str(), (detail != "") ? ": " : "",
		detail.c_str());
}

void addError(Error e)
{
	errors.push_back(e);
}

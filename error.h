#ifndef _ERROR_
#define _ERROR_

/*
    The tokeniser and parser can generate these; if errors
    occur they are displayed showing the line(s) and column(s)
    where the error occurred, a message, and extra detail if
    available.
*/

#include <list>
#include <string>

class Token;

class Error
{
  public:
    Error(Token *t, std::string m, std::string d = "");

    Error()
    {
        br = 0;
        bc = 0;
        er = 0;
        ec = 0;
    }

    void print();

  protected:
    std::string file;
    int br, bc, er, ec;
    std::string message;
    std::string detail;
};

extern std::list<Error> errors;
void addError(Error);

#endif

#ifndef _LEXER_
#define _LEXER_

/*
    First point of contact with the code. Takes in a list of Chars
    (actually Unicode UCS-4 codepoints) read all at once from a source file
    (because it is 2017 and source code is simply not that much of a memory
    hog on a desktop computer), outputs a list of Tokens with a type and optional
    associated text.
*/

#include <list>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

#include "platform.h"

typedef std::vector<uint32_t> Chars;

#define DONE 0
#define STRING_LITERAL 1
#define NUMBER 2
#define IDENTIFIER 3
#define OPEN_BRACKET 4
#define CLOSE_BRACKET 5
#define COMMA 6
#define DOT 11
#define EQUALS 14
#define ASSIGN 15
#define DEREF 16
#define EOL 17

#define BINOP 20
#define INTEGER_LITERAL 21
#define FLOAT_LITERAL 22
#define UNARYOP 23

#define CHAR_ERROR 25

#define BEGIN 26
#define END 27

#define OPEN_SQUARE 28
#define CLOSE_SQUARE 29

#define IF 30
#define ELSE 31
#define WHILE 32

#define PASS 33
#define RETURN 34
#define CONTINUE 35
#define BREAK 36
#define YIELD 37
#define POINTER 38

#define STRUCT 39
#define DEF 40
#define ADDRESSOF 41

#define CONST 42

#define BREAKPOINT 43

#define FUNCVAR 44
#define MACRO 45
#define UNION 46

#define EXTERN 47

#define EVAL_FUNCVAR 48
#define FPTR 49

#define ELIF 50

#define GENERIC 51
#define RAW 52

#define IMPORT 53
#define MODULE 54
#define CAST 55
#define CONSTIF 56
#define SIZEOF 57

class Token
{
  public:
    Chars value;
    int type;
    int bline;
    int bcol;
    int eline;
    int ecol;
    std::string file;

    void print();
    std::string toString()
    {
        std::string ret;
        for (auto &val : value)
        {
            char c = val;
            ret += c;
        }
        return ret;
    }
};

class ReadChar
{
  public:
    ReadChar()
    {
        val = 0;
        line = 0;
        col = 0;
    }

    uint32_t val;
    int line;
    int col;
};

class OpRec
{
  public:
    OpRec()
    {
        type = 0;
        left = false;
        pri = 0;
    }

    OpRec(int t, bool l, int p)
    {
        type = t;
        left = l;
        pri = p;
    }

    int type;
    bool left;
    int pri;
};

class Lexer
{
  public:
    Lexer();
    void lex(Chars &input);
    void setFile(std::string f)
    {
        file = f;
        col = 0;
        line = 0;
        oldcol = -1;
        oldline = -1;
        push_list.clear();
    }

    std::vector<Token> &tokens()
    {
        return token_list;
    }

    void push(ReadChar r)
    {
        push_list.push_back(r);
    }

    bool validChar(ReadChar r)
    {
        if (r.val >= 'a' && r.val <= 'z')
            return true;
        if (r.val >= 'A' && r.val <= 'Z')
            return true;
        if (r.val >= '0' && r.val <= '9')
            return true;
        if (r.val == '?' || r.val == '_' || r.val == '-' || r.val == ':')
            return true;
        if (r.val > 127)
            return true;
        return false;
    }

    ReadChar next()
    {
        ReadChar ret;

        if (push_list.size() > 0)
        {
            ret = *push_list.begin();
            push_list.pop_front();
            return ret;
        }

        if (pos >= line_list.size())
        {
            return ret;
        }
        else
        {
            ret.val = line_list[pos];
            ret.line = line;
            ret.col = col;

            if (ret.val == '\r')
            {
                // Ignore
            }
            else if (!isCombining(ret.val))
            {
                col++;
            }
            pos++;
            return ret;
        }
    }

    void addOp(OpRec o, uint32_t first, uint32_t second_t = 0);
    bool isOp(uint32_t first, uint32_t second, bool &two_char, OpRec &rec, std::string);

    void endLexing();

  protected:
    bool getLine(int &indent);
    ReadChar eatWhitespace();
    ReadChar eatLine();
    void readStringLiteral(uint32_t term);
    void readNumber();
    void readIdentifier();

    bool isCombining(uint32_t val)
    {
        if (val > 0x300 && val <= 0x36f)
            return true;
        if (val > 0x1dc0 && val <= 0x1dcf)
            return true;
        if (val > 0x20d0 && val <= 0x20ff)
            return true;
        if (val > 0xfe20 && val <= 0xfe2f)
            return true;
        return false;
    }

    void beginToken(ReadChar ch, int t)
    {
        current_token.type = t;
        current_token.bline = ch.line;
        current_token.bcol = ch.col;
        current_token.file = file;
    }

    void pushToken(ReadChar ch)
    {
        current_token.eline = ch.line;
        current_token.ecol = ch.col;
        token_list.push_back(current_token);
        current_token = Token();
    }

    void simpleToken(ReadChar ch, int t)
    {
        beginToken(ch, t);
        pushToken(ch);
    }

    void endToken(ReadChar ch)
    {
        current_token.eline = ch.line;
        current_token.ecol = ch.col;
    }

    Token current_token;
    ReadChar previous_char;

    Chars chars_list;
    Chars line_list;

    std::list<ReadChar> push_list;

    std::vector<Token> token_list;
    unsigned int pos;
    int col;
    int line;
    int oldcol;
    int oldline;
    unsigned int linepos;

    std::unordered_map<uint64_t, OpRec> ops;
    std::unordered_map<std::string, int> keywords;
    std::stack<int> indentations;
    std::string file;

    bool whitespace_before_token;

};

#endif

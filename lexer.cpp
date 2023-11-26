#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "error.h"
#include "platform.h"

void Token::print()
{
	fprintf(log_file, "Type %d begin %d %d end %d %d - [%s]\n", type, bline, bcol, eline, ecol, toString().c_str());
}

Lexer::Lexer()
{
	pos = 0;
	col = 0;
	line = 0;

	addOp(OpRec(BINOP, true, 2), '+');
	addOp(OpRec(BINOP, true, 2), '-');
	addOp(OpRec(BINOP, true, 3), '*');
	addOp(OpRec(BINOP, true, 3), '/');
	addOp(OpRec(BINOP, true, 3), '%');
	addOp(OpRec(BINOP, true, 4), '=', '=');
	addOp(OpRec(BINOP, true, 4), '!', '=');

	addOp(OpRec(BINOP, true, 5), '<', '<');
	addOp(OpRec(BINOP, true, 5), '>', '>');
    addOp(OpRec(BINOP, true, 5), '>', '-');   // Arithmetic shift right
	addOp(OpRec(BINOP, true, 4), '<');
	addOp(OpRec(BINOP, true, 4), '>');
	addOp(OpRec(BINOP, true, 4), '<', '=');
	addOp(OpRec(BINOP, true, 4), '>', '=');

	addOp(OpRec(BINOP, true, 4), '&');
	addOp(OpRec(BINOP, true, 4), '|');
	//addOp(OpRec(BINOP, true, 4), '^');

	addOp(OpRec(UNARYOP, true, 4), '!');
	addOp(OpRec(UNARYOP, true, 4), '@');
	// addOp(OpRec(UNARYOP, true, 4), '%');
	addOp(OpRec(UNARYOP, true, 4), '~');

	// Lowest priority, because it needs to run /last/
    addOp(OpRec(BINOP, true, 1), '+', '=');
    addOp(OpRec(BINOP, true, 1), '-', '=');
    addOp(OpRec(BINOP, true, 1), '*', '=');
    addOp(OpRec(BINOP, true, 1), '/', '=');
    addOp(OpRec(BINOP, true, 1), '|', '=');
    addOp(OpRec(BINOP, true, 1), '&', '=');
    addOp(OpRec(BINOP, true, 1), '^', '=');
	addOp(OpRec(BINOP, true, 1), '=');
	addOp(OpRec(BINOP, true, 1), ':', '=');

	keywords["if"] = IF;
	keywords["else"] = ELSE;
	keywords["while"] = WHILE;
	keywords["pass"] = PASS;
	keywords["return"] = RETURN;
	keywords["break"] = BREAK;
	keywords["continue"] = CONTINUE;
	keywords["yield"] = YIELD;
	keywords["struct"] = STRUCT;
	keywords["union"] = UNION;
	keywords["def"] = DEF;
	keywords["macro"] = MACRO;
	keywords["const"] = CONST;
	keywords["__break"] = BREAKPOINT;
	keywords["or"] = BINOP;
	keywords["and"] = BINOP;
	keywords["not"] = BINOP;
	keywords["xor"] = BINOP;
	keywords["extern"] = EXTERN;
	keywords["fptr"] = FPTR;
	keywords["elif"] = ELIF;
	keywords["generic"] = GENERIC;
	keywords["raw"] = RAW;
	keywords["import"] = IMPORT;
	keywords["module"] = MODULE;
	keywords["cast"] = CAST;
	keywords["constif"] = CONSTIF;
	keywords["sizeof"] = SIZEOF;

	oldcol = -1;
	oldline = -1;
	indentations.push(0);

	ReadChar b;
	simpleToken(b, BEGIN);  // Implicit block
}

void Lexer::addOp(OpRec op, uint32_t first, uint32_t second)
{
	uint64_t s64 = second;
	uint64_t encoded = ((s64 << 32) | first);
	ops[encoded] = op;
}

bool Lexer::isOp(uint32_t first, uint32_t second, bool & two_char,
	OpRec & op, std::string full_value)
{
	if (full_value == "or")
	{
		op = OpRec(BINOP, true, 4);
		return true;
	}
	else if (full_value == "and")
	{
		op = OpRec(BINOP, true, 4);
		return true;
	}
	else if (full_value == "xor")
	{
		op = OpRec(BINOP, true, 4);
		return true;
	}
	else if (full_value == "not")
	{
		op = OpRec(UNARYOP, true, 4);
		return true;
	}
	else if (full_value == "<<")
	{
		op = OpRec(BINOP, true, 5);
		return true;
	}
	else if (full_value == ">>")
	{
		op = OpRec(BINOP, true, 5);
		return true;
	}

	uint64_t s64 = second;
	uint64_t encoded = (s64 << 32) | first;
	std::map<uint64_t, OpRec>::iterator it1 = ops.find(encoded);
	if (it1 != ops.end())
	{
		two_char = true;
		op = (*it1).second;
		return true;
	}
	std::map<uint64_t, OpRec>::iterator it2 = ops.find(first);
	if (it2 != ops.end())
	{
		two_char = false;
		op = (*it2).second;
		return true;
	}

	return false;
}

ReadChar Lexer::eatWhitespace()
{
	ReadChar ch;
	do
	{
		ch = next();
		if (ch.val != ' ' && ch.val != '\t')
			break;
	} while (true);
	return ch;
}

ReadChar Lexer::eatLine()
{
	ReadChar ch = next();
	uint32_t val = ch.val;
	while (val != '\n' && val != 0)
	{
		ch = next();
		val = ch.val;
	}
	ch = next();
	return ch;
}

void Lexer::readIdentifier()
{
	while (true)
	{
		ReadChar ch = next();
		if (validChar(ch))
		{
			current_token.value.push_back(ch.val);
		}
		else
		{
			std::map<std::string, int>::iterator it =
				keywords.find(current_token.toString());
			if (it != keywords.end())
			{
				current_token.type = (*it).second;
			}

			ReadChar dummy = ch;
			dummy.col = (dummy.col > 0) ? dummy.col - 1 : 0;
			pushToken(dummy);
			push(ch);
			return;
		}
	}
}

int hexdig(uint32_t val)
{
	if (val >= '0' && val <= '9')
	{
		return val - '0';
	}

	if (val >= 'a' && val <= 'f')
	{
		return (val - 'a') + 10;
	}

	if (val >= 'A' && val <= 'F')
	{
		return (val - 'A') + 10;
	}

	return -1;
}

void Lexer::readStringLiteral(uint32_t term)
{
	bool not_raw = true;

	while (true)
	{
		ReadChar ch = next();
		uint32_t val = ch.val;
		if (val == term || val == 0)
		{
			pushToken(ch);
			return;
		}
		else if (val == '\\' && not_raw)
		{
			ReadChar nch = next();
			uint32_t val2 = nch.val;
			if (val2 == 'r')
			{
				not_raw = false;
			}
			else if (val2 == '\\')
			{
				current_token.value.push_back('\\');
			}
			else if (val2 == '\'')
			{
				current_token.value.push_back('\'');
			}
			else if (val2 == '"')
			{
				current_token.value.push_back('"');
			}
			else if (val2 == 'n')
			{
				current_token.value.push_back('\n');
			}
			else if (val2 == 'r')
			{
				current_token.value.push_back('\r');
			}
			else if (val2 == 'e')
			{
				current_token.value.push_back(27);  // Escape
			}
			else if (val2 == 't')
			{
				current_token.value.push_back(9);   // Tab
			}
			else if (val2 == 'a')
			{
				current_token.value.push_back(7);
			}
			else if (val2 == 'b')
			{
				current_token.value.push_back(8);  // Backspace
			}
			else if (val2 == 't')
			{
				current_token.value.push_back(9);
			}
			else if (val2 == '0')
			{
				current_token.value.push_back(0);
			}
			else if (val2 == 'f')
			{
				current_token.value.push_back(12);
			}
			else if (val2 == 'F' || val2 == 'B')
			{
				std::string out[3];
				for (int loopc = 0; loopc < 3; loopc++)
				{
					char buf[3];
					buf[0] = next().val;
					buf[1] = next().val;
					buf[2] = 0;
					long int ret = strtol(buf, 0, 16);
					char convbuf[4096];
					sprintf(convbuf, "%03ld", ret);
					out[loopc] = convbuf;
				}

				current_token.value.push_back(27);
				current_token.value.push_back('[');
				current_token.value.push_back((val2 == 'F') ? '3' : '4');
				current_token.value.push_back('8');
				current_token.value.push_back(';');
				current_token.value.push_back('2');
				current_token.value.push_back(';');
				current_token.value.push_back(out[0][0]);
				current_token.value.push_back(out[0][1]);
				current_token.value.push_back(out[0][2]);
				current_token.value.push_back(';');
				current_token.value.push_back(out[1][0]);
				current_token.value.push_back(out[1][1]);
				current_token.value.push_back(out[1][2]);
				current_token.value.push_back(';');
				current_token.value.push_back(out[2][0]);
				current_token.value.push_back(out[2][1]);
				current_token.value.push_back(out[2][2]);
				current_token.value.push_back('m');
			}
			else if (val2 == 'R')
			{
				current_token.value.push_back(27);
				current_token.value.push_back('[');
				current_token.value.push_back('3');
				current_token.value.push_back('9');
				current_token.value.push_back(';');
				current_token.value.push_back('4');
				current_token.value.push_back('9');
				current_token.value.push_back('m');
			}
			else if (val2 == 'x')
			{
				ReadChar ch1 = next();
				ReadChar ch2 = next();
				int d1 = hexdig(ch1.val);
				int d2 = hexdig(ch2.val);
				if (d1 == -1 || d2 == -1)
				{
					endToken(ch2);
					char buf[4096];
					sprintf(buf, "%c %c", (char)ch1.val, (char)ch2.val);
					addError(Error(&current_token, "Not a hex digit", buf));
				}
				else
				{
					unsigned char topush;
					topush = ((d1 & 0xf) << 4) | (d2 & 0xf);
					current_token.value.push_back(topush);
				}
			}
			else if (val2 == 'b')
			{
				unsigned char foo = 0;
				for (int loopc = 0; loopc < 8; loopc++)
				{
					ReadChar ch = next();
					foo = foo << 1;
					if (ch.val == '1')
					{
						foo |= 1;
					}
					else if (ch.val == '0')
					{
					}
					else
					{
						char buf[4096];
						sprintf(buf, "%c", (char)ch.val);
						addError(Error(&current_token,
							"Not a binary digit", buf));
					}
				}
				current_token.value.push_back(foo);
			}
			else
			{
				endToken(nch);
				char buf[4096];
				sprintf(buf, "%c (%x)", (char)val2, val2);
				addError(Error(&current_token,
					"Unknown escape sequence",
					buf));
			}
		}
		else if (val == '\n')
		{
			endToken(ch);
			addError(Error(&current_token,
				"Unterminated string literal"));
			return;
		}
		else
		{
			current_token.value.push_back(val);
		}
	}
}

void Lexer::readNumber()
{
	bool first = true;
	bool a_dot = false;
	bool hex_prefixed = false;

	while (true)
	{
		ReadChar ch = next();
		if (ch.val == 0)
		{
			pushToken(ch);
			return;
		}
		else if (ch.val == '-')
		{
			if (first)
			{
				current_token.value.push_back(ch.val);
			}
			else
			{
				addError(Error(&current_token, "Unexpected - in number"));
			}
		}
		else if (ch.val >= '0' && ch.val <= '9')
		{
			current_token.value.push_back(ch.val);
		}
		else if ((ch.val >= 'a' && ch.val <= 'f') ||
			(ch.val >= 'A' && ch.val <= 'F'))
		{
			if (!hex_prefixed)
			{
				current_token.value.insert(current_token.value.begin(), 'x');
				current_token.value.insert(current_token.value.begin(), '0');
				hex_prefixed = true;
			}
			current_token.value.push_back(ch.val);
		}
		else if (ch.val == '.')
		{
			if (a_dot)
			{
				endToken(ch);
				addError(Error(&current_token,
					"Float has more than one dot"));
				return;
			}
			else
			{
				current_token.type = FLOAT_LITERAL;
				a_dot = true;
				current_token.value.push_back(ch.val);
			}
		}
		else if (ch.val == 'x')
		{
			if (!first || current_token.value[0] != '0')
			{
				endToken(ch);
				addError(Error(&current_token, "Unexpected x in float"));
				return;
			}
			else
			{
				current_token.value.push_back(ch.val);
				hex_prefixed = true;
			}
		}
		else
		{
			ReadChar dummy = ch;
			dummy.col = (dummy.col > 0) ? dummy.col - 1 : 0;
			pushToken(dummy);

			push(ch);
			return;
		}
		first = false;
	}
}

bool Lexer::getLine(int & indent)
{
    line_list.clear();
    indent = 0;
    bool seen_non_whitespace = false;
    bool in_string = false;

    while (linepos < chars_list.size())
    {
        uint32_t val = chars_list[linepos];
        linepos++;

        if (!seen_non_whitespace)
        {
            if (val == ' ')
            {
                indent++;
            }
            else if (val == '\t')
            {
                indent += 4;
            }
        }

        if (val == '"' || val == '\'')
        {
            in_string = !in_string;
        }

        if (val == '#' || (val == '\\' && in_string == false))
        {
            bool cont = (val == '\\');
            while (val != '\n')
            {
                linepos++;

                if (linepos == chars_list.size())
                {
                    line_list.push_back('\n');
                    return true;
                }
                val = chars_list[linepos];
            }
            linepos++;

            if (cont)
            {
                while (true)
                {
                    if (chars_list[linepos] != ' ' && chars_list[linepos] != '\t' && chars_list[linepos] != '\n')
                    {
                        val = chars_list[linepos];
                        break;
                    }
                    linepos++;
                    if (linepos == chars_list.size())
                    {
                        line_list.push_back('\n');
                        return true;
                    }
                }
                continue;
            }
        }

        if (val == '\n')
        {
            if (seen_non_whitespace)
            {
                line_list.push_back('\n');
                return true;
            }
            else
            {
                line_list.clear();
                indent = 0;
                line++;
            }
        }

        if (val != ' ' && val != '\t' && val != '\n')
        {
            seen_non_whitespace = true;
        }

        if (seen_non_whitespace)
        {
            line_list.push_back(val);
        }
    }

    line_list.push_back('\n');
    return false;
}

void Lexer::lex(Chars & input)
{
	chars_list = input;
	pos = 0;
    linepos = 0;

    int indent;

    bool more = true;
    while (more)
    {
        more = getLine(indent);

        line++;
        col = indent;
        pos = 0;

        while (true)
        {
            ReadChar begin = eatWhitespace();
            uint32_t val = begin.val;
            if (val == 0)
            {
                    /*
                      for (unsigned int loopc=0; loopc<indentations.size(); loopc++)
                      {
                      simpleToken(begin, END);
                      }
                    */
                break;
            }

            if (val == '\r')
            {
                    // ignore
                continue;
            }

            int oll = oldline;
            oldline = begin.line;

            if (begin.line > oll)
            {
                int occ = oldcol;
                oldcol = begin.col;

                if (begin.col > indentations.top())
                {
                        // New level of indentation
                    indentations.push(begin.col);
                    simpleToken(begin, BEGIN);
                    push(begin);
                    continue;
                }
                else if (begin.col < occ)
                {
                    while (indentations.size() > 1)
                    {
                        indentations.pop();

                        if (indentations.top() == begin.col)
                        {
                            simpleToken(begin, END);
                                //push(begin);
                            break;
                        }
                        else if (begin.col > indentations.top())
                        {
                            Token dummy;
                            dummy.file = file;
                            dummy.type = END;
                            dummy.bline = begin.line;
                            dummy.bcol = begin.col;
                            dummy.eline = begin.line;
                            dummy.ecol = begin.col;

                            char buf[4096];
                            sprintf(buf, "%d %d", indentations.top(), begin.col);
                            addError(Error(&dummy, "Mismatched indent levels",
                                           buf));
                            push(begin);
                            break;
                        }
                        else
                        {
                            simpleToken(begin, END);
                        }
                    }
                }
            }

            ReadChar second = next();
            bool twochar = false;
            OpRec orc;
            if (begin.val == '-' && second.val == '>')
            {
                beginToken(begin, DEREF);
                pushToken(second);
                continue;
            }
            else if (begin.val == '-' && (second.val >= '0' && second.val <= '9'))
            {
                beginToken(begin, INTEGER_LITERAL);
                current_token.value.push_back(begin.val);
                push(second);
                readNumber();
                continue;
            }
            else if (isOp(begin.val, second.val, twochar, orc, ""))
            {
                beginToken(begin, orc.type);
                current_token.value.push_back(begin.val);
                if (twochar)
                {
                    current_token.value.push_back(second.val);
                    pushToken(second);
                }
                else
                {
                    push(second);
                    pushToken(begin);
                }

                continue;
            }
            else
            {
                push(second);
            }

            if (val == '\n')
            {
                simpleToken(begin, EOL);
                continue;
            }
            else if (val == '\'' || val == '"')
            {
                beginToken(begin, STRING_LITERAL);
                readStringLiteral(begin.val);
            }
            else if (val >= '0' && val <= '9')
            {
                beginToken(begin, INTEGER_LITERAL);
                current_token.value.push_back(begin.val);
                readNumber();
            }
                // After 0-9 so numbers get caught first
            else if (validChar(begin))
            {
                beginToken(begin, IDENTIFIER);

                current_token.value.push_back(begin.val);
                readIdentifier();
            }
            else if (val == '(')
            {
                simpleToken(begin, OPEN_BRACKET);
            }
            else if (val == ')')
            {
                simpleToken(begin, CLOSE_BRACKET);
            }
            else if (val == '[')
            {
                simpleToken(begin, OPEN_SQUARE);
            }
            else if (val == ']')
            {
                simpleToken(begin, CLOSE_SQUARE);
            }
            else if (val == ',')
            {
                simpleToken(begin, COMMA);
            }
            else if (val == '.')
            {
                simpleToken(begin, DOT);
            }
            else if (val == '=')
            {
                simpleToken(begin, ASSIGN);
            }
            else if (val == '^')
            {
                simpleToken(begin, POINTER);
            }
            else if (val == '&')
            {
                simpleToken(begin, ADDRESSOF);
            }
            else if (val == '$')
            {
                simpleToken(begin, FUNCVAR);
            }
            else
            {
                Token dummy;
                dummy.file = file;
                dummy.type = CHAR_ERROR;
                dummy.bline = begin.line;
                dummy.bcol = begin.col;
                dummy.eline = begin.line;
                dummy.ecol = begin.col;

                char buf[4096];
                sprintf(buf, "%c (%x)", (char)val, val);
                addError(Error(&dummy, "Unknown character",
                               buf));
            }
        }
    }
}

void Lexer::endLexing()
{
	for (unsigned int loopc = 0; loopc > indentations.size(); loopc++)
	{
		ReadChar b;
		simpleToken(b, END);
	}
}

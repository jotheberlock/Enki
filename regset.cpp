#include "regset.h"
#include <stdio.h>

#define REGELEMENTS (MAXREG / 64)

RegSet::RegSet()
{
    setEmpty();
}

void RegSet::setAll()
{
    for (int loopc = 0; loopc < REGELEMENTS; loopc++)
    {
        regs[loopc] = 0xffffffffffffffffLL;
    }
}

void RegSet::setEmpty()
{
    for (int loopc = 0; loopc < REGELEMENTS; loopc++)
    {
        regs[loopc] = 0;
    }
}

bool RegSet::isEmpty()
{
    for (int loopc = 0; loopc < REGELEMENTS; loopc++)
    {
        if (regs[loopc])
        {
            return false;
        }
    }

    return true;
}

RegSet RegSet::operator|(const RegSet &r)
{
    RegSet ret;
    for (int loopc = 0; loopc < REGELEMENTS; loopc++)
    {
        ret.regs[loopc] = regs[loopc] | r.regs[loopc];
    }
    return ret;
}

RegSet RegSet::operator!()
{
    RegSet ret;
    for (int loopc = 0; loopc < REGELEMENTS; loopc++)
    {
        ret.regs[loopc] = !regs[loopc];
    }
    return ret;
}

RegSet RegSet::operator&(const RegSet &r)
{
    RegSet ret;
    for (int loopc = 0; loopc < REGELEMENTS; loopc++)
    {
        ret.regs[loopc] = regs[loopc] & r.regs[loopc];
    }
    return ret;
}

RegSet RegSet::operator^(const RegSet &r)
{
    RegSet ret;
    for (int loopc = 0; loopc < REGELEMENTS; loopc++)
    {
        ret.regs[loopc] = regs[loopc] ^ r.regs[loopc];
    }
    return ret;
}

bool RegSet::operator==(const RegSet &r)
{
    for (int loopc = 0; loopc < REGELEMENTS; loopc++)
    {
        if (regs[loopc] != r.regs[loopc])
        {
            return false;
        }
    }

    return true;
}

std::string RegSet::toString()
{
    std::string ret;
    for (int loopc = 0; loopc < MAXREG; loopc++)
    {
        if (isSet(loopc))
        {
            if (loopc > 0)
            {
                ret += ",";
            }

            char buf[4096];
            sprintf(buf, "%d", loopc);
            ret += buf;
        }
    }
    return ret;
}

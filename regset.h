#ifndef _REGSET_
#define _REGSET_

// A bitset used for holding registers, mostly used
// by the register allocator and by Assemblers to indicate
// e.g. which registers are valid for a given operand.

#include <assert.h>
#include <string>

#include "platform.h"

#define MAXREG 256

class RegSet
{
public:

	RegSet();

	void setAll();
	void setEmpty();
	bool isEmpty();

	int count()
	{
		int ret = 0;
		for (int loopc = 0; loopc < MAXREG; loopc++)
		{
			if (isSet(loopc))
			{
				ret++;
			}
		}

		return ret;
	}

	void set(int i)
	{
		assert(i >= 0 && i < MAXREG);
		regs[i / 64] |= (uint64_t)0x1 << (i % 64);
	}

	void clear(int i)
	{
		assert(i >= 0 && i < MAXREG);
		regs[i / 64] = (uint64_t)regs[i / 64] & ~(0x1 << (i % 64));
	}

	bool isSet(int i)
	{
		assert(i >= 0 && i < MAXREG);
		return (regs[i / 64] & (uint64_t)0x1 << (i % 64)) ? true : false;
	}

	bool operator[](int i)
	{
		return isSet(i);
	}

	RegSet operator|(const RegSet &);
	RegSet operator!();
	RegSet operator&(const RegSet &);
	RegSet operator^(const RegSet &);
	bool operator==(const RegSet &);
	bool operator!=(const RegSet & r)
	{
		return !(*this == r);
	}

	std::string toString();

protected:

	uint64_t regs[MAXREG / 64];

};

#endif

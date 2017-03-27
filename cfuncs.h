#ifndef _CFUNCS_
#define _CFUNCS_

/*
	A relic from when this compiler JIT-compiled in memory;
	left in because I may eventually want it to implement a
	macro system.
*/

class MemoryImage;

void set_cfuncs(MemoryImage *);
void register_cfuncs(MemoryImage *);

#endif

#include "platform.h"

#include <stdio.h>
#include <assert.h>

#ifdef HOST_BIG_ENDIAN
void wle16(unsigned char *& ptr, uint16 v)
{
    unsigned char * p = (unsigned char *)&v;
    ptr[0] = p[1];
    ptr[1] = p[0];
    ptr += 2;
}

void wle32(unsigned char *& ptr, uint32 v)
{
    unsigned char * p = (unsigned char *)&v;
    ptr[0] = p[3];
    ptr[1] = p[2];
    ptr[2] = p[1];
    ptr[3] = p[0];
    ptr += 4;
}

void wle64(unsigned char *& ptr, uint64 v)
{
    unsigned char * p = (unsigned char *)&v;
    ptr[0] = p[7];
    ptr[1] = p[6];
    ptr[2] = p[5];
    ptr[3] = p[4];
    ptr[4] = p[3];
    ptr[5] = p[2];
    ptr[6] = p[1];
    ptr[7] = p[0];
    ptr += 8;
}


void wles16(unsigned char *& ptr, int16 v)
{
    unsigned char * p = (unsigned char *)&v;
    ptr[0] = p[1];
    ptr[1] = p[0];
    ptr += 2;
}

void wles32(unsigned char *& ptr, int32 v)
{
    unsigned char * p = (unsigned char *)&v;
    ptr[0] = p[3];
    ptr[1] = p[2];
    ptr[2] = p[1];
    ptr[3] = p[0];
    ptr += 4;
}

void wles64(unsigned char *& ptr, int64 v)
{
    unsigned char * p = (unsigned char *)&v;
    ptr[0] = p[7];
    ptr[1] = p[6];
    ptr[2] = p[5];
    ptr[3] = p[4];
    ptr[4] = p[3];
    ptr[5] = p[2];
    ptr[6] = p[1];
    ptr[7] = p[0];
    ptr += 8;
}

void wbe16(unsigned char *& ptr, uint16 v)
{
    *((uint16 *)ptr) = v;
    ptr += 2;
}

void wbe32(unsigned char *& ptr, uint32 v)
{
    *((uint32 *)ptr) = v;
    ptr += 4;
}

void wbe64(unsigned char *& ptr, uint64 v)
{
    *((uint64 *)ptr) = v;
    ptr += 8;
}

void wbes16(unsigned char *& ptr, int16 v)
{
    uint16 us;
    *((int16 *)(&us)) = v;
    wle16(ptr, us);
}

void wbes32(unsigned char *& ptr, int32 v)
{
    uint32 us;
    *((int32 *)(&us)) = v;
    wle32(ptr, us);
}

void wbes64(unsigned char *& ptr, int64 v)
{
    uint64 us;
    *((int64 *)(&us)) = v;
    wle64(ptr, us);
}
#else
void wle16(unsigned char *& ptr, uint16 v)
{
    *((uint16 *)ptr) = v;
    ptr += 2;
}

void wle32(unsigned char *& ptr, uint32 v)
{
    *((uint32 *)ptr) = v;
    ptr += 4;
}

void wle64(unsigned char *& ptr, uint64 v)
{
    *((uint64 *)ptr) = v;
    ptr += 8;
}

void wles16(unsigned char *& ptr, int16 v)
{
    uint16 us;
    *((int16 *)(&us)) = v;
    wle16(ptr, us);
}

void wles32(unsigned char *& ptr, int32 v)
{
    uint32 us;
    *((int32 *)(&us)) = v;
    wle32(ptr, us);
}

void wles64(unsigned char *& ptr, int64 v)
{
    uint64 us;
    *((int64 *)(&us)) = v;
    wle64(ptr, us);
}

void wbe16(unsigned char *& ptr, uint16 v)
{
    unsigned char * p = (unsigned char *)&v;
    ptr[0] = p[1];
    ptr[1] = p[0];
    ptr += 2;
}

void wbe32(unsigned char *& ptr, uint32 v)
{
    unsigned char * p = (unsigned char *)&v;
    ptr[0] = p[3];
    ptr[1] = p[2];
    ptr[2] = p[1];
    ptr[3] = p[0];
    ptr += 4;
}

void wbe64(unsigned char *& ptr, uint64 v)
{
    unsigned char * p = (unsigned char *)&v;
    ptr[0] = p[7];
    ptr[1] = p[6];
    ptr[2] = p[5];
    ptr[3] = p[4];
    ptr[4] = p[3];
    ptr[5] = p[2];
    ptr[6] = p[1];
    ptr[7] = p[0];
    ptr += 8;
}


void wbes16(unsigned char *& ptr, int16 v)
{
    unsigned char * p = (unsigned char *)&v;
    ptr[0] = p[1];
    ptr[1] = p[0];
    ptr += 2;
}

void wbes32(unsigned char *& ptr, int32 v)
{
    unsigned char * p = (unsigned char *)&v;
    ptr[0] = p[3];
    ptr[1] = p[2];
    ptr[2] = p[1];
    ptr[3] = p[0];
    ptr += 4;
}

void wbes64(unsigned char *& ptr, int64 v)
{
    unsigned char * p = (unsigned char *)&v;
    ptr[0] = p[7];
    ptr[1] = p[6];
    ptr[2] = p[5];
    ptr[3] = p[4];
    ptr[4] = p[3];
    ptr[5] = p[2];
    ptr[6] = p[1];
    ptr[7] = p[0];
    ptr += 8;
}

#endif

void wee16(bool b, unsigned char *& c, uint16 v)
{
    b ? wle16(c,v) : wbe16(c,v);
}

void wee32(bool b, unsigned char *& c, uint32 v)
{
    b ? wle32(c,v) : wbe32(c,v);
}

void wee64(bool b, unsigned char *& c, uint64 v)
{
    b ? wle64(c,v) : wbe64(c,v);
}

void wees16(bool b, unsigned char *& c, int16 v)
{
    b ? wles16(c,v) : wbes16(c,v);
}

void wees32(bool b, unsigned char *& c, int32 v)
{
    b ? wles32(c,v) : wbes32(c,v);
}

void wees64(bool b, unsigned char *& c, int64 v)
{
    b ? wles64(c,v) : wbes64(c,v);
}

uint32 checked_32(uint64 v)
{
	assert(v < 0x100000000);
	return v & 0xffffffff;
}

uint64 roundup(uint64 in, uint64 align)
{
	if (in % align == 0)
	{
		return in;
	}
	return in + (align - (in % align));
}

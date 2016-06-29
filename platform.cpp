#include "platform.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

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
	memcpy(ptr, &v, 2);
	ptr += 2;
}

void wbe32(unsigned char *& ptr, uint32 v)
{
	memcpy(ptr, &v, 4);
	ptr += 4;
}

void wbe64(unsigned char *& ptr, uint64 v)
{
	memcpy(ptr, &v, 8);
	ptr += 8;
}

void wbes16(unsigned char *& ptr, int16 v)
{
	memcpy(ptr, &v, 2);
	ptr += 2;
}

void wbes32(unsigned char *& ptr, int32 v)
{
	memcpy(ptr, &v, 4);
	ptr += 4;
}

void wbes64(unsigned char *& ptr, int64 v)
{
	memcpy(ptr, &v, 8);
}
#else
void wle16(unsigned char *& ptr, uint16 v)
{
	memcpy(ptr, &v, 2);
	ptr += 2;
}

void wle32(unsigned char *& ptr, uint32 v)
{
	memcpy(ptr, &v, 4);
	ptr += 4;
}

void wle64(unsigned char *& ptr, uint64 v)
{
	memcpy(ptr, &v, 8);
	ptr += 8;
}

void wles16(unsigned char *& ptr, int16 v)
{
	memcpy(ptr, &v, 2);
	ptr += 2;
}

void wles32(unsigned char *& ptr, int32 v)
{
	memcpy(ptr, &v, 4);
	ptr += 4;
}

void wles64(unsigned char *& ptr, int64 v)
{
	memcpy(ptr, &v, 8);
	ptr += 8;
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

uint16 rle16(unsigned char * ptr)
{
	uint16 ret;
	ret = ptr[0] | (ptr[1] << 8);
	return ret;
}

uint32 rle32(unsigned char * ptr)
{
	uint32 ret;
	ret = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
	return ret;
}

uint64 rle64(unsigned char * ptr)
{
	uint64 ret;
	ret = ptr[0] | ((uint64)ptr[1] << 8) | ((uint64)ptr[2] << 16) | ((uint64)ptr[3] << 24) |
		((uint64)ptr[4] << 32) | ((uint64)ptr[5] << 40) | ((uint64)ptr[6] << 48) | ((uint64)ptr[7] << 56);
	return ret;
}

uint16 rbe16(unsigned char * ptr)
{
	uint16 ret;
	ret = ptr[1] | (ptr[0] << 8);
	return ret;
}

uint32 rbe32(unsigned char * ptr)
{
	uint32 ret;
	ret = ptr[3] | (ptr[2] << 8) | (ptr[1] << 16) | (ptr[0] << 24);
	return ret;
}

uint64 rbe64(unsigned char * ptr)
{
	uint64 ret;
	ret = ptr[7] | ((uint64)ptr[6] << 8) | ((uint64)ptr[5] << 16) | ((uint64)ptr[4] << 24) |
		((uint64)ptr[3] << 32) | ((uint64)ptr[2] << 40) | ((uint64)ptr[1] << 48) | ((uint64)ptr[0] << 56);
	return ret;
}

void wee16(bool b, unsigned char *& c, uint16 v)
{
	b ? wle16(c, v) : wbe16(c, v);
}

void wee32(bool b, unsigned char *& c, uint32 v)
{
	b ? wle32(c, v) : wbe32(c, v);
}

void wee64(bool b, unsigned char *& c, uint64 v)
{
	b ? wle64(c, v) : wbe64(c, v);
}

void wees16(bool b, unsigned char *& c, int16 v)
{
	b ? wles16(c, v) : wbes16(c, v);
}

void wees32(bool b, unsigned char *& c, int32 v)
{
	b ? wles32(c, v) : wbes32(c, v);
}

void wees64(bool b, unsigned char *& c, int64 v)
{
	b ? wles64(c, v) : wbes64(c, v);
}

uint16 ree16(bool b, unsigned char * c)
{
	return b ? rle16(c) : rbe16(c);
}

uint32 ree32(bool b, unsigned char * c)
{
	return b ? rle32(c) : rbe32(c);
}

uint64 ree64(bool b, unsigned char * c)
{
	return b ? rle64(c) : rbe64(c);
}

uint32 checked_32(uint64 v)
{
	assert(v < 0x100000000LL);
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

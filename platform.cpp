#include "platform.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef HOST_BIG_ENDIAN
void wle16(unsigned char *&ptr, uint16_t v)
{
    unsigned char *p = (unsigned char *)&v;
    ptr[0] = p[1];
    ptr[1] = p[0];
    ptr += 2;
}

void wle32(unsigned char *&ptr, uint32_t v)
{
    unsigned char *p = (unsigned char *)&v;
    ptr[0] = p[3];
    ptr[1] = p[2];
    ptr[2] = p[1];
    ptr[3] = p[0];
    ptr += 4;
}

void wle64(unsigned char *&ptr, uint64_t v)
{
    unsigned char *p = (unsigned char *)&v;
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

void wles16(unsigned char *&ptr, int16_t v)
{
    unsigned char *p = (unsigned char *)&v;
    ptr[0] = p[1];
    ptr[1] = p[0];
    ptr += 2;
}

void wles32(unsigned char *&ptr, int32_t v)
{
    unsigned char *p = (unsigned char *)&v;
    ptr[0] = p[3];
    ptr[1] = p[2];
    ptr[2] = p[1];
    ptr[3] = p[0];
    ptr += 4;
}

void wles64(unsigned char *&ptr, int64_t v)
{
    unsigned char *p = (unsigned char *)&v;
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

void wbe16(unsigned char *&ptr, uint16_t v)
{
    memcpy(ptr, &v, 2);
    ptr += 2;
}

void wbe32(unsigned char *&ptr, uint32_t v)
{
    memcpy(ptr, &v, 4);
    ptr += 4;
}

void wbe64(unsigned char *&ptr, uint64_t v)
{
    memcpy(ptr, &v, 8);
    ptr += 8;
}

void wbes16(unsigned char *&ptr, int16_t v)
{
    memcpy(ptr, &v, 2);
    ptr += 2;
}

void wbes32(unsigned char *&ptr, int32_t v)
{
    memcpy(ptr, &v, 4);
    ptr += 4;
}

void wbes64(unsigned char *&ptr, int64_t v)
{
    memcpy(ptr, &v, 8);
}
#else
void wle16(unsigned char *&ptr, uint16_t v)
{
    memcpy(ptr, &v, 2);
    ptr += 2;
}

void wle32(unsigned char *&ptr, uint32_t v)
{
    memcpy(ptr, &v, 4);
    ptr += 4;
}

void wle64(unsigned char *&ptr, uint64_t v)
{
    memcpy(ptr, &v, 8);
    ptr += 8;
}

void wles16(unsigned char *&ptr, int16_t v)
{
    memcpy(ptr, &v, 2);
    ptr += 2;
}

void wles32(unsigned char *&ptr, int32_t v)
{
    memcpy(ptr, &v, 4);
    ptr += 4;
}

void wles64(unsigned char *&ptr, int64_t v)
{
    memcpy(ptr, &v, 8);
    ptr += 8;
}

void wbe16(unsigned char *&ptr, uint16_t v)
{
    unsigned char *p = (unsigned char *)&v;
    ptr[0] = p[1];
    ptr[1] = p[0];
    ptr += 2;
}

void wbe32(unsigned char *&ptr, uint32_t v)
{
    unsigned char *p = (unsigned char *)&v;
    ptr[0] = p[3];
    ptr[1] = p[2];
    ptr[2] = p[1];
    ptr[3] = p[0];
    ptr += 4;
}

void wbe64(unsigned char *&ptr, uint64_t v)
{
    unsigned char *p = (unsigned char *)&v;
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

void wbes16(unsigned char *&ptr, int16_t v)
{
    unsigned char *p = (unsigned char *)&v;
    ptr[0] = p[1];
    ptr[1] = p[0];
    ptr += 2;
}

void wbes32(unsigned char *&ptr, int32_t v)
{
    unsigned char *p = (unsigned char *)&v;
    ptr[0] = p[3];
    ptr[1] = p[2];
    ptr[2] = p[1];
    ptr[3] = p[0];
    ptr += 4;
}

void wbes64(unsigned char *&ptr, int64_t v)
{
    unsigned char *p = (unsigned char *)&v;
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

uint16_t rle16(unsigned char *ptr)
{
    uint16_t ret;
    ret = ptr[0] | (ptr[1] << 8);
    return ret;
}

uint32_t rle32(unsigned char *ptr)
{
    uint32_t ret;
    ret = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
    return ret;
}

uint64_t rle64(unsigned char *ptr)
{
    uint64_t ret;
    ret = ptr[0] | ((uint64_t)ptr[1] << 8) | ((uint64_t)ptr[2] << 16) | ((uint64_t)ptr[3] << 24) |
          ((uint64_t)ptr[4] << 32) | ((uint64_t)ptr[5] << 40) | ((uint64_t)ptr[6] << 48) | ((uint64_t)ptr[7] << 56);
    return ret;
}

uint16_t rbe16(unsigned char *ptr)
{
    uint16_t ret;
    ret = ptr[1] | (ptr[0] << 8);
    return ret;
}

uint32_t rbe32(unsigned char *ptr)
{
    uint32_t ret;
    ret = ptr[3] | (ptr[2] << 8) | (ptr[1] << 16) | (ptr[0] << 24);
    return ret;
}

uint64_t rbe64(unsigned char *ptr)
{
    uint64_t ret;
    ret = ptr[7] | ((uint64_t)ptr[6] << 8) | ((uint64_t)ptr[5] << 16) | ((uint64_t)ptr[4] << 24) |
          ((uint64_t)ptr[3] << 32) | ((uint64_t)ptr[2] << 40) | ((uint64_t)ptr[1] << 48) | ((uint64_t)ptr[0] << 56);
    return ret;
}

void wee16(bool b, unsigned char *&c, uint16_t v)
{
    b ? wle16(c, v) : wbe16(c, v);
}

void wee32(bool b, unsigned char *&c, uint32_t v)
{
    b ? wle32(c, v) : wbe32(c, v);
}

void wee64(bool b, unsigned char *&c, uint64_t v)
{
    b ? wle64(c, v) : wbe64(c, v);
}

void wees16(bool b, unsigned char *&c, int16_t v)
{
    b ? wles16(c, v) : wbes16(c, v);
}

void wees32(bool b, unsigned char *&c, int32_t v)
{
    b ? wles32(c, v) : wbes32(c, v);
}

void wees64(bool b, unsigned char *&c, int64_t v)
{
    b ? wles64(c, v) : wbes64(c, v);
}

uint16_t ree16(bool b, unsigned char *c)
{
    return b ? rle16(c) : rbe16(c);
}

uint32_t ree32(bool b, unsigned char *c)
{
    return b ? rle32(c) : rbe32(c);
}

uint64_t ree64(bool b, unsigned char *c)
{
    return b ? rle64(c) : rbe64(c);
}

uint32_t checked_32(uint64_t v)
{
    assert(v < 0x100000000LL);
    return v & 0xffffffff;
}

uint64_t roundup(uint64_t in, uint64_t align)
{
    if (in % align == 0)
    {
        return in;
    }
    return in + (align - (in % align));
}

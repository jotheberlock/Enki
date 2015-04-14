#include "platform.h"

#include <stdio.h>

void wle16(unsigned char *& ptr, uint16_t v)
{
    *((uint16_t *)ptr) = v;
    ptr += 2;
}

void wle32(unsigned char *& ptr, uint32_t v)
{
    *((uint32_t *)ptr) = v;
    ptr += 4;
}

void wle64(unsigned char *& ptr, uint64_t v)
{
    *((uint64_t *)ptr) = v;
    ptr += 8;
}

void wles16(unsigned char *& ptr, int16_t v)
{
    uint16_t us;
    *((int16_t *)(&us)) = v;
    wle16(ptr, us);
}

void wles32(unsigned char *& ptr, int32_t v)
{
    uint32_t us;
    *((int32_t *)(&us)) = v;
    wle32(ptr, us);
}

void wles64(unsigned char *& ptr, int64_t v)
{
    uint64_t us;
    *((int64_t *)(&us)) = v;
    wle64(ptr, us);
}

void wbe16(unsigned char *& ptr, uint16_t v)
{
    *((uint16_t *)ptr) = v;
    ptr += 2;
}

void wbe32(unsigned char *& ptr, uint32_t v)
{
    *((uint32_t *)ptr) = v;
    ptr += 4;
}

void wbe64(unsigned char *& ptr, uint64_t v)
{
    *((uint64_t *)ptr) = v;
    ptr += 8;
}

void wbes16(unsigned char *& ptr, int16_t v)
{
    uint16_t us;
    *((int16_t *)(&us)) = v;
    wle16(ptr, us);
}

void wbes32(unsigned char *& ptr, int32_t v)
{
    uint32_t us;
    *((int32_t *)(&us)) = v;
    wle32(ptr, us);
}

void wbes64(unsigned char *& ptr, int64_t v)
{
    uint64_t us;
    *((int64_t *)(&us)) = v;
    wle64(ptr, us);
}

void wee16(bool b, unsigned char *& c, uint16_t v)
{
    b ? wle16(c,v) : wbe16(c,v);
}

void wee32(bool b, unsigned char *& c, uint32_t v)
{
    b ? wle32(c,v) : wbe32(c,v);
}

void wee64(bool b, unsigned char *& c, uint64_t v)
{
    b ? wle64(c,v) : wbe64(c,v);
}

void wees16(bool b, unsigned char *& c, int16_t v)
{
    b ? wles16(c,v) : wbes16(c,v);
}

void wees32(bool b, unsigned char *& c, int32_t v)
{
    b ? wles32(c,v) : wbes32(c,v);
}

void wees64(bool b, unsigned char *& c, int64_t v)
{
    b ? wles64(c,v) : wbes64(c,v);
}

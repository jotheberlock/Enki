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

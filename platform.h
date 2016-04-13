#ifndef _PLATFORM_
#define _PLATFORM_

typedef unsigned long long uint64;
typedef signed long long int64;
typedef unsigned int uint32;
typedef signed int int32;
typedef unsigned short uint16;
typedef signed short int16;

#include <stdio.h>

#if defined(__CYGWIN__)
#define HAVE_MPROTECT 1
#define WINDOWS_CC 1
// #define POSIX_SIGNALS 1
#define CYGWIN_HOST 1
#define POSIX_HOST 1
#elif defined(__linux__)
#define HAVE_MPROTECT 1
#define SYSV_CC 
#define POSIX_SIGNALS 1
#define LINUX_HOST 1
#define POSIX_HOST 1
#elif defined(__APPLE__)
#define HAVE_MPROTECT 1
#define SYSV_CC 1
#define POSIX_SIGNALS 1
#define MACOS_HOST 1
#define POSIX_HOST 1
#elif defined(_MSC_VER)
#define HAVE_WINDOWS_API 1
#define WINDOWS_CC 1
#define WINDOWS_HOST 1
#else
#warning "Unknown host platform!"
// Guess at Posix
#define POSIX_HOST 1
#define SYSV_CC 1
#define HAVE_MPROTECT 1
#endif

void wle16(unsigned char *&, uint16);
void wle32(unsigned char *&, uint32);
void wle64(unsigned char *&, uint64);
void wles16(unsigned char *&, int16);
void wles32(unsigned char *&, int32);
void wles64(unsigned char *&, int64);

void wbe16(unsigned char *&, uint16);
void wbe32(unsigned char *&, uint32);
void wbe64(unsigned char *&, uint64);
void wbes16(unsigned char *&, int16);
void wbes32(unsigned char *&, int32);
void wbes64(unsigned char *&, int64);

void wee16(bool, unsigned char *&, uint16);
void wee32(bool, unsigned char *&, uint32);
void wee64(bool, unsigned char *&, uint64);
void wees16(bool, unsigned char *&, int16);
void wees32(bool, unsigned char *&, int32);
void wees64(bool, unsigned char *&, int64);

uint32 checked_32(uint64);

extern FILE * log_file;

uint64 roundup(uint64 in, uint64 align);

#endif

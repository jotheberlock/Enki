#ifndef _PLATFORM_
#define _PLATFORM_

#include <stdint.h>
#include <stdio.h>

#if defined(__CYGWIN__)
#define HAVE_MPROTECT 1
#define WINDOWS_CC 1
// #define POSIX_SIGNALS 1
#define CYGWIN_HOST 1
#elif defined(__linux__)
#define HAVE_MPROTECT 1
#define SYSV_CC 1
#define POSIX_SIGNALS 1
#define LINUX_HOST 1
#elif defined(__APPLE__)
#define HAVE_MPROTECT 1
#define SYSV_CC 1
#define POSIX_SIGNALS 1
#define MACOS_HOST 1
#elif defined(_MSC_VER)
#define HAVE_WINDOWS_API 1
#define WINDOWS_CC 1
#define WINDOWS_HOST 1
#else
#error "Unknown host platform!"
#endif

void wle16(unsigned char *&, uint16_t);
void wle32(unsigned char *&, uint32_t);
void wle64(unsigned char *&, uint64_t);
void wles16(unsigned char *&, int16_t);
void wles32(unsigned char *&, int32_t);
void wles64(unsigned char *&, int64_t);

void wbe16(unsigned char *&, uint16_t);
void wbe32(unsigned char *&, uint32_t);
void wbe64(unsigned char *&, uint64_t);
void wbes16(unsigned char *&, int16_t);
void wbes32(unsigned char *&, int32_t);
void wbes64(unsigned char *&, int64_t);

void wee16(bool, unsigned char *&, uint16_t);
void wee32(bool, unsigned char *&, uint32_t);
void wee64(bool, unsigned char *&, uint64_t);
void wees16(bool, unsigned char *&, int16_t);
void wees32(bool, unsigned char *&, int32_t);
void wees64(bool, unsigned char *&, int64_t);

uint32_t checked_32(uint64_t);

extern FILE * log_file;

uint64_t roundup(uint64_t in, uint64_t align);

#endif

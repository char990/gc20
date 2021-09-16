#ifndef __PRINTDBG_H__
#define __PRINTDBG_H__

#include <cstdio>

#ifdef DEBUG
#define PrintDbg(...) MyPrintf(__VA_ARGS__)
#else
#define PrintDbg(...)
#endif

extern void MyThrow(const char * fmt, ...);
extern int MyPrintf(const char * fmt, ...);

#endif

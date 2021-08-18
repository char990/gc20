#ifndef __PRINTDBG_H__
#define __PRINTDBG_H__

#include <cstdio>

#ifdef DEBUG
#define PrintDbg(...) printf(__VA_ARGS__)
#else
#define PrintDbg(...)
#endif

void MyThrow(const char * fmt, ...);


#endif

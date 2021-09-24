#pragma once


#include <cstdio>

#ifdef DEBUG
#define PrintDbg(...) MyPrintf(__VA_ARGS__)
#else
#define PrintDbg(...)
#endif

extern void MyThrow(const char * fmt, ...);
extern int MyPrintf(const char * fmt, ...);
extern void PrintDash(void);



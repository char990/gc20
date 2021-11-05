#pragma once


#include <cstdio>

// PrintDbg level
#define DBG_0 0
#define DBG_LOG 1

extern int dbg_level;
extern char _r_need_n;
extern void MyThrow(const char * fmt, ...);
extern int PrintDbg(int level, const char * fmt, ...);
extern void PrintDash(void);

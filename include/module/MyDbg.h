#pragma once


#include <cstdio>

// PrintDbg level
enum DBG_LEVEL { DBG_HB, DBG_PRT, DBG_LOG};

extern void MyThrow(const char * fmt, ...);
extern int PrintDbg(DBG_LEVEL level, const char * fmt, ...);
extern void PrintDash(void);

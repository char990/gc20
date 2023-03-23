#pragma once


#include <cstdio>
#include <string>
#include <stdexcept>

// PrintDbg level
enum DBG_LEVEL { DBG_HB, DBG_PRT, DBG_LOG};


extern int PrintDbg(DBG_LEVEL level, const char * fmt, ...);
extern void PrintDash(char c);

/// Print debug 
#define DebugPrt(...) PrintDbg(DBG_PRT, __VA_ARGS__)

/// Log debug
#define DebugLog(...) PrintDbg(DBG_LOG, __VA_ARGS__)


#define PrintAsc(x) printf((x < 0x20 || x > 0x7E) ? "<%02X>" : "%c", x)

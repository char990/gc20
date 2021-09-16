#include <cstdarg>
#include <stdexcept>
#include <cstdio>
#include <ctime>
#include <module/MyDbg.h>
#include <module/Utils.h>

using namespace Utils;

static char MyDbgBuf[1024];
void MyThrow(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(MyDbgBuf, 1024 - 1, fmt, args);
	va_end(args);
	throw std::runtime_error(&MyDbgBuf[0]);
}

int MyPrintf(const char *fmt, ...)
{
	time_t t = time(nullptr);
	MyDbgBuf[0]='[';
	char *p = Cnvt::ParseTmToLocalStr(t, MyDbgBuf+1);
	*p++=']';
	int len = p - MyDbgBuf;
	va_list args;
	va_start(args, fmt);
	len += vsnprintf(p, 1024 - 1 - len, fmt, args);
	va_end(args);
	puts(MyDbgBuf);
	return len;
}

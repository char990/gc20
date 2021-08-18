#include <cstdio>
#include <cstdarg>
#include <stdexcept>

static char MyThrowBuf[1024];

void MyThrow(const char * fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(MyThrowBuf, 1024-1 , fmt, args);
	va_end(args);
	throw std::runtime_error(&MyThrowBuf[0]);
}

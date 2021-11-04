#include <cstdarg>
#include <stdexcept>
#include <cstdio>
#include <ctime>
#include <sys/time.h>
#include <module/MyDbg.h>
#include <module/Utils.h>


using namespace Utils;
#define MyDbgBuf_SIZE 1024

static char MyDbgBuf[MyDbgBuf_SIZE];
void MyThrow(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(MyDbgBuf, MyDbgBuf_SIZE - 1, fmt, args);
	va_end(args);
	throw std::runtime_error(&MyDbgBuf[0]);
}

char _r_need_n=0;
int MyPrintf(const char *fmt, ...)
{
	struct timeval t;
	if(_r_need_n!=0)
	{
		putchar('\n');
		_r_need_n=0;
	}
	gettimeofday(&t,nullptr);
	MyDbgBuf[0]='[';
	char *p = Cnvt::ParseTmToLocalStr(&t, MyDbgBuf+1);
	*p++=']';
	int len = p - MyDbgBuf;
	va_list args;
	va_start(args, fmt);
	len += vsnprintf(p, MyDbgBuf_SIZE - 1 - len, fmt, args);
	va_end(args);
	printf("%s", MyDbgBuf);
	return len;
}

void PrintDash()
{
    #define DASH_LEN 40
	static char buf[DASH_LEN+2]={0};
    if(buf[0]==0)
    {
        char *p=buf;
		for(int i=0;i<DASH_LEN;i++)
		{
			*p++='-';
		}
        *p++='\n';
        *p++='\0';
    }
    puts(buf);
}

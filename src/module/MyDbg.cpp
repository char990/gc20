#include <cstdarg>
#include <stdexcept>
#include <cstdio>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <module/MyDbg.h>
#include <module/Utils.h>
#include <module/BootTimer.h>

using namespace Utils;
#define MyDbgBuf_SIZE 1024

int dbg_level{DBG_LOG};

static char MyDbgBuf[MyDbgBuf_SIZE];
void MyThrow(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(MyDbgBuf, MyDbgBuf_SIZE - 1, fmt, args);
	va_end(args);
	throw std::runtime_error(&MyDbgBuf[0]);
}

void Log(int);
char _r_need_n = 0;
int PrintDbg(int level, const char *fmt, ...)
{
	int len = 0;
	if (level == -1)
	{
		level = dbg_level;
	}
	if (level >= 0)
	{
		struct timeval t;
		if (_r_need_n != 0)
		{
			_r_need_n = 0;
			putchar('\n');
		}
		gettimeofday(&t, nullptr);
		MyDbgBuf[0] = '[';
		char *p = Cnvt::ParseTmToLocalStr(&t, MyDbgBuf + 1);
		*p++ = ']';
		len = p - MyDbgBuf;
		va_list args;
		va_start(args, fmt);
		len += vsnprintf(p, MyDbgBuf_SIZE - 1 - len, fmt, args);
		va_end(args);
		printf("%s", MyDbgBuf);
	}
	if (level >= DBG_LOG)
	{
		Log(len);
	}
	return len;
}

//BootTimer printTmr;
extern char *mainpath;
int days = 0;
void Log(int len)
{
	char filename[256];
	int d, m, y;
	int today;
	if (sscanf(MyDbgBuf, "[%d/%d/%d", &d, &m, &y) == 3)
	{
		snprintf(filename, 255, "%s/log/%d_%02d_%02d", mainpath, y, m, d);
		today = ((y * 0x100) + m) * 0x100 + d;
	}
	else
	{
		return;
	}
	if (days != 0 && days != today)
	{
		char rm[256];
		snprintf(rm, 255, "rm %s/log/*_%02d", mainpath, d);
		system(rm);
	}
	days = today;
	int log_fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
	if (log_fd < 0)
	{
		MyThrow("Open log file failed:%s", filename);
	}
	write(log_fd, MyDbgBuf, len);
	close(log_fd);
}

void PrintDash()
{
#define DASH_LEN 40
	static char buf[DASH_LEN + 2] = {0};
	if (buf[0] == 0)
	{
		char *p = buf;
		for (int i = 0; i < DASH_LEN; i++)
		{
			*p++ = '-';
		}
		*p++ = '\n';
		*p++ = '\0';
	}
	puts(buf);
}

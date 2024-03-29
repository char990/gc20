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
using namespace std;

#define MyDbgBuf_SIZE PRINT_BUF_SIZE
static char MyDbgBuf[MyDbgBuf_SIZE];

void Log(int);

int PrintDbg(DBG_LEVEL level, const char *fmt, ...)
{
	static DBG_LEVEL lastlvl;
	int len = 0;
	if (level >= DBG_HB)
	{
		if (lastlvl == DBG_HB)
		{
			putchar(level == DBG_HB ? '\r' : '\n');
		}
		lastlvl = level;
		struct timeval t;
		gettimeofday(&t, nullptr);
		MyDbgBuf[0] = '[';
		char *p = Time::ParseTimeToLocalStr(&t, MyDbgBuf + 1);
		*p++ = ']';
		len = p - MyDbgBuf;
		va_list args;
		va_start(args, fmt);
		int xs = MyDbgBuf_SIZE - len - 1;
		int xlen = vsnprintf(p, xs, fmt, args);
		va_end(args);
		if (xlen >= xs)
		{ // MyDbgBuf full
			strcat(MyDbgBuf + MyDbgBuf_SIZE - 9, " ......");
			len = MyDbgBuf_SIZE - 2;
		}
		else
		{
			len += xlen;
		}
		if (level > DBG_HB)
		{
			MyDbgBuf[len++] = '\n';
			MyDbgBuf[len] = '\0';
		}
		printf("%s", MyDbgBuf);
	}
	if (level >= DBG_LOG)
	{
		Log(len);
	}
	return len;
}

// BootTimer printTmr;
extern char *mainpath;
int days = 0;
void Log(int len)
{
	char filename[PRINT_BUF_SIZE];
	int d, m, y;
	if (sscanf(MyDbgBuf, "[%d/%d/%d", &d, &m, &y) != 3)
	{
		return;
	}
	snprintf(filename, PRINT_BUF_SIZE - 1, "%s/log/%d_%02d_%02d", mainpath, y, m, d);
	int today = ((y * 0x100) + m) * 0x100 + d;
	if (days != 0 && days != today)
	{
		Exec::Shell("rm %s/log/*_%02d > /dev/null 2>&1", mainpath, d);
	}
	days = today;
	int log_fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
	if (log_fd < 0)
	{
		throw runtime_error(StrFn::PrintfStr("Open log file failed:%s", filename));
	}
	write(log_fd, MyDbgBuf, len);
	close(log_fd);
}

void PrintDash(char c)
{
#define DASH_LEN 80
	string dash(DASH_LEN, c);
	puts(dash.c_str());
}

/* Copyright (c) 2011, RidgeRun
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the RidgeRun.
 * 4. Neither the name of the RidgeRun nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY RIDGERUN ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL RIDGERUN BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "gpio/GpioEx.h"
#include "module/MyDbg.h"
#include <module/Utils.h>

using namespace Utils;
using namespace std;

/****************************************************************
 * Constants
 *****************************************************************/

#define SYSFS_GPIO_DIR "/sys/class/gpio"

#define GpioEx_BUF_SIZE 256

GpioEx::GpioEx(unsigned int pin, DIR inout)
	: _fd(-1), _pin(pin)
{
	// Export();
	SetDir(inout);
	if (inout == DIR::INPUT)
	{
		SetEdge(EDGE::NONE);
	}
	OpenFd();
}

GpioEx::GpioEx(unsigned int pin, EDGE edge)
	: _fd(-1), _pin(pin)
{
	// Export();
	SetDir(DIR::INPUT);
	SetEdge(edge);
	OpenFd();
}

GpioEx::~GpioEx()
{
	if (_fd > 0)
	{
		close(_fd);
	}
	Unexport();
}

/****************************************************************
 * gpio_export
 ****************************************************************/
void GpioEx::Export(unsigned int pin)
{
	int fd, len;
	char buf[GpioEx_BUF_SIZE];

	fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
	if (fd < 0)
	{
		throw runtime_error(StrFn::PrintfStr("Can't open \"export\" for gpio%d\n", pin));
	}

	len = snprintf(buf, sizeof(buf) - 1, "%d\n", pin);
	write(fd, buf, len);
	close(fd);
	snprintf(buf, sizeof(buf) - 1, "%s/gpio%d/value", SYSFS_GPIO_DIR, pin);
	if (!Exec::FileExists(buf))
	{
		throw runtime_error(StrFn::PrintfStr("export gpio%d failed\n", pin));
	}
}

/****************************************************************
 * gpio_unexport
 ****************************************************************/
void GpioEx::Unexport(unsigned int pin)
{
	int fd, len;
	char buf[GpioEx_BUF_SIZE];

	fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
	if (fd < 0)
	{
		throw runtime_error(StrFn::PrintfStr("Can't open \"unexport\" for gpio%d\n", pin));
	}

	len = snprintf(buf, sizeof(buf) - 1, "%d\n", pin);
	write(fd, buf, len);
	close(fd);
	snprintf(buf, sizeof(buf) - 1, "%s/gpio%d/value", SYSFS_GPIO_DIR, pin);
	if (Exec::FileExists(buf))
	{
		throw runtime_error(StrFn::PrintfStr("unexport gpio%d failed\n", pin));
	}
}

void GpioEx::Unexport()
{
	Unexport(_pin);
}

/****************************************************************
 * gpio_set_dir
 ****************************************************************/
void GpioEx::SetDir(DIR inout)
{
	int fd;
	char buf[GpioEx_BUF_SIZE];
	snprintf(buf, sizeof(buf) - 1, SYSFS_GPIO_DIR "/gpio%d/direction", _pin);
	int retry = 3;
	do
	{
		fd = open(buf, O_WRONLY);
		if (fd < 0)
		{
			if (--retry)
			{
				Utils::Time::SleepMs(1000);
			}
			else
			{
				throw runtime_error(StrFn::PrintfStr("Can't open \"direction\" for gpio%d\n", _pin));
			}
		}
	}while(fd<0);

	if (inout == DIR::OUTPUT)
	{
		if (write(fd, "out", 4) < 4)
		{
			close(fd);
			throw runtime_error(StrFn::PrintfStr("Write \"direction\" failed for gpio%d\n", _pin));
		}
		_dir = DIR::OUTPUT;
	}
	else
	{
		if (write(fd, "in", 3) < 3)
		{
			close(fd);
			throw runtime_error(StrFn::PrintfStr("Write \"direction\" failed for gpio%d\n", _pin));
		}
		_dir = DIR::INPUT;
	}
	close(fd);
}

/****************************************************************
 * gpio_set_value
 ****************************************************************/
void GpioEx::SetValue(bool value)
{
	if (_dir != DIR::OUTPUT)
	{
		throw runtime_error(StrFn::PrintfStr("pin %d is NOT output\n", _pin));
	}
	if (lseek(_fd, 0, SEEK_SET) >= 0)
	{
		if (write(_fd, value ? "1" : "0", 2) > 0)
		{
			fdatasync(_fd);
			return;
		}
	}
	close(_fd);
	_fd = -1;
	throw runtime_error(StrFn::PrintfStr("GpioEX::SetValue(): Write gpio%d/value failed\n", _pin));
}

/****************************************************************
 * gpio_get_value
 ****************************************************************/
int GpioEx::GetValue()
{
	char ch;
	if (_dir != DIR::INPUT)
	{
		throw runtime_error(StrFn::PrintfStr("gpio%d is NOT input\n", _pin));
	}
	if (lseek(_fd, 0, SEEK_SET) >= 0)
	{
		if (read(_fd, &ch, 1) > 0)
		{
			return ch == '1';
		}
	}
	close(_fd);
	_fd = -1;
	throw runtime_error(StrFn::PrintfStr("GpioEX::GetValue(): Read gpio%d/value failed\n", _pin));
	return -1;
}

/****************************************************************
 * gpio_set_edge
 ****************************************************************/
void GpioEx::SetEdge(EDGE edge)
{
	if (_dir == DIR::OUTPUT)
	{
		return;
	}
	int fd;
	char buf[GpioEx_BUF_SIZE];
	const char *p;
	switch (edge)
	{
	case EDGE::BOTHRF:
		p = "both";
		break;
	case EDGE::RISING:
		p = "rising";
		break;
	case EDGE::FALLING:
		p = "falling";
		break;
	default:
		p = "none";
		break;
	}
	snprintf(buf, sizeof(buf) - 1, SYSFS_GPIO_DIR "/gpio%d/edge", _pin);
	fd = open(buf, O_WRONLY);
	if (fd < 0)
	{
		throw runtime_error(StrFn::PrintfStr("Can't open \"edge\" for gpio%d\n", _pin));
	}
	write(fd, p, strlen(p) + 1);
	close(fd);
	_edge = edge;
}

/****************************************************************
 * gpio_fd_open
 ****************************************************************/
int GpioEx::OpenFd()
{
	char buf[GpioEx_BUF_SIZE];
	snprintf(buf, sizeof(buf) - 1, SYSFS_GPIO_DIR "/gpio%d/value", _pin);
	_fd = open(buf, (_dir == DIR::OUTPUT) ? O_WRONLY : O_RDONLY | O_NONBLOCK);
	if (_fd < 0)
	{
		throw runtime_error(StrFn::PrintfStr("GpioEX::_Open(): Open gpio%d/value failed\n", _pin));
	}
	return _fd;
}

/****************************************************************
 * gpio_fd_close
 ****************************************************************/

int GpioEx::CloseFd()
{
	if (_fd > 0)
	{
		return close(_fd);
	}
	return 0;
}

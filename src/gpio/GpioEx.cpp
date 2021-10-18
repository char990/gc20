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

/****************************************************************
 * Constants
 *****************************************************************/

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_BUF 128

GpioEx::GpioEx(unsigned int pin, int inout)
	: _fd(-1), _pin(pin)

{
	_ex = Export();
	SetDir(inout);
}

GpioEx::~GpioEx()
{
	if (_fd > 0)
	{
		close(_fd);
	}

	if (_ex == 0)
	{
		Unexport();
	}
}

/****************************************************************
 * gpio_export
 ****************************************************************/
int GpioEx::Export()
{
	int fd, len;
	char buf[MAX_BUF];

	fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
	if (fd < 0)
	{
		MyThrow("Can't open \"export\" for pin %d\n", _pin);
		return fd;
	}

	len = snprintf(buf, sizeof(buf) - 1, "%d\n", _pin);
	write(fd, buf, len);
	close(fd);

	return 0;
}

/****************************************************************
 * gpio_unexport
 ****************************************************************/
int GpioEx::Unexport()
{
	int fd, len;
	char buf[MAX_BUF];

	fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
	if (fd < 0)
	{
		MyThrow("Can't open \"unexport\" for pin %d\n", _pin);
		return fd;
	}

	len = snprintf(buf, sizeof(buf), "%d\n", _pin);
	write(fd, buf, len);
	close(fd);
	return 0;
}

/****************************************************************
 * gpio_set_dir
 ****************************************************************/
int GpioEx::SetDir(int inout)
{
	int fd;
	char buf[MAX_BUF];
	_dir = -1;
	if (_ex == 0)
	{
		snprintf(buf, sizeof(buf) - 1, SYSFS_GPIO_DIR "/gpio%d/direction", _pin);

		fd = open(buf, O_WRONLY);
		if (fd < 0)
		{
			MyThrow("Can't open \"direction\" for pin %d\n", _pin);
			return fd;
		}

		if (inout == 0)
		{
			write(fd, "out", 4);
			_dir = 0;
		}
		else
		{
			write(fd, "in", 3);
			_dir = 1;
		}
		close(fd);
	}
	return _dir;
}

/****************************************************************
 * gpio_set_value
 ****************************************************************/
int GpioEx::SetValue(bool value)
{
	if (_dir != 0)
	{
		return -1;
	}

	if (_fd > 0)
	{
		write(_fd, value ? "1" : "0", 2);
	}
	else
	{
		int fd;
		char buf[MAX_BUF];
		snprintf(buf, sizeof(buf) - 1, SYSFS_GPIO_DIR "/gpio%d/value", _pin);
		fd = open(buf, O_WRONLY);
		if (fd < 0)
		{
			MyThrow("Can't set \"value\" for pin %d\n", _pin);
			return fd;
		}
		write(fd, value ? "1" : "0", 2);
		fsync(fd);
		close(fd);
	}
	return 0;
}

/****************************************************************
 * gpio_get_value
 ****************************************************************/
int GpioEx::GetValue()
{
	if (_dir == 1)
	{
		char ch;
		if (_fd > 0)
		{
			lseek(_fd, 0, SEEK_SET);
			if (read(_fd, &ch, 1) <= 0)
			{
				MyThrow("Can't get \"value\" for pin %d\n", _pin);
			}
		}
		else
		{
			int fd;
			char buf[MAX_BUF];

			snprintf(buf, sizeof(buf) - 1, SYSFS_GPIO_DIR "/gpio%d/value", _pin);

			fd = open(buf, O_RDONLY | O_NONBLOCK );
			if (fd < 0)
			{
				MyThrow("Can't get \"value\" for pin %d\n", _pin);
				return fd;
			}
			if (read(fd, &ch, 1) <= 0)
			{
				MyThrow("Can't get \"value\" for pin %d\n", _pin);
			}
			close(fd);
		}
		return ch;
	}
	return -1;
}

/****************************************************************
 * gpio_set_edge
 ****************************************************************/

int GpioEx::SetEdge(char *edge)
{
	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf) - 1, SYSFS_GPIO_DIR "/gpio%d/edge", _pin);

	fd = open(buf, O_WRONLY);
	if (fd < 0)
	{
		MyThrow("Can't open \"edge\" for pin %d\n", _pin);
		return fd;
	}

	write(fd, edge, strlen(edge) + 1);
	close(fd);
	return 0;
}

/****************************************************************
 * gpio_fd_open
 ****************************************************************/

int GpioEx::OpenFd()
{
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf) - 1, SYSFS_GPIO_DIR "/gpio%d/value", _pin);
	if (_dir == 0)
	{
		_fd = open(buf, O_WRONLY);
	}
	else if (_dir == 1)
	{
		_fd = open(buf, O_RDONLY | O_NONBLOCK);
	}
	else
	{
		MyThrow("\"direction\" for pin %d undefined\n", _pin);
		return -1;
	}

	if (_fd < 0)
	{
		MyThrow("Can't open \"value\" for pin %d\n", _pin);
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

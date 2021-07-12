/*
 * 
 *
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <linux/serial.h>
#include <termios.h>

#include <stdexcept>

#include "SerialPort.h"
#include "BootTimer.h"

using namespace std;

SerialPort::SerialPort(const std::string &device, SerialPortConfig &config, Epoll * epoll)
	: spConfig(config), spDevice(device), epoll(epoll), spFileDesc(-1)
{
	spConfig.ByteBits();
}

SerialPort::~SerialPort()
{
	try
	{
		Close();
	}
	catch (...)
	{
		// We can't do anything about this!
		// But we don't want to throw within destructor, so swallow
	}
}

int SerialPort::Open()
{
	// Attempt to open file
	//this->fileDesc = open(this->filePath, O_RDWR | O_NOCTTY | O_NDELAY);

	// O_RDONLY for read-only, O_WRONLY for write only, O_RDWR for both read/write access
	// 3rd, optional parameter is mode_t mode
	spFileDesc = open(spDevice.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
	// Check status
	if (spFileDesc == -1)
	{
		return -1;
	}
	ConfigureTermios();
	spState = State::OPEN;
	epoll->AddEvent(spFileDesc, EPOLLIN, this);
	return 0;
}

void SerialPort::ConfigureTermios()
{
	//================== CONFIGURE ==================//
	struct termios tty;
	memset(&tty, 0, sizeof(tty));
	// Get current settings (will be stored in termios structure)
	if (tcgetattr(spFileDesc, &tty) != 0)
	{
		throw std::runtime_error("tcgetattr() failed: " + spDevice);
	}
	//================= (.c_cflag) ===============//

	tty.c_cflag &= ~PARENB;		   // No parity bit is added to the output characters
	tty.c_cflag &= ~CSTOPB;		   // Only one stop-bit is used
	tty.c_cflag &= ~CSIZE;		   // CSIZE is a mask for the number of bits per character
	tty.c_cflag |= CS8;			   // Set to 8 bits per character
	tty.c_cflag &= ~CRTSCTS;	   // Disable hadrware flow control (RTS/CTS)
	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

	//===================== BAUD RATE =================//
	tty.c_cflag &= ~CBAUD;
	switch (spConfig.baudRate)
	{
	case 300:
		tty.c_cflag |= B300;
		break;
	case 600:
		tty.c_cflag |= B600;
		break;
	case 1200:
		tty.c_cflag |= B1200;
		break;
	case 2400:
		tty.c_cflag |= B2400;
		break;
	case 4800:
		tty.c_cflag |= B4800;
		break;
	case 9600:
		tty.c_cflag |= B9600;
		break;
	case 19200:
		tty.c_cflag |= B19200;
		break;
	case 38400:
		tty.c_cflag |= B38400;
		break;
	case 57600:
		tty.c_cflag |= B57600;
		break;
	case 115200:
		tty.c_cflag |= B115200;
		break;
	case 230400:
		tty.c_cflag |= B230400;
		break;
	case 460800:
		tty.c_cflag |= B460800;
		break;
	case 921600:
		tty.c_cflag |= B921600;
		break;
	default:
		throw invalid_argument("BaudRate unrecognized: " + std::to_string(spConfig.baudRate));
	}

	//===================== (.c_oflag) =================//

	tty.c_oflag = 0; // No remapping, no delays
	//tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	//tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

	//================= CONTROL CHARACTERS (.c_cc[]) ==================//

	// c_cc[VTIME] sets the inter-character timer, in units of 0.1s.
	// Only meaningful when port is set to non-canonical mode
	// VMIN = 0, VTIME = 0: No blocking, return immediately with what is available
	// VMIN > 0, VTIME = 0: read() waits for VMIN bytes, could block indefinitely
	// VMIN = 0, VTIME > 0: Block until any amount of data is available, OR timeout occurs
	// VMIN > 0, VTIME > 0: Block until either VMIN characters have been received, or VTIME
	//                      after first character has elapsed
	// c_cc[WMIN] sets the number of characters to block (wait) for when read() is called.
	// Set to 0 if you don't want read to block. Only meaningful when port set to non-canonical mode
	// Setting both to 0 will give a non-blocking read
	tty.c_cc[VTIME] = 0;
	tty.c_cc[VMIN] = 0;

	//======================== (.c_iflag) ====================//

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

	//=========================== LOCAL MODES (c_lflag) =======================//

	// Canonical input is when read waits for EOL or EOF characters before returning. In non-canonical mode, the rate at which
	// read() returns is instead controlled by c_cc[VMIN] and c_cc[VTIME]
	tty.c_lflag &= ~ICANON; // Turn off canonical input, which is suitable for pass-through
	tty.c_lflag &= ~(ECHO);
	tty.c_lflag &= ~ECHOE;	// Turn off echo erase (echo erase only relevant if canonical input is active)
	tty.c_lflag &= ~ECHONL; //
	tty.c_lflag &= ~ISIG;	// Disables recognition of INTR (interrupt), QUIT and SUSP (suspend) characters

	// Flush port, then apply attributes
	tcflush(spFileDesc, TCIFLUSH);
	if (tcsetattr(spFileDesc, TCSANOW, &tty) != 0)
	{
		throw std::runtime_error("tcsetattr() failed: " + spDevice);
	}
}

int SerialPort::Write(const uint8_t *data, int len)
{
	if (spState != State::OPEN || spFileDesc < 0)
	{
		throw std::runtime_error("Write() failed: " + spDevice + " is not open");
	}
	if(len>TX_BUFFER_SIZE)
	{
		throw std::out_of_range("Write() failed: len>TxBuffer");
	}
	if(bytesSent!=txDataLen && !txTimeout.IsExpired())
	{
		return -1;	// have not finished
	}
	memcpy(txBuffer, data, len);
	txDataLen=len;
	bytesSent=0;
	int writeResult = write(spFileDesc, data, len);
	// Check status
	if (writeResult == -1)
	{
		throw std::runtime_error("Write() failed: write()");
	}
	bytesSent+=writeResult;
	int us = len*spConfig.byteBits*1000000L/spConfig.baudRate;
	if(us==0)us=1;
	txTimeout.Setus(us);
	return us;
}

void SerialPort::FlushBuffer()
{
	if (spState != State::OPEN || spFileDesc < 0)
	{
		throw std::runtime_error("Write() failed: " + spDevice + " is not open");
	}
	if(bytesSent!=txDataLen)
	{
		int writeResult = write(spFileDesc, &txBuffer[bytesSent], txDataLen-bytesSent);
		// Check status
		if (writeResult > 0)
		{
			bytesSent+=writeResult;
		}
	}
}

int SerialPort::Read(uint8_t *buffer, int buffer_len)
{
	if (spFileDesc == 0)
	{
		throw std::runtime_error("Read() failed: " + spDevice + " is not open");
	}

	// Read from file
	// We provide the underlying raw array from the readBuffer_ vector to this C api.
	// This will work because we do not delete/resize the vector while this method
	// is called
	ssize_t n = read(spFileDesc, buffer, buffer_len);

	// Error Handling
	if (n < 0)
	{
		// Read was unsuccessful
		throw std::runtime_error("Read() failed: return " + n);
	}
	return n;
}

void SerialPort::InEvent()
{
    #define BUF_SIZE 4096
	uint8_t buf[BUF_SIZE];
    int n = Read(buf, BUF_SIZE);
	rcvd+=n;
    printf("Got %d ,total %d\n", n, rcvd);
}

void SerialPort::OutEvent()
{

}

void SerialPort::Error(uint32_t events)
{

}

int SerialPort::Close()
{
	if (spFileDesc > 0)
	{
		epoll->DeleteEvent(spFileDesc, EPOLLIN, this);
		auto retVal = close(spFileDesc);
		if (retVal != 0)
		{
			throw std::runtime_error("Close() failed: " + spDevice);
		}
		spFileDesc = -1;
	}
	spState = State::CLOSED;
	return 0;
}

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

using namespace std;

namespace GC20 {
    SerialPort::SerialPort(const std::string &device, SerialPortConfig &config)
	:_config(config)
    {
		for(int i=0;i<7;i++)
		{
			if(device.compare(names[i][0])==0)
			{
				_device = names[i][1];
				break;
			}
			if(i==6)
			{
				throw invalid_argument("SerialPort unrecognized: " + device);
			}
		}
        _config.ByteBits();
	}

	SerialPort::~SerialPort() {
        try {
            Close();
        } catch(...) {
            // We can't do anything about this!
            // But we don't want to throw within destructor, so swallow
        }
	}

	void SerialPort::Open()
	{
		// Attempt to open file
		//this->fileDesc = open(this->filePath, O_RDWR | O_NOCTTY | O_NDELAY);

		// O_RDONLY for read-only, O_WRONLY for write only, O_RDWR for both read/write access
		// 3rd, optional parameter is mode_t mode
		_spFileDesc = open(_device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
		// Check status
		if(_spFileDesc == -1) {
		    throw runtime_error("Could not open device " + _device);
		}
        ConfigureTermios();
        _state = State::OPEN;
	}

	void SerialPort::ConfigureTermios()
	{
		//================== CONFIGURE ==================//
		struct termios tty;
		memset(&tty, 0, sizeof(tty));
		// Get current settings (will be stored in termios structure)
		if(tcgetattr(_spFileDesc, &tty) != 0)
		{
			throw std::runtime_error("tcgetattr() failed: " + _device);
		}
		//================= (.c_cflag) ===============//

		tty.c_cflag     &=  ~PARENB;       	// No parity bit is added to the output characters
		tty.c_cflag     &=  ~CSTOPB;		// Only one stop-bit is used
		tty.c_cflag     &=  ~CSIZE;			// CSIZE is a mask for the number of bits per character
		tty.c_cflag     |=  CS8;			// Set to 8 bits per character
		tty.c_cflag     &=  ~CRTSCTS;       // Disable hadrware flow control (RTS/CTS)
		tty.c_cflag     |=  CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

        //===================== BAUD RATE =================//
		tty.c_cflag &= ~CBAUD;
		switch(_config.baudRate)
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
				throw invalid_argument("BaudRate unrecognized: " + std::to_string(_config.baudRate));
		}
		
		//===================== (.c_oflag) =================//

		tty.c_oflag     =   0;              // No remapping, no delays
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

		tty.c_iflag     &= ~(IXON | IXOFF | IXANY);			// Turn off s/w flow ctrl
		tty.c_iflag 	&= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);

		//=========================== LOCAL MODES (c_lflag) =======================//

		// Canonical input is when read waits for EOL or EOF characters before returning. In non-canonical mode, the rate at which
		// read() returns is instead controlled by c_cc[VMIN] and c_cc[VTIME]
		tty.c_lflag	&= ~ICANON;								// Turn off canonical input, which is suitable for pass-through
		tty.c_lflag &= ~(ECHO);
		tty.c_lflag	&= ~ECHOE;								// Turn off echo erase (echo erase only relevant if canonical input is active)
		tty.c_lflag	&= ~ECHONL;								//
		tty.c_lflag	&= ~ISIG;								// Disables recognition of INTR (interrupt), QUIT and SUSP (suspend) characters

		// Flush port, then apply attributes
		tcflush(_spFileDesc, TCIFLUSH);
		if(tcsetattr(_spFileDesc, TCSANOW, &tty) != 0)
		{
			throw std::runtime_error("tcsetattr() failed: " + _device);
		}
	}

	int SerialPort::Write(const uint8_t *data, int len)
	{
        if(_state != State::OPEN || _spFileDesc < 0)
		{
			throw std::runtime_error("Write() failed: " + _device + " is not open");
		}
		int writeResult = write(_spFileDesc, data, len);
		// Check status
		if (writeResult == -1)
		{
			throw std::runtime_error("Write() failed: write()");
		}
		if(writeResult != len)
		{
			throw std::runtime_error("Write() failed: " + std::to_string(writeResult) + " of " + std::to_string(len) + " sent");
		}
		return 0;
	}

	int SerialPort::Read(uint8_t *buffer, int buffer_len)
	{
        if(_spFileDesc == 0)
		{
			throw std::runtime_error("Read() failed: " + _device + " is not open");
		}

		// Read from file
        // We provide the underlying raw array from the readBuffer_ vector to this C api.
        // This will work because we do not delete/resize the vector while this method
        // is called
		ssize_t n = read(_spFileDesc, buffer, buffer_len);

		// Error Handling
		if(n < 0)
		{
			// Read was unsuccessful
			throw std::runtime_error("Read() failed: return " + n);
		}
		return n;
	}

    void SerialPort::Close()
	{
        if(_spFileDesc != -1)
		{
            auto retVal = close(_spFileDesc);
            if(retVal != 0)
			{
				throw std::runtime_error("Close() failed: " + _device);
			}
			_spFileDesc = -1;
        }
        _state = State::CLOSED;
    }
} // namespace GC20

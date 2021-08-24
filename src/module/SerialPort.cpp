#include <unistd.h>
#include <fcntl.h>
#include <linux/serial.h>
#include <termios.h>

#include <cstring>
#include <module/MyDbg.h>

#include <module/SerialPort.h>

using namespace std;

SerialPort::SerialPort(const std::string &device, SerialPortConfig &config)
	: spConfig(config), spDevice(device), spFileDesc(-1)
{
	spConfig.Bytebits();
}

SerialPort::~SerialPort()
{
	Close();
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
		MyThrow("tcgetattr() failed: %s", spDevice.c_str());
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
	switch (spConfig.baudrate)
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
		MyThrow("baudrate unrecognized: %d", spConfig.baudrate);
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
		MyThrow("tcsetattr() failed: %s", spDevice.c_str());
	}
}

int SerialPort::Close()
{
	if (spFileDesc > 0)
	{
		auto retVal = close(spFileDesc);
		if (retVal != 0)
		{
			MyThrow("Close() failed: %s", spDevice.c_str());
		}
		spFileDesc = -1;
	}
	return 0;
}

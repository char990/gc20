#ifndef __SERIALPORT_H__
#define __SERIALPORT_H__

#include <string>

class SerialPortConfig
{
public:
    enum class SpMode
    {
        RS232,    // RS232 mode, no RTS/CTS
        RS485_01, // RS485 mode, RTC=0 when sending / RTS=1 after sent
        RS485_10, // RS485 mode, RTC=1 when sending / RTS=0 after sent
    };
    enum class DataBits
    {
        B_7 = 7,
        B_8 = 8
    };
    enum class Parity
    {
        NONE,
        ODD,
        EVEN
    };
    enum class StopBits
    {
        B_1 = 1,
        B_2 = 2
    };
    SerialPortConfig(SpMode mode, int baudrate) : mode(mode), baudrate(baudrate)
    {
    }
    SpMode mode;
    int baudrate;
    DataBits databits = DataBits::B_8;
    Parity parity = Parity::NONE;      // not used
    StopBits stopbits = StopBits::B_1; // not used
    int bytebits = 10;
    int Bytebits()
    {
        bytebits = 1 /*start*/ + (int)databits + ((parity == Parity::NONE) ? 0 : 1) + (int)stopbits;
        return bytebits;
    }
};

/// \brief		SerialPort object is used to perform rx/tx serial communication.
class SerialPort
{
public:
    /// \brief		Constructor that sets up serial port with parameters.
    SerialPort(const std::string &device, SerialPortConfig &config);

    /// \brief		Destructor. Closes serial port if still open.
    virtual ~SerialPort();

    /// \brief		Get current configuration
    /// \return     struct SerialPortConfig *
    SerialPortConfig *Config() { return &spConfig; };

    /// \brief		Opens the COM port for use.
    /// \return     0:success; -1:failed
    /// \note		Must call this before you can configure the COM port.
    int Open();

    /// \brief		Closes the COM port.
    /// \return     0:success; -1:failed
    int Close();

    /// \brief		Get read/write fd
    /// \return     int fd. -1 if not open
    int GetFd() { return spFileDesc; };

private:
    /*
        std::string names[7][2] = {
        {"RS232", "/dev/ttymxc3"},
        {"COM1", "/dev/ttymxc2"},
        {"COM2", "/dev/ttymxc1"},
        {"COM3", "/dev/ttymxc5"},
        {"COM4", "/dev/ttymxc4"},
        {"COM5", "/dev/ttySC1"},
        {"COM6", "/dev/ttySC0"},
        };*/

    /// \brief      The file path to the serial port device (e.g. "/dev/ttyUSB0").
    std::string spDevice;

    /// \brief		Serial port configuration.
    struct SerialPortConfig spConfig;

    /// \brief		Configures the tty device as a serial port.
    /// \warning    Device must be open (valid file descriptor) when this is called.
    void ConfigureTermios();

    /// \brief		The file descriptor for the open file. This gets written to when Open() is called.
    int spFileDesc;
};
#endif /* __SERIALPORT_H__ */
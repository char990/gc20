#ifndef __SERIALPORT_H__
#define __SERIALPORT_H__

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/serial.h>
#include <termios.h>

#include <string>

#include "IByteStream.h"
#include "IGcEvent.h"
#include "BootTimer.h"
#include "Epoll.h"

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
    SerialPortConfig(SpMode mode, int baudRate) : mode(mode), baudRate(baudRate)
    {
    }
    SpMode mode;
    int baudRate;
    DataBits databits = DataBits::B_8;
    Parity parity = Parity::NONE;      // not used
    StopBits stopbits = StopBits::B_1; // not used
    int byteBits = 10;
    int ByteBits()
    {
        byteBits = 1 /*start*/ + (int)databits + ((parity == Parity::NONE) ? 0 : 1) + (int)stopbits;
        return byteBits;
    }
};

/// \brief		SerialPort object is used to perform rx/tx serial communication.
class SerialPort : public IByteStream, public IGcEvent
{
public:
    /// \brief		Constructor that sets up serial port with parameters.
    SerialPort(const std::string &device, SerialPortConfig &config, Epoll * epoll);

    /// \brief		Destructor. Closes serial port if still open.
    virtual ~SerialPort();

    /// \brief      Represents the state of the serial port.
    enum class State
    {
        CLOSED,
        OPEN
    };

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

    /// \brief		Copy msg to buffer and send over the com port.
    /// \param		data		The data that will be written to the COM port.
    /// \param		len		    data length.
    /// \return     int         >0 time(us) for sending out all data; -1 : busy
    /// \throws		if state != OPEN.
    int Write(const uint8_t *data, int len);
    
    /// \brief		Send data in Tx buffer over the com port.
    /// \throws		if state != OPEN.
    void FlushBuffer();

    /// \brief		Use to read from the COM port.
    /// \param		buffer		The object the read characters from the COM port will be saved to.
    /// \param      maxlen      buffer length
    /// \return     int         bytes haved been saved in buffer; -1:
    int Read(uint8_t *buffer, int buffer_len);

    int SpFileDesc(){return spFileDesc;};

    /// \brief		Send data in Tx buffer over the com port.
    void InEvent();

    /// \brief		Send data in Tx buffer over the com port.
    void OutEvent();

    /// \brief		Send data in Tx buffer over the com port.
    void Error(uint32_t events);

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
    /// \brief      epoll for event
    Epoll * epoll=nullptr;

    /// \brief      The file path to the serial port device (e.g. "/dev/ttyUSB0").
    std::string spDevice;

    /// \brief		Serial port configuration.
    struct SerialPortConfig spConfig;

    /// \brief		Configures the tty device as a serial port.
    /// \warning    Device must be open (valid file descriptor) when this is called.
    void ConfigureTermios();

    /// \brief      Keeps track of the serial port's state.
    State spState;

    /// \brief		The file descriptor for the open file. This gets written to when Open() is called.
    int spFileDesc;

    /// \brief		Tx buffer
    #define TX_BUFFER_SIZE (129*1024)
    uint8_t txBuffer[TX_BUFFER_SIZE];
    int txDataLen;
    int bytesSent;

    BootTimer txTimeout;

    int rcvd;
};
#endif /* __SERIALPORT_H__ */

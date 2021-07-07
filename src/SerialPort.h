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

#include "ByteStream.h"

namespace GC20
{
    class SerialPortConfig
    {
    public:
        enum class SpMode
        {
            RS232,    // RS232 mode, no RTS/CTS
            RS485_01, // RS485 mode, RTC=0 when sending / RTS=1 after sent
            RS485_10, // RS485 mode, RTC=1 when sending / RTS=0 after sent
        };
        enum class DataBits {B_7=7, B_8=8};
        enum class Parity {NONE, ODD, EVEN};
        enum class StopBits {B_1=1, B_2=2};
        SerialPortConfig(SpMode mode, int baudRate):mode(mode),baudRate(baudRate)
        {}
        SpMode mode;
        int baudRate;
        DataBits databits=DataBits::B_8;
        Parity parity=Parity::NONE;      // not used
        StopBits stopbits=StopBits::B_1;  // not used
        int byteBits=10;
        int ByteBits()
        {
            byteBits = 1/*start*/ + (int)databits + ((parity==Parity::NONE)?0:1) + (int)stopbits;
            return byteBits;
        }
    };

    /// \brief		SerialPort object is used to perform rx/tx serial communication.
    class SerialPort : public IByteStream
    {
    public:
        /// \brief		Constructor that sets up serial port with parameters.
        SerialPort(const std::string &device, SerialPortConfig &config);

        /// \brief		Destructor. Closes serial port if still open.
        virtual ~SerialPort();

        /// \brief      Represents the state of the serial port.
        enum class State {CLOSED, OPEN};

        /// \brief		Get current configuration
        /// \return     struct SerialPortConfig *
        struct SerialPortConfig *Config() { return &_config; };

        /// \brief		Opens the COM port for use.
        /// \throws		CppLinuxSerial::Exception if device cannot be opened.
        /// \note		Must call this before you can configure the COM port.
        void Open();

        /// \brief		Closes the COM port.
        void Close();

        /// \brief		Sends a message over the com port.
        /// \param		data		The data that will be written to the COM port.
        /// \param		len		    data length.
        /// \return     int         time(us) for sending out all data
        /// \throws		if state != OPEN.
        int Write(const uint8_t *data, int len);

        /// \brief		Use to read from the COM port.
        /// \param		buffer		The object the read characters from the COM port will be saved to.
        /// \param      maxlen      buffer length
        /// \return     int         byte haved been saved in buffer
        /// \throws		if state != OPEN.
        int Read(uint8_t *buffer, int buffer_len);

    private:
        std::string names[7][2] = {
        {"RS232", "/dev/ttymxc3"},
        {"COM1", "/dev/ttymxc2"},
        {"COM2", "/dev/ttymxc1"},
        {"COM3", "/dev/ttymxc5"},
        {"COM4", "/dev/ttymxc4"},
        {"COM5", "/dev/ttySC1"},
        {"COM6", "/dev/ttySC0"},
        };
        
        /// \brief      The file path to the serial port device (e.g. "/dev/ttyUSB0").
        std::string _device;

        /// \brief		Serial port configuration.
        struct SerialPortConfig _config;

        /// \brief		Configures the tty device as a serial port.
        /// \warning    Device must be open (valid file descriptor) when this is called.
        void ConfigureTermios();


        /// \brief      Keeps track of the serial port's state.
        State _state;

        /// \brief		The file descriptor for the open file. This gets written to when Open() is called.
        int _spFileDesc;
    };
} // namespace GC20

#endif /* __SERIALPORT_H__ */

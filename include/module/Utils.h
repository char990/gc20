#ifndef __UTILS_H__
#define __UTILS_H__

#include <cstdint>
#include <string>

namespace Utils
{

    extern const uint32_t MASK_BIT[32];

    class Check
    {
    public:
        /// \brief  Check if input is a legal Hex string
        /// \return 0:success
        static int HexStr(uint8_t *frm, int len);

        /// \brief  Check if input is text(0x20-0x7F)
        /// \return 0:success
        static int Text(uint8_t *frm, int len);
    };

    class Cnvt
    {
    public:
        /// \brief  Reverse a uint8_t Example: 0x1A => 0x85
        static uint8_t Reverse(uint8_t n);

        /// \brief  parse 2 Asc to 1 uint8_t Hex. Example: "1F" => 0x1F
        /// \return     int: 0-255:success, -1:failed, there is invalid chars
        static int ParseToU8(char *p);

        /// \brief  parse 2 Asc to 1 Hex. Example: "1F....." => 0x1F......
        /// \param      src : ascii buffer
        /// \param      dst : hex buffer
        /// \param      srclen : ascii len ( = hex_len *2)
        /// \return     int: 0:success, -1:failed, there is invalid chars
        static int ParseToU8(char *src, uint8_t *dst, int srclen);

        /// \brief  parse Asc to uint16_t hex. Example: "1F0A" => 0x1F0A
        /// \param      src : ascii buffer
        /// \return     int: >0:success, -1:failed, there is invalid chars
        static int ParseToU16(char *src);

        /// \brief  parse Asc to uint32_t hex. Example: "1F0A3456" => 0x1F0A3456
        /// \param      src : ascii buffer
        /// \return     int64_t: >0:success, -1:failed, there is invalid chars
        static int64_t ParseToU32(char *src);

        /// \brief  parse 1 Hex to 2 Asc. Example: 0x1F => "1F"
        static void ParseToAsc(uint8_t h, char *p);

        /// \brief  parse 1 Hex to 2 Asc. Example: 0x1F..... => "1F......."
        /// \param      src : hex buffer
        /// \param      dst : ascii buffer
        /// \param      srclen : hex len ( = asc_len / 2)
        static void ParseToAsc(uint8_t *src, char *dst, int srclen);

        /// \brief	convert int string to array, "2,3,100" => {0x02,0x03,0x64}
        ///			If number is less than min or greater than max, just return
        /// \param	src: input
        /// \param	srcmax: max src size to be converted
        /// \param	dst: output
        /// \param	min: min
        /// \param	max: max
        /// \return numbers converted
        static int GetIntArray(const char *src, int srcmax, int *dst, int min, int max);
    };

    class Crc
    {
    public:
// online crc calculator http://www.sunshine2k.de/coding/javascript/crc/crc_js.html
// If combines the data packet with the crc to be a new data packet,
// the crc of the new data packet will be 0

//CRC8-CCITT - x^8+x^2+x+1
/*
        This CRC can be specified as:
        Width  : 8
        Poly   : 0x07
        Init   : 0
        RefIn  : false
        RefOut : false
        XorOut : 0
        */
#define PRE_CRC8 0
        static const uint8_t crc8_table[256];
        static uint8_t Crc8(uint8_t *buf, int len, uint8_t precrc = PRE_CRC8);

/// \brief Crc16
#define PRE_CRC16 0

        static uint16_t Crc16(const uint16_t *table, uint8_t *buf, int len,
                              uint16_t init, bool refIn, bool refOut, uint16_t xorOut);

        //CRC16-CCITT polynomial X^16 + X^12 + X^5 + X^0 0x11021
        /*
        This CRC can be specified as:
        Width  : 16
        Poly   : 0x1021
        Init   : 0
        RefIn  : false
        RefOut : false
        XorOut : 0
        */
        static const uint16_t crc16_1021[256];
        static uint16_t Crc16_1021(uint8_t *buf, int len, uint16_t precrc = PRE_CRC16);

        //CRC16-IBM polynomial X^15 + X^2 + X^0 0x8005
        /* !!! Not test
        This CRC can be specified as:
        Width  : 16
        Poly   : 0x8005
        Init   : 0
        RefIn  : true
        RefOut : true
        XorOut : 0
        */
        static const uint16_t crc16_8005[256];
        static uint16_t Crc16_8005(uint8_t *buf, int len, uint16_t precrc = PRE_CRC16);

/*
        https://android.googlesource.com/toolchain/binutils/+/53b6ed3bceea971857c996b6dcb96de96b99335f/binutils-2.19/libiberty/crc32.c
        This CRC can be specified as:
        Width  : 32
        Poly   : 0x04c11db7
        Init   : 0xffffffff
        RefIn  : false
        RefOut : false
        XorOut : 0
        This differs from the "standard" CRC-32 algorithm in that the values
        are not reflected, and there is no final XOR value.  These differences
        make it easy to compose the values of multiple blocks.
        */
#define PRE_CRC32 0xFFFFFFFF
        static const uint32_t crc32_table[256];

        /*! \brief CRC32 function. Poly   : 0x04C11DB7
         *
         * \param buf   input array
         * \param len   how many bytes
         *
         * \return crc32
         */
        static uint32_t Crc32(uint8_t *buf, int len, uint32_t precrc = PRE_CRC32);
    };

    class Exec
    {
    public:
        /// \brief      Run a command and save the output to outbuf
        /// \param      cmd:command
        /// \param      outbuf:output
        /// \param      len:output buffer size
        /// \return     -1:failed; >=0:output size(include 0x0a attached)
        static int Run(const char *cmd, char *outbuf, int len);
    
        /// \brief      Copy a file
        /// \param      src:
        /// \param      dst:
        static void CopyFile(const char *src, const char *dst);


        /// \brief      Check if a file/dir exists
        static bool FileExists(const char *path);
        static bool DirExists(const char *path);

    };

    class Time
    {
    public:
        /// \brief      Print time
        void PrintTime();
        
        /// \brief      Get interval from last Interval() called
        /// \return     interval of ms
        long Interval();
    };


    class BitOption
    {
    public:
        BitOption();
        BitOption(uint32_t v);
        void Set(uint32_t v);
        uint32_t Get();
        void SetBit(int b);
        void ClrBit(int b);
        bool GetBit(int b);
        std::string ToString();
    private:
        uint32_t bits;
    };
} // namespace Utils
#endif

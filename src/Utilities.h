#ifndef __UTILITIES_H__
#define __UTILITIES_H__

#include <cstdint>

namespace Util
{
    class Cnvt
    {
    public:
        static uint8_t Reverse(uint8_t n)
        {
            static uint8_t lookup[16] = {
                0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
                0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf};
            // Reverse the top and bottom nibble then swap them.
            return (lookup[n & 0b1111] << 4) | lookup[n >> 4];
        }

        /// \brief  parse 2 Asc to 1 uint8_t Hex. Example: "1F" => 0x1F
        /// \return     int: 0-255:success, -1:failed, there is invalid chars
        static int ParseToU8(uint8_t *p)
        {
            int k = 0;
            for (int i = 0; i < 2; i++)
            {
                int j;
                if (*p >= 0x30 && *p <= 0x39) // '0' - '9'
                {
                    j = (*p - 0x30);
                }
                else if (*p >= 0x41 && *p <= 0x46) // 'A' - 'F'
                {
                    j = (*p - 0x41 + 0x0A);
                }
                else
                {
                    return -1;
                }
                k = k * 16 + j;
            }
            return k;
        }

        /// \brief  parse 2 Asc to 1 Hex. Example: "1F" => 0x1F
        /// \param      src : ascii buffer
        /// \param      dst : hex buffer
        /// \param      len : ascii len ( = hex_len *2)
        /// \return     int: 0:success, -1:failed, there is invalid chars
        static int ParseToU8(uint8_t *src, uint8_t *dst, int len)
        {
            if((len&1)==1 || len <=0)
                return -1;
            for(int i=0;i<len;i++)
            {
                int x = ParseToU8(src);
                if(x<0)
                    return -1;
                *dst=x;
                dst++;
                src+=2;
            }
            return 0;
        }

        /// \brief  parse Asc to uin16_6 hex. Example: "1F0A" => 0x1F0A
        /// \param      src : ascii buffer
        /// \return     int: >0:success, -1:failed, there is invalid chars
        static int ParseToU16(uint8_t *src)
        {
            int k=0;
            for(int i=0;i<2;i++)
            {
                int x = ParseToU8(src);
                if(x<0)
                    return -1;
                k+=x;
                src+=2;
            }
            return k;
        }

        /// \brief  parse 1 Hex to 2 Asc. Example: 0x1F => "1F"
        static void ParseToAsc(uint8_t h, uint8_t *p)
        {
            static uint8_t ASC[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
            *p++ = ASC[(h >> 4)];
            *p = ASC[h & 0x0F];
        }

        /// \brief  parse 1 Hex to 2 Asc. Example: 0x1F => "1F"
        /// \param      src : hex buffer
        /// \param      dst : ascii buffer
        /// \param      len : hex len ( = asc_len / 2)
        static void ParseToAsc(uint8_t *src, uint8_t *dst, int len)
        {
            for(int i=0;i<len;i++)
            {
                ParseToAsc(*src, dst);
                src++;
                dst+=2;
            }
        }

#if 0
    /// <summary>
    /// Get a uint8_t of upper hex Ascii from 0-F
    /// Example: 0x01 => 0x31
    /// </summary>
    /// <param name="b">Hex number</param>
    /// <returns>uint8_t Ascii</returns>
    static public uint8_t HexToAsc(uint8_t b)
    {
        var v = b & 0x0F;
        if (v >= 0 && v <= 9) // 0 - 9
        {
            return (uint8_t)(v + 0x30);
        }
        else // A - F
        {
            return (uint8_t)(v - 0x0A + 0x41);
        }
    }

    /// <summary>
    /// Get a uint8_t from 1 upper hex Ascii
    /// Example: 0x31 => 0x01
    /// </summary>
    /// <param name="a1">uint8_t Ascii</param>
    /// <param name="hex">uint8_t hex</param>
    /// <returns>true : sucess; false:illegal a1</returns>
    static public bool AscToHex(uint8_t a1, out uint8_t hex)
    {
        if (a1 >= 0x30 && a1 <= 0x39) // '0' - '9'
        {
            hex = (uint8_t)(a1 - 0x30);
            return true;
        }
        if (a1 >= 0x41 && a1 <= 0x46) // 'A' - 'F'
        {
            hex = (uint8_t)(a1 - 0x41 + 0x0A);
            return true;
        }
        hex = 0;
        return false;
    }

    /// <summary>
    /// Get a uint8_t from 2 upper hex Ascii
    /// Example: 0x31 0x39 => 0x19
    /// </summary>
    /// <param name="a1-2">uint8_t Ascii</param>
    /// <param name="hex">uint8_t hex</param>
    /// <returns>true : sucess; false:illegal a1-2</returns>
    static public bool AscToHex(uint8_t a1, uint8_t a2, out uint8_t hex)
    {
        bool r1, r2;
        uint8_t h1, h2;
        r1 = AscToHex(a1, out h1);
        r2 = AscToHex(a2, out h2);
        hex = (uint8_t)(h1 * 16 + h2);
        return r1 & r2;
    }

    /// <summary>
    /// Get a uint8_t from 4 upper hex Ascii
    /// Example: 0x31 0x39 0x41 0x4F => 0x19AF
    /// </summary>
    /// <param name="a1-4">uint8_t Ascii</param>
    /// <param name="hex">uint8_t hex</param>
    /// <returns>true : sucess; false:illegal a1-4</returns>
    static public bool AscToHex(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, out ushort hex)
    {
        bool r1, r2;
        uint8_t h1, h2;
        r1 = AscToHex(a1, a2, out h1);
        r2 = AscToHex(a3, a4, out h2);
        hex = (ushort)(h1 * 256 + h2);
        return r1 & r2;
    }

    /// <summary>
    /// Get a UInt64 from uint8_ts in a char array
    /// Example: uint8_t[] buf is {0x31, 0x41, 0x30, 0x34}("1A04"), GetUInt64Fromuint8_tChar(buf,2,2) gets 0x04
    /// </summary>
    /// <param name="charArray">uint8_t array</param>
    /// <param name="index">start from the array</param>
    /// <param name="len">how many uint8_ts to get</param>
    /// <returns>true:success; false:illegal chars</returns>
    static public bool AscToHex(uint8_t[] charArray, int index, int len, out UInt64 hex)
    {
        if (len == 0 || len > 8 || (index + len > charArray.Length))
        {
            throw new ArgumentOutOfRangeException();
        }
        hex = 0;
        uint8_t h;
        for (int i = 0; i < len; i++, index++)
        {
            if (AscToHex(charArray[index], out h))
            {
                hex = hex * 0x10 + h;
            }
            else
            {
                return false;
            }
        }
        return true;
    }

    /// <summary>
    /// Get a UInt64 from uint8_ts in a char array
    /// Example: uint8_t[] buf is {0x31, 0x41, 0x30, 0x34}("1A04"), GetUInt64Fromuint8_tChar(buf,2,2) returns 0x04
    /// </summary>
    /// <param name="charArray">uint8_t array</param>
    /// <param name="index">start from the array</param>
    /// <param name="len">how many uint8_ts to get</param>
    /// <returns>UInt64</returns>
    static public UInt64 GetUInt64FromAscHexArray(uint8_t[] charArray, int index, int len)
    {
        if (len == 0 || len > 8 || (index + len > charArray.Length))
        {
            throw new ArgumentOutOfRangeException();
        }
        UInt64 r = 0;
        for (int i = 0; i < len; i++, index++)
        {
            r = r * 0x10 + AscToHex(charArray[index]);
        }
        return r;
    }

    /// <summary>
    /// Get a UInt32 from 4 uint8_ts in a uint8_t array
    /// Example: uint8_t[] buf is {0,1,2,3,4,5 }, GetUInt32Fromuint8_tArray(buf,2) returns 0x02030405
    /// </summary>
    /// <param name="uint8_tArray">uint8_t array</param>
    /// <param name="index">start from the array</param>
    /// <returns>UInt32</returns>
    static public UInt32 GetUInt32Fromuint8_tArray(uint8_t[] uint8_tArray, int index)
    {
        if ((index + 4 > uint8_tArray.Length))
        {
            throw new ArgumentOutOfRangeException();
        }
        UInt32 r = 0;
        for (int i = 0; i < 4; i++)
        {
            r <<= 8;
            r |= uint8_tArray[index++];
        }
        return r;
    }

    /// <summary>
    /// Get a UInt16 from 2 uint8_ts in a uint8_t array
    /// Example: uint8_t[] buf is {0,1,2,3,4,5 }, GetUInt32Fromuint8_tArray(buf,2) returns 0x0203
    /// </summary>
    /// <param name="uint8_tArray">uint8_t array</param>
    /// <param name="index">start from the array</param>
    /// <returns>UInt16</returns>
    static public UInt16 GetUInt16Fromuint8_tArray(uint8_t[] uint8_tArray, int index)
    {
        if ((index + 2 > uint8_tArray.Length))
        {
            throw new ArgumentOutOfRangeException();
        }
        UInt16 r = 0;
        for (int i = 0; i < 2; i++)
        {
            r <<= 8;
            r |= uint8_tArray[index++];
        }
        return r;
    }

    /// <summary>
    /// convert string to Hex
    /// </summary>
    /// <param name="text"></param>
    /// <returns>UInt32</returns>
    static public UInt32 FromHexText(string text)
    {
        try
        {
            return UInt32.Parse(text, System.Globalization.NumberStyles.HexNumber);
        }
        catch
        {
            return 0;
        }
    }

    /// <summary>
    /// convert string to Hex
    /// </summary>
    /// <param name="text"></param>
    /// <param name="min"></param>
    /// <param name="max"></param>
    /// <returns>UInt32</returns>
    static public UInt32 FromHexText(string text, UInt32 min, UInt32 max)
    {
        try
        {
            UInt32 r = UInt32.Parse(text, System.Globalization.NumberStyles.HexNumber);
            if (r < min || r > max)
            {
                return min;
            }
            return r;
        }
        catch
        {
            return min;
        }
    }

    /// <summary>
    /// convert string to Dec
    /// </summary>
    /// <param name="text"></param>
    /// <returns>UInt32</returns>
    static public UInt32 FromDecText(string text)
    {
        try
        {
            return UInt32.Parse(text, System.Globalization.NumberStyles.Integer);
        }
        catch
        {
            return 0;
        }
    }

    /// <summary>
    /// convert string to Dec
    /// </summary>
    /// <param name="text"></param>
    /// <param name="min"></param>
    /// <param name="max"></param>
    /// <returns>UInt32</returns>
    static public UInt32 FromDecText(string text, UInt32 min, UInt32 max)
    {
        try
        {
            UInt32 r = UInt32.Parse(text, System.Globalization.NumberStyles.Integer);
            if (r < min || r > max)
            {
                return min;
            }
            return r;
        }
        catch
        {
            return min;
        }
    }

    /// <summary>
    /// Convert a hex array to ascii array
    /// Example: {0x01, 0xAF} => {0x30,0x31,0x41,0x46} ('0','1','A','F')
    /// </summary>
    /// <param name="hex"></param>
    /// <param name="index"></param>
    /// <param name="len"></param>
    /// <returns></returns>
    static public uint8_t[] HexArray2AscArray(uint8_t[] hex, int index, int len)
    {
        if (index < 0 || index > hex.Length - 1)
        {
            throw new Exception("index error");
        }
        if (index + len > hex.Length)
        {
            throw new Exception("len error");
        }
        uint8_t[] r = new uint8_t[len * 2];
        for (int i = 0; i < len; i++, index++)
        {
            r[i * 2] = HexToAsc((uint8_t)((hex[index] / 16) & 0x0F));
            r[i * 2 + 1] = HexToAsc((uint8_t)(hex[index] & 0x0F));
        }
        return r;
    }

    /// <summary>
    /// Convert a ascii array to hex array
    /// Example: {0x30,0x31,0x41,0x46} ('0','1','A','F') => {0x01, 0xAF}
    /// </summary>
    /// <param name="asc"></param>
    /// <param name="index"></param>
    /// <param name="len"></param>
    /// <returns></returns>
    static public uint8_t[] AscArray2HexArray(uint8_t[] asc, int index, int len)
    {
        if (index < 0 || index > asc.Length - 1)
        {
            throw new Exception("index error");
        }
        if (index + len > asc.Length)
        {
            throw new Exception("len error");
        }
        uint8_t[] r = new uint8_t[len];
        for (int i = 0; i < len; i++, index += 2)
        {
            r[i] = (uint8_t)(AscToHex(asc[index]) * 16 + AscToHex(asc[index + 1]));
        }
        return r;
    }
#endif
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
} // namespace Util

#endif

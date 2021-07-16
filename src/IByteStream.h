#ifndef __IBYTESTREAM_H__
#define __IBYTESTREAM_H__

#include <cstdint>

class IByteStream {
   public:
      virtual int Open() = 0;
      virtual int Close() = 0;
      virtual int GetFd() = 0;
      virtual int Read(uint8_t *buffer, int buffer_len) = 0;
      virtual int Write(const uint8_t *data, int len) = 0;
};

#endif

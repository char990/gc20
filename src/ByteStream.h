#ifndef __BYTESTREAM_H__
#define __BYTESTREAM_H__

#include <cstdint>

class IByteStream {
   public:
      // pure virtual function providing interface framework.
      virtual void Open() = 0;
      virtual void Close() = 0;
      virtual int Write(const uint8_t *data, int len) = 0;
      virtual int Read(uint8_t *buffer, int buffer_len) = 0;
};

#endif

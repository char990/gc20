#ifndef __IBYTESTREAM_H__
#define __IBYTESTREAM_H__

#include <cstdint>

class IByteStream {
   public:
      // pure virtual function providing interface framework.
      virtual int Open() = 0;
      virtual int Close() = 0;
      virtual int Write(const uint8_t *data, int len) = 0;
      virtual int Read(uint8_t *buffer, int buffer_len) = 0;
      uint8_t dataIn[129*1024];
      uint8_t dataOut[129*1024];
};

#endif

#include <cstring>
#include <module/RingBuffer.h>

RingBuffer::RingBuffer(int n)
{
    buffer = new uint8_t[n];
    size=n;
    used=0;
    index=0;
}

RingBuffer::~RingBuffer()
{
    delete [] buffer;
}

int RingBuffer::Write(uint8_t *src, int len)
{
    return 0;
}

int RingBuffer::Read(uint8_t *dst, int len)
{
    return 0;
}

void Drop(int n)
{
    
}

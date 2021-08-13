#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#include <cstdint>

class RingBuffer
{
public:
    RingBuffer(int n);
    ~RingBuffer();

    int Size() { return size; };

    int Used() { return used; };

    int Unused() { return size-used; };

    /// \brief      Write from src to Ringbuffer
    /// \param		src		    src buffer
    /// \param		len		    data length
    /// \return     int         bytes were written
    int Write(uint8_t *src, int len);

    /// \brief      Read from Ringbuffer to dst
    /// \param		dst 		dst buffer
    /// \param		len		    data length
    /// \return     int         bytes were read
    int Read(uint8_t *dst, int len);

    /// \brief      Drop n bytes in Ringbuffer
    /// \param		n bytes
    void Drop(int n);

private:
    uint8_t *buffer;
    int used;
    int size;
    int index;
};

#endif

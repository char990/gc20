#pragma once

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <cstring>

class RingBuf
{
public:
    RingBuf(int s) : size(s), cnt(0)
    {
        buf = new char[s];
        pPush = pPop = buf;
        bufEnd = buf + s;
    };
    ~RingBuf()
    {
        delete[] buf;
    }

    int Push(char *inbuf, int len)
    {
        if (len > (size - cnt) || len <= 0)
        {
            return 0;
        }
        int size1 = bufEnd - pPush;
        if (size1 > len)
        {
            memcpy(pPush, inbuf, len);
            pPush += len;
        }
        else
        {
            memcpy(pPush, inbuf, size1);
            memcpy(buf, inbuf + size1, len - size1);
            pPush = buf + len - size1;
        }
        cnt += len;
        return len;
    }

    int Pop(char *outbuf, int len)
    {
        if (cnt == 0 || len == 0)
        {
            return 0;
        }
        if (len > cnt)
        {
            len = cnt;
        }
        int size1 = bufEnd - pPop;
        if (len > size1)
        {
            memcpy(outbuf, pPop, size1);
            memcpy(outbuf + size1, buf, len - size1);
            pPop = buf + len - size1;
        }
        else
        {
            memcpy(outbuf, pPop, len);
            pPop += len;
        }
        cnt -= len;
        return len;
    }

    int Size() { return size; };
    int Cnt() { return cnt; };
    int Cap() { return size - cnt; };

private:
    int size;
    int cnt;
    char *pPush;
    char *pPop;
    char *buf;
    char *bufEnd;
};

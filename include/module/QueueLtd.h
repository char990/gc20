#pragma once

#include <string>
#include <queue>
#include <module/Utils.h>

class QueueLtd : public std::queue<std::string>
{
public:
    QueueLtd(int size) : ltdSize(size){};

    void Push(const std::string &value)
    {
        if (size() == ltdSize)
        {
            pop();
        }
        push(value);
    }

    void Push(const std::string &&value)
    {
        if (size() == ltdSize)
        {
            pop();
        }
        push(value);
    }

    // 1: in; 0:out
    void Push(char id, uint8_t *buf, int len, int in_out)
    {
        std::vector<char> pbuf(len + 23 + 3 + 1);
        struct timeval t;
        gettimeofday(&t, nullptr);
        char *p = Utils::Time::ParseTimeToLocalStr(&t, pbuf.data()); // "dd/mm/yyyy hh:mm:ss.mmm" 23 bytes
        if (in_out)                                                  // 3 bytes
        {
            *p++ = '<';
            *p++ = '<';
            *p++ = '<';
        }
        else
        {
            *p++ = '>';
            *p++ = '>';
            *p++ = '>';
        }

        int i;
        for (i = 0; i < len; i++)
        {
            *p++ = (buf[i] < ' ' || buf[i] >= 0x7F) ? ' ' : buf[i];
        }
        *p = '\0';
        if (id == ' ')
        {
            p = pbuf.data() + 11; // skip data, start from "hh:mm:ss.mm"
        }
        else
        {
            p = pbuf.data() + 8; // set id, start from "[X]hh:mm:ss.mm"
            p[0] = '[';
            p[1] = id;
            p[2] = ']';
        }
        Push(std::string(p)); // skip data, start from "hh:mm:ss.mm"
    }

    std::string Pop()
    {
        auto r = front();
        pop();
        return r;
    }

    int MaxSize() { return ltdSize; }

private:
    const int ltdSize;
};

extern QueueLtd *qltdSlave;
extern QueueLtd *qltdTmc;

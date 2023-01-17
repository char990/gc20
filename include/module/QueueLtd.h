#pragma once

#include <string>
#include <deque>
#include <module/Utils.h>

class QueueLtd : public std::deque<std::string>
{
public:
    QueueLtd(int size) : ltdSize(size){};

    void PushBack(const std::string &value)
    {
        if (size() == ltdSize)
        {
            pop_front();
        }
        push_back(value);
    }

    void PushBack(const std::string &&value)
    {
        if (size() == ltdSize)
        {
            pop_front();
        }
        push_back(value);
    }

    // 1: in; 0:out
    void PushBack(char id, uint8_t *buf, int len, int in_out)
    {
        std::vector<char> pbuf(len + 23 + 3 + 1);
        struct timeval t;
        gettimeofday(&t, nullptr);
        char *p = Utils::Time::ParseTimeToLocalStr(&t, pbuf.data()); // "dd/mm/yyyy hh:mm:ss.mmm" max 23 bytes
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
        // modify "dd/mm/yyyy hh:mm:ss.mmm"
        if (id == ' ')
        {
            p = pbuf.data() + 11; // skip header, start from "hh:mm:ss.mm"
        }
        else
        {
            p = pbuf.data() + 7; // set id, start from "[X]-hh:mm:ss.mm"
            p[0] = '[';
            p[1] = id;
            p[2] = ']';
            p[3] = '-';
        }
        PushBack(std::string(p)); // skip data, start from "hh:mm:ss.mm"
    }

    std::string PopFront()
    {
        auto r = front();
        pop_front();
        return r;
    }

    std::string PopBack()
    {
        auto r = back();
        pop_back();
        return r;
    }

    int MaxSize() { return ltdSize; }

private:
    const int ltdSize;
};

extern QueueLtd *qltdSlave;
extern QueueLtd *qltdTmc;

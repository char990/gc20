#ifndef __FREETIMER_H__
#define __FREETIMER_H__

#include <time.h>

class FreeTimer
{
    public:

        /// \brief		Set timer in ms (1/1000 second)
        /// \param		ms (If ms<0, set as LONG_MAX)
        void SetMs(int ms);

        /// \brief		If the timer is expired
        bool IsExpired();

    private:
        time_t sec=0;
        __syscall_slong_t ns=0;
};

#endif

#ifndef __BOOTTIMER_H__
#define __BOOTTIMER_H__

#include <time.h>

class BootTimer
{
    public:

        /// \brief		Set timer in ms (1/1000 second)
        /// \param		ms (If ms<0, set as LONG_MAX)
        void Setms(long ms);

        /// \brief		Set timer in us (1/1000000 second)
        /// \param		us (If ms<0, set as LONG_MAX)
        void Setus(long us);

        /// \brief		If the timer is expired
        bool IsExpired();

    private:
        time_t sec;
        __syscall_slong_t ns;
};

#endif

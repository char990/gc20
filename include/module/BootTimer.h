#pragma once

#include <limits.h>
#include <time.h>

class BootTimer
{
    public:

        /// \brief		Set timer in ms (1/1000 second)
        /// \param		ms (If ms<0, set as LONG_MAX)
        void Setms(long ms);

        /// \brief		Set timer in us (1/1000000 second)
        /// \param		us (If us<0, set as LONG_MAX)
        void Setus(long us);

        /// \brief		If the timer is expired
        bool IsExpired();

        /// \brief		Clear timer, will be never expired
        void Clear();

    private:
        time_t sec{LONG_MAX};
        __syscall_slong_t ns{0};
};


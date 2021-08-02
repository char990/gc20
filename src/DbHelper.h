#ifndef __DBHELPER_H__
#define __DBHELPER_H__

#include <cstdint>

#define dbHelper (DbHelper::Instance())
class DbHelper
{
private:
    DbHelper() {}

public:
    DbHelper(DbHelper const &) = delete;
    void operator=(DbHelper const &) = delete;
    static DbHelper &Instance()
    {
        static DbHelper instance;
        return instance;
    }

    void Init();

    uint8_t TsiSp003Ver();

    uint8_t BroadcastId();
    void BroadcastId(uint8_t);

    uint8_t DeviceId();
    void DeviceId(uint8_t);

    uint8_t SeedOffset();
    void SeedOffset(uint8_t);

    uint16_t PasswdOffset();
    void PasswdOffset(uint16_t);

    uint16_t SessionTimeout();
    void SessionTimeout(uint16_t);
    
    uint16_t DisplayTimeout();
    void DisplayTimeout(uint16_t);
};

#endif

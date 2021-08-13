#ifndef __USERCFG_H__
#define __USERCFG_H__


#include <uci/UciCfg.h>

class UciUser : public UciCfg
{
public:
    UciUser();
    char const* path = "/etc/config";
    char const* package = "goblin_user";

    void LoadConfig() override;
	void Dump() override;

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

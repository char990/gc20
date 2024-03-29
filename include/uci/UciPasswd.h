#pragma once

#include <string>
#include <map>

#include <uci/UciCfg.h>
#include <module/Utils.h>

class UserPasswd
{
public:
    std::string passwd;
    int permission;
};

class UciPasswd : public UciCfg
{
public:
    virtual void LoadConfig() override;
    virtual void Dump() override;

    void Set(const std::string & user, const std::string & passwd, const int permission);
    UserPasswd * GetUserPasswd(const std::string & user);

private:
    std::map<std::string, UserPasswd> mapUserPass;
    // section type
    const char *_User = "user";
    
    // option name in section user
    const char *_Passwd = "passwd";
    const char *_Permission = "permission";
};

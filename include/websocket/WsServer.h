#pragma once
#include <map>
#include <memory>
#include <stdexcept>

#include <3rdparty/mongoose/mongoose.h>
#include <3rdparty/nlohmann/json.hpp>

#include <module/TimerEvent.h>
#include <module/Utils.h>
#include <sign/Controller.h>

using json = nlohmann::json;

class WsCmd
{
public:
    const char *cmd;
    void (*function)(struct mg_connection *c, json &msg, json &rpl);
};

class WsClient
{
#define WsMsgBuf_SIZE (4 * 1024 * 1024)
public:
    WsClient() { buf[0] = '\0'; }
    size_t len{0};
    char buf[WsMsgBuf_SIZE];
    bool login{false};
    std::string user;
};
class WsServer : public IPeriodicRun
{
public:
    WsServer(int port, TimerEvent *tmrEvt);
    ~WsServer();

    virtual void PeriodicRun() override;

private:
    static bool isWsOccupied;
    static Controller *ctrller;
    struct mg_mgr mgr; // Event manager
    static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data);
    TimerEvent *tmrEvt{nullptr};

    static const char *uri_ws;

    static int GetUint(json &msg, const char *str, unsigned int min, unsigned int max);
    static int GetUintFromStr(json &msg, const char *str, unsigned int min, unsigned int max, bool chknull=true);
    static std::string GetStr(json &msg, const char *str, bool chknull=true);
    template <typename T>
    static std::vector<T> GetVector(json &msg, const char *str)
    {
        try
        {
            return msg[str].get<std::vector<T>>();
        }
        catch (...)
        {
            throw std::invalid_argument(Utils::StrFn::PrintfStr("Invalid '%s'", str));
        }
    }

    static void WebSokectProtocol(struct mg_connection *c, struct mg_ws_message *wm);
    static size_t WebSocketSend(struct mg_connection *c, json &reply);

    static const WsCmd CMD_LIST[];

    static void CMD_Login(struct mg_connection *c, json &msg, json &reply);
    static void CMD_GetGroupConfig(struct mg_connection *c, json &msg, json &reply);
    static void CMD_SetGroupConfig(struct mg_connection *c, json &msg, json &reply);
    static void CMD_GetStatus(struct mg_connection *c, json &msg, json &reply);
    static void CMD_ChangePassword(struct mg_connection *c, json &msg, json &reply);
    static void CMD_GetUserConfig(struct mg_connection *c, json &msg, json &reply);
    static void CMD_SetUserConfig(struct mg_connection *c, json &msg, json &reply);
    static void CMD_DefaultUserConfig(struct mg_connection *c, json &msg, json &reply);
    static void CMD_GetNetworkConfig(struct mg_connection *c, json &msg, json &reply);
    static void CMD_SetNetworkConfig(struct mg_connection *c, json &msg, json &reply);
    static void CMD_ControlDimming(struct mg_connection *c, json &msg, json &reply);
    static void CMD_ControlPower(struct mg_connection *c, json &msg, json &reply);
    static void CMD_ControlDevice(struct mg_connection *c, json &msg, json &reply);
    static void CMD_SystemReset(struct mg_connection *c, json &msg, json &reply);
    static void CMD_UpdateTime(struct mg_connection *c, json &msg, json &reply);
    static void CMD_GetFrameSetting(struct mg_connection *c, json &msg, json &reply);
    static void CMD_GetStoredFrame(struct mg_connection *c, json &msg, json &reply);
    static void CMD_SetFrame(struct mg_connection *c, json &msg, json &reply);
    static void CMD_DisplayFrame(struct mg_connection *c, json &msg, json &reply);
    static void CMD_GetStoredMessage(struct mg_connection *c, json &msg, json &reply);
    static void CMD_SetMessage(struct mg_connection *c, json &msg, json &reply);
    static void CMD_DisplayMessage(struct mg_connection *c, json &msg, json &reply);
    static void CMD_GetStoredPlan(struct mg_connection *c, json &msg, json &reply);
    static void CMD_SetPlan(struct mg_connection *c, json &msg, json &reply);
    static void CMD_RetrieveFaultLog(struct mg_connection *c, json &msg, json &reply);
    static void CMD_RetrieveAlarmLog(struct mg_connection *c, json &msg, json &reply);
    static void CMD_RetrieveEventLog(struct mg_connection *c, json &msg, json &reply);
    static void CMD_ResetFaultLog(struct mg_connection *c, json &msg, json &reply);
    static void CMD_ResetAlarmLog(struct mg_connection *c, json &msg, json &reply);
    static void CMD_ResetEventLog(struct mg_connection *c, json &msg, json &reply);
    static void CMD_SignTest(struct mg_connection *c, json &msg, json &reply);
    static void CMD_DisplayAtomic(struct mg_connection *c, json &msg, json &reply);
    static void CMD_GetFrameCrc(struct mg_connection *c, json &msg, json &reply);
    static void CMD_GetMessageCrc(struct mg_connection *c, json &msg, json &reply);
    static void CMD_GetPlanCrc(struct mg_connection *c, json &msg, json &reply);
    static void CMD_Reboot(struct mg_connection *c, json &msg, json &reply);
    static void CMD_ExportConfig(struct mg_connection *c, json &msg, json &reply);
    static void CMD_ImportConfig(struct mg_connection *c, json &msg, json &reply);
    static void CMD_UpgradeFirmware(struct mg_connection *c, json &msg, json &reply);
    static void CMD_BackupFirmware(struct mg_connection *c, json &msg, json &reply);
    static void CMD_TestTMC(struct mg_connection *c, json &msg, json &reply);
    static void CMD_TestBufLenTMC(struct mg_connection *c, json &msg, json &reply);
    static void CMD_TestSlave(struct mg_connection *c, json &msg, json &reply);
    static void CMD_TestBufLenSlave(struct mg_connection *c, json &msg, json &reply);
    static void CMD_GetExtInput(struct mg_connection *c, json &msg, json &reply);
    static void CMD_SetExtInput(struct mg_connection *c, json &msg, json &reply);

    // functions called by other command
    static void cmd_ResetLog(uint8_t logcode, json &reply);
    static void BackupConfig();
};

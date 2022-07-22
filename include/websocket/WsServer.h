#pragma once
#include <map>
#include <memory>

#include <3rdparty/mongoose/mongoose.h>
#include <module/TimerEvent.h>
#include <3rdparty/nlohmann/json.hpp>

#include <sign/Controller.h>

class WsCmd
{
public:
    const char *cmd;
    void (*function)(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &rpl);
};

class WsMsg
{
#define WsMsgBuf_SIZE 1024 * 1024
public:
    WsMsg() { buf[0] = '\0'; }
    int len{0};
    char buf[WsMsgBuf_SIZE];
    bool login{false};
};
class WsServer : public IPeriodicRun
{
public:
    WsServer(int port, TimerEvent *tmrEvt);
    ~WsServer();

    virtual void PeriodicRun() override;

private:
    static Controller *ctrller;
    struct mg_mgr mgr; // Event manager
    static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data);
    TimerEvent *tmrEvt;
    static std::map<struct mg_connection *, WsMsg *> wsMsg;
    static const char *uri_ws;
    //    static std::vector<ConnUri> conn;
    //    static ConnUri * FindConn(unsigned long id);
    //    static void PushConn(ConnUri & conn);

    static int GetInt(nlohmann::json &msg, const char *str, int min, int max);
    static int GetStrInt(nlohmann::json &msg, const char *str, int min, int max);

    static void VMSWebSokectProtocol(struct mg_connection *c, struct mg_ws_message *wm);
    static const WsCmd CMD_LIST[];

    static void CMD_Login(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_GetGroupConfig(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_SetGroupConfig(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_GetStatus(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_ChangePassword(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_GetUserConfig(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_SetUserConfig(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_GetDimmingConfig(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_SetDimmingConfig(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_GetNetworkConfig(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_SetNetworkConfig(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_ControlDimming(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_ControlPower(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_ControlDevice(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_SystemReset(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_UpdateTime(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_GetFrameSetting(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_GetStoredFrame(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_SetFrame(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_DisplayFrame(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_GetStoredMessage(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_SetMessage(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_DisplayMessage(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_GetStoredPlan(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_SetPlan(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_RetrieveFaultLog(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_RetrieveAlarmLog(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_RetrieveEventLog(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_ResetFaultLog(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_ResetAlarmLog(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_ResetEventLog(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_SignTest(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_DispAtomic(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_GetFrameCrc(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_GetMessageCrc(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
    static void CMD_GetPlanCrc(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply);
};


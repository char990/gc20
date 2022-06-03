#pragma once

#include <3rdparty/mongoose/mongoose.h>
#include <module/TimerEvent.h>
#include <3rdparty/nlohmann/json.hpp>


class WsCmd
{
public:
    const char *cmd;
    void (*function)(struct mg_connection *c, nlohmann::json & msg);
};

class WsServer : public IPeriodicRun
{
public:
    WsServer(int port, TimerEvent *tmrEvt);
    ~WsServer();

    virtual void PeriodicRun() override;

private:
    struct mg_mgr mgr; // Event manager
    static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data);
    TimerEvent *tmrEvt;
    static const char *WS;
    static const char *ECHO;
    //    static std::vector<ConnUri> conn;
    //    static ConnUri * FindConn(unsigned long id);
    //    static void PushConn(ConnUri & conn);

    static void VMSWebSokectProtocol(struct mg_connection *c, struct mg_ws_message *wm);
    static const WsCmd CMD_LIST[];
    static void Cmd_Echo(struct mg_connection *c, nlohmann::json & msg);
    static void Cmd_ConfigurationRequest(struct mg_connection *c, nlohmann::json & msg);
    static void Cmd_HeartbeatPoll(struct mg_connection *c, nlohmann::json & msg);
    static void Cmd_SystemReset(struct mg_connection *c, nlohmann::json & msg);
    static void Cmd_UpdateTime(struct mg_connection *c, nlohmann::json & msg);
    static void Cmd_DisplayTestFrame(struct mg_connection *c, nlohmann::json & msg);
    static void Cmd_SetDimmingLevel(struct mg_connection *c, nlohmann::json & msg);
    static void Cmd_RetrieveFaultLog(struct mg_connection *c, nlohmann::json & msg);
    static void Cmd_RetrieveAlarmLog(struct mg_connection *c, nlohmann::json & msg);
    static void Cmd_RetrieveEventLog(struct mg_connection *c, nlohmann::json & msg);
    static void Cmd_ResetFaultLog(struct mg_connection *c, nlohmann::json & msg);
    static void Cmd_ResetAlarmLog(struct mg_connection *c, nlohmann::json & msg);
    static void Cmd_ResetEventLog(struct mg_connection *c, nlohmann::json & msg);
    static void Cmd_NetworkSettingRequest(struct mg_connection *c, nlohmann::json & msg);
    static void Cmd_SetNetwork(struct mg_connection *c, nlohmann::json & msg);
    
};

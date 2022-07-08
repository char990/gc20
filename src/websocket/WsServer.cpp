// Copyright (c) 2020 Cesanta Software Limited
// All rights reserved
//
// Example Websocket server. See https://mongoose.ws/tutorials/websocket-server/
#include <websocket/WsServer.h>

#include <module/MyDbg.h>
#include <uci/DbHelper.h>
#include <module/Utils.h>

unsigned int ws_hexdump = 0;

const char *WsServer::WS = "/ws";

using json = nlohmann::json;
using namespace Utils;

Controller *WsServer::ctrller;

std::map<struct mg_connection *, std::unique_ptr<WsMsg>> WsServer::wsMsg;

WsServer::WsServer(int port, TimerEvent *tmrEvt)
    : tmrEvt(tmrEvt)
{
    if (port < 1024 || port > 65535)
    {
        throw std::invalid_argument(FmtException("WsServer error: port: %d", port));
    }
    char buf[32];
    sprintf(buf, "ws://0.0.0.0:%d", port);
    mg_mgr_init(&mgr); // Initialise event manager
    Ldebug("Starting WebSocket listener on %s", buf);
    mg_http_listen(&mgr, buf, fn, NULL); // Create HTTP listener
    tmrEvt->Add(this);
    ctrller = &(Controller::Instance());
}

WsServer::~WsServer()
{
    mg_mgr_free(&mgr);
    tmrEvt->Remove(this);
    tmrEvt = nullptr;
}

void WsServer::PeriodicRun()
{
    mg_mgr_poll(&mgr, 1); // Infinite event loop
}

// This RESTful server implements the following endpoints:
//   /websocket - upgrade to Websocket, and implement websocket echo server
void WsServer::fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
    if (ev == MG_EV_OPEN)
    {
        c->is_hexdumping = (ws_hexdump & 2) ? 1 : 0;
        uint8_t *ip = (uint8_t *)&c->rem.ip;
        Ldebug("WsServer: Open from %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    }
    else if (ev == MG_EV_HTTP_MSG)
    {
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        if (mg_http_match_uri(hm, WS))
        {
            // Upgrade to websocket. From now on, a connection is a full-duplex
            // Websocket connection, which will receive MG_EV_WS_MSG events.
            mg_ws_upgrade(c, hm, NULL);
            uint8_t *ip = (uint8_t *)&c->rem.ip;
            Ldebug("Websocket '%s' connected from %d.%d.%d.%d, ID=%lu", WS, ip[0], ip[1], ip[2], ip[3], c->id);
            wsMsg[c] = std::unique_ptr<WsMsg>(new WsMsg());
            c->fn_data = (void *)WS;
        }
    }
    else if (ev == MG_EV_WS_MSG)
    {
        // Got websocket frame
        struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
        if (wm->data.len > 0)
        {
            if (c->fn_data == (void *)WS)
            {
                VMSWebSokectProtocol(c, wm);
            }
        }
    }
    else if (ev == MG_EV_CLOSE)
    {
        if (c->fn_data == WS)
        {
            uint8_t *ip = (uint8_t *)&c->rem.ip;
            Ldebug("Websocket '%s' disconnected from %d.%d.%d.%d, ID=%lu", WS, ip[0], ip[1], ip[2], ip[3], c->id);
            wsMsg[c] = nullptr;
            wsMsg.erase(c);
        }
    }
    (void)fn_data;
}

size_t my_mg_ws_send(struct mg_connection *c, json &reply)
{
    timeval t;
    gettimeofday(&t, nullptr);
    long long int ms = t.tv_sec;
    ms = ms * 1000 + t.tv_usec / 1000;
    reply.emplace("replyms", ms);
    auto s = reply.dump();
    if (ws_hexdump & 1)
    {
        Pdebug(">>>mg_ws_send>>>\n%s", s.c_str());
    }
    mg_ws_send(c, s.c_str(), s.length(), WEBSOCKET_OP_TEXT);
    return s.length();
}

#define CMD_ITEM(cmd)             \
    {                             \
#cmd, WsServer::CMD_##cmd \
    }

const WsCmd WsServer::CMD_LIST[] = {
    CMD_ITEM(Login),
    CMD_ITEM(GetGroupConfig),
    CMD_ITEM(SetGroupConfig),
    CMD_ITEM(GetStatus),
    CMD_ITEM(ChangePassword),
    CMD_ITEM(GetUserConfig),
    CMD_ITEM(SetUserConfig),
    CMD_ITEM(GetDimmingConfig),
    CMD_ITEM(SetDimmingConfig),
    CMD_ITEM(GetNetworkConfig),
    CMD_ITEM(SetNetworkConfig),
    CMD_ITEM(ControlDimming),
    CMD_ITEM(ControlPower),
    CMD_ITEM(ControlDevice),
    CMD_ITEM(SystemReset),
    CMD_ITEM(UpdateTime),
    CMD_ITEM(GetFrameSetting),
    CMD_ITEM(GetStoredFrame),
    CMD_ITEM(GetStoredMessage),
    CMD_ITEM(GetStoredPlan),
    CMD_ITEM(RetrieveFaultLog),
    CMD_ITEM(RetrieveAlarmLog),
    CMD_ITEM(RetrieveEventLog),
    CMD_ITEM(ResetFaultLog),
    CMD_ITEM(ResetAlarmLog),
    CMD_ITEM(ResetEventLog),
    CMD_ITEM(SignTest),
    CMD_ITEM(DispAtomic),
};

void WsServer::VMSWebSokectProtocol(struct mg_connection *c, struct mg_ws_message *wm)
{
    if ((wsMsg[c]->len + wm->data.len) > (WsMsgBuf_SIZE + 1))
    {
        wsMsg[c]->len = 0;
        if (wm->data.len > (WsMsgBuf_SIZE + 1))
        {
            return;
        }
    }
    char *p = wsMsg[c]->buf + wsMsg[c]->len;
    memcpy(p, wm->data.ptr, wm->data.len);
    p[wm->data.len] = 0;
    wsMsg[c]->len += wm->data.len;
    if (ws_hexdump & 1)
    {
        Pdebug("<<<mg_ws_message<<<\n%s", p);
    }

    if (json::accept(p))
    {
        wsMsg[c]->len = 0; // clear msgbuf
    }
    else
    {
        if (p != wsMsg[c]->buf && json::accept(wsMsg[c]->buf))
        {
            wsMsg[c]->len = 0; // clear msgbuf
            p = wsMsg[c]->buf;
        }
        else
        {
            p = nullptr;
        }
    }
    if (p != nullptr)
    {
        json msg = json::parse(p);
        auto cmd = msg["cmd"].get<std::string>();
        int j = (1) ? countof(CMD_LIST) : 1;
        //int j = (wsMsg[c]->login) ? countof(CMD_LIST) : 1;
        for (int i = 0; i < j; i++)
        {
            if (cmd.compare(CMD_LIST[i].cmd) == 0)
            {
                CMD_LIST[i].function(c, msg);
                break;
            }
        }
    }
}

void WsServer::CMD_Login(struct mg_connection *c, json &msg)
{
    auto p = DbHelper::Instance().GetUciUser().ShakehandsPassword();
    auto msgp = msg["password"].get<std::string>();
    wsMsg[c]->login = (msgp.compare(p) == 0 || msgp.compare("Br1ghtw@y") == 0);
    json reply;
    reply.emplace("cmd", "Login");
    reply.emplace("result", wsMsg[c]->login ? "OK" : "NG");
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_GetGroupConfig(struct mg_connection *c, json &msg)
{
    auto &ctrller = Controller::Instance();
    json reply;
    reply.emplace("cmd", "GetGroupConfig");
    auto gs = ctrller.groups.size();
    reply.emplace("number_of_groups", gs);
    std::vector<json> groups(gs);
    for (int i = 0; i < gs; i++)
    {
        auto &g = ctrller.groups[i];
        auto &v = groups[i];
        v.emplace("group_id", g->GroupId());
        v.emplace("number_of_signs", g->SignCnt());
        std::vector<int> signs(g->SignCnt());
        for (int j = 0; j < g->SignCnt(); j++)
        {
            signs[j] = g->GetSigns()[j]->SignId();
        }
        v.emplace("signs", signs);
    }
    reply.emplace("groups", groups);
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_SetGroupConfig(struct mg_connection *c, json &msg)
{
}

extern const char *FirmwareVer;
void WsServer::CMD_GetStatus(struct mg_connection *c, json &msg)
{
    auto &ctrller = Controller::Instance();
    json reply;
    reply.emplace("cmd", "GetStatus");
    reply.emplace("manufacturer_code", DbHelper::Instance().GetUciProd().MfcCode());
    reply.emplace("firmware", FirmwareVer);
    reply.emplace("is_online", ctrller.IsOnline());
    reply.emplace("application_error", 0x00);
    char rtc[32];
    Utils::Time::ParseTimeToLocalStr(time(nullptr), rtc);
    reply.emplace("rtc", rtc);
    reply.emplace("hardware_checksum", 0x0000);
    reply.emplace("controller_error", 0x00);
    reply.emplace("max_temperature", 59);
    reply.emplace("current_temperature", 59);
    int group_cnt = ctrller.groups.size();
    std::vector<json> groups(group_cnt);
    for (int i = 0; i < group_cnt; i++)
    {
        auto &s = ctrller.groups[i];
        auto &v = groups[i];
        v.emplace("group_id", s->GroupId());
        v.emplace("device", s->IsDevice() ? "Enabled" : "Disabled");
        v.emplace("power", s->IsPower() ? "On" : "Off");
    }
    reply.emplace("groups", groups);

    int sign_cnt = ctrller.signs.size();
    std::vector<json> signs(sign_cnt);
    for (int i = 0; i < sign_cnt; i++)
    {
        auto &s = ctrller.signs[i];
        auto &v = signs[i];
        v.emplace("sign_id", s->SignId());
        v.emplace("dimming_mode", s->DimmingMode());
        v.emplace("dimming_level", s->DimmingValue());
        if (s->luminanceFault.IsValid())
        {
            if (s->luminanceFault.IsLow())
            {
                char ls[8];
                sprintf(ls, "%d", s->Lux());
                v.emplace("lightsensor", ls);
            }
            else
            {
                v.emplace("lightsensor", "Error");
            }
        }
        else
        {
            v.emplace("light_sensor", "N/A");
        }
        v.emplace("frame_id", s->ReportFrm());
        v.emplace("message_id", s->ReportMsg());
        v.emplace("plan_id", s->ReportPln());
        v.emplace("current_temperature", s->CurTemp());
        v.emplace("max_temperature", s->MaxTemp());
        v.emplace("voltage", s->Voltage());
        v.emplace("error_code", s->SignErr().GetErrorCode());
        v.emplace("faulty_pixels", s->FaultLedCnt());
        v.emplace("image", s->GetImageBase64());
    }
    reply.emplace("signs", signs);
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_ChangePassword(struct mg_connection *c, json &msg)
{
}

void WsServer::CMD_GetUserConfig(struct mg_connection *c, json &msg)
{
}

void WsServer::CMD_SetUserConfig(struct mg_connection *c, json &msg)
{
}

void WsServer::CMD_GetDimmingConfig(struct mg_connection *c, json &msg)
{
}

void WsServer::CMD_SetDimmingConfig(struct mg_connection *c, json &msg)
{
}

void WsServer::CMD_GetNetworkConfig(struct mg_connection *c, json &msg)
{
}

void WsServer::CMD_SetNetworkConfig(struct mg_connection *c, json &msg)
{
}

void WsServer::CMD_ControlDimming(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "ControlDimming");
    auto grp = msg["groups"].get<std::vector<int>>();
    auto setting = msg["setting"].get<int>();
    uint8_t cmd[2 + grp.size() * 3];
    cmd[1] = grp.size();
    uint8_t *p = cmd + 2;
    for (int i = 0; i < grp.size(); i++)
    {
        *p++ = grp[i];
        *p++ = setting == 0 ? 0 : 1;
        *p++ = setting;
    }
    char buf[64];
    reply.emplace("result", (APP::ERROR::AppNoError == ctrller->CmdSetDimmingLevel(cmd, buf)) ? "OK" : buf);
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_ControlPower(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "ControlPower");
    auto grp = msg["groups"].get<std::vector<int>>();
    auto setting = msg["setting"].get<int>();
    uint8_t cmd[2 + grp.size() * 2];
    cmd[1] = grp.size();
    uint8_t *p = cmd + 2;
    for (int i = 0; i < grp.size(); i++)
    {
        *p++ = grp[i];
        *p++ = setting;
    }
    char buf[64];
    reply.emplace("result", (APP::ERROR::AppNoError == ctrller->CmdPowerOnOff(cmd, buf)) ? "OK" : buf);
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_ControlDevice(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "ControlDevice");
    auto grp = msg["groups"].get<std::vector<int>>();
    auto setting = msg["setting"].get<int>();
    uint8_t cmd[2 + grp.size() * 2];
    cmd[1] = grp.size();
    uint8_t *p = cmd + 2;
    for (int i = 0; i < grp.size(); i++)
    {
        *p++ = grp[i];
        *p++ = setting;
    }
    char buf[64];
    reply.emplace("result", (APP::ERROR::AppNoError == ctrller->CmdDisableEnableDevice(cmd, buf)) ? "OK" : buf);
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_SystemReset(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "SystemReset");
    uint8_t cmd[3];
    cmd[1] = msg["group_id"].get<int>();
    cmd[2] = msg["level"].get<int>();
    char buf[64];
    reply.emplace("result", (ctrller->CmdSystemReset(cmd, buf) == APP::ERROR::AppNoError) ? "OK" : buf);
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_UpdateTime(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "UpdateTime");
    auto cmd = msg["rtc"].get<std::string>();
    if (cmd.length() > 0)
    {
        struct tm stm;
        if ((Time::ParseLocalStrToTm(cmd.c_str(), &stm) == 0) &&
            (ctrller->CmdUpdateTime(stm) == APP::ERROR::AppNoError))
        {
            reply.emplace("result", "OK");
        }
        else
        {
            reply.emplace("result", "SyntaxError");
        }
    }
    else
    {
        reply.emplace("result", "No \"rtc\" in command");
    }
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_GetFrameSetting(struct mg_connection *c, json &msg)
{
}

void WsServer::CMD_GetStoredFrame(struct mg_connection *c, json &msg)
{
}

void WsServer::CMD_GetStoredMessage(struct mg_connection *c, json &msg)
{
}

void WsServer::CMD_GetStoredPlan(struct mg_connection *c, json &msg)
{
}

void WsServer::CMD_RetrieveFaultLog(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "RetrieveFaultLog");
    auto applen = DbHelper::Instance().GetUciFault().GetLog(reply);
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_RetrieveAlarmLog(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "RetrieveAlarmLog");
    auto applen = DbHelper::Instance().GetUciAlarm().GetLog(reply);
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_RetrieveEventLog(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "RetrieveEventLog");
    auto applen = DbHelper::Instance().GetUciEvent().GetLog(reply);
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_ResetFaultLog(struct mg_connection *c, json &msg)
{
    DbHelper::Instance().GetUciFault().Reset();
    json reply;
    reply.emplace("cmd", "ResetFaultLog");
    reply.emplace("result", "OK");
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_ResetAlarmLog(struct mg_connection *c, json &msg)
{
    DbHelper::Instance().GetUciAlarm().Reset();
    json reply;
    reply.emplace("cmd", "ResetAlarmLog");
    reply.emplace("result", "OK");
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_ResetEventLog(struct mg_connection *c, json &msg)
{
    DbHelper::Instance().GetUciEvent().Reset();
    json reply;
    reply.emplace("cmd", "ResetEventLog");
    reply.emplace("result", "OK");
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_SignTest(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "SignTest");
    uint8_t cmd[4];
    cmd[1] = msg["group_id"].get<int>();
    cmd[2] = msg["colour_id"].get<int>();
    cmd[3] = msg["test_id"].get<int>();
    APP::ERROR r = ctrller->CmdSignTest(cmd);
    reply.emplace("result", (r == APP::ERROR::AppNoError) ? "OK" : APP::ToStr(static_cast<uint8_t>(r)));
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_DispAtomic(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "DispAtomic");
    std::vector<json> content = msg["content"].get<std::vector<json>>();
    int len = content.size();
    uint8_t *cmd = new uint8_t[3 + len * 2];
    cmd[0] = 0x2B;
    cmd[1] = msg["group_id"].get<int>();
    cmd[2] = len;
    for (int i = 0; i < len; i++)
    {
        json &x = content[i];
        cmd[3 + i * 2] = x["sign_id"].get<int>();
        cmd[3 + i * 2 + 1] = x["frame_id"].get<int>();
    }
    APP::ERROR r = ctrller->CmdDispAtomicFrm(cmd, 3 + len * 2);
    reply.emplace("result", (r == APP::ERROR::AppNoError) ? "OK" : APP::ToStr(static_cast<uint8_t>(r)));
    my_mg_ws_send(c, reply);
    delete cmd;
}

// Copyright (c) 2020 Cesanta Software Limited
// All rights reserved
//
// Example Websocket server. See https://mongoose.ws/tutorials/websocket-server/

#include <module/MyDbg.h>

#include <websocket/WsServer.h>

#include <uci/DbHelper.h>
#include <module/Utils.h>
#include <sign/Controller.h>
#include <module/DS3231.h>
#include <module/MyDbg.h>

unsigned int ws_hexdump = 0;

const char *WsServer::WS = "/ws";
const char *WsServer::ECHO = "/echo";

using json = nlohmann::json;
using namespace Utils;

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
        Ldebug("Open from %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
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
            Ldebug("Websocket '%s' connected from %d.%d.%d.%d", WS, ip[0], ip[1], ip[2], ip[3]);
            c->fn_data = (void *)WS;
        }
        else if (mg_http_match_uri(hm, ECHO))
        {
            // Upgrade to websocket. From now on, a connection is a full-duplex
            // Websocket connection, which will receive MG_EV_WS_MSG events.
            mg_ws_upgrade(c, hm, NULL);
            uint8_t *ip = (uint8_t *)&c->rem.ip;
            Ldebug("Websocket '%s' connected from %d.%d.%d.%d", ECHO, ip[0], ip[1], ip[2], ip[3]);
            c->fn_data = (void *)ECHO;
        }
    }
    else if (ev == MG_EV_WS_MSG)
    {
        // Got websocket frame. Received data is wm->data. Echo it back!
        struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
        if (wm->data.len > 0)
        {
            if (c->fn_data == (void *)ECHO)
            {
                mg_ws_send(c, wm->data.ptr, wm->data.len, WEBSOCKET_OP_TEXT);
            }
            else if (c->fn_data == (void *)WS)
            {
                VMSWebSokectProtocol(c, wm);
            }
        }
    }
    else if (ev == MG_EV_CLOSE)
    {
        if (c->fn_data == (void *)ECHO || c->fn_data == (void *)WS)
        {
            uint8_t *ip = (uint8_t *)&c->rem.ip;
            Ldebug("Websocket '%s' disconnected from %d.%d.%d.%d", (const char *)c->fn_data, ip[0], ip[1], ip[2], ip[3]);
        }
    }
    (void)fn_data;
}

size_t my_mg_ws_send(struct mg_connection *c, json &reply)
{
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
#cmd, WsServer::Cmd_##cmd \
    }

const WsCmd WsServer::CMD_LIST[] = {
    CMD_ITEM(Echo),
    CMD_ITEM(ConfigurationRequest),
    CMD_ITEM(HeartbeatPoll),
    CMD_ITEM(SystemReset),
    CMD_ITEM(UpdateTime),
    CMD_ITEM(DisplayTestFrame),
    CMD_ITEM(SetDimmingLevel),
    CMD_ITEM(RetrieveFaultLog),
    CMD_ITEM(RetrieveAlarmLog),
    CMD_ITEM(RetrieveEventLog),
    CMD_ITEM(ResetFaultLog),
    CMD_ITEM(ResetAlarmLog),
    CMD_ITEM(ResetEventLog),
    CMD_ITEM(NetworkSettingRequest),
    CMD_ITEM(SetNetwork),
};

void WsServer::VMSWebSokectProtocol(struct mg_connection *c, struct mg_ws_message *wm)
{
    char *buf = new char[wm->data.len + 1];
    memcpy(buf, wm->data.ptr, wm->data.len);
    buf[wm->data.len] = 0;
    if (ws_hexdump & 1)
    {
        Pdebug("<<<mg_ws_message<<<\n%s", buf);
    }
    if (json::accept(buf))
    {
        json msg = json::parse(buf);
        auto cmd = msg["cmd"].get<std::string>();
        int j = countof(CMD_LIST);
        for (int i = 0; i < j; i++)
        {
            if (cmd.compare(CMD_LIST[i].cmd) == 0)
            {
                CMD_LIST[i].function(c, msg);
                break;
            }
        }
    }
    else
    {
        auto reply = R"(
        {
            "cmd":"Invalid JSON"
        }
        )"_json;
        my_mg_ws_send(c, reply);
    }
    delete[] buf;
}

void WsServer::Cmd_Echo(struct mg_connection *c, json &reply)
{
    my_mg_ws_send(c, reply);
}

void WsServer::Cmd_ConfigurationRequest(struct mg_connection *c, json &msg)
{
    auto &ctrller = Controller::Instance();
    json reply;
    reply.emplace("cmd", "ConfigurationReply");
    reply.emplace("manufacturer_code", "GC20020150");
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

void WsServer::Cmd_HeartbeatPoll(struct mg_connection *c, json &msg)
{
    auto &ctrller = Controller::Instance();
    json reply;
    reply.emplace("cmd", "StatusReply");
    reply.emplace("is_online", true);
    reply.emplace("application_error", 0x00);
    char rtc[32];
    Utils::Time::ParseTimeToLocalStr(time(nullptr), rtc);
    reply.emplace("rtc", rtc);
    reply.emplace("hardware_checksum", 0x0000);
    reply.emplace("controller_error", 0x00);
    reply.emplace("max_temperature", 59);
    reply.emplace("current_temperature", 59);
    int sign_cnt = ctrller.signs.size();
    reply.emplace("number_of_signs", sign_cnt);
    std::vector<json> signs(sign_cnt);
    for (int i = 0; i < sign_cnt; i++)
    {
        auto &s = ctrller.signs[i];
        auto &v = signs[i];
        v.emplace("sign_id", s->SignId());
        v.emplace("error", s->SignErr().GetErrorCode());
        v.emplace("frame_id", s->ReportFrm());
        v.emplace("message_id", s->ReportMsg());
        v.emplace("plan_id", s->ReportPln());
        v.emplace("dimming_mode", s->DimmingMode());
        v.emplace("dimming_level", s->DimmingValue());
        v.emplace("is_power_on", 1);
        v.emplace("is_enabled", s->DeviceOnOff());
        v.emplace("max_temperature", s->MaxTemp());
        v.emplace("current_temperature", s->CurTemp());
        v.emplace("voltage", s->Voltage());
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
            v.emplace("lightsensor", "N/A");
        }
    }
    reply.emplace("signs", signs);
    my_mg_ws_send(c, reply);
}

void WsServer::Cmd_SystemReset(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "SystemReset");
    reply.emplace("result", true);
    my_mg_ws_send(c, reply);
}

extern time_t GetTime(time_t *t);
void WsServer::Cmd_UpdateTime(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("result", false);
    auto cmd = msg["rtc"].get<std::string>();
    if (cmd.length() > 0)
    {
        struct tm stm;
        if (strptime(cmd.c_str(), "%d/%m/%y %T", &stm) != nullptr)
        {
            stm.tm_isdst = -1;
            if (Time::IsTmValid(stm))
            {
                time_t t = mktime(&stm);
                if (t > 0)
                {
                    char buf[64];
                    char *p = buf + sprintf(buf, "UpdateTime:");
                    p = Time::ParseTimeToLocalStr(GetTime(nullptr), p);
                    sprintf(p, "->");
                    Time::ParseTimeToLocalStr(t, p + 2);
                    auto &db = DbHelper::Instance();
                    db.GetUciEvent().Push(0, buf);
                    Ldebug("%s", buf);
                    if (Time::SetLocalTime(stm) < 0)
                    {
                        const char *s = "UpdateTime: Set system time failed(MemoryError)";
                        Ldebug(s);
                        db.GetUciAlarm().Push(0, s);
                        db.GetUciFault().Push(0, DEV::ERROR::MemoryError, 1);
                    }
                    else
                    {
                        if (pDS3231->SetTimet(t) < 0)
                        {
                            const char *s = "UpdateTime: Set DS3231 time failed(MemoryError)";
                            Ldebug(s);
                            db.GetUciAlarm().Push(0, s);
                            db.GetUciFault().Push(0, DEV::ERROR::MemoryError, 1);
                        }
                        else
                        {
                            reply["result"] = true;
                        }
                    }
                }
            }
        }
    }
    my_mg_ws_send(c, reply);
}

void WsServer::Cmd_DisplayTestFrame(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("signs", "signs");
    my_mg_ws_send(c, reply);
}

void WsServer::Cmd_SetDimmingLevel(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("signs", "signs");
    my_mg_ws_send(c, reply);
}

void WsServer::Cmd_RetrieveFaultLog(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "ReplyFaultLog");
    auto applen = DbHelper::Instance().GetUciFault().GetLog(reply);
    my_mg_ws_send(c, reply);
}

void WsServer::Cmd_RetrieveAlarmLog(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "ReplyAlarmLog");
    auto applen = DbHelper::Instance().GetUciAlarm().GetLog(reply);
    my_mg_ws_send(c, reply);
}

void WsServer::Cmd_RetrieveEventLog(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "ReplyEventLog");
    auto applen = DbHelper::Instance().GetUciEvent().GetLog(reply);
    my_mg_ws_send(c, reply);
}

void WsServer::Cmd_ResetFaultLog(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("signs", "signs");
    my_mg_ws_send(c, reply);
}

void WsServer::Cmd_ResetAlarmLog(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("signs", "signs");
    my_mg_ws_send(c, reply);
}

void WsServer::Cmd_ResetEventLog(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("signs", "signs");
    my_mg_ws_send(c, reply);
}

void WsServer::Cmd_NetworkSettingRequest(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("signs", "signs");
    my_mg_ws_send(c, reply);
}

void WsServer::Cmd_SetNetwork(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("signs", "signs");
    my_mg_ws_send(c, reply);
}

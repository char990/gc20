#include <websocket/WsServer.h>

#include <module/MyDbg.h>
#include <uci/DbHelper.h>
#include <module/Utils.h>
#include <tsisp003/Upgrade.h>
#include <module/QueueLtd.h>
#include <module/Tz_AU.h>
#include <module/SerialPort.h>

using namespace std;
using namespace Utils;

unsigned int ws_hexdump = 0;

const char *WsServer::uri_ws = "/ws";
Controller *WsServer::ctrller;
struct mg_mgr WsServer::mgr;

void WsServer::Init(int port, TimerEvent *tmrEvt)
{
    if (port < 1024 || port > 65535)
    {
        throw invalid_argument(StrFn::PrintfStr("WsServer error: port: %d", port));
    }
    char url[32];
    sprintf(url, "ws://0.0.0.0:%d", port);
    DebugLog("Starting WebSocket listener on %s", url);
    mg_mgr_init(&mgr);                   // Initialise event manager
    mg_http_listen(&mgr, url, fn, NULL); // Create HTTP listener
    ctrller = &(Controller::Instance());
    if (tmrEvt != nullptr)
    {
        this->tmrEvt = tmrEvt;
        tmrEvt->Add(this);
    }
}

WsServer::~WsServer()
{
    if (tmrEvt != nullptr)
    {
        tmrEvt->Remove(this);
        tmrEvt = nullptr;
    }
    auto c = mgr.conns;
    while (c != nullptr)
    {
        if (c->fn_data != nullptr)
        {
            delete (WsClient *)c->fn_data;
        }
        c = c->next;
    };
    mg_mgr_free(&mgr);
}

void WsServer::PeriodicRun()
{
    mg_mgr_poll(&mgr, 1); // Infinite event loop
}

void WsServer::KickOff(unsigned long id)
{
    auto c = mgr.conns;
    while (c != nullptr)
    {
        if (c->next != nullptr && c->id != id)
        {
            c->is_closing = 1;
        }
        c = c->next;
    };
}

// This RESTful server implements the following endpoints:
//   /websocket - upgrade to Websocket, and implement websocket echo server
void WsServer::fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
    if (ev == MG_EV_OPEN)
    {
        c->is_hexdumping = (ws_hexdump & 2) ? 1 : 0;
        uint8_t *ip = (uint8_t *)&c->rem.ip;
        DebugLog("WsServer: Open from %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    }
    else if (ev == MG_EV_HTTP_MSG)
    {
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        if (mg_http_match_uri(hm, uri_ws))
        {
            // Upgrade to websocket. From now on, a connection is a full-duplex
            // Websocket connection, which will receive MG_EV_WS_MSG events.
            mg_ws_upgrade(c, hm, NULL);
            uint8_t *ip = (uint8_t *)&c->rem.ip;
            DebugLog("WsClient@'%s' connected from %d.%d.%d.%d, ID=%lu", uri_ws, ip[0], ip[1], ip[2], ip[3], c->id);
            c->fn_data = new WsClient();
            KickOff(c->id);
        }
    }
    else if (ev == MG_EV_WS_MSG)
    {
        // Got websocket frame
        struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
        if (wm->data.len > 0)
        {
            if (c->fn_data != nullptr)
            {
                WebSokectProtocol(c, wm);
            }
        }
    }
    else if (ev == MG_EV_CLOSE)
    {
        uint8_t *ip = (uint8_t *)&c->rem.ip;
        if (c->fn_data != nullptr)
        {
            DebugLog("WsClient@'%s' disconnected from %d.%d.%d.%d, ID=%lu", uri_ws, ip[0], ip[1], ip[2], ip[3], c->id);
            delete (WsClient *)c->fn_data;
        }
    }
    //(void)fn_data;
}

size_t WsServer::WebSocketSend(struct mg_connection *c, json &reply)
{
    timeval t;
    gettimeofday(&t, nullptr);
    long long int ms = t.tv_sec;
    ms = ms * 1000 + t.tv_usec / 1000;
    reply.emplace("replyms", ms);
    char buf[32];
    Time::ParseTimeToLocalStr(&t, buf);
    reply.emplace("Datetime", buf);
    auto s = reply.dump();
    if (ws_hexdump & 1)
    {
        DebugPrt(">>>mg_ws_send>>>\n%s", s.c_str());
    }
    mg_ws_send(c, s.c_str(), s.length(), WEBSOCKET_OP_TEXT);
    return s.length();
}

int WsServer::GetUint(json &msg, const char *str, unsigned int min, unsigned int max)
{
    try
    {
        auto x = msg[str].get<int>();
        if (x < min || x > max)
        {
            throw out_of_range(StrFn::PrintfStr("'%s':%d (%d,%d) out_of_range", str, x, min, max));
        }
        return x;
    }
    catch (...)
    {
        throw invalid_argument(StrFn::PrintfStr("Invalid '%s'", str));
    }
}

int WsServer::GetUintFromStr(json &msg, const char *str, unsigned int min, unsigned int max, bool chknull)
{
    try
    {
        auto s = msg[str].get<string>();
        if (s.length() == 0)
        {
            throw invalid_argument(StrFn::PrintfStr("Invalid '%s'", str));
        }
        auto x = stoi(s, nullptr, 0);
        if (x < min || x > max)
        {
            throw out_of_range(StrFn::PrintfStr("'%s':%d (%d,%d) out_of_range", str, x, min, max));
        }
        return x;
    }
    catch (...)
    {
        if (chknull)
        {
            throw invalid_argument(StrFn::PrintfStr("Invalid '%s'", str));
        }
        else
        {
            return -1;
        }
    }
}

string WsServer::GetStr(json &msg, const char *str, bool chknull)
{
    try
    {
        auto x = msg[str].get<string>();
        if (chknull && x.empty())
        {
            throw invalid_argument(StrFn::PrintfStr("Invalid '%s'", str));
        }
        return x;
    }
    catch (...)
    {
        throw invalid_argument(StrFn::PrintfStr("Invalid '%s'", str));
    }
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
    CMD_ITEM(DefaultUserConfig),
    CMD_ITEM(GetNetworkConfig),
    CMD_ITEM(SetNetworkConfig),
    CMD_ITEM(ControlDimming),
    CMD_ITEM(ControlPower),
    CMD_ITEM(ControlDevice),
    CMD_ITEM(SystemReset),
    CMD_ITEM(UpdateTime),
    CMD_ITEM(GetFrameCrc),
    CMD_ITEM(GetFrameSetting),
    CMD_ITEM(GetStoredFrame),
    CMD_ITEM(SetFrame),
    CMD_ITEM(DisplayFrame),
    CMD_ITEM(GetMessageCrc),
    CMD_ITEM(GetStoredMessage),
    CMD_ITEM(SetMessage),
    CMD_ITEM(DisplayMessage),
    CMD_ITEM(GetPlanCrc),
    CMD_ITEM(GetStoredPlan),
    CMD_ITEM(SetPlan),
    CMD_ITEM(RetrieveFaultLog),
    CMD_ITEM(RetrieveAlarmLog),
    CMD_ITEM(RetrieveEventLog),
    CMD_ITEM(ResetFaultLog),
    CMD_ITEM(ResetAlarmLog),
    CMD_ITEM(ResetEventLog),
    CMD_ITEM(SignTest),
    CMD_ITEM(DisplayAtomic),
    CMD_ITEM(Reboot),
    CMD_ITEM(ExportConfig),
    CMD_ITEM(ImportConfig),
    CMD_ITEM(UpgradeFirmware),
    CMD_ITEM(BackupFirmware),
    CMD_ITEM(TestTMC),
    CMD_ITEM(TestBufLenTMC),
    CMD_ITEM(TestSlave),
    CMD_ITEM(TestBufLenSlave),
    CMD_ITEM(GetExtInput),
    CMD_ITEM(SetExtInput),
};

void WsServer::WebSokectProtocol(struct mg_connection *c, struct mg_ws_message *wm)
{
    auto wsClient = (WsClient *)c->fn_data;
    if ((wsClient->len + wm->data.len) >= WsMsgBuf_SIZE)
    {
        wsClient->len = 0;
        if (wm->data.len >= WsMsgBuf_SIZE)
        {
            return;
        }
    }
    char *p = wsClient->buf + wsClient->len;
    memcpy(p, wm->data.ptr, wm->data.len);
    p[wm->data.len] = 0;
    wsClient->len += wm->data.len;
    if (ws_hexdump & 1)
    {
        DebugPrt("<<<mg_ws_message<<<\n%s", p);
    }

    if (wsClient->len <= 0 || wsClient->buf[wsClient->len - 1] != '}')
    {
        return;
    }
    if (json::accept(p, true))
    {
        wsClient->len = 0; // clear msgbuf
    }
    else
    {
        if (p != wsClient->buf && json::accept(wsClient->buf, true))
        {
            wsClient->len = 0; // clear msgbuf
            p = wsClient->buf;
        }
        else
        {
            p = nullptr;
        }
    }
    if (p != nullptr)
    {
        json reply;
        try
        {
            auto msg = json::parse(p, nullptr, true, true);
            auto cmd = GetStr(msg, "cmd");
            reply.emplace("cmd", cmd);
            int j = (wsClient->login) ? countof(CMD_LIST) : 1;
            for (int i = 0; i < j; i++)
            {
                if (strcasecmp(cmd.c_str(), CMD_LIST[i].cmd) == 0)
                {
                    CMD_LIST[i].function(c, msg, reply);
                    WebSocketSend(c, reply);
                    if (wsClient->login)
                    {
                        Controller::Instance().RefreshDispTime();
                    }
                    return;
                }
            }
            reply["result"] = "Unknown cmd:" + cmd;
        }
        catch (exception &e)
        {
            reply.clear();
            reply.emplace("result", e.what());
        }
        catch (...)
        {
            reply.clear();
            reply.emplace("result", "Something wrong");
        }
        WebSocketSend(c, reply);
    }
}

void WsServer::CMD_Login(struct mg_connection *c, json &msg, json &reply)
{
    auto wsClient = (WsClient *)c->fn_data;
    auto msgu = GetStr(msg, "user");
    reply.emplace("user", msgu);
    auto ps = DbHelper::Instance().GetUciPasswd().GetUserPasswd(msgu);
    if (ps != nullptr && ps->passwd.compare(GetStr(msg, "password")) == 0)
    {
        wsClient->user = msgu;
        wsClient->login = true;
        reply.emplace("result", "OK");
    }
    else
    {
        wsClient->user.clear();
        wsClient->login = false;
        reply.emplace("result", "Invalid user or password");
    }
}

void WsServer::CMD_ChangePassword(struct mg_connection *c, json &msg, json &reply)
{
    auto wsClient = (WsClient *)c->fn_data;
    auto cp = GetStr(msg, "current");
    auto np = GetStr(msg, "new");
    if (np.length() < 4 || np.length() > 10)
    {
        throw invalid_argument("Length of password should be 4 - 10");
    }
    else
    {
        auto &up = DbHelper::Instance().GetUciPasswd();
        auto ps = up.GetUserPasswd(wsClient->user);
        if (ps == nullptr)
        {
            throw invalid_argument("Invalid user");
        }
        else if (ps->passwd.compare(cp) != 0)
        {
            throw invalid_argument("Current password NOT match");
        }
        for (auto s : np)
        {
            if (s <= 0x20 || s == '#' || s == '"' || s == '\'' || s == '\\' || s >= 0x7F)
            {
                throw invalid_argument("Invalid character in new password");
            }
        }
        up.Set(wsClient->user, np, ps->permission);
        reply.emplace("result", "Password changed");
    }
}

void WsServer::CMD_GetGroupConfig(struct mg_connection *c, json &msg, json &reply)
{
    auto gs = ctrller->GroupCnt();
    reply.emplace("number_of_groups", gs);
    vector<json> groups(gs);
    for (int i = 0; i < gs; i++)
    {
        auto g = ctrller->GetGroup(i + 1);
        auto &v = groups[i];
        v.emplace("group_id", g->GroupId());
        v.emplace("number_of_signs", g->SignCnt());
        vector<int> signs(g->SignCnt());
        for (int j = 0; j < g->SignCnt(); j++)
        {
            signs[j] = g->GetSigns()[j]->SignId();
        }
        v.emplace("signs", signs);
    }
    reply.emplace("groups", groups);
}

void WsServer::CMD_SetGroupConfig(struct mg_connection *c, json &msg, json &reply)
{
    // TODO: Because group configuration related to hardware (signs in same group should at same COM port).
    // SO changing group configuration should directly edit "config/UciHardware" file.
    reply.emplace("result", "Unsupported command");
    return;
#if 0
    auto gc = ctrller->GroupCnt();
    auto vgroups = GetVector<json>(msg,"groups");
    vector<int> vgid(gc);
    vgid.assign(gc, 0);
    auto sc = ctrller->SignCnt();
    vector<int> vsign_gid(sc);
    vsign_gid.assign(sc, 0);
    for (int i = 0; i < gc; i++)
    {
        auto gid = GetUint(vgroups[i], "group_id", 1, gc);
        if (vgid[gid - 1] > 0)
        {
            throw invalid_argument(StrFn::PrintfStr("Repeated group id [%d]", gid));
        }
        vgid[gid - 1] = gid;
        auto vsid = GetVector<int>(vgroups[i],"signs");
        for (auto id : vsid)
        {
            if (id <= 0 || id > sc)
            {
                throw invalid_argument(StrFn::PrintfStr("Invalid sign id [%d]", id));
            }
            if (vsign_gid[id - 1] == 0)
            {
                vsign_gid[id - 1] = gid;
            }
            else
            {
                            throw invalid_argument(StrFn::PrintfStr("Repeated sign id [%d]", id));
            }
        }
    }
    for (int i = 0; i < vgid.size(); i++)
    {
        if (vgid[i] == 0)
        {
            throw invalid_argument(StrFn::PrintfStr("Missing group[%d] config", i+1));
        }
    }
    for (int i = 0; i < vsign_gid.size(); i++)
    {
        if (vsign_gid[i] == 0)
        {
            throw invalid_argument(StrFn::PrintfStr("Missing sign[%d] config", i+1));
        }
    }
#endif
}

extern const char *FirmwareVer;
void WsServer::CMD_GetStatus(struct mg_connection *c, json &msg, json &reply)
{
    char buf[64];
    reply.emplace("manufacturer_code", DbHelper::Instance().GetUciHardware().MfcCode());
    reply.emplace("firmware", FirmwareVer);
    reply.emplace("is_online", ctrller->IsOnline());
    reply.emplace("application_error", "0x00:No error");
    char rtc[32];
    Utils::Time::ParseTimeToLocalStr(time(nullptr), rtc);
    reply.emplace("rtc", rtc);
    sprintf(buf, "0x%04X", DbHelper::Instance().HdrChksum());
    reply.emplace("hardware_checksum", buf);
    DEV::ERROR dev_err;
    dev_err = ctrller->GetErrorCode();
    sprintf(buf, "0x%02X:%s", static_cast<int>(dev_err), DEV::ToStr(dev_err));
    reply.emplace("controller_error", buf);
    reply.emplace("max_temperature", ctrller->MaxTemp());
    reply.emplace("current_temperature", ctrller->CurTemp());
    int group_cnt = ctrller->GroupCnt();
    vector<json> groups(group_cnt);
    for (int i = 0; i < group_cnt; i++)
    {
        auto s = ctrller->GetGroup(i + 1);
        auto &v = groups[i];
        v.emplace("group_id", s->GroupId());
        v.emplace("device", s->IsDevice() ? "Enabled" : "Disabled");
        v.emplace("power", s->IsPower() ? "On" : "Off");
    }
    reply.emplace("groups", groups);

    int sign_cnt = ctrller->SignCnt();
    vector<json> signs(sign_cnt);
    for (int i = 0; i < sign_cnt; i++)
    {
        auto s = ctrller->GetSign(i + 1);
        auto &v = signs[i];
        v.emplace("sign_id", s->SignId());
        v.emplace("dimming_mode", s->DimmingMode() == 0 ? "Auto" : "Manual");
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
        dev_err = s->SignErr().GetErrorCode();
        sprintf(buf, "0x%02X:%s", static_cast<int>(dev_err), DEV::ToStr(dev_err));
        v.emplace("error_code", buf);
        v.emplace("faulty_pixels", s->FaultLedCnt());
        v.emplace("image", s->GetImageBase64());
    }
    reply.emplace("signs", signs);
}

void WsServer::CMD_GetUserConfig(struct mg_connection *c, json &msg, json &reply)
{
    auto &usercfg = DbHelper::Instance().GetUciUserCfg();
    char buf[8];
    sprintf(buf, "0x%02X", usercfg.SeedOffset());
    reply.emplace("seed", buf);
    sprintf(buf, "0x%04X", usercfg.PasswordOffset());
    reply.emplace("password", buf);
    reply.emplace("device_id", usercfg.DeviceId());
    reply.emplace("broadcast_id", usercfg.BroadcastId());
    reply.emplace("session_timeout", usercfg.SessionTimeoutSec());
    reply.emplace("display_timeout", usercfg.DisplayTimeoutMin());
    vector<string> tmc_com_port_list;
    for(int i=0;i<COMPORT_SIZE;i++)
    {
        tmc_com_port_list.emplace_back(COM_NAME[i]);
    }
    reply.emplace("tmc_com_port_list", tmc_com_port_list);
    reply.emplace("tmc_com_port", COM_NAME[usercfg.TmcComPort()]);

    vector<int> baudrate_list;
    for(int i=0;i<EXTENDEDBPS_SIZE;i++)
    {
        baudrate_list.emplace_back(ALLOWEDBPS[i]);
    }
    reply.emplace("baudrate_list", baudrate_list);
    reply.emplace("baudrate", usercfg.TmcBaudrate());
    reply.emplace("multiled_fault", usercfg.MultiLedFaultThreshold());
    reply.emplace("tmc_tcp_port", usercfg.TmcTcpPort());
    reply.emplace("over_temp", usercfg.OverTemp());
    reply.emplace("locked_frame", usercfg.LockedFrm());
    reply.emplace("locked_msg", usercfg.LockedMsg());
    vector<string> city_list;
    for(int i=0;i<NUMBER_OF_TZ;i++)
    {
        city_list.emplace_back(Tz_AU::tz_au[i].city);
    }
    reply.emplace("city_list", city_list);
    reply.emplace("city", usercfg.City());
    reply.emplace("last_frame_time", usercfg.LastFrmTime());
    reply.emplace("night_level", usercfg.NightDimmingLevel());
    reply.emplace("dawn_dusk_level", usercfg.DawnDimmingLevel());
    reply.emplace("day_level", usercfg.DayDimmingLevel());
    reply.emplace("night_max_lux", usercfg.LuxNightMax());
    reply.emplace("day_min_lux", usercfg.LuxDayMin());
    reply.emplace("min_lux_18_hours", usercfg.Lux18HoursMin());
}

void WsServer::CMD_SetUserConfig(struct mg_connection *c, json &msg, json &reply)
{
    unsigned char rr_flag = 0;
    auto device_id = GetUint(msg, "device_id", 0, 255);
    auto broadcast_id = GetUint(msg, "broadcast_id", 0, 255);
    if (device_id == broadcast_id)
    {
        throw invalid_argument("device_id should not equal to broadcast_id");
    }
    auto session_timeout = GetUint(msg, "session_timeout", 0, 65535);
    auto display_timeout = GetUint(msg, "display_timeout", 0, 65535);
    auto baudrate = GetUint(msg, "baudrate", 19200, 115200);
    auto multiled_fault = GetUint(msg, "multiled_fault", 0, 255);
    auto tmc_tcp_port = GetUint(msg, "tmc_tcp_port", 1024, 65535);
    auto over_temp = GetUint(msg, "over_temp", 0, 99);
    auto locked_frame = GetUint(msg, "locked_frame", 0, 255);
    auto locked_msg = GetUint(msg, "locked_msg", 0, 255);
    auto last_frame_time = GetUint(msg, "last_frame_time", 0, 255);

    auto seed = GetUintFromStr(msg, "seed", 0, 0xFF);
    auto password = GetUintFromStr(msg, "password", 0, 0xFFFF);
    auto tmc_com_port = Pick::PickStr(GetStr(msg, "tmc_com_port").c_str(), COM_NAME, COMPORT_SIZE, true);
    if (tmc_com_port == -1)
    {
        throw invalid_argument("'tmc_com_port' NOT valid");
    }
    const char *CITY[NUMBER_OF_TZ];
    for (int i = 0; i < NUMBER_OF_TZ; i++)
    {
        CITY[i] = Tz_AU::tz_au[i].city;
    }
    int city = Pick::PickStr(GetStr(msg, "city").c_str(), CITY, NUMBER_OF_TZ, true);
    if (city == -1)
    {
        throw invalid_argument("'city' NOT valid");
    }

    auto night_level = GetUint(msg, "night_level", 1, 8);
    auto dawn_dusk_level = GetUint(msg, "dawn_dusk_level", night_level + 1, 15);
    auto day_level = GetUint(msg, "day_level", dawn_dusk_level + 1, 16);
    auto night_max_lux = GetUint(msg, "night_max_lux", 1, 9999);
    auto day_min_lux = GetUint(msg, "day_min_lux", night_max_lux + 1, 65535);
    auto min_lux_18_hours = GetUint(msg, "min_lux_18_hours", day_min_lux + 1, 65535);

    auto &usercfg = DbHelper::Instance().GetUciUserCfg();
    auto &evt = DbHelper::Instance().GetUciEvent();

    if (seed != usercfg.SeedOffset())
    {
        evt.Push(0, "User.SeedOffset changed: 0x%02X->0x%02X", usercfg.SeedOffset(), seed);
        usercfg.SeedOffset(seed);
    }

    if (password != usercfg.PasswordOffset())
    {
        evt.Push(0, "User.PasswordOffset changed: 0x%04X->0x%04X", usercfg.PasswordOffset(), password);
        usercfg.PasswordOffset(password);
    }

    if (device_id != usercfg.DeviceId())
    {
        evt.Push(0, "User.DeviceID changed: %u->%u", usercfg.DeviceId(), device_id);
        usercfg.DeviceId(device_id);
    }

    if (broadcast_id != usercfg.BroadcastId())
    {
        evt.Push(0, "User.BroadcastID changed: %u->%u", usercfg.BroadcastId(), broadcast_id);
        usercfg.BroadcastId(broadcast_id);
    }

    if (tmc_com_port != usercfg.TmcComPort())
    {
        evt.Push(0, "User.TmcComPort changed: %s->%s . Restart to load new setting",
                 COM_NAME[usercfg.TmcComPort()], COM_NAME[tmc_com_port]);
        usercfg.TmcComPort(tmc_com_port);
        rr_flag |= RQST_RESTART;
    }

    if (baudrate != usercfg.TmcBaudrate())
    {
        evt.Push(0, "User.TmcBaudrate changed: %u->%u . Restart to load new setting",
                 usercfg.TmcBaudrate(), baudrate);
        usercfg.TmcBaudrate(baudrate);
        rr_flag |= RQST_RESTART;
    }

    if (tmc_tcp_port != usercfg.TmcTcpPort())
    {
        evt.Push(0, "User.TmcTcpPort changed: %u->%u. Restart to load new setting",
                 usercfg.TmcTcpPort(), tmc_tcp_port);
        usercfg.TmcTcpPort(tmc_tcp_port);
        rr_flag |= RQST_RESTART;
    }

    if (session_timeout != usercfg.SessionTimeoutSec())
    {
        evt.Push(0, "User.SessionTimeout changed: %u->%u",
                 usercfg.SessionTimeoutSec(), session_timeout);
        usercfg.SessionTimeoutSec(session_timeout);
    }

    if (display_timeout != usercfg.DisplayTimeoutMin())
    {
        evt.Push(0, "User.DisplayTimeout changed: %u->%u",
                 usercfg.DisplayTimeoutMin(), display_timeout);
        usercfg.DisplayTimeoutMin(display_timeout);
    }

    if (over_temp != usercfg.OverTemp())
    {
        evt.Push(0, "User.OverTemp changed: %u->%u",
                 usercfg.OverTemp(), over_temp);
        usercfg.OverTemp(over_temp);
    }

    if (city != usercfg.CityId())
    {
        evt.Push(0, "User.City changed: %s->%s",
                 Tz_AU::tz_au[usercfg.CityId()].city, Tz_AU::tz_au[city].city);
        usercfg.CityId(city);
        rr_flag |= RQST_RESTART;
    }

    if (multiled_fault != usercfg.MultiLedFaultThreshold())
    {
        evt.Push(0, "User.MultiLedFaultThreshold changed: %u->%u",
                 usercfg.MultiLedFaultThreshold(), multiled_fault);
        usercfg.MultiLedFaultThreshold(multiled_fault);
    }

    if (locked_msg != usercfg.LockedMsg())
    {
        evt.Push(0, "User.LockedMsg changed: %u->%u",
                 usercfg.LockedMsg(), locked_msg);
        usercfg.LockedMsg(locked_msg);
    }

    if (locked_frame != usercfg.LockedFrm())
    {
        evt.Push(0, "User.LockedFrm changed: %u->%u",
                 usercfg.LockedFrm(), locked_frame);
        usercfg.LockedFrm(locked_frame);
    }

    if (last_frame_time != usercfg.LastFrmTime())
    {
        evt.Push(0, "User.LastFrmTime changed: %u->%u",
                 usercfg.LastFrmTime(), last_frame_time);
        usercfg.LastFrmTime(last_frame_time);
    }

    if (night_level != usercfg.NightDimmingLevel())
    {
        evt.Push(0, "User.NightDimmingLevel changed: %d->%d", usercfg.NightDimmingLevel(), night_level);
        usercfg.NightDimmingLevel(night_level);
    }
    if (dawn_dusk_level != usercfg.DawnDimmingLevel())
    {
        evt.Push(0, "User.DawnDimmingLevel changed: %d->%d", usercfg.DawnDimmingLevel(), dawn_dusk_level);
        usercfg.DawnDimmingLevel(dawn_dusk_level);
    }
    if (day_level != usercfg.DayDimmingLevel())
    {
        evt.Push(0, "User.DayDimmingLevel changed: %d->%d", usercfg.DayDimmingLevel(), day_level);
        usercfg.DayDimmingLevel(day_level);
    }
    if (night_max_lux != usercfg.LuxNightMax())
    {
        evt.Push(0, "User.LuxNightMax changed: %d->%d", usercfg.LuxNightMax(), night_max_lux);
        usercfg.LuxNightMax(night_max_lux);
    }
    if (day_min_lux != usercfg.LuxDayMin())
    {
        evt.Push(0, "User.LuxDayMin changed: %d->%d", usercfg.LuxDayMin(), day_min_lux);
        usercfg.LuxDayMin(day_min_lux);
    }
    if (min_lux_18_hours != usercfg.Lux18HoursMin())
    {
        evt.Push(0, "User.Lux18HoursMin changed: %d->%d", usercfg.Lux18HoursMin(), min_lux_18_hours);
        usercfg.Lux18HoursMin(min_lux_18_hours);
    }
    reply.emplace("result", (rr_flag != 0) ? "'Reboot' to active new setting" : "OK");
}

void WsServer::CMD_DefaultUserConfig(struct mg_connection *c, json &msg, json &reply)
{
    auto &usercfg = DbHelper::Instance().GetUciUserCfg();
    usercfg.LoadFactoryDefault();
    CMD_GetUserConfig(c, msg, reply);
    reply.emplace("result", "Controller will reboot after 5 seconds");
    ctrller->RR_flag(RQST_REBOOT);
}

void WsServer::CMD_GetNetworkConfig(struct mg_connection *c, json &msg, json &reply)
{
    auto &net = DbHelper::Instance().GetUciNetwork();
    string ethname[2]{"ETH1", "ETH2"};
    json ethjs[2];
    for (int i = 0; i < 2; i++)
    {
        auto eth = net.GetETH(ethname[i]);
        ethjs[i].emplace(net._Proto, eth->proto);
        ethjs[i].emplace(net._Ipaddr, eth->ipaddr.ToString());
        ethjs[i].emplace(net._Netmask, eth->netmask.ToString());
        ethjs[i].emplace(net._Gateway, eth->gateway.ToString());
        ethjs[i].emplace(net._Dns, eth->dns);
        reply.emplace(ethname[i], ethjs[i]);
    }
    json ntpjs;
    auto ntp = net.GetNtp();
    ntpjs.emplace(net._Server, ntp->server);
    ntpjs.emplace(net._Port, ntp->port);
    reply.emplace("NTP", ntpjs);
}

void WsServer::CMD_SetNetworkConfig(struct mg_connection *c, json &msg, json &reply)
{
    auto &net = DbHelper::Instance().GetUciNetwork();

    string ethname[2]{"ETH1", "ETH2"};
    json ethjs[2];
    NetInterface interfaces[2];
    for (int i = 0; i < 2; i++)
    {
        ethjs[i] = msg[ethname[i]].get<json>();
        interfaces[i].proto = ethjs[i][net._Proto].get<std::string>();
        if (interfaces[i].proto.compare("static") == 0)
        {
            if (interfaces[i].ipaddr.Set(ethjs[i][net._Ipaddr].get<std::string>()) == false)
            {
                throw invalid_argument(StrFn::PrintfStr("Invalid %s.ipaddr", ethname[i]));
            }
            if (interfaces[i].netmask.Set(ethjs[i][net._Netmask].get<std::string>()) == false)
            {
                throw invalid_argument(StrFn::PrintfStr("Invalid %s.netmask", ethname[i]));
            }
            string str_gateway;
            try
            {
                str_gateway = ethjs[i][net._Gateway].get<std::string>();
                interfaces[i].dns = ethjs[i][net._Dns].get<std::string>();
            }
            catch (...)
            {
            }
            if (!str_gateway.empty())
            {
                if (interfaces[i].gateway.Set(str_gateway) == false)
                {
                    throw invalid_argument(StrFn::PrintfStr("Invalid %s.gateway", ethname[i]));
                }
            }
        }
        else if (interfaces[i].proto.compare("dhcp") == 0)
        {
            interfaces[i].dns = string("");
            interfaces[i].ipaddr.ip.ip32 = 0;
            interfaces[i].netmask.ip.ip32 = 0;
            interfaces[i].gateway.ip.ip32 = 0;
        }
        else
        {
            throw invalid_argument(StrFn::PrintfStr("Invalid %s.proto", ethname[i]));
        }
    }

    if (interfaces[0].gateway.Isvalid() && interfaces[1].gateway.Isvalid())
    {
        throw invalid_argument("Only one ETH can have gateway");
    }
    if (interfaces[0].ipaddr.Isvalid() && interfaces[1].ipaddr.Isvalid())
    {
        if (interfaces[0].ipaddr.Compare(interfaces[1].ipaddr) == 0)
        {
            throw invalid_argument("ETH1.ipaddr and ETH2.ipaddr should not be same");
        }
    }
#if false // TODO NTP disabled
    json ntpjs = msg["NTP"].get<json>();
    NtpServer ntp;
    ntp.server = GetStr(ntpjs, net._Server);
    ntp.port = GetUint(ntpjs, net._Port, 1, 65535);

    for (int i = 0; i < 2; i++)
    {
        if (interfaces[i].ipaddr.Isvalid() && ntp.server.find(interfaces[i].ipaddr.ToString()) != string::npos)
        {
            throw invalid_argument("NTP Server should not be ETH" + to_string(i + 1));
        }
    }
#endif
    // all parameters are OK, save
    int r;
    net.evts.clear();

    bool ntpChanged{false};
#if false // TODO NTP disabled
    r = net.SaveNtp(ntp);
    if (r != 0)
    {
        throw runtime_error("Set NTP failed: " + to_string(r));
    }
    if (!net.evts.empty())
    {
        // TODO restart NTP service
        ntpChanged = true;
        auto &evlog = DbHelper::Instance().GetUciEvent();
        for (auto &e : net.evts)
        {
            evlog.Push(0, e.c_str());
        }
        net.evts.clear();
    }
#endif

    for (int i = 0; i < 2; i++)
    {
        r = net.SaveETH(ethname[i], interfaces[i]);
        if (r != 0)
        {
            throw runtime_error("Set network." + ethname[i] + " failed: " + to_string(r));
        }
    }
    if (!net.evts.empty())
    {
        r = net.UciCommit();
        if (r != 0)
        {
            throw runtime_error("Commit network failed: " + to_string(r));
        }
        reply.emplace("result", "Network changed. Controller will REBOOT to active new settings");
        ctrller->RR_flag(RQST_REBOOT);
    }
    else
    {
        reply.emplace("result", ntpChanged ? "NTP settings refreshed" : "Nothing changed");
    }
}

void WsServer::CMD_ControlDimming(struct mg_connection *c, json &msg, json &reply)
{
    uint8_t cmd[5];
    cmd[0] = static_cast<uint8_t>(MI::CODE::SignSetDimmingLevel);
    cmd[1] = 1;
    cmd[2] = GetUint(msg, "group_id", 0, 255);
    cmd[4] = GetUint(msg, "setting", 0, 16);
    cmd[3] = cmd[4] == 0 ? 0 : 1;

    char buf[64];
    reply.emplace("result", (APP::ERROR::AppNoError == ctrller->CmdSetDimmingLevel(cmd, buf)) ? "OK" : buf);
}

void WsServer::CMD_ControlPower(struct mg_connection *c, json &msg, json &reply)
{
    uint8_t cmd[4];
    cmd[0] = static_cast<uint8_t>(MI::CODE::PowerOnOff);
    cmd[1] = 1;
    cmd[2] = GetUint(msg, "group_id", 0, 255);
    cmd[3] = GetUint(msg, "setting", 0, 1);
    char buf[64];
    reply.emplace("result", (APP::ERROR::AppNoError == ctrller->CmdPowerOnOff(cmd, buf)) ? "OK" : buf);
}

void WsServer::CMD_ControlDevice(struct mg_connection *c, json &msg, json &reply)
{
    uint8_t cmd[4];
    cmd[1] = 1;
    cmd[2] = GetUint(msg, "group_id", 0, 255);
    cmd[3] = GetUint(msg, "setting", 0, 1);
    char buf[64];
    reply.emplace("result", (APP::ERROR::AppNoError == ctrller->CmdDisableEnableDevice(cmd, buf)) ? "OK" : buf);
}

void WsServer::CMD_SystemReset(struct mg_connection *c, json &msg, json &reply)
{
    uint8_t cmd[3];
    cmd[0] = static_cast<uint8_t>(MI::CODE::SystemReset);
    cmd[1] = GetUint(msg, "group_id", 0, ctrller->GroupCnt());
    cmd[2] = GetUint(msg, "level", 0, 255);
    char buf[64];
    reply.emplace("result", (ctrller->CmdSystemReset(cmd, buf) == APP::ERROR::AppNoError) ? "OK" : buf);
}

void WsServer::CMD_UpdateTime(struct mg_connection *c, json &msg, json &reply)
{
    auto cmd = GetStr(msg, "rtc");
    if (cmd.length() <= 0)
    {
        throw invalid_argument("No 'rtc' in command");
    }
    struct tm stm;
    if ((Time::ParseLocalStrToTm(cmd.c_str(), &stm) == 0) &&
        (ctrller->CmdUpdateTime(stm) == APP::ERROR::AppNoError))
    {
        reply.emplace("result", "OK");
    }
    else
    {
        throw invalid_argument("Invalid 'rtc'");
    }
}

void WsServer::CMD_GetFrameSetting(struct mg_connection *c, json &msg, json &reply)
{
    auto &ucihw = DbHelper::Instance().GetUciHardware();
    vector<string> frametype;
    frametype.push_back("Text Frame");
    if (ucihw.PixelRows() < 255 && ucihw.PixelColumns() < 255)
    {
        frametype.push_back("Graphics Frame");
    }
    frametype.push_back("HR Graphics Frame");
    reply.emplace("frame_type", frametype);

    vector<string> txt_c;
    for (int i = 0; i < MONO_COLOUR_NAME_SIZE; i++)
    {
        if (ucihw.IsTxtFrmColourValid(i))
        {
            txt_c.push_back(string(FrameColour::COLOUR_NAME[i]));
        }
    }
    reply.emplace("txt_frame_colours", txt_c);

    vector<string> gfx_c;
    for (int i = 0; i < MONO_COLOUR_NAME_SIZE; i++)
    {
        if (ucihw.IsGfxFrmColourValid(i))
        {
            gfx_c.push_back(string(FrameColour::COLOUR_NAME[i]));
        }
    }
    if (ucihw.IsGfxFrmColourValid(static_cast<int>(FRMCOLOUR::Multi_4bit)))
    {
        gfx_c.push_back(string("Multi(4-bit)"));
    }
    if (ucihw.IsGfxFrmColourValid(static_cast<int>(FRMCOLOUR::RGB_24bit)))
    {
        gfx_c.push_back(string("RGB(24-bit)"));
    }
    reply.emplace("gfx_frame_colours", gfx_c);

    vector<string> hrg_c;
    for (int i = 0; i < MONO_COLOUR_NAME_SIZE; i++)
    {
        if (ucihw.IsHrgFrmColourValid(i))
        {
            hrg_c.push_back(string(FrameColour::COLOUR_NAME[i]));
        }
    }
    if (ucihw.IsHrgFrmColourValid(static_cast<int>(FRMCOLOUR::Multi_4bit)))
    {
        hrg_c.push_back(string("Multi(4-bit)"));
    }
    if (ucihw.IsHrgFrmColourValid(static_cast<int>(FRMCOLOUR::RGB_24bit)))
    {
        hrg_c.push_back(string("RGB(24-bit)"));
    }
    reply.emplace("hrg_frame_colours", hrg_c);

    vector<int> fonts;
    for (int i = 0; i <= ucihw.MaxFont(); i++)
    {
        if (ucihw.IsFont(i))
        {
            fonts.push_back(i);
        }
    }
    reply.emplace("fonts", fonts);

    vector<int> txt_columns;
    vector<int> txt_rows;
    for (auto f : fonts)
    {
        txt_columns.emplace_back(ucihw.CharColumns(f));
        txt_rows.emplace_back(ucihw.CharRows(f));
    }
    reply.emplace("txt_columns", txt_columns);
    reply.emplace("txt_rows", txt_rows);

    vector<string> conspicuity;
    for (int i = 0; i <= ucihw.MaxConspicuity(); i++)
    {
        if (ucihw.IsConspicuity(i))
        {
            conspicuity.push_back(string(Conspicuity[i]));
        }
    }
    reply.emplace("conspicuity", conspicuity);

    vector<string> annulus;
    for (int i = 0; i <= ucihw.MaxAnnulus(); i++)
    {
        if (ucihw.IsAnnulus(i))
        {
            annulus.push_back(string(Annulus[i]));
        }
    }
    reply.emplace("annulus", annulus);
}

void WsServer::CMD_GetStoredFrame(struct mg_connection *c, json &msg, json &reply)
{
    auto id = GetUint(msg, "id", 1, 255);
    reply.emplace("id", id);
    auto frm = DbHelper::Instance().GetUciFrm().GetFrm(id);
    if (frm == nullptr)
    {
        reply.emplace("result", "Error: Undefined frame");
    }
    else
    {
        reply.emplace("result", "OK");
        reply.emplace("revision", frm->frmRev);
        reply.emplace("colour", FrameColour::GetColourName(frm->colour));
        reply.emplace("conspicuity", Conspicuity[frm->conspicuity & 0x07]);
        reply.emplace("annulus", Annulus[(frm->conspicuity >> 3) & 0x03]);
        if (frm->micode == 0x0A) // SignSetTextFrame
        {
            reply.emplace("type", "Text Frame");
            auto &rawdata = frm->stFrm.rawData;
            reply.emplace("font", rawdata[3]);

            // vector<char> txt(rawdata[6] + 1);
            // memcpy(txt.data(), rawdata.data() + 7, rawdata[6]);
            // txt.back() = '\0';
            // reply.emplace("text", txt.data());

            auto vtxt = static_cast<FrmTxt *>(frm)->ToStringVector();
            string text;
            int s = vtxt.size() - 1;
            for (int vi = 0; vi <= s; vi++)
            {
                text.append(vtxt.at(vi));
                if (vi < s)
                {
                    text.append("\n");
                }
            }
            std::replace(text.begin(), text.end(), '_', ' ');
            reply.emplace("text", text);
        }
        else if (frm->micode == 0x0B || frm->micode == 0x1D)
        {
            reply.emplace("type", (frm->micode == 0x0B) ? "Graphics Frame" : "HR Graphics Frame");
            unique_ptr<FrameImage> fi(new FrameImage());
            fi->SetId(0, id);
            fi->FillCoreFromUciFrame();
            reply.emplace("image", fi->Save2Base64().data());
        }
    }
}

void WsServer::CMD_SetFrame(struct mg_connection *c, nlohmann::json &msg, json &reply)
{
    auto &ucihw = DbHelper::Instance().GetUciHardware();
    string str;
    auto id = GetUint(msg, "id", 1, 255);
    auto rev = GetUint(msg, "revision", 0, 255);
    const char *FRM_TYPE[] = {"Text Frame", "Graphics Frame", "HR Graphics Frame"};
    str = msg["type"].get<std::string>();
    int ftype = Pick::PickStr(str.c_str(), FRM_TYPE, countof(FRM_TYPE), true);
    if (ftype < 0)
    {
        throw invalid_argument("CMD_SetFrame: type error");
    }
    MI::CODE FrmType[3]{MI::CODE::SignSetTextFrame, MI::CODE::SignSetGraphicsFrame, MI::CODE::SignSetHighResolutionGraphicsFrame};
    MI::CODE frmType = FrmType[ftype];
    str = msg["colour"].get<std::string>();
    int colour = Pick::PickStr(str.c_str(), FrameColour::COLOUR_NAME, ALL_COLOUR_NAME_SIZE, true);
    if (colour < 0)
    {
        throw invalid_argument("CMD_SetFrame: colour error");
    }
    if (frmType == MI::CODE::SignSetTextFrame && colour > 9)
    {
        throw invalid_argument("CMD_SetFrame: colour error(Text Frame)");
    }
    if (frmType == MI::CODE::SignSetGraphicsFrame && colour > 10)
    {
        throw invalid_argument("CMD_SetFrame: colour error(Graphics Frame)");
    }
    str = msg["conspicuity"].get<std::string>();
    int conspicuity = Pick::PickStr(str.c_str(), Conspicuity, CONSPICUITY_SIZE, true);
    if (conspicuity < 0)
    {
        throw invalid_argument("CMD_SetFrame: conspicuity error");
    }
    str = msg["annulus"].get<std::string>();
    int annulus = Pick::PickStr(str.c_str(), Annulus, ANNULUS_SIZE, true);
    if (annulus < 0)
    {
        throw("CMD_SetFrame: annulus error");
    }
    conspicuity = (conspicuity) | (annulus << 3);
    vector<uint8_t> frm; // will be resized by frmType
    if (frmType == MI::CODE::SignSetTextFrame)
    {
        str = msg["text"].get<std::string>();
        if (str.length() < 1)
        {
            throw invalid_argument("'Text' is null");
        }
        std::replace(str.begin(), str.end(), ' ', '_');
        StrFn::ReplaceAll(str, "\n", " ");
        frm.resize(str.length() + 9);
        frm[0] = static_cast<uint8_t>(frmType);
        frm[1] = id;
        frm[2] = rev;
        frm[3] = GetUint(msg, "font", 0, 255);
        frm[4] = colour;
        frm[5] = conspicuity;
        frm[6] = str.length();
        memcpy(frm.data() + 7, str.data(), str.length());
    }
    else // Gfx or HR Gfx
    {
        str = msg["image"].get<std::string>();
        if (str.length() < 120) // a 5*7 mono bmp file is 90 bytes in binary and 120 bytes in base64
        {
            throw invalid_argument("Invalid Base64 BMP data");
        }
        unique_ptr<FrameImage> frmImg(new FrameImage);
        frmImg->LoadBmpFromBase64(str.c_str(), str.length());
        auto &bmp = frmImg->GetBmp();
        auto rows = ucihw.PixelRows();
        auto columns = ucihw.PixelColumns();
        if (bmp.TellWidth() == columns + 2 * ucihw.CoreOffsetX() && bmp.TellHeight() == rows + 2 * ucihw.CoreOffsetY())
        { // bmp including conspicuity & annulus
            bmp.TrimFrame(ucihw.CoreOffsetX(), ucihw.CoreOffsetY());
        }
        if (bmp.TellHeight() != rows || bmp.TellWidth() != columns)
        {
            throw invalid_argument("Image size NOT matched with sign");
        }
        int corelen;
        if (colour <= 9)
        { // mono
            corelen = ucihw.Gfx1CoreLen();
        }
        if (colour == 10)
        {
            colour = static_cast<uint8_t>(FRMCOLOUR::Multi_4bit);
            corelen = ucihw.Gfx4CoreLen();
        }
        else if (colour == 11)
        {
            colour = static_cast<uint8_t>(FRMCOLOUR::RGB_24bit);
            corelen = ucihw.Gfx24CoreLen();
        }
        int f_offset;
        if (rows < 255 && columns < 255)
        {
            f_offset = 9;
            frm.resize(corelen + f_offset + 2);
            frm[3] = rows;
            frm[4] = columns;
            frm[5] = colour;
            frm[6] = conspicuity;
            Utils::Cnvt::PutU16(corelen, frm.data() + 7);
        }
        else
        {
            f_offset = 13;
            frm.resize(corelen + f_offset + 2);
            Cnvt::PutU16(rows, frm.data() + 3);
            Cnvt::PutU16(columns, frm.data() + 5);
            frm[7] = colour;
            frm[8] = conspicuity;
            Utils::Cnvt::PutU32(corelen, frm.data() + 9);
        }
        frm[0] = static_cast<uint8_t>(frmType);
        frm[1] = id;
        frm[2] = rev;
        int bitOffset = 0;
        if (colour <= 9)
        { // mono
            uint8_t *core = frm.data() + f_offset;
            memset(core, 0, corelen);
            int bitOffset = 0;
            for (int y = 0; y < rows; y++)
            {
                for (int x = 0; x < columns; x++)
                {
                    auto pixel = bmp.GetPixel(x, y);
                    if (pixel.Blue > 0 || pixel.Green > 0 || pixel.Red > 0)
                    {
                        BitOffset::Set70Bit(core, bitOffset);
                    }
                    bitOffset++;
                }
            }
        }
        else if (colour == static_cast<uint8_t>(FRMCOLOUR::Multi_4bit))
        {
            for (int y = 0; y < rows; y++)
            {
                for (int x = 0; x < columns; x++)
                {
                    auto pixel = bmp.GetPixel(x, y);
                    uint8_t c = FrameColour::Rgb2Colour(((pixel.Red * 0x100) + pixel.Green) * 0x100 + pixel.Blue);
                    int p = f_offset + bitOffset / 2;
                    frm.at(p) |= (bitOffset & 1) ? (c << 4) : c;
                    bitOffset++;
                }
            }
        }
        else if (colour == static_cast<uint8_t>(FRMCOLOUR::RGB_24bit))
        {
            bitOffset = f_offset;
            for (int y = 0; y < rows; y++)
            {
                for (int x = 0; x < columns; x++)
                {
                    auto pixel = bmp.GetPixel(x, y);
                    frm.at(bitOffset++) = pixel.Red;
                    frm.at(bitOffset++) = pixel.Green;
                    frm.at(bitOffset++) = pixel.Blue;
                }
            }
        }
    }
    Cnvt::PutU16(Crc::Crc16_1021(frm.data(), frm.size() - 2), frm.data() + frm.size() - 2);
    char rejectStr[REJECT_BUF_SIZE];
    auto r = ctrller->SignSetFrame(frm.data(), frm.size(), rejectStr);
    reply.emplace("result", (r == APP::ERROR::AppNoError) ? "OK" : rejectStr);
}

void WsServer::CMD_DisplayFrame(struct mg_connection *c, nlohmann::json &msg, json &reply)
{
    uint8_t cmd[3];
    cmd[0] = static_cast<uint8_t>(MI::CODE::SignDisplayFrame);
    cmd[1] = GetUint(msg, "group_id", 1, ctrller->GroupCnt());
    cmd[2] = GetUint(msg, "frame_id", 0, 255);
    auto r = ctrller->CmdDispFrm(cmd);
    reply.emplace("result", (r == APP::ERROR::AppNoError) ? "OK" : APP::ToStr(r));
}

void WsServer::CMD_GetStoredMessage(struct mg_connection *c, json &msg, json &reply)
{
    vector<json> messages;
    auto &ucimsg = DbHelper::Instance().GetUciMsg();
    for (int id = 1; id <= 255; id++)
    {
        auto m = ucimsg.GetMsg(id);
        if (m != nullptr)
        {
            json message;
            message.emplace("id", id);
            message.emplace("revision", m->msgRev);
            message.emplace("transition", m->transTime);
            vector<json> entries(6);
            for (int i = 0; i < 6; i++)
            {
                if (m->msgEntries[i].frmId == 0)
                {
                    entries[i].emplace("id", "");
                    entries[i].emplace("ontime", "");
                }
                else
                {
                    entries[i].emplace("id", std::to_string(m->msgEntries[i].frmId));
                    entries[i].emplace("ontime", std::to_string(m->msgEntries[i].onTime));
                }
            }
            message.emplace("entries", entries);
            messages.push_back(message);
        }
    }
    reply.emplace("messages", messages);
}

void WsServer::CMD_SetMessage(struct mg_connection *c, nlohmann::json &msg, json &reply)
{
    auto entries = GetVector<json>(msg, "entries");
    if (entries.size() != 6)
    {
        throw invalid_argument("Invalid 'entries'");
    }
    vector<uint8_t> cmd(6 * 2 + 4, 0);
    cmd[0] = static_cast<uint8_t>(MI::CODE::SignSetMessage);
    cmd[1] = GetUint(msg, "id", 1, 255);
    cmd[2] = GetUint(msg, "revision", 0, 255);
    cmd[3] = GetUint(msg, "transition", 0, 255);
    uint8_t *p = cmd.data() + 4;
    for (int i = 0; i < 6; i++)
    {
        auto fid = GetUintFromStr(entries[i], "id", 1, 255, false);
        if (fid == -1)
        {
            p++;
            break;
        }
        *p++ = fid;
        *p++ = GetUintFromStr(entries[i], "ontime", 0, 255);
    }
    char rejectStr[64];
    auto r = ctrller->SignSetMessage(cmd.data(), p - cmd.data(), rejectStr);
    reply.emplace("result", (r == APP::ERROR::AppNoError) ? "OK" : rejectStr);
}

void WsServer::CMD_DisplayMessage(struct mg_connection *c, nlohmann::json &msg, json &reply)
{
    uint8_t cmd[3];
    cmd[0] = static_cast<uint8_t>(MI::CODE::SignDisplayMessage);
    cmd[1] = GetUint(msg, "group_id", 1, ctrller->GroupCnt());
    cmd[2] = GetUint(msg, "message_id", 0, 255);
    auto r = ctrller->CmdDispMsg(cmd);
    reply.emplace("result", (r == APP::ERROR::AppNoError) ? "OK" : APP::ToStr(r));
}

void WsServer::CMD_GetStoredPlan(struct mg_connection *c, json &msg, json &reply)
{
    auto grp = ctrller->GetGroups();
    auto &uciplan = DbHelper::Instance().GetUciPln();
    vector<json> plans;
    for (int id = 1; id <= 255; id++)
    {
        auto m = uciplan.GetPln(id);
        if (m != nullptr)
        {
            json plan;
            plan.emplace("id", id);
            plan.emplace("revision", m->plnRev);
            vector<const char *> week;
            for (int i = 0; i < 7; i++)
            {
                if (m->weekdays & MASK_BIT[i])
                {
                    week.emplace_back(WEEKDAY[i]);
                }
            }
            plan.emplace("week", week);
            vector<json> entries(6);
            int i = 0;
            for (int i = 0; i < 6; i++)
            {
                if (m->plnEntries[i].fmType == 0)
                {
                    entries[i].emplace("type", "");
                    entries[i].emplace("id", "");
                    entries[i].emplace("start", "");
                    entries[i].emplace("stop", "");
                }
                else
                {
                    entries[i].emplace("type", m->plnEntries[i].fmType == 1 ? "frame" : "message");
                    entries[i].emplace("id", to_string(m->plnEntries[i].fmId));
                    entries[i].emplace("start", m->plnEntries[i].start.ToString());
                    entries[i].emplace("stop", m->plnEntries[i].stop.ToString());
                }
            }
            plan.emplace("entries", entries);

            vector<string> enabled_group;
            for (auto g : grp)
            {
                if (g->IsPlanEnabled(id))
                {
                    enabled_group.emplace_back(to_string(g->GroupId()));
                }
            }
            plan.emplace("enabled_group", enabled_group);

            plans.push_back(plan);
        }
    }
    reply.emplace("plans", plans);
}

void WsServer::CMD_SetPlan(struct mg_connection *c, nlohmann::json &msg, json &reply)
{
    auto week = GetVector<string>(msg, "week");
    uint8_t bweek = 0;
    for (auto &w : week)
    {
        int d = Pick::PickStr(w.c_str(), WEEKDAY, countof(WEEKDAY), true);
        if (d < 0)
        {
            throw invalid_argument("Invalid 'week'");
        }
        else
        {
            bweek |= 1 << d;
        }
    }
    if (bweek == 0)
    {
        throw invalid_argument("Invalid 'week'");
    }
    auto entries = GetVector<json>(msg, "entries");
    if (entries.size() != 6)
    {
        //        throw invalid_argument("Invalid 'entries'");
    }
    vector<uint8_t> cmd(6 * 6 + 4, 0);
    int id = GetUint(msg, "id", 1, 255);
    cmd[0] = static_cast<uint8_t>(MI::CODE::SignSetPlan);
    cmd[1] = id;
    cmd[2] = GetUint(msg, "revision", 0, 255);
    cmd[3] = bweek;
    uint8_t *p = cmd.data() + 4;
    auto GetHM = [](vector<json> &js, int i, const char *str, uint8_t *p) -> uint8_t *
    {
        string s = GetStr(js[i], str);
        int h, m;
        if (sscanf(s.c_str(), "%d:%d", &h, &m) == 2)
        {
            if (h >= 0 && h <= 59 && m >= 0 && m <= 59)
            {
                *p++ = h;
                *p++ = m;
                return p;
            }
        }
        throw invalid_argument(StrFn::PrintfStr("Invalid %s time in entry[%d]", str, i));
    };
    for (int i = 0; i < entries.size(); i++)
    {
        auto t = GetStr(entries[i], "type", false);
        if (t.length() == 0)
        {
            p++;
            break;
        }
        else if (t.compare("frame") == 0)
        {
            *p++ = 1;
        }
        else if (t.compare("message") == 0)
        {
            *p++ = 2;
        }
        else
        {
            throw invalid_argument(StrFn::PrintfStr("Invalid entry type in entry[%d]:%s", i, t));
        }
        auto fid = GetUintFromStr(entries[i], "id", 0, 255, false);
        if (fid == -1)
        {
            fid = GetUint(entries[i], "id", 0, 255);
            // throw invalid_argument(StrFn::PrintfStr("Invalid F/M id in entry[%d]", i));
        }
        *p++ = (uint8_t)fid;
        p = GetHM(entries, i, "start", p);
        p = GetHM(entries, i, "stop", p);
    }
    if (entries.size() < 6)
    {
        p++;
    }
    char rejectStr[64];
    auto r = ctrller->SignSetPlan(cmd.data(), p - cmd.data(), rejectStr);
    if (r != APP::ERROR::AppNoError)
    {
        reply.emplace("result", rejectStr);
        return;
    }

    vector<int> disable_g;
    vector<int> enable_g;
    auto enabled_group = GetVector<string>(msg, "enabled_group");
    vector<int> egs;
    for (auto eg : enabled_group)
    {
        egs.push_back(strtol(eg.c_str(), nullptr, 10));
    }
    for (auto g : ctrller->GetGroups())
    {
        int gid = g->GroupId();
        if (!egs.empty() && find(egs.begin(), egs.end(), gid) != egs.end())
        {
            enable_g.emplace_back(gid);
        }
        else
        {
            disable_g.emplace_back(gid);
        }
    }

    uint8_t endis_plan[3];
    // disable plan
    if (disable_g.size() > 0)
    {
        endis_plan[0] = static_cast<uint8_t>(MI::CODE::DisablePlan);
        for (auto gi : disable_g)
        {
            auto g = ctrller->GetGroup(gi);
            if (g != nullptr)
            {
                if (g->IsPlanEnabled(id))
                {
                    endis_plan[1] = gi;
                    endis_plan[2] = id;
                    r = ctrller->CmdEnDisPlan(endis_plan);
                    if (r != APP::ERROR::AppNoError)
                    {
                        reply.emplace("result", APP::ToStr(r));
                        return;
                    }
                }
            }
        }
    }

    // enable plan
    if (enable_g.size() > 0)
    {
        endis_plan[0] = static_cast<uint8_t>(MI::CODE::EnablePlan);
        for (auto gi : enable_g)
        {
            auto g = ctrller->GetGroup(gi);
            if (g != nullptr)
            {
                if (!g->IsPlanEnabled(id))
                {
                    endis_plan[1] = gi;
                    endis_plan[2] = id;
                    r = ctrller->CmdEnDisPlan(endis_plan);
                    if (r != APP::ERROR::AppNoError)
                    {
                        reply.emplace("result", APP::ToStr(r));
                        return;
                    }
                }
            }
        }
    }

    // All done
    reply.emplace("result", "OK");
}

void WsServer::CMD_RetrieveFaultLog(struct mg_connection *c, json &msg, json &reply)
{
    DbHelper::Instance().GetUciFault().GetLog(reply);
}

void WsServer::CMD_RetrieveAlarmLog(struct mg_connection *c, json &msg, json &reply)
{
    DbHelper::Instance().GetUciAlarm().GetLog(reply);
}

void WsServer::CMD_RetrieveEventLog(struct mg_connection *c, json &msg, json &reply)
{
    DbHelper::Instance().GetUciEvent().GetLog(reply);
}

void WsServer::cmd_ResetLog(uint8_t logcode, json &reply)
{
    uint8_t cmd[3];
    cmd[0] = static_cast<uint8_t>(MI::CODE::UserDefinedCmdFA);
    cmd[1] = static_cast<uint8_t>(FACMD::FACMD_RESET_LOGS);
    cmd[2] = logcode;
    auto r = ctrller->CmdResetLog(cmd);
    reply.emplace("result", r == APP::ERROR::AppNoError ? "OK" : APP::ToStr(r));
}

void WsServer::CMD_ResetFaultLog(struct mg_connection *c, json &msg, json &reply)
{
    cmd_ResetLog(FACMD_RPL_FLT_LOGS, reply);
}

void WsServer::CMD_ResetAlarmLog(struct mg_connection *c, json &msg, json &reply)
{
    cmd_ResetLog(FACMD_RPL_ALM_LOGS, reply);
}

void WsServer::CMD_ResetEventLog(struct mg_connection *c, json &msg, json &reply)
{
    cmd_ResetLog(FACMD_RPL_EVT_LOGS, reply);
}

void WsServer::CMD_SignTest(struct mg_connection *c, json &msg, json &reply)
{
    uint8_t cmd[4];
    cmd[0] = static_cast<uint8_t>(MI::CODE::UserDefinedCmdFA);
    cmd[1] = static_cast<uint8_t>(FACMD::FACMD_SIGNTEST);
    string colour = GetStr(msg, "colour");
    for (int i = 0; i < MONO_COLOUR_NAME_SIZE; i++)
    {
        if (strcasecmp(FrameColour::COLOUR_NAME[i], colour.c_str()) == 0)
        {
            cmd[2] = i;
        }
    }
    string pixels = GetStr(msg, "pixels");
    for (int i = 0; i < TEST_PIXELS_SIZE; i++)
    {
        if (strcasecmp(TestPixels[i], pixels.c_str()) == 0)
        {
            cmd[3] = i;
        }
    }
    APP::ERROR r = ctrller->CmdSignTest(cmd);
    reply.emplace("result", (r == APP::ERROR::AppNoError) ? "OK" : APP::ToStr(r));
}

void WsServer::CMD_DisplayAtomic(struct mg_connection *c, json &msg, json &reply)
{
    vector<json> content = GetVector<json>(msg, "content");
    int len = content.size();
    vector<uint8_t> cmd(3 + len * 2);
    cmd[0] = static_cast<uint8_t>(MI::CODE::SignDisplayAtomicFrames);
    cmd[1] = GetUint(msg, "group_id", 1, ctrller->GroupCnt());
    cmd[2] = len;
    for (int i = 0; i < len; i++)
    {
        cmd[3 + i * 2] = GetUint(content[i], "sign_id", 1, ctrller->SignCnt());
        cmd[3 + i * 2 + 1] = GetUint(content[i], "frame_id", 1, 255);
    }
    auto r = ctrller->CmdDispAtomicFrm(cmd.data(), cmd.size());
    reply.emplace("result", (r == APP::ERROR::AppNoError) ? "OK" : APP::ToStr(r));
}

void WsServer::CMD_GetFrameCrc(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply)
{
    auto &uci = DbHelper::Instance().GetUciFrm();
    vector<int> crc(256);
    for (int i = 0; i < 256; i++)
    {
        auto item = uci.GetFrm(i);
        crc[i] = (item == nullptr) ? -1 : item->crc;
    }
    reply.emplace("crc", crc);
}

void WsServer::CMD_GetMessageCrc(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply)
{
    auto &uci = DbHelper::Instance().GetUciMsg();
    vector<int> crc(256);
    for (int i = 0; i < 256; i++)
    {
        auto item = uci.GetMsg(i);
        crc[i] = (item == nullptr) ? -1 : item->crc;
    }
    reply.emplace("crc", crc);
}

void WsServer::CMD_GetPlanCrc(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply)
{
    auto &uci = DbHelper::Instance().GetUciPln();
    vector<int> crc(256);
    for (int i = 0; i < 256; i++)
    {
        auto item = uci.GetPln(i);
        crc[i] = (item == nullptr) ? -1 : item->crc;
    }
    reply.emplace("crc", crc);
}

void WsServer::CMD_Reboot(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply)
{
    reply.emplace("result", "Controller will reboot after 5 seconds");
    ctrller->RR_flag(RQST_REBOOT);
}

const char *CFG_BACKUP = "cfg_bak.tar";
const char *CONFIG_GZ_TAR = "config.gz.tar";
const char *CONFIG_MD5 = "config.md5";
const char *CFG_IMPORT = "cfg_imp.tar";

void WsServer::BackupConfig()
{
    char buf[PRINT_BUF_SIZE];
    int len = 0;
    auto CanNotRemove = [&buf](const char *file)
    {
        return snprintf(buf, PRINT_BUF_SIZE - 1, "Can NOT remove old file: %s", file);
    };
    Exec::Shell(TO_NULL("rm %s %s %s"), CFG_BACKUP, CONFIG_GZ_TAR, CONFIG_MD5);
    if (Exec::FileExists(CFG_BACKUP) == true)
    {
        len = CanNotRemove(CFG_BACKUP);
    }
    else if (Exec::FileExists(CONFIG_GZ_TAR) == true)
    {
        len = CanNotRemove(CONFIG_GZ_TAR);
    }
    else if (Exec::FileExists(CONFIG_MD5) == true)
    {
        len = CanNotRemove(CONFIG_MD5);
    }
    else
    {
        auto CanNotCreat = [&buf](const char *file)
        {
            return snprintf(buf, PRINT_BUF_SIZE - 1, "Can NOT creat file: %s", file);
        };
        Exec::Shell("tar -czf %s config/", CONFIG_GZ_TAR);
        if (Exec::FileExists(CONFIG_GZ_TAR) == false)
        {
            len = CanNotCreat(CONFIG_GZ_TAR);
        }
        else
        {
            Exec::Shell("md5sum %s > %s", CONFIG_GZ_TAR, CONFIG_MD5);
            if (Exec::FileExists(CONFIG_MD5) == false)
            {
                len = CanNotCreat(CONFIG_MD5);
            }
            else
            {
                Exec::Shell("tar -cf %s %s %s", CFG_BACKUP, CONFIG_GZ_TAR, CONFIG_MD5);
                if (Exec::FileExists(CFG_BACKUP) == false)
                {
                    len = CanNotCreat(CFG_BACKUP);
                }
            }
        }
    }

    Exec::Shell(TO_NULL("rm %s %s"), CONFIG_GZ_TAR, CONFIG_MD5);
    if (len)
    {
        throw runtime_error(buf);
    }
}

void WsServer::CMD_ExportConfig(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply)
{
    vector<char> vc;
    BackupConfig();
    if (Cnvt::File2Base64(CFG_BACKUP, vc) == 0)
    {
        reply.emplace("result", "OK");
        reply.emplace("file", vc.data());
    }
    else
    {
        throw runtime_error("Can't open config file package");
    }
}

void WsServer::CMD_ImportConfig(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply)
{
    string file = GetStr(msg, "file");
    if (file.size() <= 0)
    {
        throw invalid_argument("Invalid 'file'");
    }
    if (Cnvt::Base64ToFile(file, CFG_IMPORT) < 0)
    {
        throw runtime_error("Saving config file failed");
    }
    BackupConfig();
    char buf[PRINT_BUF_SIZE];
    int len = 0;
    if (Exec::Shell("tar -xf %s", CFG_IMPORT) != 0)
    {
        len = sprintf(buf, "Unpacking %s failed", CFG_IMPORT);
    }
    else if (Exec::Shell(TO_NULL("md5sum -c %s"), CONFIG_MD5) != 0)
    {
        len = sprintf(buf, "MD5 NOT match");
    }
    else if (Exec::Shell("tar -xzf %s", CONFIG_GZ_TAR) != 0)
    {
        len = sprintf(buf, "Unpacking %s failed", CONFIG_GZ_TAR);
    }
    Exec::Shell(TO_NULL("rm %s %s"), CONFIG_GZ_TAR, CONFIG_MD5);
    if (len > 0)
    {
        throw runtime_error(buf);
    }
    DbHelper::Instance().GetUciEvent().Push(0, "Import configuration");
    reply.emplace("result", "Controller will restart after 5 seconds");
    ctrller->RR_flag(RQST_RESTART);
}

void WsServer::CMD_BackupFirmware(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply)
{
    Upgrade::BackupFirmware();
    vector<char> vc;
    if (Exec::FileExists(UFILE) && Cnvt::File2Base64(UFILE, vc) == 0)
    {
        reply.emplace("result", "OK");
        reply.emplace("file", vc.data());
    }
    else
    {
        throw runtime_error("Creating firmware file package failed");
    }
}

void WsServer::CMD_UpgradeFirmware(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply)
{
    string file = GetStr(msg, "file");
    if (file.size() <= 0)
    {
        throw invalid_argument("Invalid 'file'");
    }
    char gufile[STRLOG_SIZE];
    snprintf(gufile, STRLOG_SIZE - 1, "%s/%s", GDIR, UFILE);
    if (Cnvt::Base64ToFile(file, gufile) < 0)
    {
        throw runtime_error(StrFn::PrintfStr("Saving %s failed", gufile));
    }

    Upgrade::BackupFirmware();

    char buf[PRINT_BUF_SIZE];
    char md5[33];
    if (Upgrade::UnpackFirmware(buf, md5) == 0)
    {
        DbHelper::Instance().GetUciEvent().Push(0, buf);
        reply.emplace("result", "Controller will restart after 5 seconds");
        ctrller->RR_flag(RQST_RESTART);
    }
    else
    {
        reply.emplace("result", buf);
    }
    DebugLog(buf);
}

void WsServer::CMD_TestTMC(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply)
{
    string text;
    text.reserve(64 * 1024);
    while (qltdTmc->size() > 0)
    {
        text.append(qltdTmc->PopBack());
        text.append("\n");
    }
    reply.emplace("text", text);
}

void WsServer::CMD_TestBufLenTMC(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply)
{
    auto len = GetUint(msg, "len", 1, 255);
    qltdTmc->Resize(len);
    reply.emplace("result", "OK");
}

void WsServer::CMD_TestSlave(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply)
{
    string text;
    text.reserve(64 * 1024);
    while (qltdSlave->size() > 0)
    {
        text.append(qltdSlave->PopBack());
        text.append("\n");
    }
    reply.emplace("text", text);
}

void WsServer::CMD_TestBufLenSlave(struct mg_connection *c, nlohmann::json &msg, nlohmann::json &reply)
{
    auto len = GetUint(msg, "len", 1, 255);
    qltdSlave->Resize(len);
    reply.emplace("result", "OK");
}

void WsServer::CMD_GetExtInput(struct mg_connection *c, json &msg, json &reply)
{
    auto &usercfg = DbHelper::Instance().GetUciUserCfg();
    vector<json> entries(usercfg.EXT_SIZE);
    for (int i = 0; i < usercfg.EXT_SIZE; i++)
    {
        auto &x = usercfg.ExtInputCfgX(i);
        auto &en = entries.at(i);
        en.emplace("input_id", i + 1);
        en.emplace("display_time", x.dispTime);
        en.emplace("reserved_byte", x.reserved);
        en.emplace("emergency", x.emergency);
        en.emplace("flashing_override", x.flashingOv);
    }
    reply.emplace("ExtInput", entries);
}

void WsServer::CMD_SetExtInput(struct mg_connection *c, json &msg, json &reply)
{
    auto &usercfg = DbHelper::Instance().GetUciUserCfg();
    vector<ExtInput> extin(usercfg.EXT_SIZE);
    vector<int> ids(usercfg.EXT_SIZE, 0);
    auto entries = GetVector<json>(msg, "ExtInput");
    if (entries.size() != usercfg.EXT_SIZE)
    {
        throw invalid_argument(StrFn::PrintfStr("There should be %d entries in ExtInput", usercfg.EXT_SIZE));
    }
    for (auto &en : entries)
    {
        auto id = GetUint(en, "input_id", 1, usercfg.EXT_SIZE) - 1;
        ids.at(id) = 1;
        auto &in = extin.at(id);
        in.dispTime = GetUint(en, "display_time", 0, 65535);
        in.reserved = GetUint(en, "reserved_byte", 0, 255);
        in.emergency = GetUint(en, "emergency", 0, 1);
        in.flashingOv = GetUint(en, "flashing_override", 0, 1);
    }
    for (int i = 0; i < usercfg.EXT_SIZE; i++)
    {
        if (ids.at(i) == 0)
        {
            throw invalid_argument(StrFn::PrintfStr("Missing entry: \"input_id\":%d", i + 1));
        }
    }
    for (int i = 0; i < usercfg.EXT_SIZE; i++)
    {
        usercfg.ExtInputCfgX(i, extin.at(i));
    }
    auto &evt = DbHelper::Instance().GetUciEvent();
    evt.Push(0, "UserCfg.ExtInput[1~%d] changed", usercfg.EXT_SIZE);
    reply.emplace("result", "OK");
}

// Copyright (c) 2020 Cesanta Software Limited
// All rights reserved
//
// Example Websocket server. See https://mongoose.ws/tutorials/websocket-server/
#include <websocket/WsServer.h>

#include <module/MyDbg.h>
#include <uci/DbHelper.h>
#include <module/Utils.h>

unsigned int ws_hexdump = 0;

const char *WsServer::uri_ws = "/ws";

using json = nlohmann::json;
using namespace Utils;
using namespace std;

Controller *WsServer::ctrller;

map<struct mg_connection *, WsMsg *> WsServer::wsMsg;

WsServer::WsServer(int port, TimerEvent *tmrEvt)
    : tmrEvt(tmrEvt)
{
    if (port < 1024 || port > 65535)
    {
        throw invalid_argument(FmtException("WsServer error: port: %d", port));
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
        if (mg_http_match_uri(hm, uri_ws))
        {
            // Upgrade to websocket. From now on, a connection is a full-duplex
            // Websocket connection, which will receive MG_EV_WS_MSG events.
            mg_ws_upgrade(c, hm, NULL);
            uint8_t *ip = (uint8_t *)&c->rem.ip;
            Ldebug("Websocket '%s' connected from %d.%d.%d.%d, ID=%lu", uri_ws, ip[0], ip[1], ip[2], ip[3], c->id);
            wsMsg[c] = new WsMsg();
            c->fn_data = (void *)uri_ws;
        }
    }
    else if (ev == MG_EV_WS_MSG)
    {
        // Got websocket frame
        struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
        if (wm->data.len > 0)
        {
            if (c->fn_data == (void *)uri_ws)
            {
                VMSWebSokectProtocol(c, wm);
            }
        }
    }
    else if (ev == MG_EV_CLOSE)
    {
        if (c->fn_data == uri_ws)
        {
            uint8_t *ip = (uint8_t *)&c->rem.ip;
            Ldebug("Websocket '%s' disconnected from %d.%d.%d.%d, ID=%lu", uri_ws, ip[0], ip[1], ip[2], ip[3], c->id);
            delete wsMsg[c];
            wsMsg.erase(c);
        }
    }
    //(void)fn_data;
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

int WsServer::GetInt(json &msg, const char *str, int min, int max)
{
    auto x = msg[str].get<int>();
    if (x < min || x > max)
    {
        throw "Invalid '" + string(str) + "'";
    }
    return x;
}

int WsServer::GetStrInt(json &msg, const char *str, int min, int max)
{
    auto x = stoi(msg[str].get<string>(), nullptr, 0);
    if (x < min || x > max)
    {
        throw "Invalid '" + string(str) + "'";
    }
    return x;
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
    CMD_ITEM(SetFrame),
    CMD_ITEM(DisplayFrame),
    CMD_ITEM(GetStoredMessage),
    CMD_ITEM(SetMessage),
    CMD_ITEM(DisplayMessage),
    CMD_ITEM(GetStoredPlan),
    CMD_ITEM(SetFrame),
    CMD_ITEM(RetrieveFaultLog),
    CMD_ITEM(RetrieveAlarmLog),
    CMD_ITEM(RetrieveEventLog),
    CMD_ITEM(ResetFaultLog),
    CMD_ITEM(ResetAlarmLog),
    CMD_ITEM(ResetEventLog),
    CMD_ITEM(SignTest),
    CMD_ITEM(DispAtomic),
    CMD_ITEM(GetFrameCrc),
    CMD_ITEM(GetMessageCrc),
    CMD_ITEM(GetPlanCrc),
};

void WsServer::VMSWebSokectProtocol(struct mg_connection *c, struct mg_ws_message *wm)
{
    if ((wsMsg[c]->len + wm->data.len) >= WsMsgBuf_SIZE)
    {
        wsMsg[c]->len = 0;
        if (wm->data.len >= WsMsgBuf_SIZE)
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

    if (json::accept(p, true))
    {
        wsMsg[c]->len = 0; // clear msgbuf
    }
    else
    {
        if (p != wsMsg[c]->buf && json::accept(wsMsg[c]->buf, true))
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
        json reply;
        string cmd;
        try
        {
            json msg = json::parse(p, nullptr, true, true);
            cmd = msg["cmd"].get<string>();
            reply.emplace("cmd", cmd);
            int j = countof(CMD_LIST);
            // int j = (wsMsg[c]->login) ? countof(CMD_LIST) : 1;
            for (int i = 0; i < j; i++)
            {
                if (strcasecmp(cmd.c_str(), CMD_LIST[i].cmd) == 0)
                {
                    CMD_LIST[i].function(c, msg, reply);
                    my_mg_ws_send(c, reply);
                    return;
                }
            }
            reply.emplace("result", "Unknown cmd:" + cmd);
        }
        catch (exception &e)
        {
            reply.emplace("result", e.what());
        }
        my_mg_ws_send(c, reply);
    }
}

void WsServer::CMD_Login(struct mg_connection *c, json &msg, json &reply)
{
    const char *r;
    auto msgp = msg["password"].get<string>();
    auto msgu = msg["user"].get<string>();
    if (msgu.compare("Administrator") == 0)
    {
        wsMsg[c]->login = (msgp.compare("Br1ghtw@y") == 0);
        r = wsMsg[c]->login ? "OK" : "Wrong password";
    }
    else
    {
        auto ps = DbHelper::Instance().GetUciPasswd().GetUserPasswd(msgu);
        if (ps == nullptr)
        {
            r = "Invalid user";
        }
        else
        {
            wsMsg[c]->login = (msgp.compare(DbHelper::Instance().GetUciPasswd().GetUserPasswd(msgu)->passwd) == 0);
            r = wsMsg[c]->login ? "OK" : "Wrong password";
        }
    }
    reply.emplace("user", msgu);
    reply.emplace("result", r);
}

void WsServer::CMD_ChangePassword(struct mg_connection *c, json &msg, json &reply)
{
    auto msgu = msg["user"].get<string>();
    auto cp = msg["current"].get<string>();
    auto np = msg["new"].get<string>();
    const char *r;
    if (np.length() < 4 || np.length() > 10)
    {
        r = "Length of password should be 4 - 10";
    }
    else
    {
        auto &up = DbHelper::Instance().GetUciPasswd();
        auto ps = up.GetUserPasswd(msgu);
        if (ps == nullptr)
        {
            r = "Invalid user";
        }
        if (cp.compare(ps->passwd) == 0)
        {
            bool invalid = false;
            for (auto s : np)
            {
                if (s <= 0x20 || s == '"' || s == '\'' || s == '\\' || s >= 0x7F)
                {
                    invalid = true;
                    break;
                }
            }
            if (invalid)
            {
                r = "result", "Invalid character in new password";
            }
            else
            {
                up.Set(msgu, np, ps->permission);
                r = "OK";
            }
        }
        else
        {
            r = "Current password NOT matched";
        }
    }
    reply.emplace("result", r);
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
    // SO changing group configuration should directly edit "config/UciProd" file.
    reply.emplace("result", "Unsupported command");
    return;
#if 0
    auto gc = ctrller->GroupCnt();
    auto vgroups = msg["groups"].get<vector<json>>();
    if (vgroups.size() != gc)
    {
        throw "Missing group config";
    }
    vector<int> vgid(gc);
    vgid.assign(gc, 0);
    auto sc = ctrller->SignCnt();
    vector<int> vsign_gid(sc);
    vsign_gid.assign(sc, 0);
    for (int i = 0; i < gc; i++)
    {
        auto gid = GetInt(vgroups[i], "group_id", 1, gc);
        if (vgid[gid - 1] > 0)
        {
            throw FmtException("Repeated group id [%d]", gid);
        }
        vgid[gid - 1] = gid;
        auto vsid = vgroups[i].get<vector<int>>();
        if (vsid.size() == 0)
        {
            throw FmtException("Missing sign id in group[%d]", gid);
        }
        for (auto id : vsid)
        {
            if (id <= 0 || id > sc)
            {
                throw FmtException("Invalid sign id [%d]", id);
            }
            if (vsign_gid[id - 1] == 0)
            {
                vsign_gid[id - 1] = gid;
            }
            else
            {
                throw FmtException("Repeated sign id [%d]", id);
            }
        }
    }
    for (int i = 0; i < vgid.size(); i++)
    {
        if (vgid[i] == 0)
        {
            throw FmtException("Missing group[%d] config", i + 1);
        }
    }
    for (int i = 0; i < vsign_gid.size(); i++)
    {
        if (vsign_gid[i] == 0)
        {
            throw FmtException("Missing sign[%d] config", i + 1);
        }
    }
#endif
}

extern const char *FirmwareVer;
void WsServer::CMD_GetStatus(struct mg_connection *c, json &msg, json &reply)
{
    char buf[64];
    reply.emplace("manufacturer_code", DbHelper::Instance().GetUciProd().MfcCode());
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
    reply.emplace("tmc_com_port", COM_NAME[usercfg.ComPort()]);
    reply.emplace("baudrate", usercfg.Baudrate());
    reply.emplace("multiled_fault", usercfg.MultiLedFaultThreshold());
    reply.emplace("tmc_tcp_port", usercfg.SvcPort());
    reply.emplace("over_temp", usercfg.OverTemp());
    reply.emplace("locked_frame", usercfg.LockedFrm());
    reply.emplace("locked_msg", usercfg.LockedMsg());
    reply.emplace("city", usercfg.City());
    reply.emplace("last_frame_time", usercfg.LastFrmTime());
}

void WsServer::CMD_SetUserConfig(struct mg_connection *c, json &msg, json &reply)
{
    unsigned char rr_flag = 0;
    auto device_id = GetInt(msg, "device_id", 0, 255);
    auto broadcast_id = GetInt(msg, "broadcast_id", 0, 255);
    if (device_id == broadcast_id)
    {
        throw string("device_id should not equal to broadcast_id");
    }
    auto session_timeout = GetInt(msg, "session_timeout", 0, 65535);
    auto display_timeout = GetInt(msg, "display_timeout", 0, 65535);
    auto baudrate = GetInt(msg, "baudrate", 19200, 115200);
    auto multiled_fault = GetInt(msg, "multiled_fault", 0, 255);
    auto tmc_tcp_port = GetInt(msg, "tmc_tcp_port", 1024, 65535);
    auto over_temp = GetInt(msg, "over_temp", 0, 99);
    auto locked_frame = GetInt(msg, "locked_frame", 0, 255);
    auto locked_msg = GetInt(msg, "locked_msg", 0, 255);
    auto last_frame_time = GetInt(msg, "last_frame_time", 0, 255);

    auto seed = GetStrInt(msg, "seed", 0, 0xFF);
    auto password = GetStrInt(msg, "password", 0, 0xFF);

    int tmc_com_port = -1;
    auto tmc_com_port_str = msg["tmc_com_port"].get<string>();
    for (int i = 0; i < COMPORT_SIZE; i++)
    {
        if (strcasecmp(tmc_com_port_str.c_str(), COM_NAME[i]) == 0)
        {
            tmc_com_port = i;
            break;
        }
    }
    if (tmc_com_port == -1)
    {
        throw string("'tmc_com_port' NOT valid");
    }
    int city = -1;
    auto city_str = msg["city"].get<string>();
    for (int i = 0; i < NUMBER_OF_TZ; i++)
    {
        if (strcasecmp(city_str.c_str(), Tz_AU::tz_au[i].city) == 0)
        {
            city = i;
            break;
        }
    }
    if (city == -1)
    {
        throw string("'city' NOT valid");
    }
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

    if (baudrate != usercfg.Baudrate())
    {
        evt.Push(0, "User.Baudrate changed: %u->%u . Restart to load new setting",
                 usercfg.Baudrate(), baudrate);
        usercfg.Baudrate(baudrate);
        rr_flag |= RQST_RESTART;
    }

    if (tmc_tcp_port != usercfg.SvcPort())
    {
        evt.Push(0, "User.SvcPort changed: %u->%u. Restart to load new setting",
                 usercfg.SvcPort(), tmc_tcp_port);
        usercfg.SvcPort(tmc_tcp_port);
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
    reply.emplace("result", (rr_flag != 0) ? "'Reboot' to active new setting" : "OK");
}

void WsServer::CMD_GetDimmingConfig(struct mg_connection *c, json &msg, json &reply)
{
    auto &usercfg = DbHelper::Instance().GetUciUserCfg();
    reply.emplace("night_level", usercfg.NightDimmingLevel());
    reply.emplace("dawn_dusk_level", usercfg.DawnDimmingLevel());
    reply.emplace("day_level", usercfg.DayDimmingLevel());
    reply.emplace("night_max_lux", usercfg.LuxNightMax());
    reply.emplace("day_min_lux", usercfg.LuxDayMin());
    reply.emplace("18_hours_min_lux", usercfg.Lux18HoursMin());
}

void WsServer::CMD_SetDimmingConfig(struct mg_connection *c, json &msg, json &reply)
{
    auto night_level = GetInt(msg, "night_level", 1, 8);
    auto dawn_dusk_level = GetInt(msg, "dawn_dusk_level", night_level + 1, 15);
    auto day_level = GetInt(msg, "day_level", dawn_dusk_level + 1, 16);
    auto night_max_lux = GetInt(msg, "night_max_lux", 1, 9999);
    auto day_min_lux = GetInt(msg, "day_min_lux", night_max_lux + 1, 65535);
    auto _18_hours_min_lux = GetInt(msg, "18_hours_min_lux", day_min_lux + 1, 65535);
    auto &usercfg = DbHelper::Instance().GetUciUserCfg();
    auto &evt = DbHelper::Instance().GetUciEvent();
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
    if (_18_hours_min_lux != usercfg.Lux18HoursMin())
    {
        evt.Push(0, "User.Lux18HoursMin changed: %d->%d", usercfg.Lux18HoursMin(), _18_hours_min_lux);
        usercfg.Lux18HoursMin(_18_hours_min_lux);
    }
    reply.emplace("result", "OK");
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
                throw "Invalid " + ethname[i] + ".ipaddr";
            }
            if (interfaces[i].netmask.Set(ethjs[i][net._Netmask].get<std::string>()) == false)
            {
                throw "Invalid " + ethname[i] + ".netmask";
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
            if (str_gateway.length() > 0)
            {
                if (interfaces[i].netmask.Set(str_gateway) == false)
                {
                    throw "Invalid " + ethname[i] + ".gateway";
                }
            }
        }
        else if (interfaces[i].proto.compare("dhcp") != 0)
        {
            throw "Invalid " + ethname[i] + ".proto";
        }
    }

    json ntpjs = msg["NTP"].get<json>();
    NtpServer ntp;
    ntp.server = ntpjs[net._Server].get<string>();
    ntp.port = GetInt(ntpjs, net._Port, 1, 65535);

    if (interfaces[0].gateway.Isvalid() && interfaces[1].gateway.Isvalid())
    {
        throw "Only one ETH can have gateway";
    }
    if (memcmp(interfaces[0].ipaddr.ip, interfaces[1].ipaddr.ip, 4) == 0)
    {
        throw "ETH1.ipaddr and ETH2.ipaddr should not be same";
    }
    if (ntp.server.compare(interfaces[0].ipaddr.ToString()) == 0 || ntp.server.compare(interfaces[1].ipaddr.ToString()) == 0)
    {
        throw "NTP Server should not be ETH1/2";
    }

    // all parameters are OK, save
    for (int i = 0; i < 2; i++)
    {
        memcpy(net.GetETH(ethname[i]), &interfaces[i], sizeof(NetInterface));
        net.SaveETH(ethname[i]);
    }
    memcpy(net.GetNtp(), &ntp, sizeof(NtpServer));
    net.SaveNtp();

    reply.emplace("result", "'Reboot' to active new setting");
}

void WsServer::CMD_ControlDimming(struct mg_connection *c, json &msg, json &reply)
{
    auto grp = msg["groups"].get<vector<int>>();
    auto setting = GetInt(msg, "setting", 0, 16);
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
}

void WsServer::CMD_ControlPower(struct mg_connection *c, json &msg, json &reply)
{
    auto grp = msg["groups"].get<vector<int>>();
    auto setting = GetInt(msg, "setting", 0, 1);
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
}

void WsServer::CMD_ControlDevice(struct mg_connection *c, json &msg, json &reply)
{
    auto grp = msg["groups"].get<vector<int>>();
    auto setting = GetInt(msg, "setting", 0, 1);
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
}

void WsServer::CMD_SystemReset(struct mg_connection *c, json &msg, json &reply)
{
    uint8_t cmd[3];
    cmd[1] = GetInt(msg, "group_id", 0, ctrller->GroupCnt());
    cmd[2] = GetInt(msg, "level", 0, 255);
    char buf[64];
    reply.emplace("result", (ctrller->CmdSystemReset(cmd, buf) == APP::ERROR::AppNoError) ? "OK" : buf);
}

void WsServer::CMD_UpdateTime(struct mg_connection *c, json &msg, json &reply)
{
    auto cmd = msg["rtc"].get<string>();
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
}

void WsServer::CMD_GetFrameSetting(struct mg_connection *c, json &msg, json &reply)
{
    auto &prod = DbHelper::Instance().GetUciProd();
    vector<string> frametype{"Text Frame"};
    if (prod.PixelRows() < 255 && prod.PixelColumns() < 255)
    {
        frametype.push_back("Graphics Frame");
    }
    frametype.push_back("HR Graphics Frame");
    reply.emplace("frame_type", frametype);

    vector<string> txt_c;
    for (int i = 0; i < MONO_COLOUR_NAME_SIZE; i++)
    {
        if (prod.IsTxtFrmColourValid(i))
        {
            txt_c.push_back(string(FrameColour::COLOUR_NAME[i]));
        }
    }
    reply.emplace("txt_frame_colours", txt_c);

    vector<string> gfx_c;
    for (int i = 0; i < MONO_COLOUR_NAME_SIZE; i++)
    {
        if (prod.IsGfxFrmColourValid(i))
        {
            gfx_c.push_back(string(FrameColour::COLOUR_NAME[i]));
        }
    }
    if (prod.IsGfxFrmColourValid(static_cast<int>(FRMCOLOUR::Multi_4bit)))
    {
        gfx_c.push_back(string("Multi(4-bit)"));
    }
    if (prod.IsGfxFrmColourValid(static_cast<int>(FRMCOLOUR::RGB_24bit)))
    {
        gfx_c.push_back(string("RGB(24-bit)"));
    }
    reply.emplace("gfx_frame_colours", gfx_c);

    vector<string> hrg_c;
    for (int i = 0; i < MONO_COLOUR_NAME_SIZE; i++)
    {
        if (prod.IsHrgFrmColourValid(i))
        {
            hrg_c.push_back(string(FrameColour::COLOUR_NAME[i]));
        }
    }
    if (prod.IsHrgFrmColourValid(static_cast<int>(FRMCOLOUR::Multi_4bit)))
    {
        hrg_c.push_back(string("Multi(4-bit)"));
    }
    if (prod.IsHrgFrmColourValid(static_cast<int>(FRMCOLOUR::RGB_24bit)))
    {
        hrg_c.push_back(string("RGB(24-bit)"));
    }
    reply.emplace("hrg_frame_colours", hrg_c);

    vector<int> fonts;
    for (int i = 0; i < prod.MaxFont(); i++)
    {
        if (prod.IsFont(i))
        {
            fonts.push_back(i);
        }
    }
    reply.emplace("fonts", fonts);

    vector<int> txt_columns;
    vector<int> txt_rows;
    for (auto f : fonts)
    {
        txt_columns.emplace_back(prod.CharColumns(f));
        txt_rows.emplace_back(prod.CharRows(f));
    }
    reply.emplace("txt_columns", txt_columns);
    reply.emplace("txt_rows", txt_rows);

    vector<string> conspicuity;
    for (int i = 0; i < prod.MaxConspicuity(); i++)
    {
        if (prod.IsConspicuity(i))
        {
            conspicuity.push_back(string(Conspicuity[i]));
        }
    }
    reply.emplace("conspicuity", conspicuity);

    vector<string> annulus;
    for (int i = 0; i < prod.MaxAnnulus(); i++)
    {
        if (prod.IsAnnulus(i))
        {
            annulus.push_back(string(Annulus[i]));
        }
    }
    reply.emplace("annulus", annulus);
}

void WsServer::CMD_GetStoredFrame(struct mg_connection *c, json &msg, json &reply)
{
    auto id = GetInt(msg, "id", 1, 255);
    reply.emplace("id", id);
    auto &ucifrm = DbHelper::Instance().GetUciFrm();
    auto frm = ucifrm.GetFrm(id);
    if (frm == nullptr)
    {
        reply.emplace("text", "UNDEFINED");
    }
    else
    {
        reply.emplace("revision", frm->frmRev);
        reply.emplace("colour", FrameColour::GetColourName(frm->colour));
        reply.emplace("conspicuity", Conspicuity[frm->conspicuity & 0x07]);
        reply.emplace("annulus", Annulus[(frm->conspicuity >> 3) & 0x03]);
        if (frm->micode == 0x0A) // SignSetTextFrame
        {
            reply.emplace("type", "Text Frame");
            auto &rawdata = frm->stFrm.rawData;
            reply.emplace("font", rawdata[3]);
            vector<char> txt(rawdata[6] + 1);
            memcpy(txt.data(), rawdata.data() + 7, rawdata[6]);
            txt.back() = '\0';
            reply.emplace("text", txt.data());
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
    auto &prod = DbHelper::Instance().GetUciProd();
    string str;
    auto id = GetInt(msg, "id", 1, 255);
    auto rev = GetInt(msg, "revision", 1, 255);
    const char *FRM_TYPE[] = {"Text Frame", "Graphics Frame", "HR Graphics Frame"};
    str = msg["type"].get<std::string>();
    int ftype = Pick::PickStr(str.c_str(), FRM_TYPE, countof(FRM_TYPE), true);
    if (ftype < 0)
    {
        throw string("CMD_SetFrame: type error");
    }
    MI::CODE FrmType[3]{MI::CODE::SignSetTextFrame, MI::CODE::SignSetGraphicsFrame, MI::CODE::SignSetHighResolutionGraphicsFrame};
    MI::CODE frmType = FrmType[ftype];
    str = msg["colour"].get<std::string>();
    int colour = Pick::PickStr(str.c_str(), FrameColour::COLOUR_NAME, ALL_COLOUR_NAME_SIZE, true);
    if (colour < 0)
    {
        throw string("CMD_SetFrame: colour error");
    }
    if (frmType == MI::CODE::SignSetTextFrame && colour > 9)
    {
        throw string("CMD_SetFrame: colour error(Text Frame)");
    }
    if (frmType == MI::CODE::SignSetGraphicsFrame && colour > 10)
    {
        throw string("CMD_SetFrame: colour error(Graphics Frame)");
    }
    str = msg["conspicuity"].get<std::string>();
    int conspicuity = Pick::PickStr(str.c_str(), Conspicuity, CONSPICUITY_SIZE, true);
    if (conspicuity < 0)
    {
        throw string("CMD_SetFrame: conspicuity error");
    }
    str = msg["annulus"].get<std::string>();
    int annulus = Pick::PickStr(str.c_str(), Annulus, ANNULUS_SIZE, true);
    if (annulus < 0)
    {
        throw string("CMD_SetFrame: annulus error");
    }
    conspicuity = (conspicuity << 3) | annulus;
    char rejectStr[64];
    vector<uint8_t> frm;
    if (frmType == MI::CODE::SignSetTextFrame)
    {
        str = msg["text"].get<std::string>();
        if (str.length() < 1)
        {
            throw "'Text' is null";
        }
        frm.resize(str.length() + 9);
        frm[0] = static_cast<uint8_t>(frmType);
        frm[1] = id;
        frm[2] = rev;
        frm[3] = GetInt(msg, "font", 0, 255);
        frm[4] = colour;
        frm[5] = conspicuity;
        frm[6] = str.length();
        memcpy(frm.data() + 7, str.data(), str.length());
    }
    else
    {
        str = msg["image"].get<std::string>();
        unique_ptr<FrameImage> frmImg(new FrameImage);
        frmImg->LoadBmpFromBase64(str.c_str(), str.length());
        auto &bmp = frmImg->GetBmp();
        auto rows = prod.PixelRows();
        auto columns = prod.PixelColumns();
        if (bmp.TellHeight() != rows || bmp.TellWidth() != columns)
        {
            throw "Image size NOT matched with sign";
        }
        int corelen;
        if (colour <= 9)
        { // mono
            corelen = prod.Gfx1CoreLen();
        }
        if (colour == 10)
        {
            colour = static_cast<uint8_t>(FRMCOLOUR::Multi_4bit);
            corelen = prod.Gfx4CoreLen();
        }
        else if (colour == 11)
        {
            colour = static_cast<uint8_t>(FRMCOLOUR::RGB_24bit);
            corelen = prod.Gfx24CoreLen();
        }
        int f_offset;
        if (rows < 255 && columns < 255)
        {
            f_offset = 9;
            frm.resize(corelen + f_offset + 2);
            frm[3] = rows;
            frm[4] = columns;
            frm[5] = colour;
            frm[6] = 0;
            Utils::Cnvt::PutU16(corelen, frm.data() + 7);
        }
        else
        {
            f_offset = 13;
            frm.resize(corelen + f_offset + 2);
            Cnvt::PutU16(rows, frm.data() + 3);
            Cnvt::PutU16(columns, frm.data() + 5);
            frm[7] = colour;
            frm[8] = 0;
            Utils::Cnvt::PutU32(corelen, frm.data() + 9);
        }
        frm[0] = static_cast<uint8_t>(frmType);
        frm[1] = 255;
        frm[2] = 255;
        int bitOffset = 0;
        if (colour <= 9)
        { // mono
            uint8_t *core = frm.data() + f_offset;
            memset(core, 0, corelen);
            int bitOffset = 0;
            for (int j = 0; j < rows; j++)
            {
                for (int i = 0; i < columns; i++)
                {
                    auto pixel = bmp.GetPixel(i, j);
                    if (pixel.Blue > 0 || pixel.Green > 0 || pixel.Red > 0)
                    {
                        BitOffset::Set70Bit(core, bitOffset++);
                    }
                }
            }
        }
        else if (colour == static_cast<uint8_t>(FRMCOLOUR::Multi_4bit))
        {
            for (int j = 0; j < rows; j++)
            {
                for (int i = 0; i < columns; i++)
                {
                    auto pixel = bmp.GetPixel(i, j);
                    uint8_t c = FrameColour::Rgb2Colour(((pixel.Red * 0x100) + pixel.Green) * 0x100 + pixel.Blue);
                    int x = f_offset + bitOffset / 2;
                    frm.at(x) |= (bitOffset & 1) ? (c << 4) : c;
                    bitOffset++;
                }
            }
        }
        else if (colour == static_cast<uint8_t>(FRMCOLOUR::RGB_24bit))
        {
            bitOffset = f_offset;
            for (int j = 0; j < rows; j++)
            {
                for (int i = 0; i < columns; i++)
                {
                    auto pixel = bmp.GetPixel(i, j);
                    frm.at(bitOffset++) = pixel.Red;
                    frm.at(bitOffset++) = pixel.Green;
                    frm.at(bitOffset++) = pixel.Blue;
                }
            }
        }
    }
    Cnvt::PutU16(Crc::Crc16_1021(frm.data(), frm.size() - 2), frm.data() + frm.size() - 2);
    auto r = ctrller->SignSetFrame(frm.data(), frm.size(), rejectStr);
    reply.emplace("result", (r == APP::ERROR::AppNoError) ? "OK" : rejectStr);
}

void WsServer::CMD_DisplayFrame(struct mg_connection *c, nlohmann::json &msg, json &reply)
{
    uint8_t cmd[3];
    cmd[1] = GetInt(msg, "group_id", 1, ctrller->GroupCnt());
    cmd[1] = GetInt(msg, "frame_id", 0, 255);
    auto r = ctrller->CmdDispFrm(cmd);
    reply.emplace("result", (r == APP::ERROR::AppNoError) ? "OK" : APP::ToStr(r));
}

void WsServer::CMD_GetStoredMessage(struct mg_connection *c, json &msg, json &reply)
{
    auto id = GetInt(msg, "id", 1, 255);
    reply.emplace("id", id);
    auto m = DbHelper::Instance().GetUciMsg().GetMsg(id);
    if (m == nullptr)
    {
        reply.emplace("result", "UNDEFINED");
    }
    else
    {
        reply.emplace("result", "OK");
        reply.emplace("revision", m->msgRev);
        reply.emplace("transition", m->transTime);
        vector<json> entries(m->entries);
        for (int i = 0; i < m->entries; i++)
        {
            entries[i].emplace("id", m->msgEntries[i].frmId);
            entries[i].emplace("ontime", m->msgEntries[i].onTime);
        }
        reply.emplace("entries", entries);
    }
}

void WsServer::CMD_SetMessage(struct mg_connection *c, nlohmann::json &msg, json &reply)
{
    auto entries = msg["entries"].get<vector<json>>();
    if (entries.size() == 0)
    {
        throw "There is no etry";
    }
    vector<uint8_t> cmd(entries.size() * 2 + 4 + (entries.size() == 6 ? 0 : 1));
    cmd.back() = 0;
    cmd[1] = GetInt(msg, "id", 1, 255);
    cmd[2] = GetInt(msg, "revision", 0, 255);
    cmd[3] = GetInt(msg, "transition", 0, 255);
    uint8_t *p = cmd.data() + 4;
    for (int i = 0; i < entries.size(); i++)
    {
        *p++ = GetInt(entries[i], "id", 1, 255);
        *p++ = GetInt(entries[i], "ontime", 0, 255);
    }
    char rejectStr[64];
    auto r = ctrller->SignSetMessage(cmd.data(), cmd.size(), rejectStr);
    reply.emplace("result", (r == APP::ERROR::AppNoError) ? "OK" : rejectStr);
}

void WsServer::CMD_DisplayMessage(struct mg_connection *c, nlohmann::json &msg, json &reply)
{
    uint8_t cmd[3];
    cmd[1] = GetInt(msg, "group_id", 1, ctrller->GroupCnt());
    cmd[2] = GetInt(msg, "message_id", 1, 255);
    auto r = ctrller->CmdDispMsg(cmd);
    reply.emplace("result", (r == APP::ERROR::AppNoError) ? "OK" : APP::ToStr(r));
}

void WsServer::CMD_GetStoredPlan(struct mg_connection *c, json &msg, json &reply)
{
    auto id = GetInt(msg, "id", 1, 255);
    reply.emplace("id", id);
    auto m = DbHelper::Instance().GetUciPln().GetPln(id);
    if (m == nullptr)
    {
        reply.emplace("result", "UNDEFINED");
    }
    else
    {
        reply.emplace("result", "OK");
        reply.emplace("revision", m->plnRev);
        vector<const char *> week;
        for (int i = 0; i < 7; i++)
        {
            if (m->weekdays & MASK_BIT[i])
            {
                week.emplace_back(WEEKDAY[i]);
            }
        }
        reply.emplace("week", week);
        vector<json> entries(m->entries);
        for (int i = 0; i < m->entries; i++)
        {
            entries[i].emplace("type", m->plnEntries[i].fmType == 1 ? "frame" : "message");
            entries[i].emplace("id", m->plnEntries[i].fmId);
            char buf[64];
            sprintf(buf, "%d:%02d", m->plnEntries[i].start.hour, m->plnEntries[i].start.min);
            entries[i].emplace("start", buf);
            sprintf(buf, "%d:%02d", m->plnEntries[i].stop.hour, m->plnEntries[i].stop.min);
            entries[i].emplace("stop", buf);
        }
        reply.emplace("entries", entries);
    }
    auto grp = ctrller->GetGroups();
    vector<int> enabled_group;
    for (auto g : grp)
    {
        if (g->IsPlanEnabled(id))
        {
            enabled_group.emplace_back(g->GroupId());
        }
    }
    if (enabled_group.size() == 0)
    {
        enabled_group.emplace_back(0);
    }
    reply.emplace("enabled_group", enabled_group);
}

void WsServer::CMD_SetPlan(struct mg_connection *c, nlohmann::json &msg, json &reply)
{
    auto week = msg["week"].get<vector<string>>();
    uint8_t bweek = 0;
    for (int i = 0; i < 7; i++)
    {
        int d = Pick::PickStr(week[i].c_str(), WEEKDAY, countof(WEEKDAY), true);
        if (d < 0)
        {
            throw "'week' error";
        }
        else
        {
            bweek |= 1 << d;
        }
    }
    if (bweek == 0)
    {
        throw "'week' error";
    }
    auto entries = msg["entries"].get<vector<json>>();
    if (entries.size() == 0)
    {
        throw "There is no etry";
    }

    vector<uint8_t> cmd(entries.size() * 6 + 4 + (entries.size() == 6 ? 0 : 1));
    cmd.back() = 0;
    int id = GetInt(msg, "id", 1, 255);
    cmd[1] = id;
    cmd[2] = GetInt(msg, "revision", 0, 255);
    cmd[3] = bweek;
    uint8_t *p = cmd.data() + 4;
    auto GetHM = [](json &js, const char *str, uint8_t *p) -> uint8_t *
    {
        string s = js[str].get<string>();
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
        throw "Invalid time";
    };
    for (int i = 0; i < entries.size(); i++)
    {
        *p++ = GetInt(entries[i], "type", 1, 2);
        *p++ = GetInt(entries[i], "id", 1, 255);
        p = GetHM(entries[i], "start", p);
        p = GetHM(entries[i], "stop", p);
    }
    char rejectStr[64];
    auto r = ctrller->SignSetPlan(cmd.data(), cmd.size(), rejectStr);
    if (r != APP::ERROR::AppNoError)
    {
        reply.emplace("result", rejectStr);
        return;
    }

    vector<int> disable_g;
    vector<int> enable_g;
    auto enabled_group = msg["enabled_group"].get<vector<int>>();
    auto grp = ctrller->GetGroups();
    for (auto g : grp)
    {
        int gid = g->GroupId();
        if (Pick::PickInt(gid, enabled_group.data(), enabled_group.size()) == -1)
        {
            disable_g.emplace_back(gid);
        }
        else
        {
            enable_g.emplace_back(gid);
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

void WsServer::CMD_ResetFaultLog(struct mg_connection *c, json &msg, json &reply)
{
    DbHelper::Instance().GetUciFault().Reset();
    reply.emplace("result", "OK");
}

void WsServer::CMD_ResetAlarmLog(struct mg_connection *c, json &msg, json &reply)
{
    DbHelper::Instance().GetUciAlarm().Reset();
    reply.emplace("result", "OK");
}

void WsServer::CMD_ResetEventLog(struct mg_connection *c, json &msg, json &reply)
{
    DbHelper::Instance().GetUciEvent().Reset();
    reply.emplace("result", "OK");
}

void WsServer::CMD_SignTest(struct mg_connection *c, json &msg, json &reply)
{
    uint8_t cmd[5];
    cmd[0] = 0xFA;
    cmd[1] = 0x30;
    cmd[2] = GetInt(msg, "group_id", 1, ctrller->GroupCnt());
    cmd[3] = 255;
    cmd[4] = 255;
    auto p = DbHelper::Instance().GetUciProd();
    string colour = msg["colour"].get<string>();
    for (int i = 0; i < MONO_COLOUR_NAME_SIZE; i++)
    {
        if (strcasecmp(FrameColour::COLOUR_NAME[i], colour.c_str()) == 0)
        {
            cmd[3] = i;
        }
    }
    string pixels = msg["pixels"].get<string>();
    for (int i = 0; i < TEST_PIXELS_SIZE; i++)
    {
        if (strcasecmp(TestPixels[i], pixels.c_str()) == 0)
        {
            cmd[4] = i;
        }
    }
    APP::ERROR r = ctrller->CmdSignTest(cmd);
    reply.emplace("result", (r == APP::ERROR::AppNoError) ? "OK" : APP::ToStr(r));
}

void WsServer::CMD_DispAtomic(struct mg_connection *c, json &msg, json &reply)
{
    vector<json> content = msg["content"].get<vector<json>>();
    int len = content.size();
    unique_ptr<uint8_t[]> cmd(new uint8_t[3 + len * 2]);
    cmd[0] = 0x2B;
    cmd[1] = GetInt(msg, "group_id", 1, ctrller->GroupCnt());
    cmd[2] = len;
    for (int i = 0; i < len; i++)
    {
        json &x = content[i];
        cmd[3 + i * 2] = GetInt(msg, "sign_id", 1, ctrller->SignCnt());
        cmd[3 + i * 2 + 1] = GetInt(msg, "frame_id", 1, 255);
    }
    APP::ERROR r = ctrller->CmdDispAtomicFrm(cmd.get(), 3 + len * 2);
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

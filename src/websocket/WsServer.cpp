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
    #if 0
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
    #endif
}

void WsServer::CMD_ChangePassword(struct mg_connection *c, json &msg, json &reply)
{
    #if 0
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
#endif
}

void WsServer::CMD_GetGroupConfig(struct mg_connection *c, json &msg, json &reply)
{
    auto gs = ctrller->groups.size();
    reply.emplace("number_of_groups", gs);
    vector<json> groups(gs);
    for (int i = 0; i < gs; i++)
    {
        auto &g = ctrller->groups[i];
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
}

extern const char *FirmwareVer;
void WsServer::CMD_GetStatus(struct mg_connection *c, json &msg, json &reply)
{
    reply.emplace("manufacturer_code", DbHelper::Instance().GetUciProd().MfcCode());
    reply.emplace("firmware", FirmwareVer);
    reply.emplace("is_online", ctrller->IsOnline());
    reply.emplace("application_error", 0x00);
    char rtc[32];
    Utils::Time::ParseTimeToLocalStr(time(nullptr), rtc);
    reply.emplace("rtc", rtc);
    reply.emplace("hardware_checksum", 0x0000);
    reply.emplace("controller_error", 0x00);
    reply.emplace("max_temperature", 59);
    reply.emplace("current_temperature", 59);
    int group_cnt = ctrller->groups.size();
    vector<json> groups(group_cnt);
    for (int i = 0; i < group_cnt; i++)
    {
        auto &s = ctrller->groups[i];
        auto &v = groups[i];
        v.emplace("group_id", s->GroupId());
        v.emplace("device", s->IsDevice() ? "Enabled" : "Disabled");
        v.emplace("power", s->IsPower() ? "On" : "Off");
    }
    reply.emplace("groups", groups);

    int sign_cnt = ctrller->signs.size();
    vector<json> signs(sign_cnt);
    for (int i = 0; i < sign_cnt; i++)
    {
        auto &s = ctrller->signs[i];
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
}

void WsServer::CMD_GetUserConfig(struct mg_connection *c, json &msg, json &reply)
{
    auto &user = DbHelper::Instance().GetUciUser();
    char buf[8];
    sprintf(buf, "0x%02X", user.SeedOffset());
    reply.emplace("seed", buf);
    sprintf(buf, "0x%04X", user.PasswordOffset());
    reply.emplace("password", buf);
    reply.emplace("device_id", user.DeviceId());
    reply.emplace("broadcast_id", user.BroadcastId());
    reply.emplace("session_timeout", user.SessionTimeoutSec());
    reply.emplace("display_timeout", user.DisplayTimeoutMin());
    reply.emplace("tmc_com_port", COM_NAME[user.ComPort()]);
    reply.emplace("baudrate", user.Baudrate());
    reply.emplace("multiled_fault", user.MultiLedFaultThreshold());
    reply.emplace("tmc_tcp_port", user.SvcPort());
    reply.emplace("over_temp", user.OverTemp());
    reply.emplace("locked_frame", user.LockedFrm());
    reply.emplace("locked_msg", user.LockedMsg());
    reply.emplace("city", user.City());
    reply.emplace("last_frame_time", user.LastFrmTime());
}

void WsServer::CMD_SetUserConfig(struct mg_connection *c, json &msg, json &reply)
{
    try
    {
        unsigned char rr_flag = 0;
        auto device_id = GetInt(msg, "device_id", 0, 255);
        auto broadcast_id = GetInt(msg, "broadcast_id", 0, 255);
        if (device_id == broadcast_id)
        {
            throw string("device_id==broadcast_id");
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
        auto &user = DbHelper::Instance().GetUciUser();
        auto &evt = DbHelper::Instance().GetUciEvent();

        if (seed != user.SeedOffset())
        {
            evt.Push(0, "User.SeedOffset changed: 0x%02X->0x%02X", user.SeedOffset(), seed);
            user.SeedOffset(seed);
        }

        if (password != user.PasswordOffset())
        {
            evt.Push(0, "User.PasswordOffset changed: 0x%04X->0x%04X", user.PasswordOffset(), password);
            user.PasswordOffset(password);
        }

        if (device_id != user.DeviceId())
        {
            evt.Push(0, "User.DeviceID changed: %u->%u", user.DeviceId(), device_id);
            user.DeviceId(device_id);
        }

        if (broadcast_id != user.BroadcastId())
        {
            evt.Push(0, "User.BroadcastID changed: %u->%u", user.BroadcastId(), broadcast_id);
            user.BroadcastId(broadcast_id);
        }

        if (baudrate != user.Baudrate())
        {
            evt.Push(0, "User.Baudrate changed: %u->%u . Restart to load new setting",
                     user.Baudrate(), baudrate);
            user.Baudrate(baudrate);
            rr_flag |= RQST_RESTART;
        }

        if (tmc_tcp_port != user.SvcPort())
        {
            evt.Push(0, "User.SvcPort changed: %u->%u. Restart to load new setting",
                     user.SvcPort(), tmc_tcp_port);
            user.SvcPort(tmc_tcp_port);
            rr_flag |= RQST_RESTART;
        }

        if (session_timeout != user.SessionTimeoutSec())
        {
            evt.Push(0, "User.SessionTimeout changed: %u->%u",
                     user.SessionTimeoutSec(), session_timeout);
            user.SessionTimeoutSec(session_timeout);
        }

        if (display_timeout != user.DisplayTimeoutMin())
        {
            evt.Push(0, "User.DisplayTimeout changed: %u->%u",
                     user.DisplayTimeoutMin(), display_timeout);
            user.DisplayTimeoutMin(display_timeout);
        }

        if (over_temp != user.OverTemp())
        {
            evt.Push(0, "User.OverTemp changed: %u->%u",
                     user.OverTemp(), over_temp);
            user.OverTemp(over_temp);
        }

        if (city != user.CityId())
        {
            evt.Push(0, "User.City changed: %s->%s",
                     Tz_AU::tz_au[user.CityId()].city, Tz_AU::tz_au[city].city);
            user.CityId(city);
            rr_flag |= RQST_RESTART;
        }

        if (multiled_fault != user.MultiLedFaultThreshold())
        {
            evt.Push(0, "User.MultiLedFaultThreshold changed: %u->%u",
                     user.MultiLedFaultThreshold(), multiled_fault);
            user.MultiLedFaultThreshold(multiled_fault);
        }

        if (locked_msg != user.LockedMsg())
        {
            evt.Push(0, "User.LockedMsg changed: %u->%u",
                     user.LockedMsg(), locked_msg);
            user.LockedMsg(locked_msg);
        }

        if (locked_frame != user.LockedFrm())
        {
            evt.Push(0, "User.LockedFrm changed: %u->%u",
                     user.LockedFrm(), locked_frame);
            user.LockedFrm(locked_frame);
        }

        if (last_frame_time != user.LastFrmTime())
        {
            evt.Push(0, "User.LastFrmTime changed: %u->%u",
                     user.LastFrmTime(), last_frame_time);
            user.LastFrmTime(last_frame_time);
        }
        reply.emplace("result", (rr_flag != 0) ? "'Reboot' to active new setting" : "OK");
    }
    catch (const string &e)
    {
        reply.emplace("result", e);
    }
}

void WsServer::CMD_GetDimmingConfig(struct mg_connection *c, json &msg, json &reply)
{
    auto &user = DbHelper::Instance().GetUciUser();
    reply.emplace("night_level", user.NightDimmingLevel());
    reply.emplace("dawn_dusk_level", user.DawnDimmingLevel());
    reply.emplace("day_level", user.DayDimmingLevel());
    reply.emplace("night_max_lux", user.LuxNightMax());
    reply.emplace("day_min_lux", user.LuxDayMin());
    reply.emplace("18_hours_min_lux", user.Lux18HoursMin());
}

void WsServer::CMD_SetDimmingConfig(struct mg_connection *c, json &msg, json &reply)
{
    try
    {
        auto night_level = GetInt(msg, "night_level", 1, 8);
        auto dawn_dusk_level = GetInt(msg, "dawn_dusk_level", night_level + 1, 15);
        auto day_level = GetInt(msg, "day_level", dawn_dusk_level + 1, 16);
        auto night_max_lux = GetInt(msg, "night_max_lux", 1, 9999);
        auto day_min_lux = GetInt(msg, "day_min_lux", night_max_lux + 1, 65535);
        auto _18_hours_min_lux = GetInt(msg, "18_hours_min_lux", day_min_lux + 1, 65535);
        auto &user = DbHelper::Instance().GetUciUser();
        auto &evt = DbHelper::Instance().GetUciEvent();
        if (night_level != user.NightDimmingLevel())
        {
            evt.Push(0, "User.NightDimmingLevel changed: %d->%d", user.NightDimmingLevel(), night_level);
            user.NightDimmingLevel(night_level);
        }
        if (dawn_dusk_level != user.DawnDimmingLevel())
        {
            evt.Push(0, "User.DawnDimmingLevel changed: %d->%d", user.DawnDimmingLevel(), dawn_dusk_level);
            user.DawnDimmingLevel(dawn_dusk_level);
        }
        if (day_level != user.DayDimmingLevel())
        {
            evt.Push(0, "User.DayDimmingLevel changed: %d->%d", user.DayDimmingLevel(), day_level);
            user.DayDimmingLevel(day_level);
        }
        if (night_max_lux != user.LuxNightMax())
        {
            evt.Push(0, "User.LuxNightMax changed: %d->%d", user.LuxNightMax(), night_max_lux);
            user.LuxNightMax(night_max_lux);
        }
        if (day_min_lux != user.LuxDayMin())
        {
            evt.Push(0, "User.LuxDayMin changed: %d->%d", user.LuxDayMin(), day_min_lux);
            user.LuxDayMin(day_min_lux);
        }
        if (_18_hours_min_lux != user.Lux18HoursMin())
        {
            evt.Push(0, "User.Lux18HoursMin changed: %d->%d", user.Lux18HoursMin(), _18_hours_min_lux);
            user.Lux18HoursMin(_18_hours_min_lux);
        }
        reply.emplace("result", "OK");
    }
    catch (const string &e)
    {
        reply.emplace("result", e);
    }
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
            interfaces[i].netmask.Set(ethjs[i][net._Netmask].get<std::string>());
            {
                throw "Invalid " + ethname[i] + ".netmask";
            }
            try
            {
                interfaces[i].gateway.Set(ethjs[i][net._Gateway].get<std::string>());
                interfaces[i].dns = ethjs[i][net._Dns].get<std::string>();
            }
            catch (...)
            {
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
}

void WsServer::CMD_ControlPower(struct mg_connection *c, json &msg, json &reply)
{
    auto grp = msg["groups"].get<vector<int>>();
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
}

void WsServer::CMD_ControlDevice(struct mg_connection *c, json &msg, json &reply)
{
    auto grp = msg["groups"].get<vector<int>>();
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
}

void WsServer::CMD_SystemReset(struct mg_connection *c, json &msg, json &reply)
{
    uint8_t cmd[3];
    cmd[1] = msg["group_id"].get<int>();
    cmd[2] = msg["level"].get<int>();
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
    auto id = msg["id"].get<int>();
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
    str = msg["colour"].get<std::string>();
    int colour = Pick::PickStr(str.c_str(), FrameColour::COLOUR_NAME, ALL_COLOUR_NAME_SIZE, true);
    if (colour < 0)
    {
        throw string("CMD_SetFrame: colour error");
    }
    if (ftype == 0 && colour > 9)
    {
        throw string("CMD_SetFrame: colour error(Graphics Frame)");
    }
    if (ftype == 1 && colour > 10)
    {
        throw string("CMD_SetFrame: colour error(Text Frame)");
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
    if (ftype == 0)
    {
        str = msg["text"].get<std::string>();
        if (str.length() < 1)
        {
            throw "'Text' is null";
        }
        frm.resize(str.length() + 9);
        frm[0] = static_cast<uint8_t>(MI::CODE::SignSetTextFrame);
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
        {
            corelen = prod.Gfx1CoreLen();
        }
        if (colour == 10)
        {
            colour = 0x0D;
            corelen = prod.Gfx4CoreLen();
        }
        else if (colour == 11)
        {
            colour = 0x0E;
            corelen = prod.Gfx24CoreLen();
        }
        int f_offset;
        if (rows < 255 && columns < 255)
        {
            f_offset = 9;
            frm.resize(corelen + f_offset + 2);
            frm[0] = static_cast<uint8_t>(MI::CODE::SignSetGraphicsFrame);
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
            frm[0] = static_cast<uint8_t>(MI::CODE::SignSetHighResolutionGraphicsFrame);
            Cnvt::PutU16(rows, frm.data() + 3);
            Cnvt::PutU16(columns, frm.data() + 5);
            frm[7] = colour;
            frm[8] = 0;
            Utils::Cnvt::PutU32(corelen, frm.data() + 9);
        }
        frm[1] = 255;
        frm[2] = 255;
        int bitOffset = 0;
        if (colour <= 9)
        {
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
        else if (colour == 0x0D)
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
        else if (colour == 0x0E)
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
    cmd[1] = msg["group_id"].get<int>();
    cmd[2] = msg["frame_id"].get<int>();
    auto r = ctrller->CmdDispFrm(cmd);
    reply.emplace("result", (r == APP::ERROR::AppNoError) ? "OK" : APP::ToStr(r));
}

void WsServer::CMD_GetStoredMessage(struct mg_connection *c, json &msg, json &reply)
{
}

void WsServer::CMD_SetMessage(struct mg_connection *c, nlohmann::json &msg, json &reply)
{
}

void WsServer::CMD_DisplayMessage(struct mg_connection *c, nlohmann::json &msg, json &reply)
{
    uint8_t cmd[3];
    cmd[1] = msg["group_id"].get<int>();
    cmd[2] = msg["message_id"].get<int>();
    auto r = ctrller->CmdDispMsg(cmd);
    reply.emplace("result", (r == APP::ERROR::AppNoError) ? "OK" : APP::ToStr(r));
}

void WsServer::CMD_GetStoredPlan(struct mg_connection *c, json &msg, json &reply)
{
}

void WsServer::CMD_SetPlan(struct mg_connection *c, nlohmann::json &msg, json &reply)
{
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
    cmd[2] = msg["group_id"].get<int>();
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
    reply.emplace("result", (r == APP::ERROR::AppNoError) ? "OK" : APP::ToStr(static_cast<uint8_t>(r)));
}

void WsServer::CMD_DispAtomic(struct mg_connection *c, json &msg, json &reply)
{
    vector<json> content = msg["content"].get<vector<json>>();
    int len = content.size();
    unique_ptr<uint8_t[]> cmd(new uint8_t[3 + len * 2]);
    cmd[0] = 0x2B;
    cmd[1] = msg["group_id"].get<int>();
    cmd[2] = len;
    for (int i = 0; i < len; i++)
    {
        json &x = content[i];
        cmd[3 + i * 2] = x["sign_id"].get<int>();
        cmd[3 + i * 2 + 1] = x["frame_id"].get<int>();
    }
    APP::ERROR r = ctrller->CmdDispAtomicFrm(cmd.get(), 3 + len * 2);
    reply.emplace("result", (r == APP::ERROR::AppNoError) ? "OK" : APP::ToStr(static_cast<uint8_t>(r)));
}
